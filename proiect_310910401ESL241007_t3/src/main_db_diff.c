#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "db.h"

// ===== Load file DB =====
file_record_t *load_file_db(const char *path, db_header_t *hdr_out) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    read(fd, hdr_out, sizeof(db_header_t));

    file_record_t *arr = malloc(hdr_out->record_count * sizeof(file_record_t));

    for (uint64_t i = 0; i < hdr_out->record_count; i++) {
        read(fd, &arr[i], sizeof(file_record_t));
    }

    close(fd);
    return arr;
}

// ===== Load proc DB =====
proc_record_t *load_proc_db(const char *path, db_header_t *hdr_out) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    read(fd, hdr_out, sizeof(db_header_t));

    proc_record_t *arr = malloc(hdr_out->record_count * sizeof(proc_record_t));

    for (uint64_t i = 0; i < hdr_out->record_count; i++) {
        read(fd, &arr[i], sizeof(proc_record_t));
    }

    close(fd);
    return arr;
}

// ===== Find helpers =====
int find_file(file_record_t *arr, uint64_t n, const char *path) {
    for (uint64_t i = 0; i < n; i++) {
        if (strcmp(arr[i].path, path) == 0)
            return i;
    }
    return -1;
}

int find_proc(proc_record_t *arr, uint64_t n, int pid) {
    for (uint64_t i = 0; i < n; i++) {
        if (arr[i].pid == pid)
            return i;
    }
    return -1;
}

// ===== Compare file DB =====
void diff_files(file_record_t *old, uint64_t n_old,
                file_record_t *new, uint64_t n_new,
                FILE *out) {

    // Appeared
    for (uint64_t i = 0; i < n_new; i++) {
        if (find_file(old, n_old, new[i].path) < 0) {
            fprintf(out, "ADDED: %s\n", new[i].path);
        }
    }

    // Disappeared
    for (uint64_t i = 0; i < n_old; i++) {
        if (find_file(new, n_new, old[i].path) < 0) {
            fprintf(out, "REMOVED: %s\n", old[i].path);
        }
    }

    // Modified
    for (uint64_t i = 0; i < n_new; i++) {
        int j = find_file(old, n_old, new[i].path);
        if (j >= 0) {
            if (old[j].size != new[i].size ||
                old[j].mtime != new[i].mtime ||
                old[j].hash != new[i].hash ||
                old[j].type != new[i].type) {

                fprintf(out, "MODIFIED: %s\n", new[i].path);
            }
        }
    }
}

// ===== Compare proc DB =====
void diff_proc(proc_record_t *old, uint64_t n_old,
               proc_record_t *new, uint64_t n_new,
               FILE *out) {

    // Appeared
    for (uint64_t i = 0; i < n_new; i++) {
        if (find_proc(old, n_old, new[i].pid) < 0) {
            fprintf(out, "ADDED PID: %d\n", new[i].pid);
        }
    }

    // Disappeared
    for (uint64_t i = 0; i < n_old; i++) {
        if (find_proc(new, n_new, old[i].pid) < 0) {
            fprintf(out, "REMOVED PID: %d\n", old[i].pid);
        }
    }

    // Modified (RSS threshold example)
    for (uint64_t i = 0; i < n_new; i++) {
        int j = find_proc(old, n_old, new[i].pid);
        if (j >= 0) {
            if (llabs((long long)new[i].rss - (long long)old[j].rss) > 100) {
                fprintf(out, "CHANGED PID: %d (RSS diff)\n", new[i].pid);
            }
        }
    }
}

// ===== MAIN =====
int main(int argc, char *argv[]) {
    const char *old_path = NULL;
    const char *new_path = NULL;
    const char *out_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--old") == 0) old_path = argv[++i];
        else if (strcmp(argv[i], "--new") == 0) new_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0) out_path = argv[++i];
    }

    if (!old_path || !new_path || !out_path) {
        fprintf(stderr, "Usage: db_diff --old <db1> --new <db2> --out <file>\n");
        return 1;
    }

    db_header_t h1, h2;

    int fd1 = open(old_path, O_RDONLY);
    int fd2 = open(new_path, O_RDONLY);

    read(fd1, &h1, sizeof(h1));
    read(fd2, &h2, sizeof(h2));

    close(fd1);
    close(fd2);

    // Validate type
    if (memcmp(h1.magic, h2.magic, 4) != 0 || h1.version != h2.version) {
        fprintf(stderr, "DB type/version mismatch\n");
        return 1;
    }

    FILE *out = fopen(out_path, "w");

    if (memcmp(h1.magic, MAGIC_IDX, 4) == 0) {
        file_record_t *old = load_file_db(old_path, &h1);
        file_record_t *new = load_file_db(new_path, &h2);

        diff_files(old, h1.record_count, new, h2.record_count, out);

        free(old);
        free(new);

    } else {
        proc_record_t *old = load_proc_db(old_path, &h1);
        proc_record_t *new = load_proc_db(new_path, &h2);

        diff_proc(old, h1.record_count, new, h2.record_count, out);

        free(old);
        free(new);
    }

    fclose(out);
    return 0;
}

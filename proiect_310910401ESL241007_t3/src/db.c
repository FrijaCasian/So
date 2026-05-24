#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <inttypes.h>

#include "db.h"
#include "lock.h"

// ===== Helpers =====

static void read_header(int fd, db_header_t *hdr) {
    lseek(fd, 0, SEEK_SET);
    if (read(fd, hdr, sizeof(db_header_t)) != sizeof(db_header_t)) {
        perror("read header");
        exit(1);
    }
}

static void write_header(int fd, db_header_t *hdr) {
    lseek(fd, 0, SEEK_SET);
    if (write(fd, hdr, sizeof(db_header_t)) != sizeof(db_header_t)) {
        perror("write header");
        exit(1);
    }
}

static void read_record(int fd, off_t index, file_record_t *rec) {
    off_t offset = sizeof(db_header_t) + index * sizeof(file_record_t);
    lseek(fd, offset, SEEK_SET);
    if (read(fd, rec, sizeof(file_record_t)) != sizeof(file_record_t)) {
        perror("read record");
        exit(1);
    }
}

static void write_record(int fd, off_t index, file_record_t *rec) {
    off_t offset = sizeof(db_header_t) + index * sizeof(file_record_t);

    lock_region(fd, offset, sizeof(file_record_t));

    lseek(fd, offset, SEEK_SET);
    if (write(fd, rec, sizeof(file_record_t)) != sizeof(file_record_t)) {
        perror("write record");
        exit(1);
    }

    unlock_region(fd, offset, sizeof(file_record_t));
}

static int find_record_by_path(int fd, const char *path) {
    db_header_t hdr;
    read_header(fd, &hdr);

    for (uint64_t i = 0; i < hdr.record_count; i++) {
        file_record_t rec;
        read_record(fd, i, &rec);

        if (strcmp(rec.path, path) == 0) {
            return i;
        }
    }

    return -1;
}

// ===== DB lifecycle =====

int open_or_init_db(const char *path, const char *magic) {
    int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        exit(1);
    }

    db_header_t hdr;

    lock_region(fd, 0, sizeof(db_header_t));

    if (st.st_size == 0) {
        memset(&hdr, 0, sizeof(hdr));

        memcpy(hdr.magic, magic, 4);
        hdr.version = 1;
        hdr.snapshot_id = (uint64_t)time(NULL);
        hdr.snapshot_state = SNAPSHOT_OPEN;
        hdr.active_writers = 1;
        hdr.record_count = 0;

        write_header(fd, &hdr);

        printf("[INIT] snapshot_id=%" PRIu64 "\n", hdr.snapshot_id);

    } else {
        read_header(fd, &hdr);

        if (memcmp(hdr.magic, magic, 4) != 0) {
            fprintf(stderr, "Invalid DB type\n");
            unlock_region(fd, 0, sizeof(db_header_t));
            close(fd);
            exit(1);
        }

        if (hdr.snapshot_state == SNAPSHOT_SEALED) {
            fprintf(stderr, "Snapshot SEALED\n");
            unlock_region(fd, 0, sizeof(db_header_t));
            close(fd);
            exit(1);
        }

        hdr.active_writers++;
        write_header(fd, &hdr);

        printf("[ATTACH] snapshot_id=%" PRIu64 ", writers=%u\n",
               hdr.snapshot_id, hdr.active_writers);
    }

    unlock_region(fd, 0, sizeof(db_header_t));
    return fd;
}

void close_db(int fd) {
    db_header_t hdr;

    lock_region(fd, 0, sizeof(db_header_t));

    read_header(fd, &hdr);

    hdr.active_writers--;

    if (hdr.active_writers == 0) {
        hdr.snapshot_state = SNAPSHOT_SEALED;
        printf("[SEALED] snapshot_id=%" PRIu64 "\n", hdr.snapshot_id);
    }

    write_header(fd, &hdr);

    unlock_region(fd, 0, sizeof(db_header_t));
    close(fd);
}

// ===== Record logic =====

void upsert_file_record(int fd, file_record_t *rec) {
    // First try to find existing record
    int index = find_record_by_path(fd, rec->path);

    if (index >= 0) {
        // UPDATE
        write_record(fd, index, rec);
        return;
    }

    // INSERT
    db_header_t hdr;

    lock_region(fd, 0, sizeof(db_header_t));

    read_header(fd, &hdr);


    int again = find_record_by_path(fd, rec->path);
    if (again >= 0) {
        unlock_region(fd, 0, sizeof(db_header_t));
        write_record(fd, again, rec);
        return;
    }

    off_t new_index = hdr.record_count;
    hdr.record_count++;

    write_header(fd, &hdr);

    unlock_region(fd, 0, sizeof(db_header_t));

    write_record(fd, new_index, rec);
}
static int find_record_by_pid(int fd, int pid) {
    db_header_t hdr;
    read_header(fd, &hdr);

    for (uint64_t i = 0; i < hdr.record_count; i++) {
        proc_record_t rec;
        off_t offset = sizeof(db_header_t) + i * sizeof(proc_record_t);

        lseek(fd, offset, SEEK_SET);
        if (read(fd, &rec, sizeof(rec)) != sizeof(rec))
            continue;

        if (rec.pid == pid)
            return i;
    }

    return -1;
}

void upsert_proc_record(int fd, proc_record_t *rec) {
    int index = find_record_by_pid(fd, rec->pid);

    if (index >= 0) {
        off_t offset = sizeof(db_header_t) + index * sizeof(proc_record_t);

        lock_region(fd, offset, sizeof(proc_record_t));
        lseek(fd, offset, SEEK_SET);
        write(fd, rec, sizeof(proc_record_t));
        unlock_region(fd, offset, sizeof(proc_record_t));
        return;
    }

    db_header_t hdr;

    lock_region(fd, 0, sizeof(db_header_t));
    read_header(fd, &hdr);

    int again = find_record_by_pid(fd, rec->pid);
    if (again >= 0) {
        unlock_region(fd, 0, sizeof(db_header_t));
        upsert_proc_record(fd, rec);
        return;
    }

    off_t new_index = hdr.record_count;
    hdr.record_count++;

    write_header(fd, &hdr);
    unlock_region(fd, 0, sizeof(db_header_t));

    off_t offset = sizeof(db_header_t) + new_index * sizeof(proc_record_t);

    lock_region(fd, offset, sizeof(proc_record_t));
    lseek(fd, offset, SEEK_SET);
    write(fd, rec, sizeof(proc_record_t));
    unlock_region(fd, offset, sizeof(proc_record_t));
}

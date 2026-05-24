#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "db.h"

// ===== Simple hash (fast + deterministic) =====
uint64_t simple_hash(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    uint64_t hash = 5381;
    int c;

    while ((c = fgetc(f)) != EOF) {
        hash = ((hash << 5) + hash) + c; // djb2
    }

    fclose(f);
    return hash;
}

// ===== Build record from stat =====
void build_record(const char *path, struct stat *st, file_record_t *rec) {
    memset(rec, 0, sizeof(*rec));

    strncpy(rec->path, path, MAX_PATH - 1);
    rec->size = 0;
    rec->mtime = st->st_mtime;
    rec->inode = st->st_ino;
    rec->dev = st->st_dev;

    if (S_ISREG(st->st_mode)) {
        rec->type = 1;
        rec->size = st->st_size;
        rec->hash = simple_hash(path);
    } else if (S_ISDIR(st->st_mode)) {
        rec->type = 2;
    } else if (S_ISLNK(st->st_mode)) {
        rec->type = 3;
    } else {
        rec->type = 0;
    }
}

// ===== Recursive traversal =====
void traverse(int fd, const char *path) {
    struct stat st;

    if (lstat(path, &st) < 0) {
        perror("lstat");
        return;
    }

    // Build + insert record
    file_record_t rec;
    build_record(path, &st, &rec);
    upsert_file_record(fd, &rec);

    // If directory → recurse
    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) return;

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            traverse(fd, full_path);
        }

        closedir(dir);
    }
}

// ===== MAIN =====
int main(int argc, char *argv[]) {
    const char *root = NULL;
    const char *db_path = "data/index.db";

    // Simple arg parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--root") == 0 && i + 1 < argc) {
            root = argv[++i];
        } else if (strcmp(argv[i], "--db") == 0 && i + 1 < argc) {
            db_path = argv[++i];
        }
    }

    if (!root) {
        fprintf(stderr, "Usage: fileops_indexer --root <dir> [--db <path>]\n");
        return 1;
    }

    int fd = open_or_init_db(db_path, MAGIC_IDX);

    traverse(fd, root);

    close_db(fd);
    return 0;
}

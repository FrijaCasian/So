#ifndef DB_H
#define DB_H

#include <stdint.h>
#include "ipc.h"

typedef struct {
    char magic[4];
    uint32_t version;
    uint32_t complete;

    uint32_t file_record_count;
    uint32_t worker_count;
} DBHeader;

int write_database(
    const char *path,
    FileRecord *records,
    uint32_t record_count,
    WorkerStats *stats,
    uint32_t worker_count
);

int verify_database(const char *path);
int dump_database(const char *path);

#endif

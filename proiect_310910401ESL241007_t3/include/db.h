#ifndef DB_H
#define DB_H

#include <stdint.h>

#define MAGIC_IDX "IDX1"
#define MAGIC_PRC "PRC1"

#define SNAPSHOT_OPEN 0
#define SNAPSHOT_SEALED 1

#define MAX_PATH 512

#define MAX_COMM 64
#define MAX_CMD 256

typedef struct {
    int pid;
    int ppid;
    char state;
    char comm[MAX_COMM];
    char cmdline[MAX_CMD];
    uint64_t rss;
    uint64_t cpu_time;
} proc_record_t;

typedef struct {
    char magic[4];
    uint32_t version;
    uint64_t snapshot_id;
    uint32_t snapshot_state;
    uint32_t active_writers;
    uint64_t record_count;
} db_header_t;


typedef struct {
    char path[MAX_PATH];
    uint8_t type;
    uint64_t size;
    uint64_t mtime;
    uint64_t hash;
    uint64_t inode;
    uint64_t dev;
} file_record_t;


int open_or_init_db(const char *path, const char *magic);
void close_db(int fd);
void upsert_file_record(int fd, file_record_t *rec);
void upsert_proc_record(int fd, proc_record_t *rec);

#endif

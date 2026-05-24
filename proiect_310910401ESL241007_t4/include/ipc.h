#ifndef IPC_H
#define IPC_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>

#include "config.h"

#define IPC_MAGIC "IPC4"
#define IPC_VERSION 1

#define DB_MAGIC "INV4"
#define DB_VERSION 1

typedef struct {
    char path[PATH_SIZE];
    int depth;
} Job;

typedef struct {
    char path[PATH_SIZE];
    uint64_t size;
    uint64_t mtime;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    unsigned char sha256[32];
} FileRecord;

typedef struct {
    int worker_id;
    pid_t pid;
    int exit_status;

    uint64_t jobs_processed;
    uint64_t files_emitted;
    uint64_t bytes_emitted;

    uint64_t wall_time_ms;
    uint64_t user_cpu_us;
    uint64_t sys_cpu_us;
} WorkerStats;

typedef struct {
    char magic[4];
    uint32_t version;

    int worker_count;
    int max_depth;
    int simulate_work_ms;

    volatile int queued_jobs;
    volatile int active_workers;
    volatile int shutdown;

    int job_head;
    int job_tail;

    int result_head;
    int result_tail;
} IPCHeader;

typedef struct {
    IPCHeader header;

    sem_t job_mutex;
    sem_t job_items;
    sem_t job_slots;

    sem_t result_mutex;
    sem_t result_items;
    sem_t result_slots;

    sem_t stats_mutex;

    Job jobs[MAX_JOBS];
    FileRecord results[MAX_RESULTS];

    WorkerStats stats[MAX_WORKERS];
} SharedIPC;

int ipc_create(const char *path, size_t size);
SharedIPC *ipc_map(const char *path, int create);
void ipc_init(SharedIPC *ipc, int workers, int max_depth, int simulate_ms);

#endif

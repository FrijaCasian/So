#include "ipc.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Calculate size needed for the shared IPC region
static size_t calculate_ipc_size(void) {
    return sizeof(SharedIPC);
}

// Create the mmap file with proper size
int ipc_create(const char *path, size_t size) {
    // Unlink old file if it exists
    unlink(path);

    int fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    // Extend file to desired size by seeking and writing
    if (lseek(fd, size - 1, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        return -1;
    }

    if (write(fd, "", 1) < 0) {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

// Map the IPC file into memory
SharedIPC *ipc_map(const char *path, int create) {
    size_t size = calculate_ipc_size();

    if (create) {
        if (ipc_create(path, size) < 0) {
            return NULL;
        }
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    SharedIPC *ipc = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    close(fd);

    if (ipc == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    return ipc;
}

// Initialize shared IPC structures and semaphores
void ipc_init(
    SharedIPC *ipc,
    int workers,
    int max_depth,
    int simulate_ms
) {
    // Initialize header
    memcpy(ipc->header.magic, IPC_MAGIC, 4);
    ipc->header.version = IPC_VERSION;

    ipc->header.worker_count = workers;
    ipc->header.max_depth = max_depth;
    ipc->header.simulate_work_ms = simulate_ms;

    ipc->header.queued_jobs = 0;
    ipc->header.active_workers = 0;
    ipc->header.shutdown = 0;

    ipc->header.job_head = 0;
    ipc->header.job_tail = 0;

    ipc->header.result_head = 0;
    ipc->header.result_tail = 0;

    // Initialize job queue semaphores (process-shared)
    sem_init(&ipc->job_mutex, 1, 1);
    sem_init(&ipc->job_items, 1, 0);
    sem_init(&ipc->job_slots, 1, MAX_JOBS);

    // Initialize result queue semaphores (process-shared)
    sem_init(&ipc->result_mutex, 1, 1);
    sem_init(&ipc->result_items, 1, 0);
    sem_init(&ipc->result_slots, 1, MAX_RESULTS);

    // Initialize stats semaphore (process-shared)
    sem_init(&ipc->stats_mutex, 1, 1);

    // Initialize worker stats array
    for (int i = 0; i < MAX_WORKERS; i++) {
        ipc->stats[i].worker_id = -1;
        ipc->stats[i].pid = 0;
        ipc->stats[i].exit_status = 0;
        ipc->stats[i].jobs_processed = 0;
        ipc->stats[i].files_emitted = 0;
        ipc->stats[i].bytes_emitted = 0;
        ipc->stats[i].wall_time_ms = 0;
        ipc->stats[i].user_cpu_us = 0;
        ipc->stats[i].sys_cpu_us = 0;
    }
}

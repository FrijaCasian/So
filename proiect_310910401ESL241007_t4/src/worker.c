#include "worker.h"
#include "ipc.h"
#include "queue.h"
#include "scan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

int worker_main(int argc, char **argv) {
    int worker_id = -1;
    const char *ipc_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--worker-id") == 0) {
            worker_id = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--ipc") == 0) {
            ipc_path = argv[++i];
        }
    }

    if (worker_id < 0 || !ipc_path) {
        return 1;
    }

    SharedIPC *ipc = ipc_map(ipc_path, 0);

    if (!ipc) {
        return 1;
    }

    ipc->stats[worker_id].worker_id = worker_id;
    ipc->stats[worker_id].pid = getpid();

    struct timeval start;
    gettimeofday(&start, NULL);

    while (1) {
        if (ipc->header.shutdown) {
            break;
        }

        Job job;

        if (dequeue_job(ipc, &job) != 0) {
            break;
        }

        __sync_fetch_and_add(&ipc->header.active_workers, 1);

        process_directory(ipc, worker_id, job.path, job.depth);

        sem_wait(&ipc->stats_mutex);
        ipc->stats[worker_id].jobs_processed++;
        sem_post(&ipc->stats_mutex);

        __sync_fetch_and_sub(&ipc->header.active_workers, 1);

        if (ipc->header.queued_jobs == 0 &&
            ipc->header.active_workers == 0) {

            ipc->header.shutdown = 1;

            for (int i = 0; i < ipc->header.worker_count; i++) {
                sem_post(&ipc->job_items);
            }
        }
    }

    struct timeval end;
    gettimeofday(&end, NULL);

    uint64_t elapsed =
        (end.tv_sec - start.tv_sec) * 1000ULL +
        (end.tv_usec - start.tv_usec) / 1000ULL;

    ipc->stats[worker_id].wall_time_ms = elapsed;

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    ipc->stats[worker_id].user_cpu_us =
        usage.ru_utime.tv_sec * 1000000ULL +
        usage.ru_utime.tv_usec;

    ipc->stats[worker_id].sys_cpu_us =
        usage.ru_stime.tv_sec * 1000000ULL +
        usage.ru_stime.tv_usec;

    return 0;
}

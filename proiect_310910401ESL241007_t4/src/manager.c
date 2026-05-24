#include "ipc.h"
#include "queue.h"
#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

int manager_inventory_mode(int argc, char **argv)
{
    const char *root = NULL;
    const char *ipc_path = "data/ipc.mmap";
    const char *db_path = "data/inventory.db";

    int workers = 1;
    int max_depth = -1;
    int simulate_ms = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--root") == 0)
            root = argv[++i];

        else if (strcmp(argv[i], "--workers") == 0)
            workers = atoi(argv[++i]);

        else if (strcmp(argv[i], "--ipc") == 0)
            ipc_path = argv[++i];

        else if (strcmp(argv[i], "--db") == 0)
            db_path = argv[++i];

        else if (strcmp(argv[i], "--max-depth") == 0)
            max_depth = atoi(argv[++i]);

        else if (strcmp(argv[i], "--simulate-work-ms") == 0)
            simulate_ms = atoi(argv[++i]);
    }

    if (!root) {
        fprintf(stderr, "missing root\n");
        return 1;
    }

    struct stat st;

    if (stat(root, &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "invalid root\n");
        return 1;
    }

    SharedIPC *ipc = ipc_map(ipc_path, 1);

    if (!ipc) {
        return 1;
    }

    ipc_init(ipc, workers, max_depth, simulate_ms);

    Job root_job;

    memset(&root_job, 0, sizeof(root_job));

    realpath(root, root_job.path);

    root_job.depth = 0;

    enqueue_job(ipc, &root_job);

    pid_t pids[MAX_WORKERS];

    for (int i = 0; i < workers; i++) {

        pid_t pid = fork();

        if (pid == 0) {

            char id_buf[32];

            snprintf(id_buf, sizeof(id_buf), "%d", i);

            execl(
                "./bin/fileops_worker",
                "fileops_worker",
                "--worker-id",
                id_buf,
                "--ipc",
                ipc_path,
                NULL
            );

            perror("execl");
            exit(1);
        }

        pids[i] = pid;
    }

    FileRecord *records = NULL;

    uint32_t record_count = 0;

    while (!ipc->header.shutdown ||
           ipc->header.active_workers > 0 ||
           ipc->header.queued_jobs > 0) {

        FileRecord rec;

        if (dequeue_result(ipc, &rec) == 0) {

            records = realloc(
                records,
                sizeof(FileRecord) * (record_count + 1)
            );

            records[record_count++] = rec;
        }
    }

    for (int i = 0; i < workers; i++) {

        int status;

        waitpid(pids[i], &status, 0);

        ipc->stats[i].exit_status = status;
    }

    write_database(
        db_path,
        records,
        record_count,
        ipc->stats,
        workers
    );

    free(records);

    return 0;
}

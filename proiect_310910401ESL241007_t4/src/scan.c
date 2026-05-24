#include "scan.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <semaphore.h>

int process_directory(
    SharedIPC *ipc,
    int worker_id,
    const char *path,
    int depth
) {
    DIR *dir = opendir(path);

    if (!dir) {
        return -1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full[PATH_MAX];

        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);

        struct stat st;

        if (lstat(full, &st) < 0) {
            continue;
        }

        if (S_ISLNK(st.st_mode)) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (ipc->header.max_depth < 0 ||
                depth < ipc->header.max_depth) {

                Job job;
                memset(&job, 0, sizeof(job));

                realpath(full, job.path);
                job.depth = depth + 1;

                enqueue_job(ipc, &job);
            }
        }

        if (S_ISREG(st.st_mode)) {
            FileRecord rec;
            memset(&rec, 0, sizeof(rec));

            realpath(full, rec.path);

            rec.size = st.st_size;
            rec.mtime = st.st_mtime;
            rec.mode = st.st_mode;
            rec.uid = st.st_uid;
            rec.gid = st.st_gid;

            compute_sha256(full, rec.sha256);

            enqueue_result(ipc, &rec);

            sem_wait(&ipc->stats_mutex);

            ipc->stats[worker_id].files_emitted++;
            ipc->stats[worker_id].bytes_emitted += st.st_size;

            sem_post(&ipc->stats_mutex);
        }
    }

    closedir(dir);

    return 0;
}

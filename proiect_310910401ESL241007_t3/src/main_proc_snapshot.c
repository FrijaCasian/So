#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

#include "db.h"

int is_number(const char *s) {
    for (; *s; s++) {
        if (!isdigit(*s)) return 0;
    }
    return 1;
}

// Read /proc/[pid]/stat
int read_stat(int pid, proc_record_t *rec) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    // simplified parsing
    fscanf(f, "%d (%[^)]) %c %d",
           &rec->pid,
           rec->comm,
           &rec->state,
           &rec->ppid);

    // skip many fields until utime, stime
    long utime, stime;
    for (int i = 0; i < 11; i++) fscanf(f, "%*s");
    fscanf(f, "%ld %ld", &utime, &stime);

    rec->cpu_time = utime + stime;

    // skip to RSS
    long rss;
    for (int i = 0; i < 7; i++) fscanf(f, "%*s");
    fscanf(f, "%ld", &rss);

    rec->rss = rss;

    fclose(f);
    return 0;
}

// Read cmdline
void read_cmdline(int pid, proc_record_t *rec) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    FILE *f = fopen(path, "r");
    if (!f) return;

    size_t n = fread(rec->cmdline, 1, MAX_CMD - 1, f);
    rec->cmdline[n] = '\0';

    fclose(f);
}

int main(int argc, char *argv[]) {
    const char *db_path = "data/proc.db";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--db") == 0 && i + 1 < argc) {
            db_path = argv[++i];
        }
    }

    int fd = open_or_init_db(db_path, MAGIC_PRC);

    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("opendir /proc");
        return 1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;

        int pid = atoi(entry->d_name);

        proc_record_t rec;
        memset(&rec, 0, sizeof(rec));

        if (read_stat(pid, &rec) < 0) continue;

        read_cmdline(pid, &rec);

        upsert_proc_record(fd, &rec);
    }

    closedir(dir);
    close_db(fd);
    return 0;
}

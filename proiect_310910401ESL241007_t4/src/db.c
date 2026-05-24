#include "db.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int write_database(
    const char *path,
    FileRecord *records,
    uint32_t record_count,
    WorkerStats *stats,
    uint32_t worker_count
)
{
    char tmp[4096];

    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "wb");

    if (!f) {
        return -1;
    }

    DBHeader hdr;

    memcpy(hdr.magic, DB_MAGIC, 4);

    hdr.version = DB_VERSION;
    hdr.complete = 1;
    hdr.file_record_count = record_count;
    hdr.worker_count = worker_count;

    fwrite(&hdr, sizeof(hdr), 1, f);

    fwrite(records, sizeof(FileRecord), record_count, f);

    fwrite(stats, sizeof(WorkerStats), worker_count, f);

    fflush(f);

    fsync(fileno(f));

    fclose(f);

    rename(tmp, path);

    return 0;
}

int verify_database(const char *path)
{
    FILE *f = fopen(path, "rb");

    if (!f) {
        return 1;
    }

    DBHeader hdr;

    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        fclose(f);
        return 1;
    }

    if (memcmp(hdr.magic, DB_MAGIC, 4) != 0) {
        fclose(f);
        return 1;
    }

    if (hdr.version != DB_VERSION) {
        fclose(f);
        return 1;
    }

    fclose(f);

    return 0;
}

int dump_database(const char *path)
{
    FILE *f = fopen(path, "rb");

    if (!f) {
        return 1;
    }

    DBHeader hdr;

    fread(&hdr, sizeof(hdr), 1, f);

    printf("magic=%.4s\n", hdr.magic);
    printf("version=%u\n", hdr.version);
    printf("complete=%u\n", hdr.complete);
    printf("file_record_count=%u\n", hdr.file_record_count);
    printf("worker_count=%u\n", hdr.worker_count);

    fclose(f);

    return 0;
}

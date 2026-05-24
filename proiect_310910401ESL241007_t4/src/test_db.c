#include <string.h>
#include <unistd.h>
#include "db.h"

int main() {
    int fd = open_or_init_db("data/index.db", MAGIC_IDX);

    file_record_t rec;
    memset(&rec, 0, sizeof(rec));

    strcpy(rec.path, "/tmp/testfile");
    rec.size = 123;

    upsert_file_record(fd, &rec);

    sleep(2);

    close_db(fd);
    return 0;
}

#ifndef SCAN_H
#define SCAN_H

#include "ipc.h"

int process_directory(
    SharedIPC *ipc,
    int worker_id,
    const char *path,
    int depth
);

#endif

#ifndef QUEUE_H
#define QUEUE_H

#include "ipc.h"

int enqueue_job(SharedIPC *ipc, const Job *job);
int dequeue_job(SharedIPC *ipc, Job *job);

int enqueue_result(SharedIPC *ipc, const FileRecord *record);
int dequeue_result(SharedIPC *ipc, FileRecord *record);

#endif

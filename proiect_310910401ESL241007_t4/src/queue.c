#include "queue.h"

int enqueue_job(SharedIPC *ipc, const Job *job)
{
    sem_wait(&ipc->job_slots);
    sem_wait(&ipc->job_mutex);

    ipc->jobs[ipc->header.job_tail] = *job;

    ipc->header.job_tail =
        (ipc->header.job_tail + 1) % MAX_JOBS;

    ipc->header.queued_jobs++;

    sem_post(&ipc->job_mutex);
    sem_post(&ipc->job_items);

    return 0;
}

int dequeue_job(SharedIPC *ipc, Job *job)
{
    sem_wait(&ipc->job_items);
    sem_wait(&ipc->job_mutex);

    if (ipc->header.shutdown) {
        sem_post(&ipc->job_mutex);
        return -1;
    }

    *job = ipc->jobs[ipc->header.job_head];

    ipc->header.job_head =
        (ipc->header.job_head + 1) % MAX_JOBS;

    ipc->header.queued_jobs--;

    sem_post(&ipc->job_mutex);
    sem_post(&ipc->job_slots);

    return 0;
}

int enqueue_result(SharedIPC *ipc, const FileRecord *record)
{
    sem_wait(&ipc->result_slots);
    sem_wait(&ipc->result_mutex);

    ipc->results[ipc->header.result_tail] = *record;

    ipc->header.result_tail =
        (ipc->header.result_tail + 1) % MAX_RESULTS;

    sem_post(&ipc->result_mutex);
    sem_post(&ipc->result_items);

    return 0;
}

int dequeue_result(SharedIPC *ipc, FileRecord *record)
{
    sem_wait(&ipc->result_items);
    sem_wait(&ipc->result_mutex);

    *record = ipc->results[ipc->header.result_head];

    ipc->header.result_head =
        (ipc->header.result_head + 1) % MAX_RESULTS;

    sem_post(&ipc->result_mutex);
    sem_post(&ipc->result_slots);

    return 0;
}

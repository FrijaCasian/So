# MMAP Protocol Documentation

## Overview

The multiprocess file inventory application uses memory-mapped files combined with POSIX semaphores for inter-process communication (IPC). This document describes the shared memory layout, synchronization mechanisms, and protocol semantics.

## Shared Memory File

- **Location**: `data/ipc.mmap` (configurable via `--ipc` parameter)
- **Mechanism**: POSIX mmap() with MAP_SHARED on a regular file
- **Lifecycle**: Created by manager, destroyed after workers exit
- **Isolation**: All access is through the manager process; workers attach read-write

## Memory Layout

The shared IPC region is a single mmap-ed structure containing:

```
Offset 0:   IPCHeader (96 bytes)
            - Magic, version, configuration
            - Job queue indices and counts
            - Result queue indices and counts
            - Global shutdown flag

Offset 96:  Semaphores (56 bytes = 7 × sem_t)
            - job_mutex, job_items, job_slots
            - result_mutex, result_items, result_slots
            - stats_mutex

Offset 152: Job Queue (MAX_JOBS × sizeof(Job))
            - Circular buffer for directory scan jobs
            - Each job: path (4096 bytes) + depth (4 bytes)

Offset ~4,198,552: Result Queue (MAX_RESULTS × sizeof(FileRecord))
                   - Circular buffer for file metadata records
                   - Each record: path + size + mtime + mode + uid + gid + sha256

Offset ~8,397,052: Worker Statistics (MAX_WORKERS × sizeof(WorkerStats))
                   - Per-worker counters: jobs, files, bytes, CPU time
```

Total size: ~8,463,120 bytes (≈ 8.1 MB)

## Job Queue

**Purpose**: Distribute directory scan jobs to worker processes

**Structure**: Circular buffer of Job entries

```c
typedef struct {
    char path[4096];    // Absolute path to directory
    int depth;          // Current recursion depth
} Job;
```

**Producers**: Manager (enqueues root), Workers (enqueue subdirectories)
**Consumers**: Workers (dequeue jobs to process)

**Synchronization**:
- `job_mutex`: Protects head/tail indices
- `job_items`: Semaphore counting available jobs (signal count = queue depth)
- `job_slots`: Semaphore counting free slots (post when dequeuing)

**Enqueue Operation** (in queue.c):
1. `sem_wait(&job_slots)` — Block if queue is at capacity (MAX_JOBS)
2. `sem_wait(&job_mutex)` — Lock queue
3. Place job at `jobs[job_tail]`, increment `job_tail % MAX_JOBS`
4. Increment `queued_jobs` counter
5. `sem_post(&job_mutex)` — Unlock
6. `sem_post(&job_items)` — Signal job availability

**Dequeue Operation**:
1. `sem_wait(&job_items)` — Block if queue is empty or shutdown
2. `sem_wait(&job_mutex)` — Lock queue
3. If shutdown, return error
4. Retrieve job at `jobs[job_head]`, increment `job_head % MAX_JOBS`
5. Decrement `queued_jobs` counter
6. `sem_post(&job_mutex)` — Unlock
7. Return job

## Result Queue

**Purpose**: Collect file metadata records from workers to manager

**Structure**: Circular buffer of FileRecord entries

```c
typedef struct {
    char path[4096];
    uint64_t size, mtime;
    mode_t mode;
    uid_t uid, gid;
    unsigned char sha256[32];
} FileRecord;
```

**Producer**: Workers (scan drives and emit records)
**Consumer**: Manager (collects records and assembles database)

**Synchronization**:
- `result_mutex`: Protects head/tail indices
- `result_items`: Counts records available to consume
- `result_slots`: Counts free slots to produce

**Backpressure Mechanism**:
- If queue fills, worker blocks on `sem_wait(&result_slots)`
- Manager unblocks worker by dequeueing and posting `result_slots`
- **No data loss**: Buffers never overflow; records are never lost or overwritten

**Enqueue Operation** (worker):
1. `sem_wait(&result_slots)` — Block if result queue is full
2. `sem_wait(&result_mutex)` — Lock queue
3. Place record at `results[result_tail]`, increment `result_tail % MAX_RESULTS`
4. `sem_post(&result_mutex)` — Unlock
5. `sem_post(&result_items)` — Signal record availability

**Dequeue Operation** (manager):
1. `sem_wait(&result_items)` — Non-blocking or return error if empty
2. `sem_wait(&result_mutex)` — Lock queue
3. Retrieve record at `results[result_head]`, increment `result_head % MAX_RESULTS`
4. `sem_post(&result_mutex)` — Unlock
5. `sem_post(&result_slots)` — Unblock waiting producer
6. Return record

## Worker Statistics

**Purpose**: Track per-worker performance metrics and resource usage

```c
typedef struct {
    int worker_id;
    pid_t pid;
    int exit_status;
    
    uint64_t jobs_processed;
    uint64_t files_emitted;
    uint64_t bytes_emitted;
    
    uint64_t wall_time_ms;
    uint64_t user_cpu_us;
    uint64_t sys_cpu_us;
} WorkerStats;
```

**Location**: `ipc->stats[worker_id]` (array of MAX_WORKERS entries)

**Synchronization**: `stats_mutex` protects concurrent updates

**Lifetime**:
1. Manager initializes all stats entries to zero during `ipc_init()`
2. Worker updates `worker_id`, `pid` at startup
3. Worker increments `jobs_processed`, `files_emitted`, `bytes_emitted` after each job
4. Worker records timing at termination: `wall_time_ms`, `user_cpu_us`, `sys_cpu_us`
5. Manager collects exit_status from waitpid()

## Synchronization

### Semaphore Initialization

All semaphores are initialized with `pshared=1` to enable cross-process sharing:

```c
sem_init(&ipc->job_mutex, 1, 1);           // pshared=1, init_value=1
sem_init(&ipc->job_items, 1, 0);           // Initially no jobs
sem_init(&ipc->job_slots, 1, MAX_JOBS);    // All slots available
// (similar for result queue)
```

### Critical Sections

1. **Job enqueueing (worker discovering subdirectories)**:
   - Lock: `job_mutex`
   - Operations: Modify job_tail, job_head, queued_jobs
   - Duration: Microseconds

2. **Result enqueueing (worker emitting file records)**:
   - Lock: `result_mutex`
   - Backpressure: Wait on `result_slots` (can block indefinitely if manager is slow)
   - Duration: Microseconds per record

3. **Statistics updates**:
   - Lock: `stats_mutex`
   - Operations: Increment counters
   - Duration: Microseconds

### Atomic Operations

Certain operations use lock-free atomic increments instead of semaphores:

```c
__sync_fetch_and_add(&ipc->header.active_workers, 1);     // Before processing job
__sync_fetch_and_sub(&ipc->header.active_workers, 1);     // After processing job
```

These are safe because they only read the field's current value, not compare-and-swap.

## Termination Logic

Workers terminate deterministically when:
1. **Condition**: `queued_jobs == 0 AND active_workers == 0`
   - No jobs remain in queue
   - No worker is actively processing

2. **Action by last active worker**:
   - Set `shutdown = 1`
   - Post `job_items` semaphore for all workers (N times) to unblock them
   - Each worker checks `shutdown` flag and exits

3. **Manager collects termination**:
   - Waits for all workers via `waitpid()`
   - Reads `exit_status` from WorkerStats
   - Collects `wall_time_ms`, CPU times before workers exit

### Race-Free Shutdown Sequence

```
Worker A (last active):
  // Process dequeues and sees queued_jobs and active_workers both zero
  if (ipc->header.queued_jobs == 0 && ipc->header.active_workers == 0) {
      ipc->header.shutdown = 1;
      for (int i = 0; i < ipc->header.worker_count; i++) {
          sem_post(&ipc->job_items);   // Unblock all blocked workers
      }
  }

Worker B (blocked on dequeue):
  // Wakes from sem_wait, checks shutdown flag
  if (ipc->header.shutdown) {
      break;  // Exit loop
  }
```

## Capacity Limits

- **MAX_WORKERS**: 32 — Maximum concurrent worker processes
- **MAX_JOBS**: 1024 — Depth of job queue; prevents queuing too many directories
- **MAX_RESULTS**: 1024 — Depth of result queue; buffers file records during processing

These are defined in `include/config.h` and can be adjusted if needed.

## IPC File Cleanup

After execution:
- IPC file persists at `data/ipc.mmap` unless deleted
- Semaphores are not explicitly destroyed (kernel cleans up on file deletion)
- Safe to unlink the file even if processes are still mapped

## Error Conditions

1. **IPC creation failures**: Manager logs error and exits
2. **Worker attachment failures**: Worker logs error and exits
3. **Deadlocks**: Unlikely due to strict lock ordering, but possible if:
   - Semaphore initialization fails
   - A process crashes while holding a semaphore
   - (Proper error handling prevents these scenarios)

## Verification and Debugging

To inspect IPC state:
1. Check `data/ipc.mmap` file size (should be constant at ~8.1 MB)
2. Monitor semaphore states via `/proc/sys/kernel/sem` (Linux)
3. Use `ipcs` command to check active semaphores (if using System V IPC)
4. Process accounting in `logs/` directory for timing and status information

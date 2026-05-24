# T4 Database Format

## Overview

The T4 database is a binary format that persists file inventory records collected by the multiprocess scanner. The database is written atomically by the manager process after all workers complete.

## File Layout

The database file is structured as follows:

```
[Header (27 bytes)]
[File Records (variable)]
[Worker Statistics (variable)]
```

## Header Structure

```c
typedef struct {
    char magic[4];           // "INV4"
    uint32_t version;        // 1
    uint32_t complete;       // 1 = complete, 0 = incomplete
    uint32_t file_record_count;
    uint32_t worker_count;
} DBHeader;
```

### Header Fields

- **magic**: Fixed 4-byte string "INV4" for identification
- **version**: Format version, currently 1
- **complete**: 0 or 1; set to 1 when inventory completes normally
- **file_record_count**: Total number of FileRecord entries following the header
- **worker_count**: Number of WorkerStats entries following the records

## File Record Structure

```c
typedef struct {
    char path[4096];        // Absolute path
    uint64_t size;          // File size in bytes
    uint64_t mtime;         // Modification time (seconds since epoch)
    mode_t mode;            // File mode/permissions
    uid_t uid;              // Owner's user ID
    gid_t gid;              // Owner's group ID
    unsigned char sha256[32];  // SHA256 hash digest
} FileRecord;
```

### Constraints

- Path is limited to 4096 bytes (PATH_SIZE)
- SHA256 hash is always exactly 32 bytes
- Only regular files are recorded (directories and symlinks excluded)

## Worker Statistics Structure

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

### Worker Fields

- **worker_id**: Worker process identifier (0 to N-1)
- **pid**: Process ID of the worker
- **exit_status**: Exit status value from waitpid()
- **jobs_processed**: Number of directories scanned by this worker
- **files_emitted**: Number of files recorded by this worker
- **bytes_emitted**: Total bytes of files recorded by this worker
- **wall_time_ms**: Elapsed wall-clock time in milliseconds
- **user_cpu_us**: User-mode CPU time in microseconds (from getrusage)
- **sys_cpu_us**: System-mode CPU time in microseconds (from getrusage)

## Capacity Limits

- Maximum workers: 32
- Maximum queued jobs: 1024
- Maximum pending results: 1024
- Maximum path length: 4096 bytes
- File count is bounded by result queue capacity and memory

## Validity Conditions

A database is considered valid if:

1. Exactly 27 bytes of header are present
2. Magic field is exactly "INV4" (0x494E5634)
3. Version field equals 1
4. File size is >= header_size + (file_record_count * sizeof(FileRecord)) + (worker_count * sizeof(WorkerStats))
5. If complete == 1, all expected records must be present and readable

## Atomic Write

The database is written atomically using a two-phase commit strategy:

1. Write all data to a temporary file with .tmp suffix
2. Call fsync() to ensure data is durable
3. Call rename() to atomically replace the target file

This ensures that the database is never in an inconsistent partial-write state if the process crashes.

## Reading the Database

The `dump_database()` function reads and prints:
- magic: 4-byte identifier
- version: Format version
- complete: Completion flag
- file_record_count: Number of file records
- worker_count: Number of worker statistics

The `verify_database()` function performs basic structural validation and returns 0 (success) only if the magic and version match, and the header is readable.

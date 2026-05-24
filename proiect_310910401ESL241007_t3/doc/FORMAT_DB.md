# Binary Database Format — T3

## 1. General structure

Each database file has the following layout:

[ HEADER ][ RECORDS ]

---

## 2. Header structure

```c
typedef struct {
    char magic[4];          // "IDX1" or "PRC1"
    uint32_t version;       // format version (1)
    uint64_t snapshot_id;   // unique snapshot identifier
    uint32_t snapshot_state; // OPEN (0) or SEALED (1)
    uint32_t active_writers; // number of active processes
    uint64_t record_count;  // number of records
} db_header_t;

#!/usr/bin/env bash
set -e

rm -rf testdata
mkdir -p testdata/a
mkdir -p testdata/b

printf 'hello' > testdata/a/file1.txt
printf 'world' > testdata/b/file2.txt

./tools/fileops.sh build

./tools/fileops.sh run -- fileops_manager \
    --root testdata \
    --workers 2

./tools/fileops.sh run -- fileops_manager \
    --db data/inventory.db \
    --verify

./tools/fileops.sh run -- fileops_manager \
    --db data/inventory.db \
    --dump > reports/t4_dump.txt

grep 'magic=INV4' reports/t4_dump.txt
grep 'complete=1' reports/t4_dump.txt
grep 'worker_count=2' reports/t4_dump.txt

echo 'T4 TEST PASSED'

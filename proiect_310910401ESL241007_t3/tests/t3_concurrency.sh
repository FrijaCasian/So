bash id="1q0wlc"
#!/bin/bash

set -e

echo "[TEST] Cleaning..."
rm -f data/index.db data/proc.db
rm -f reports/T3_filediff.txt reports/T3_procdiff.txt

mkdir -p data reports logs

echo "[TEST] Running file indexer concurrently..."

for i in {1..5}; do
  ./bin/fileops_indexer --root . > logs/fileops_$i.log 2>&1 &
done
wait

echo "[TEST] File DB created."

cp data/index.db data/index_old.db

touch test_diff_file

./bin/fileops_indexer --root .

cp data/index.db data/index_new.db

./bin/db_diff --old data/index_old.db \
              --new data/index_new.db \
              --out reports/T3_filediff.txt

echo "[TEST] Running proc snapshot concurrently..."

for i in {1..5}; do
  ./bin/proc_snapshot > logs/proc_$i.log 2>&1 &
done
wait

cp data/proc.db data/proc_old.db

sleep 2

./bin/proc_snapshot

cp data/proc.db data/proc_new.db

./bin/db_diff --old data/proc_old.db \
              --new data/proc_new.db \
              --out reports/T3_procdiff.txt

echo "[TEST] Done."

echo "[CHECK] Reports:"
cat reports/T3_filediff.txt
cat reports/T3_procdiff.txt

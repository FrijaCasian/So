#!/bin/bash

START=$(date +%s)
START_TIME=$(date)

mkdir -p reports/fs
mkdir -p reports/process
mkdir -p reports/proc
mkdir -p reports/pipeline

# Filesystem
ls -la > reports/fs/A1_ls_long.txt
find . -type f -name "*.sh" > reports/fs/A2_find_sh.txt
du -h --max-depth=1 > reports/fs/A3_du_level1.txt

# Processes
ps aux --sort=-%mem | head -n 10 > reports/process/B1_top_mem.txt
pstree -p > reports/process/B2_pstree.txt

sleep 6zzzzz0 &
pgrep sleep > reports/process/B3_pgrep_sleep.txt
pkill sleep

# /proc
grep "model name" /proc/cpuinfo | head -n 1 > reports/proc/C1_cpu_model.txt
grep -E "MemTotal|MemAvailable" /proc/meminfo > reports/proc/C2_mem_total_avail.txt
cut -d " " -f1 /proc/uptime > reports/proc/C3_uptime.txt

# Pipelines
find . -type f | xargs du -h | sort -hr | head -n 5 > reports/pipeline/D1_top5_large_files.txt
ps aux --sort=-%mem | head -n 6 | cut -d " " -f2,11 | uniq > reports/pipeline/D2_top5_proc_mem_pid_name.txt
grep "^" doc/T1_commands.md | sort | uniq | wc -l > reports/pipeline/D3_count_commands.txt

END=$(date +%s)
END_TIME=$(date)

RUNTIME=$((END-START))

echo "Start: $START_TIME" > reports/T1_summary.txt
echo "End: $END_TIME" >> reports/T1_summary.txt
echo "Runtime: $RUNTIME seconds" >> reports/T1_summary.txt
echo "Reports generated in reports/" >> reports/T1_summary.txt


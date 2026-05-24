# A1. 
## List the contents of the current directory in long format, including hidden files.
```
ls -al > reports/fs/A1_ls_long.txt
```
###Output: reports/fs/A1_ls_long.txt

# A2.
## Find all files with the .sh extension in the project and display their relative paths.
```
find . -type f -name "*.sh" > reports/fs/A2_find_sh.txt
```
###Output: reports/fs/A2_find_sh.txt

# A3.
## Display the size, in human-readable format, of all first-level directories in the project (for example, doc/ and reports/).
```
du -h -d 1 ./ > reports/fs/A3_du_level1.txt
```
###Output: reports/fs/A2_find_sh.txt


# B1.
## Display the top 10 processes sorted in descending order by memory consumption (RSS or %MEM).
```
ps aux --sort=-%mem | head -n 10 > reports/process/B1_top_mem.txt
```
###Output: reports/process/B1_top_mem.txt
# B2.
## Display the system process tree with visible PIDs.
```
pstree -p > reports/process/B2_pstree.txt
```
###reports/process/B2_pstree.txt
# B3.
## Start a test process (for example, sleep 60 in the background) and demonstrate that it can be identified by name and PID.
```
sleep 60 &
pgrep sleep > reports/process/B3_pgrep_sleep.txt
kill <PID>
```
###Output: reports/process/B3_pgrep_sleep.txt


# C1.
## Extract the CPU model or model name from /proc/cpuinfo.
```
grep "model name" /proc/cpuinfo | head -n 1 > reports/proc/C1_cpu_model.txt
```
###Output: reports/proc/C1_cpu_model.txt
# C2.
## Extract MemTotal and MemAvailable from /proc/meminfo.
```
grep -E "MemTotal|MemAvailable" /proc/meminfo > reports/proc/C2_mem_total_avail.txt
```
###Output: reports/proc/C2_mem_total_avail.txt
# C3.
## Display the raw uptime value, in seconds, from /proc/uptime.
```
cut -d " " -f1 /proc/uptime > reports/proc/C3_uptime.txt
```
###Output: reports/proc/C3_uptime.txt


# D1.
## Build a top-5 list of the largest files in the project, showing only size and path.
```
find . -type f | xargs du -h | sort -hr | head -n 5 > reports/pipeline/D1_top5_large_files.txt
```
### Constraint: exactly 4 subcommands joined with |. Output: reports/pipeline/D1_top5_large_files.txt
# D2.
## Build a top-5 list of processes by memory usage, keeping only the relevant fields (PID and process name), removing duplicates if they occur, and limiting the result to 5 lines.
```
ps aux --sort=-%mem | head -n 6 | cut -d " " -f2,11 | uniq > reports/pipeline/D2_top5_proc_mem_pid_name.txt
```
###Constraint: exactly 4 subcommands joined with |. Output: reports/pipeline/D2_top5_proc_mem_pid_name.txt
# D3.
## From doc/T1_commands.md, extract the lines that contain commands (for example, code-block lines or lines beginning with a shell prompt), sort them, and count them.
```
grep "^" doc/T1_commands.md | sort | uniq | wc -l > reports/pipeline/D3_count_commands.txt
```
###Constraint: exactly 4 subcommands joined with |. Output: reports/pipeline/D3_count_commands.txt


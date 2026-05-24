#!/usr/bin/env bash

set -e

./tools/fileops.sh init

dirs=(bin src include data logs reports tmp tests doc tools)

for d in "${dirs[@]}"; do
    if [[ ! -d "$d" ]]; then
        echo "Missing directory: $d"
        exit 1
    fi
done

exit 0

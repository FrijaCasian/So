#!/usr/bin/env bash

set -e

SRC="tmp/test_scenario_src"

mkdir -p "$SRC/app"
mkdir -p "$SRC/lib/include"

# header
cat > "$SRC/lib/include/util.h" <<EOF
#ifndef UTIL_H
#define UTIL_H
int util_add(int a, int b);
#endif
EOF

# source
cat > "$SRC/lib/util.c" <<EOF
#include "util.h"
int util_add(int a, int b) {
    return a + b;
}
EOF

# main
cat > "$SRC/app/main_demo.c" <<EOF
#include <stdio.h>
#include "util.h"

int main() {
    printf("%d\n", util_add(2,3));
    return 0;
}
EOF

# compile
CFLAGS="-I$SRC/lib/include -std=c11 -Wall -Wextra" \
./tools/fileops.sh build --src "$SRC"

# check executable
if [[ ! -x bin/demo ]]; then
    echo "Executable not found"
    exit 1
fi

# run
./bin/demo > tmp/demo_out.txt

if grep -q "5" tmp/demo_out.txt; then
    exit 0
else
    echo "Wrong output"
    exit 1
fi

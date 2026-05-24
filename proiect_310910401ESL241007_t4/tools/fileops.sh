#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

mkdir -p "$ROOT_DIR/bin"
mkdir -p "$ROOT_DIR/data"
mkdir -p "$ROOT_DIR/logs"
mkdir -p "$ROOT_DIR/reports"

cmd="$1"
shift || true

build() {
    gcc -Wall -Wextra -O2 -Iinclude \
        src/main_fileops_manager.c \
        src/manager.c \
        src/ipc.c \
        src/queue.c \
        src/db.c \
        src/scan.c \
        src/sha256.c \
        -o bin/fileops_manager \
        -pthread -lcrypto

    gcc -Wall -Wextra -O2 -Iinclude \
        src/main_fileops_worker.c \
        src/worker.c \
        src/ipc.c \
        src/queue.c \
        src/scan.c \
        src/sha256.c \
        -o bin/fileops_worker \
        -pthread -lcrypto
}

run() {
    if [[ "$1" == "--" ]]; then
        shift
    fi

    exe="$1"
    shift

    exec "./bin/$exe" "$@"
}

case "$cmd" in
    init)
        echo "initialized"
        ;;

    build)
        build
        ;;

    run)
        run "$@"
        ;;

    test)
        ./tests/t4_inventory.sh
        ;;

    *)
        echo "unknown command"
        exit 1
        ;;
esac

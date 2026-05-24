#!/usr/bin/env bash

set -e

LOG_DIR="logs"
mkdir -p "$LOG_DIR"

START_TIME=$(date +%s)
LOG_FILE="$LOG_DIR/fileops_$(date +%Y%m%d_%H%M%S).log"

log() {
    echo "$1" | tee -a "$LOG_FILE"
}

finish() {
    EXIT_CODE=$?
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))

    log "End time: $(date)"
    log "Duration: ${DURATION}s"
    log "Exit code: $EXIT_CODE"

    exit $EXIT_CODE
}

trap finish EXIT

SUBCOMMAND="$1"
shift || true

log "Subcommand: $SUBCOMMAND"
log "Start time: $(date)"

# ========================
# INIT
# ========================
cmd_init() {
    if ! command -v gcc >/dev/null 2>&1; then
        echo "ERROR: gcc is not installed."
        exit 1
    fi

    mkdir -p bin src include data logs reports tmp tests doc tools
    mkdir -p tmp/obj

    echo "Project initialized."
}

# ========================
# BUILD (recursive + incremental)
# ========================
compile_file() {
    local file="$1"
    local src_root="$2"

    rel_path="${file#$src_root/}"
    obj_file="tmp/obj/${rel_path%.c}.o"

    mkdir -p "$(dirname "$obj_file")"

    if [[ ! -f "$obj_file" || "$file" -nt "$obj_file" ]]; then
        gcc ${CFLAGS:-} -c "$file" -o "$obj_file"
    fi
}

build_recursive() {
    local dir="$1"
    local src_root="$2"

    for entry in "$dir"/*; do
        [[ -e "$entry" ]] || continue

        if [[ -d "$entry" ]]; then
            build_recursive "$entry" "$src_root"
        elif [[ "$entry" == *.c ]]; then
            compile_file "$entry" "$src_root"
        fi
    done
}

cmd_build() {
    SRC_DIR="src"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --src)
                SRC_DIR="$2"
                shift 2
                ;;
            *)
                shift
                ;;
        esac
    done

    mkdir -p tmp/obj bin

    build_recursive "$SRC_DIR" "$SRC_DIR"

    # link executables
    find "$SRC_DIR" -name "main_*.c" | while read -r main_file; do
        name=$(basename "$main_file")
        exe="${name#main_}"
        exe="${exe%.c}"

        obj_files=$(find tmp/obj -name "*.o")

        gcc $obj_files -o "bin/$exe"
    done

    echo "Build complete."
}

# ========================
# RUN
# ========================
cmd_run() {
    if [[ "$1" != "--" ]]; then
        echo "Usage: run -- <exe> [args...]"
        exit 1
    fi
    shift

    exe="$1"
    shift

    "./bin/$exe" "$@"
}

# ========================
# CLEAN
# ========================
cmd_clean() {
    rm -rf tmp/obj/*
    rm -rf bin/*
    echo "Clean complete."
}

# ========================
# TEST (implicit recursion)
# ========================
cmd_test() {
    REPORT="reports/T2_tests.txt"
    mkdir -p reports

    > "$REPORT"

    FAIL=0

    find tests -type f -name "*.sh" | while read -r testfile; do
        if bash "$testfile"; then
            echo "$(basename "$testfile"): PASS" >> "$REPORT"
        else
            echo "$(basename "$testfile"): FAIL" >> "$REPORT"
            FAIL=1
        fi
    done

    if [[ $FAIL -ne 0 ]]; then
        exit 1
    fi

    echo "Tests completed."
}

# ========================
# DISPATCH
# ========================
case "$SUBCOMMAND" in
    init) cmd_init ;;
    build) cmd_build "$@" ;;
    run) cmd_run "$@" ;;
    clean) cmd_clean ;;
    test) cmd_test ;;
    *)
        echo "Unknown command"
        exit 1
        ;;
esac

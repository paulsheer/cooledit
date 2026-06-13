#!/bin/bash
# Comprehensive test suite for cooledit --filetool
# Tests every case from filetool.txt against remotefs server at 172.16.10.5
# Both server and client are run under valgrind with leak checking.
SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"
COOLEDIT="$(cd "$SCRIPTDIR/../editor" && pwd)/cooledit"
REMOTEFS="$(cd "$SCRIPTDIR/../remotefs" && pwd)/remotefs"
WORKDIR_BASE="$SCRIPTDIR/work-dir"
VGLOG_DIR="$WORKDIR_BASE/valgrind-logs"
VALGRIND="valgrind"
VALGRIND_FLAGS="--leak-check=full --show-leak-kinds=all --error-exitcode=42"
LISTEN_IP="172.16.10.5"
IP_RANGE="172.16.10.0/24"
REMOTE="${LISTEN_IP}:"
PORT=50095
PASSED=0
FAILED=0
VGLOG_COUNTER=0

WORKDIR=""
SERVER_PID=""
SERVER_VGLOG=""
TMPDIR_CLEANUP=()

cleanup() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        sleep 2
        # kill orphaned memcheck children, their diagnostics are irrelevant
        pkill -9 -P "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    for d in "${TMPDIR_CLEANUP[@]}"; do
        rm -rf "$d" 2>/dev/null || true
    done
}

trap cleanup EXIT

# --- helpers ---

tmpdir() {
    local d
    d=$(mktemp -d "$WORKDIR_BASE/filetool-test-XXXXXX")
    TMPDIR_CLEANUP+=("$d")
    echo "$d"
}

createtext() {
    local f="$1" text="$2"
    mkdir -p "$(dirname "$f")"
    echo "$text" > "$f"
}

fail() {
    echo "  FAIL: $1"
    ((FAILED++))
}

pass() {
    echo "  PASS: $1"
    ((PASSED++))
}

assert_eq() {
    if [ "$1" != "$2" ]; then
        fail "$3: expected '$2' got '$1'"
        return 1
    fi
    pass "$3"
    return 0
}

assert_file_eq() {
    if ! diff -q "$1" "$2" >/dev/null 2>&1; then
        fail "$3: files differ"
        return 1
    fi
    pass "$3"
    return 0
}

assert_error() {
    if [ "$1" -ne 0 ]; then
        pass "$2 (got expected error)"
    else
        fail "$2 (expected error, got success)"
        return 1
    fi
}

assert_success() {
    if [ "$1" -eq 0 ]; then
        pass "$2"
    else
        fail "$2 (expected success, got error $1)"
        return 1
    fi
}

# Run cooledit --filetool under valgrind. Returns cooledit exit code.
# Use "yes n |" prefix to answer overwrite prompts with "no".
run_filetool() {
    local vglog
    VGLOG_COUNTER=$((VGLOG_COUNTER + 1))
    vglog=$(printf "%s/client-%03d.log" "$VGLOG_DIR" "$VGLOG_COUNTER")
    (cd "$WORKDIR" && "$VALGRIND" $VALGRIND_FLAGS --log-file="$vglog" \
        "$COOLEDIT" --filetool "$@") >/dev/null 2>&1
    local ret=$?
    if [ "$ret" -eq 42 ]; then
        echo "  VALGRIND: error exit code 42 for --filetool $*"
        ((FAILED++))
    fi
    if grep -q "ERROR SUMMARY: [1-9]" "$vglog" 2>/dev/null; then
        echo "  VALGRIND: errors detected for --filetool $*"
        grep "ERROR SUMMARY" "$vglog"
        ((FAILED++))
    fi
    if grep -q "definitely lost: [1-9]\|indirectly lost: [1-9]" "$vglog" 2>/dev/null; then
        echo "  VALGRIND: memory leaks detected for --filetool $*"
        grep "lost:" "$vglog"
        ((FAILED++))
    fi
    if grep -q "in use at exit: [1-9]" "$vglog" 2>/dev/null; then
        local inuse
        inuse=$(grep "in use at exit:" "$vglog" | head -1 | sed 's/.*in use at exit: *//' | sed 's/ bytes.*//' | tr -d ',')
        if [ -n "$inuse" ] && [ "$inuse" -gt 400 ] 2>/dev/null; then
            echo "  VALGRIND: memory in use at exit ($inuse bytes) for --filetool $*"
            grep "in use at exit" "$vglog"
            ((FAILED++))
        fi
    fi
    return $ret
}

# Start remotefs server under valgrind
start_server() {
    SERVER_VGLOG="$VGLOG_DIR/server.log"
    echo "Starting remotefs server under valgrind..."
    (cd "$WORKDIR" && "$VALGRIND" $VALGRIND_FLAGS --log-file="$SERVER_VGLOG" \
        "$REMOTEFS" --no-crypto "$LISTEN_IP" "$IP_RANGE" >/dev/null 2>&1) &
    # Wait for server to be listening
    local waited=0
    while ! ss -tln 2>/dev/null | grep -q ":$PORT " && [ $waited -lt 30 ]; do
        sleep 1
        waited=$((waited + 1))
    done
    if [ $waited -ge 30 ]; then
        echo "ERROR: server failed to start listening on port $PORT"
        exit 1
    fi
    SERVER_PID=$(cat $SERVER_VGLOG | head -1 | awk '{print $1}' | tr -d '=')
    echo "Server PID=$SERVER_PID listening on $LISTEN_IP:$PORT"
}

stop_server() {
    echo "Stopping server..."
    kill -INT "$SERVER_PID" 2>/dev/null || true
    # wait for valgrind to finish leak check and write summary
    local waited=0 server_ret
    while kill -0 "$SERVER_PID" 2>/dev/null && [ $waited -lt 10 ]; do
        sleep 2
        waited=$((waited + 1))
    done
    # if still alive, kill orphans and force-exit
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        pkill -9 -P "$SERVER_PID" 2>/dev/null || true
        sleep 1
    fi
    wait "$SERVER_PID" 2>/dev/null
    server_ret=$?
    SERVER_PID=""
    echo "Checking server valgrind log..."
    if [ "$server_ret" -eq 42 ]; then
        echo "  VALGRIND: error exit code 42 for server (exit $server_ret)"
        ((FAILED++))
    fi
    if grep -q "ERROR SUMMARY: [1-9]" "$SERVER_VGLOG" 2>/dev/null; then
        echo "  VALGRIND: errors detected in server"
        grep "ERROR SUMMARY" "$SERVER_VGLOG"
        ((FAILED++))
    fi
    if grep -q "definitely lost: [1-9]\|indirectly lost: [1-9]" "$SERVER_VGLOG" 2>/dev/null; then
        echo "  VALGRIND: memory leaks detected in server"
        grep "lost:" "$SERVER_VGLOG"
        ((FAILED++))
    fi
    if grep -q "in use at exit: [1-9]" "$SERVER_VGLOG" 2>/dev/null; then
        local inuse
        inuse=$(grep "in use at exit:" "$SERVER_VGLOG" | head -1 | sed 's/.*in use at exit: *//' | sed 's/ bytes.*//' | tr -d ',')
        if [ -n "$inuse" ] && [ "$inuse" -gt 2000 ] 2>/dev/null; then
            echo "  VALGRIND: memory in use at exit ($inuse bytes) in server"
            grep "in use at exit" "$SERVER_VGLOG"
            ((FAILED++))
        fi
    fi
}

check_valgrind_errors() {
    # Search all valgrind logs for errors
    local errors=0
    for vglog in "${TMPDIR_CLEANUP[@]}"; do
        if [ -f "$vglog" ] && grep -q "ERROR SUMMARY: [1-9]" "$vglog" 2>/dev/null; then
            echo "Valgrind errors in $vglog:"
            grep "ERROR SUMMARY" "$vglog"
            ((errors++))
        fi
    done
    return $errors
}

# ============================================================
# Setup
# ============================================================
echo "=== cooledit --filetool Test Suite ==="
echo ""

# Kill all stale remotefs processes and verify port is free
echo "Cleaning up stale processes..."
pkill -9 -x remotefs 2>/dev/null || true
pkill -9 -f "valgrind.*remotefs" 2>/dev/null || true
pkill -9 -f "remotefs.*remotefs" 2>/dev/null || true
sleep 2
fuser -k 50095/tcp 2>/dev/null || true
sleep 1
if ss -tln 2>/dev/null | grep -q ":$PORT "; then
    echo "ERROR: port $PORT still in use after cleanup"
    ss -tln 2>/dev/null | grep ":$PORT "
    exit 1
fi
STALE=$(pgrep -x remotefs 2>/dev/null || true)
if [ -n "$STALE" ]; then
    echo "ERROR: remotefs process(es) still running after cleanup:"
    pgrep -ax remotefs 2>/dev/null
    exit 1
fi
echo "No stale remotefs processes, port $PORT is free."

mkdir -p "$WORKDIR_BASE" "$VGLOG_DIR"
WORKDIR=$(tmpdir)
echo "Test workspace: $WORKDIR"

# Create test data in workspace
echo "Setting up test data..."

# Local directories for source data
mkdir -p "$WORKDIR/local-src/subdir/deep"
mkdir -p "$WORKDIR/local-src/emptydir"
createtext "$WORKDIR/local-src/file1.txt" "Hello from file1"
createtext "$WORKDIR/local-src/file2.txt" "Contents of file2"
createtext "$WORKDIR/local-src/subdir/nested.txt" "nested file content"
createtext "$WORKDIR/local-src/subdir/deep/deep.txt" "deeply nested"

# Deep tree (remote side): 3 levels, 4 files at each level
mkdir -p "$WORKDIR/remote-src/deep-tree/sub1/sub2"
for i in 1 2 3 4; do
    createtext "$WORKDIR/remote-src/deep-tree/a${i}.txt" "level0-file-${i}"
    createtext "$WORKDIR/remote-src/deep-tree/sub1/b${i}.txt" "level1-file-${i}"
    createtext "$WORKDIR/remote-src/deep-tree/sub1/sub2/c${i}.txt" "level2-file-${i}"
done

# Local directories for destination tests
mkdir -p "$WORKDIR/local-dst/existing-dir"
createtext "$WORKDIR/local-dst/existing-file.txt" "pre-existing local file"

# Remote directories (on server side)
mkdir -p "$WORKDIR/remote-src/subdir"
createtext "$WORKDIR/remote-src/file1.txt" "Remote file1 content"
createtext "$WORKDIR/remote-src/file2.txt" "Remote file2 content"
createtext "$WORKDIR/remote-src/subdir/nested.txt" "Remote nested"

mkdir -p "$WORKDIR/remote-dst/existing-dir"
createtext "$WORKDIR/remote-dst/existing-file.txt" "pre-existing remote file"

# Start the server
start_server

echo ""
echo "=== Test Cases ==="
echo ""

# ============================================================
# Cases 1-2: local file -> remote
# ============================================================
echo "--- Case 1: local file -> remote directory ---"
rm -rf "$WORKDIR/remote-dst/existing-dir/file1.txt"
run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/existing-dir/file1.txt" \
    "local file -> remote dir creates basename"

echo ""
echo "--- Case 2: local file -> remote non-existent path ---"
rm -f "$WORKDIR/remote-dst/newfile.txt"
run_filetool "$WORKDIR/local-src/file2.txt" "${REMOTE}${WORKDIR}/remote-dst/newfile.txt"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/remote-dst/newfile.txt" \
    "local file -> remote non-existent creates as named"

echo ""
echo "--- Case 2 (overwrite): local file -> existing remote file ---"
cp "$WORKDIR/remote-dst/existing-file.txt" "$WORKDIR/remote-dst/existing-file.txt.ref"
echo "n" | run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt"
assert_file_eq "$WORKDIR/remote-dst/existing-file.txt.ref" \
    "$WORKDIR/remote-dst/existing-file.txt" \
    "local file -> existing remote file: not overwritten when answering n"

echo ""
echo "--- Case 2 (force overwrite): local file -> existing remote file with -f ---"
run_filetool -f "$WORKDIR/local-src/file1.txt" "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/existing-file.txt" \
    "local file -> remote file with -f overwrites"

# ============================================================
# Cases 3-4: remote file -> local
# ============================================================
echo ""
echo "--- Case 3: remote file -> local directory ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt"
run_filetool "${REMOTE}${WORKDIR}/remote-src/file1.txt" "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "remote file -> local dir creates basename"

echo ""
echo "--- Case 4: remote file -> local non-existent path ---"
rm -f "$WORKDIR/local-dst/new-remote-file.txt"
run_filetool "${REMOTE}${WORKDIR}/remote-src/file2.txt" "$WORKDIR/local-dst/new-remote-file.txt"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/new-remote-file.txt" \
    "remote file -> local non-existent creates as named"

echo ""
echo "--- Case 4 (force overwrite): remote file -> existing local file with -f ---"
cp "$WORKDIR/local-src/file2.txt" "$WORKDIR/local-dst/tmp-overwrite.txt"
run_filetool -f "${REMOTE}${WORKDIR}/remote-src/file1.txt" "$WORKDIR/local-dst/tmp-overwrite.txt"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/tmp-overwrite.txt" \
    "remote file -> local file with -f overwrites"

# ============================================================
# Cases 5-7: local directory -> remote
# ============================================================
echo ""
echo "--- Case 5: local dir -> remote existing directory ---"
rm -rf "$WORKDIR/remote-dst/existing-dir/local-src"
run_filetool "$WORKDIR/local-src" "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/existing-dir/local-src/file1.txt" \
    "local dir -> remote dir: file1 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/remote-dst/existing-dir/local-src/subdir/nested.txt" \
    "local dir -> remote dir: nested file copied"
assert_file_eq "$WORKDIR/local-src/subdir/deep/deep.txt" \
    "$WORKDIR/remote-dst/existing-dir/local-src/subdir/deep/deep.txt" \
    "local dir -> remote dir: deep file copied"
[ -d "$WORKDIR/remote-dst/existing-dir/local-src/emptydir" ] && \
    pass "local dir -> remote dir: empty dir present" || \
    fail "local dir -> remote dir: empty dir missing"

echo ""
echo "--- Case 6: local dir -> remote non-existent path ---"
rm -rf "$WORKDIR/remote-dst/created-dir"
run_filetool "$WORKDIR/local-src" "${REMOTE}${WORKDIR}/remote-dst/created-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/created-dir/file1.txt" \
    "local dir -> remote non-existent: file1 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/remote-dst/created-dir/subdir/nested.txt" \
    "local dir -> remote non-existent: nested file copied"

echo ""
echo "--- Case 7: local dir -> existing remote file (error) ---"
run_filetool "$WORKDIR/local-src" "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt" && \
    fail "local dir -> remote file: should have errored" || \
    pass "local dir -> remote file: correctly errors"

echo ""
echo "--- Deep tree: 3-level remote dir -> local dir, verify with diff -r ---"
rm -rf "$WORKDIR/local-dst/deep-tree"
run_filetool "${REMOTE}${WORKDIR}/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree"
if diff -r "$WORKDIR/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree" >/dev/null 2>&1; then
    pass "deep tree remote->local: diff -r matches"
else
    fail "deep tree remote->local: diff -r shows differences"
fi

# ============================================================
# Cases 8-10: remote directory -> local
# ============================================================
echo ""
echo "--- Case 8: remote dir -> local existing directory ---"
rm -rf "$WORKDIR/local-dst/existing-dir/remote-src"
run_filetool "${REMOTE}${WORKDIR}/remote-src" "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/remote-src/file1.txt" \
    "remote dir -> local dir: file1 copied"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/existing-dir/remote-src/file2.txt" \
    "remote dir -> local dir: file2 copied"
assert_file_eq "$WORKDIR/remote-src/subdir/nested.txt" \
    "$WORKDIR/local-dst/existing-dir/remote-src/subdir/nested.txt" \
    "remote dir -> local dir: nested file copied"

echo ""
echo "--- Case 9: remote dir -> local non-existent path ---"
rm -rf "$WORKDIR/local-dst/created-remote-dir"
run_filetool "${REMOTE}${WORKDIR}/remote-src" "$WORKDIR/local-dst/created-remote-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/created-remote-dir/file1.txt" \
    "remote dir -> local non-existent: file1 copied"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/created-remote-dir/file2.txt" \
    "remote dir -> local non-existent: file2 copied"

echo ""
echo "--- Case 10: remote dir -> existing local file (error) ---"
run_filetool "${REMOTE}${WORKDIR}/remote-src" "$WORKDIR/local-dst/existing-file.txt" && \
    fail "remote dir -> local file: should have errored" || \
    pass "remote dir -> local file: correctly errors"

# ============================================================
# Case 11: multi-source local -> remote directory
# ============================================================
echo ""
echo "--- Case 11: multi-source local -> remote dir ---"
rm -rf "$WORKDIR/remote-dst/existing-dir/file1.txt" "$WORKDIR/remote-dst/existing-dir/file2.txt"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/existing-dir/file1.txt" \
    "multi src local->remote: file1 copied"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/remote-dst/existing-dir/file2.txt" \
    "multi src local->remote: file2 copied"

echo ""
echo "--- Case 11 (error): multi-source local -> remote file ---"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt" && \
    fail "multi src local->remote file: should have errored" || \
    pass "multi src local->remote file: correctly errors"

echo ""
echo "--- Case 11 (mixed): local file + local dir -> remote dir ---"
rm -rf "$WORKDIR/remote-dst/existing-dir/subdir" "$WORKDIR/remote-dst/existing-dir/file2.txt"
run_filetool \
    "$WORKDIR/local-src/subdir" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/remote-dst/existing-dir/file2.txt" \
    "multi src local file+dir->remote: file2 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/remote-dst/existing-dir/subdir/nested.txt" \
    "multi src local file+dir->remote: nested file copied"

# ============================================================
# Case 12: multi-source remote -> local directory
# ============================================================
echo ""
echo "--- Case 12: multi-source remote -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt" "$WORKDIR/local-dst/existing-dir/file2.txt"
run_filetool \
    "${REMOTE}${WORKDIR}/remote-src/file1.txt" \
    "${REMOTE}${WORKDIR}/remote-src/file2.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "multi src remote->local: file1 copied"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/existing-dir/file2.txt" \
    "multi src remote->local: file2 copied"

echo ""
echo "--- Case 12 (error): multi-source remote -> local file ---"
run_filetool \
    "${REMOTE}${WORKDIR}/remote-src/file1.txt" \
    "${REMOTE}${WORKDIR}/remote-src/file2.txt" \
    "$WORKDIR/local-dst/existing-file.txt" && \
    fail "multi src remote->local file: should have errored" || \
    pass "multi src remote->local file: correctly errors"

echo ""
echo "--- Case 12 (mixed): remote file + remote dir -> local dir ---"
rm -rf "$WORKDIR/local-dst/existing-dir/remote-src" "$WORKDIR/local-dst/existing-dir/file1.txt"
run_filetool \
    "${REMOTE}${WORKDIR}/remote-src/subdir" \
    "${REMOTE}${WORKDIR}/remote-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "multi src remote file+dir->local: file1 copied"
assert_file_eq "$WORKDIR/remote-src/subdir/nested.txt" \
    "$WORKDIR/local-dst/existing-dir/subdir/nested.txt" \
    "multi src remote file+dir->local: nested file copied"

# ============================================================
# Cross-remote error
# ============================================================
echo ""
echo "--- Cross-remote: IP-to-IP should error ---"
run_filetool "${REMOTE}${WORKDIR}/remote-src/file1.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/should-not-exist" && \
    fail "cross-remote copy: should have errored" || \
    pass "cross-remote copy: correctly errors"

# ============================================================
# -f / --force flag
# ============================================================
echo ""
echo "--- Force flag (-f) ---"
rm -f "$WORKDIR/remote-dst/force-test.txt"
createtext "$WORKDIR/remote-dst/force-test.txt" "original content"
createtext "$WORKDIR/local-src/force-test.txt" "new content"
run_filetool -f "$WORKDIR/local-src/force-test.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/force-test.txt"
assert_file_eq "$WORKDIR/local-src/force-test.txt" \
    "$WORKDIR/remote-dst/force-test.txt" \
    "-f flag overwrites without prompting"

echo ""
echo "--- Force flag (--force) ---"
rm -f "$WORKDIR/remote-dst/force-test2.txt"
createtext "$WORKDIR/remote-dst/force-test2.txt" "original"
createtext "$WORKDIR/local-src/force-test2.txt" "new content 2"
run_filetool --force "$WORKDIR/local-src/force-test2.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/force-test2.txt"
assert_file_eq "$WORKDIR/local-src/force-test2.txt" \
    "$WORKDIR/remote-dst/force-test2.txt" \
    "--force flag overwrites without prompting"

# ============================================================
# Non-existent source error
# ============================================================
echo ""
echo "--- Non-existent source ---"
run_filetool "$WORKDIR/local-src/does-not-exist.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/should-not-be-created" && \
    fail "non-existent source: should have errored" || \
    pass "non-existent source: correctly errors"

# ============================================================
# Stop server and check results
# ============================================================
echo ""
echo "=== Stopping Server ==="
stop_server

echo ""
echo "=== Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ "$FAILED" -gt 0 ]; then
    echo ""
    echo "=== Valgrind Summary ==="
    for vglog in "${TMPDIR_CLEANUP[@]}"; do
        if [ -f "$vglog" ] && grep -q "ERROR SUMMARY" "$vglog" 2>/dev/null; then
            echo "${vglog}:"
            grep "ERROR SUMMARY" "$vglog"
            grep "lost:" "$vglog" 2>/dev/null | grep -v "0 bytes"
        fi
    done
    exit 1
fi

echo ""
echo "All tests passed. No valgrind errors or leaks detected."
exit 0

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

# Verify that symlinks in srcdir are reproduced in dstdir with matching targets
assert_symlinks_match() {
    local srcdir="$1" dstdir="$2" desc="$3"
    local ok=1 src_target dst_target relpath dstlink
    while IFS= read -r link; do
        relpath="${link#$srcdir/}"
        dstlink="$dstdir/$relpath"
        dst_target=$(readlink "$dstlink" 2>/dev/null) || {
            fail "$desc: $relpath missing or not a symlink at dst"; ok=0; continue; }
        src_target=$(readlink "$link")
        if [ "$src_target" != "$dst_target" ]; then
            fail "$desc: $relpath target '$dst_target' != '$src_target'"
            ok=0
        fi
    done < <(find "$srcdir" -type l 2>/dev/null | sort)
    [ "$ok" -eq 1 ] && pass "$desc"
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

# Run cooledit --filetool and capture stderr into FILE_TOOL_STDERR.
run_filetool_stderr() {
    local vglog
    VGLOG_COUNTER=$((VGLOG_COUNTER + 1))
    vglog=$(printf "%s/client-%03d.log" "$VGLOG_DIR" "$VGLOG_COUNTER")
    FILE_TOOL_STDERR=$(cd "$WORKDIR" && "$VALGRIND" $VALGRIND_FLAGS --log-file="$vglog" \
        "$COOLEDIT" --filetool "$@" 2>&1 1>/dev/null)
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

# Run cooledit --filetool --ls and capture stdout into LS_STDOUT.
run_filetool_ls_capture() {
    local vglog
    VGLOG_COUNTER=$((VGLOG_COUNTER + 1))
    vglog=$(printf "%s/client-%03d.log" "$VGLOG_DIR" "$VGLOG_COUNTER")
    LS_STDOUT=$(cd "$WORKDIR" && "$VALGRIND" $VALGRIND_FLAGS --log-file="$vglog" \
        "$COOLEDIT" --filetool --ls "$@" 2>/dev/null)
    local ret=$?
    if [ "$ret" -eq 42 ]; then
        echo "  VALGRIND: error exit code 42 for --filetool --ls $*"
        ((FAILED++))
    fi
    if grep -q "ERROR SUMMARY: [1-9]" "$vglog" 2>/dev/null; then
        echo "  VALGRIND: errors detected for --filetool --ls $*"
        grep "ERROR SUMMARY" "$vglog"
        ((FAILED++))
    fi
    if grep -q "definitely lost: [1-9]\|indirectly lost: [1-9]" "$vglog" 2>/dev/null; then
        echo "  VALGRIND: memory leaks detected for --filetool --ls $*"
        grep "lost:" "$vglog"
        ((FAILED++))
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
ln -s "a1.txt" "$WORKDIR/remote-src/deep-tree/link-a1"
ln -s "sub1" "$WORKDIR/remote-src/deep-tree/link-sub1"
ln -s "sub2" "$WORKDIR/remote-src/deep-tree/sub1/link-deep"
ln -s "../b1.txt" "$WORKDIR/remote-src/deep-tree/sub1/sub2/link-up"

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

# Dash-prefixed filenames for -- delimiter tests
createtext "$WORKDIR/local-src/-leading-dash.txt" "file with leading dash"
createtext "$WORKDIR/local-src/--leading-double-dash.txt" "file with leading double dash"
createtext "$WORKDIR/remote-src/-leading-dash.txt" "remote file with leading dash"
createtext "$WORKDIR/remote-src/--leading-double-dash.txt" "remote file with leading double dash"

# Symlink test data (local side)
mkdir -p "$WORKDIR/local-symlinks/subdir"
createtext "$WORKDIR/local-symlinks/regular.txt" "regular file for symlink target"
createtext "$WORKDIR/local-symlinks/subdir/nested.txt" "nested file for symlink target"
ln -s "regular.txt" "$WORKDIR/local-symlinks/link-to-file"
ln -s "subdir" "$WORKDIR/local-symlinks/link-to-dir"
ln -s "/etc/hosts" "$WORKDIR/local-symlinks/link-absolute"

# Symlink test data (remote side)
mkdir -p "$WORKDIR/remote-symlinks/subdir"
createtext "$WORKDIR/remote-symlinks/rfile.txt" "remote symlink target"
createtext "$WORKDIR/remote-symlinks/subdir/rnested.txt" "remote nested target"
ln -s "rfile.txt" "$WORKDIR/remote-symlinks/rlink-to-file"
ln -s "subdir" "$WORKDIR/remote-symlinks/rlink-to-dir"

# Start the server
start_server

echo ""
echo "=== Test Cases ==="
echo ""

# ============================================================
# Cases 1-2: local file -> remote
# ============================================================
echo "--- Case: local file -> remote directory ---"
rm -rf "$WORKDIR/remote-dst/existing-dir/file1.txt"
run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/existing-dir/file1.txt" \
    "local file -> remote dir creates basename"

echo ""
echo "--- Case: local file -> remote non-existent path ---"
rm -f "$WORKDIR/remote-dst/newfile.txt"
run_filetool "$WORKDIR/local-src/file2.txt" "${REMOTE}${WORKDIR}/remote-dst/newfile.txt"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/remote-dst/newfile.txt" \
    "local file -> remote non-existent creates as named"

echo ""
echo "--- Case: (overwrite): local file -> existing remote file ---"
cp "$WORKDIR/remote-dst/existing-file.txt" "$WORKDIR/remote-dst/existing-file.txt.ref"
echo "n" | run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt"
assert_file_eq "$WORKDIR/remote-dst/existing-file.txt.ref" \
    "$WORKDIR/remote-dst/existing-file.txt" \
    "local file -> existing remote file: not overwritten when answering n"

echo ""
echo "--- Case: (force overwrite): local file -> existing remote file with -f ---"
run_filetool -f "$WORKDIR/local-src/file1.txt" "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/existing-file.txt" \
    "local file -> remote file with -f overwrites"

# ============================================================
# Cases 3-4: remote file -> local
# ============================================================
echo ""
echo "--- Case: remote file -> local directory ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt"
run_filetool "${REMOTE}${WORKDIR}/remote-src/file1.txt" "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "remote file -> local dir creates basename"

echo ""
echo "--- Case: remote file -> local non-existent path ---"
rm -f "$WORKDIR/local-dst/new-remote-file.txt"
run_filetool "${REMOTE}${WORKDIR}/remote-src/file2.txt" "$WORKDIR/local-dst/new-remote-file.txt"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/new-remote-file.txt" \
    "remote file -> local non-existent creates as named"

echo ""
echo "--- Case: (force overwrite): remote file -> existing local file with -f ---"
cp "$WORKDIR/local-src/file2.txt" "$WORKDIR/local-dst/tmp-overwrite.txt"
run_filetool -f "${REMOTE}${WORKDIR}/remote-src/file1.txt" "$WORKDIR/local-dst/tmp-overwrite.txt"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/tmp-overwrite.txt" \
    "remote file -> local file with -f overwrites"

# ============================================================
# Cases 5-7: local directory -> remote
# ============================================================
echo ""
echo "--- Case: local dir -> remote existing directory ---"
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
echo "--- Case: local dir -> remote non-existent path ---"
rm -rf "$WORKDIR/remote-dst/created-dir"
run_filetool "$WORKDIR/local-src" "${REMOTE}${WORKDIR}/remote-dst/created-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/remote-dst/created-dir/file1.txt" \
    "local dir -> remote non-existent: file1 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/remote-dst/created-dir/subdir/nested.txt" \
    "local dir -> remote non-existent: nested file copied"

echo ""
echo "--- Case: local dir -> existing remote file (error) ---"
run_filetool "$WORKDIR/local-src" "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt" && \
    fail "local dir -> remote file: should have errored" || \
    pass "local dir -> remote file: correctly errors"

echo ""
echo "--- Case: Deep tree: 3-level remote dir -> local dir, verify with diff -r ---"
rm -rf "$WORKDIR/local-dst/deep-tree"
run_filetool "${REMOTE}${WORKDIR}/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree"
if diff -r "$WORKDIR/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree" >/dev/null 2>&1; then
    pass "deep tree remote->local: diff -r matches"
else
    fail "deep tree remote->local: diff -r shows differences"
fi
assert_symlinks_match "$WORKDIR/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree" \
    "deep tree remote->local: symlinks preserved"

# ============================================================
# Cases 8-10: remote directory -> local
# ============================================================
echo ""
echo "--- Case: remote dir -> local existing directory ---"
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
echo "--- Case: remote dir -> local non-existent path ---"
rm -rf "$WORKDIR/local-dst/created-remote-dir"
run_filetool "${REMOTE}${WORKDIR}/remote-src" "$WORKDIR/local-dst/created-remote-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/created-remote-dir/file1.txt" \
    "remote dir -> local non-existent: file1 copied"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/created-remote-dir/file2.txt" \
    "remote dir -> local non-existent: file2 copied"

echo ""
echo "--- Case: remote dir -> existing local file (error) ---"
run_filetool "${REMOTE}${WORKDIR}/remote-src" "$WORKDIR/local-dst/existing-file.txt" && \
    fail "remote dir -> local file: should have errored" || \
    pass "remote dir -> local file: correctly errors"

# ============================================================
# Case 11: multi-source local -> remote directory
# ============================================================
echo ""
echo "--- Case: multi-source local -> remote dir ---"
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
echo "--- Case: (error): multi-source local -> remote file ---"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt" && \
    fail "multi src local->remote file: should have errored" || \
    pass "multi src local->remote file: correctly errors"

echo ""
echo "--- Case: (mixed): local file + local dir -> remote dir ---"
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
echo "--- Case: multi-source remote -> local dir ---"
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
echo "--- Case: (error): multi-source remote -> local file ---"
run_filetool \
    "${REMOTE}${WORKDIR}/remote-src/file1.txt" \
    "${REMOTE}${WORKDIR}/remote-src/file2.txt" \
    "$WORKDIR/local-dst/existing-file.txt" && \
    fail "multi src remote->local file: should have errored" || \
    pass "multi src remote->local file: correctly errors"

echo ""
echo "--- Case: (mixed): remote file + remote dir -> local dir ---"
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
echo "--- Case: Cross-remote: IP-to-IP should error ---"
run_filetool "${REMOTE}${WORKDIR}/remote-src/file1.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/should-not-exist" && \
    fail "cross-remote copy: should have errored" || \
    pass "cross-remote copy: correctly errors"

# ============================================================
# -f / --force flag
# ============================================================
echo ""
echo "--- Case: Force flag (-f) ---"
rm -f "$WORKDIR/remote-dst/force-test.txt"
createtext "$WORKDIR/remote-dst/force-test.txt" "original content"
createtext "$WORKDIR/local-src/force-test.txt" "new content"
run_filetool -f "$WORKDIR/local-src/force-test.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/force-test.txt"
assert_file_eq "$WORKDIR/local-src/force-test.txt" \
    "$WORKDIR/remote-dst/force-test.txt" \
    "-f flag overwrites without prompting"

echo ""
echo "--- Case: Force flag (--force) ---"
rm -f "$WORKDIR/remote-dst/force-test2.txt"
createtext "$WORKDIR/remote-dst/force-test2.txt" "original"
createtext "$WORKDIR/local-src/force-test2.txt" "new content 2"
run_filetool --force "$WORKDIR/local-src/force-test2.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/force-test2.txt"
assert_file_eq "$WORKDIR/local-src/force-test2.txt" \
    "$WORKDIR/remote-dst/force-test2.txt" \
    "--force flag overwrites without prompting"

# ============================================================
# -- end-of-options delimiter
# ============================================================
echo ""
echo "--- Case: -- delimiter: local file with leading dash -> remote dir ---"
rm -f "$WORKDIR/remote-dst/existing-dir/-leading-dash.txt"
run_filetool -- "$WORKDIR/local-src/-leading-dash.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/-leading-dash.txt" \
    "$WORKDIR/remote-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter: file starting with - copied to remote"

echo ""
echo "--- Case: -- delimiter: local file with leading double-dash -> remote dir ---"
rm -f "$WORKDIR/remote-dst/existing-dir/--leading-double-dash.txt"
run_filetool -- "$WORKDIR/local-src/--leading-double-dash.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/--leading-double-dash.txt" \
    "$WORKDIR/remote-dst/existing-dir/--leading-double-dash.txt" \
    "-- delimiter: file starting with -- copied to remote"

echo ""
echo "--- Case: -- delimiter: remote file with leading dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/-leading-dash.txt"
run_filetool -- "${REMOTE}${WORKDIR}/remote-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter: remote file starting with - copied to local"

echo ""
echo "--- Case: -- delimiter: remote file with leading double-dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt"
run_filetool -- "${REMOTE}${WORKDIR}/remote-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt" \
    "-- delimiter: remote file starting with -- copied to local"

echo ""
echo "--- Case: -- delimiter with -f: local dash-file -> remote, force overwrite ---"
createtext "$WORKDIR/remote-dst/existing-dir/-leading-dash.txt" "old dash file content"
run_filetool -f -- "$WORKDIR/local-src/-leading-dash.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/-leading-dash.txt" \
    "$WORKDIR/remote-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter with -f: overwrites dash-prefixed file"

# ============================================================
# Symlink reproduction tests
# ============================================================
echo ""
echo "--- Case: Symlinks: local dir -> remote, verify targets preserved ---"
rm -rf "$WORKDIR/remote-dst/symlinks"
run_filetool "$WORKDIR/local-symlinks" "${REMOTE}${WORKDIR}/remote-dst/symlinks"
assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/remote-dst/symlinks" \
    "local symlinks -> remote: targets match"

echo ""
echo "--- Case: Symlinks: remote dir -> local, verify targets preserved ---"
rm -rf "$WORKDIR/local-dst/remote-symlinks"
run_filetool "${REMOTE}${WORKDIR}/remote-symlinks" "$WORKDIR/local-dst/remote-symlinks"
assert_symlinks_match "$WORKDIR/remote-symlinks" "$WORKDIR/local-dst/remote-symlinks" \
    "remote symlinks -> local: targets match"

echo ""
echo "--- Case: Symlinks: local -> remote -> local roundtrip ---"
rm -rf "$WORKDIR/local-dst/symlinks-rt"
run_filetool "${REMOTE}${WORKDIR}/remote-dst/symlinks" "$WORKDIR/local-dst/symlinks-rt"
assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-rt" \
    "symlinks local->remote->local roundtrip: targets match"
# Also diff -r to verify regular file contents intact
if diff -r "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-rt" >/dev/null 2>&1; then
    pass "symlinks roundtrip: diff -r matches (files intact)"
else
    fail "symlinks roundtrip: diff -r shows differences"
fi

echo ""
echo "--- Case: Symlinks: single local symlink -> remote, verify target preserved ---"
# Standalone (non-directory) symlink copy: symlink with existing target
ln -sf "hosts-target" "$WORKDIR/standalone-link"
createtext "$WORKDIR/hosts-target" "hosts target content"
rm -rf "$WORKDIR/remote-dst/standalone-link"
run_filetool "$WORKDIR/standalone-link" "${REMOTE}${WORKDIR}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/remote-dst/standalone-link" 2>/dev/null)
    if [ "$dst_target" = "hosts-target" ]; then
        pass "single local symlink -> remote: target preserved as symlink"
    elif [ -z "$dst_target" ]; then
        fail "single local symlink -> remote: NOT a symlink (symlink reproduction missing for single-file copy)"
    else
        fail "single local symlink -> remote: wrong target '$dst_target' expected 'hosts-target'"
    fi
else
    fail "single local symlink -> remote: copy failed (exit $ret)"
fi

echo ""
echo "--- Case: Symlinks: single remote symlink -> local, verify target preserved ---"
# Ensure a symlink exists on the remote side to copy back as standalone
rm -rf "$WORKDIR/remote-dst/remote-standalone-link"
ln -sf "rfile-target" "$WORKDIR/remote-symlinks/remote-standalone-link"
createtext "$WORKDIR/remote-symlinks/rfile-target" "remote standalone target"
run_filetool "$WORKDIR/remote-symlinks/remote-standalone-link" "${REMOTE}${WORKDIR}/remote-dst/"
# Now copy that remote symlink as a standalone source back to local
rm -rf "$WORKDIR/local-dst/remote-standalone-link"
run_filetool "${REMOTE}${WORKDIR}/remote-dst/remote-standalone-link" "$WORKDIR/local-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/local-dst/remote-standalone-link" 2>/dev/null)
    if [ "$dst_target" = "rfile-target" ]; then
        pass "single remote symlink -> local: target preserved as symlink"
    elif [ -z "$dst_target" ]; then
        fail "single remote symlink -> local: NOT a symlink (symlink reproduction missing for single-file copy)"
    else
        fail "single remote symlink -> local: wrong target '$dst_target' expected 'rfile-target'"
    fi
else
    fail "single remote symlink -> local: copy failed (exit $ret)"
fi

echo ""
echo "--- Case: Symlinks: single local symlink (broken target) -> remote ---"
# Symlink whose target does not exist (dangling symlink) should still copy as symlink
ln -sf "/nonexistent/target/path" "$WORKDIR/broken-link"
rm -rf "$WORKDIR/remote-dst/broken-link"
run_filetool "$WORKDIR/broken-link" "${REMOTE}${WORKDIR}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/remote-dst/broken-link" 2>/dev/null)
    if [ "$dst_target" = "/nonexistent/target/path" ]; then
        pass "single local broken symlink -> remote: target preserved as symlink"
    elif [ -z "$dst_target" ]; then
        fail "single local broken symlink -> remote: NOT a symlink (broken symlink not copied)"
    else
        fail "single local broken symlink -> remote: wrong target '$dst_target' expected '/nonexistent/target/path'"
    fi
else
    fail "single local broken symlink -> remote: copy failed — broken symlinks should be copyable (exit $ret)"
fi

# ============================================================
# Non-existent source error
# ============================================================
echo ""
echo "--- Case: Non-existent source ---"
run_filetool "$WORKDIR/local-src/does-not-exist.txt" \
    "${REMOTE}${WORKDIR}/remote-dst/should-not-be-created" && \
    fail "non-existent source: should have errored" || \
    pass "non-existent source: correctly errors"

# ============================================================
# Trailing slash: source is not a directory
# ============================================================
echo ""
echo "--- Case: Source file with trailing slash (error) ---"
run_filetool_stderr "$WORKDIR/local-src/file1.txt/" "${REMOTE}${WORKDIR}/remote-dst/should-not-exist"
assert_error $? "source file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -q "is not a directory"; then
    pass "source file with trailing slash: error message says 'is not a directory'"
else
    fail "source file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: Destination file with trailing slash (error) ---"
run_filetool_stderr "$WORKDIR/local-src" "${REMOTE}${WORKDIR}/remote-dst/existing-file.txt/"
assert_error $? "destination file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -q "is not a directory"; then
    pass "destination file with trailing slash: error message says 'is not a directory'"
else
    fail "destination file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

# ============================================================
# --ls trailing slash validation
# ============================================================

echo ""
echo "--- Case: --ls: local file with trailing slash ---"
run_filetool_stderr --ls "$WORKDIR/local-src/file1.txt/"
ret=$?
assert_error $ret "--ls local file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -q "is not a directory"; then
    pass "--ls local file with trailing slash: error message says 'is not a directory'"
else
    fail "--ls local file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: --ls: remote file with trailing slash ---"
run_filetool_stderr --ls "${REMOTE}${WORKDIR}/remote-src/file1.txt/"
ret=$?
assert_error $ret "--ls remote file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -q "is not a directory"; then
    pass "--ls remote file with trailing slash: error message says 'is not a directory'"
else
    fail "--ls remote file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: --ls: local directory with trailing slash (should succeed) ---"
run_filetool --ls "$WORKDIR/local-src/subdir/" > /dev/null
assert_success $? "--ls local dir with trailing slash: succeeds"

echo ""
echo "--- Case: --ls: remote directory with trailing slash (should succeed) ---"
run_filetool --ls "${REMOTE}${WORKDIR}/remote-src/subdir/" > /dev/null
assert_success $? "--ls remote dir with trailing slash: succeeds"

echo ""
echo "--- Case: --ls: multi-path, local file with trailing slash ---"
run_filetool_stderr --ls "$WORKDIR/local-src/file1.txt/" "$WORKDIR/local-src/file2.txt"
ret=$?
assert_error $ret "--ls multi with trailing-slash file: errors"
if echo "$FILE_TOOL_STDERR" | grep -q "is not a directory"; then
    pass "--ls multi with trailing-slash file: error message says 'is not a directory'"
else
    fail "--ls multi with trailing-slash file: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: --ls: multi-path, remote file with trailing slash ---"
run_filetool_stderr --ls "${REMOTE}${WORKDIR}/remote-src/file1.txt/" "$WORKDIR/local-src/file2.txt"
ret=$?
assert_error $ret "--ls multi with remote trailing-slash file: errors"
if echo "$FILE_TOOL_STDERR" | grep -q "is not a directory"; then
    pass "--ls multi with remote trailing-slash file: error message says 'is not a directory'"
else
    fail "--ls multi with remote trailing-slash file: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi


# ==== --ls symlink-to-directory trailing slash behavior ====
# Set up symlink test data for --ls
rm -rf "$WORKDIR/ls-symlink-test"
mkdir -p "$WORKDIR/ls-symlink-test/targetdir"
createtext "$WORKDIR/ls-symlink-test/targetdir/nested.txt" "nested file in target"
ln -s "targetdir" "$WORKDIR/ls-symlink-test/link-to-dir"

echo ""
echo "--- Case: --ls: -l on local symlink-to-dir shows entry, does not list contents ---"
run_filetool_ls_capture -l "$WORKDIR/ls-symlink-test/link-to-dir"
if [ $? -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "link-to-dir -> targetdir" && ! echo "$LS_STDOUT" | grep -q "nested.txt"; then
        pass "--ls -l local symlink-to-dir: shows symlink entry, does not list contents"
    else
        fail "--ls -l local symlink-to-dir: expected symlink entry only, got: $LS_STDOUT"
    fi
else
    fail "--ls -l local symlink-to-dir: failed, got: $LS_STDOUT"
fi

echo ""
echo "--- Case: --ls: trailing slash on local symlink-to-dir lists contents ---"
run_filetool_ls_capture -l "$WORKDIR/ls-symlink-test/link-to-dir/"
if [ $? -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "nested.txt" && ! echo "$LS_STDOUT" | grep -q "link-to-dir ->"; then
        pass "--ls -l local symlink-to-dir/: follows symlink, lists contents"
    else
        fail "--ls -l local symlink-to-dir/: expected directory contents, got: $LS_STDOUT"
    fi
else
    fail "--ls -l local symlink-to-dir/: failed, got: $LS_STDOUT"
fi

echo ""
echo "--- Case: --ls: -l on remote symlink-to-dir shows entry, does not list contents ---"
run_filetool_ls_capture -l "${REMOTE}${WORKDIR}/ls-symlink-test/link-to-dir"
if [ $? -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "link-to-dir -> targetdir" && ! echo "$LS_STDOUT" | grep -q "nested.txt"; then
        pass "--ls -l remote symlink-to-dir: shows symlink entry, does not list contents"
    else
        fail "--ls -l remote symlink-to-dir: expected symlink entry only, got: $LS_STDOUT"
    fi
else
    fail "--ls -l remote symlink-to-dir: failed, got: $LS_STDOUT"
fi

echo ""
echo "--- Case: --ls: trailing slash on remote symlink-to-dir lists contents ---"
run_filetool_ls_capture -l "${REMOTE}${WORKDIR}/ls-symlink-test/link-to-dir/"
if [ $? -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "nested.txt" && ! echo "$LS_STDOUT" | grep -q "link-to-dir ->"; then
        pass "--ls -l remote symlink-to-dir/: follows symlink, lists contents"
    else
        fail "--ls -l remote symlink-to-dir/: expected directory contents, got: $LS_STDOUT"
    fi
else
    fail "--ls -l remote symlink-to-dir/: failed, got: $LS_STDOUT"
fi
# ============================================================
# Local → Local tests (mirror all local↔remote patterns)
# ============================================================

echo ""
echo "--- Case: Local→Local: local file -> local directory ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt"
run_filetool "$WORKDIR/local-src/file1.txt" "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "local file -> local dir creates basename"

echo ""
echo "--- Case: Local→Local: local file -> local non-existent path ---"
rm -f "$WORKDIR/local-dst/new-local-file.txt"
run_filetool "$WORKDIR/local-src/file2.txt" "$WORKDIR/local-dst/new-local-file.txt"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/new-local-file.txt" \
    "local file -> local non-existent creates as named"

echo ""
echo "--- Case: Local→Local: local file -> existing local file, no overwrite ---"
cp "$WORKDIR/local-dst/existing-file.txt" "$WORKDIR/local-dst/existing-file.txt.ref"
echo "n" | run_filetool "$WORKDIR/local-src/file1.txt" "$WORKDIR/local-dst/existing-file.txt"
assert_file_eq "$WORKDIR/local-dst/existing-file.txt.ref" \
    "$WORKDIR/local-dst/existing-file.txt" \
    "local file -> existing local file: not overwritten when answering n"

echo ""
echo "--- Case: Local→Local: local file -> existing local file with -f ---"
run_filetool -f "$WORKDIR/local-src/file1.txt" "$WORKDIR/local-dst/existing-file.txt"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/existing-file.txt" \
    "local file -> local file with -f overwrites"

echo ""
echo "--- Case: Local→Local: local dir -> local existing directory ---"
rm -rf "$WORKDIR/local-dst/existing-dir/local-src"
run_filetool "$WORKDIR/local-src" "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/local-src/file1.txt" \
    "local dir -> local dir: file1 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/local-dst/existing-dir/local-src/subdir/nested.txt" \
    "local dir -> local dir: nested file copied"
assert_file_eq "$WORKDIR/local-src/subdir/deep/deep.txt" \
    "$WORKDIR/local-dst/existing-dir/local-src/subdir/deep/deep.txt" \
    "local dir -> local dir: deep file copied"
[ -d "$WORKDIR/local-dst/existing-dir/local-src/emptydir" ] && \
    pass "local dir -> local dir: empty dir present" || \
    fail "local dir -> local dir: empty dir missing"

echo ""
echo "--- Case: Local→Local: local dir -> local non-existent path ---"
rm -rf "$WORKDIR/local-dst/created-local-dir"
run_filetool "$WORKDIR/local-src" "$WORKDIR/local-dst/created-local-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/created-local-dir/file1.txt" \
    "local dir -> local non-existent: file1 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/local-dst/created-local-dir/subdir/nested.txt" \
    "local dir -> local non-existent: nested file copied"

echo ""
echo "--- Case: Local→Local: local dir -> existing local file (error) ---"
run_filetool "$WORKDIR/local-src" "$WORKDIR/local-dst/existing-file.txt" && \
    fail "local dir -> local file: should have errored" || \
    pass "local dir -> local file: correctly errors"

echo ""
echo "--- Case: Local→Local: deep tree local dir -> local dir, verify with diff -r ---"
rm -rf "$WORKDIR/local-dst/deep-tree-local"
run_filetool "$WORKDIR/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree-local"
if diff -r "$WORKDIR/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree-local" >/dev/null 2>&1; then
    pass "deep tree local->local: diff -r matches"
else
    fail "deep tree local->local: diff -r shows differences"
fi
assert_symlinks_match "$WORKDIR/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree-local" \
    "deep tree local->local: symlinks preserved"

echo ""
echo "--- Case: Local→Local: multi-source local files -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt" "$WORKDIR/local-dst/existing-dir/file2.txt"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "multi src local->local: file1 copied"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/existing-dir/file2.txt" \
    "multi src local->local: file2 copied"

echo ""
echo "--- Case: Local→Local: multi-source local files -> local file (error) ---"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/existing-file.txt" && \
    fail "multi src local->local file: should have errored" || \
    pass "multi src local->local file: correctly errors"

echo ""
echo "--- Case: Local→Local: multi-source local file + local dir -> local dir ---"
rm -rf "$WORKDIR/local-dst/existing-dir/subdir" "$WORKDIR/local-dst/existing-dir/file2.txt"
run_filetool \
    "$WORKDIR/local-src/subdir" \
    "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/existing-dir/file2.txt" \
    "multi src local file+dir->local: file2 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/local-dst/existing-dir/subdir/nested.txt" \
    "multi src local file+dir->local: nested file copied"

echo ""
echo "--- Case: Local→Local: -- delimiter, file with leading dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/-leading-dash.txt"
run_filetool -- "$WORKDIR/local-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter local→local: file starting with - copied"

echo ""
echo "--- Case: Local→Local: -- delimiter, file with leading double-dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt"
run_filetool -- "$WORKDIR/local-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt" \
    "-- delimiter local→local: file starting with -- copied"

echo ""
echo "--- Case: Local→Local: symlinks dir -> local dir, verify targets preserved ---"
rm -rf "$WORKDIR/local-dst/symlinks-local"
run_filetool "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-local"
assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-local" \
    "symlinks local->local: targets match"

echo ""
echo "--- Case: Local→Local: single local symlink -> local dir ---"
ln -sf "hosts-target" "$WORKDIR/standalone-link2"
rm -rf "$WORKDIR/local-dst/standalone-link2"
run_filetool "$WORKDIR/standalone-link2" "$WORKDIR/local-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/local-dst/standalone-link2" 2>/dev/null)
    if [ "$dst_target" = "hosts-target" ]; then
        pass "single symlink local->local: target preserved as symlink"
    elif [ -z "$dst_target" ]; then
        fail "single symlink local->local: NOT a symlink"
    else
        fail "single symlink local->local: wrong target '$dst_target'"
    fi
else
    fail "single symlink local->local: copy failed (exit $ret)"
fi

echo ""
echo "--- Case: Local→Local: single broken symlink -> local dir ---"
ln -sf "/nonexistent/target/path" "$WORKDIR/broken-link2"
rm -rf "$WORKDIR/local-dst/broken-link2"
run_filetool "$WORKDIR/broken-link2" "$WORKDIR/local-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/local-dst/broken-link2" 2>/dev/null)
    if [ "$dst_target" = "/nonexistent/target/path" ]; then
        pass "single broken symlink local->local: target preserved as symlink"
    elif [ -z "$dst_target" ]; then
        fail "single broken symlink local->local: NOT a symlink"
    else
        fail "single broken symlink local->local: wrong target '$dst_target'"
    fi
else
    fail "single broken symlink local->local: copy failed (exit $ret)"
fi

# ============================================================
# /proc/version copy tests (md5sum verification)
# ============================================================
LOCAL_PROV_VERSION_MD5=$(md5sum /proc/version | awk '{print $1}')

echo ""
echo "--- Case: /proc/version: local -> local dir, md5sum ---"
rm -f "$WORKDIR/local-dst/existing-dir/version"
run_filetool /proc/version "$WORKDIR/local-dst/existing-dir"
assert_file_eq /proc/version "$WORKDIR/local-dst/existing-dir/version" \
    "local /proc/version -> local dir: file matches"

echo ""
echo "--- Case: /proc/version: local -> remote dir, round-trip md5sum ---"
rm -f "$WORKDIR/remote-dst/existing-dir/version"
run_filetool /proc/version "${REMOTE}${WORKDIR}/remote-dst/existing-dir"
ret=$?
if [ $ret -ne 0 ]; then
    fail "local /proc/version -> remote dir: copy failed (exit $ret)"
else
    # Copy back from remote to a different local path
    rm -f "$WORKDIR/local-dst/version-roundtrip.txt"
    run_filetool "${REMOTE}${WORKDIR}/remote-dst/existing-dir/version" \
        "$WORKDIR/local-dst/version-roundtrip.txt"
    if [ $? -ne 0 ]; then
        fail "local /proc/version -> remote dir: round-trip copy back failed"
    else
        RT_MD5=$(md5sum "$WORKDIR/local-dst/version-roundtrip.txt" | awk '{print $1}')
        assert_eq "$RT_MD5" "$LOCAL_PROV_VERSION_MD5" \
            "local /proc/version -> remote dir: round-trip md5sum matches"
    fi
fi

echo ""
echo "--- Case: /proc/version: remote -> local dir, md5sum vs local ---"
rm -f "$WORKDIR/local-dst/existing-dir/version"
run_filetool "${REMOTE}/proc/version" "$WORKDIR/local-dst/existing-dir"
ret=$?
if [ $ret -ne 0 ]; then
    fail "remote /proc/version -> local dir: copy failed (exit $ret)"
else
    REMOTE_MD5=$(md5sum "$WORKDIR/local-dst/existing-dir/version" | awk '{print $1}')
    assert_eq "$REMOTE_MD5" "$LOCAL_PROV_VERSION_MD5" \
        "remote /proc/version -> local dir: md5sum matches local /proc/version"
fi

echo ""
echo "--- Case: /proc/version: remote -> local specific path, md5sum vs local ---"
rm -f "$WORKDIR/local-dst/remote-proc-version.txt"
run_filetool "${REMOTE}/proc/version" "$WORKDIR/local-dst/remote-proc-version.txt"
ret=$?
if [ $ret -ne 0 ]; then
    fail "remote /proc/version -> local specific path: copy failed (exit $ret)"
else
    REMOTE2_MD5=$(md5sum "$WORKDIR/local-dst/remote-proc-version.txt" | awk '{print $1}')
    assert_eq "$REMOTE2_MD5" "$LOCAL_PROV_VERSION_MD5" \
        "remote /proc/version -> local specific path: md5sum matches local /proc/version"
fi

# ============================================================
# Special file skipping tests
# ============================================================

SPECIAL_DIR="$WORKDIR/special-files"
mkdir -p "$SPECIAL_DIR"

echo ""
echo "--- Case: special files: character device /dev/null -> local dir ---"
run_filetool_stderr /dev/null "$WORKDIR/local-dst/existing-dir/"
ret=$?
assert_success $ret "char device /dev/null: exit code 0 (skipped)"
if echo "$FILE_TOOL_STDERR" | grep -q "skipping character device"; then
    pass "char device /dev/null: warning says 'skipping character device'"
else
    fail "char device /dev/null: expected 'skipping character device' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: special files: character device /dev/null -> remote dir ---"
run_filetool_stderr /dev/null "${REMOTE}${WORKDIR}/remote-dst/existing-dir/"
ret=$?
assert_success $ret "char device -> remote: exit code 0 (skipped)"
if echo "$FILE_TOOL_STDERR" | grep -q "skipping character device"; then
    pass "char device -> remote: warning says 'skipping character device'"
else
    fail "char device -> remote: expected 'skipping character device' in stderr, got: $FILE_TOOL_STDERR"
fi

mkfifo "$SPECIAL_DIR/test-fifo" 2>/dev/null
if [ -p "$SPECIAL_DIR/test-fifo" ]; then
    echo ""
    echo "--- Case: special files: FIFO -> local dir ---"
    run_filetool_stderr "$SPECIAL_DIR/test-fifo" "$WORKDIR/local-dst/existing-dir/"
    ret=$?
    assert_success $ret "FIFO: exit code 0 (skipped)"
    if echo "$FILE_TOOL_STDERR" | grep -q "skipping FIFO"; then
        pass "FIFO: warning says 'skipping FIFO'"
    else
        fail "FIFO: expected 'skipping FIFO' in stderr, got: $FILE_TOOL_STDERR"
    fi

    echo ""
    echo "--- Case: special files: FIFO -> remote dir ---"
    run_filetool_stderr "$SPECIAL_DIR/test-fifo" "${REMOTE}${WORKDIR}/remote-dst/existing-dir/"
    ret=$?
    assert_success $ret "FIFO -> remote: exit code 0 (skipped)"
    if echo "$FILE_TOOL_STDERR" | grep -q "skipping FIFO"; then
        pass "FIFO -> remote: warning says 'skipping FIFO'"
    else
        fail "FIFO -> remote: expected 'skipping FIFO' in stderr, got: $FILE_TOOL_STDERR"
    fi

    echo ""
    echo "--- Case: special files: directory containing FIFO -> local dir ---"
    createtext "$SPECIAL_DIR/regular.txt" "regular file alongside special file"
    mkfifo "$SPECIAL_DIR/dir-fifo" 2>/dev/null
    rm -rf "$WORKDIR/local-dst/existing-dir/special-files"
    run_filetool_stderr "$SPECIAL_DIR" "$WORKDIR/local-dst/existing-dir/"
    ret=$?
    assert_success $ret "dir with FIFO: exit code 0"
    assert_file_eq "$SPECIAL_DIR/regular.txt" \
        "$WORKDIR/local-dst/existing-dir/special-files/regular.txt" \
        "dir with FIFO: regular file still copied"
    if echo "$FILE_TOOL_STDERR" | grep -q "skipping FIFO"; then
        pass "dir with FIFO: warning says 'skipping FIFO'"
    else
        fail "dir with FIFO: expected 'skipping FIFO' in stderr, got: $FILE_TOOL_STDERR"
    fi

    echo ""
    echo "--- Case: special files: directory containing FIFO -> remote dir ---"
    run_filetool_stderr "$SPECIAL_DIR" "${REMOTE}${WORKDIR}/remote-dst/special-dst"
    ret=$?
    assert_success $ret "dir with FIFO -> remote: exit code 0"
    if echo "$FILE_TOOL_STDERR" | grep -q "skipping FIFO"; then
        pass "dir with FIFO -> remote: warning says 'skipping FIFO'"
    else
        fail "dir with FIFO -> remote: expected 'skipping FIFO' in stderr, got: $FILE_TOOL_STDERR"
    fi
else
    echo ""
    echo "--- Case: special files: FIFO tests SKIPPED (filesystem does not support FIFOs) ---"
fi

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

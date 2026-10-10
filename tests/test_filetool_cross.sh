#!/bin/bash
# Cross-machine cooledit --filetool test suite.
#
# Runs the full set of cases from test_filetool.sh with the CLIENT on this
# machine and a remotefs SERVER on a separate (Linux) host. The two machines
# do NOT share a filesystem, so every remote-side artifact is verified by
# round-tripping it back to the client and comparing locally, following the
# pattern of test_filetool_win32.sh.
#
# Usage:  test_filetool_cross.sh [remote-host-ip]
#
# Prerequisites:
#   - a remotefs server is already running on <remote-host-ip> (e.g. with
#     "remotefs --no-crypto <listen-ip> <allow-range>").
#   - the client's ~/.cedit/.password contains an entry for <remote-host-ip>.
SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"
COOLEDIT="$(cd "$SCRIPTDIR/../editor" && pwd)/cooledit"
GETFATTR="$SCRIPTDIR/../getfattr-portable"
MKJUNCTION="$SCRIPTDIR/../mkjunction"
VALGRIND="valgrind"
VALGRIND_FLAGS="--leak-check=full --show-leak-kinds=all --error-exitcode=42"

REMOTE_HOST="${1:-10.0.2.2}"
REMOTE="${REMOTE_HOST}:"
# Server-side (Linux) base directory. Unique per run so no stale state leaks
# between runs. The client addresses it as ${REMOTE}${R_BASE}/...
R_BASE="/tmp/remotefs-cross-$$"
REMOTE_BASE="${REMOTE}${R_BASE}"

# Optional: md5 of the *remote* machine's /proc/1/cmdline, for the
# "remote -> local" procfs tests. Computed by whoever launches this script.
REMOTE_PROC_CMDLINE_MD5="${REMOTE_PROC_CMDLINE_MD5:-}"

WORKDIR_BASE="$SCRIPTDIR/work-dir"
VGLOG_DIR="$WORKDIR_BASE/valgrind-logs"

if test x"`uname`" = xFreeBSD ; then
    INUSE_CLIENT_LIMIT=250000
else
    INUSE_CLIENT_LIMIT=400
fi
PASSED=0
FAILED=0
VGLOG_COUNTER=0
RT_COUNTER=0

WORKDIR=""
TMPDIR_CLEANUP=()

cleanup() {
    for d in "${TMPDIR_CLEANUP[@]}"; do
        rm -rf "$d" 2>/dev/null || true
    done
}

trap cleanup EXIT

# --- helpers ---

tmpdir() {
    local d
    d=$(mktemp -d "$WORKDIR_BASE/filetool-cross-XXXXXX")
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
    if ! cmp "$1" "$2" >/dev/null 2>&1; then
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

find_junctions() {
    local dir="$1"
    find "$dir" -type l 2>/dev/null | while read -r link; do
        local val
        val=$("$GETFATTR" -h -n trusted.windows.junction "$link" --only-values 2>/dev/null)
        [ "$val" = "1" ] && echo "$link"
    done | sort
}

assert_junctions_match() {
    local srcdir="$1" dstdir="$2" desc="$3"
    local ok=1 src_target dst_target relpath dstlink dst_xattr
    while IFS= read -r link; do
        relpath="${link#$srcdir/}"
        dstlink="$dstdir/$relpath"
        if [ ! -L "$dstlink" ]; then
            fail "$desc: $relpath missing or not a symlink at dst"; ok=0; continue
        fi
        dst_target=$(readlink "$dstlink")
        src_target=$(readlink "$link")
        if [ "$src_target" != "$dst_target" ]; then
            fail "$desc: $relpath target '$dst_target' != '$src_target'"; ok=0
        fi
        dst_xattr=$("$GETFATTR" -h -n trusted.windows.junction "$dstlink" --only-values 2>/dev/null)
        if [ "$dst_xattr" != "1" ]; then
            fail "$desc: $relpath missing junction xattr at dst (got '$dst_xattr')"; ok=0
        fi
    done < <(find_junctions "$srcdir")
    [ "$ok" -eq 1 ] && pass "$desc"
}

assert_is_junction() {
    local path="$1" target="$2" desc="$3"
    local actual_target xattr_val ok=1
    if [ ! -L "$path" ]; then
        fail "$desc: not a symlink"; return 1
    fi
    actual_target=$(readlink "$path")
    if [ "$actual_target" != "$target" ]; then
        fail "$desc: target '$actual_target' != '$target'"; ok=0
    fi
    xattr_val=$("$GETFATTR" -h -n trusted.windows.junction "$path" --only-values 2>/dev/null)
    if [ "$xattr_val" != "1" ]; then
        fail "$desc: missing junction xattr (got '$xattr_val')"; ok=0
    fi
    [ "$ok" -eq 1 ] && pass "$desc"
}

# Run cooledit --filetool under valgrind. Returns cooledit exit code.
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
        if [ -n "$inuse" ] && [ "$inuse" -gt "$INUSE_CLIENT_LIMIT" ] 2>/dev/null; then
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
        if [ -n "$inuse" ] && [ "$inuse" -gt "$INUSE_CLIENT_LIMIT" ] 2>/dev/null; then
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

# Round-trip a remote file back to local and compare content with a local ref.
assert_remote_file_eq() {
    local ref="$1" remote="$2" desc="$3"
    RT_COUNTER=$((RT_COUNTER + 1))
    local rt="$WORKDIR/roundtrip/rt-$RT_COUNTER"
    rm -f "$rt"
    run_filetool "$remote" "$rt" >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        fail "$desc: round-trip copy back failed"
        return 1
    fi
    assert_file_eq "$ref" "$rt" "$desc"
}

# Round-trip a remote directory to a local path (dest is created fresh).
remote_roundtrip_dir() {
    local remote="$1" dest="$2"
    rm -rf "$dest"
    run_filetool "$remote" "$dest" >/dev/null 2>&1
    return $?
}

# ============================================================
# Setup
# ============================================================
echo "=== cooledit --filetool Cross-Machine Test Suite ==="
echo "Client: $(uname -s), Server: ${REMOTE_HOST} (${R_BASE})"
echo ""

mkdir -p "$WORKDIR_BASE" "$VGLOG_DIR"
WORKDIR=$(tmpdir)
echo "Test workspace: $WORKDIR"

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

# Remote directories (mirrors that will be pushed to the server)
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

# Junction test data (local side)
mkdir -p "$WORKDIR/local-junctions/subdir"
createtext "$WORKDIR/local-junctions/regular.txt" "regular file in junction test dir"
createtext "$WORKDIR/local-junctions/subdir/nested.txt" "nested file in junction test dir"
ln -s "regular.txt" "$WORKDIR/local-junctions/link-to-file"
ln -s "subdir" "$WORKDIR/local-junctions/link-to-dir"
"$MKJUNCTION" "regular.txt" "$WORKDIR/local-junctions/junction-to-file"
"$MKJUNCTION" "subdir" "$WORKDIR/local-junctions/junction-to-dir"
"$MKJUNCTION" "some-target" "$WORKDIR/standalone-junction1"
"$MKJUNCTION" "/absolute/target/path" "$WORKDIR/standalone-junction2"

# Junction test data (remote side)
mkdir -p "$WORKDIR/remote-junctions/subdir"
createtext "$WORKDIR/remote-junctions/rfile.txt" "remote file in junction test dir"
createtext "$WORKDIR/remote-junctions/subdir/rnested.txt" "remote nested file in junction test dir"
ln -s "rfile.txt" "$WORKDIR/remote-junctions/rlink-to-file"
ln -s "subdir" "$WORKDIR/remote-junctions/rlink-to-dir"
"$MKJUNCTION" "rfile.txt" "$WORKDIR/remote-junctions/rjunction-to-file"
"$MKJUNCTION" "subdir" "$WORKDIR/remote-junctions/rjunction-to-dir"
"$MKJUNCTION" "remote-target" "$WORKDIR/remote-junctions/remote-standalone-junction"

# --ls symlink-to-directory test data (used for both local and remote --ls)
mkdir -p "$WORKDIR/ls-symlink-test/targetdir"
createtext "$WORKDIR/ls-symlink-test/targetdir/nested.txt" "nested file in target"
ln -s "targetdir" "$WORKDIR/ls-symlink-test/link-to-dir"

mkdir -p "$WORKDIR/roundtrip"

# ============================================================
# Push remote-side mirrors to the server
# ============================================================
echo "Pushing remote-side data to server..."
mkdir -p "$WORKDIR/empty-staging"
run_filetool "$WORKDIR/empty-staging" "${REMOTE}${R_BASE}" >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: cannot create remote base dir on ${REMOTE_HOST}. Is remotefs running?"
    exit 1
fi
run_filetool "$WORKDIR/remote-src" "${REMOTE}${R_BASE}/" >/dev/null 2>&1 || \
    { echo "ERROR: failed to push remote-src"; exit 1; }
run_filetool "$WORKDIR/remote-dst" "${REMOTE}${R_BASE}/" >/dev/null 2>&1 || \
    { echo "ERROR: failed to push remote-dst"; exit 1; }
run_filetool "$WORKDIR/remote-symlinks" "${REMOTE}${R_BASE}/" >/dev/null 2>&1 || \
    { echo "ERROR: failed to push remote-symlinks"; exit 1; }
run_filetool "$WORKDIR/remote-junctions" "${REMOTE}${R_BASE}/" >/dev/null 2>&1 || \
    { echo "ERROR: failed to push remote-junctions"; exit 1; }
run_filetool "$WORKDIR/ls-symlink-test" "${REMOTE}${R_BASE}/" >/dev/null 2>&1 || \
    { echo "ERROR: failed to push ls-symlink-test"; exit 1; }
echo "Remote data pushed."

echo ""
echo "=== Test Cases ==="
echo ""

# ============================================================
# Cases 1-2: local file -> remote
# ============================================================
echo "--- Case: local file -> remote directory ---"
run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE_BASE}/remote-dst/existing-dir"
assert_remote_file_eq "$WORKDIR/local-src/file1.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/file1.txt" \
    "local file -> remote dir creates basename"

echo ""
echo "--- Case: local file -> remote non-existent path ---"
run_filetool "$WORKDIR/local-src/file2.txt" "${REMOTE_BASE}/remote-dst/newfile.txt"
assert_remote_file_eq "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_BASE}/remote-dst/newfile.txt" \
    "local file -> remote non-existent creates as named"

echo ""
echo "--- Case: (overwrite): local file -> existing remote file ---"
echo "n" | run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE_BASE}/remote-dst/existing-file.txt"
assert_remote_file_eq "$WORKDIR/remote-dst/existing-file.txt" \
    "${REMOTE_BASE}/remote-dst/existing-file.txt" \
    "local file -> existing remote file: not overwritten when answering n"

echo ""
echo "--- Case: (force overwrite): local file -> existing remote file with -f ---"
run_filetool -f "$WORKDIR/local-src/file1.txt" "${REMOTE_BASE}/remote-dst/existing-file.txt"
assert_remote_file_eq "$WORKDIR/local-src/file1.txt" \
    "${REMOTE_BASE}/remote-dst/existing-file.txt" \
    "local file -> remote file with -f overwrites"

# ============================================================
# Cases 3-4: remote file -> local
# ============================================================
echo ""
echo "--- Case: remote file -> local directory ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt"
run_filetool "${REMOTE_BASE}/remote-src/file1.txt" "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "remote file -> local dir creates basename"

echo ""
echo "--- Case: remote file -> local non-existent path ---"
rm -f "$WORKDIR/local-dst/new-remote-file.txt"
run_filetool "${REMOTE_BASE}/remote-src/file2.txt" "$WORKDIR/local-dst/new-remote-file.txt"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/new-remote-file.txt" \
    "remote file -> local non-existent creates as named"

echo ""
echo "--- Case: (force overwrite): remote file -> existing local file with -f ---"
cp "$WORKDIR/local-src/file2.txt" "$WORKDIR/local-dst/tmp-overwrite.txt"
run_filetool -f "${REMOTE_BASE}/remote-src/file1.txt" "$WORKDIR/local-dst/tmp-overwrite.txt"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/tmp-overwrite.txt" \
    "remote file -> local file with -f overwrites"

# ============================================================
# Cases 5-7: local directory -> remote
# ============================================================
echo ""
echo "--- Case: local dir -> remote existing directory ---"
run_filetool "$WORKDIR/local-src" "${REMOTE_BASE}/remote-dst/existing-dir"
assert_remote_file_eq "$WORKDIR/local-src/file1.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/local-src/file1.txt" \
    "local dir -> remote dir: file1 copied"
assert_remote_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/local-src/subdir/nested.txt" \
    "local dir -> remote dir: nested file copied"
assert_remote_file_eq "$WORKDIR/local-src/subdir/deep/deep.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/local-src/subdir/deep/deep.txt" \
    "local dir -> remote dir: deep file copied"
# empty dir presence: round-trip the local-src dir back and check emptydir
remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/existing-dir/local-src" "$WORKDIR/roundtrip/emptydir-check"
if [ -d "$WORKDIR/roundtrip/emptydir-check/emptydir" ]; then
    pass "local dir -> remote dir: empty dir present"
else
    fail "local dir -> remote dir: empty dir missing"
fi

echo ""
echo "--- Case: local dir -> remote non-existent path ---"
run_filetool "$WORKDIR/local-src" "${REMOTE_BASE}/remote-dst/created-dir"
assert_remote_file_eq "$WORKDIR/local-src/file1.txt" \
    "${REMOTE_BASE}/remote-dst/created-dir/file1.txt" \
    "local dir -> remote non-existent: file1 copied"
assert_remote_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "${REMOTE_BASE}/remote-dst/created-dir/subdir/nested.txt" \
    "local dir -> remote non-existent: nested file copied"

echo ""
echo "--- Case: local dir -> existing remote file (error) ---"
run_filetool "$WORKDIR/local-src" "${REMOTE_BASE}/remote-dst/existing-file.txt" && \
    fail "local dir -> remote file: should have errored" || \
    pass "local dir -> remote file: correctly errors"

echo ""
echo "--- Case: Deep tree: 3-level remote dir -> local dir, verify with diff -r ---"
rm -rf "$WORKDIR/local-dst/deep-tree"
run_filetool "${REMOTE_BASE}/remote-src/deep-tree" "$WORKDIR/local-dst/deep-tree"
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
run_filetool "${REMOTE_BASE}/remote-src" "$WORKDIR/local-dst/existing-dir"
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
run_filetool "${REMOTE_BASE}/remote-src" "$WORKDIR/local-dst/created-remote-dir"
assert_file_eq "$WORKDIR/remote-src/file1.txt" \
    "$WORKDIR/local-dst/created-remote-dir/file1.txt" \
    "remote dir -> local non-existent: file1 copied"
assert_file_eq "$WORKDIR/remote-src/file2.txt" \
    "$WORKDIR/local-dst/created-remote-dir/file2.txt" \
    "remote dir -> local non-existent: file2 copied"

echo ""
echo "--- Case: remote dir -> existing local file (error) ---"
run_filetool "${REMOTE_BASE}/remote-src" "$WORKDIR/local-dst/existing-file.txt" && \
    fail "remote dir -> local file: should have errored" || \
    pass "remote dir -> local file: correctly errors"

# ============================================================
# Case 11: multi-source local -> remote directory
# ============================================================
echo ""
echo "--- Case: multi-source local -> remote dir ---"
run_filetool -f \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir"
assert_remote_file_eq "$WORKDIR/local-src/file1.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/file1.txt" \
    "multi src local->remote: file1 copied"
assert_remote_file_eq "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/file2.txt" \
    "multi src local->remote: file2 copied"

echo ""
echo "--- Case: (error): multi-source local -> remote file ---"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_BASE}/remote-dst/existing-file.txt" && \
    fail "multi src local->remote file: should have errored" || \
    pass "multi src local->remote file: correctly errors"

echo ""
echo "--- Case: (mixed): local file + local dir -> remote dir ---"
run_filetool -f \
    "$WORKDIR/local-src/subdir" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir"
assert_remote_file_eq "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/file2.txt" \
    "multi src local file+dir->remote: file2 copied"
assert_remote_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/subdir/nested.txt" \
    "multi src local file+dir->remote: nested file copied"

# ============================================================
# Case 12: multi-source remote -> local directory
# ============================================================
echo ""
echo "--- Case: multi-source remote -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt" "$WORKDIR/local-dst/existing-dir/file2.txt"
run_filetool \
    "${REMOTE_BASE}/remote-src/file1.txt" \
    "${REMOTE_BASE}/remote-src/file2.txt" \
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
    "${REMOTE_BASE}/remote-src/file1.txt" \
    "${REMOTE_BASE}/remote-src/file2.txt" \
    "$WORKDIR/local-dst/existing-file.txt" && \
    fail "multi src remote->local file: should have errored" || \
    pass "multi src remote->local file: correctly errors"

echo ""
echo "--- Case: (mixed): remote file + remote dir -> local dir ---"
rm -rf "$WORKDIR/local-dst/existing-dir/remote-src" "$WORKDIR/local-dst/existing-dir/file1.txt"
run_filetool \
    "${REMOTE_BASE}/remote-src/subdir" \
    "${REMOTE_BASE}/remote-src/file1.txt" \
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
run_filetool "${REMOTE_BASE}/remote-src/file1.txt" \
    "${REMOTE_BASE}/remote-dst/should-not-exist" && \
    fail "cross-remote copy: should have errored" || \
    pass "cross-remote copy: correctly errors"

# ============================================================
# -f / --force flag
# ============================================================
echo ""
echo "--- Case: Force flag (-f) ---"
createtext "$WORKDIR/local-src/force-test.txt" "new content"
run_filetool -f "$WORKDIR/local-src/force-test.txt" \
    "${REMOTE_BASE}/remote-dst/force-test.txt"
assert_remote_file_eq "$WORKDIR/local-src/force-test.txt" \
    "${REMOTE_BASE}/remote-dst/force-test.txt" \
    "-f flag overwrites without prompting"

echo ""
echo "--- Case: Force flag (--force) ---"
createtext "$WORKDIR/local-src/force-test2.txt" "new content 2"
run_filetool --force "$WORKDIR/local-src/force-test2.txt" \
    "${REMOTE_BASE}/remote-dst/force-test2.txt"
assert_remote_file_eq "$WORKDIR/local-src/force-test2.txt" \
    "${REMOTE_BASE}/remote-dst/force-test2.txt" \
    "--force flag overwrites without prompting"

# ============================================================
# -- end-of-options delimiter
# ============================================================
echo ""
echo "--- Case: -- delimiter: local file with leading dash -> remote dir ---"
run_filetool -- "$WORKDIR/local-src/-leading-dash.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir"
assert_remote_file_eq "$WORKDIR/local-src/-leading-dash.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter: file starting with - copied to remote"

echo ""
echo "--- Case: -- delimiter: local file with leading double-dash -> remote dir ---"
run_filetool -- "$WORKDIR/local-src/--leading-double-dash.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir"
assert_remote_file_eq "$WORKDIR/local-src/--leading-double-dash.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/--leading-double-dash.txt" \
    "-- delimiter: file starting with -- copied to remote"

echo ""
echo "--- Case: -- delimiter: remote file with leading dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/-leading-dash.txt"
run_filetool -- "${REMOTE_BASE}/remote-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter: remote file starting with - copied to local"

echo ""
echo "--- Case: -- delimiter: remote file with leading double-dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt"
run_filetool -- "${REMOTE_BASE}/remote-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/remote-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt" \
    "-- delimiter: remote file starting with -- copied to local"

echo ""
echo "--- Case: -- delimiter with -f: local dash-file -> remote, force overwrite ---"
run_filetool -f -- "$WORKDIR/local-src/-leading-dash.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir"
assert_remote_file_eq "$WORKDIR/local-src/-leading-dash.txt" \
    "${REMOTE_BASE}/remote-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter with -f: overwrites dash-prefixed file"

# ============================================================
# Symlink reproduction tests
# ============================================================
echo ""
echo "--- Case: Symlinks: local dir -> remote, verify targets preserved ---"
run_filetool "$WORKDIR/local-symlinks" "${REMOTE_BASE}/remote-dst/symlinks"
remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/symlinks" "$WORKDIR/roundtrip/symlinks"
assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/roundtrip/symlinks" \
    "local symlinks -> remote: targets match"

echo ""
echo "--- Case: Symlinks: remote dir -> local, verify targets preserved ---"
rm -rf "$WORKDIR/local-dst/remote-symlinks"
run_filetool "${REMOTE_BASE}/remote-symlinks" "$WORKDIR/local-dst/remote-symlinks"
assert_symlinks_match "$WORKDIR/remote-symlinks" "$WORKDIR/local-dst/remote-symlinks" \
    "remote symlinks -> local: targets match"

echo ""
echo "--- Case: Symlinks: local -> remote -> local roundtrip ---"
rm -rf "$WORKDIR/local-dst/symlinks-rt"
run_filetool "${REMOTE_BASE}/remote-dst/symlinks" "$WORKDIR/local-dst/symlinks-rt"
assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-rt" \
    "symlinks local->remote->local roundtrip: targets match"
if diff -r "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-rt" >/dev/null 2>&1; then
    pass "symlinks roundtrip: diff -r matches (files intact)"
else
    fail "symlinks roundtrip: diff -r shows differences"
fi

echo ""
echo "--- Case: Symlinks: single local symlink -> remote, verify target preserved ---"
ln -sf "hosts-target" "$WORKDIR/standalone-link"
createtext "$WORKDIR/hosts-target" "hosts target content"
run_filetool "$WORKDIR/standalone-link" "${REMOTE_BASE}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/standalone-link" "$WORKDIR/roundtrip/standalone-link"
    dst_target=$(readlink "$WORKDIR/roundtrip/standalone-link" 2>/dev/null)
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
ln -sf "rfile-target" "$WORKDIR/remote-symlinks/remote-standalone-link"
createtext "$WORKDIR/remote-symlinks/rfile-target" "remote standalone target"
run_filetool "$WORKDIR/remote-symlinks/remote-standalone-link" "${REMOTE_BASE}/remote-dst/"
run_filetool "${REMOTE_BASE}/remote-dst/remote-standalone-link" "$WORKDIR/local-dst/"
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
ln -sf "/nonexistent/target/path" "$WORKDIR/broken-link"
run_filetool "$WORKDIR/broken-link" "${REMOTE_BASE}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/broken-link" "$WORKDIR/roundtrip/broken-link"
    dst_target=$(readlink "$WORKDIR/roundtrip/broken-link" 2>/dev/null)
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
# Junction copy tests
# ============================================================
echo ""
echo "=== Junction Copy Tests ==="

echo ""
echo "--- Case: Junction dir: local -> local (recursive) ---"
rm -rf "$WORKDIR/local-dst/junctions-local"
run_filetool "$WORKDIR/local-junctions" "$WORKDIR/local-dst/junctions-local"
assert_junctions_match "$WORKDIR/local-junctions" "$WORKDIR/local-dst/junctions-local" \
    "junction dir local->local: junctions preserved"
for link in link-to-file link-to-dir; do
    xv=$("$GETFATTR" -h -n trusted.windows.junction "$WORKDIR/local-dst/junctions-local/$link" --only-values 2>/dev/null)
    if [ -z "$xv" ]; then
        pass "junction dir local->local: $link remains plain symlink (no xattr)"
    else
        fail "junction dir local->local: $link got unexpected junction xattr"
    fi
done

echo ""
echo "--- Case: Single junction: local -> local ---"
rm -rf "$WORKDIR/local-dst/standalone-junction1" "$WORKDIR/local-dst/standalone-junction2"
run_filetool "$WORKDIR/standalone-junction1" "$WORKDIR/local-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    assert_is_junction "$WORKDIR/local-dst/standalone-junction1" "some-target" \
        "single junction local->local: junction1 preserved"
else
    fail "single junction local->local: junction1 copy failed (exit $ret)"
fi
run_filetool "$WORKDIR/standalone-junction2" "$WORKDIR/local-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    assert_is_junction "$WORKDIR/local-dst/standalone-junction2" "/absolute/target/path" \
        "single junction local->local: junction2 (absolute target) preserved"
else
    fail "single junction local->local: junction2 copy failed (exit $ret)"
fi

echo ""
echo "--- Case: Junction dir: local -> remote (recursive) ---"
run_filetool "$WORKDIR/local-junctions" "${REMOTE_BASE}/remote-dst/junctions-remote"
remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/junctions-remote" "$WORKDIR/roundtrip/junctions-remote"
assert_junctions_match "$WORKDIR/local-junctions" "$WORKDIR/roundtrip/junctions-remote" \
    "junction dir local->remote: junctions preserved"
for link in link-to-file link-to-dir; do
    xv=$("$GETFATTR" -h -n trusted.windows.junction "$WORKDIR/roundtrip/junctions-remote/$link" --only-values 2>/dev/null)
    if [ -z "$xv" ]; then
        pass "junction dir local->remote: $link remains plain symlink"
    else
        fail "junction dir local->remote: $link got unexpected junction xattr"
    fi
done

echo ""
echo "--- Case: Single junction: local -> remote ---"
run_filetool "$WORKDIR/standalone-junction1" "${REMOTE_BASE}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/standalone-junction1" "$WORKDIR/roundtrip/standalone-junction1"
    assert_is_junction "$WORKDIR/roundtrip/standalone-junction1" "some-target" \
        "single junction local->remote: junction1 preserved"
else
    fail "single junction local->remote: junction1 copy failed (exit $ret)"
fi
run_filetool "$WORKDIR/standalone-junction2" "${REMOTE_BASE}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/standalone-junction2" "$WORKDIR/roundtrip/standalone-junction2"
    assert_is_junction "$WORKDIR/roundtrip/standalone-junction2" "/absolute/target/path" \
        "single junction local->remote: junction2 (absolute target) preserved"
else
    fail "single junction local->remote: junction2 copy failed (exit $ret)"
fi

echo ""
echo "--- Case: Junction dir: remote -> local (recursive) ---"
rm -rf "$WORKDIR/local-dst/junctions-from-remote"
run_filetool "${REMOTE_BASE}/remote-junctions" "$WORKDIR/local-dst/junctions-from-remote"
assert_junctions_match "$WORKDIR/remote-junctions" "$WORKDIR/local-dst/junctions-from-remote" \
    "junction dir remote->local: junctions preserved"
for link in rlink-to-file rlink-to-dir; do
    xv=$("$GETFATTR" -h -n trusted.windows.junction "$WORKDIR/local-dst/junctions-from-remote/$link" --only-values 2>/dev/null)
    if [ -z "$xv" ]; then
        pass "junction dir remote->local: $link remains plain symlink"
    else
        fail "junction dir remote->local: $link got unexpected junction xattr"
    fi
done

echo ""
echo "--- Case: Single junction: remote -> local ---"
rm -rf "$WORKDIR/local-dst/remote-standalone-junction"
run_filetool "${REMOTE_BASE}/remote-junctions/remote-standalone-junction" "$WORKDIR/local-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    assert_is_junction "$WORKDIR/local-dst/remote-standalone-junction" "remote-target" \
        "single junction remote->local: junction preserved"
else
    fail "single junction remote->local: junction copy failed (exit $ret)"
fi

echo ""
echo "--- Case: Junction roundtrip: local -> remote -> local ---"
rm -rf "$WORKDIR/local-dst/junctions-rt"
run_filetool "${REMOTE_BASE}/remote-dst/junctions-remote" "$WORKDIR/local-dst/junctions-rt"
assert_junctions_match "$WORKDIR/local-junctions" "$WORKDIR/local-dst/junctions-rt" \
    "junction roundtrip: junctions preserved"
if diff -r "$WORKDIR/local-junctions" "$WORKDIR/local-dst/junctions-rt" >/dev/null 2>&1; then
    pass "junction roundtrip: diff -r matches (files intact)"
else
    fail "junction roundtrip: diff -r shows differences"
fi

# ============================================================
# Non-existent source error
# ============================================================
echo ""
echo "--- Case: Non-existent source ---"
run_filetool "$WORKDIR/local-src/does-not-exist.txt" \
    "${REMOTE_BASE}/remote-dst/should-not-be-created" && \
    fail "non-existent source: should have errored" || \
    pass "non-existent source: correctly errors"

# ============================================================
# Trailing slash: source is not a directory
# ============================================================
echo ""
echo "--- Case: Source file with trailing slash (error) ---"
run_filetool_stderr "$WORKDIR/local-src/file1.txt/" "${REMOTE_BASE}/remote-dst/should-not-exist"
assert_error $? "source file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
    pass "source file with trailing slash: error message says 'is not a directory'"
else
    fail "source file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: Destination file with trailing slash (error) ---"
run_filetool_stderr "$WORKDIR/local-src" "${REMOTE_BASE}/remote-dst/existing-file.txt/"
assert_error $? "destination file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
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
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
    pass "--ls local file with trailing slash: error message says 'is not a directory'"
else
    fail "--ls local file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: --ls: remote file with trailing slash ---"
run_filetool_stderr --ls "${REMOTE_BASE}/remote-src/file1.txt/"
ret=$?
assert_error $ret "--ls remote file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
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
run_filetool --ls "${REMOTE_BASE}/remote-src/subdir/" > /dev/null
assert_success $? "--ls remote dir with trailing slash: succeeds"

echo ""
echo "--- Case: --ls: multi-path, local file with trailing slash ---"
run_filetool_stderr --ls "$WORKDIR/local-src/file1.txt/" "$WORKDIR/local-src/file2.txt"
ret=$?
assert_error $ret "--ls multi with trailing-slash file: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
    pass "--ls multi with trailing-slash file: error message says 'is not a directory'"
else
    fail "--ls multi with trailing-slash file: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: --ls: multi-path, remote file with trailing slash ---"
run_filetool_stderr --ls "${REMOTE_BASE}/remote-src/file1.txt/" "$WORKDIR/local-src/file2.txt"
ret=$?
assert_error $ret "--ls multi with remote trailing-slash file: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
    pass "--ls multi with remote trailing-slash file: error message says 'is not a directory'"
else
    fail "--ls multi with remote trailing-slash file: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

# ==== --ls symlink-to-directory trailing slash behavior ====
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
run_filetool_ls_capture -l "${REMOTE_BASE}/ls-symlink-test/link-to-dir"
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
run_filetool_ls_capture -l "${REMOTE_BASE}/ls-symlink-test/link-to-dir/"
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
# Local -> Local tests (mirror all local<->remote patterns)
# ============================================================
echo ""
echo "--- Case: Local->Local: local file -> local directory ---"
rm -f "$WORKDIR/local-dst/existing-dir/file1.txt"
run_filetool "$WORKDIR/local-src/file1.txt" "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/existing-dir/file1.txt" \
    "local file -> local dir creates basename"

echo ""
echo "--- Case: Local->Local: local file -> local non-existent path ---"
rm -f "$WORKDIR/local-dst/new-local-file.txt"
run_filetool "$WORKDIR/local-src/file2.txt" "$WORKDIR/local-dst/new-local-file.txt"
assert_file_eq "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/new-local-file.txt" \
    "local file -> local non-existent creates as named"

echo ""
echo "--- Case: Local->Local: local file -> existing local file, no overwrite ---"
cp "$WORKDIR/local-dst/existing-file.txt" "$WORKDIR/local-dst/existing-file.txt.ref"
echo "n" | run_filetool "$WORKDIR/local-src/file1.txt" "$WORKDIR/local-dst/existing-file.txt"
assert_file_eq "$WORKDIR/local-dst/existing-file.txt.ref" \
    "$WORKDIR/local-dst/existing-file.txt" \
    "local file -> existing local file: not overwritten when answering n"

echo ""
echo "--- Case: Local->Local: local file -> existing local file with -f ---"
run_filetool -f "$WORKDIR/local-src/file1.txt" "$WORKDIR/local-dst/existing-file.txt"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/existing-file.txt" \
    "local file -> local file with -f overwrites"

echo ""
echo "--- Case: Local->Local: local dir -> local existing directory ---"
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
echo "--- Case: Local->Local: local dir -> local non-existent path ---"
rm -rf "$WORKDIR/local-dst/created-local-dir"
run_filetool "$WORKDIR/local-src" "$WORKDIR/local-dst/created-local-dir"
assert_file_eq "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-dst/created-local-dir/file1.txt" \
    "local dir -> local non-existent: file1 copied"
assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
    "$WORKDIR/local-dst/created-local-dir/subdir/nested.txt" \
    "local dir -> local non-existent: nested file copied"

echo ""
echo "--- Case: Local->Local: local dir -> existing local file (error) ---"
run_filetool "$WORKDIR/local-src" "$WORKDIR/local-dst/existing-file.txt" && \
    fail "local dir -> local file: should have errored" || \
    pass "local dir -> local file: correctly errors"

echo ""
echo "--- Case: Local->Local: deep tree local dir -> local dir, verify with diff -r ---"
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
echo "--- Case: Local->Local: multi-source local files -> local dir ---"
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
echo "--- Case: Local->Local: multi-source local files -> local file (error) ---"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "$WORKDIR/local-dst/existing-file.txt" && \
    fail "multi src local->local file: should have errored" || \
    pass "multi src local->local file: correctly errors"

echo ""
echo "--- Case: Local->Local: multi-source local file + local dir -> local dir ---"
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
echo "--- Case: Local->Local: -- delimiter, file with leading dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/-leading-dash.txt"
run_filetool -- "$WORKDIR/local-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/-leading-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/-leading-dash.txt" \
    "-- delimiter local->local: file starting with - copied"

echo ""
echo "--- Case: Local->Local: -- delimiter, file with leading double-dash -> local dir ---"
rm -f "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt"
run_filetool -- "$WORKDIR/local-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir"
assert_file_eq "$WORKDIR/local-src/--leading-double-dash.txt" \
    "$WORKDIR/local-dst/existing-dir/--leading-double-dash.txt" \
    "-- delimiter local->local: file starting with -- copied"

echo ""
echo "--- Case: Local->Local: symlinks dir -> local dir, verify targets preserved ---"
rm -rf "$WORKDIR/local-dst/symlinks-local"
run_filetool "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-local"
assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/local-dst/symlinks-local" \
    "symlinks local->local: targets match"

echo ""
echo "--- Case: Local->Local: single local symlink -> local dir ---"
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
echo "--- Case: Local->Local: single broken symlink -> local dir ---"
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
# /proc/1/cmdline copy tests (md5sum verification)
# ============================================================
LOCAL_PROC_CMDLINE_MD5=$(md5 -q /proc/1/cmdline 2>/dev/null || md5sum /proc/1/cmdline 2>/dev/null | awk '{print $1}')

echo ""
echo "--- Case: /proc/1/cmdline: local -> local dir, md5sum ---"
rm -f "$WORKDIR/local-dst/existing-dir/cmdline"
run_filetool /proc/1/cmdline "$WORKDIR/local-dst/existing-dir"
assert_file_eq /proc/1/cmdline "$WORKDIR/local-dst/existing-dir/cmdline" \
    "local /proc/1/cmdline -> local dir: file matches"

echo ""
echo "--- Case: /proc/1/cmdline: local -> remote dir, round-trip md5sum ---"
run_filetool /proc/1/cmdline "${REMOTE_BASE}/remote-dst/existing-dir"
ret=$?
if [ $ret -ne 0 ]; then
    fail "local /proc/1/cmdline -> remote dir: copy failed (exit $ret)"
else
    rm -f "$WORKDIR/local-dst/cmdline-roundtrip.txt"
    run_filetool "${REMOTE_BASE}/remote-dst/existing-dir/cmdline" \
        "$WORKDIR/local-dst/cmdline-roundtrip.txt"
    if [ $? -ne 0 ]; then
        fail "local /proc/1/cmdline -> remote dir: round-trip copy back failed"
    else
        RT_MD5=$(md5 -q "$WORKDIR/local-dst/cmdline-roundtrip.txt" 2>/dev/null || md5sum "$WORKDIR/local-dst/cmdline-roundtrip.txt" | awk '{print $1}')
        assert_eq "$RT_MD5" "$LOCAL_PROC_CMDLINE_MD5" \
            "local /proc/1/cmdline -> remote dir: round-trip md5sum matches"
    fi
fi

echo ""
echo "--- Case: /proc/1/cmdline: remote -> local dir, md5sum vs remote ---"
rm -f "$WORKDIR/local-dst/existing-dir/cmdline"
run_filetool "${REMOTE}/proc/1/cmdline" "$WORKDIR/local-dst/existing-dir"
ret=$?
if [ $ret -ne 0 ]; then
    fail "remote /proc/1/cmdline -> local dir: copy failed (exit $ret)"
else
    REMOTE_MD5=$(md5 -q "$WORKDIR/local-dst/existing-dir/cmdline" 2>/dev/null || md5sum "$WORKDIR/local-dst/existing-dir/cmdline" | awk '{print $1}')
    if [ -n "$REMOTE_PROC_CMDLINE_MD5" ]; then
        assert_eq "$REMOTE_MD5" "$REMOTE_PROC_CMDLINE_MD5" \
            "remote /proc/1/cmdline -> local dir: md5sum matches remote /proc/1/cmdline"
    else
        pass "remote /proc/1/cmdline -> local dir: copy succeeded (md5 check skipped)"
    fi
fi

echo ""
echo "--- Case: /proc/1/cmdline: remote -> local specific path, md5sum vs remote ---"
rm -f "$WORKDIR/local-dst/remote-proc-cmdline.txt"
run_filetool "${REMOTE}/proc/1/cmdline" "$WORKDIR/local-dst/remote-proc-cmdline.txt"
ret=$?
if [ $ret -ne 0 ]; then
    fail "remote /proc/1/cmdline -> local specific path: copy failed (exit $ret)"
else
    REMOTE2_MD5=$(md5 -q "$WORKDIR/local-dst/remote-proc-cmdline.txt" 2>/dev/null || md5sum "$WORKDIR/local-dst/remote-proc-cmdline.txt" | awk '{print $1}')
    if [ -n "$REMOTE_PROC_CMDLINE_MD5" ]; then
        assert_eq "$REMOTE2_MD5" "$REMOTE_PROC_CMDLINE_MD5" \
            "remote /proc/1/cmdline -> local specific path: md5sum matches remote /proc/1/cmdline"
    else
        pass "remote /proc/1/cmdline -> local specific path: copy succeeded (md5 check skipped)"
    fi
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
run_filetool_stderr /dev/null "${REMOTE_BASE}/remote-dst/existing-dir/"
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
    run_filetool_stderr "$SPECIAL_DIR/test-fifo" "${REMOTE_BASE}/remote-dst/existing-dir/"
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
    run_filetool_stderr "$SPECIAL_DIR" "${REMOTE_BASE}/remote-dst/special-dst"
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
# Backslash in filenames (Unix-to-Unix)
# ============================================================
echo ""
echo "=== Backslash in Filenames (Unix-to-Unix) ==="

bsf_start1='\bs-start1.txt'
bsf_start2='\\bs-start2.txt'
bsf_start3='\\\bs-start3.txt'
bsf_mid1='bs\mid1.txt'
bsf_mid2='bs\\mid2.txt'
bsf_mid3='bs\\\mid3.txt'
bsf_end1='bs-end1\'
bsf_end2='bs-end2\\'
bsf_end3='bs-end3\\\'

bsd_start1='\dir-s1'
bsd_start2='\\dir-s2'
bsd_start3='\\\dir-s3'
bsd_mid1='dir\m1'
bsd_mid2='dir\\m2'
bsd_mid3='dir\\\m3'
bsd_end1='dir-e1\'
bsd_end2='dir-e2\\'
bsd_end3='dir-e3\\\'

mkdir -p "$WORKDIR/bs-test"
for f in "$bsf_start1" "$bsf_start2" "$bsf_start3" \
         "$bsf_mid1" "$bsf_mid2" "$bsf_mid3" \
         "$bsf_end1" "$bsf_end2" "$bsf_end3"; do
    createtext "$WORKDIR/bs-test/$f" "content of $f"
done
for d in "$bsd_start1" "$bsd_start2" "$bsd_start3" \
         "$bsd_mid1" "$bsd_mid2" "$bsd_mid3" \
         "$bsd_end1" "$bsd_end2" "$bsd_end3"; do
    mkdir -p "$WORKDIR/bs-test/$d"
    createtext "$WORKDIR/bs-test/$d/nested.txt" "nested in $d"
done

echo ""
echo "--- Case: local dir with backslash names -> remote ---"
run_filetool "$WORKDIR/bs-test" "${REMOTE_BASE}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    remote_roundtrip_dir "${REMOTE_BASE}/remote-dst/bs-test" "$WORKDIR/roundtrip/bs-test"
    ok=1
    for f in "$bsf_start1" "$bsf_start2" "$bsf_start3" \
             "$bsf_mid1" "$bsf_mid2" "$bsf_mid3" \
             "$bsf_end1" "$bsf_end2" "$bsf_end3"; do
        if [ -f "$WORKDIR/roundtrip/bs-test/$f" ]; then
            pass "dir local->remote: file '$f' present on remote"
        else
            fail "dir local->remote: file '$f' missing on remote"
            ok=0
        fi
    done
    for d in "$bsd_start1" "$bsd_start2" "$bsd_start3" \
             "$bsd_mid1" "$bsd_mid2" "$bsd_mid3" \
             "$bsd_end1" "$bsd_end2" "$bsd_end3"; do
        if [ -f "$WORKDIR/roundtrip/bs-test/$d/nested.txt" ]; then
            pass "dir local->remote: dir '$d' with nested file present"
        else
            fail "dir local->remote: dir '$d' nested file missing"
            ok=0
        fi
    done
    [ "$ok" -eq 0 ] && fail "dir local->remote: one or more backslash names not preserved"
else
    fail "dir local->remote: copy failed (exit $ret)"
fi

echo ""
echo "--- Case: backslash names roundtrip (remote->local) with trailing slash ---"
rm -rf "$WORKDIR/roundtrip/bs-rt-trailing"
run_filetool_stderr "${REMOTE_BASE}/remote-dst/bs-test" "$WORKDIR/roundtrip/bs-rt-trailing/"
ret=$?
assert_error $ret "backslash names roundtrip with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
    pass "backslash names roundtrip with trailing slash: error message says 'not a directory'"
else
    fail "backslash names roundtrip with trailing slash: expected 'not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

echo ""
echo "--- Case: backslash names roundtrip (remote->local) ---"
rm -rf "$WORKDIR/roundtrip/bs-rt"
mkdir -p "$WORKDIR/roundtrip/bs-rt"
run_filetool "${REMOTE_BASE}/remote-dst/bs-test" "$WORKDIR/roundtrip/bs-rt/"
ret=$?
if [ $ret -eq 0 ]; then
    if diff -r "$WORKDIR/bs-test" "$WORKDIR/roundtrip/bs-rt/bs-test" >/dev/null 2>&1; then
        pass "backslash names roundtrip: diff -r matches (all backslash names preserved exactly)"
    else
        fail "backslash names roundtrip: diff -r shows differences"
    fi
else
    fail "backslash names roundtrip: remote->local copy failed (exit $ret)"
fi

echo ""
echo "--- Case: single file, double backslash at start, roundtrip ---"
rm -rf "$WORKDIR/roundtrip/bs-start"
mkdir -p "$WORKDIR/roundtrip/bs-start"
run_filetool "$WORKDIR/bs-test/$bsf_start2" "${REMOTE_BASE}/remote-dst/"
run_filetool "${REMOTE_BASE}/remote-dst/$bsf_start2" "$WORKDIR/roundtrip/bs-start"
assert_file_eq "$WORKDIR/bs-test/$bsf_start2" \
    "$WORKDIR/roundtrip/bs-start/$bsf_start2" \
    "single file with \\\\ at start: roundtrip content matches"

echo ""
echo "--- Case: single file, double backslash in middle, roundtrip ---"
rm -rf "$WORKDIR/roundtrip/bs-mid"
mkdir -p "$WORKDIR/roundtrip/bs-mid"
run_filetool "$WORKDIR/bs-test/$bsf_mid2" "${REMOTE_BASE}/remote-dst/"
run_filetool "${REMOTE_BASE}/remote-dst/$bsf_mid2" "$WORKDIR/roundtrip/bs-mid"
assert_file_eq "$WORKDIR/bs-test/$bsf_mid2" \
    "$WORKDIR/roundtrip/bs-mid/$bsf_mid2" \
    "single file with \\\\ in middle: roundtrip content matches"

echo ""
echo "--- Case: single file, double backslash at end, roundtrip ---"
rm -rf "$WORKDIR/roundtrip/bs-end"
mkdir -p "$WORKDIR/roundtrip/bs-end"
run_filetool "$WORKDIR/bs-test/$bsf_end2" "${REMOTE_BASE}/remote-dst/"
run_filetool "${REMOTE_BASE}/remote-dst/$bsf_end2" "$WORKDIR/roundtrip/bs-end"
assert_file_eq "$WORKDIR/bs-test/$bsf_end2" \
    "$WORKDIR/roundtrip/bs-end/$bsf_end2" \
    "single file with \\\\ at end: roundtrip content matches"

echo ""
echo "--- Case: directory with double trailing backslash, roundtrip ---"
rm -rf "$WORKDIR/roundtrip/bs-enddir"
run_filetool "$WORKDIR/bs-test/$bsd_end2" "${REMOTE_BASE}/remote-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    mkdir -p "$WORKDIR/roundtrip/bs-enddir"
    run_filetool "${REMOTE_BASE}/remote-dst/$bsd_end2" "$WORKDIR/roundtrip/bs-enddir"
    if [ -f "$WORKDIR/roundtrip/bs-enddir/$bsd_end2/nested.txt" ]; then
        pass "dir with trailing \\\\: nested file present after roundtrip"
    else
        fail "dir with trailing \\\\: nested file missing after roundtrip"
    fi
else
    fail "dir with trailing \\\\: local->remote copy failed (exit $ret)"
fi

# ============================================================
# Results
# ============================================================
echo ""
echo "=== Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ "$FAILED" -gt 0 ]; then
    echo ""
    echo "=== Valgrind Summary ==="
    for vglog in "$VGLOG_DIR"/client-*.log; do
        if [ -f "$vglog" ] && grep -q "ERROR SUMMARY: [1-9]\|definitely lost: [1-9]\|indirectly lost: [1-9]" "$vglog" 2>/dev/null; then
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

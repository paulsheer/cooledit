#!/bin/bash
# Test suite for cooledit --filetool remote copy against Windows REMOTEFS.EXE
# at a remote location. Uses two-step round-trip: local->remote, remote->local, diff.
# Modeled on test_filetool.sh but adapted for remote Windows target.
SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"
COOLEDIT="$(cd "$SCRIPTDIR/../editor" && pwd)/cooledit"
KEYFILE="$SCRIPTDIR/../AESKEYFILE-windows"
REMOTE_HOST="$1"
REMOTE="${REMOTE_HOST}:"
REMOTE_BASE="C:/Users/Owner/REMOTEFS-TESTS"
PASSED=0
FAILED=0
AES_KEY=""

WORKDIR_BASE="$SCRIPTDIR/work-dir"
WORKDIR=""
TMPDIR_CLEANUP=()
PASSWORD_DIR="$HOME/.cedit"
PASSWORD_FILE="$PASSWORD_DIR/.password"
PASSWORD_BACKUP=""

test "$REMOTE_HOST" = "" && {
    echo 'Usage: test_filetool_remote.sh <IP-address>'
    exit 1
}

cleanup() {
    if [ -n "$PASSWORD_BACKUP" ] && [ -f "$PASSWORD_BACKUP" ]; then
        cat "$PASSWORD_BACKUP" > "$PASSWORD_FILE" 2>/dev/null || true
        rm -f "$PASSWORD_BACKUP"
    elif [ -n "$PASSWORD_BACKUP" ]; then
        rm -f "$PASSWORD_FILE" 2>/dev/null || true
    fi
    for d in "${TMPDIR_CLEANUP[@]}"; do
        rm -rf "$d" 2>/dev/null || true
    done
}

trap cleanup EXIT

# --- helpers ---

tmpdir() {
    local d
    d=$(mktemp -d "$WORKDIR_BASE/filetool-remote-test-XXXXXX")
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

assert_success() {
    if [ "$1" -eq 0 ]; then
        pass "$2"
    else
        fail "$2 (expected success, got error $1)"
        return 1
    fi
}

assert_error() {
    if [ "$1" -ne 0 ]; then
        pass "$2 (got expected error)"
    else
        fail "$2 (expected error, got success)"
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

# Run cooledit --filetool. Password is pre-populated so no stdin needed.
run_filetool() {
    local ret
    "$COOLEDIT" --filetool "$@" >/dev/null 2>&1
    ret=$?
    return $ret
}

# Run cooledit --filetool and capture stderr into FILE_TOOL_STDERR.
run_filetool_stderr() {
    FILE_TOOL_STDERR=$("$COOLEDIT" --filetool "$@" 2>&1 1>/dev/null)
    return $?
}

# Run with stdin provided (for overwrite prompts).
run_filetool_stdin() {
    local input="$1"
    shift
    local ret
    echo "$input" | "$COOLEDIT" --filetool "$@" >/dev/null 2>&1
    ret=$?
    return $ret
}

# ============================================================
# Setup
# ============================================================
echo "=== cooledit --filetool Remote Test Suite ==="
echo "Target: ${REMOTE_HOST} (${REMOTE_BASE})"
echo ""

# Read AES key
if [ ! -f "$KEYFILE" ]; then
    echo "ERROR: AES key file not found: $KEYFILE"
    exit 1
fi
AES_KEY=$(head -1 "$KEYFILE" | tr -d '\r\n')
if [ -z "$AES_KEY" ]; then
    echo "ERROR: AES key is empty"
    exit 1
fi

# Pre-populate password file so filetool doesn't prompt on stdin.
# Back up original if it exists.
mkdir -p "$PASSWORD_DIR"
if [ -f "$PASSWORD_FILE" ]; then
    PASSWORD_BACKUP=$(mktemp "$PASSWORD_FILE.bak.XXXXXX")
    cp "$PASSWORD_FILE" "$PASSWORD_BACKUP"
fi
echo "$REMOTE_HOST crypto-enabled $AES_KEY" > "$PASSWORD_FILE"
echo "Password file pre-populated for $REMOTE_HOST"

# Create local workspace
mkdir -p "$WORKDIR_BASE"
WORKDIR=$(tmpdir)
REMOTE_TESTDIR="${REMOTE}${REMOTE_BASE}/filetool-test-$$"
echo "Local workspace: $WORKDIR"
echo "Remote test dir: $REMOTE_TESTDIR"

# Create remote test directory by copying an empty local dir
mkdir -p "$WORKDIR/empty-staging"
run_filetool "$WORKDIR/empty-staging" "$REMOTE_TESTDIR"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to create remote test directory. Is REMOTEFS.EXE running on $REMOTE_HOST?"
    exit 1
fi
echo "Remote test directory created"

# Create remote destination directory
run_filetool "$WORKDIR/empty-staging" "${REMOTE_TESTDIR}/dst"
run_filetool "$WORKDIR/empty-staging" "${REMOTE_TESTDIR}/src"

echo ""
echo "Setting up test data..."

# Local source data
mkdir -p "$WORKDIR/local-src/subdir/deep"
mkdir -p "$WORKDIR/local-src/emptydir"
createtext "$WORKDIR/local-src/file1.txt" "Hello from file1"
createtext "$WORKDIR/local-src/file2.txt" "Contents of file2"
createtext "$WORKDIR/local-src/subdir/nested.txt" "nested file content"
createtext "$WORKDIR/local-src/subdir/deep/deep.txt" "deeply nested"

# Deep tree: 3 levels, 4 files per level
mkdir -p "$WORKDIR/deep-tree/sub1/sub2"
for i in 1 2 3 4; do
    createtext "$WORKDIR/deep-tree/a${i}.txt" "level0-file-${i}"
    createtext "$WORKDIR/deep-tree/sub1/b${i}.txt" "level1-file-${i}"
    createtext "$WORKDIR/deep-tree/sub1/sub2/c${i}.txt" "level2-file-${i}"
done
ln -s "a1.txt" "$WORKDIR/deep-tree/link-a1"
ln -s "sub1" "$WORKDIR/deep-tree/link-sub1"
ln -s "sub2" "$WORKDIR/deep-tree/sub1/link-deep"
ln -s "../b1.txt" "$WORKDIR/deep-tree/sub1/sub2/link-up"

# Symlink test data (local side)
mkdir -p "$WORKDIR/local-symlinks/subdir"
createtext "$WORKDIR/local-symlinks/regular.txt" "regular file for symlink target"
createtext "$WORKDIR/local-symlinks/subdir/nested.txt" "nested file for symlink target"
ln -s "regular.txt" "$WORKDIR/local-symlinks/link-to-file"
ln -s "subdir" "$WORKDIR/local-symlinks/link-to-dir"
ln -s "/etc/hosts" "$WORKDIR/local-symlinks/link-absolute"

# Local roundtrip staging area
mkdir -p "$WORKDIR/roundtrip"

echo ""
echo "=== Test Cases ==="
echo ""

# ============================================================
# Case 1: local file -> remote directory (roundtrip)
# ============================================================
echo "--- Case 1: local file -> remote directory (roundtrip) ---"
rm -rf "$WORKDIR/roundtrip/case1"
mkdir -p "$WORKDIR/roundtrip/case1"
run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE_TESTDIR}/dst"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/dst/file1.txt" "$WORKDIR/roundtrip/case1"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_file_eq "$WORKDIR/local-src/file1.txt" \
        "$WORKDIR/roundtrip/case1/file1.txt" \
        "local file -> remote dir -> local (roundtrip)"
else
    fail "local file -> remote dir: copy failed (exit $ret)"
fi

# ============================================================
# Case 2: local file -> remote non-existent path (roundtrip)
# ============================================================
echo ""
echo "--- Case 2: local file -> remote non-existent path (roundtrip) ---"
rm -rf "$WORKDIR/roundtrip/case2"
mkdir -p "$WORKDIR/roundtrip/case2"
run_filetool "$WORKDIR/local-src/file2.txt" "${REMOTE_TESTDIR}/newfile.txt"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/newfile.txt" "$WORKDIR/roundtrip/case2"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_file_eq "$WORKDIR/local-src/file2.txt" \
        "$WORKDIR/roundtrip/case2/newfile.txt" \
        "local file -> remote non-existent -> local (roundtrip)"
else
    fail "local file -> remote non-existent: copy failed (exit $ret)"
fi

# ============================================================
# Case 3: overwrite confirmation (answer "n")
# ============================================================
echo ""
echo "--- Case 3: overwrite confirmation (answer n) ---"
createtext "$WORKDIR/local-src/overw-a.txt" "original remote content A"
createtext "$WORKDIR/local-src/overw-b.txt" "new local content B"
# Place file A on remote
run_filetool "$WORKDIR/local-src/overw-a.txt" "${REMOTE_TESTDIR}/exist.txt"
# Try to overwrite with B, answer "n"
run_filetool_stdin "n" "$WORKDIR/local-src/overw-b.txt" "${REMOTE_TESTDIR}/exist.txt"
# Copy back and verify it's still A
rm -rf "$WORKDIR/roundtrip/case3"
mkdir -p "$WORKDIR/roundtrip/case3"
run_filetool "${REMOTE_TESTDIR}/exist.txt" "$WORKDIR/roundtrip/case3"
assert_file_eq "$WORKDIR/local-src/overw-a.txt" \
    "$WORKDIR/roundtrip/case3/exist.txt" \
    "overwrite confirm n: original file preserved"

# ============================================================
# Case 4: force overwrite with -f (roundtrip)
# ============================================================
echo ""
echo "--- Case 4: force overwrite with -f (roundtrip) ---"
run_filetool -f "$WORKDIR/local-src/overw-b.txt" "${REMOTE_TESTDIR}/exist.txt"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case4"
    mkdir -p "$WORKDIR/roundtrip/case4"
    run_filetool "${REMOTE_TESTDIR}/exist.txt" "$WORKDIR/roundtrip/case4"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_file_eq "$WORKDIR/local-src/overw-b.txt" \
        "$WORKDIR/roundtrip/case4/exist.txt" \
        "-f flag overwrites without prompt (roundtrip)"
else
    fail "-f overwrite: copy failed (exit $ret)"
fi

# ============================================================
# Case 5: force overwrite with --force
# ============================================================
echo ""
echo "--- Case 5: force overwrite with --force ---"
createtext "$WORKDIR/local-src/overw-c.txt" "force flag content C"
run_filetool --force "$WORKDIR/local-src/overw-c.txt" "${REMOTE_TESTDIR}/exist.txt"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case5"
    mkdir -p "$WORKDIR/roundtrip/case5"
    run_filetool "${REMOTE_TESTDIR}/exist.txt" "$WORKDIR/roundtrip/case5"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_file_eq "$WORKDIR/local-src/overw-c.txt" \
        "$WORKDIR/roundtrip/case5/exist.txt" \
        "--force flag overwrites without prompt (roundtrip)"
else
    fail "--force overwrite: copy failed (exit $ret)"
fi

# ============================================================
# Case 6: local dir -> remote existing directory (roundtrip)
# ============================================================
echo ""
echo "--- Case 6: local dir -> remote existing directory (roundtrip) ---"
run_filetool "$WORKDIR/local-src" "${REMOTE_TESTDIR}/dst"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case6"
    mkdir -p "$WORKDIR/roundtrip/case6"
    run_filetool "${REMOTE_TESTDIR}/dst/local-src" "$WORKDIR/roundtrip/case6"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    if diff -r "$WORKDIR/local-src" "$WORKDIR/roundtrip/case6/local-src" >/dev/null 2>&1; then
        pass "local dir -> remote dir -> local: diff -r matches"
    else
        fail "local dir -> remote dir -> local: diff -r shows differences"
    fi
else
    fail "local dir -> remote dir: copy failed (exit $ret)"
fi

# ============================================================
# Case 7: local dir -> remote non-existent path (roundtrip)
# ============================================================
echo ""
echo "--- Case 7: local dir -> remote non-existent path (roundtrip) ---"
run_filetool "$WORKDIR/local-src" "${REMOTE_TESTDIR}/created-dir"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case7"
    mkdir -p "$WORKDIR/roundtrip/case7"
    run_filetool "${REMOTE_TESTDIR}/created-dir" "$WORKDIR/roundtrip/case7"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    if diff -r "$WORKDIR/local-src" "$WORKDIR/roundtrip/case7/created-dir" >/dev/null 2>&1; then
        pass "local dir -> remote non-existent -> local: diff -r matches"
    else
        fail "local dir -> remote non-existent -> local: diff -r shows differences"
    fi
else
    fail "local dir -> remote non-existent: copy failed (exit $ret)"
fi

# ============================================================
# Case 8: deep tree (3 levels) roundtrip
# ============================================================
echo ""
echo "--- Case 8: deep tree (3 levels) roundtrip ---"
run_filetool "$WORKDIR/deep-tree" "${REMOTE_TESTDIR}/deep-tree"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case8"
    mkdir -p "$WORKDIR/roundtrip/case8"
    run_filetool "${REMOTE_TESTDIR}/deep-tree" "$WORKDIR/roundtrip/case8"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    if diff -r "$WORKDIR/deep-tree" "$WORKDIR/roundtrip/case8/deep-tree" >/dev/null 2>&1; then
        pass "deep tree local->remote->local: diff -r matches (12 files, 4 symlinks, 3 levels)"
    else
        fail "deep tree local->remote->local: diff -r shows differences"
    fi
    assert_symlinks_match "$WORKDIR/deep-tree" "$WORKDIR/roundtrip/case8/deep-tree" \
        "deep tree roundtrip: symlinks preserved"
else
    fail "deep tree: copy failed (exit $ret)"
fi

# ============================================================
# Case 9: local symlinks -> remote, roundtrip verify targets preserved
# ============================================================
echo ""
echo "--- Case 9: local symlinks -> remote -> local roundtrip ---"
rm -rf "$WORKDIR/roundtrip/case9"
mkdir -p "$WORKDIR/roundtrip/case9"
run_filetool "$WORKDIR/local-symlinks" "${REMOTE_TESTDIR}/symlinks"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/symlinks" "$WORKDIR/roundtrip/case9"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/roundtrip/case9/symlinks" \
        "local symlinks -> remote -> local: targets match"
    if diff -r "$WORKDIR/local-symlinks" "$WORKDIR/roundtrip/case9/symlinks" >/dev/null 2>&1; then
        pass "symlinks roundtrip: diff -r matches (files intact)"
    else
        fail "symlinks roundtrip: diff -r shows differences"
    fi
else
    fail "symlinks roundtrip: copy failed (exit $ret)"
fi

# ============================================================
# Case 10: remote symlinks -> local, verify targets preserved
# ============================================================
echo ""
echo "--- Case 10: remote symlinks -> local, verify targets preserved ---"
rm -rf "$WORKDIR/roundtrip/case10"
mkdir -p "$WORKDIR/roundtrip/case10"
# Use the symlinks that were pushed to remote in Case 9
run_filetool "${REMOTE_TESTDIR}/symlinks/link-to-file" "$WORKDIR/roundtrip/case10"
run_filetool "${REMOTE_TESTDIR}/symlinks/link-to-dir" "$WORKDIR/roundtrip/case10"
run_filetool "${REMOTE_TESTDIR}/symlinks/regular.txt" "$WORKDIR/roundtrip/case10"
run_filetool "${REMOTE_TESTDIR}/symlinks/subdir" "$WORKDIR/roundtrip/case10"
# Verify link-to-file points to "regular.txt"
dst_target=$(readlink "$WORKDIR/roundtrip/case10/link-to-file" 2>/dev/null)
assert_eq "$dst_target" "regular.txt" "remote symlink link-to-file: target preserved"
# Verify link-to-dir points to "subdir"
dst_target=$(readlink "$WORKDIR/roundtrip/case10/link-to-dir" 2>/dev/null)
assert_eq "$dst_target" "subdir" "remote symlink link-to-dir: target preserved"
# Verify nested file inside symlinked dir was copied
assert_file_eq "$WORKDIR/local-symlinks/subdir/nested.txt" \
    "$WORKDIR/roundtrip/case10/subdir/nested.txt" \
    "remote symlink to dir: nested file copied"

# ============================================================
# Case 11: local dir -> existing remote file (error)
# ============================================================
echo ""
echo "--- Case 11: local dir -> existing remote file (error) ---"
run_filetool "$WORKDIR/local-src" "${REMOTE_TESTDIR}/exist.txt"
assert_error $? "local dir -> existing remote file: correctly errors"

# ============================================================
# Case 12: multi-source files -> remote dir (roundtrip)
# ============================================================
echo ""
echo "--- Case 12: multi-source files -> remote dir (roundtrip) ---"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_TESTDIR}/dst"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case12"
    mkdir -p "$WORKDIR/roundtrip/case12"
    run_filetool "${REMOTE_TESTDIR}/dst/file1.txt" "$WORKDIR/roundtrip/case12"
    run_filetool "${REMOTE_TESTDIR}/dst/file2.txt" "$WORKDIR/roundtrip/case12"
    assert_file_eq "$WORKDIR/local-src/file1.txt" \
        "$WORKDIR/roundtrip/case12/file1.txt" \
        "multi src files->remote: file1 roundtrip"
    assert_file_eq "$WORKDIR/local-src/file2.txt" \
        "$WORKDIR/roundtrip/case12/file2.txt" \
        "multi src files->remote: file2 roundtrip"
else
    fail "multi src files->remote: copy failed (exit $ret)"
fi

# ============================================================
# Case 13: multi-source mixed (file + dir) -> remote dir (roundtrip)
# ============================================================
echo ""
echo "--- Case 13: multi-source mixed -> remote dir (roundtrip) ---"
run_filetool \
    "$WORKDIR/local-src/subdir" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_TESTDIR}/dst"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case13"
    mkdir -p "$WORKDIR/roundtrip/case13"
    run_filetool "${REMOTE_TESTDIR}/dst/subdir" "$WORKDIR/roundtrip/case13"
    run_filetool "${REMOTE_TESTDIR}/dst/file2.txt" "$WORKDIR/roundtrip/case13"
    assert_file_eq "$WORKDIR/local-src/file2.txt" \
        "$WORKDIR/roundtrip/case13/file2.txt" \
        "multi src file+dir->remote: file2 roundtrip"
    assert_file_eq "$WORKDIR/local-src/subdir/nested.txt" \
        "$WORKDIR/roundtrip/case13/subdir/nested.txt" \
        "multi src file+dir->remote: nested file roundtrip"
else
    fail "multi src file+dir->remote: copy failed (exit $ret)"
fi

# ============================================================
# Case 14: multi-source -> remote file (error)
# ============================================================
echo ""
echo "--- Case 14: multi-source -> remote file (error) ---"
run_filetool \
    "$WORKDIR/local-src/file1.txt" \
    "$WORKDIR/local-src/file2.txt" \
    "${REMOTE_TESTDIR}/exist.txt"
assert_error $? "multi src -> remote file: correctly errors"

# ============================================================
# Case 15: non-existent source (error)
# ============================================================
echo ""
echo "--- Case 15: non-existent source (error) ---"
run_filetool "$WORKDIR/local-src/does-not-exist.txt" \
    "${REMOTE_TESTDIR}/should-not-be-created"
assert_error $? "non-existent source: correctly errors"

# ============================================================
# Case 16: cross-remote error (IP-to-IP)
# ============================================================
echo ""
echo "--- Case 16: cross-remote (IP-to-IP) error ---"
run_filetool "${REMOTE_TESTDIR}/exist.txt" \
    "10.1.0.99:C:/Users/Owner/REMOTEFS-TESTS/should-not-exist"
assert_error $? "cross-remote copy: correctly errors"

# ============================================================
# Case 17: remote file -> local directory (roundtrip)
# ============================================================
echo ""
echo "--- Case 17: remote file -> local directory (roundtrip) ---"
# First ensure we have a remote file in src/
run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE_TESTDIR}/src"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case17"
    mkdir -p "$WORKDIR/roundtrip/case17"
    run_filetool "${REMOTE_TESTDIR}/src/file1.txt" "$WORKDIR/roundtrip/case17"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_file_eq "$WORKDIR/local-src/file1.txt" \
        "$WORKDIR/roundtrip/case17/file1.txt" \
        "remote file -> local dir creates basename (roundtrip)"
else
    fail "remote file -> local dir: copy failed (exit $ret)"
fi

# ============================================================
# Case 18: local file -> remote directory with trailing slash (roundtrip)
# ============================================================
echo ""
echo "--- Case 18: local file -> remote directory with trailing slash ---"
rm -rf "$WORKDIR/roundtrip/case18"
mkdir -p "$WORKDIR/roundtrip/case18"
run_filetool "$WORKDIR/local-src/file2.txt" "${REMOTE_TESTDIR}/dst/"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/dst/file2.txt" "$WORKDIR/roundtrip/case18"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_file_eq "$WORKDIR/local-src/file2.txt" \
        "$WORKDIR/roundtrip/case18/file2.txt" \
        "local file -> remote dir with trailing slash (roundtrip)"
else
    fail "local file -> remote dir with trailing slash: copy failed (exit $ret)"
fi

# ============================================================
# Case 19: remote file -> local directory with trailing slash (roundtrip)
# ============================================================
echo ""
echo "--- Case 19: remote file -> local directory with trailing slash ---"
rm -rf "$WORKDIR/roundtrip/case19" "$WORKDIR/roundtrip/case19-dst"
mkdir -p "$WORKDIR/roundtrip/case19" "$WORKDIR/roundtrip/case19-dst"
# First put a file on remote
run_filetool "$WORKDIR/local-src/file1.txt" "${REMOTE_TESTDIR}/src"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/src/file1.txt" "$WORKDIR/roundtrip/case19-dst/"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    assert_file_eq "$WORKDIR/local-src/file1.txt" \
        "$WORKDIR/roundtrip/case19-dst/file1.txt" \
        "remote file -> local dir with trailing slash (roundtrip)"
else
    fail "remote file -> local dir with trailing slash: copy failed (exit $ret)"
fi

# ============================================================
# Case 20: local dir -> remote existing dir with trailing slash
# ============================================================
echo ""
echo "--- Case 20: local dir -> remote existing dir with trailing slash ---"
run_filetool "$WORKDIR/empty-staging" "${REMOTE_TESTDIR}/trail-dst"
run_filetool "$WORKDIR/local-src" "${REMOTE_TESTDIR}/trail-dst/"
ret=$?
if [ $ret -eq 0 ]; then
    rm -rf "$WORKDIR/roundtrip/case20"
    mkdir -p "$WORKDIR/roundtrip/case20"
    run_filetool "${REMOTE_TESTDIR}/trail-dst/local-src" "$WORKDIR/roundtrip/case20"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    if diff -r "$WORKDIR/local-src" "$WORKDIR/roundtrip/case20/local-src" >/dev/null 2>&1; then
        pass "local dir -> remote existing dir with trailing slash: diff -r matches"
    else
        fail "local dir -> remote existing dir with trailing slash: diff -r shows differences"
    fi
else
    fail "local dir -> remote existing dir with trailing slash: copy failed (exit $ret)"
fi

# ============================================================
# Case 21: source file with trailing slash (not a directory)
# ============================================================
echo ""
echo "--- Case 21: source file with trailing slash (error) ---"
run_filetool_stderr "$WORKDIR/local-src/file1.txt/" "${REMOTE_TESTDIR}/dst"
assert_error $? "source file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
    pass "source file with trailing slash: error message says 'is not a directory'"
else
    fail "source file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

# ============================================================
# Case 22: destination file with trailing slash (not a directory)
# ============================================================
echo ""
echo "--- Case 22: destination file with trailing slash (error) ---"
run_filetool_stderr "$WORKDIR/local-src" "${REMOTE_TESTDIR}/exist.txt/"
assert_error $? "destination file with trailing slash: errors"
if echo "$FILE_TOOL_STDERR" | grep -qiE "(is not a directory|not a directory)"; then
    pass "destination file with trailing slash: error message says 'is not a directory'"
else
    fail "destination file with trailing slash: expected 'is not a directory' in stderr, got: $FILE_TOOL_STDERR"
fi

# ============================================================
# Case 23: /proc/version local -> remote -> local, md5sum roundtrip
# ============================================================
LOCAL_PROC_VERSION_MD5=$(md5sum /proc/version | awk '{print $1}')
echo ""
echo "--- Case 23: /proc/version roundtrip md5sum ---"
rm -rf "$WORKDIR/roundtrip/case23"
mkdir -p "$WORKDIR/roundtrip/case23"
run_filetool /proc/version "${REMOTE_TESTDIR}/dst"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/dst/version" "$WORKDIR/roundtrip/case23"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    ROUNDTRIP_MD5=$(md5sum "$WORKDIR/roundtrip/case23/version" | awk '{print $1}')
    assert_eq "$ROUNDTRIP_MD5" "$LOCAL_PROC_VERSION_MD5" \
        "/proc/version -> remote -> local: roundtrip md5sum matches"
else
    fail "/proc/version roundtrip: copy failed (exit $ret)"
fi

# ============================================================
# Case 24: remote dir -> local, verify symlinks preserved
# ============================================================
echo ""
echo "--- Case 24: remote symlinks dir -> local, verify symlinks preserved ---"
rm -rf "$WORKDIR/roundtrip/case24"
mkdir -p "$WORKDIR/roundtrip/case24"
# Use the remote symlinks directory pushed in Case 9
run_filetool "${REMOTE_TESTDIR}/symlinks" "$WORKDIR/roundtrip/case24"
ret=$?
if [ $ret -eq 0 ]; then
    assert_symlinks_match "$WORKDIR/local-symlinks" "$WORKDIR/roundtrip/case24/symlinks" \
        "remote symlinks dir -> local: targets match"
else
    fail "remote symlinks dir -> local: copy failed (exit $ret)"
fi

# ============================================================
# Case 25: single local symlink -> remote, verify target preserved
# ============================================================
echo ""
echo "--- Case 25: single local symlink -> remote, verify target preserved ---"
ln -sf "hosts-target" "$WORKDIR/standalone-link"
createtext "$WORKDIR/hosts-target" "hosts target content"
rm -rf "$WORKDIR/roundtrip/case25"
mkdir -p "$WORKDIR/roundtrip/case25"
run_filetool "$WORKDIR/standalone-link" "${REMOTE_TESTDIR}/dst"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/dst/standalone-link" "$WORKDIR/roundtrip/case25"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/roundtrip/case25/standalone-link" 2>/dev/null)
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

# ============================================================
# Case 26: single remote symlink -> local, verify target preserved
# ============================================================
echo ""
echo "--- Case 26: single remote symlink -> local, verify target preserved ---"
# Create a standalone symlink on the remote side by pushing one from local
ln -sf "rfile-target" "$WORKDIR/remote-standalone-link"
createtext "$WORKDIR/rfile-target" "remote standalone target"
run_filetool "$WORKDIR/remote-standalone-link" "${REMOTE_TESTDIR}/dst"
# Now copy that remote symlink as a standalone source back to local
rm -rf "$WORKDIR/roundtrip/case26"
mkdir -p "$WORKDIR/roundtrip/case26"
run_filetool "${REMOTE_TESTDIR}/dst/remote-standalone-link" "$WORKDIR/roundtrip/case26"
ret=$?
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/roundtrip/case26/remote-standalone-link" 2>/dev/null)
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

# ============================================================
# Case 27: single local broken symlink -> remote
# ============================================================
echo ""
echo "--- Case 27: single local broken symlink -> remote ---"
ln -sf "/nonexistent/target/path" "$WORKDIR/broken-link"
rm -rf "$WORKDIR/roundtrip/case27"
mkdir -p "$WORKDIR/roundtrip/case27"
run_filetool "$WORKDIR/broken-link" "${REMOTE_TESTDIR}/dst"
ret=$?
if [ $ret -eq 0 ]; then
    run_filetool "${REMOTE_TESTDIR}/dst/broken-link" "$WORKDIR/roundtrip/case27"
    ret=$?
fi
if [ $ret -eq 0 ]; then
    dst_target=$(readlink "$WORKDIR/roundtrip/case27/broken-link" 2>/dev/null)
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
# --ls tests: verify listing of different file types on remote Windows
# ============================================================

# Create local test data for --ls, then push to remote
mkdir -p "$WORKDIR/ls-test/subdir"
createtext "$WORKDIR/ls-test/regular.txt" "regular file for --ls test"
createtext "$WORKDIR/ls-test/subdir/nested.txt" "nested file for --ls test"
ln -sf "regular.txt" "$WORKDIR/ls-test/link1"
ln -sf "/nonexistent/target" "$WORKDIR/ls-test/broken"
ln -sf "subdir" "$WORKDIR/ls-test/link-to-dir"
rm -rf "$WORKDIR/ls-test-staging"
run_filetool "$WORKDIR/ls-test" "${REMOTE_TESTDIR}/ls-test"
if [ $? -ne 0 ]; then
    echo "WARNING: Failed to push --ls test data to remote; --ls tests may fail"
fi

# Helper: run --ls, capture stdout+stderr separately. Returns exit code.
run_filetool_ls_capture() {
    # Writes stdout to $LS_STDOUT, stderr to $LS_STDERR
    local tmpout tmperr ret
    tmpout=$(mktemp)
    tmperr=$(mktemp)
    "$COOLEDIT" --filetool --ls "$@" >"$tmpout" 2>"$tmperr"
    ret=$?
    LS_STDOUT=$(cat "$tmpout")
    LS_STDERR=$(cat "$tmperr")
    rm -f "$tmpout" "$tmperr"
    return $ret
}

# ============================================================
# Case 28: --ls -l symlink -> target arrow and non-zero size
# ============================================================
echo ""
echo "--- Case 28: --ls -l symlink shows -> target and size ---"
# symlink to file: should show "link1 -> regular.txt" with size 11
run_filetool_ls_capture -l "${REMOTE_TESTDIR}/ls-test/link1"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "link1 -> regular.txt"; then
        pass "--ls -l symlink-to-file: shows '-> regular.txt'"
    else
        fail "--ls -l symlink-to-file: missing -> target, got: $LS_STDOUT"
    fi
    size=$(echo "$LS_STDOUT" | awk '{print $5}')
    if [ "$size" != "0" ] && [ -n "$size" ]; then
        pass "--ls -l symlink-to-file: size is $size (non-zero, expected length of 'regular.txt' = 11)"
    else
        fail "--ls -l symlink-to-file: size is 0, expected 11 (length of 'regular.txt')"
    fi
else
    fail "--ls -l symlink-to-file: failed (exit $ret, stderr: $LS_STDERR)"
fi
# symlink to directory: should show "link-to-dir -> subdir" with size 6
run_filetool_ls_capture -l "${REMOTE_TESTDIR}/ls-test/link-to-dir"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "link-to-dir -> subdir"; then
        pass "--ls -l symlink-to-dir: shows '-> subdir'"
    else
        fail "--ls -l symlink-to-dir: missing -> target, got: $LS_STDOUT"
    fi
    size=$(echo "$LS_STDOUT" | awk '{print $5}')
    if [ "$size" != "0" ] && [ -n "$size" ]; then
        pass "--ls -l symlink-to-dir: size is $size (non-zero, expected length of 'subdir' = 6)"
    else
        fail "--ls -l symlink-to-dir: size is 0, expected 6 (length of 'subdir')"
    fi
else
    fail "--ls -l symlink-to-dir: failed (exit $ret, stderr: $LS_STDERR)"
fi

# ============================================================
# Case 29: --ls directory listing includes broken symlink with -> target
# ============================================================
echo ""
echo "--- Case 29: --ls directory listing includes broken symlink ---"
run_filetool_ls_capture -l "${REMOTE_TESTDIR}/ls-test"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "broken"; then
        pass "--ls -l dir: broken symlink appears in listing"
    else
        fail "--ls -l dir: broken symlink missing from listing, got: $LS_STDOUT"
    fi
    if echo "$LS_STDOUT" | grep -q "broken -> /nonexistent/target"; then
        pass "--ls -l dir: broken symlink shows -> target"
    else
        fail "--ls -l dir: broken symlink missing -> /nonexistent/target, got: $LS_STDOUT"
    fi
else
    fail "--ls -l dir: failed (exit $ret, stderr: $LS_STDERR)"
fi

# ============================================================
# Case 30: --ls on broken symlink directly
# ============================================================
echo ""
echo "--- Case 30: --ls on broken symlink directly ---"
run_filetool_ls_capture "${REMOTE_TESTDIR}/ls-test/broken"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "broken"; then
        pass "--ls broken symlink: output contains name"
    else
        fail "--ls broken symlink: output missing name, got: $LS_STDOUT"
    fi
else
    fail "--ls broken symlink: failed — broken symlinks should be listable (exit $ret, stderr: $LS_STDERR)"
fi
run_filetool_ls_capture -l "${REMOTE_TESTDIR}/ls-test/broken"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "broken -> /nonexistent/target"; then
        pass "--ls -l broken symlink: shows -> target"
    else
        fail "--ls -l broken symlink: missing -> target, got: $LS_STDOUT"
    fi
else
    fail "--ls -l broken symlink: failed — broken symlinks should be listable (exit $ret, stderr: $LS_STDERR)"
fi

# ============================================================
# Case 31: --ls regular file: no symlink arrow, correct size
# ============================================================
echo ""
echo "--- Case 31: --ls regular file: no false symlink arrow ---"
run_filetool_ls_capture -l "${REMOTE_TESTDIR}/ls-test/regular.txt"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "regular.txt" && ! echo "$LS_STDOUT" | grep -q "regular.txt ->"; then
        pass "--ls -l regular file: no symlink arrow (correct)"
    else
        fail "--ls -l regular file: incorrectly shows -> arrow, got: $LS_STDOUT"
    fi
    size=$(echo "$LS_STDOUT" | awk '{print $5}')
    if [ "$size" = "27" ]; then
        pass "--ls -l regular file: size is 27 (correct)"
    else
        fail "--ls -l regular file: size is $size, expected 27"
    fi
else
    fail "--ls -l regular file: failed (exit $ret, stderr: $LS_STDERR)"
fi

# ============================================================
# Case 32: --ls directory: -d shows name without ->, lists contents without -d
# ============================================================
echo ""
echo "--- Case 32: --ls directory listing ---"
run_filetool_ls_capture "${REMOTE_TESTDIR}/ls-test/subdir"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "nested.txt"; then
        pass "--ls directory: lists contents (nested.txt found)"
    else
        fail "--ls directory: nested.txt not in listing, got: $LS_STDOUT"
    fi
else
    fail "--ls directory: failed (exit $ret, stderr: $LS_STDERR)"
fi
run_filetool_ls_capture -d "${REMOTE_TESTDIR}/ls-test/subdir"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "subdir" && ! echo "$LS_STDOUT" | grep -q "nested.txt"; then
        pass "--ls -d directory: lists directory name, not contents"
    else
        fail "--ls -d directory: -d flag ignored or missing name, got: $LS_STDOUT"
    fi
else
    fail "--ls -d directory: failed (exit $ret, stderr: $LS_STDERR)"
fi

# ============================================================
# Case 33: --ls symlink-to-dir: follows by default, -d shows symlink itself
# ============================================================
echo ""
echo "--- Case 33: --ls symlink-to-directory ---"
run_filetool_ls_capture "${REMOTE_TESTDIR}/ls-test/link-to-dir"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "nested.txt"; then
        pass "--ls symlink-to-dir: follows symlink, lists target contents"
    else
        fail "--ls symlink-to-dir: target contents not listed, got: $LS_STDOUT"
    fi
else
    fail "--ls symlink-to-dir: failed (exit $ret, stderr: $LS_STDERR)"
fi
	# Added test: --ls -l symlink-to-dir should show symlink itself, not contents
	run_filetool_ls_capture -l "${REMOTE_TESTDIR}/ls-test/link-to-dir"
	ret=$?
	if [ $ret -eq 0 ]; then
	    if echo "$LS_STDOUT" | grep -q "link-to-dir -> subdir" && ! echo "$LS_STDOUT" | grep -q "nested.txt"; then
	        pass "--ls -l symlink-to-dir: shows symlink entry, does not list contents"
	    else
	        fail "--ls -l symlink-to-dir: expected symlink entry only, got: $LS_STDOUT"
	    fi
	else
	    fail "--ls -l symlink-to-dir: failed (exit $ret, stderr: $LS_STDERR)"
	fi
run_filetool_ls_capture -l -d "${REMOTE_TESTDIR}/ls-test/link-to-dir"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "link-to-dir -> subdir"; then
        pass "--ls -l -d symlink-to-dir: shows symlink name with -> target"
    else
        fail "--ls -l -d symlink-to-dir: missing -> target, got: $LS_STDOUT"
    fi
else
    fail "--ls -l -d symlink-to-dir: failed (exit $ret, stderr: $LS_STDERR)"
fi


# ============================================================
# Case 34: --ls -l symlink-to-dir/ with trailing slash follows symlink
# ============================================================
echo ""
echo "--- Case 34: --ls -l symlink-to-dir with trailing slash ---"
run_filetool_ls_capture -l "${REMOTE_TESTDIR}/ls-test/link-to-dir/"
ret=$?
if [ $ret -eq 0 ]; then
    if echo "$LS_STDOUT" | grep -q "nested.txt" && ! echo "$LS_STDOUT" | grep -q "link-to-dir ->"; then
        pass "--ls -l symlink-to-dir/: follows symlink, lists target contents"
    else
        fail "--ls -l symlink-to-dir/: expected directory contents, got: $LS_STDOUT"
    fi
else
    fail "--ls -l symlink-to-dir/: failed (exit $ret, stderr: $LS_STDERR)"
fi
# ============================================================
# Case 35: remote dir with trailing dot/space in source path
# ============================================================
echo ""
echo "--- Case 35: remote dir with trailing dot/space in source path ---"
# Create a remote directory without any trailing dot or space
run_filetool "$WORKDIR/empty-staging" "${REMOTE_TESTDIR}/trailspec-dir"
ret=$?
if [ $ret -ne 0 ]; then
    fail "trailing dot/space: failed to create remote test dir"
else
    # --- trailing dot ---
    rm -rf "$WORKDIR/roundtrip/case35-dot"
    mkdir -p "$WORKDIR/roundtrip/case35-dot"
    run_filetool_stderr "${REMOTE_TESTDIR}/trailspec-dir." "$WORKDIR/roundtrip/case35-dot"
    ret=$?
    if [ $ret -ne 0 ]; then
        pass "remote dir with trailing dot: correctly errors (path does not exist)"
    elif [ -d "$WORKDIR/roundtrip/case35-dot/trailspec-dir." ]; then
        fail "remote dir with trailing dot: created local dir with trailing dot (BUG: trailing dot preserved)"
    elif [ -d "$WORKDIR/roundtrip/case35-dot/trailspec-dir" ]; then
        fail "remote dir with trailing dot: copy succeeded and trailing dot silently stripped (should have errored)"
    else
        fail "remote dir with trailing dot: copy succeeded but unexpected result"
    fi

    # --- trailing space ---
    rm -rf "$WORKDIR/roundtrip/case35-space"
    mkdir -p "$WORKDIR/roundtrip/case35-space"
    run_filetool_stderr "${REMOTE_TESTDIR}/trailspec-dir " "$WORKDIR/roundtrip/case35-space"
    ret=$?
    if [ $ret -ne 0 ]; then
        pass "remote dir with trailing space: correctly errors (path does not exist)"
    elif [ -d "$WORKDIR/roundtrip/case35-space/trailspec-dir " ]; then
        fail "remote dir with trailing space: created local dir with trailing space (BUG: trailing space preserved)"
    elif [ -d "$WORKDIR/roundtrip/case35-space/trailspec-dir" ]; then
        fail "remote dir with trailing space: copy succeeded and trailing space silently stripped (should have errored)"
    else
        fail "remote dir with trailing space: copy succeeded but unexpected result"
    fi
fi

# ============================================================
# Case 36: remote dir with trailing dot/space — verify --ls also errors
# ============================================================
echo ""
echo "--- Case 36: --ls on remote dir with trailing dot/space ---"
run_filetool_ls_capture "${REMOTE_TESTDIR}/trailspec-dir."
ret=$?
if [ $ret -ne 0 ]; then
    pass "--ls remote dir with trailing dot: correctly errors (path does not exist)"
else
    if echo "$LS_STDOUT" | grep -q "trailspec-dir"; then
        fail "--ls remote dir with trailing dot: succeeded but should have errored (listing '$LS_STDOUT')"
    else
        fail "--ls remote dir with trailing dot: succeeded unexpectedly (output: $LS_STDOUT)"
    fi
fi
run_filetool_ls_capture "${REMOTE_TESTDIR}/trailspec-dir "
ret=$?
if [ $ret -ne 0 ]; then
    pass "--ls remote dir with trailing space: correctly errors (path does not exist)"
else
    if echo "$LS_STDOUT" | grep -q "trailspec-dir"; then
        fail "--ls remote dir with trailing space: succeeded but should have errored (listing '$LS_STDOUT')"
    else
        fail "--ls remote dir with trailing space: succeeded unexpectedly (output: $LS_STDOUT)"
    fi
fi

# ============================================================
# Case 37: Windows backslash path — basename extraction
# ============================================================
echo ""
echo "--- Case 37: Windows backslash path basename extraction ---"
# First create a file on remote using forward slashes (known working)
createtext "$WORKDIR/local-src/basename.txt" "backslash basename test content"
run_filetool "$WORKDIR/local-src/basename.txt" "${REMOTE_TESTDIR}/src"
ret=$?
if [ $ret -ne 0 ]; then
    fail "backslash basename: failed to push test file to remote"
else
    rm -rf "$WORKDIR/roundtrip/case37"
    mkdir -p "$WORKDIR/roundtrip/case37"
    # Build a backslash path for the same file:
    #   host:C:\Users\Owner\REMOTEFS-TESTS\filetool-test-$$\src\basename.txt
    REMOTE_BACKSLASH="${REMOTE_TESTDIR}/src/basename.txt"
    REMOTE_BACKSLASH=$(echo "$REMOTE_BACKSLASH" | sed 's|/|\\|g')
    run_filetool "${REMOTE_BACKSLASH}" "$WORKDIR/roundtrip/case37"
    ret=$?
    if [ $ret -ne 0 ]; then
        fail "backslash basename: copy failed (exit $ret)"
    elif [ -f "$WORKDIR/roundtrip/case37/basename.txt" ]; then
        pass "backslash basename: file created with correct basename (backslash path separator honored)"
    elif [ -f "$WORKDIR/roundtrip/case37/${REMOTE_BACKSLASH#*:}" ]; then
        fail "backslash basename: BUG — created file with full backslash path as name instead of just basename"
    else
        fail "backslash basename: unexpected result — no basename.txt and no full-path file found"
    fi
fi

# ============================================================
# Case 38: multi-source with mixed forward/backslash separators
# ============================================================
echo ""
echo "--- Case 38: multi-source with mixed forward/backslash separators ---"
# Push three files to remote using forward slashes
createtext "$WORKDIR/local-src/mixed1.txt" "mixed slash test 1"
createtext "$WORKDIR/local-src/mixed2.txt" "mixed slash test 2"
createtext "$WORKDIR/local-src/mixed3.txt" "mixed slash test 3"
run_filetool "$WORKDIR/local-src/mixed1.txt" "${REMOTE_TESTDIR}/src"
run_filetool "$WORKDIR/local-src/mixed2.txt" "${REMOTE_TESTDIR}/src"
run_filetool "$WORKDIR/local-src/mixed3.txt" "${REMOTE_TESTDIR}/src"
# Build three variants of the same remote directory path:
#   all-backslash:   host:C:\...\src
#   all-forward:     host:C:/.../src
#   mixed:           host:C:/...\src   (forward then backslash)
REMOTE_FWD="${REMOTE_TESTDIR}/src"
REMOTE_BSL=$(echo "${REMOTE_TESTDIR}/src" | sed 's|/|\\|g')
# Mixed: only the path separator before "src" is backslash, rest forward
REMOTE_MIXED="${REMOTE_TESTDIR}"'\src'

rm -rf "$WORKDIR/roundtrip/case38"
mkdir -p "$WORKDIR/roundtrip/case38"
run_filetool \
    "${REMOTE_BSL}\\mixed1.txt" \
    "${REMOTE_FWD}/mixed2.txt" \
    "${REMOTE_MIXED}\\mixed3.txt" \
    "$WORKDIR/roundtrip/case38"
ret=$?
if [ $ret -ne 0 ]; then
    fail "mixed slashes: multi-source copy failed (exit $ret)"
else
    ok=1
    for f in mixed1.txt mixed2.txt mixed3.txt; do
        if [ -f "$WORKDIR/roundtrip/case38/$f" ]; then
            pass "mixed slashes: $f extracted correctly"
        else
            fail "mixed slashes: $f missing from destination"
            ok=0
        fi
    done
    [ "$ok" -eq 1 ] && ls_out=$(ls "$WORKDIR/roundtrip/case38") && \
        pass "mixed slashes: all three basenames correct (got: $ls_out)"
fi

# ============================================================
# Results
# ============================================================
echo ""
echo "=== Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ "$FAILED" -gt 0 ]; then
    exit 1
fi

echo ""
echo "All tests passed."
exit 0

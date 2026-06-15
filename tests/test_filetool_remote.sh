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
    d=$(mktemp -d "$WORKDIR/filetool-remote-test-XXXXXX")
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

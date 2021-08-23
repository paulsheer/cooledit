/* remotefs.c - remote fs access
   Copyright (C) 1996-2020 Paul Sheer
 */

#include "global.h"
#ifdef MSWIN
#include <config-mswin.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <error.h>
#else
#include <config.h>
#endif

#include <assert.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#include <my_string.h>
#include "stringtools.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_IOCTL_H
#include <ioctl.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifndef MSWIN
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/sysmacros.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "remotefs.h"
#include "dirtools.h"


#ifdef MSWIN

#define makedev(a,b)            (((a) << 8) | ((b) & 0xFF))
#define random()                rand()
#define rename(a,b)             windows_rename((a),(b))

#undef INVALID_HANDLE_VALUE
#define HANDLE                  int
#define INVALID_HANDLE_VALUE    (-1)

#define socklen_t               int

#else

#define _O_BINARY               0
#define SOCKET                  int
#define INVALID_SOCKET          (-1)
#define SOCKET_ERROR            (-1)
#define HANDLE                  int
#define INVALID_HANDLE_VALUE    (-1)
#define ioctlsocket(a,b,c)      ioctl(a,b,c)
#define closesocket(s)          close(s)
#define socklen_t               unsigned int

#endif


int option_remote_timeout = 2000;


char *pathdup_ (const char *p, const char *home_dir);

static int translate_unix_errno (int err);

#ifdef MSWIN

static int windows_rename (const char *a, const char *b)
{
    if (!MoveFileExA (a, b, MOVEFILE_REPLACE_EXISTING)) {
        errno = GetLastError ();
        return -1;
    }
    return 0;
}

#define PATH_SEP_STR            "\\"
#define IS_DRIVE_LETTER(a)      ((a >= 'A' && a <= 'Z') || (a >= 'a' && a <= 'z'))
#define TOUPPER(a)              ((a >= 'A' && a <= 'Z') ? (a) : ((a) - ('a' - 'A')))

static const char *translate_path_sep (const char *p)
{
    char *s;
    static unsigned int rotate = 0;
    static char r_[2][MAX_PATH_LEN * 2];
    char *r;
    int l;
    rotate++;
    r = r_[rotate & 1];
    l = strlen (p);
    if (l > sizeof (r_[0]) - 4)
        return "";
    if (p[0] == '/') {
/*   /D:/Users/Fred  ->  D:\Users\Fred
     /D:             ->  D:\
  */
        if (IS_DRIVE_LETTER (p[1]) && p[2] == ':' && (p[3] == '/' || !p[3])) {
            r[0] = TOUPPER (p[1]);
            r[1] = ':';
            if (!p[3])
                strcpy (r + 2, "/");
            else
                strcpy (r + 2, p + 3);
        } else {
            r[0] = 'C';
            r[1] = ':';
            strcpy (r + 2, p);
        }
    }
    if (!r[0])
        return "\\";
    for (s = r; *s; s++)
        if (*s == '/')
            *s = '\\';

    return r;
}

static char *windows_path_to_unix (const char *p)
{
    int l;
    char *r, *q;

/*   convert   C:\The\Path   to   /The/Path    */
/*   convert   D:\The\Path   to   /D:/The/Path  */

    l = strlen (p) + 4;
    r = (char *) malloc (l);
    if (!strncmp (p, "C:\\", 3) || !strncmp (p, "c:\\", 3)) {
        strcpy (r, p + 2);
    } else if (IS_DRIVE_LETTER (p[0]) && p[1] == ':' && (p[2] == '\\' || !p[2])) {
        r[0] = '/';
        r[1] = TOUPPER (p[0]);
        r[2] = ':';
        strcpy (r + 3, p + 2);
    } else {
        strcpy (r, p);
    }
    for (q = r; *q; q++)
        if (*q == '\\')
            *q = '/';

    return r;
}

static int translate_mswin_lasterror (int mswin_error)
{
    switch (mswin_error) {
    case ERROR_INVALID_FUNCTION:
        return RFSERR_MSWIN_ERROR_INVALID_FUNCTION;
    case ERROR_FILE_NOT_FOUND:
        return RFSERR_MSWIN_ERROR_FILE_NOT_FOUND;
    case ERROR_PATH_NOT_FOUND:
        return RFSERR_MSWIN_ERROR_PATH_NOT_FOUND;
    case ERROR_TOO_MANY_OPEN_FILES:
        return RFSERR_MSWIN_ERROR_TOO_MANY_OPEN_FILES;
    case ERROR_ACCESS_DENIED:
        return RFSERR_MSWIN_ERROR_ACCESS_DENIED;
    case ERROR_INVALID_HANDLE:
        return RFSERR_MSWIN_ERROR_INVALID_HANDLE;
    case ERROR_ARENA_TRASHED:
        return RFSERR_MSWIN_ERROR_ARENA_TRASHED;
    case ERROR_NOT_ENOUGH_MEMORY:
        return RFSERR_MSWIN_ERROR_NOT_ENOUGH_MEMORY;
    case ERROR_INVALID_BLOCK:
        return RFSERR_MSWIN_ERROR_INVALID_BLOCK;
    case ERROR_BAD_ENVIRONMENT:
        return RFSERR_MSWIN_ERROR_BAD_ENVIRONMENT;
    case ERROR_BAD_FORMAT:
        return RFSERR_MSWIN_ERROR_BAD_FORMAT;
    case ERROR_INVALID_ACCESS:
        return RFSERR_MSWIN_ERROR_INVALID_ACCESS;
    case ERROR_INVALID_DATA:
        return RFSERR_MSWIN_ERROR_INVALID_DATA;
    case ERROR_INVALID_DRIVE:
        return RFSERR_MSWIN_ERROR_INVALID_DRIVE;
    case ERROR_CURRENT_DIRECTORY:
        return RFSERR_MSWIN_ERROR_CURRENT_DIRECTORY;
    case ERROR_NOT_SAME_DEVICE:
        return RFSERR_MSWIN_ERROR_NOT_SAME_DEVICE;
    case ERROR_NO_MORE_FILES:
        return RFSERR_MSWIN_ERROR_NO_MORE_FILES;
    case ERROR_WRITE_PROTECT:
        return RFSERR_MSWIN_ERROR_WRITE_PROTECT;
    case ERROR_BAD_UNIT:
        return RFSERR_MSWIN_ERROR_BAD_UNIT;
    case ERROR_NOT_READY:
        return RFSERR_MSWIN_ERROR_NOT_READY;
    case ERROR_BAD_COMMAND:
        return RFSERR_MSWIN_ERROR_BAD_COMMAND;
    case ERROR_CRC:
        return RFSERR_MSWIN_ERROR_CRC;
    case ERROR_BAD_LENGTH:
        return RFSERR_MSWIN_ERROR_BAD_LENGTH;
    case ERROR_SEEK:
        return RFSERR_MSWIN_ERROR_SEEK;
    case ERROR_NOT_DOS_DISK:
        return RFSERR_MSWIN_ERROR_NOT_DOS_DISK;
    case ERROR_SECTOR_NOT_FOUND:
        return RFSERR_MSWIN_ERROR_SECTOR_NOT_FOUND;
    case ERROR_OUT_OF_PAPER:
        return RFSERR_MSWIN_ERROR_OUT_OF_PAPER;
    case ERROR_WRITE_FAULT:
        return RFSERR_MSWIN_ERROR_WRITE_FAULT;
    case ERROR_READ_FAULT:
        return RFSERR_MSWIN_ERROR_READ_FAULT;
    case ERROR_GEN_FAILURE:
        return RFSERR_MSWIN_ERROR_GEN_FAILURE;
    case ERROR_SHARING_VIOLATION:
        return RFSERR_MSWIN_ERROR_SHARING_VIOLATION;
    case ERROR_LOCK_VIOLATION:
        return RFSERR_MSWIN_ERROR_LOCK_VIOLATION;
    case ERROR_WRONG_DISK:
        return RFSERR_MSWIN_ERROR_WRONG_DISK;
    case ERROR_FCB_UNAVAILABLE:
        return RFSERR_MSWIN_ERROR_FCB_UNAVAILABLE;
    case ERROR_SHARING_BUFFER_EXCEEDED:
        return RFSERR_MSWIN_ERROR_SHARING_BUFFER_EXCEEDED;
    case ERROR_NOT_SUPPORTED:
        return RFSERR_MSWIN_ERROR_NOT_SUPPORTED;
    case ERROR_FILE_EXISTS:
        return RFSERR_MSWIN_ERROR_FILE_EXISTS;
    case ERROR_DUP_FCB:
        return RFSERR_MSWIN_ERROR_DUP_FCB;
    case ERROR_CANNOT_MAKE:
        return RFSERR_MSWIN_ERROR_CANNOT_MAKE;
    case ERROR_FAIL_I24:
        return RFSERR_MSWIN_ERROR_FAIL_I24;
    case ERROR_OUT_OF_STRUCTURES:
        return RFSERR_MSWIN_ERROR_OUT_OF_STRUCTURES;
    case ERROR_ALREADY_ASSIGNED:
        return RFSERR_MSWIN_ERROR_ALREADY_ASSIGNED;
    case ERROR_INVALID_PASSWORD:
        return RFSERR_MSWIN_ERROR_INVALID_PASSWORD;
    case ERROR_INVALID_PARAMETER:
        return RFSERR_MSWIN_ERROR_INVALID_PARAMETER;
    case ERROR_NET_WRITE_FAULT:
        return RFSERR_MSWIN_ERROR_NET_WRITE_FAULT;
    case ERROR_NO_PROC_SLOTS:
        return RFSERR_MSWIN_ERROR_NO_PROC_SLOTS;
    case ERROR_NOT_FROZEN:
        return RFSERR_MSWIN_ERROR_NOT_FROZEN;
    case ERROR_NO_ITEMS:
        return RFSERR_MSWIN_ERROR_NO_ITEMS;
    case ERROR_INTERRUPT:
        return RFSERR_MSWIN_ERROR_INTERRUPT;
    case ERROR_TOO_MANY_SEMAPHORES:
        return RFSERR_MSWIN_ERROR_TOO_MANY_SEMAPHORES;
    case ERROR_EXCL_SEM_ALREADY_OWNED:
        return RFSERR_MSWIN_ERROR_EXCL_SEM_ALREADY_OWNED;
    case ERROR_SEM_IS_SET:
        return RFSERR_MSWIN_ERROR_SEM_IS_SET;
    case ERROR_TOO_MANY_SEM_REQUESTS:
        return RFSERR_MSWIN_ERROR_TOO_MANY_SEM_REQUESTS;
    case ERROR_INVALID_AT_INTERRUPT_TIME:
        return RFSERR_MSWIN_ERROR_INVALID_AT_INTERRUPT_TIME;
    case ERROR_SEM_OWNER_DIED:
        return RFSERR_MSWIN_ERROR_SEM_OWNER_DIED;
    case ERROR_SEM_USER_LIMIT:
        return RFSERR_MSWIN_ERROR_SEM_USER_LIMIT;
    case ERROR_DISK_CHANGE:
        return RFSERR_MSWIN_ERROR_DISK_CHANGE;
    case ERROR_DRIVE_LOCKED:
        return RFSERR_MSWIN_ERROR_DRIVE_LOCKED;
    case ERROR_BROKEN_PIPE:
        return RFSERR_MSWIN_ERROR_BROKEN_PIPE;
    case ERROR_OPEN_FAILED:
        return RFSERR_MSWIN_ERROR_OPEN_FAILED;
    case ERROR_BUFFER_OVERFLOW:
        return RFSERR_MSWIN_ERROR_BUFFER_OVERFLOW;
    case ERROR_DISK_FULL:
        return RFSERR_MSWIN_ERROR_DISK_FULL;
    case ERROR_NO_MORE_SEARCH_HANDLES:
        return RFSERR_MSWIN_ERROR_NO_MORE_SEARCH_HANDLES;
    case ERROR_INVALID_TARGET_HANDLE:
        return RFSERR_MSWIN_ERROR_INVALID_TARGET_HANDLE;
    case ERROR_PROTECTION_VIOLATION:
        return RFSERR_MSWIN_ERROR_PROTECTION_VIOLATION;
    case ERROR_VIOKBD_REQUEST:
        return RFSERR_MSWIN_ERROR_VIOKBD_REQUEST;
    case ERROR_INVALID_CATEGORY:
        return RFSERR_MSWIN_ERROR_INVALID_CATEGORY;
    case ERROR_INVALID_VERIFY_SWITCH:
        return RFSERR_MSWIN_ERROR_INVALID_VERIFY_SWITCH;
    case ERROR_BAD_DRIVER_LEVEL:
        return RFSERR_MSWIN_ERROR_BAD_DRIVER_LEVEL;
    case ERROR_CALL_NOT_IMPLEMENTED:
        return RFSERR_MSWIN_ERROR_CALL_NOT_IMPLEMENTED;
    case ERROR_SEM_TIMEOUT:
        return RFSERR_MSWIN_ERROR_SEM_TIMEOUT;
    case ERROR_INSUFFICIENT_BUFFER:
        return RFSERR_MSWIN_ERROR_INSUFFICIENT_BUFFER;
    case ERROR_INVALID_NAME:
        return RFSERR_MSWIN_ERROR_INVALID_NAME;
    case ERROR_INVALID_LEVEL:
        return RFSERR_MSWIN_ERROR_INVALID_LEVEL;
    case ERROR_NO_VOLUME_LABEL:
        return RFSERR_MSWIN_ERROR_NO_VOLUME_LABEL;
    case ERROR_MOD_NOT_FOUND:
        return RFSERR_MSWIN_ERROR_MOD_NOT_FOUND;
    case ERROR_PROC_NOT_FOUND:
        return RFSERR_MSWIN_ERROR_PROC_NOT_FOUND;
    case ERROR_WAIT_NO_CHILDREN:
        return RFSERR_MSWIN_ERROR_WAIT_NO_CHILDREN;
    case ERROR_CHILD_NOT_COMPLETE:
        return RFSERR_MSWIN_ERROR_CHILD_NOT_COMPLETE;
    case ERROR_DIRECT_ACCESS_HANDLE:
        return RFSERR_MSWIN_ERROR_DIRECT_ACCESS_HANDLE;
    case ERROR_NEGATIVE_SEEK:
        return RFSERR_MSWIN_ERROR_NEGATIVE_SEEK;
    case ERROR_SEEK_ON_DEVICE:
        return RFSERR_MSWIN_ERROR_SEEK_ON_DEVICE;
    case ERROR_IS_JOIN_TARGET:
        return RFSERR_MSWIN_ERROR_IS_JOIN_TARGET;
    case ERROR_IS_JOINED:
        return RFSERR_MSWIN_ERROR_IS_JOINED;
    case ERROR_IS_SUBSTED:
        return RFSERR_MSWIN_ERROR_IS_SUBSTED;
    case ERROR_NOT_JOINED:
        return RFSERR_MSWIN_ERROR_NOT_JOINED;
    case ERROR_NOT_SUBSTED:
        return RFSERR_MSWIN_ERROR_NOT_SUBSTED;
    case ERROR_JOIN_TO_JOIN:
        return RFSERR_MSWIN_ERROR_JOIN_TO_JOIN;
    case ERROR_SUBST_TO_SUBST:
        return RFSERR_MSWIN_ERROR_SUBST_TO_SUBST;
    case ERROR_JOIN_TO_SUBST:
        return RFSERR_MSWIN_ERROR_JOIN_TO_SUBST;
    case ERROR_SUBST_TO_JOIN:
        return RFSERR_MSWIN_ERROR_SUBST_TO_JOIN;
    case ERROR_BUSY_DRIVE:
        return RFSERR_MSWIN_ERROR_BUSY_DRIVE;
    case ERROR_SAME_DRIVE:
        return RFSERR_MSWIN_ERROR_SAME_DRIVE;
    case ERROR_DIR_NOT_ROOT:
        return RFSERR_MSWIN_ERROR_DIR_NOT_ROOT;
    case ERROR_DIR_NOT_EMPTY:
        return RFSERR_MSWIN_ERROR_DIR_NOT_EMPTY;
    case ERROR_IS_SUBST_PATH:
        return RFSERR_MSWIN_ERROR_IS_SUBST_PATH;
    case ERROR_IS_JOIN_PATH:
        return RFSERR_MSWIN_ERROR_IS_JOIN_PATH;
    case ERROR_PATH_BUSY:
        return RFSERR_MSWIN_ERROR_PATH_BUSY;
    case ERROR_IS_SUBST_TARGET:
        return RFSERR_MSWIN_ERROR_IS_SUBST_TARGET;
    case ERROR_SYSTEM_TRACE:
        return RFSERR_MSWIN_ERROR_SYSTEM_TRACE;
    case ERROR_INVALID_EVENT_COUNT:
        return RFSERR_MSWIN_ERROR_INVALID_EVENT_COUNT;
    case ERROR_TOO_MANY_MUXWAITERS:
        return RFSERR_MSWIN_ERROR_TOO_MANY_MUXWAITERS;
    case ERROR_INVALID_LIST_FORMAT:
        return RFSERR_MSWIN_ERROR_INVALID_LIST_FORMAT;
    case ERROR_LABEL_TOO_LONG:
        return RFSERR_MSWIN_ERROR_LABEL_TOO_LONG;
    case ERROR_TOO_MANY_TCBS:
        return RFSERR_MSWIN_ERROR_TOO_MANY_TCBS;
    case ERROR_SIGNAL_REFUSED:
        return RFSERR_MSWIN_ERROR_SIGNAL_REFUSED;
    case ERROR_DISCARDED:
        return RFSERR_MSWIN_ERROR_DISCARDED;
    case ERROR_NOT_LOCKED:
        return RFSERR_MSWIN_ERROR_NOT_LOCKED;
    case ERROR_BAD_THREADID_ADDR:
        return RFSERR_MSWIN_ERROR_BAD_THREADID_ADDR;
    case ERROR_BAD_ARGUMENTS:
        return RFSERR_MSWIN_ERROR_BAD_ARGUMENTS;
    case ERROR_BAD_PATHNAME:
        return RFSERR_MSWIN_ERROR_BAD_PATHNAME;
    case ERROR_SIGNAL_PENDING:
        return RFSERR_MSWIN_ERROR_SIGNAL_PENDING;
    case ERROR_UNCERTAIN_MEDIA:
        return RFSERR_MSWIN_ERROR_UNCERTAIN_MEDIA;
    case ERROR_MAX_THRDS_REACHED:
        return RFSERR_MSWIN_ERROR_MAX_THRDS_REACHED;
    case ERROR_MONITORS_NOT_SUPPORTED:
        return RFSERR_MSWIN_ERROR_MONITORS_NOT_SUPPORTED;
    case ERROR_INVALID_SEGMENT_NUMBER:
        return RFSERR_MSWIN_ERROR_INVALID_SEGMENT_NUMBER;
    case ERROR_INVALID_CALLGATE:
        return RFSERR_MSWIN_ERROR_INVALID_CALLGATE;
    case ERROR_INVALID_ORDINAL:
        return RFSERR_MSWIN_ERROR_INVALID_ORDINAL;
    case ERROR_ALREADY_EXISTS:
        return RFSERR_MSWIN_ERROR_ALREADY_EXISTS;
    case ERROR_NO_CHILD_PROCESS:
        return RFSERR_MSWIN_ERROR_NO_CHILD_PROCESS;
    case ERROR_CHILD_ALIVE_NOWAIT:
        return RFSERR_MSWIN_ERROR_CHILD_ALIVE_NOWAIT;
    case ERROR_INVALID_FLAG_NUMBER:
        return RFSERR_MSWIN_ERROR_INVALID_FLAG_NUMBER;
    case ERROR_SEM_NOT_FOUND:
        return RFSERR_MSWIN_ERROR_SEM_NOT_FOUND;
    case ERROR_INVALID_STARTING_CODESEG:
        return RFSERR_MSWIN_ERROR_INVALID_STARTING_CODESEG;
    case ERROR_INVALID_STACKSEG:
        return RFSERR_MSWIN_ERROR_INVALID_STACKSEG;
    case ERROR_INVALID_MODULETYPE:
        return RFSERR_MSWIN_ERROR_INVALID_MODULETYPE;
    case ERROR_INVALID_EXE_SIGNATURE:
        return RFSERR_MSWIN_ERROR_INVALID_EXE_SIGNATURE;
    case ERROR_EXE_MARKED_INVALID:
        return RFSERR_MSWIN_ERROR_EXE_MARKED_INVALID;
    case ERROR_BAD_EXE_FORMAT:
        return RFSERR_MSWIN_ERROR_BAD_EXE_FORMAT;
    case ERROR_ITERATED_DATA_EXCEEDS_64k:
        return RFSERR_MSWIN_ERROR_ITERATED_DATA_EXCEEDS_64k;
    case ERROR_INVALID_MINALLOCSIZE:
        return RFSERR_MSWIN_ERROR_INVALID_MINALLOCSIZE;
    case ERROR_DYNLINK_FROM_INVALID_RING:
        return RFSERR_MSWIN_ERROR_DYNLINK_FROM_INVALID_RING;
    case ERROR_IOPL_NOT_ENABLED:
        return RFSERR_MSWIN_ERROR_IOPL_NOT_ENABLED;
    case ERROR_INVALID_SEGDPL:
        return RFSERR_MSWIN_ERROR_INVALID_SEGDPL;
    case ERROR_AUTODATASEG_EXCEEDS_64k:
        return RFSERR_MSWIN_ERROR_AUTODATASEG_EXCEEDS_64k;
    case ERROR_RING2SEG_MUST_BE_MOVABLE:
        return RFSERR_MSWIN_ERROR_RING2SEG_MUST_BE_MOVABLE;
    case ERROR_RELOC_CHAIN_XEEDS_SEGLIM:
        return RFSERR_MSWIN_ERROR_RELOC_CHAIN_XEEDS_SEGLIM;
    case ERROR_INFLOOP_IN_RELOC_CHAIN:
        return RFSERR_MSWIN_ERROR_INFLOOP_IN_RELOC_CHAIN;
    case ERROR_ENVVAR_NOT_FOUND:
        return RFSERR_MSWIN_ERROR_ENVVAR_NOT_FOUND;
    case ERROR_NOT_CURRENT_CTRY:
        return RFSERR_MSWIN_ERROR_NOT_CURRENT_CTRY;
    case ERROR_NO_SIGNAL_SENT:
        return RFSERR_MSWIN_ERROR_NO_SIGNAL_SENT;
    case ERROR_FILENAME_EXCED_RANGE:
        return RFSERR_MSWIN_ERROR_FILENAME_EXCED_RANGE;
    case ERROR_RING2_STACK_IN_USE:
        return RFSERR_MSWIN_ERROR_RING2_STACK_IN_USE;
    case ERROR_META_EXPANSION_TOO_LONG:
        return RFSERR_MSWIN_ERROR_META_EXPANSION_TOO_LONG;
    case ERROR_INVALID_SIGNAL_NUMBER:
        return RFSERR_MSWIN_ERROR_INVALID_SIGNAL_NUMBER;
    case ERROR_THREAD_1_INACTIVE:
        return RFSERR_MSWIN_ERROR_THREAD_1_INACTIVE;
    case ERROR_INFO_NOT_AVAIL:
        return RFSERR_MSWIN_ERROR_INFO_NOT_AVAIL;
    case ERROR_LOCKED:
        return RFSERR_MSWIN_ERROR_LOCKED;
    case ERROR_BAD_DYNALINK:
        return RFSERR_MSWIN_ERROR_BAD_DYNALINK;
    case ERROR_TOO_MANY_MODULES:
        return RFSERR_MSWIN_ERROR_TOO_MANY_MODULES;
    case ERROR_NESTING_NOT_ALLOWED:
        return RFSERR_MSWIN_ERROR_NESTING_NOT_ALLOWED;
    }
    return RFSERR_MSWIN_ERROR_WITHOUT_TRANSLATION;
}

static char *wchar_to_char (const wchar_t * w)
{
    char *r;
    size_t c, n;

    n = wcslen (w) + 2;
    r = (char *) malloc (n * 2);
    c = 0;
    wcstombs_s (&c, r, n * 2, w, _TRUNCATE);
    r[c] = '\0';

    return r;
}

static const char *mswin_error_to_text (long error)
{
    wchar_t *s = NULL;
    static char r[1024];
    char *p;
    int l;
    FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) & s, 0, NULL);
    if (!s)
        return "unknown error";
    p = wchar_to_char (s);
    LocalFree (s);
    l = strlen (p);
    if (l > sizeof (r) - 1)
        l = sizeof (r) - 1;
    memcpy (r, p, l);
    r[l] = '\0';
    free (p);
/* strip trailing whitespace, newlines, and carriage returns: */
    while (l >= 0 && ((unsigned char *) r)[l] <= ' ')
        r[l--] = '\0';
    return r;
}

static int portable_stat (int link, const char *fname, struct portable_stat *p, int *just_not_there, enum remotefs_error_code *remotefs_error_code_, char *errmsg)
{
    WIN32_FILE_ATTRIBUTE_DATA a;
    int r;
    memset (p, '\0', sizeof (*p));
    if (just_not_there)
        *just_not_there = 0;

    if (errmsg)
        *errmsg = '\0';
    if (remotefs_error_code_)
        *remotefs_error_code_ = RFSERR_SUCCESS;

    if (!(r = GetFileAttributesExA (fname, GetFileExInfoStandard, &a))) {
        int errval;
        errval = GetLastError ();
/* this condition is from boost */
        if (    errval == ERROR_FILE_NOT_FOUND
             || errval == ERROR_PATH_NOT_FOUND
             || errval == ERROR_INVALID_NAME            // "tools/jam/src/:sys:stat.h", "//foo"
             || errval == ERROR_INVALID_DRIVE           // USB card reader with no card inserted
             || errval == ERROR_NOT_READY               // CD/DVD drive with no disc inserted
             || errval == ERROR_INVALID_PARAMETER       // ":sys:stat.h"
             || errval == ERROR_BAD_PATHNAME            // "//nosuch" on Win64
             || errval == ERROR_BAD_NETPATH) {          // "//nosuch" on Win32
            if (just_not_there)
                *just_not_there = 1;
        }
        if (errmsg) {
            strncpy (errmsg, mswin_error_to_text (errval), REMOTEFS_ERR_MSG_LEN);
            errmsg[REMOTEFS_ERR_MSG_LEN - 1] = '\0';
        }
        if (remotefs_error_code_)
            *remotefs_error_code_ = translate_mswin_lasterror (errval);
        return -1;
    }

    p->wattr.file_attributes = a.dwFileAttributes;

    p->wattr.creation_time = a.ftCreationTime.dwHighDateTime;
    p->wattr.creation_time <<= 32;
    p->wattr.creation_time |= a.ftCreationTime.dwLowDateTime;

    p->wattr.last_accessed_time = a.ftLastAccessTime.dwHighDateTime;
    p->wattr.last_accessed_time <<= 32;
    p->wattr.last_accessed_time |= a.ftLastAccessTime.dwLowDateTime;

    p->wattr.last_write_time = a.ftLastWriteTime.dwHighDateTime;
    p->wattr.last_write_time <<= 32;
    p->wattr.last_write_time |= a.ftLastWriteTime.dwLowDateTime;

    p->wattr.file_size = a.nFileSizeHigh;
    p->wattr.file_size <<= 32;
    p->wattr.file_size |= a.nFileSizeLow;

    if ((r = stat (fname, &p->ustat))) {
        if (errmsg) {
            strncpy (errmsg, strerror (errno), REMOTEFS_ERR_MSG_LEN);
            errmsg[REMOTEFS_ERR_MSG_LEN - 1] = '\0';
        }
        if (remotefs_error_code_)
            *remotefs_error_code_ = translate_unix_errno (errno);
        return r;
    }

    return 0;
}

#define ERROR_EINTR()           (WSAGetLastError () == WSAEINTR)
#define ERROR_EAGAIN()          (WSAGetLastError () == WSAEWOULDBLOCK || WSAGetLastError () == WSAEINPROGRESS)

static void exitmsg (int c)
{
    printf ("exitting %d\n", c);
    exit (c);
}

#define exit(c)     exitmsg(c)

static const char *strerrorsocket (void)
{
    return mswin_error_to_text (WSAGetLastError ());
}

static void perrorsocket (const char *msg)
{
    fprintf (stderr, "%s: %s\n", msg, strerrorsocket ());
}

#else

#define PATH_SEP_STR            "/"

#define translate_path_sep(s)   (s)

static int portable_stat (int link, const char *fname, struct portable_stat *p, int *just_not_there, enum remotefs_error_code *remotefs_error_code_, char *errmsg)
{
    int r;
    if (just_not_there)
        *just_not_there = 0;
    memset (p, '\0', sizeof (*p));

    if (errmsg)
        *errmsg = '\0';
    if (remotefs_error_code_)
        *remotefs_error_code_ = RFSERR_SUCCESS;

    if (link)
        r = lstat (fname, &p->ustat);
    else
        r = stat (fname, &p->ustat);
    if (r) {
        if (errno == ENOENT)
            if (just_not_there)
                *just_not_there = 1;
        if (errmsg) {
            strncpy (errmsg, strerror (errno), REMOTEFS_ERR_MSG_LEN);
            errmsg[REMOTEFS_ERR_MSG_LEN - 1] = '\0';
        }
        if (remotefs_error_code_)
            *remotefs_error_code_ = translate_unix_errno (errno);
    }
    return r;
}

#define ERROR_EINTR()           (errno == EINTR)
#define ERROR_EAGAIN()          (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)

#define wchar_to_char(s)        (s)

static const char *strerrorsocket (void)
{
    return strerror (errno);
}

static void perrorsocket (const char *msg)
{
    perror (msg);
}

#endif

/*

  for reference, the combined Windows errors of all network functions we use in this source are as follows:

        WSAEACCES WSAEADDRINUSE WSAEADDRNOTAVAIL WSAEAFNOSUPPORT WSAEALREADY
        WSAECONNABORTED WSAECONNREFUSED WSAECONNRESET WSAEFAULT WSAEHOSTUNREACH
        WSAEINPROGRESS WSAEINTR WSAEINVAL WSAEINVALIDPROCTABLE WSAEINVALIDPROVIDER
        WSAEISCONN WSAEMFILE WSAEMSGSIZE WSAENETDOWN WSAENETRESET WSAENETUNREACH
        WSAENOBUFS WSAENOPROTOOPT WSAENOTCONN WSAENOTSOCK WSAEOPNOTSUPP
        WSAEPROTONOSUPPORT WSAEPROTOTYPE WSAEPROVIDERFAILEDINIT WSAESHUTDOWN
        WSAESOCKTNOSUPPORT WSAETIMEDOUT WSAEWOULDBLOCK WSANOTINITIALISED

*/


static int translate_unix_errno (int err)
{
    switch (err) {

#if defined(EAGAIN) && defined(EWOULDBLOCK) && EAGAIN == EWOULDBLOCK
    case EAGAIN:
        return RFSERR_EAGAIN;
#else
#ifdef EAGAIN
    case EAGAIN:
        return RFSERR_EAGAIN;
#endif
#ifdef EWOULDBLOCK
    case EWOULDBLOCK:
        return RFSERR_EAGAIN;
#endif
#endif

#if defined(EDEADLK) && defined(EDEADLOCK) && EDEADLK == EDEADLOCK
    case EDEADLK:
        return RFSERR_EDEADLK;
#else
#ifdef EDEADLK
    case EDEADLK:
        return RFSERR_EDEADLK;
#endif
#ifdef EDEADLOCK
    case EDEADLOCK:
        return RFSERR_EDEADLK;
#endif
#endif

#if defined(ENOTSUP) && defined(EOPNOTSUPP) && ENOTSUP == EOPNOTSUPP
    case ENOTSUP:
        return RFSERR_ENOTSUP;
#else
#ifdef ENOTSUP
    case ENOTSUP:
        return RFSERR_ENOTSUP;
#endif
#ifdef EOPNOTSUPP
    case EOPNOTSUPP:
        return RFSERR_ENOTSUP;
#endif
#endif

#if defined(ECONNREFUSED) && defined(EREFUSED) && ECONNREFUSED == EREFUSED
    case ECONNREFUSED:
        return RFSERR_ECONNREFUSED;
#else
#ifdef ECONNREFUSED
    case ECONNREFUSED:
        return RFSERR_ECONNREFUSED;
#endif
#ifdef EREFUSED
    case EREFUSED:
        return RFSERR_ECONNREFUSED;
#endif
#endif

#ifdef E2BIG
    case E2BIG:
        return RFSERR_E2BIG;
#endif
#ifdef EACCES
    case EACCES:
        return RFSERR_EACCES;
#endif
#ifdef EADDRINUSE
    case EADDRINUSE:
        return RFSERR_EADDRINUSE;
#endif
#ifdef EADDRNOTAVAIL
    case EADDRNOTAVAIL:
        return RFSERR_EADDRNOTAVAIL;
#endif
#ifdef EADV
    case EADV:
        return RFSERR_EADV;
#endif
#ifdef EAFNOSUPPORT
    case EAFNOSUPPORT:
        return RFSERR_EAFNOSUPPORT;
#endif
#ifdef EALREADY
    case EALREADY:
        return RFSERR_EALREADY;
#endif
#ifdef EAUTH
    case EAUTH:
        return RFSERR_EAUTH;
#endif
#ifdef EBADE
    case EBADE:
        return RFSERR_EBADE;
#endif
#ifdef EBADF
    case EBADF:
        return RFSERR_EBADF;
#endif
#ifdef EBADFD
    case EBADFD:
        return RFSERR_EBADFD;
#endif
#ifdef EBADMSG
    case EBADMSG:
        return RFSERR_EBADMSG;
#endif
#ifdef EBADR
    case EBADR:
        return RFSERR_EBADR;
#endif
#ifdef EBADRPC
    case EBADRPC:
        return RFSERR_EBADRPC;
#endif
#ifdef EBADRQC
    case EBADRQC:
        return RFSERR_EBADRQC;
#endif
#ifdef EBADSLT
    case EBADSLT:
        return RFSERR_EBADSLT;
#endif
#ifdef EBFONT
    case EBFONT:
        return RFSERR_EBFONT;
#endif
#ifdef EBUSY
    case EBUSY:
        return RFSERR_EBUSY;
#endif
#ifdef ECANCELED
    case ECANCELED:
        return RFSERR_ECANCELED;
#endif
#ifdef ECAPMODE
    case ECAPMODE:
        return RFSERR_ECAPMODE;
#endif
#ifdef ECHILD
    case ECHILD:
        return RFSERR_ECHILD;
#endif
#ifdef ECHRNG
    case ECHRNG:
        return RFSERR_ECHRNG;
#endif
#ifdef ECOMM
    case ECOMM:
        return RFSERR_ECOMM;
#endif
#ifdef ECONNABORTED
    case ECONNABORTED:
        return RFSERR_ECONNABORTED;
#endif
#ifdef ECONNRESET
    case ECONNRESET:
        return RFSERR_ECONNRESET;
#endif
#ifdef EDESTADDRREQ
    case EDESTADDRREQ:
        return RFSERR_EDESTADDRREQ;
#endif
#ifdef EDOM
    case EDOM:
        return RFSERR_EDOM;
#endif
#ifdef EDOOFUS
    case EDOOFUS:
        return RFSERR_EDOOFUS;
#endif
#ifdef EDOTDOT
    case EDOTDOT:
        return RFSERR_EDOTDOT;
#endif
#ifdef EDQUOT
    case EDQUOT:
        return RFSERR_EDQUOT;
#endif
#ifdef EEXIST
    case EEXIST:
        return RFSERR_EEXIST;
#endif
#ifdef EFAULT
    case EFAULT:
        return RFSERR_EFAULT;
#endif
#ifdef EFBIG
    case EFBIG:
        return RFSERR_EFBIG;
#endif
#ifdef EFTYPE
    case EFTYPE:
        return RFSERR_EFTYPE;
#endif
#ifdef EHOSTDOWN
    case EHOSTDOWN:
        return RFSERR_EHOSTDOWN;
#endif
#ifdef EHOSTUNREACH
    case EHOSTUNREACH:
        return RFSERR_EHOSTUNREACH;
#endif
#ifdef EHWPOISON
    case EHWPOISON:
        return RFSERR_EHWPOISON;
#endif
#ifdef EIDRM
    case EIDRM:
        return RFSERR_EIDRM;
#endif
#ifdef EILSEQ
    case EILSEQ:
        return RFSERR_EILSEQ;
#endif
#ifdef EINPROGRESS
    case EINPROGRESS:
        return RFSERR_EINPROGRESS;
#endif
#ifdef EINTR
    case EINTR:
        return RFSERR_EINTR;
#endif
#ifdef EINVAL
    case EINVAL:
        return RFSERR_EINVAL;
#endif
#ifdef EIO
    case EIO:
        return RFSERR_EIO;
#endif
#ifdef EISCONN
    case EISCONN:
        return RFSERR_EISCONN;
#endif
#ifdef EISDIR
    case EISDIR:
        return RFSERR_EISDIR;
#endif
#ifdef EISNAM
    case EISNAM:
        return RFSERR_EISNAM;
#endif
#ifdef EKEYEXPIRED
    case EKEYEXPIRED:
        return RFSERR_EKEYEXPIRED;
#endif
#ifdef EKEYREJECTED
    case EKEYREJECTED:
        return RFSERR_EKEYREJECTED;
#endif
#ifdef EKEYREVOKED
    case EKEYREVOKED:
        return RFSERR_EKEYREVOKED;
#endif
#ifdef EL2HLT
    case EL2HLT:
        return RFSERR_EL2HLT;
#endif
#ifdef EL2NSYNC
    case EL2NSYNC:
        return RFSERR_EL2NSYNC;
#endif
#ifdef EL3HLT
    case EL3HLT:
        return RFSERR_EL3HLT;
#endif
#ifdef ELAST
    case ELAST:
        return RFSERR_ELAST;
#endif
#ifdef ELIBACC
    case ELIBACC:
        return RFSERR_ELIBACC;
#endif
#ifdef ELIBBAD
    case ELIBBAD:
        return RFSERR_ELIBBAD;
#endif
#ifdef ELIBEXEC
    case ELIBEXEC:
        return RFSERR_ELIBEXEC;
#endif
#ifdef ELIBMAX
    case ELIBMAX:
        return RFSERR_ELIBMAX;
#endif
#ifdef ELIBSCN
    case ELIBSCN:
        return RFSERR_ELIBSCN;
#endif
#ifdef ELNRNG
    case ELNRNG:
        return RFSERR_ELNRNG;
#endif
#ifdef ELOOP
    case ELOOP:
        return RFSERR_ELOOP;
#endif
#ifdef EMEDIUMTYPE
    case EMEDIUMTYPE:
        return RFSERR_EMEDIUMTYPE;
#endif
#ifdef EMFILE
    case EMFILE:
        return RFSERR_EMFILE;
#endif
#ifdef EMGSIZE
    case EMGSIZE:
        return RFSERR_EMGSIZE;
#endif
#ifdef EMLINK
    case EMLINK:
        return RFSERR_EMLINK;
#endif
#ifdef EMSGSIZE
    case EMSGSIZE:
        return RFSERR_EMSGSIZE;
#endif
#ifdef EMULTIHOP
    case EMULTIHOP:
        return RFSERR_EMULTIHOP;
#endif
#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
        return RFSERR_ENAMETOOLONG;
#endif
#ifdef ENAVAIL
    case ENAVAIL:
        return RFSERR_ENAVAIL;
#endif
#ifdef ENEEDAUTH
    case ENEEDAUTH:
        return RFSERR_ENEEDAUTH;
#endif
#ifdef ENETDOWN
    case ENETDOWN:
        return RFSERR_ENETDOWN;
#endif
#ifdef ENETRESET
    case ENETRESET:
        return RFSERR_ENETRESET;
#endif
#ifdef ENETUNREACH
    case ENETUNREACH:
        return RFSERR_ENETUNREACH;
#endif
#ifdef ENFILE
    case ENFILE:
        return RFSERR_ENFILE;
#endif
#ifdef ENOANO
    case ENOANO:
        return RFSERR_ENOANO;
#endif
#ifdef ENOATTR
    case ENOATTR:
        return RFSERR_ENOATTR;
#endif
#ifdef ENOBUFS
    case ENOBUFS:
        return RFSERR_ENOBUFS;
#endif
#ifdef ENOCSI
    case ENOCSI:
        return RFSERR_ENOCSI;
#endif
#ifdef ENODATA
    case ENODATA:
        return RFSERR_ENODATA;
#endif
#ifdef ENODEV
    case ENODEV:
        return RFSERR_ENODEV;
#endif
#ifdef ENOENT
    case ENOENT:
        return RFSERR_ENOENT;
#endif
#ifdef ENOEXEC
    case ENOEXEC:
        return RFSERR_ENOEXEC;
#endif
#ifdef ENOKEY
    case ENOKEY:
        return RFSERR_ENOKEY;
#endif
#ifdef ENOLCK
    case ENOLCK:
        return RFSERR_ENOLCK;
#endif
#ifdef ENOLINK
    case ENOLINK:
        return RFSERR_ENOLINK;
#endif
#ifdef ENOMEDIUM
    case ENOMEDIUM:
        return RFSERR_ENOMEDIUM;
#endif
#ifdef ENOMEM
    case ENOMEM:
        return RFSERR_ENOMEM;
#endif
#ifdef ENOMSG
    case ENOMSG:
        return RFSERR_ENOMSG;
#endif
#ifdef ENONET
    case ENONET:
        return RFSERR_ENONET;
#endif
#ifdef ENOPKG
    case ENOPKG:
        return RFSERR_ENOPKG;
#endif
#ifdef ENOPROTOOPT
    case ENOPROTOOPT:
        return RFSERR_ENOPROTOOPT;
#endif
#ifdef ENOSPC
    case ENOSPC:
        return RFSERR_ENOSPC;
#endif
#ifdef ENOSR
    case ENOSR:
        return RFSERR_ENOSR;
#endif
#ifdef ENOSTR
    case ENOSTR:
        return RFSERR_ENOSTR;
#endif
#ifdef ENOSYM
    case ENOSYM:
        return RFSERR_ENOSYM;
#endif
#ifdef ENOSYS
    case ENOSYS:
        return RFSERR_ENOSYS;
#endif
#ifdef ENOTBLK
    case ENOTBLK:
        return RFSERR_ENOTBLK;
#endif
#ifdef ENOTCAPABLE
    case ENOTCAPABLE:
        return RFSERR_ENOTCAPABLE;
#endif
#ifdef ENOTCONN
    case ENOTCONN:
        return RFSERR_ENOTCONN;
#endif
#ifdef ENOTDIR
    case ENOTDIR:
        return RFSERR_ENOTDIR;
#endif
#ifdef ENOTEMPTY
    case ENOTEMPTY:
        return RFSERR_ENOTEMPTY;
#endif
#ifdef ENOTNAM
    case ENOTNAM:
        return RFSERR_ENOTNAM;
#endif
#ifdef ENOTRECOVERABLE
    case ENOTRECOVERABLE:
        return RFSERR_ENOTRECOVERABLE;
#endif
#ifdef ENOTSOCK
    case ENOTSOCK:
        return RFSERR_ENOTSOCK;
#endif
#ifdef ENOTTY
    case ENOTTY:
        return RFSERR_ENOTTY;
#endif
#ifdef ENOTUNIQ
    case ENOTUNIQ:
        return RFSERR_ENOTUNIQ;
#endif
#ifdef ENXIO
    case ENXIO:
        return RFSERR_ENXIO;
#endif
#ifdef EOVERFLOW
    case EOVERFLOW:
        return RFSERR_EOVERFLOW;
#endif
#ifdef EOWNERDEAD
    case EOWNERDEAD:
        return RFSERR_EOWNERDEAD;
#endif
#ifdef EPERM
    case EPERM:
        return RFSERR_EPERM;
#endif
#ifdef EPFNOSUPPORT
    case EPFNOSUPPORT:
        return RFSERR_EPFNOSUPPORT;
#endif
#ifdef EPIPE
    case EPIPE:
        return RFSERR_EPIPE;
#endif
#ifdef EPROCLIM
    case EPROCLIM:
        return RFSERR_EPROCLIM;
#endif
#ifdef EPROCUNAVAIL
    case EPROCUNAVAIL:
        return RFSERR_EPROCUNAVAIL;
#endif
#ifdef EPROGMISMATCH
    case EPROGMISMATCH:
        return RFSERR_EPROGMISMATCH;
#endif
#ifdef EPROGUNAVAIL
    case EPROGUNAVAIL:
        return RFSERR_EPROGUNAVAIL;
#endif
#ifdef EPROTO
    case EPROTO:
        return RFSERR_EPROTO;
#endif
#ifdef EPROTONOSUPPORT
    case EPROTONOSUPPORT:
        return RFSERR_EPROTONOSUPPORT;
#endif
#ifdef EPROTOTYPE
    case EPROTOTYPE:
        return RFSERR_EPROTOTYPE;
#endif
#ifdef ERANGE
    case ERANGE:
        return RFSERR_ERANGE;
#endif
#ifdef EREMCHG
    case EREMCHG:
        return RFSERR_EREMCHG;
#endif
#ifdef EREMOTE
    case EREMOTE:
        return RFSERR_EREMOTE;
#endif
#ifdef EREMOTEIO
    case EREMOTEIO:
        return RFSERR_EREMOTEIO;
#endif
#ifdef ERESTART
    case ERESTART:
        return RFSERR_ERESTART;
#endif
#ifdef ERFKILL
    case ERFKILL:
        return RFSERR_ERFKILL;
#endif
#ifdef EROFS
    case EROFS:
        return RFSERR_EROFS;
#endif
#ifdef ERPCMISMATCH
    case ERPCMISMATCH:
        return RFSERR_ERPCMISMATCH;
#endif
#ifdef ESHUTDOWN
    case ESHUTDOWN:
        return RFSERR_ESHUTDOWN;
#endif
#ifdef ESOCKTNOSUPPORT
    case ESOCKTNOSUPPORT:
        return RFSERR_ESOCKTNOSUPPORT;
#endif
#ifdef ESPIPE
    case ESPIPE:
        return RFSERR_ESPIPE;
#endif
#ifdef ESRCH
    case ESRCH:
        return RFSERR_ESRCH;
#endif
#ifdef ESRMNT
    case ESRMNT:
        return RFSERR_ESRMNT;
#endif
#ifdef ESTALE
    case ESTALE:
        return RFSERR_ESTALE;
#endif
#ifdef ESTART
    case ESTART:
        return RFSERR_ESTART;
#endif
#ifdef ESTRPIPE
    case ESTRPIPE:
        return RFSERR_ESTRPIPE;
#endif
#ifdef ETIME
    case ETIME:
        return RFSERR_ETIME;
#endif
#ifdef ETIMEDOUT
    case ETIMEDOUT:
        return RFSERR_ETIMEDOUT;
#endif
#ifdef ETOOMANYREFS
    case ETOOMANYREFS:
        return RFSERR_ETOOMANYREFS;
#endif
#ifdef ETXTBSY
    case ETXTBSY:
        return RFSERR_ETXTBSY;
#endif
#ifdef EUCLEAN
    case EUCLEAN:
        return RFSERR_EUCLEAN;
#endif
#ifdef EUNATCH
    case EUNATCH:
        return RFSERR_EUNATCH;
#endif
#ifdef EUSERS
    case EUSERS:
        return RFSERR_EUSERS;
#endif
#ifdef EXDEV
    case EXDEV:
        return RFSERR_EXDEV;
#endif
#ifdef EXFULL
    case EXFULL:
        return RFSERR_EXFULL;
#endif
#ifdef STRUNCATE
    case STRUNCATE:
        return RFSERR_STRUNCATE;
#endif
    }
    return RFSERR_UNIX_ERROR_WITHOUT_TRANSLATION;
}



struct remotefs_private {
    SOCKET sock;
    char remote[256];
};


#define SHUTSOCK(sock)  \
        do { \
            if ((sock) != INVALID_SOCKET) { \
                shutdown ((sock), 2); \
                closesocket (sock); \
                (sock) = INVALID_SOCKET; \
            } \
        } while(0)


#define MSG_VERSION                             1
#define FILE_PROTO_MAGIC                        0x726f


#define REMOTEFS_ACTION_NOTIMPLEMENTED          0
#define REMOTEFS_ACTION_READDIR                 1
#define REMOTEFS_ACTION_READFILE                2
#define REMOTEFS_ACTION_WRITEFILE               3
#define REMOTEFS_ACTION_CHECKORDINARYFILEACCESS 4
#define REMOTEFS_ACTION_STAT                    5
#define REMOTEFS_ACTION_CHDIR                   6
#define REMOTEFS_ACTION_REALPATHIZE             7
#define REMOTEFS_ACTION_GETHOMEDIR              8

const char *action_descr[] = {
    "NOTIMPLEMENTED",
    "READDIR",
    "READFILE",
    "WRITEFILE",
    "CHECKORDINARYFILEACCESS",
    "STAT",
    "CHDIR",
    "REALPATHIZE",
    "GETHOMEDIR",
};

const char *error_code_descr[] = {
    "success",                                  /* 0 */
    "unix error without translation",           /* 1 */
    "mswin error without translation",          /* 2 */
    "unimplemented function",                   /* 3 */
    "bad version",                              /* 4 */
    "server closed idle client",                /* 5 */
    "early terminate from write",               /* 6 */
    "other error",                              /* 7 */
    "endoffile",                                /* 8 */
    "pathname too long",                        /* 9 */
    "last internal error",                      /* 10 */
};


#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define MAX(a,b)        ((a) > (b) ? (a) : (b))


struct cooledit_remote_msg_header {
    unsigned char magic[2];
    unsigned char version[2];
    unsigned char action[2];
    unsigned char msglen[6];
};

struct cooledit_remote_msg_ack {
    unsigned char error_code[6];
    unsigned char version[2];
};

#define READER_CHUNK            65536

struct reader_data {
    SOCKET *sock;
    unsigned char buf[READER_CHUNK];
    int avail;
    int written;
};

static void decode_uint48 (const unsigned char *p, unsigned long long *i_)
{
    unsigned long long i;

    i = p[0];
    i <<= 8;
    i |= p[1];
    i <<= 8;
    i |= p[2];
    i <<= 8;
    i |= p[3];
    i <<= 8;
    i |= p[4];
    i <<= 8;
    i |= p[5];

    *i_ = i;
}

static void encode_uint48 (unsigned char *p, unsigned long long i)
{
    p[5] = i & 0xff;
    i >>= 8;
    p[4] = i & 0xff;
    i >>= 8;
    p[3] = i & 0xff;
    i >>= 8;
    p[2] = i & 0xff;
    i >>= 8;
    p[1] = i & 0xff;
    i >>= 8;
    p[0] = i & 0xff;
}

static void decode_uint16 (const unsigned char *p, unsigned long *i_)
{
    unsigned int i;

    i = p[0];
    i <<= 8;
    i |= p[1];

    *i_ = i;
}

static void encode_uint16 (unsigned char *p, unsigned long i)
{
    p[1] = i & 0xff;
    i >>= 8;
    p[0] = i & 0xff;
}

static int encode_sint (unsigned char **p_, long long v)
{
    int r = 0, sign = 0;
    if (v < 0LL) {
        sign = 0x40;
        v = -v;
    }
    if (!p_) {
        for (;;) {
            if (v <= 0x3f) {
                r++;
                break;
            } else {
                r++;
                v >>= 7;
            }
        }
    } else {
        unsigned char *p = *p_;
        for (;;) {
            if (v <= 0x3f) {
                *p++ = (v & 0x3f) | 0x80 | sign;
                break;
            } else {
                *p++ = (v & 0x7f);
                v >>= 7;
            }
        }
        r = (p - *p_);
        *p_ = p;
    }
    return r;
}

static int decode_sint (const unsigned char **p, const unsigned char *end, long long *v_)
{
    int i = 0, sign = 0;
    long long v = 0ULL;
    const unsigned char *q = *p;
    const unsigned char *e;
    while (!(*q & 0x80)) {
        i++;
        if (i > 16)
            return -1;
        q++;
        if (q >= end)
            return -1;
    }
    e = q + 1;
    for (;;) {
        long long n;
        if (e == q + 1) {
            v |= (*q & 0x3f);
            sign = *q & 0x40;
        } else {
            v |= (*q & 0x7f);
        }
        if (q == *p)
            break;
        q--;
        n = v << 7;
        if (n < v)              /* overflow */
            return -1;
        v = n;
    }
    *v_ = sign ? -v : v;
    *p = e;
    return 0;
}

/* decode arbitrary precision signed integer */
static int decode_apsint (const unsigned char **p, const unsigned char *end, int *exponent, long long *v_)
{
    int i = 0, sign = 0;
    long long v = 0ULL;
    const unsigned char *q = *p;
    const unsigned char *e;
    while (!(*q & 0x80)) {
        i++;
        q++;
        if (q >= end)
            return -1;
    }
    e = q + 1;
    for (;;) {
        long long n;
        if (e == q + 1) {
            v |= (*q & 0x3f);
            sign = *q & 0x40;
        } else {
            v |= (*q & 0x7f);
        }
        if (q == *p)
            break;
        q--;
        for (;;) {
            n = v << 7;
            if (n >= v)
                break;
/* deal with overflow by dropping the least significant bits: */
            (*exponent)++;
            v >>= 1;
        }
        v = n;
    }
    *v_ = sign ? -v : v;
    *p = e;
    return 0;
}

union float_conv {
    double f;
    unsigned long long l;
};

#define SWAP(a,b) \
    do { \
        s = (a); \
        (a) = (b); \
        (b) = s; \
    } while (0)

/* The following encoding format for floating point numbers nicely compresses values
   that are small integers, while also handling arbitrary precision. This means that
   round numbers between -65 and +65 will encode to 2 bytes and round numbers
   between -8193 and +8193 will encode to 2 or 3 bytes. PI, as a IEEE 64-bit double,
   encodes to 9 bytes which is only 1 byte more than its native representation.
   Powers of two encode to 2 bytes.

   The encoding is arbitrary-precision, supporting an exponent up to 2^1000000
   (10^300000) and a mantissa as large as you like.

   The NaN format encodes the most common forms in 4 bytes, and encodes general NaNs
   with all status bits. This method ought to encode and decode all possible
   QNaN and SNaN formats in a lossless manner. Future-proofing is achieved by
   allowing additional common NaN forms and decoding unknown NaN forms to generic
   SNaN.

   All Inf and zero values are encoded as 4 or five bytes.

   The encoding and decoding work only with integer bitwise operations.

   All floating point endianesses are catered for, i.e. 01234567, 45670123, 67452301,
   and 76543210.

   See https://en.wikipedia.org/wiki/Double-precision_floating-point_format */

#define REMOTEFS_MIN_EXP        (-1000000)
#define REMOTEFS_NEG_ZERO       (-1048575)
#define REMOTEFS_INF            (1048575)
#define REMOTEFS_NAN_           (1048574)

#define REMOTEFS_SNAN           (REMOTEFS_NAN_ -  0)     /* 7FFz xxxx xxxx xxxx */
#define REMOTEFS_QNAN           (REMOTEFS_NAN_ -  1)     /* 7FFy xxxx xxxx xxxx */
#define __REMOTEFS_SNAN0        invalid                  /* 7FF0 0000 0000 0000 */
#define REMOTEFS_SNAN01         (REMOTEFS_NAN_ -  2)     /* 7FF0 0000 0000 0001 */
#define REMOTEFS_SNAN40         (REMOTEFS_NAN_ -  3)     /* 7FF4 0000 0000 0000 */
#define REMOTEFS_SNAN41         (REMOTEFS_NAN_ -  4)     /* 7FF4 0000 0000 0001 */
#define REMOTEFS_QNAN00         (REMOTEFS_NAN_ -  5)     /* 7FF8 0000 0000 0000 */
#define REMOTEFS_QNAN01         (REMOTEFS_NAN_ -  6)     /* 7FF8 0000 0000 0001 */
#define REMOTEFS_QNAN40         (REMOTEFS_NAN_ -  7)     /* 7FFC 0000 0000 0000 */
#define REMOTEFS_QNAN41         (REMOTEFS_NAN_ -  8)     /* 7FFC 0000 0000 0001 */
#define REMOTEFS_QNAN7_F        (REMOTEFS_NAN_ -  9)     /* 7FFF FFFF FFFF FFFF */

#define REMOTEFS_NAN_LAST       (1047550)
#define REMOTEFS_MAX_EXP        (1000000)

static void fix_endianness (union float_conv *c)
{
    const union float_conv t = { 1002.00006103515625 }; /* 40 8f 50 00 20 00 00 00 */
    assert (sizeof (t) == sizeof (t.f));
    assert (sizeof (t) == sizeof (t.l));
    if (t.l == 0x408f500020000000ULL) {
        /* we are good */
    } else if (t.l == 0x20000000408f5000ULL) {
        c->l = (((c->l >> 32) & 0x00000000FFFFFFFF) |
                ((c->l << 32) & 0xFFFFFFFF00000000));
    } else if (t.l == 0x000020005000408fULL) {
        c->l = (((c->l >> 48) & 0x000000000000FFFF) |
                ((c->l >> 16) & 0x00000000FFFF0000) |
                ((c->l << 16) & 0x0000FFFF00000000) |
                ((c->l << 48) & 0xFFFF000000000000));
    } else if (t.l == 0x0000002000508f40ULL) {
        c->l = (((c->l >> 56) & 0x00000000000000FF) |
                ((c->l >> 40) & 0x000000000000FF00) |
                ((c->l >> 24) & 0x0000000000FF0000) |
                ((c->l >>  8) & 0x00000000FF000000) |
                ((c->l <<  8) & 0x000000FF00000000) |
                ((c->l << 24) & 0x0000FF0000000000) |
                ((c->l << 40) & 0x00FF000000000000) |
                ((c->l << 56) & 0xFF00000000000000));
    } else {
        assert (!"unknown endianness");
    }
}

static int encode_float (unsigned char **p_, double v)
{
    int r;
    long long mantissa, exponent;
    union float_conv d;
    d.f = v;
    fix_endianness (&d);
/* there is REMOTEFS_POS_ZERO since a regular zero is encoded as mantissa=0 exp=0 */
    if (d.l == 0x8000000000000000ULL) {
        exponent = REMOTEFS_NEG_ZERO;
        mantissa = -1;
    } else if (d.l == 0x7FF0000000000000ULL) {
        exponent = REMOTEFS_INF;
        mantissa = 1;
    } else if (d.l == 0xFFF0000000000000ULL) {
        exponent = REMOTEFS_INF;
        mantissa = -1;
    } else if ((d.l & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) {
        long long l;
        l = d.l & 0x7FFFFFFFFFFFFFFFULL;
        mantissa = 1;
#define if_else(a,b)     if (l == b) exponent = a; else
        if_else (REMOTEFS_SNAN01, 0x7FF0000000000001ULL)
        if_else (REMOTEFS_SNAN40, 0x7FF4000000000000ULL)
        if_else (REMOTEFS_SNAN41, 0x7FF4000000000001ULL)
        if_else (REMOTEFS_QNAN00, 0x7FF8000000000000ULL)
        if_else (REMOTEFS_QNAN01, 0x7FF8000000000001ULL)
        if_else (REMOTEFS_QNAN40, 0x7FFC000000000000ULL)
        if_else (REMOTEFS_QNAN41, 0x7FFC000000000001ULL)
        if_else (REMOTEFS_QNAN7_F, 0x7FFFFFFFFFFFFFFFULL)
        if ((l & 0x0008000000000000ULL)) {
            exponent = REMOTEFS_QNAN;
            mantissa = l & 0x0007FFFFFFFFFFFFULL;
        } else {
            exponent = REMOTEFS_SNAN;
            mantissa = l & 0x0007FFFFFFFFFFFFULL;
            if (!mantissa)
                mantissa = 1;
        }
        if ((d.l & 0x8000000000000000ULL))
            mantissa = -mantissa;
#undef if_else
    } else {
        exponent = ((d.l >> 52) & 2047) - (1023 + 52);
        if (exponent == -1075.0) {
            mantissa = (d.l & ((1ULL << 52) - 1));
            if (!mantissa)
                exponent = 0;   /* encode 0.0 as mant=0 exp=0 */
        } else {
            mantissa = (d.l & ((1ULL << 52) - 1)) | (1ULL << 52);
        }
        if (mantissa) {
            unsigned long long mask = 0xFFFFFFFFULL;
            int shift = 32;
            do {
                if (!(mantissa & mask)) {
                    mantissa >>= shift;
                    exponent += shift;
                }
                shift >>= 1;
                mask >>= shift;
            } while (shift);
        }
        if (exponent > 1023) {
            /* send 0x7FF4000000000001ULL */
            exponent = REMOTEFS_SNAN;
            mantissa = 0x1001;
        } else if ((d.l & (1ULL << 63))) {
            mantissa = -mantissa;
        }
    }
    r = encode_sint (p_, exponent);
    r += encode_sint (p_, mantissa);
    return r;
}

static int decode_float (const unsigned char **p, const unsigned char *end, double *v_)
{
    union float_conv d;
    long long mantissa, exponent;
    int adj = 0;
    (void) encode_float;        /* prevent 'unused' warning */
    if (decode_sint (p, end, &exponent))
        return -1;
    if (decode_apsint (p, end, &adj, &mantissa))
        return -1;

/* A naive decoder can ignore these special cases. REMOTEFS_NEG_ZERO is a
   high negative power, so it will give negative zero in any case.
   INF and NAN are high positive powers, and so-also, will yield INF. */
    if (exponent == REMOTEFS_NEG_ZERO) {
        d.l = 0x8000000000000000ULL;
    } else if (exponent == REMOTEFS_INF) {
        if (mantissa >= 0)
            d.l = 0x7FF0000000000000ULL;
        else
            d.l = 0xFFF0000000000000ULL;
    } else if (exponent >= REMOTEFS_NAN_LAST && exponent <= REMOTEFS_NAN_) {
#define if_else(a,b)     if (exponent == a) d.l |= b; else
        d.l = 0;
        if (mantissa < 0) {
            mantissa = -mantissa;
            d.l |= 0x8000000000000000ULL;
        }
        while (adj-- > 0)
            mantissa <<= 1;
        if_else (REMOTEFS_SNAN01, 0x7FF0000000000001ULL)
        if_else (REMOTEFS_SNAN40, 0x7FF4000000000000ULL)
        if_else (REMOTEFS_SNAN41, 0x7FF4000000000001ULL)
        if_else (REMOTEFS_QNAN00, 0x7FF8000000000000ULL)
        if_else (REMOTEFS_QNAN01, 0x7FF8000000000001ULL)
        if_else (REMOTEFS_QNAN40, 0x7FFC000000000000ULL)
        if_else (REMOTEFS_QNAN41, 0x7FFC000000000001ULL)
        if_else (REMOTEFS_QNAN7_F, 0x7FFFFFFFFFFFFFFFULL)
        if (exponent == REMOTEFS_QNAN) {
            d.l |= 0x7FF8000000000000ULL | (mantissa & 0x0007FFFFFFFFFFFFULL);
        } else {        /* SNaN or unknown other future NaN form */
            d.l |= 0x7FF0000000000000ULL | (mantissa & 0x0007FFFFFFFFFFFFULL);
        }
        if (!(d.l & 0x000FFFFFFFFFFFFFULL))
            d.l |= 0x0004000000000001ULL;       /* distinguish from Inf */
#undef if_else
    } else {
        exponent += adj;
        d.l = 0ULL;
        if (mantissa < 0LL) {
            mantissa = -mantissa;
            d.l |= (1ULL << 63);
        }
        while ((mantissa & 0xffe0000000000000ULL)) {
            mantissa >>= 1;
            exponent++;
        }
        if (mantissa) {
            unsigned long long mask = 0x1fffffffe00000ULL;
            int shift = 32;
            do {
                if (!(mantissa & mask) && exponent - shift >= -1075) {
                    mantissa <<= shift;
                    exponent -= shift;
                }
                shift >>= 1;
                mask = (mask << shift) & 0x1fffffffe00000ULL;
            } while (shift);
        } else {
            exponent = -1075;   /* we encode 0.0 as mant=0 exp=0, but IEEE encodes it as 0 x 2^(-1075) */
        }
        if (exponent > REMOTEFS_MAX_EXP) {
            if (mantissa >= 0)
                d.l = 0x7FF0000000000000ULL;    /* +Inf */
            else
                d.l = 0xFFF0000000000000ULL;    /* -Inf */
        } else if (exponent < REMOTEFS_MIN_EXP) {
            if (mantissa >= 0)
                d.l = 0x0000000000000000ULL;    /* +0 */
            else
                d.l = 0x8000000000000000ULL;    /* -0 */
        } else if (exponent < -1075 || exponent > 971) {
            return -1;
        } else {
            d.l |= mantissa & (((1ULL << 52) - 1));
            d.l |= ((exponent + (1023 + 52))) << 52;
        }
    }
    fix_endianness (&d);
    *v_ = d.f;
    return 0;
}

static int encode_uint (unsigned char **p_, unsigned long long v)
{
    int r = 0;
    if (!p_) {
        for (;;) {
            if (v <= 0x7f) {
                r++;
                break;
            } else {
                r++;
                v >>= 7;
            }
        }
    } else {
        unsigned char *p = *p_;
        for (;;) {
            if (v <= 0x7f) {
                *p++ = (v & 0x7f) | 0x80;
                break;
            } else {
                *p++ = (v & 0x7f);
                v >>= 7;
            }
        }
        r = (p - *p_);
        *p_ = p;
    }
    return r;
}

static int decode_uint (const unsigned char **p, const unsigned char *end, unsigned long long *v_)
{
    int i = 0;
    unsigned long long v = 0ULL;
    const unsigned char *q = *p;
    const unsigned char *e;
    while (!(*q & 0x80)) {
        i++;
        if (i > 16)
            return -1;
        q++;
        if (q >= end)
            return -1;
    }
    e = q + 1;
    for (;;) {
        unsigned long long n;
        v |= (*q & 0x7f);
        if (q == *p)
            break;
        q--;
        n = v << 7;
        if (n < v)      /* overflow */
            return -1;
        v = n;
    }
    *v_ = v;
    *p = e;
    return 0;
}

static int encode_str (unsigned char **p_, const char *str, int len)
{
    int r;
    r = encode_uint (p_, len);
    r += len;
    if (p_) {
        memcpy (*p_, str, len);
        (*p_) += len;
    }
    return r;
}

static int decode_cstr (const unsigned char **p, const unsigned char *end, CStr * str)
{
    unsigned long long v;
    v = 0ULL;
    if (decode_uint (p, end, &v))
        return -1;
    if ((*p) + v > end)
        return -1;
    str->len = v;
    str->data = (char *) malloc (v + 1);
    if (v > 0)
        memcpy (str->data, *p, v);
    str->data[v] = '\0';
    (*p) += v;
    return 0;
}

static int decode_str (const unsigned char **p, const unsigned char *end, char *str, int buflen)
{
    unsigned long long v;
    assert (buflen >= 1);
    buflen--;
    v = 0ULL;
    if (decode_uint (p, end, &v))
        return -1;
    if ((*p) + v > end)
        return -1;
    if (v < (unsigned int) buflen) {
        if (v > 0)
            memcpy (str, *p, v);
        str[v] = '\0';
    } else {
        if (buflen > 0)
            memcpy (str, *p, buflen);
        str[buflen] = '\0';
    }
    (*p) += v;
    return 0;
}

static void decode_msg_header (const struct cooledit_remote_msg_header *m, unsigned long long *msglen, unsigned long *version, unsigned long *action, unsigned long *magic)
{
    decode_uint48 (m->msglen, msglen);
    decode_uint16 (m->version, version);
    decode_uint16 (m->action, action);
    decode_uint16 (m->magic, magic);
}

static void encode_msg_header (struct cooledit_remote_msg_header *m, unsigned long long msglen, unsigned long version, unsigned long action, unsigned long magic)
{
    encode_uint48 (m->msglen, msglen);
    encode_uint16 (m->version, version);
    encode_uint16 (m->action, action);
    encode_uint16 (m->magic, magic);
}

static void decode_msg_ack (const struct cooledit_remote_msg_ack *m, remotefs_error_code_t *error_code, unsigned long *version)
{
    decode_uint48 (m->error_code, error_code);
    decode_uint16 (m->version, version);
}

static void encode_msg_ack (struct cooledit_remote_msg_ack *m, remotefs_error_code_t error_code, unsigned long version)
{
    encode_uint48 (m->error_code, error_code);
    encode_uint16 (m->version, version);
}


/* The following extensible struct-packing mechanism allows new fields
   to be added in future versions. For instance ACLs.
   
   For instance, struct stat is encoded as follows:

    struct enclose {
        IN os_type;
        struct stat {
            INT st_dev;
            INT st_ino;
            INT st_mode;
            ...
        }
        struct future1 {
            ...
        }
        struct future2 {
            ...
        }
    }

*/

#define FIELD_TYPE_END                  0

#define FIELD_TYPE_UINT                 1
#define FIELD_TYPE_SINT                 2
#define FIELD_TYPE_FLOAT                3
#define FIELD_TYPE_STRING               4
#define FIELD_TYPE_TYPEDCHUNK           5
#define FIELD_TYPE_STRUCT               6

#define _FIELD_TYPE_VECTOR              6

#define FIELD_TYPE_VECTORUINT           (FIELD_TYPE_UINT       + _FIELD_TYPE_VECTOR)
#define FIELD_TYPE_VECTORSINT           (FIELD_TYPE_SINT       + _FIELD_TYPE_VECTOR)
#define FIELD_TYPE_VECTORFLOAT          (FIELD_TYPE_FLOAT      + _FIELD_TYPE_VECTOR)
#define FIELD_TYPE_VECTORSTRING         (FIELD_TYPE_STRING     + _FIELD_TYPE_VECTOR)
#define FIELD_TYPE_VECTORTYPEDCHUNK     (FIELD_TYPE_TYPEDCHUNK + _FIELD_TYPE_VECTOR)
#define FIELD_TYPE_VECTORSTRUCT         (FIELD_TYPE_STRUCT     + _FIELD_TYPE_VECTOR)

#define FIELD_TYPE_EXTENDED__FIRST_TYPE 16

#define MAX_PATH_DEPTH                  100

#define SET_FIELD_TYPE(f, i, t) \
    do { \
        (f)[(i) >> 1] &= ~(0xF << (4 * ((i) & 1))); \
        (f)[(i) >> 1] |=  ((t) << (4 * ((i) & 1))); \
    } while (0)

#define GET_FIELD_TYPE(f, i)            (((f)[(i) >> 1] >> (4 * ((i) & 1))) & 0xF)

struct storage_hook {
    int (*store_vector) (void *user_data, const unsigned short *path, const int depth, const unsigned long long the_type, long n);
    int (*store_uint) (void *user_data, const unsigned short *path, const int depth, unsigned long long v);
    int (*store_sint) (void *user_data, const unsigned short *path, const int depth, long long v);
    int (*store_float) (void *user_data, const unsigned short *path, const int depth, double f);
    int (*store_str) (void *user_data, const unsigned short *path, const int depth, char **storage, int *storage_len);
    int (*store_chunkedtype) (void *user_data, const unsigned short *path, const int depth, const unsigned long long the_type, const unsigned char *chunk, const int chunk_len);
    void *user_data;
    unsigned short path[MAX_PATH_DEPTH];
};

static int decode_struct (const unsigned char **p_, const unsigned char *end, struct storage_hook *hook, const int depth);

static int decode_thing (const unsigned char **p_, const unsigned char *end, const int the_type, struct storage_hook *hook, const int depth)
{
    unsigned long long v, n;
    long long sv;
    unsigned long long extended_type;
    CStr typed_chunk;
    double f;
    unsigned int i;
    char dummy;
    char *storage;
    int storage_len;
    switch (the_type) {
    case FIELD_TYPE_UINT:
        if (decode_uint (p_, end, &v))
            return -1;
        if (hook->store_uint)
            if ((*hook->store_uint) (hook->user_data, hook->path, depth, v))
                return -1;
        break;
    case FIELD_TYPE_SINT:
        if (decode_sint (p_, end, &sv))
            return -1;
        if (hook->store_sint)
            if ((*hook->store_sint) (hook->user_data, hook->path, depth, sv))
                return -1;
        break;
    case FIELD_TYPE_FLOAT:
        if (decode_float (p_, end, &f))
            return -1;
        if (hook->store_float)
            if ((*hook->store_float) (hook->user_data, hook->path, depth, f))
                return -1;
        break;
    case FIELD_TYPE_STRING:
        storage = &dummy;
        storage_len = sizeof (dummy);
        if (hook->store_str)
            if ((*hook->store_str) (hook->user_data, hook->path, depth, &storage, &storage_len))
                return -1;
        if (decode_str (p_, end, storage, storage_len))
            return -1;
        break;
    case FIELD_TYPE_TYPEDCHUNK:
        extended_type = 0ULL;
        if (decode_uint (p_, end, &extended_type))
            return -1;
        if (hook->store_chunkedtype) {
            int r;
            memset (&typed_chunk, '\0', sizeof (typed_chunk));
            if (decode_cstr (p_, end, &typed_chunk))
                return -1;
            r = (*hook->store_chunkedtype) (hook->user_data, hook->path, depth, extended_type, (const unsigned char *) typed_chunk.data, typed_chunk.len);
            free (typed_chunk.data);
            if (r)
                return -1;
        } else {
            storage = &dummy;
            storage_len = sizeof (dummy);
            if (decode_str (p_, end, storage, storage_len))
                return -1;
        }
        break;
    case FIELD_TYPE_STRUCT:
        if (decode_struct (p_, end, hook, depth))
            return -1;
        break;
    case FIELD_TYPE_VECTORUINT:
    case FIELD_TYPE_VECTORSINT:
    case FIELD_TYPE_VECTORFLOAT:
    case FIELD_TYPE_VECTORSTRING:
    case FIELD_TYPE_VECTORSTRUCT:
        if (decode_uint (p_, end, &n))
            return -1;
        if (hook->store_vector)
            if ((*hook->store_vector) (hook->user_data, hook->path, depth, (unsigned long long) (the_type - _FIELD_TYPE_VECTOR), n))
                return -1;
        for (i = 0; i < n; i++)
            if (decode_thing (p_, end, the_type - _FIELD_TYPE_VECTOR, hook, depth))
                return -1;
        break;
    case FIELD_TYPE_VECTORTYPEDCHUNK:
/* for vectors we only want to send the extended type once: */
        extended_type = 0ULL;
        if (decode_uint (p_, end, &extended_type))
            return -1;
        if (decode_uint (p_, end, &n))
            return -1;
        if (hook->store_vector)
            if ((*hook->store_vector) (hook->user_data, hook->path, depth, extended_type, n))
                return -1;
        for (i = 0; i < n; i++) {
            if (hook->store_chunkedtype) {
                int r;
                memset (&typed_chunk, '\0', sizeof (typed_chunk));
                if (decode_cstr (p_, end, &typed_chunk))
                    return -1;
                r = (*hook->store_chunkedtype) (hook->user_data, hook->path, depth, extended_type, (const unsigned char *) typed_chunk.data, typed_chunk.len);
                free (typed_chunk.data);
                if (r)
                    return -1;
            } else {
                storage = &dummy;
                storage_len = sizeof (dummy);
                if (decode_str (p_, end, storage, storage_len))
                    return -1;
            }
        }
        break;
    default:
        return -1;
    }
    return 0;
}


static int decode_struct (const unsigned char **p_, const unsigned char *end, struct storage_hook *hook, const int depth)
{
    int field;
    CStr f;
    if (depth >= MAX_PATH_DEPTH)
        return -1;
    if (decode_cstr (p_, end, &f))
        return -1;
    for (field = 0; field < f.len * 2; field++) {
        int the_type;
        hook->path[depth] = field;
        the_type = GET_FIELD_TYPE (f.data, field);
        if (!the_type) {
            free (f.data);
            return 0;
        }
        if (decode_thing (p_, end, the_type, hook, depth + 1)) {
            free (f.data);
            return -1;
        }
    }
    free (f.data);
    return -1;
}

#define N_STAT_FIELDS           14
#define N_WINDOWS_FIELDS        5

static int encode_stat (unsigned char **p_, const struct portable_stat *ps)
{
    int r, i;
    unsigned char enclose[3];
    const struct stat *s;
#ifdef MSWIN
    const struct windows_file_attributes *w;
#endif

    s = &ps->ustat;
#ifdef MSWIN
    w = &ps->wattr;
#endif

    memset (enclose, '\0', sizeof (enclose));
    SET_FIELD_TYPE (enclose, 0, FIELD_TYPE_UINT);
    SET_FIELD_TYPE (enclose, 1, FIELD_TYPE_UINT);
    SET_FIELD_TYPE (enclose, 2, FIELD_TYPE_STRUCT);
#ifdef MSWIN
    SET_FIELD_TYPE (enclose, 3, FIELD_TYPE_STRUCT);
    SET_FIELD_TYPE (enclose, 4, FIELD_TYPE_END);
#else
    SET_FIELD_TYPE (enclose, 3, FIELD_TYPE_END);
#endif
    r = encode_str (p_, (const char *) enclose, sizeof (enclose));
#ifdef MSWIN
    r += encode_uint (p_, OS_TYPE_WINDOWS);
    r += encode_uint (p_, OS_SUBTYPE_WINDOWS);
#else
    r += encode_uint (p_, OS_TYPE_POSIX);
    r += encode_uint (p_, OS_SUBTYPE_LINUX);
#endif
    {
        unsigned char fields[(N_STAT_FIELDS + 1 + 1) / 2];
        memset (fields, '\0', sizeof (fields));
        for (i = 0; i < N_STAT_FIELDS; i++)
            SET_FIELD_TYPE (fields, i, FIELD_TYPE_UINT);
        SET_FIELD_TYPE (fields, N_STAT_FIELDS, FIELD_TYPE_END);
        r += encode_str (p_, (const char *) fields, sizeof (fields));
        r += encode_uint (p_, (unsigned long long) s->st_dev);          /* 0 */
        r += encode_uint (p_, (unsigned long long) s->st_ino);          /* 1 */
        r += encode_uint (p_, (unsigned long long) s->st_mode);         /* 2 */
        r += encode_uint (p_, (unsigned long long) s->st_nlink);        /* 3 */
        r += encode_uint (p_, (unsigned long long) s->st_uid);          /* 4 */
        r += encode_uint (p_, (unsigned long long) s->st_gid);          /* 5 */
#ifdef MSWIN
        r += encode_uint (p_, (unsigned long long) 0);                  /* 6 */
        r += encode_uint (p_, (unsigned long long) 0);                  /* 7 */
#else
        r += encode_uint (p_, (unsigned long long) major (s->st_rdev)); /* 6 */
        r += encode_uint (p_, (unsigned long long) minor (s->st_rdev)); /* 7 */
#endif
#ifdef MSWIN
        r += encode_uint (p_, ((s->st_mode & S_IFMT) == S_IFCHR) ? 0ULL : (unsigned long long) s->st_size);         /* 8 */
#else
        r += encode_uint (p_, (unsigned long long) s->st_size);         /* 8 */
#endif
#ifdef MSWIN
        r += encode_uint (p_, (unsigned long long) 0);                  /* 9 */
        r += encode_uint (p_, (unsigned long long) 0);                  /* 10 */
#else
        r += encode_uint (p_, (unsigned long long) s->st_blksize);      /* 9 */
        r += encode_uint (p_, (unsigned long long) s->st_blocks);       /* 10 */
#endif
        r += encode_uint (p_, (unsigned long long) s->st_atime);        /* 11 */
        r += encode_uint (p_, (unsigned long long) s->st_mtime);        /* 12 */
        r += encode_uint (p_, (unsigned long long) s->st_ctime);        /* 13 */
    }
#ifdef MSWIN
    {
        unsigned char fields[(N_WINDOWS_FIELDS + 1 + 1) / 2];
        memset (fields, '\0', sizeof (fields));
        for (i = 0; i < N_WINDOWS_FIELDS; i++)
            SET_FIELD_TYPE (fields, i, FIELD_TYPE_UINT);
        SET_FIELD_TYPE (fields, N_WINDOWS_FIELDS, FIELD_TYPE_END);
        r += encode_str (p_, (const char *) fields, sizeof (fields));
        r += encode_uint (p_, w->file_attributes);              /* 0 */
        r += encode_uint (p_, w->creation_time);                /* 1 */
        r += encode_uint (p_, w->last_accessed_time);           /* 2 */
        r += encode_uint (p_, w->last_write_time);              /* 3 */
        r += encode_uint (p_, w->file_size);                    /* 4 */
    }
#endif
    return r;
}

struct decode_stat_data {
#define DECODE_STAT_MAGIC       0xdc2a683a
    unsigned int magic;
    unsigned long rdev_major;
    unsigned long rdev_minor;
    struct portable_stat *s;
};

#define FLD(t,a)        case t: a = v; break

int stat_store_int (void *user_data_, const unsigned short *path, const int depth, unsigned long long v)
{
    struct decode_stat_data *user_data;
    struct portable_stat *s;

    user_data = (struct decode_stat_data *) user_data_;

    assert (user_data->magic == DECODE_STAT_MAGIC);

    s = user_data->s;

    if (depth == 1 && path[0] == 0)
        s->os = v;

    if (depth == 1 && path[0] == 1)
        s->os_sub = v;

    if (depth == 2 && path[0] == 2) {
        switch (path[1]) {
            FLD (0, s->ustat.st_dev);
            FLD (1, s->ustat.st_ino);
            FLD (2, s->ustat.st_mode);
            FLD (3, s->ustat.st_nlink);
            FLD (4, s->ustat.st_uid);
            FLD (5, s->ustat.st_gid);
            FLD (6, user_data->rdev_major);
            FLD (7, user_data->rdev_minor);
            FLD (8, s->ustat.st_size);
#ifdef MSWIN
            /* windows minimal stat */
#else
            FLD (9, s->ustat.st_blksize);
            FLD (10, s->ustat.st_blocks);
#endif
            FLD (11, s->ustat.st_atime);
            FLD (12, s->ustat.st_mtime);
            FLD (13, s->ustat.st_ctime);
        }
    }

    if (depth == 2 && path[0] == 3) {
        switch (path[1]) {
            FLD (0, s->wattr.file_attributes);
            FLD (1, s->wattr.creation_time);
            FLD (2, s->wattr.last_accessed_time);
            FLD (3, s->wattr.last_write_time);
            FLD (4, s->wattr.file_size);
        }
    }

    return 0;
}

static int decode_stat (const unsigned char **p, const unsigned char *end, struct portable_stat *s)
{
    struct storage_hook hook;
    struct decode_stat_data user_data;

    memset (s, '\0', sizeof (*s));
    memset (&hook, '\0', sizeof (hook));
    memset (&user_data, '\0', sizeof (user_data));

    user_data.magic = DECODE_STAT_MAGIC;
    user_data.s = s;

    hook.user_data = &user_data;
    hook.store_uint = stat_store_int;

    if (decode_struct (p, end, &hook, 0))
        return -1;

    s->ustat.st_rdev = makedev (user_data.rdev_major, user_data.rdev_minor);

    return 0;
}

#define FORCE_SHUTDOWN          1

static int encode_error (unsigned char **p_, remotefs_error_code_t error_code, const char *msg, const int force_shutdown)
{
    int l, r;
    r = encode_uint (p_, REMOTEFS_ERROR);
    r += encode_uint (p_, error_code);
    l = strlen (msg);
    if (l > REMOTEFS_ERR_MSG_LEN - 1)
        l = REMOTEFS_ERR_MSG_LEN - 1;
    r += encode_str (p_, msg, l);
    r += encode_uint (p_, force_shutdown);
    return r;
}

static int decode_error (const unsigned char **p, const unsigned char *end, remotefs_error_code_t *error_code, char *msg, int *force_shutdown)
{
    unsigned long long v;
    *msg = '\0';
    v = 0ULL;
    if (decode_uint (p, end, &v))
        return -1;
    if (v != REMOTEFS_ERROR)
        return -1;
    if (decode_uint (p, end, &v))
        return -1;
    if (error_code)
        *error_code = v;
    if (decode_str (p, end, msg, REMOTEFS_ERR_MSG_LEN))
        return -1;
    if (decode_uint (p, end, &v))
        return -1;
    if (force_shutdown)
        *force_shutdown = v;
    return 0;
}

static char *dname (struct dirent *directentry)
{
    int l;
    static char t[MAX_PATH_LEN];
    l = NAMLEN (directentry);
    if (l >= MAX_PATH_LEN)
        l = MAX_PATH_LEN - 1;
    memcpy (t, directentry->d_name, l);
    t[l] = 0;
    return t;
}

struct file_entry_item {
    struct file_entry_item *next;
    struct file_entry data;
};

static int encode_filelist (unsigned char **p_, struct file_entry_item *list)
{
    struct file_entry_item *i;
    int r, n = 0;
    for (i = list; i; i = i->next)
        n++;
    r = encode_uint (p_, REMOTEFS_SUCCESS);
    r += encode_uint (p_, n);
    for (i = list; i; i = i->next) {
        r += encode_str (p_, i->data.name, strlen (i->data.name));
        r += encode_stat (p_, &i->data.pstat);
    }
    return r;
}

static int decode_filelist (const unsigned char **p, const unsigned char *end, struct file_entry **r, int *n)
{
    unsigned long long v;
    unsigned int i;
    *r = NULL;
    if (decode_uint (p, end, &v))
        return -1;
    *n = v;
    *r = (struct file_entry *) malloc ((v + 1) * sizeof (struct file_entry));
    memset (*r, '\0', (v + 1) * sizeof (struct file_entry));
    for (i = 0; i < v; i++) {
        if (decode_str (p, end, (*r)[i].name, sizeof ((*r)[i].name)))
            goto errout;
        if (decode_stat (p, end, &(*r)[i].pstat))
            goto errout;
    }
    (*r)[v].options = FILELIST_LAST_ENTRY;
    return 0;

  errout:
    if (*r)
        free (*r);
    *r = NULL;
    return -1;
}

static void alloc_encode_error (CStr * r, remotefs_error_code_t error_code, const char *errstr, const int force_shutdown)
{
    unsigned char *p;
    r->len = encode_error (NULL, error_code, errstr, force_shutdown);
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_error (&p, error_code, errstr, force_shutdown);
}

static void alloc_encode_errno_strerror (CStr * r, const int force_shutdown)
{
    alloc_encode_error (r, translate_unix_errno (errno), strerror (errno), force_shutdown);
}

static void alloc_encode_success (CStr * r)
{
    unsigned char *p;
    r->len = encode_uint (NULL, REMOTEFS_SUCCESS);
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_uint (&p, REMOTEFS_SUCCESS);
}

static void set_sockerrmsg_to_errno (char *errmsg, int the_errno)
{
    if (!the_errno) {
        strcpy (errmsg, "Remote closed connection");
    } else {
        strncpy (errmsg, strerror (the_errno), REMOTEFS_ERR_MSG_LEN - 1);
        errmsg[REMOTEFS_ERR_MSG_LEN - 1] = '\0';
    }
}

static int writer (SOCKET sock, const void *p_, long l)
{
    const unsigned char *p;
    p = (const unsigned char *) p_;
    while (l > 0) {
        int c;
        c = send (sock, (void *) p, MIN (l, READER_CHUNK), 0);
        if (!c) {
            errno = 0;
            return -1;
        } else if (c == SOCKET_ERROR && ERROR_EINTR()) {
            c = 0;
        } else if (c == SOCKET_ERROR && ERROR_EAGAIN()) {
            struct timeval tv;
            int sr;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            fd_set wr;
            FD_ZERO (&wr);
            FD_SET (sock, &wr);
            sr = select (sock + 1, NULL, &wr, NULL, &tv);
            if (sr == SOCKET_ERROR && !(ERROR_EINTR() || ERROR_EAGAIN()))
                return -1;
            c = 0;
        } else if (c == SOCKET_ERROR) {
            return -1;
        }
        p += c;
        l -= c;
    }
    return 0;
}

static int wait_for_read (SOCKET sock, long msecond)
{
    struct timeval tv;
    int sr;
    tv.tv_sec = msecond / 1000;
    tv.tv_usec = (msecond % 1000) * 1000;
    fd_set rd;
    FD_ZERO (&rd);
    FD_SET (sock, &rd);
    sr = select (sock + 1, &rd, NULL, NULL, msecond > 0 ? &tv : NULL);
    if (sr == SOCKET_ERROR && !(ERROR_EINTR() || ERROR_EAGAIN())) {
        perrorsocket ("select");
        exit (0);
    }
    return (sr > 0);
}

#define TV_DELTA(t1, t2) ((long) (((long long) t2.tv_sec * 1000ULL + (long long) t2.tv_usec / 1000ULL) - ((long long) t1.tv_sec * 1000ULL + (long long) t1.tv_usec / 1000ULL)))

static int reader_timeout (struct reader_data *d, void *buf_, int buflen, long milliseconds, int *timeout)
{
    struct timeval t1, t2;
    unsigned char *buf;

    gettimeofday (&t1, NULL);

    buf = (unsigned char *) buf_;
    while (buflen) {
        if (d->written < d->avail) {
            int n;
            n = MIN (d->avail - d->written, buflen);
            memcpy (buf, d->buf + d->written, n);
            d->written += n;
            if (d->written == d->avail)
                d->written = d->avail = 0;
            buflen -= n;
            buf += n;
        } else {
            int c;
            c = recv (*d->sock, (void *) (d->buf + d->avail), READER_CHUNK - d->avail, 0);
            if (!c) {
                errno = 0;
                return -1;
            } else if (c == SOCKET_ERROR && ERROR_EINTR()) {
                c = 0;
            } else if (c == SOCKET_ERROR && ERROR_EAGAIN()) {
                long elapsed;
                wait_for_read (*d->sock, milliseconds);
                gettimeofday (&t2, NULL);
                elapsed = TV_DELTA (t1, t2);
                if (milliseconds > 0 && elapsed > milliseconds) {
                    *timeout = 1;
                    return -1;
                }
                c = 0;
            } else if (c == SOCKET_ERROR) {
                return -1;
            }
            d->avail += c;
        }
    }
    return 0;
}

static int reader (struct reader_data *d, void *buf, int buflen)
{
    return reader_timeout (d, buf, buflen, 0, NULL);
}


static void remotefs_listdir_ (const char *directory, unsigned long options, char *filter, CStr *r)
{
    struct file_entry_item *first = NULL, *i, *next;
    struct dirent *directentry;
    struct portable_stat stats;
    DIR *dir;
    char path_fname[MAX_PATH_LEN * 2];
    unsigned char *p;
    int got_dot_dot = 0;

    if (!filter || !*filter)
        filter = "*";

    if ((dir = opendir (translate_path_sep (directory))) == NULL) {
        alloc_encode_errno_strerror (r, 0);
        return;
    }

    if (directory[0] == '/' && directory[1] == '\0') {
        got_dot_dot = 1;
#ifdef MSWIN
        if (!(options & FILELIST_FILES_ONLY)) {
            long long drives;
            int j;
            drives = GetLogicalDrives ();
            for (j = 'A'; j <= 'Z' && drives; j++, drives >>= 1) {
                if ((drives & 1)) {
                    i = (struct file_entry_item *) malloc (sizeof (*i));
                    memset (i, '\0', sizeof (*i));
                    i->data.pstat.ustat.st_mode = S_IFDIR | 00777;
                    i->data.name[0] = j;
                    i->data.name[1] = ':';
                    i->data.name[2] = '\0';
                    i->next = first;
                    first = i;
                }
            }
        }
#endif
    }

    while ((directentry = readdir (dir))) {
        char *dn;
        const char *q;
        dn = dname (directentry);
        if (directory[0] == '/' && directory[1] == '\0') {
            strcpy (path_fname, "/");
            strcat (path_fname, dn);
        } else {
            strcpy (path_fname, directory);
            strcat (path_fname, "/");
            strcat (path_fname, dn);
        }
        q = translate_path_sep (path_fname);
        if (strcmp (dn, ".") && !portable_stat (0, q, &stats, NULL, NULL, NULL)) {
            if (!strcmp (dn, ".."))
                got_dot_dot = 1;
            if ((S_ISDIR (stats.ustat.st_mode) && (options & FILELIST_DIRECTORIES_ONLY)) ||
                (!S_ISDIR (stats.ustat.st_mode) && (options & FILELIST_FILES_ONLY))) {
                if (regexp_match (filter, dn, match_file) == 1) {
                    i = (struct file_entry_item *) malloc (sizeof (*i));
                    memset (i, '\0', sizeof (*i));
                    portable_stat (1, q, &i->data.pstat, NULL, NULL, NULL);
                    strcpy (i->data.name, dn);
                    i->next = first;
                    first = i;
                }
            }
        }
    }

    closedir (dir);

    if (!got_dot_dot && !(options & FILELIST_FILES_ONLY)) {
        char *e;
        strcpy (path_fname, directory);
        e = strrchr (path_fname, '/');
        if (e)
            *e = '/';
        i = (struct file_entry_item *) malloc (sizeof (*i));
        memset (i, '\0', sizeof (*i));
        if (portable_stat (1, translate_path_sep (path_fname), &i->data.pstat, NULL, NULL, NULL))
            i->data.pstat.ustat.st_mode = S_IFDIR | 00777;
        strcpy (i->data.name, "..");
        i->next = first;
        first = i;
    }

    r->len = encode_filelist (NULL, first);
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_filelist (&p, first);

    for (i = first; i; i = next) {
        next = i->next;
        free (i);
    }
}

static void remotefs_readfile_ (int (*chunk_cb) (void *, const unsigned char *, int, long, char *), int (*start_cb) (void *, long, char *), void *hook, const char *filename, CStr * r)
{
    unsigned char chunk[READER_CHUNK];
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    HANDLE fd = INVALID_HANDLE_VALUE;
    struct stat st;
    unsigned long long progress = 0ULL;

    memset (&st, '\0', sizeof (st));

    fd = open (translate_path_sep (filename), O_RDONLY | _O_BINARY);
    if (fd == INVALID_HANDLE_VALUE || fstat (fd, &st)) {
        alloc_encode_errno_strerror (r, 0);
        return;
    }

    if (start_cb && (*start_cb) (hook, st.st_size, errmsg)) {
        alloc_encode_error (r, RFSERR_OTHER_ERROR, errmsg, 0);
        goto errout;
    }

    for (;;) {
        int c;
        if (progress >= (unsigned long long) st.st_size)
            break;
        c = read (fd, chunk, READER_CHUNK);
        if (!c) {
            alloc_encode_error (r, RFSERR_ENDOFFILE, "System call read() returned zero", 0);
            goto errout;
        }
        if (c < 0) {
            alloc_encode_errno_strerror (r, 0);
            goto errout;
        }

        if ((*chunk_cb) (hook, chunk, c, st.st_size, errmsg)) {
            alloc_encode_error (r, RFSERR_OTHER_ERROR, errmsg, 0);
            goto errout;
        }
        progress += c;
    }

    close (fd);

    alloc_encode_success (r);
    return;

  errout:
    if (fd != INVALID_HANDLE_VALUE)
        close (fd);
    return;
}

#define HASH(c)         (((c + 9UL) * (c + 2UL) * 401UL) >> 1)
static unsigned long long random_seed = 0L;

static void scramble_random (void)
{
    unsigned long long c;

    random_seed ^= (unsigned long long) getpid ();
    c = random_seed % 214480403ULL;
    random_seed ^= HASH (c);

    random_seed ^= (unsigned long long) time (NULL);
    c = random_seed % 214480403ULL;
    random_seed ^= HASH (c);

    random_seed ^= (unsigned long long) clock ();
    c = random_seed % 214480403ULL;
    random_seed ^= HASH (c);
}

static void init_random (void)
{
    unsigned long long t;
    FILE *p;
#ifdef MSWIN
#define SHELLRND	"DIR C:\\Windows\\System32"
#else
#define SHELLRND	"ls -al /etc/ /tmp/ 2>/dev/null"
#endif
    p = popen (SHELLRND, "r");
    if (!p) {
        perror ("could not execute '" SHELLRND "'");
        return;
    }
    while (fread (&t, 1, sizeof (t), p) == sizeof (t)) {
        unsigned long long c;
// printf ("%.8s", (char *) &t);
        random_seed ^= t;
        c = random_seed % 214480403ULL;
        random_seed ^= HASH (c);
    }
    pclose (p);
    scramble_random ();
}

struct randblock {
    unsigned int d[2];
};

static void get_random (struct randblock *r)
{
    unsigned long long v;

    random_seed ^= random ();
    random_seed ^= random_seed << 22;
    random_seed ^= random ();
    random_seed ^= random_seed << 22;
    random_seed ^= random ();

    if (!(random_seed & 0x1f))
        scramble_random ();

/* make sure there are 16 hex digits and more than zero if cast to signed: */
    v = (random_seed >> 2) | (1ULL << 62);

    r->d[0] = (v >> 32);
    r->d[1] = (v & 0xFFFFFFFF);
}

static void remotefs_writefile_ (void (*intermediate_ack_cb) (void *, int), int (*chunk_cb) (void *, unsigned char *, int *, char *), void *hook, const char *filename, long filelen, int overwritemode, unsigned int permissions, const char *backup_extension, CStr * r)
{
    struct portable_stat st, st_orig;
    unsigned char *p;
    unsigned char chunk[READER_CHUNK];
    char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
    enum remotefs_error_code remotefs_error_code_ = RFSERR_SUCCESS;
    HANDLE fd = INVALID_HANDLE_VALUE;
    HANDLE fd_exists = INVALID_HANDLE_VALUE;
    char temp_name[MAX_PATH_LEN + 40];

    if (strlen (filename) > MAX_PATH_LEN - 1) {
        alloc_encode_error (r, RFSERR_PATHNAME_TOO_LONG, "Pathname too long", FORCE_SHUTDOWN);
        goto errout;
    }

    memset (&st, '\0', sizeof (st));
    memset (&st_orig, '\0', sizeof (st_orig));

    fd_exists = open (translate_path_sep (filename), O_WRONLY | _O_BINARY, permissions);

    if (fd_exists == INVALID_HANDLE_VALUE) {
        if (errno != ENOENT) {
/* The only reason we should not be able to write to the file is that it does not exist,
   any other reason is a problem: */
            alloc_encode_errno_strerror (r, FORCE_SHUTDOWN);
            goto errout;
        }
/* If the file does not exist, then no point in safe save or backup mode */
        overwritemode = REMOTEFS_WRITEFILE_OVERWRITEMODE_QUICK;
    } else {
        if (overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_SAFE || overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_BACKUP) {
            if (portable_stat (0, translate_path_sep (filename), &st_orig, NULL, &remotefs_error_code_, errmsg)) {
                alloc_encode_error (r, remotefs_error_code_, errmsg, FORCE_SHUTDOWN);
                goto errout;
            }
        }
        close (fd_exists);
        fd_exists = INVALID_HANDLE_VALUE;
    }

    if (overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_SAFE || overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_BACKUP) {
        char *p;
        struct randblock b;
        strcpy (temp_name, filename);
        p = strrchr (temp_name, '/');
        if (p)
            p++;
        else
            p = temp_name;
        get_random (&b);
        snprintf (p, 40, "tmp%x%x", b.d[0], b.d[1]);
        fd = open (translate_path_sep (temp_name), O_CREAT|O_WRONLY|O_TRUNC | _O_BINARY, permissions);
        if (fd == INVALID_HANDLE_VALUE) {
            alloc_encode_errno_strerror (r, FORCE_SHUTDOWN);
            goto errout;
        }
    } else {
        fd = open (translate_path_sep (filename), O_CREAT|O_WRONLY|O_TRUNC | _O_BINARY, permissions);
        if (fd == INVALID_HANDLE_VALUE) {
            alloc_encode_errno_strerror (r, FORCE_SHUTDOWN);
            goto errout;
        }
    }


    while (filelen > 0) {
        int c;
        c = READER_CHUNK;
        if ((*chunk_cb) (hook, chunk, &c, errmsg)) {
            alloc_encode_error (r, RFSERR_OTHER_ERROR, errmsg, FORCE_SHUTDOWN);
            goto errout;
        }
        assert (c <= filelen);
        filelen -= c;
        if (write (fd, chunk, c) != c) {
            alloc_encode_errno_strerror (r, FORCE_SHUTDOWN);
            goto errout;
        }
    }

/* from here on client and server are in sync, so FORCE_SHUTDOWN is not necessary */

    close (fd);
    fd = INVALID_HANDLE_VALUE;

    if (overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_SAFE || overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_BACKUP) {
        if (overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_BACKUP) {
            char backup[MAX_PATH_LEN + 40];
            snprintf (backup, sizeof (backup), "%s%s", filename, backup_extension);
	    if (rename (translate_path_sep (filename), translate_path_sep (backup)) == -1) {
                alloc_encode_errno_strerror (r, 0);
                goto errout;
            }
        }
	if (rename (translate_path_sep (temp_name), translate_path_sep (filename)) == -1) {
            alloc_encode_errno_strerror (r, 0);
            goto errout;
        }
#ifdef MSWIN
        /* chown (filename, st_orig.st_uid, st_orig.st_gid); */
#else
        {
            int chown_result;
            chown_result = chown (translate_path_sep (filename), st_orig.ustat.st_uid, st_orig.ustat.st_gid);
/* we don't care if this fails, the important part is that the file was written out. */
            (void) chown_result;
        }
#endif
/* chmod comes after since chown removes the setuid bit */
        chmod (translate_path_sep (filename), st_orig.ustat.st_mode & 07777);
    }

/* get the modified time: */
    if (portable_stat (0, translate_path_sep (filename), &st, NULL, &remotefs_error_code_, errmsg)) {
        alloc_encode_error (r, remotefs_error_code_, errmsg, 0);
        goto errout;
    }

    r->len = encode_uint (NULL, REMOTEFS_SUCCESS);
    r->len += encode_stat (NULL, &st);
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_uint (&p, REMOTEFS_SUCCESS);
    encode_stat (&p, &st);

    (*intermediate_ack_cb) (hook, 0);

    return;

  errout:
    if (fd != INVALID_HANDLE_VALUE)
        close (fd);
    if (fd_exists != INVALID_HANDLE_VALUE)
        close (fd_exists);

/* tell the remote to hang up and not send any more data: */
    (*intermediate_ack_cb) (hook, 1);
}

static void remotefs_checkordinaryfileaccess_ (const char *filename, unsigned long long sizelimit, CStr * r)
{
    struct portable_stat st;
    unsigned char *p;
    char errmsg_[REMOTEFS_ERR_MSG_LEN] = "";
    char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
    enum remotefs_error_code remotefs_error_code_ = RFSERR_SUCCESS;

    HANDLE fd = INVALID_HANDLE_VALUE;
    memset (&st, '\0', sizeof (st));
#ifdef MSWIN
#warning must use OpenFile to get full errors
#endif
    if ((fd = open ((char *) translate_path_sep (filename), O_RDONLY | _O_BINARY)) == INVALID_HANDLE_VALUE) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, " Failed trying to open file for reading: %s \n [%s] ", filename, strerror (errno));
        goto errout;
    }
    if (portable_stat (0, translate_path_sep (filename), &st, NULL, &remotefs_error_code_, errmsg_) < 0) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, " Cannot get size/permissions info on file: %s \n [%.*s] ", filename, REMOTEFS_ERR_MSG_LEN - 58, errmsg_);
        goto errout;
    }
    if (S_ISDIR (st.ustat.st_mode) || S_ISSOCK (st.ustat.st_mode) || S_ISFIFO (st.ustat.st_mode)) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, " Not an ordinary file: %s ", filename);
        goto errout;
    }
    if ((unsigned long long) st.ustat.st_size >= sizelimit) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, " File is too large: %s \n Increase edit.h:MAXBUFF and recompile the editor. ", filename);
        goto errout;
    }
    close (fd);

    r->len = encode_uint (NULL, REMOTEFS_SUCCESS);
    r->len += encode_stat (NULL, &st);
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_uint (&p, REMOTEFS_SUCCESS);
    encode_stat (&p, &st);
    return;

  errout:
    if (fd != INVALID_HANDLE_VALUE)
        close (fd);
    alloc_encode_error (r, RFSERR_OTHER_ERROR, errmsg, 0);
    return;
}

static void remotefs_stat_ (const char *pathname, int handle_just_not_there, CStr * r)
{
    char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
    enum remotefs_error_code remotefs_error_code_ = RFSERR_SUCCESS;
    struct portable_stat st;
    unsigned char *p;
    int just_not_there = 0;

    memset (&st, '\0', sizeof (st));

    if (portable_stat (0, translate_path_sep (pathname), &st, handle_just_not_there ? &just_not_there : NULL, &remotefs_error_code_, errmsg) < 0) {
        if (!handle_just_not_there || !just_not_there) {
            alloc_encode_error (r, remotefs_error_code_, errmsg, 0);
            return;
        }
    }

    r->len = encode_uint (NULL, REMOTEFS_SUCCESS);
    r->len += encode_uint (NULL, just_not_there);
    r->len += encode_uint (NULL, remotefs_error_code_);
    r->len += encode_str (NULL, errmsg, strlen (errmsg));
    r->len += encode_stat (NULL, &st);
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_uint (&p, REMOTEFS_SUCCESS);
    encode_uint (&p, just_not_there);
    encode_uint (&p, remotefs_error_code_);
    encode_str (&p, errmsg, strlen (errmsg));
    encode_stat (&p, &st);
}

char *get_current_wd (char *p, int size)
{
    char *q = NULL;
    char *d;

    strcpy (p, "/");

#ifdef HAVE_GETCWD
    d = getcwd (p, size - 1);
#else
    d = getwd (p);
#endif
    (void) d;

    p[size - 1] = '\0';

#ifdef MSWIN
    q = windows_path_to_unix (p);
    strncpy (p, q, size - 1);
    free (q);
#else
    (void) q;
#endif

    p[size - 1] = '\0';
    return p;
}

static void remotefs_chdir_ (const char *dirname, CStr * r)
{
    unsigned char *p;
    char current_dir[MAX_PATH_LEN];

    if (*dirname) {
        if (chdir (translate_path_sep (dirname)) < 0) {
            alloc_encode_errno_strerror (r, 0);
            return;
        }
    }
    if (!get_current_wd (current_dir, MAX_PATH_LEN))
	strcpy (current_dir, PATH_SEP_STR);

    r->len = encode_uint (NULL, REMOTEFS_SUCCESS);
    r->len += encode_str (NULL, current_dir, strlen (current_dir));
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_uint (&p, REMOTEFS_SUCCESS);
    encode_str (&p, current_dir, strlen (current_dir));
}

static void remotefs_realpathize_ (const char *path, const char *homedir, CStr * r)
{
    unsigned char *p;
    char *out = NULL;

    out = pathdup_ (path, homedir);
    if (!out) {
        alloc_encode_errno_strerror (r, 0);
        return;
    }

    r->len = encode_uint (NULL, REMOTEFS_SUCCESS);
    r->len += encode_str (NULL, out, strlen (out));
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_uint (&p, REMOTEFS_SUCCESS);
    encode_str (&p, out, strlen (out));

    free (out);
}

static void remotefs_gethomedir_ (CStr * r)
{
    unsigned char *p;
    static char *homedir = NULL;

    if (!homedir || !*homedir) {
#ifdef MSWIN
        homedir = getenv ("HOMEPATH");
        if (!homedir || !*homedir) {
            alloc_encode_error (r, RFSERR_OTHER_ERROR, "HOMEPATH env var is empty", 0);
            return;
        }
        homedir = windows_path_to_unix (homedir);
#else
        homedir = getenv ("HOME");
        if (!homedir || !*homedir) {
            struct passwd *pe;
            pe = getpwuid (geteuid ());
            if (!pe) {
                alloc_encode_errno_strerror (r, 0);
                return;
            }
            homedir = pe->pw_dir;
            if (!homedir || !*homedir) {
                alloc_encode_error (r, RFSERR_OTHER_ERROR,
                                    "getpwuid returned empty field pw_dir for home directory and HOME env var is empty", 0);
                return;
            }
        }
#endif
    }

    r->len = encode_uint (NULL, REMOTEFS_SUCCESS);
    r->len += encode_str (NULL, homedir, strlen (homedir));
    r->data = (char *) malloc (r->len);
    p = (unsigned char *) r->data;
    encode_uint (&p, REMOTEFS_SUCCESS);
    encode_str (&p, homedir, strlen (homedir));
}


#define MARSHAL_START_LOCAL \
    { \
        const unsigned char *p, *end; \
        unsigned long long v; \
        end = (const unsigned char *) s.data + s.len; \
        p = (const unsigned char *) s.data; \
        if (decode_uint (&p, end, &v)) \
            goto errout; \
        if (v != REMOTEFS_SUCCESS) \
            goto errout

#define MARSHAL_END_LOCAL(error_code_) \
        free (s.data); \
        return 0; \
      errout: \
        p = (const unsigned char *) s.data; \
        decode_error (&p, (const unsigned char *) s.data + s.len, error_code_, errmsg, NULL); \
        free (s.data); \
        return -1; \
    }

#define MARSHAL_START_REMOTE \
    { \
        int force_shutdown = 0; \
        const unsigned char *p, *end; \
        unsigned long long v; \
        end = (const unsigned char *) s.data + s.len; \
        p = (const unsigned char *) s.data; \
        if (decode_uint (&p, end, &v)) \
            goto errout; \
        if (v != REMOTEFS_SUCCESS) \
            goto errout

#define MARSHAL_END_REMOTE(error_code_) \
        free (s.data); \
        return 0; \
      errout: \
        p = (const unsigned char *) s.data; \
        decode_error (&p, (const unsigned char *) s.data + s.len, error_code_, errmsg, &force_shutdown); \
        free (s.data); \
        if (force_shutdown) \
            SHUTSOCK (rfs->remotefs_private->sock); \
        return -1; \
    }


static int local_listdir (struct remotefs *rfs, const char *directory, unsigned long options, char *filter, struct file_entry **r, int *n, char *errmsg)
{
    CStr s;
    remotefs_listdir_ (directory, options, filter, &s);

    MARSHAL_START_LOCAL;
    if (decode_filelist (&p, (const unsigned char *) s.data + s.len, r, n)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_LOCAL(NULL);
}

static int local_chunk_reader_cb (void *hook, const unsigned char *chunk, int chunklen, long filelen, char *errmsg)
{
    struct action_callbacks *o;
    o = (struct action_callbacks *) hook;

    return (*o->sock_reader) (o, chunk, chunklen, filelen, errmsg);
}

static int local_readfile (struct remotefs *rfs, struct action_callbacks *o, const char *filename, char *errmsg)
{
    CStr s;
    *errmsg = '\0';
    remotefs_readfile_ (local_chunk_reader_cb, NULL, (void *) o, filename, &s);

    MARSHAL_START_LOCAL;
    /* nothing to decode */
    MARSHAL_END_LOCAL(NULL);
}

static int local_chunk_writer_cb (void *hook, unsigned char *chunk, int *chunklen, char *errmsg)
{
    struct action_callbacks *o;
    o = (struct action_callbacks *) hook;

    return (*o->sock_writer) (o, chunk, chunklen, errmsg);
}

static void local_intermediate_ack_cb (void *hook, int got_error)
{
    /* only used for remote connections */
}

static int local_writefile (struct remotefs *rfs, struct action_callbacks *o, const char *filename, long filelen, int overwritemode, unsigned int permissions, const char *backup_extension, struct portable_stat *st, char *errmsg)
{
    CStr s;
    *errmsg = '\0';
    remotefs_writefile_ (local_intermediate_ack_cb, local_chunk_writer_cb, (void *) o, filename, filelen, overwritemode, permissions, backup_extension, &s);

    MARSHAL_START_LOCAL;
    if (decode_stat (&p, end, st)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_LOCAL(NULL);
}

static int local_checkordinaryfileaccess (struct remotefs *rfs, const char *filename, unsigned long long sizelimit, struct portable_stat *st, char *errmsg)
{
    CStr s;
    *errmsg = '\0';
    remotefs_checkordinaryfileaccess_ (filename, sizelimit, &s);

    MARSHAL_START_LOCAL;
    if (decode_stat (&p, end, st)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_LOCAL(NULL);
}

static int local_stat (struct remotefs *rfs, const char *pathname, struct portable_stat *st, int *just_not_there, remotefs_error_code_t *error_code, char *errmsg)
{
    CStr s;
    unsigned long long just_not_there_;
    unsigned long long error_code_;
    *errmsg = '\0';
    remotefs_stat_ (pathname, just_not_there != NULL, &s);

    MARSHAL_START_LOCAL;
    if (decode_uint (&p, end, &just_not_there_))
        return -1;
    if (just_not_there)
        *just_not_there = just_not_there_;
    if (decode_uint (&p, end, &error_code_))
        return -1;
    *error_code = error_code_;
    if (decode_str (&p, end, errmsg, REMOTEFS_ERR_MSG_LEN))
        return -1;
    if (decode_stat (&p, end, st)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_LOCAL(error_code);
}

static int local_chdir (struct remotefs *rfs, const char *dirname, char *cwd, int cwdlen, char *errmsg)
{
    CStr s;
    *errmsg = '\0';
    remotefs_chdir_ (dirname, &s);

    MARSHAL_START_LOCAL;
    if (decode_str (&p, end, cwd, cwdlen)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_LOCAL(NULL);
}

static int local_realpathize (struct remotefs *rfs, const char *path, const char *homedir, char *out, int outlen, char *errmsg)
{
    CStr s;
    *errmsg = '\0';
    remotefs_realpathize_ (path, homedir, &s);

    MARSHAL_START_LOCAL;
    if (decode_str (&p, end, out, outlen)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_LOCAL(NULL);
}

static int local_gethomedir (struct remotefs *rfs, char *out, int outlen, char *errmsg)
{
    CStr s;
    *errmsg = '\0';
    remotefs_gethomedir_ (&s);

    MARSHAL_START_LOCAL;
    if (decode_str (&p, end, out, outlen)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_LOCAL(NULL);
}

static int encode_listdir_params (unsigned char **p_, const char *directory, unsigned long options, char *filter)
{
    int r;
    r = encode_str (p_, directory, strlen (directory));
    r += encode_uint (p_, options);
    r += encode_str (p_, filter, strlen (filter));
    return r;
}


static int send_recv_mesg (struct remotefs *rfs, CStr *msg, CStr *response, int action, char *errmsg);
static int send_mesg (struct remotefs *rfs, struct reader_data *d, CStr * msg, int action, char *errmsg);
static int recv_mesg (struct remotefs *rfs, struct reader_data *d, CStr * response, int action, char *errmsg);
static int reader (struct reader_data *d, void *buf_, int buflen);


static int remote_listdir (struct remotefs *rfs, const char *directory, unsigned long options, char *filter, struct file_entry **r, int *n, char *errmsg)
{
    CStr s, msg;
    unsigned char *q;
    *errmsg = '\0';

    msg.len = encode_listdir_params (NULL, directory, options, filter);
    msg.data = (char *) malloc (msg.len);
    q = (unsigned char *) msg.data;
    msg.len = encode_listdir_params (&q, directory, options, filter);

    if (send_recv_mesg (rfs, &msg, &s, REMOTEFS_ACTION_READDIR, errmsg)) {
        free (msg.data);
        return -1;
    }
    free (msg.data);

    MARSHAL_START_REMOTE;
    if (decode_filelist (&p, (const unsigned char *) s.data + s.len, r, n)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_REMOTE(NULL);
}


static int remote_readfile (struct remotefs *rfs, struct action_callbacks *o, const char *filename, char *errmsg)
{
    char throw_away[REMOTEFS_ERR_MSG_LEN];
    CStr s, msg;
    unsigned char *q;
    unsigned long long filelen, remaining;
    struct reader_data d;
    unsigned char buf[READER_CHUNK];
    unsigned char t[6];
    int err = 0;

    *errmsg = '\0';

    msg.len = encode_str (NULL, filename, strlen (filename));
    msg.data = (char *) malloc (msg.len);
    q = (unsigned char *) msg.data;
    encode_str (&q, filename, strlen (filename));

    memset (&d, '\0', sizeof (d));
    d.sock = &rfs->remotefs_private->sock;

    if (send_mesg (rfs, &d, &msg, REMOTEFS_ACTION_READFILE, errmsg)) {
        free (msg.data);
        return -1;
    }

    free (msg.data);
    msg.data = NULL;

    if (reader (&d, t, 6)) {
        set_sockerrmsg_to_errno (errmsg, errno);
        return -1;
    }

    decode_uint48 (t, &filelen);
    remaining = filelen;

/* even if the remote has an error we continue reading the full network
   transaction to preserve continuity of the connection: */
    while (remaining > 0) {
        int c;
        c = (MIN ((unsigned long long) READER_CHUNK, remaining));
        if (reader (&d, buf, c)) {
            set_sockerrmsg_to_errno (errmsg, errno);
            return -1;
        }
        if (!err)
            if ((*o->sock_reader) (o, buf, c, filelen, errmsg))
                err = 1;
        remaining -= c;
    }

    if (recv_mesg (rfs, &d, &s, REMOTEFS_ACTION_READFILE, err ? throw_away : errmsg))
        return -1;

    MARSHAL_START_REMOTE;
    /* nothing to decode */
    MARSHAL_END_REMOTE(NULL);
}

static int recv_ack (struct reader_data *d, int *got_ack, int *got_stop)
{
    struct cooledit_remote_msg_ack ack;
    unsigned long version;
    remotefs_error_code_t error_code;

    if (*got_ack)
        return 0;

    if (reader (d, &ack, sizeof (ack)))
        return -1;

    decode_msg_ack (&ack, &error_code, &version);

    *got_ack = 1;
    *got_stop = (ack.error_code != RFSERR_SUCCESS);

    return 0;
}

static int maybe_see_ack (struct reader_data *d, int *got_ack, int *got_stop)
{
    fd_set rd;
    struct timeval tv;

    if (*got_ack)
        return 0;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO (&rd);
    FD_SET (*d->sock, &rd);

    if (select (*d->sock + 1, &rd, NULL, NULL, &tv) == 1 && FD_ISSET (*d->sock, &rd))
        if (recv_ack (d, got_ack, got_stop))
            return -1;

    return 0;
}

static int remote_writefile (struct remotefs *rfs, struct action_callbacks *o, const char *filename, long filelen, int overwritemode, unsigned int permissions, const char *backup_extension, struct portable_stat *st, char *errmsg)
{
    CStr s, msg;
    unsigned char *q;
    unsigned long long remaining;
    struct reader_data d;
    unsigned char buf[READER_CHUNK];
    int got_ack = 0;
    int got_stop = 0;

/* Network could hang up in the middle of a write, so do "safe saves" only: */
    if (overwritemode == REMOTEFS_WRITEFILE_OVERWRITEMODE_QUICK)
        overwritemode = REMOTEFS_WRITEFILE_OVERWRITEMODE_SAFE;

    *errmsg = '\0';
    memset (st, '\0', sizeof (*st));

    msg.len = encode_str (NULL, filename, strlen (filename));
    msg.len += encode_uint (NULL, filelen);
    msg.len += encode_uint (NULL, overwritemode);
    msg.len += encode_uint (NULL, permissions);
    msg.len += encode_str (NULL, backup_extension, strlen (backup_extension));

    msg.data = (char *) malloc (msg.len);
    q = (unsigned char *) msg.data;
    encode_str (&q, filename, strlen (filename));
    encode_uint (&q, filelen);
    encode_uint (&q, overwritemode);
    encode_uint (&q, permissions);
    encode_str (&q, backup_extension, strlen (backup_extension));

    memset (&d, '\0', sizeof (d));
    d.sock = &rfs->remotefs_private->sock;

    if (send_mesg (rfs, &d, &msg, REMOTEFS_ACTION_WRITEFILE, errmsg)) {
        free (msg.data);
        return -1;
    }

    free (msg.data);
    msg.data = NULL;

    remaining = filelen;

/* we adopt the protocol that the remote can indicate a success/failure ack at any time.
   this is primarily useful for a remote that fills up its device and returns an error
   midway */

    while (remaining > 0) {
        int c;

        if (maybe_see_ack (&d, &got_ack, &got_stop)) {
            set_sockerrmsg_to_errno (errmsg, errno);
            return -1;
        }
        if (got_stop)
            break;

        c = (MIN ((unsigned long long) READER_CHUNK, remaining));
        if ((*o->sock_writer) (o, buf, &c, errmsg)) {
            SHUTSOCK (rfs->remotefs_private->sock);
            strcpy (errmsg, "Ran out of data to write");
            return -1;
        }
        assert (c > 0);

        if (writer (*d.sock, buf, c)) {
            set_sockerrmsg_to_errno (errmsg, errno);
            if (!maybe_see_ack (&d, &got_ack, &got_stop) && got_stop)
                break;
            return -1;
        }
        remaining -= c;
    }

    if (recv_ack (&d, &got_ack, &got_stop)) {
        set_sockerrmsg_to_errno (errmsg, errno);
        return -1;
    }

    if (recv_mesg (rfs, &d, &s, REMOTEFS_ACTION_WRITEFILE, errmsg))
        return -1;

    MARSHAL_START_REMOTE;
    if (decode_stat (&p, end, st)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_REMOTE(NULL);
}

static int remote_checkordinaryfileaccess (struct remotefs *rfs, const char *filename, unsigned long long sizelimit, struct portable_stat *st, char *errmsg)
{
    CStr s, msg;
    unsigned char *q;
    *errmsg = '\0';

    msg.len = encode_str (NULL, filename, strlen (filename));
    msg.len += encode_uint (NULL, sizelimit);
    msg.data = (char *) malloc (msg.len);
    q = (unsigned char *) msg.data;
    encode_str (&q, filename, strlen (filename));
    encode_uint (&q, sizelimit);

    if (send_recv_mesg (rfs, &msg, &s, REMOTEFS_ACTION_CHECKORDINARYFILEACCESS, errmsg)) {
        free (msg.data);
        return -1;
    }
    free (msg.data);

    MARSHAL_START_REMOTE;
    if (decode_stat (&p, end, st)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_REMOTE(NULL);
}

static int remote_stat (struct remotefs *rfs, const char *pathname, struct portable_stat *st, int *just_not_there, remotefs_error_code_t *error_code, char *errmsg)
{
    CStr s, msg;
    unsigned char *q;
    unsigned long long just_not_there_;
    unsigned long long error_code_;

    *errmsg = '\0';

    msg.len = encode_str (NULL, pathname, strlen (pathname));
    msg.len += encode_uint (NULL, (just_not_there != NULL));
    msg.data = (char *) malloc (msg.len);
    q = (unsigned char *) msg.data;
    encode_str (&q, pathname, strlen (pathname));
    encode_uint (&q, (just_not_there != NULL));

    if (send_recv_mesg (rfs, &msg, &s, REMOTEFS_ACTION_STAT, errmsg)) {
        free (msg.data);
        return -1;
    }
    free (msg.data);

    MARSHAL_START_REMOTE;
    if (decode_uint (&p, end, &just_not_there_))
        return -1;
    if (just_not_there)
        *just_not_there = just_not_there_;
    if (decode_uint (&p, end, &error_code_))
        return -1;
    *error_code = error_code_;
    if (decode_str (&p, end, errmsg, REMOTEFS_ERR_MSG_LEN))
        return -1;
    if (decode_stat (&p, end, st)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_REMOTE(error_code);
}

static int remote_chdir (struct remotefs *rfs, const char *dirname, char *cwd, int cwdlen, char *errmsg)
{
    CStr s, msg;
    unsigned char *q;
    *errmsg = '\0';

    msg.len = encode_str (NULL, dirname, strlen (dirname));
    msg.data = (char *) malloc (msg.len);
    q = (unsigned char *) msg.data;
    encode_str (&q, dirname, strlen (dirname));

    if (send_recv_mesg (rfs, &msg, &s, REMOTEFS_ACTION_CHDIR, errmsg)) {
        free (msg.data);
        return -1;
    }
    free (msg.data);

    MARSHAL_START_REMOTE;
    if (decode_str (&p, end, cwd, cwdlen)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_REMOTE(NULL);
}

static int remote_realpathize (struct remotefs *rfs, const char *path, const char *homedir, char *out, int outlen, char *errmsg)
{
    CStr s, msg;
    unsigned char *q;
    *errmsg = '\0';

    msg.len = encode_str (NULL, path, strlen (path));
    msg.len += encode_str (NULL, homedir, strlen (homedir));
    msg.data = (char *) malloc (msg.len);
    q = (unsigned char *) msg.data;
    encode_str (&q, path, strlen (path));
    encode_str (&q, homedir, strlen (homedir));

    if (send_recv_mesg (rfs, &msg, &s, REMOTEFS_ACTION_REALPATHIZE, errmsg)) {
        free (msg.data);
        return -1;
    }
    free (msg.data);

    MARSHAL_START_REMOTE;
    if (decode_str (&p, end, out, outlen)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_REMOTE(NULL);
}

static int remote_gethomedir (struct remotefs *rfs, char *out, int outlen, char *errmsg)
{
    CStr s, msg;
    *errmsg = '\0';

    memset (&msg, '\0', sizeof (msg));

    if (send_recv_mesg (rfs, &msg, &s, REMOTEFS_ACTION_GETHOMEDIR, errmsg)) {
        free (msg.data);
        return -1;
    }
    free (msg.data);

    MARSHAL_START_REMOTE;
    if (decode_str (&p, end, out, outlen)) {
        free (s.data);
        return -1;
    }
    MARSHAL_END_REMOTE(NULL);
}

struct iprange_list;
int iprange_match (struct iprange_list *l, const void *a, int addrlen);
struct iprange_list *iprange_parse (const char *text, int *consumed);
int text_to_ip (const char *s, int *consumed_, void *out, int *addr_len);
void ip_to_text (const void *ip, int addrlen, char *out);

// #define LEGACY_IP4_ONLY

struct remotefs_sockaddr_s_ {
#ifdef LEGACY_IP4_ONLY
    struct sockaddr_in ss;
#else
    struct sockaddr_storage ss;
#endif
};

typedef struct remotefs_sockaddr_s_ remotefs_sockaddr_t;

static void *remotefs_sockaddr_t_address (remotefs_sockaddr_t * a)
{
    struct sockaddr_in *sa4;
#ifndef LEGACY_IP4_ONLY
    if (a->ss.ss_family == AF_INET6) {
        struct sockaddr_in6 *sa6;
        sa6 = (struct sockaddr_in6 *) (void *) &a->ss;
        return (void *) &sa6->sin6_addr;
    }
#endif
    sa4 = (struct sockaddr_in *) (void *) &a->ss;
    return (void *) &sa4->sin_addr;
}

static int remotefs_sockaddr_t_addresslen (remotefs_sockaddr_t *a)
{
#ifndef LEGACY_IP4_ONLY
    if (a->ss.ss_family == AF_INET6)
        return 16;
#endif
    return 4;
}

static int remotefs_sockaddr_t_addressfamily (remotefs_sockaddr_t *a)
{
#ifndef LEGACY_IP4_ONLY
    if (a->ss.ss_family == AF_INET6)
        return AF_INET6;
#endif
    return AF_INET;
}

static int ipaddress_port_to_remotefs_sockaddr_t (remotefs_sockaddr_t *a, const char *addr, int port)
{
    struct sockaddr_in sa4;
    char s[16];
    int addr_len;

    memset (a, 0, sizeof (*a));

    if (text_to_ip (addr, NULL, s, &addr_len))
        return -1;

#ifdef LEGACY_IP4_ONLY
    if (addr_len == 16)
        return -1;
#else
    if (addr_len == 16) {
        struct sockaddr_in6 sa6;
        memset (&sa6, '\0', sizeof (sa6));
        sa6.sin6_family = AF_INET6;
        memcpy (&sa6.sin6_addr, s, 16);
        sa6.sin6_port = htons (port);
        memcpy (a, &sa6, sizeof (sa6));
        return 0;
    }
#endif

    memset (&sa4, 0, sizeof (sa4));
    sa4.sin_family = AF_INET;
    memcpy (&sa4.sin_addr, s, 4);
    sa4.sin_port = htons (port);
    memcpy (a, &sa4, sizeof (sa4));
    return 0;
}

static SOCKET listen_socket (const char *listen_address, int listen_port)
{
    remotefs_sockaddr_t a;
    SOCKET s;
    int yes = 1;

    if (ipaddress_port_to_remotefs_sockaddr_t (&a, listen_address, listen_port)) {
        fprintf (stderr, "invalid address: %s\n", listen_address);
        return INVALID_SOCKET;
    }

    if ((s = socket (remotefs_sockaddr_t_addressfamily (&a), SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perrorsocket ("socket");
        return INVALID_SOCKET;
    }

    if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof (yes)) == SOCKET_ERROR) {
        perrorsocket ("setsockopt");
        closesocket (s);
        return INVALID_SOCKET;
    }

    if (bind (s, (struct sockaddr *) &a, sizeof (a)) == SOCKET_ERROR) {
        perrorsocket ("bind");
        closesocket (s);
        return INVALID_SOCKET;
    }
    listen (s, 10);
    return s;
}

#define CONNCHECK_SUCCESS       0
#define CONNCHECK_WAITING       1
#define CONNCHECK_ERROR         2

// #define LOG(s)  printf ("line=%d %s=%ld\n", __LINE__, #s, (long) s)
#define LOG(s)  do { } while(0)

static int connection_check (const SOCKET s, int write_set)
{
    int r;
    char c = '\0';
#ifdef MSWIN
    int optval = 0, optlen = sizeof (int);
#else
    socklen_t l;
    remotefs_sockaddr_t a;
#endif

    LOG (write_set);

#if defined(__hpux) || defined(__hpux__) || defined(hpux)
    if (1)      /* HP-UX sometimes returns writable from select() even though nothing has happened */
#else
    if (!write_set)
#endif
    {
        r = recv (s, &c, 1, 0);

        LOG (r);
        LOG (errno);

#if defined(__hpux) || defined(__hpux__) || defined(hpux)
        if (write_set && r < 0 && errno == EAGAIN) {
            /* On HP-UX this means we are connected */
        } else
#endif
#ifdef MSWIN
/* On Windows XP, getsockopt returns no error until either a timeout or connection-refused: */
        if (r < 0 && !getsockopt (s, SOL_SOCKET, SO_ERROR, (char *) &optval, &optlen) && !optval)
#else
/* HP-UX and Solaris gives ENOTCONN, Linux gives EAGAIN, both give ETIMEDOUT or ECONNRESET on error. */
        if (r < 0 && (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR || errno == ENOTCONN))
#endif
        {
            return CONNCHECK_WAITING;
        } else if (r <= 0) {
#ifdef MSWIN
            if (optval)
                WSASetLastError (optval);
#endif
            return CONNCHECK_ERROR;
        }
    }
#ifdef MSWIN
    if (getsockopt (s, SOL_SOCKET, SO_ERROR, (char *) &optval, &optlen))
        return CONNCHECK_ERROR;
    else if (optval)
        return CONNCHECK_ERROR;
    return 0;
#else
    l = sizeof (a);
    r = getpeername (s, (struct sockaddr *) &a, &l);

    LOG (r);
    LOG (errno);

    if (r) {
        if (errno == ENOTCONN || errno == EINVAL) {
            char c;
            errno = 0;
            recv (s, &c, 1, 0);
        }
        return CONNCHECK_ERROR;
    }
    return CONNCHECK_SUCCESS;
#endif
}

static SOCKET connect_socket (const char *address, int port, char *errmsg)
{
    time_t t1, t2;
    remotefs_sockaddr_t a;
    int yes = 1;
    int r;
    SOCKET s;
#ifdef MSWIN
    u_long nbio_yes = 1;
#else
    int nbio_yes = 1;
#endif

    if ((s = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "socket: %s", strerrorsocket ());
        return INVALID_SOCKET;
    }

    if (ioctlsocket (s, FIONBIO, &nbio_yes))  {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "ioctl FIONBIO: %s", strerrorsocket ());
        SHUTSOCK (s);
        return INVALID_SOCKET;
    }

    if (ipaddress_port_to_remotefs_sockaddr_t (&a, address, port)) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "invalid address: %s", address);
        SHUTSOCK (s);
        return INVALID_SOCKET;
    }

  try_again:
    r = connect (s, (struct sockaddr *) &a, sizeof (a));

LOG(r);

    if (r == SOCKET_ERROR) {
#ifdef MSWIN
        if (WSAGetLastError() == WSAEINTR)
            goto try_again;
        if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == WSAEALREADY || WSAGetLastError() == WSAEISCONN)
            r = 0;
#else
LOG(errno);
        if (errno == EINTR)
            goto try_again;
        if (errno == EALREADY || errno == EINPROGRESS)
            r = 0;
#ifdef EISCONN
        if (errno == EISCONN)
            r = 0;
#endif
#endif
    }
LOG(r);

    if (r == SOCKET_ERROR) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "connect: %s", strerrorsocket ());
        SHUTSOCK (s);
        return INVALID_SOCKET;
    }

    time (&t1);
LOG(t1);

    for (;;) {
        struct timeval tv;
        int sr;
        fd_set wr;

        time (&t2);
        if (t2 - t1 > 5) {
LOG(t2);
            snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "timeout waiting for response from remote");
            SHUTSOCK (s);
            return INVALID_SOCKET;
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO (&wr);
        FD_SET (s, &wr);
        sr = select (s + 1, NULL, &wr, NULL, &tv);
        if (sr != 1) {
            FD_ZERO (&wr);
        }
LOG(sr);
LOG(errno);
        if (sr == SOCKET_ERROR && !(ERROR_EINTR() || ERROR_EAGAIN())) {
            snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "select(): %s", strerrorsocket ());
            SHUTSOCK (s);
            return INVALID_SOCKET;
        }

        sr = connection_check (s, FD_ISSET (s, &wr));
LOG(sr);

        if (sr == CONNCHECK_SUCCESS)
            break;
        if (sr == CONNCHECK_ERROR) {
            snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "connect(): %s", strerrorsocket ());
            SHUTSOCK (s);
            return INVALID_SOCKET;
        }
    }

LOG(0);

    if (setsockopt (s, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof (yes))) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "setsockopt(TCP_NODELAY): %s", strerrorsocket ());
        SHUTSOCK (s);
        return INVALID_SOCKET;
    }

    return s;
}


static int do_connect (struct remotefs *rfs, char *errmsg)
{
    SHUTSOCK (rfs->remotefs_private->sock);

    rfs->remotefs_private->sock = connect_socket (rfs->remotefs_private->remote, 50095, errmsg);

    if (rfs->remotefs_private->sock == INVALID_SOCKET)
        return -1;

    return 0;
}

static int send_mesg (struct remotefs *rfs, struct reader_data *d, CStr * msg, int action, char *errmsg)
{
    struct cooledit_remote_msg_header m;
    struct cooledit_remote_msg_ack ack;
    unsigned long version;
    remotefs_error_code_t error_code;
    int timeout = 0;
    int retries = 0;

  retry:

    retries++;

    if (rfs->remotefs_private->sock == INVALID_SOCKET)
        if (do_connect (rfs, errmsg))
            return -1;

    memset (&m, '\0', sizeof (m));

    encode_msg_header (&m, msg->len, MSG_VERSION, action, FILE_PROTO_MAGIC);

    if (writer (rfs->remotefs_private->sock, &m, sizeof (m))) {
        set_sockerrmsg_to_errno (errmsg, errno);
        SHUTSOCK (rfs->remotefs_private->sock);
        return -1;
    }

/* See note (*1*) below. */
    if (reader_timeout (d, &ack, sizeof (ack), option_remote_timeout, &timeout)) {
        if (timeout) {
            strcpy (errmsg, "timeout waiting for response");
            SHUTSOCK (rfs->remotefs_private->sock);
            return -1;
        }
        if (retries < 3) {
            SHUTSOCK (rfs->remotefs_private->sock);
            goto retry;
        }
        if (!errno)
            strcpy (errmsg, "remote hangs up");
        else
            set_sockerrmsg_to_errno (errmsg, errno);
        SHUTSOCK (rfs->remotefs_private->sock);
        return -1;
    }

    decode_msg_ack (&ack, &error_code, &version);
    if (error_code == RFSERR_SERVER_CLOSED_IDLE_CLIENT) {
        SHUTSOCK (rfs->remotefs_private->sock);
        goto retry;
    }
    if (error_code) {
        if (error_code > 0 && error_code < RFSERR_LAST_INTERNAL_ERROR)
            strcpy (errmsg, error_code_descr[error_code]);
        else
            strcpy (errmsg, "remote returned invalid error");
        SHUTSOCK (rfs->remotefs_private->sock);
        return -1;
    }

    if (writer (rfs->remotefs_private->sock, msg->data, msg->len)) {
        set_sockerrmsg_to_errno (errmsg, errno);
        SHUTSOCK (rfs->remotefs_private->sock);
        return -1;
    }

    return 0;
}

static int recv_mesg (struct remotefs *rfs, struct reader_data *d, CStr * response, int action, char *errmsg)
{
    unsigned long long msglen;
    unsigned long version, msgtype_response, magic;
    struct cooledit_remote_msg_header m;

    if (reader (d, &m, sizeof (m))) {
        set_sockerrmsg_to_errno (errmsg, errno);
        SHUTSOCK (rfs->remotefs_private->sock);
        return -1;
    }

    decode_msg_header (&m, &msglen, &version, &msgtype_response, &magic);

    if (magic != FILE_PROTO_MAGIC) {
        const char *remote;
        remote = rfs->remotefs_private->remote;
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "invalid magic response from %s", remote);
        SHUTSOCK (rfs->remotefs_private->sock);
        return -1;
    }

    if (msgtype_response != (unsigned long) action) {
        const char *remote;
        remote = rfs->remotefs_private->remote;
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "invalid action response %ld from %s", msgtype_response, remote);
        SHUTSOCK (rfs->remotefs_private->sock);
        return -1;
    }

    response->len = msglen;
    response->data = (char *) malloc (response->len);
    if (reader (d, response->data, response->len)) {
        set_sockerrmsg_to_errno (errmsg, errno);
        free (response->data);
        return -1;
    }

    return 0;
}

static int send_recv_mesg (struct remotefs *rfs, CStr * msg, CStr * response, int action, char *errmsg)
{
    struct reader_data d;

    memset (&d, '\0', sizeof (d));
    d.sock = &rfs->remotefs_private->sock;

    if (send_mesg (rfs, &d, msg, action, errmsg))
        return -1;

    if (recv_mesg (rfs, &d, response, action, errmsg))
        return -1;

    return 0;
}


static char remotefs_error_return_[REMOTEFS_ERR_MSG_LEN];

static int remotefs_error_return (char *errmsg)
{
    strcpy (errmsg, remotefs_error_return_);
    return -1;
}

static int dummyerr_listdir (struct remotefs *rfs, const char *directory, unsigned long options, char *filter, struct file_entry **r, int *n, char *errmsg)
{
    return remotefs_error_return (errmsg);
}

static int dummyerr_readfile (struct remotefs *rfs, struct action_callbacks *o, const char *filename, char *errmsg)
{
    return remotefs_error_return (errmsg);
}

static int dummyerr_writefile (struct remotefs *rfs, struct action_callbacks *o, const char *filename, long filelen, int overwritemode, unsigned int permissions, const char *backup_extension, struct portable_stat *st, char *errmsg)
{
    return remotefs_error_return (errmsg);
}

static int dummyerr_checkordinaryfileaccess (struct remotefs *rfs, const char *filename, unsigned long long sizelimit, struct portable_stat *st, char *errmsg)
{
    return remotefs_error_return (errmsg);
}

static int dummyerr_stat (struct remotefs *rfs, const char *path, struct portable_stat *st, int *just_not_there, remotefs_error_code_t *error_code, char *errmsg)
{
    return remotefs_error_return (errmsg);
}

static int dummyerr_chdir (struct remotefs *rfs, const char *dirname, char *cwd, int cwdlen, char *errmsg)
{
    return remotefs_error_return (errmsg);
}

static int dummyerr_realpathize (struct remotefs *rfs, const char *path, const char *homedir, char *out, int outlen, char *errmsg)
{
    return remotefs_error_return (errmsg);
}

static int dummyerr_gethomedir (struct remotefs *rfs, char *out, int outlen, char *errmsg)
{
    return remotefs_error_return (errmsg);
}



struct remotefs remotefs_dummyerr = {
    0,
    dummyerr_listdir,
    dummyerr_readfile,
    dummyerr_writefile,
    dummyerr_checkordinaryfileaccess,
    dummyerr_stat,
    dummyerr_chdir,
    dummyerr_realpathize,
    dummyerr_gethomedir,
    NULL,
};

struct remotefs remotefs_local = {
    0,
    local_listdir,
    local_readfile,
    local_writefile,
    local_checkordinaryfileaccess,
    local_stat,
    local_chdir,
    local_realpathize,
    local_gethomedir,
    NULL
};

struct remotefs remotefs_socket = {
    0,
    remote_listdir,
    remote_readfile,
    remote_writefile,
    remote_checkordinaryfileaccess,
    remote_stat,
    remote_chdir,
    remote_realpathize,
    remote_gethomedir,
    NULL
};



union remotefs_impl {
    unsigned int magic;
    struct remotefs impl_;
};

struct remotefs_item {
#define REMOTEFS_ITEM_MAGIC     0xc558ee62
    union remotefs_impl impl_;
#define impl                    impl_.impl_
    struct remotefs_item *next;
    char host[256];
    char home_dir[MAX_PATH_LEN];
};

struct remotefs_item *remotefs_list = NULL;


const char *remotefs_home_dir (struct remotefs *rfs)
{
    struct remotefs_item *i;
    i = (struct remotefs_item *) (void *) rfs;
    assert (i->impl.magic == REMOTEFS_ITEM_MAGIC);
    return i->home_dir;
}


static int remotefs_start (struct remotefs_item *i, const char *host, char *errmsg)
{
    memset (i, '\0', sizeof (*i));
    strcpy (i->host, host);
    if (!strcmp (host, REMOTEFS_LOCAL)) {
        i->impl = remotefs_local;
        i->impl.remotefs_private = NULL;
    } else {
        i->impl = remotefs_socket;
        i->impl.remotefs_private = (struct remotefs_private *) malloc (sizeof (struct remotefs_private));
        memset (i->impl.remotefs_private, '\0', sizeof (struct remotefs_private));
        strcpy (i->impl.remotefs_private->remote, host);
        i->impl.remotefs_private->sock = INVALID_SOCKET;
    }
    i->impl.magic = REMOTEFS_ITEM_MAGIC;
    if ((*i->impl.remotefs_gethomedir) (&i->impl, i->home_dir, sizeof (i->home_dir), errmsg)) {
        if (i->impl.remotefs_private)
            free (i->impl.remotefs_private);
        return -1;
    }
    return 0;
}


struct remotefs *remotefs_lookup (const char *host_)
{
    static struct remotefs_item dummy;
    int r;
    char host[256];
    char addr[64];
    int addrlen = 0;
    struct remotefs_item *i;
    if (!strcmp (host_, REMOTEFS_LOCAL)) {
        strcpy (host, host_);
    } else {
        if ((r = text_to_ip (host_, NULL, addr, &addrlen))) {
            snprintf (remotefs_error_return_, REMOTEFS_ERR_MSG_LEN, "invalid ip address %s, (%d)", host_, r);
            goto errout;
        }
        ip_to_text (addr, addrlen, host);
    }
    for (i = remotefs_list; i; i = i->next)
        if (!strcmp (i->host, host))
            return &i->impl;
    i = (struct remotefs_item *) malloc (sizeof (*i));
    if (remotefs_start (i, host, remotefs_error_return_)) {
        free (i);
        goto errout;
    }
    i->next = remotefs_list;
    remotefs_list = i;
    return &i->impl;

  errout:
    dummy.impl = remotefs_dummyerr;
    dummy.impl.magic = REMOTEFS_ITEM_MAGIC;
    strcpy (dummy.home_dir, "/");
    return &dummy.impl;
}








struct server_data {
    struct reader_data *reader_data;
};

static int remote_action_fn_v1_notimplemented (struct server_data *sd, CStr * s, const unsigned char *in, int inlen)
{
    alloc_encode_error (s, RFSERR_UNIMPLEMENTED_FUNCTION, "Unimplemented function", 0);
    return 0;
}

static int remote_action_fn_v1_listdir (struct server_data *sd, CStr *s, const unsigned char *in, int inlen)
{
    const unsigned char *p, *end;
    char directory[MAX_PATH_LEN];
    char filter[256];
    unsigned long long options;

    p = in;
    end = in + inlen;
    if (decode_str (&p, end, directory, sizeof (directory)))
        return -1;
    if (decode_uint (&p, end, &options))
        return -1;
    if (decode_str (&p, end, filter, sizeof (filter)))
        return -1;
    remotefs_listdir_ (directory, options, filter, s);
    return 0;
}

struct server_reader_info {
    struct server_data *sd;
    long progress;
    long filelen;
};

static int remote_chunk_startreader_cb (void *hook, long filelen, char *errmsg)
{
    struct server_reader_info *info;
    unsigned char p[6];

    info = (struct server_reader_info *) hook;

    encode_uint48 (p, filelen);
    if (writer (*info->sd->reader_data->sock, p, 6)) {
        info->progress = -1;
        set_sockerrmsg_to_errno (errmsg, errno);
        return -1;
    }

    return 0;
}

static int remote_chunk_reader_cb (void *hook, const unsigned char *chunk, int chunklen, long filelen, char *errmsg)
{
    struct server_reader_info *info;

    info = (struct server_reader_info *) hook;

    if (writer (*info->sd->reader_data->sock, chunk, chunklen)) {
        set_sockerrmsg_to_errno (errmsg, errno);
        return -1;
    }

    info->filelen = filelen;
    info->progress += chunklen;

    return 0;
}

static int remote_action_fn_v1_readfile (struct server_data *sd, CStr * s, const unsigned char *in, int inlen)
{
    const unsigned char *p, *end;
    char filename[MAX_PATH_LEN];
    struct server_reader_info info;

    memset (&info, '\0', sizeof (info));
    info.sd = sd;

    p = in;
    end = in + inlen;
    if (decode_str (&p, end, filename, sizeof (filename)))
        return -1;

    remotefs_readfile_ (remote_chunk_reader_cb, remote_chunk_startreader_cb, (void *) &info, filename, s);
    if (info.progress != info.filelen)
        return -1;
    return 0;
}

struct server_writer_info {
    struct server_data *sd;
    long remaining;
};

static int remote_chunk_writer_cb (void *hook, unsigned char *chunk, int *chunklen, char *errmsg)
{
    struct server_writer_info *info;

    info = (struct server_writer_info *) hook;

    *chunklen = MIN (info->remaining, *chunklen);

    if (reader (info->sd->reader_data, chunk, *chunklen)) {
        set_sockerrmsg_to_errno (errmsg, errno);
        return -1;
    }

    info->remaining -= *chunklen;

    return 0;
}

static void remote_intermediate_ack_cb (void *hook, int got_error)
{
    struct server_writer_info *info;
    struct cooledit_remote_msg_ack ack;

    info = (struct server_writer_info *) hook;

    memset (&ack, '\0', sizeof (ack));
    encode_msg_ack (&ack, got_error ? RFSERR_EARLY_TERMINATE_FROM_WRITE_FILE : RFSERR_SUCCESS, MSG_VERSION);

    writer (*info->sd->reader_data->sock, &ack, sizeof (ack));
}

static int remote_action_fn_v1_writefile (struct server_data *sd, CStr *s, const unsigned char *in, int inlen)
{
    const unsigned char *p, *end;
    char filename[MAX_PATH_LEN];
    char backup_extension[MAX_PATH_LEN];
    struct server_writer_info info;
    unsigned long long filelen, overwritemode, permissions;

    memset (&info, '\0', sizeof (info));
    info.sd = sd;

    p = in;
    end = in + inlen;
    if (decode_str (&p, end, filename, sizeof (filename)))
        return -1;
    if (decode_uint (&p, end, &filelen))
        return -1;
    if (decode_uint (&p, end, &overwritemode))
        return -1;
    if (decode_uint (&p, end, &permissions))
        return -1;
    if (decode_str (&p, end, backup_extension, sizeof (backup_extension)))
        return -1;
    info.remaining = filelen;
    remotefs_writefile_ (remote_intermediate_ack_cb, remote_chunk_writer_cb, (void *) &info, filename, filelen, overwritemode, permissions, backup_extension, s);
    if (info.remaining)
        return -1;
    return 0;
}

static int remote_action_fn_v1_checkordinaryfileaccess (struct server_data *sd, CStr *s, const unsigned char *in, int inlen)
{
    const unsigned char *p, *end;
    char filename[MAX_PATH_LEN];
    unsigned long long sizelimit;
    p = in;
    end = in + inlen;
    if (decode_str (&p, end, filename, sizeof (filename)))
        return -1;
    if (decode_uint (&p, end, &sizelimit))
        return -1;
    remotefs_checkordinaryfileaccess_ (filename, sizelimit, s);
    return 0;
}

static int remote_action_fn_v1_stat (struct server_data *sd, CStr *s, const unsigned char *in, int inlen)
{
    const unsigned char *p, *end;
    char path[MAX_PATH_LEN];
    unsigned long long handle_just_not_there = 0;
    p = in;
    end = in + inlen;
    if (decode_str (&p, end, path, sizeof (path)))
        return -1;
    if (decode_uint (&p, end, &handle_just_not_there))
        return -1;
    remotefs_stat_ (path, handle_just_not_there, s);
    return 0;
}

static int remote_action_fn_v1_chdir (struct server_data *sd, CStr *s, const unsigned char *in, int inlen)
{
    const unsigned char *p, *end;
    char dirname[MAX_PATH_LEN];
    p = in;
    end = in + inlen;
    if (decode_str (&p, end, dirname, sizeof (dirname)))
        return -1;
    remotefs_chdir_ (dirname, s);
    return 0;
}

static int remote_action_fn_v1_realpathize (struct server_data *sd, CStr *s, const unsigned char *in, int inlen)
{
    const unsigned char *p, *end;
    char path[MAX_PATH_LEN];
    char homedir[MAX_PATH_LEN];
    p = in;
    end = in + inlen;
    if (decode_str (&p, end, path, sizeof (path)))
        return -1;
    if (decode_str (&p, end, homedir, sizeof (homedir)))
        return -1;
    remotefs_realpathize_ (path, homedir, s);
    return 0;
}

static int remote_action_fn_v1_gethomedir (struct server_data *sd, CStr *s, const unsigned char *in, int inlen)
{
    remotefs_gethomedir_ (s);
    return 0;
}

struct action_item {
    int (*action_fn) (struct server_data *sd, CStr *r, const unsigned char *in, int inlen);
};

struct action_item action_list[] = {
    { remote_action_fn_v1_notimplemented, },
    { remote_action_fn_v1_listdir, },
    { remote_action_fn_v1_readfile, },
    { remote_action_fn_v1_writefile, },
    { remote_action_fn_v1_checkordinaryfileaccess, },
    { remote_action_fn_v1_stat, },
    { remote_action_fn_v1_chdir, },
    { remote_action_fn_v1_realpathize, },
    { remote_action_fn_v1_gethomedir, },
};

static unsigned int client_count = 0L;

struct service {
    SOCKET h;
    struct iprange_list *iprange_list;
    struct client_item *client_list;
    const char *option_range;
};

static void init_service (struct service *serv, const char *listen_address, const char *option_range)
{
    int c = 0;
    memset (serv, '\0', sizeof (*serv));
    serv->h = listen_socket (listen_address, 50095);
    serv->iprange_list = iprange_parse (option_range, &c);
    serv->option_range = option_range;
    if (!serv->iprange_list) {
        fprintf (stderr, "ip range parse err: %s\n", option_range + c);
        exit (1);
    }
    if (serv->h == INVALID_SOCKET)
        exit (1);
}

struct client_item {
#define CLIENT_MAGIC    0xf0536726
    unsigned int magic;
    unsigned int id;  /* for logging only */
    const char *action;
    struct client_item *next;
    SOCKET sock;
    remotefs_sockaddr_t client_address;
    struct reader_data d;
    struct server_data sd;
    time_t last_accessed;

/* A soft kill sends a force_shutdown in the error message, then
   assumes the client will immediately close. A hard kill does a
   "shutdown(s,2); closesocket(s);" */
#define KILL_SOFT       1
#define KILL_HARD       2
    int kill;
};

static void add_client (struct service *serv)
{
    struct client_item *i;
    int found_ip;
    SOCKET sock;
#ifdef MSWIN
    BOOL yes = TRUE;
    u_long nbio = 1;
#else
    int yes = 1;
    int nbio = 1;
#endif
    socklen_t l;
    remotefs_sockaddr_t client_address;

    l = sizeof (client_address);
    sock = accept (serv->h, (struct sockaddr *) &client_address, &l);
    if (sock == INVALID_SOCKET) {
        perrorsocket ("accept fail");
        return;
    }

    found_ip = iprange_match (serv->iprange_list, remotefs_sockaddr_t_address (&client_address), remotefs_sockaddr_t_addresslen (&client_address));

    if (!found_ip) {
        char t[64];
        SHUTSOCK (sock);
        ip_to_text (remotefs_sockaddr_t_address (&client_address), remotefs_sockaddr_t_addresslen (&client_address), t);
        printf ("incoming address %s not in range %s\n", t, serv->option_range);
        return;
    }

    if (setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof (yes))) {
        SHUTSOCK (sock);
        perrorsocket ("setsockopt TCP_NODELAY\n");
        return;
    }
    if (ioctlsocket (sock, FIONBIO, &nbio))  {
        SHUTSOCK (sock);
        perrorsocket ("ioctl FIONBIO\n");
        return;
    }

    printf ("connection established\n");

    client_count++;

    i = (struct client_item *) malloc (sizeof (struct client_item));
    memset (i, '\0', sizeof (*i));
    i->magic  = CLIENT_MAGIC;
    i->id = client_count;
    i->sock = sock;
    i->client_address = client_address;
    i->d.sock = &i->sock;
    i->sd.reader_data = &i->d;
    time (&i->last_accessed);

    i->next = serv->client_list;
    serv->client_list = i;

    printf ("adding %u\n", i->id);
}

static void process_client (struct client_item *i)
{
    CStr r;
    unsigned char *p = NULL;
    unsigned long long msglen;
    unsigned long version, action, magic;
    struct cooledit_remote_msg_header m;
    struct cooledit_remote_msg_ack ack;

    memset (&r, '\0', sizeof (r));

#define ERR(s,m)  \
    do { \
        printf ("%d: Error: %s, %s, %s\n", i->id, (s), (m), strerrorsocket ()); \
        goto errout; \
    } while (0)

    if (reader (&i->d, &m, sizeof (m)))
        ERR ("reading header", "");

    decode_msg_header (&m, &msglen, &version, &action, &magic);

    if (magic != FILE_PROTO_MAGIC)
        ERR ("bad magic", "");

    if (msglen > 1024 * 1024)
        ERR ("msglen > 1M", "");

/* we immediately ack so that the client can timeout if it does not
   recieve this ack. the subsequent response could take a long time
   if the filesystem is slow, but this response should always come
   within an order of the ping time. the case of the client not getting
   this response means it can choose to try reconnect rather than waiste
   the users time. See (*1*): */
    memset (&ack, '\0', sizeof (ack));
    encode_msg_ack (&ack, 0, MSG_VERSION);

    if (writer (i->sock, &ack, sizeof (ack)))
        ERR ("writing ack", i->action);

    p = (unsigned char *) malloc (msglen);

    if (reader (&i->d, p, msglen))
        ERR ("reading request msg", "");

    memset (&r, '\0', sizeof (r));

    if (action <= REMOTEFS_ACTION_NOTIMPLEMENTED || action >= sizeof (action_list) / sizeof (action_list[0]))
        action = REMOTEFS_ACTION_NOTIMPLEMENTED;
    i->action = action_descr[action];
    printf ("%u: %s: \n", i->id, i->action);
    if ((*action_list[action].action_fn) (&i->sd, &r, p, msglen)) {
        i->kill = KILL_SOFT;
        printf ("Error: executing action, %s %d\n", i->action, r.len);
        if (!r.len)
            goto errout; /* and kill hard */

/* if we get here it means the action function wants to send a response but
   then also immediately close the connection. We don't immediately send a
   shutdown because stupid-windows takes a shutdown() to mean that all data
   in the send queue must be discarded and stupid-windows also has no API
   call to wait for the output of the send queue to be written to the network
   nor any API call to inspect the send queue. Instead we wait for the client
   to close. The response should include a force_shutdown=1 */

    }

    free (p);
    p = NULL;

    encode_msg_header (&m, r.len, MSG_VERSION, action, FILE_PROTO_MAGIC);
    if (writer (i->sock, &m, sizeof (m)))
        ERR ("writing response header", i->action);

    if (writer (i->sock, r.data, r.len))
        ERR ("writing response", i->action);

    free (r.data);
    r.data = NULL;
    return;

  errout:
    if (p)
        free (p);
    if (r.data)
        free (r.data);
    i->kill = KILL_HARD;
    return;
}

static void run_service (struct service *serv)
{
    struct client_item *i, **j;
    int n = 0;
    int r;
    fd_set rd;
    struct timeval tv;
    FD_ZERO (&rd);

    for (i = serv->client_list; i; i = i->next) {
        assert (i->magic == CLIENT_MAGIC);
        FD_SET (i->sock, &rd);
        n = MAX (n, i->sock);
    }
    FD_SET (serv->h, &rd);
    n = MAX (n, serv->h);

    tv.tv_sec = 0;
    tv.tv_usec = 1000000;
    r = select (n + 1, &rd, NULL, NULL, &tv);
    if (!r) {
        FD_ZERO (&rd);
    } else if (r == SOCKET_ERROR && (ERROR_EINTR() || ERROR_EAGAIN())) {
        return;
    } else if (r == SOCKET_ERROR) {
        perrorsocket ("select");
        exit (1);
    }

    if (FD_ISSET (serv->h, &rd))
        add_client (serv);

    for (i = serv->client_list; i; i = i->next) {
        assert (i->magic == CLIENT_MAGIC);
        if (FD_ISSET (i->sock, &rd)) {
            if (i->kill) {
                i->kill = KILL_HARD;
            } else {
                process_client (i);
                time (&i->last_accessed);
            }
        }
    }

    for (j = &serv->client_list;;) {
        time_t now;
        time (&now);
        i = *j;
        if (!i)
            break;
        assert (i->magic == CLIENT_MAGIC);

        if (now > i->last_accessed + 25 /* for firewalls that are 30s timeout */) {
            struct cooledit_remote_msg_ack ack;
            memset (&ack, '\0', sizeof (ack));
            encode_msg_ack (&ack, RFSERR_SERVER_CLOSED_IDLE_CLIENT, MSG_VERSION);
            writer (i->sock, &ack, sizeof (ack));
            i->kill = KILL_HARD;
        }

        if (i->kill == KILL_HARD) {
            struct client_item *next;
            SHUTSOCK (i->sock);
            next = i->next;
            i->magic = 0;
            printf ("removing %u\n", i->id);
            free (i);
            *j = next;
        } else {
            j = &i->next;
        }
    }
}


void remotefs_serverize (const char *listen_address, const char *option_range)
{
    struct service serv;

#ifdef MSWIN
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD (2, 2);

    err = WSAStartup (wVersionRequested, &wsaData);
    if (err != 0) {
        printf ("WSAStartup failed with error: %d\n", err);
        exit (1);
    }
#else
    signal (SIGPIPE, SIG_IGN);
#endif

    init_random ();

    init_service (&serv, listen_address, option_range);

    printf ("running\n");

    for (;;) {
        run_service (&serv);
    }
}














#ifndef REMOTEFS_DOTEST

#ifdef STANDALONE

#ifdef MSWIN
INT WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
#else
int main (int argc, char **argv)
#endif
{
#ifdef MSWIN
    wchar_t **argv;
    int argc;

    argv = CommandLineToArgvW (GetCommandLineW (), &argc);
    if (NULL == argv) {
        wprintf (L"CommandLineToArgvW failed\n");
        return 0;
    }
#endif

    (void) strerrorsocket;

    if (argc != 3) {
        printf ("Usage: remotefs <listenaddress> <iprange>\n");
        exit (1);
    }

    remotefs_serverize (wchar_to_char (argv[1]), wchar_to_char (argv[2]));

    return 0;
}

#endif

#endif



#ifdef REMOTEFS_DOTEST


static void test_sint (long long J, int N)
{
    unsigned char buf[256];
    unsigned char *p;
    const unsigned char *q, *end;
    int r;
    long long si = 0x0eadbeefdeadbeef;

    r = encode_sint (NULL, J);
    assert (r == N);
    p = buf;
    r = encode_sint (&p, J);
    assert (r == N);
    q = buf;
    end = buf + r;
    r = decode_sint (&q, end, &si);
    assert (!r);
    assert (si == J);
}

#define test_float(a,b,c)       test_float__(__LINE__,a,b,c)

static void test_float__ (int line, double J, int N, int assert_equality)
{
    unsigned char buf[256];
    unsigned char *p;
    const unsigned char *q, *end;
    int r, s;
    double si = 0.0;

    r = encode_float (NULL, J);
    if (N != -1) {
        if (r != N) {
            fprintf (stderr, "line %d: %d vs %d\n", line, r, N);
            exit (1);
        }
    }
    p = buf;
    s = encode_float (&p, J);
    if (s != r) {
        fprintf (stderr, "line %d: %d vs %d\n", line, s, r);
        exit (1);
    }
    q = buf;
    end = buf + r;
    r = decode_float (&q, end, &si);
    assert (!r);
    if (assert_equality != 0)
        if (si != J) {
            fprintf (stderr, "line %d: %g vs %g\n", line, si, J);
            exit (1);
        }
}

#define test_floatspecial(a,b,c)       test_floatspecial__(__LINE__,a,b,c)

static void test_floatspecial__ (int line, double J, int N, unsigned long long nan)
{
    unsigned char buf[256];
    unsigned char *p;
    const unsigned char *q, *end;
    int r, s;
    union float_conv f;

    r = encode_float (NULL, J);
    if (N != -1) {
        if (r != N) {
            fprintf (stderr, "line %d: %d vs %d\n", line, r, N);
            exit (1);
        }
    }
    p = buf;
    s = encode_float (&p, J);
    if (s != r) {
        fprintf (stderr, "line %d: %d vs %d\n", line, s, r);
        exit (1);
    }
    q = buf;
    end = buf + r;
    f.l = 0;
    r = decode_float (&q, end, &f.f);
    assert (!r);
    if (f.l != nan) {
        fprintf (stderr, "line %d: %llx vs %llx\n", line, f.l, nan);
        exit (1);
    }
}

#include "math.h"

int main (int argc, char **argv)
{
    unsigned char buf[256];
    unsigned char *p;
    const unsigned char *q, *end;
    int r, i;
    unsigned long long v = 0xdeadbeefdeadbeef;
    CStr y;
    double w;
    union float_conv f;

    memset (buf, '\0', sizeof (buf));

/* arbitrary precision decoding from non-native remote host: */
/*  (0x20<<(19*7))*(2^0) */
    buf[0] = 0x80;
    buf[20] = 0xA0;
    q = buf;
    decode_float (&q, buf + 21, &f.f);
    assert (f.f == 3.4844914372704099e+41);

/*  (-0x20<<(19*7))*(2^0) */
    buf[0] = 0x80;
    buf[20] = 0xE0;
    q = buf;
    decode_float (&q, buf + 21, &f.f);
    assert (f.f == -3.4844914372704099e+41);

/*  (0x20<<(19*7))*(2^10) */
    buf[0] = 0x8A;
    buf[20] = 0xA0;
    q = buf;
    decode_float (&q, buf + 21, &f.f);
    assert (f.f == 3.5681192317648997e+44);

/*  (-0x20<<(19*7))*(2^10) */
    buf[0] = 0x8A;
    buf[20] = 0xE0;
    q = buf;
    decode_float (&q, buf + 21, &f.f);
    assert (f.f == -3.5681192317648997e+44);

/*  (0x20<<(19*7))*(2^-10) */
    buf[0] = 0xCA;
    buf[20] = 0xA0;
    q = buf;
    decode_float (&q, buf + 21, &f.f);
    assert (f.f == 3.4028236692093846e+38);

/*  (-0x20<<(19*7))*(2^-10) */
    buf[0] = 0xCA;
    buf[20] = 0xE0;
    q = buf;
    decode_float (&q, buf + 21, &f.f);
    assert (f.f == -3.4028236692093846e+38);


/* extremes */
    test_float(0.0, 2, 1);
    test_float(1.7976931348623157e308, 10, 1);
    test_float(2.2250738585072014e-308, 3, 1);

/* general */
    for (f.f = 1.0; f.f < 1.7976931348623157e308; f.f *= 1.3)
        test_float(f.f, -1, 1);
    for (f.f = 1.0; f.f < 4.9406564584124654e-324; f.f /= 1.3)
        test_float(f.f, -1, 1);
    w = 1.0;
    for (i = 0; i < 100; i++) {
        f.f = 1.0 + w;
        test_float(f.f, -1, 1);
        w /= 2.0;
    }
    w = 1.0;
    for (i = 0; i < 100; i++) {
        f.f = w + 1.0;
        test_float(f.f, -1, 1);
        w *= 2.0;
    }

    f.f = SNAN;

/* Alternate forms of NaN */
    f.l = 0x7FF4000000000000ULL;
    test_floatspecial (f.f, 4, 0x7FF4000000000000ULL);
    f.l = 0x7FFFFFFFFFFFFFFFULL;
    test_floatspecial (f.f, 4, 0x7FFFFFFFFFFFFFFFULL);
    f.l = 0x7FFFFFFFFFFFFFFEULL;
    test_floatspecial (f.f, 11, 0x7FFFFFFFFFFFFFFEULL);
    f.l = 0x7FFFFFFFFFFFFFFDULL;
    test_floatspecial (f.f, 11, 0x7FFFFFFFFFFFFFFDULL);
    f.l = 0x7FFFFFFFFFFFFFF8ULL;
    test_floatspecial (f.f, 11, 0x7FFFFFFFFFFFFFF8ULL);
    f.l = 0x7FFFFFFFFFFFFFFAULL;
    test_floatspecial (f.f, 11, 0x7FFFFFFFFFFFFFFAULL);
    f.l = 0x7FF07FFFFFFFFFEAULL;
    test_floatspecial (f.f, 10, 0x7FF07FFFFFFFFFEAULL);
    f.l = 0x7FF0000000000002ULL;
    test_floatspecial (f.f, 4, 0x7FF0000000000002ULL);
    f.l = 0x7FF8000000000002ULL;
    test_floatspecial (f.f, 4, 0x7FF8000000000002ULL);
    f.l = 0xFFFFFFFFFFFFFFFFULL;
    test_floatspecial (f.f, 4, 0xFFFFFFFFFFFFFFFFULL);
    f.l = 0xFFFFFFFFFFFFFFFEULL;
    test_floatspecial (f.f, 11, 0xFFFFFFFFFFFFFFFEULL);
    f.l = 0xFFFFFFFFFFFFFFFDULL;
    test_floatspecial (f.f, 11, 0xFFFFFFFFFFFFFFFDULL);
    f.l = 0xFFFFFFFFFFFFFFF8ULL;
    test_floatspecial (f.f, 11, 0xFFFFFFFFFFFFFFF8ULL);
    f.l = 0xFFFFFFFFFFFFFFF9ULL;
    test_floatspecial (f.f, 11, 0xFFFFFFFFFFFFFFF9ULL);
    f.l = 0xFFF07FFFFFFFFFEAULL;
    test_floatspecial (f.f, 10, 0xFFF07FFFFFFFFFEAULL);
    f.l = 0xFFF0000000000002ULL;
    test_floatspecial (f.f, 4, 0xFFF0000000000002ULL);
    f.l = 0xFFF8000000000002ULL;
    test_floatspecial (f.f, 4, 0xFFF8000000000002ULL);

/* oddities */
    f.l = 0x0000000000000001ULL;
    test_float (f.f, 3, 1);
    f.l = 0x8000000000000001ULL;
    test_float (f.f, 3, 1);
    f.l = 0x7FE0000000000001ULL;
    test_float (f.f, 10, 1);
    f.l = 0x7FEFFFFFFFFFFFFFULL;
    test_float (f.f, 10, 1);
    f.l = 0x7FE0000000000000ULL;
    test_float (f.f, 3, 1);
    f.l = 0x7FE8000000000000ULL;
    test_float (f.f, 3, 1);
    f.l = 0x7FEFFFFFFFFFFFFEULL;
    test_float (f.f, 10, 1);
    f.l = 0x0000000000000000ULL;
    test_float (f.f, 2, 1);
    f.l = 0x8000000000000000ULL;
    test_floatspecial (f.f, 4, 0x8000000000000000ULL);
    f.l = 0x7FF0000000000000ULL;
    test_floatspecial (f.f, 4, 0x7FF0000000000000ULL);
    f.l = 0xFFF0000000000000ULL;
    test_floatspecial (f.f, 4, 0xFFF0000000000000ULL);
    f.l = 0x7FF0000000000001ULL;
    test_floatspecial (f.f, 4, 0x7FF0000000000001ULL);
    f.l = 0x7FF8000000000001ULL;
    test_floatspecial (f.f, 4, 0x7FF8000000000001ULL);
    f.l = 0x7FFFFFFFFFFFFFFFULL;
    test_floatspecial (f.f, 4, 0x7FFFFFFFFFFFFFFFULL);

/* sub normals */
    test_float(2.2250738585072011e-308, 10, 1); /* largest sub normal */
    test_float(2.2250738585072009e-308 - 4.9406564584124654e-324, 10, 1);
    test_float(2.2250738585072009e-308, 10, 1);
    test_float(4.9406564584124654e-324*2.0, 3, 1);
    test_float(4.9406564584124654e-324, 3, 1);  /* smallest sub normal */

/* negative sub normals */
    test_float(-2.2250738585072011e-308, 10, 1);
    test_float(-(2.2250738585072009e-308 - 4.9406564584124654e-324), 10, 1);
    test_float(-2.2250738585072009e-308 - 4.9406564584124654e-324, 3, 1);
    test_float(-2.2250738585072009e-308, 10, 1);
    test_float(-4.9406564584124654e-324*2.0, 3, 1);
    test_float(-4.9406564584124654e-324, 3, 1);

/* normals */
    test_float(1.0,2, 1);
    test_float(2.0,2, 1);
    test_float(3.0,2, 1);
    test_float(4.0,2, 1);
    test_float(63.0,2, 1);
    test_float(64.0,2, 1);
    test_float(65.0,3, 1);
    test_float(127.0,3, 1);
    test_float(1024,2, 1);
    test_float(8191,3, 1);
    test_float(8193,4, 1);
    test_float(65536,2, 1);
    test_float(4611686018427387904.0,2, 1);
    test_float(3.14159265358979323846,9, 1);
    test_float(3.14159265358979323846e-99,10, 1);
    test_float(3.14159265358979323846e99,10, 1);

/* negative normals */
    test_float(-1.0,2, 1);
    test_float(-2.0,2, 1);
    test_float(-3.0,2, 1);
    test_float(-4.0,2, 1);
    test_float(-63.0,2, 1);
    test_float(-65.0,3, 1);
    test_float(-127.0,3, 1);
    test_float(-1024,2, 1);
    test_float(-8191,3, 1);
    test_float(-8193,4, 1);
    test_float(-65536,2, 1);
    test_float(-4611686018427387904.0,2, 1);
    test_float(-3.14159265358979323846,9, 1);
    test_float(-3.14159265358979323846e-99,10, 1);
    test_float(-3.14159265358979323846e99,10, 1);

    test_sint(1,1);
    test_sint(-1,1);
    test_sint(63,1);
    test_sint(-63,1);
    test_sint(64,2);
    test_sint(-64,2);
    test_sint(8191,2);
    test_sint(-8191,2);
    test_sint(1048575,3);
    test_sint(-1048575,3);
    test_sint(0x5fd207fd99fae816ULL,10);
    test_sint(-0x5fd207fd99fae816ULL,10);

    r = encode_str (NULL, "", 0);
    assert (r == 1);

    r = encode_str (NULL, "x", 1);
    assert (r == 2);


    p = buf;
    r = encode_str (&p, "", 0);
    assert (r == 1);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_cstr (&q, end, &y);
    assert (!r);
    assert (y.len == 0);

    p = buf;
    r = encode_str (&p, "x", 1);
    assert (r == 2);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_cstr (&q, end, &y);
    assert (!r);
    assert (y.len == 1);
    assert (y.data[0] == 'x');

    p = buf;
    r = encode_str (&p, "xy", 2);
    assert (r == 3);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_cstr (&q, end, &y);
    assert (!r);
    assert (y.len == 2);
    assert (y.data[0] == 'x');
    assert (y.data[1] == 'y');




    r = encode_uint (NULL, 0ULL);
    assert (r == 1);

    r = encode_uint (NULL, 0x7FULL);
    assert (r == 1);

    r = encode_uint (NULL, 0x80ULL);
    assert (r == 2);

    r = encode_uint (NULL, 0x3FFFULL);
    assert (r == 2);

    r = encode_uint (NULL, 0x4000ULL);
    assert (r == 3);


    p = buf;
    r = encode_uint (&p, 0ULL);
    assert (r == 1);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_uint (&q, end, &v);
    assert (!r);
    assert (v == 0ULL);

    p = buf;
    r = encode_uint (&p, 0x7FULL);
    assert (r == 1);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_uint (&q, end, &v);
    assert (!r);
    assert (v == 0x7FULL);

    p = buf;
    r = encode_uint (&p, 0x80ULL);
    assert (r == 2);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_uint (&q, end, &v);
    assert (!r);
    assert (v == 0x80ULL);

    p = buf;
    r = encode_uint (&p, 0x3FFFULL);
    assert (r == 2);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_uint (&q, end, &v);
    assert (!r);
    assert (v == 0x3FFFULL);

    p = buf;
    r = encode_uint (&p, 0x4000ULL);
    assert (r == 3);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_uint (&q, end, &v);
    assert (!r);
    assert (v == 0x4000ULL);

    p = buf;
    r = encode_uint (&p, 13692116463583105024ULL);
    assert (r == 10);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_uint (&q, end, &v);
    assert (!r);
    assert (v == 13692116463583105024ULL);

    p = buf;
    r = encode_uint (&p, 0xdfd207fd99fae816ULL);
    assert (r == 10);
    assert (p == buf + r);
    q = buf;
    end = buf + r;
    r = decode_uint (&q, end, &v);
    assert (!r);
    assert (v == 0xdfd207fd99fae816ULL);

    printf ("Success\n");
}

#endif






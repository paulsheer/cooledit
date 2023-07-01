/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */




#ifndef REMOTEFS_H
#define REMOTEFS_H


struct simple_string;

#ifndef CSTR_DEFINED
#define CSTR_DEFINED
typedef struct simple_string CStr;
#endif


#define OS_TYPE_POSIX           0
#define OS_SUBTYPE_LINUX        0
#define OS_TYPE_WINDOWS         1
#define OS_SUBTYPE_WINDOWS      0



enum remotefs_error_code {
    RFSERR_SUCCESS = 0,                         /* 0 */
    RFSERR_UNIX_ERROR_WITHOUT_TRANSLATION = 1,  /* 1 */
    RFSERR_MSWIN_ERROR_WITHOUT_TRANSLATION,     /* 2 */
    RFSERR_UNIMPLEMENTED_FUNCTION,              /* 3 */
    RFSERR_BAD_VERSION,                         /* 4 */
    RFSERR_SERVER_CLOSED_IDLE_CLIENT,           /* 5 */
    RFSERR_EARLY_TERMINATE_FROM_WRITE_FILE,     /* 6 */
    RFSERR_OTHER_ERROR,                         /* 7 */
    RFSERR_ENDOFFILE,                           /* 8 */
    RFSERR_PATHNAME_TOO_LONG,                   /* 9 */
    RFSERR_NON_CRYPTO_OP_ATTEMPTED,             /* 10 */
    RFSERR_SERVER_CLOSED_SHELL_DIED,            /* 11 */
    RFSERR_LAST_INTERNAL_ERROR,                 /* 12 */

/* The combined errors from: opengroup.org, Linux, FreeBSD, Solaris, HP-UX,
   and Windows _sys_errlist are listed below.  This excludes the Windows WSA
   error codes since we cannot report network errors to the client. The list
   means we report every kind of error from every operating system related
   to file access. */
 
    RFSERR_E2BIG = 50,
    RFSERR_EACCES,
    RFSERR_EADDRINUSE,
    RFSERR_EADDRNOTAVAIL,
    RFSERR_EADV,
    RFSERR_EAFNOSUPPORT,
    RFSERR_EAGAIN,
    RFSERR_EALREADY,
    RFSERR_EAUTH,
    RFSERR_EBADE,
    RFSERR_EBADF,
    RFSERR_EBADFD,
    RFSERR_EBADMSG,
    RFSERR_EBADR,
    RFSERR_EBADRPC,
    RFSERR_EBADRQC,
    RFSERR_EBADSLT,
    RFSERR_EBFONT,
    RFSERR_EBUSY,
    RFSERR_ECANCELED,
    RFSERR_ECAPMODE,
    RFSERR_ECHILD,
    RFSERR_ECHRNG,
    RFSERR_ECOMM,
    RFSERR_ECONNABORTED,
    RFSERR_ECONNREFUSED,
    RFSERR_ECONNRESET,
    RFSERR_EDEADLK,
    RFSERR_EDESTADDRREQ,
    RFSERR_EDOM,
    RFSERR_EDOOFUS,
    RFSERR_EDOTDOT,
    RFSERR_EDQUOT,
    RFSERR_EEXIST,
    RFSERR_EFAULT,
    RFSERR_EFBIG,
    RFSERR_EFTYPE,
    RFSERR_EHOSTDOWN,
    RFSERR_EHOSTUNREACH,
    RFSERR_EHWPOISON,
    RFSERR_EIDRM,
    RFSERR_EILSEQ,
    RFSERR_EINPROGRESS,
    RFSERR_EINTR,
    RFSERR_EINVAL,
    RFSERR_EIO,
    RFSERR_EISCONN,
    RFSERR_EISDIR,
    RFSERR_EISNAM,
    RFSERR_EKEYEXPIRED,
    RFSERR_EKEYREJECTED,
    RFSERR_EKEYREVOKED,
    RFSERR_EL2HLT,
    RFSERR_EL2NSYNC,
    RFSERR_EL3HLT,
    RFSERR_ELAST,
    RFSERR_ELIBACC,
    RFSERR_ELIBBAD,
    RFSERR_ELIBEXEC,
    RFSERR_ELIBMAX,
    RFSERR_ELIBSCN,
    RFSERR_ELNRNG,
    RFSERR_ELOOP,
    RFSERR_EMEDIUMTYPE,
    RFSERR_EMFILE,
    RFSERR_EMGSIZE,
    RFSERR_EMLINK,
    RFSERR_EMSGSIZE,
    RFSERR_EMULTIHOP,
    RFSERR_ENAMETOOLONG,
    RFSERR_ENAVAIL,
    RFSERR_ENEEDAUTH,
    RFSERR_ENETDOWN,
    RFSERR_ENETRESET,
    RFSERR_ENETUNREACH,
    RFSERR_ENFILE,
    RFSERR_ENOANO,
    RFSERR_ENOATTR,
    RFSERR_ENOBUFS,
    RFSERR_ENOCSI,
    RFSERR_ENODATA,
    RFSERR_ENODEV,
    RFSERR_ENOENT,
    RFSERR_ENOEXEC,
    RFSERR_ENOKEY,
    RFSERR_ENOLCK,
    RFSERR_ENOLINK,
    RFSERR_ENOMEDIUM,
    RFSERR_ENOMEM,
    RFSERR_ENOMSG,
    RFSERR_ENONET,
    RFSERR_ENOPKG,
    RFSERR_ENOPROTOOPT,
    RFSERR_ENOSPC,
    RFSERR_ENOSR,
    RFSERR_ENOSTR,
    RFSERR_ENOSYM,
    RFSERR_ENOSYS,
    RFSERR_ENOTBLK,
    RFSERR_ENOTCAPABLE,
    RFSERR_ENOTCONN,
    RFSERR_ENOTDIR,
    RFSERR_ENOTEMPTY,
    RFSERR_ENOTNAM,
    RFSERR_ENOTRECOVERABLE,
    RFSERR_ENOTSOCK,
    RFSERR_ENOTSUP,
    RFSERR_ENOTTY,
    RFSERR_ENOTUNIQ,
    RFSERR_ENXIO,
    RFSERR_EOVERFLOW,
    RFSERR_EOWNERDEAD,
    RFSERR_EPERM,
    RFSERR_EPFNOSUPPORT,
    RFSERR_EPIPE,
    RFSERR_EPROCLIM,
    RFSERR_EPROCUNAVAIL,
    RFSERR_EPROGMISMATCH,
    RFSERR_EPROGUNAVAIL,
    RFSERR_EPROTO,
    RFSERR_EPROTONOSUPPORT,
    RFSERR_EPROTOTYPE,
    RFSERR_ERANGE,
    RFSERR_EREMCHG,
    RFSERR_EREMOTE,
    RFSERR_EREMOTEIO,
    RFSERR_ERESTART,
    RFSERR_ERFKILL,
    RFSERR_EROFS,
    RFSERR_ERPCMISMATCH,
    RFSERR_ESHUTDOWN,
    RFSERR_ESOCKTNOSUPPORT,
    RFSERR_ESPIPE,
    RFSERR_ESRCH,
    RFSERR_ESRMNT,
    RFSERR_ESTALE,
    RFSERR_ESTART,
    RFSERR_ESTRPIPE,
    RFSERR_ETIME,
    RFSERR_ETIMEDOUT,
    RFSERR_ETOOMANYREFS,
    RFSERR_ETXTBSY,
    RFSERR_EUCLEAN,
    RFSERR_EUNATCH,
    RFSERR_EUSERS,
    RFSERR_EXDEV,
    RFSERR_EXFULL,
    RFSERR_STRUNCATE,

    RFSERR_MSWIN_ERROR_INVALID_FUNCTION,
    RFSERR_MSWIN_ERROR_FILE_NOT_FOUND,
    RFSERR_MSWIN_ERROR_PATH_NOT_FOUND,
    RFSERR_MSWIN_ERROR_TOO_MANY_OPEN_FILES,
    RFSERR_MSWIN_ERROR_ACCESS_DENIED,
    RFSERR_MSWIN_ERROR_INVALID_HANDLE,
    RFSERR_MSWIN_ERROR_ARENA_TRASHED,
    RFSERR_MSWIN_ERROR_NOT_ENOUGH_MEMORY,
    RFSERR_MSWIN_ERROR_INVALID_BLOCK,
    RFSERR_MSWIN_ERROR_BAD_ENVIRONMENT,
    RFSERR_MSWIN_ERROR_BAD_FORMAT,
    RFSERR_MSWIN_ERROR_INVALID_ACCESS,
    RFSERR_MSWIN_ERROR_INVALID_DATA,
    RFSERR_MSWIN_ERROR_INVALID_DRIVE,
    RFSERR_MSWIN_ERROR_CURRENT_DIRECTORY,
    RFSERR_MSWIN_ERROR_NOT_SAME_DEVICE,
    RFSERR_MSWIN_ERROR_NO_MORE_FILES,
    RFSERR_MSWIN_ERROR_WRITE_PROTECT,
    RFSERR_MSWIN_ERROR_BAD_UNIT,
    RFSERR_MSWIN_ERROR_NOT_READY,
    RFSERR_MSWIN_ERROR_BAD_COMMAND,
    RFSERR_MSWIN_ERROR_CRC,
    RFSERR_MSWIN_ERROR_BAD_LENGTH,
    RFSERR_MSWIN_ERROR_SEEK,
    RFSERR_MSWIN_ERROR_NOT_DOS_DISK,
    RFSERR_MSWIN_ERROR_SECTOR_NOT_FOUND,
    RFSERR_MSWIN_ERROR_OUT_OF_PAPER,
    RFSERR_MSWIN_ERROR_WRITE_FAULT,
    RFSERR_MSWIN_ERROR_READ_FAULT,
    RFSERR_MSWIN_ERROR_GEN_FAILURE,
    RFSERR_MSWIN_ERROR_SHARING_VIOLATION,
    RFSERR_MSWIN_ERROR_LOCK_VIOLATION,
    RFSERR_MSWIN_ERROR_WRONG_DISK,
    RFSERR_MSWIN_ERROR_FCB_UNAVAILABLE,
    RFSERR_MSWIN_ERROR_SHARING_BUFFER_EXCEEDED,
    RFSERR_MSWIN_ERROR_NOT_SUPPORTED,
    RFSERR_MSWIN_ERROR_FILE_EXISTS,
    RFSERR_MSWIN_ERROR_DUP_FCB,
    RFSERR_MSWIN_ERROR_CANNOT_MAKE,
    RFSERR_MSWIN_ERROR_FAIL_I24,
    RFSERR_MSWIN_ERROR_OUT_OF_STRUCTURES,
    RFSERR_MSWIN_ERROR_ALREADY_ASSIGNED,
    RFSERR_MSWIN_ERROR_INVALID_PASSWORD,
    RFSERR_MSWIN_ERROR_INVALID_PARAMETER,
    RFSERR_MSWIN_ERROR_NET_WRITE_FAULT,
    RFSERR_MSWIN_ERROR_NO_PROC_SLOTS,
    RFSERR_MSWIN_ERROR_NOT_FROZEN,
    RFSERR_MSWIN_ERROR_NO_ITEMS,
    RFSERR_MSWIN_ERROR_INTERRUPT,
    RFSERR_MSWIN_ERROR_TOO_MANY_SEMAPHORES,
    RFSERR_MSWIN_ERROR_EXCL_SEM_ALREADY_OWNED,
    RFSERR_MSWIN_ERROR_SEM_IS_SET,
    RFSERR_MSWIN_ERROR_TOO_MANY_SEM_REQUESTS,
    RFSERR_MSWIN_ERROR_INVALID_AT_INTERRUPT_TIME,
    RFSERR_MSWIN_ERROR_SEM_OWNER_DIED,
    RFSERR_MSWIN_ERROR_SEM_USER_LIMIT,
    RFSERR_MSWIN_ERROR_DISK_CHANGE,
    RFSERR_MSWIN_ERROR_DRIVE_LOCKED,
    RFSERR_MSWIN_ERROR_BROKEN_PIPE,
    RFSERR_MSWIN_ERROR_OPEN_FAILED,
    RFSERR_MSWIN_ERROR_BUFFER_OVERFLOW,
    RFSERR_MSWIN_ERROR_DISK_FULL,
    RFSERR_MSWIN_ERROR_NO_MORE_SEARCH_HANDLES,
    RFSERR_MSWIN_ERROR_INVALID_TARGET_HANDLE,
    RFSERR_MSWIN_ERROR_PROTECTION_VIOLATION,
    RFSERR_MSWIN_ERROR_VIOKBD_REQUEST,
    RFSERR_MSWIN_ERROR_INVALID_CATEGORY,
    RFSERR_MSWIN_ERROR_INVALID_VERIFY_SWITCH,
    RFSERR_MSWIN_ERROR_BAD_DRIVER_LEVEL,
    RFSERR_MSWIN_ERROR_CALL_NOT_IMPLEMENTED,
    RFSERR_MSWIN_ERROR_SEM_TIMEOUT,
    RFSERR_MSWIN_ERROR_INSUFFICIENT_BUFFER,
    RFSERR_MSWIN_ERROR_INVALID_NAME,
    RFSERR_MSWIN_ERROR_INVALID_LEVEL,
    RFSERR_MSWIN_ERROR_NO_VOLUME_LABEL,
    RFSERR_MSWIN_ERROR_MOD_NOT_FOUND,
    RFSERR_MSWIN_ERROR_PROC_NOT_FOUND,
    RFSERR_MSWIN_ERROR_WAIT_NO_CHILDREN,
    RFSERR_MSWIN_ERROR_CHILD_NOT_COMPLETE,
    RFSERR_MSWIN_ERROR_DIRECT_ACCESS_HANDLE,
    RFSERR_MSWIN_ERROR_NEGATIVE_SEEK,
    RFSERR_MSWIN_ERROR_SEEK_ON_DEVICE,
    RFSERR_MSWIN_ERROR_IS_JOIN_TARGET,
    RFSERR_MSWIN_ERROR_IS_JOINED,
    RFSERR_MSWIN_ERROR_IS_SUBSTED,
    RFSERR_MSWIN_ERROR_NOT_JOINED,
    RFSERR_MSWIN_ERROR_NOT_SUBSTED,
    RFSERR_MSWIN_ERROR_JOIN_TO_JOIN,
    RFSERR_MSWIN_ERROR_SUBST_TO_SUBST,
    RFSERR_MSWIN_ERROR_JOIN_TO_SUBST,
    RFSERR_MSWIN_ERROR_SUBST_TO_JOIN,
    RFSERR_MSWIN_ERROR_BUSY_DRIVE,
    RFSERR_MSWIN_ERROR_SAME_DRIVE,
    RFSERR_MSWIN_ERROR_DIR_NOT_ROOT,
    RFSERR_MSWIN_ERROR_DIR_NOT_EMPTY,
    RFSERR_MSWIN_ERROR_IS_SUBST_PATH,
    RFSERR_MSWIN_ERROR_IS_JOIN_PATH,
    RFSERR_MSWIN_ERROR_PATH_BUSY,
    RFSERR_MSWIN_ERROR_IS_SUBST_TARGET,
    RFSERR_MSWIN_ERROR_SYSTEM_TRACE,
    RFSERR_MSWIN_ERROR_INVALID_EVENT_COUNT,
    RFSERR_MSWIN_ERROR_TOO_MANY_MUXWAITERS,
    RFSERR_MSWIN_ERROR_INVALID_LIST_FORMAT,
    RFSERR_MSWIN_ERROR_LABEL_TOO_LONG,
    RFSERR_MSWIN_ERROR_TOO_MANY_TCBS,
    RFSERR_MSWIN_ERROR_SIGNAL_REFUSED,
    RFSERR_MSWIN_ERROR_DISCARDED,
    RFSERR_MSWIN_ERROR_NOT_LOCKED,
    RFSERR_MSWIN_ERROR_BAD_THREADID_ADDR,
    RFSERR_MSWIN_ERROR_BAD_ARGUMENTS,
    RFSERR_MSWIN_ERROR_BAD_PATHNAME,
    RFSERR_MSWIN_ERROR_SIGNAL_PENDING,
    RFSERR_MSWIN_ERROR_UNCERTAIN_MEDIA,
    RFSERR_MSWIN_ERROR_MAX_THRDS_REACHED,
    RFSERR_MSWIN_ERROR_MONITORS_NOT_SUPPORTED,
    RFSERR_MSWIN_ERROR_INVALID_SEGMENT_NUMBER,
    RFSERR_MSWIN_ERROR_INVALID_CALLGATE,
    RFSERR_MSWIN_ERROR_INVALID_ORDINAL,
    RFSERR_MSWIN_ERROR_ALREADY_EXISTS,
    RFSERR_MSWIN_ERROR_NO_CHILD_PROCESS,
    RFSERR_MSWIN_ERROR_CHILD_ALIVE_NOWAIT,
    RFSERR_MSWIN_ERROR_INVALID_FLAG_NUMBER,
    RFSERR_MSWIN_ERROR_SEM_NOT_FOUND,
    RFSERR_MSWIN_ERROR_INVALID_STARTING_CODESEG,
    RFSERR_MSWIN_ERROR_INVALID_STACKSEG,
    RFSERR_MSWIN_ERROR_INVALID_MODULETYPE,
    RFSERR_MSWIN_ERROR_INVALID_EXE_SIGNATURE,
    RFSERR_MSWIN_ERROR_EXE_MARKED_INVALID,
    RFSERR_MSWIN_ERROR_BAD_EXE_FORMAT,
    RFSERR_MSWIN_ERROR_ITERATED_DATA_EXCEEDS_64k,
    RFSERR_MSWIN_ERROR_INVALID_MINALLOCSIZE,
    RFSERR_MSWIN_ERROR_DYNLINK_FROM_INVALID_RING,
    RFSERR_MSWIN_ERROR_IOPL_NOT_ENABLED,
    RFSERR_MSWIN_ERROR_INVALID_SEGDPL,
    RFSERR_MSWIN_ERROR_AUTODATASEG_EXCEEDS_64k,
    RFSERR_MSWIN_ERROR_RING2SEG_MUST_BE_MOVABLE,
    RFSERR_MSWIN_ERROR_RELOC_CHAIN_XEEDS_SEGLIM,
    RFSERR_MSWIN_ERROR_INFLOOP_IN_RELOC_CHAIN,
    RFSERR_MSWIN_ERROR_ENVVAR_NOT_FOUND,
    RFSERR_MSWIN_ERROR_NOT_CURRENT_CTRY,
    RFSERR_MSWIN_ERROR_NO_SIGNAL_SENT,
    RFSERR_MSWIN_ERROR_FILENAME_EXCED_RANGE,
    RFSERR_MSWIN_ERROR_RING2_STACK_IN_USE,
    RFSERR_MSWIN_ERROR_META_EXPANSION_TOO_LONG,
    RFSERR_MSWIN_ERROR_INVALID_SIGNAL_NUMBER,
    RFSERR_MSWIN_ERROR_THREAD_1_INACTIVE,
    RFSERR_MSWIN_ERROR_INFO_NOT_AVAIL,
    RFSERR_MSWIN_ERROR_LOCKED,
    RFSERR_MSWIN_ERROR_BAD_DYNALINK,
    RFSERR_MSWIN_ERROR_TOO_MANY_MODULES,
    RFSERR_MSWIN_ERROR_NESTING_NOT_ALLOWED,

};


#define REMOTEFS_SUCCESS                        0
#define REMOTEFS_ERROR                          1
#define REMOTEFS_ERR_MSG_LEN                    384

#define REMOTEFS_WRITEFILE_OVERWRITEMODE_QUICK  0
#define REMOTEFS_WRITEFILE_OVERWRITEMODE_SAFE   1
#define REMOTEFS_WRITEFILE_OVERWRITEMODE_BACKUP 2

#define REMOTEFS_FUDGE_MTU                      1312
#define TERMINAL_TCP_BUF_SIZE                   (REMOTEFS_FUDGE_MTU * 2 + 256)

#define REMOTEFS_LOCAL                          "localhost"

void remotefs_serverize (const char *listen_address, const char *acceptrange);

struct file_entry;
struct remotefs;

void remotefs_set_display_log_for_wtmp (const char *display);

void remotefs_free (struct remotefs *rfs);
struct remotefs *remotefs_new (const char *host, char *errmsg);
struct remotefs *remotefs_lookup (const char *host_, char *directory);
#define the_remotefs_local                      (remotefs_lookup (REMOTEFS_LOCAL, NULL))

struct action_callbacks {
    void *hook;
    int (*sock_reader) (struct action_callbacks *o, const unsigned char *chunk, int chunklen, long long filelen, char *errmsg);
    int (*sock_writer) (struct action_callbacks *o, unsigned char *chunk, int *chunklen, char *errmsg);
};

typedef unsigned long long remotefs_error_code_t;
#define REMOTEFS_MAX_PASSWORD_LEN               256

enum remotfs_password_return {
    REMOTFS_PASSWORD_RETURN_SUCCESS = 0,
    REMOTFS_PASSWORD_RETURN_ERROR = 1,
    REMOTFS_PASSWORD_RETURN_USERCANCEL = 2,
};

typedef enum remotfs_password_return (*remotfs_password_cb_t) (void *user_data, int again, const char *host, int *crypto_enabled, unsigned char *password, const char *user_msg, char *errmsg);

struct remotefs_private;
struct portable_stat;
struct cterminal_config;
struct xwinclient_data;
struct remotefs_terminalio {
    int cmd_fd;
    struct reader_data *reader_data;
    struct remotefs *remotefs;
    unsigned long cmd_pid;
    unsigned char *base;
    char ttydev[64]; /* fixme: make CTERMINAL_TTYDEV_SZ */
    char host[256];
    int current;
    int len;
    int alloced;
#ifdef XWIN_FWD
    struct xwinclient_data *xwinclient_data;
#else
    struct xwinclient_data *dummy__xwinclient_data;
#endif
};

void remotefs_free_terminalio (struct remotefs_terminalio *io);

const char *remotefs_home_dir (struct remotefs *rfs);
void remotefs_set_password_cb (remotfs_password_cb_t f, void *d);

int remotefs_reader_util (struct remotefs_terminalio *io, const int no_io);
void remotefs_set_die_on_error (void);
int remotefs_get_die_exit_code (void);
int remotefs_shell_util (const char *host, int xwin_fd, struct remotefs_terminalio *io, struct cterminal_config *c, int dumb_terminal, char *const argv[], char *errmsg);


struct remotefs {
    unsigned int magic;
    int (*remotefs_listdir) (struct remotefs *rfs, const char *directory, unsigned long options, const char *filter, struct file_entry **r, int *n, char *errmsg);
    int (*remotefs_readfile) (struct remotefs *rfs, struct action_callbacks *o, const char *filename, char *errmsg);
    int (*remotefs_writefile) (struct remotefs *rfs, struct action_callbacks *o, const char *filename, long long filelen, int overwritemode, unsigned int permissions, const char *backup_extension, struct portable_stat *st, char *errmsg);
    int (*remotefs_checkordinaryfileaccess) (struct remotefs *rfs, const char *filename, unsigned long long sizelimit, struct portable_stat *st, char *errmsg);
    int (*remotefs_stat) (struct remotefs *rfs, const char *path, struct portable_stat *st, int *just_not_there, remotefs_error_code_t *error_code, char *errmsg);
    int (*remotefs_chdir) (struct remotefs *rfs, const char *dirname, char *cwd, int cwdlen, char *errmsg);
    int (*remotefs_realpathize) (struct remotefs *rfs, const char *path, const char *homedir, char *out, int outlen, char *errmsg);
    int (*remotefs_gethomedir) (struct remotefs *rfs, char *out, int outlen, char *errmsg);
    int (*remotefs_enablecrypto) (struct remotefs *rfs, const unsigned char *challenge_local, unsigned char *challenge_remote, char *errmsg);
    int (*remotefs_shellcmd) (struct remotefs *rfs, struct remotefs_terminalio *io, struct cterminal_config *config, int dumb_terminal, char *const argv[], char *errmsg);
    int (*remotefs_shellresize) (struct remotefs *rfs, unsigned long pid, int columns, int rows, char *errmsg);
    int (*remotefs_shellread) (struct remotefs *rfs, struct remotefs_terminalio *io, unsigned long *multiplex, int *xfwdstatus, CStr *chunk, char *errmsg, int *time_out, int no_io);
    int (*remotefs_shellwrite) (struct remotefs *rfs, struct remotefs_terminalio *io, unsigned long multiplex, int xfwdstatus, const CStr *chunk, char *errmsg);
    int (*remotefs_shellkill) (struct remotefs *rfs, unsigned long pid);
    int (*remotefs_shellsignal) (struct remotefs *rfs, unsigned long pid, int signum, int *killret, char *errmsg);
    struct remotefs_private *remotefs_private;
};


#endif  /* REMOTEFS_H */



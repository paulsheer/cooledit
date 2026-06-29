/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#ifndef _DIRTOOLS_H
#define _DIRTOOLS_H

#include <assert.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <sys/types.h>

#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

/*
typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;
*/

struct windows_file_attributes {
    unsigned long long file_attributes;
    unsigned long long creation_time;
    unsigned long long last_accessed_time;
    unsigned long long last_write_time;
    unsigned long long file_size;
    unsigned long long reparse_tag;
};

#define REPARSE_TAG_RESERVED_ZERO       0x00000000ULL
#define REPARSE_TAG_RESERVED_ONE        0x00000001ULL
#define REPARSE_TAG_RESERVED_TWO        0x00000002ULL
#define REPARSE_TAG_MOUNT_POINT         0xA0000003ULL
#define REPARSE_TAG_HSM                 0xC0000004ULL
#define REPARSE_TAG_DRIVE_EXTENDER      0x80000005ULL
#define REPARSE_TAG_HSM2                0x80000006ULL
#define REPARSE_TAG_SIS                 0x80000007ULL
#define REPARSE_TAG_WIM                 0x80000008ULL
#define REPARSE_TAG_CSV                 0x80000009ULL
#define REPARSE_TAG_DFS                 0x8000000AULL
#define REPARSE_TAG_FILTER_MANAGER      0x8000000BULL
#define REPARSE_TAG_SYMLINK             0xA000000CULL
#define REPARSE_TAG_IIS_CACHE           0xA0000010ULL
#define REPARSE_TAG_DFSR                0x80000012ULL
#define REPARSE_TAG_DEDUP               0x80000013ULL
#define REPARSE_TAG_APPXSTRM            0xC0000014ULL
#define REPARSE_TAG_NFS                 0x80000014ULL
#define REPARSE_TAG_FILE_PLACEHOLDER    0x80000015ULL
#define REPARSE_TAG_DFM                 0x80000016ULL
#define REPARSE_TAG_WOF                 0x80000017ULL
#define REPARSE_TAG_WCI                 0x80000018ULL
#define REPARSE_TAG_WCI_1               0x90001018ULL
#define REPARSE_TAG_GLOBAL_REPARSE      0xA0000019ULL
#define REPARSE_TAG_CLOUD               0x9000001AULL
#define REPARSE_TAG_CLOUD_1             0x9000101AULL
#define REPARSE_TAG_CLOUD_2             0x9000201AULL
#define REPARSE_TAG_CLOUD_3             0x9000301AULL
#define REPARSE_TAG_CLOUD_4             0x9000401AULL
#define REPARSE_TAG_CLOUD_5             0x9000501AULL
#define REPARSE_TAG_CLOUD_6             0x9000601AULL
#define REPARSE_TAG_CLOUD_7             0x9000701AULL
#define REPARSE_TAG_CLOUD_8             0x9000801AULL
#define REPARSE_TAG_CLOUD_9             0x9000901AULL
#define REPARSE_TAG_CLOUD_A             0x9000A01AULL
#define REPARSE_TAG_CLOUD_B             0x9000B01AULL
#define REPARSE_TAG_CLOUD_C             0x9000C01AULL
#define REPARSE_TAG_CLOUD_D             0x9000D01AULL
#define REPARSE_TAG_CLOUD_E             0x9000E01AULL
#define REPARSE_TAG_CLOUD_F             0x9000F01AULL
#define REPARSE_TAG_APPEXECLINK         0x8000001BULL
#define REPARSE_TAG_PROJFS              0x9000001CULL
#define REPARSE_TAG_LX_SYMLINK          0xA000001DULL
#define REPARSE_TAG_STORAGE_SYNC        0x8000001EULL
#define REPARSE_TAG_WCI_TOMBSTONE       0xA000001FULL
#define REPARSE_TAG_UNHANDLED           0x80000020ULL
#define REPARSE_TAG_ONEDRIVE            0x80000021ULL
#define REPARSE_TAG_PROJFS_TOMBSTONE    0xA0000022ULL
#define REPARSE_TAG_AF_UNIX             0x80000023ULL
#define REPARSE_TAG_LX_FIFO             0x80000024ULL
#define REPARSE_TAG_LX_CHR              0x80000025ULL
#define REPARSE_TAG_LX_BLK              0x80000026ULL
#define REPARSE_TAG_WCI_LINK            0xA0000027ULL
#define REPARSE_TAG_WCI_LINK_1          0xA0001027ULL

#ifdef MSWIN
#define my_stat(f, s)           stat64(f, s)
#define my_lstat(f, s)          lstat64(f, s)
#define my_fstat(f, s)          fstat64(f, s)
#define stat_posix_or_mswin     _stat64
#else
#define my_stat(f, s)           stat(f, s)
#define my_lstat(f, s)          lstat(f, s)
#define my_fstat(f, s)          fstat(f, s)
#define stat_posix_or_mswin     stat
#endif

struct portable_stat {
    int os;
    int os_sub;
    struct stat_posix_or_mswin ustat;
    unsigned long dev_major;
    unsigned long dev_minor;
    struct windows_file_attributes wattr;
};

struct file_item {
    unsigned long options;
    char *name;
    char *link_target;
    struct portable_stat pstat;
};

struct file_entry {
    struct file_item **d;
    int dl;
};
void file_array_free (struct file_entry *fa);
struct file_entry *file_array_copy (struct file_entry *fa);

#define FILELIST_LAST_ENTRY		(1<<8)
#define FILELIST_FILES_ONLY		(1<<15)
#define FILELIST_DIRECTORIES_ONLY	(1<<16)
#define FILELIST_ALL_FILES              (0)
#define FILELIST_MASK                   (FILELIST_FILES_ONLY|FILELIST_DIRECTORIES_ONLY)

void pstat_to_mode_string (struct portable_stat *ps, char *mode);

#endif  /* _DIRTOOLS_H */

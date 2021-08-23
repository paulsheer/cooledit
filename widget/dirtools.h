
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
};

struct portable_stat {
    int os;
    int os_sub;
    struct stat ustat;
    struct windows_file_attributes wattr;
};

struct file_entry {
    unsigned long options;
    char name[260];
    struct portable_stat pstat;
};

#define FILELIST_LAST_ENTRY		(1<<8)
#define FILELIST_FILES_ONLY		(1<<15)
#define FILELIST_DIRECTORIES_ONLY	(1<<16)

void pstat_to_mode_string (struct portable_stat *ps, char *mode);

#endif  /* _DIRTOOLS_H */

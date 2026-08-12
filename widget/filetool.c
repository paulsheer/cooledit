/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* filetool.c
   Copyright (C) 1996-2022 Paul Sheer
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <sys/xattr.h>

#include "inspect.h"
#include <config.h>
#include "stringtools.h"
#include "dirtools.h"
#include "remotefs.h"
#include "filetool.h"
#include "remotefspassword.h"


char *get_sys_error (const char *s);
void get_home_dir (void);

#define DEFAULT_CREATE_MODE            (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static int password_loaded = 0;
static int dummy_data;
static int force_flag = 0;
static int verbose_flag = 0;
static int progress_file_count = 0;
static int ls_flag = 0;
static int ls_opt_a = 0;
static int ls_opt_d = 0;
static int ls_opt_l = 0;
static int ls_opt_r = 0;
static int ls_opt_S = 0;
static int ls_opt_t = 0;
static int ls_opt_1 = 0;

#define SAVE_MODE       REMOTEFS_WRITEFILE_OVERWRITEMODE_SAFE

extern char *option_backup_ext;

/* --- helpers for new CLI --- */

static void strip_trailing_slash (char *path, int os_type, int *last_char_is_dir)
{
    int r = 0, len;
    /* Strip trailing slashes.  '/' is always a path separator.
       '\' is a path separator only on Windows (OS_TYPE_WINDOWS);
       on Unix it is a literal filename character. */
    len = strlen (path);
    if (len > 1 && path[len - 1] == '/') {
        r = 1;
        while (len > 1 && (path[len - 1] == '/')) {
            if (len == 3 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':')
                break;
            path[--len] = '\0';
        }
    } else if (os_type == OS_TYPE_WINDOWS && len > 1 && path[len - 1] == '\\') {
        r = 1;
        while (len > 1 && (path[len - 1] == '\\')) {
            if (len == 3 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':')
                break;
            path[--len] = '\0';
        }
    }
    if (last_char_is_dir)
        *last_char_is_dir = r;
}

static int log__2 (unsigned long long n)
{
    int r = 0;
    while (n) {
        r++;
        n >>= 1;
    }
    return r;
}

static void progress (int is_dir, int filecount, int bytes)
{
    static int backspace = 0;
    static int filecount_ = 0;
    static int rotate_ = 0;
    static long long bytes_ = 0;
    static long long last_bytes_ = 0;
    char rotate[] = "|/-\\";
    if (!verbose_flag)
        return;
    if (filecount_ < filecount) {
        filecount_ = filecount;
        last_bytes_ = bytes_ = 0LL;
    }
    bytes_ += bytes;
    if (is_dir || bytes_ < 1024 * 1024) {
        last_bytes_ = bytes_;
        rotate_++;
        printf ("%c\b", rotate[rotate_ % 4]);
        fflush (stdout);
        backspace = 1;
        return;
    }
    if (backspace) {
        printf ("*");
        fflush (stdout);
        backspace = 0;
    }
    if (log__2 (last_bytes_) != log__2 (bytes_)) {
        printf (".");
        fflush (stdout);
        last_bytes_ = bytes_;
    }
}

static void parse_remote_path (const char *arg, char *ip, int ip_len, char *path, int path_len)
{
    const char *colon;

    ip[0] = '\0';
    colon = strchr (arg, ':');
    if (colon) {
        int n = colon - arg;
        if (n >= ip_len) n = ip_len - 1;
        memcpy (ip, arg, n);
        ip[n] = '\0';
        strncpy (path, colon + 1, path_len - 1);
        path[path_len - 1] = '\0';
    } else {
        strncpy (path, arg, path_len - 1);
        path[path_len - 1] = '\0';
    }
}

static int path_stat (const char *ip, const char *path, struct portable_stat *st, int *is_dir, int *exists, char *link_target, int link_target_sz, char *errmsg)
{
    struct remotefs *rfs;
    remotefs_error_code_t error_code;
    int just_not_there = 0;

    *exists = 0;
    *is_dir = 0;
    memset (st, 0, sizeof (*st));

    rfs = ip[0] ? remotefs_lookup (ip, NULL) : the_remotefs_local;

    if ((*rfs->remotefs_stat) (rfs, NULL, path, st, link_target, link_target_sz, &just_not_there, &error_code, errmsg))
        return -1;

    if (just_not_there)
        return 0;

    *exists = 1;
    *is_dir = S_ISDIR (st->ustat.st_mode);
    return 0;
}

static const char *my_basename (const char *path, int os_type)
{
    const char *p = strrchr (path, '/');
    if (os_type == OS_TYPE_WINDOWS) {
        const char *q = strrchr (path, '\\');
        if (!p || (q && q > p))
            p = q;
    }
    return p ? p + 1 : path;
}

static void path_join (const char *dir, const char *name, char *out, int outlen)
{
    int dlen = strlen (dir);
    strncpy (out, dir, outlen - 1);
    out[outlen - 1] = '\0';
    if (dlen > 0 && out[dlen - 1] != '/')
        strncat (out, "/", outlen - strlen (out) - 1);
    strncat (out, name, outlen - strlen (out) - 1);
}

static const char *reparse_tag_name (unsigned long long tag)
{
    switch (tag) {
    case REPARSE_TAG_RESERVED_ZERO:     return "reserved (0)";
    case REPARSE_TAG_RESERVED_ONE:      return "reserved (1)";
    case REPARSE_TAG_RESERVED_TWO:      return "reserved (2)";
    case REPARSE_TAG_MOUNT_POINT:       return "junction/mount-point";
    case REPARSE_TAG_HSM:               return "HSM (obsolete)";
    case REPARSE_TAG_DRIVE_EXTENDER:    return "drive extender";
    case REPARSE_TAG_HSM2:              return "HSM2 (obsolete)";
    case REPARSE_TAG_SIS:               return "single-instance storage";
    case REPARSE_TAG_WIM:               return "WIM mount filter";
    case REPARSE_TAG_CSV:               return "clustered shared volumes";
    case REPARSE_TAG_DFS:               return "distributed file system";
    case REPARSE_TAG_FILTER_MANAGER:    return "filter manager";
    case REPARSE_TAG_SYMLINK:           return "symlink";
    case REPARSE_TAG_IIS_CACHE:         return "IIS cache";
    case REPARSE_TAG_DFSR:              return "DFS replication";
    case REPARSE_TAG_DEDUP:             return "data deduplication";
    case REPARSE_TAG_APPXSTRM:          return "appx stream";
    case REPARSE_TAG_NFS:               return "NFS";
    case REPARSE_TAG_FILE_PLACEHOLDER:  return "file placeholder (obsolete)";
    case REPARSE_TAG_DFM:               return "dynamic file filter";
    case REPARSE_TAG_WOF:               return "Windows overlay filter";
    case REPARSE_TAG_WCI:               return "container isolation";
    case REPARSE_TAG_WCI_1:             return "container isolation (1)";
    case REPARSE_TAG_GLOBAL_REPARSE:    return "global reparse (named pipe)";
    case REPARSE_TAG_CLOUD:             return "cloud files (OneDrive)";
    case REPARSE_TAG_CLOUD_1:           return "cloud files (1)";
    case REPARSE_TAG_CLOUD_2:           return "cloud files (2)";
    case REPARSE_TAG_CLOUD_3:           return "cloud files (3)";
    case REPARSE_TAG_CLOUD_4:           return "cloud files (4)";
    case REPARSE_TAG_CLOUD_5:           return "cloud files (5)";
    case REPARSE_TAG_CLOUD_6:           return "cloud files (6)";
    case REPARSE_TAG_CLOUD_7:           return "cloud files (7)";
    case REPARSE_TAG_CLOUD_8:           return "cloud files (8)";
    case REPARSE_TAG_CLOUD_9:           return "cloud files (9)";
    case REPARSE_TAG_CLOUD_A:           return "cloud files (A)";
    case REPARSE_TAG_CLOUD_B:           return "cloud files (B)";
    case REPARSE_TAG_CLOUD_C:           return "cloud files (C)";
    case REPARSE_TAG_CLOUD_D:           return "cloud files (D)";
    case REPARSE_TAG_CLOUD_E:           return "cloud files (E)";
    case REPARSE_TAG_CLOUD_F:           return "cloud files (F)";
    case REPARSE_TAG_APPEXECLINK:       return "UWP app execution link";
    case REPARSE_TAG_PROJFS:            return "projected file system";
    case REPARSE_TAG_LX_SYMLINK:        return "WSL symlink";
    case REPARSE_TAG_STORAGE_SYNC:      return "Azure file sync";
    case REPARSE_TAG_WCI_TOMBSTONE:     return "container tombstone";
    case REPARSE_TAG_UNHANDLED:         return "WCI unhandled";
    case REPARSE_TAG_ONEDRIVE:          return "OneDrive (legacy)";
    case REPARSE_TAG_PROJFS_TOMBSTONE:  return "ProjFS tombstone";
    case REPARSE_TAG_AF_UNIX:           return "WSL Unix socket";
    case REPARSE_TAG_LX_FIFO:           return "WSL FIFO";
    case REPARSE_TAG_LX_CHR:            return "WSL character device";
    case REPARSE_TAG_LX_BLK:            return "WSL block device";
    case REPARSE_TAG_WCI_LINK:          return "container link";
    case REPARSE_TAG_WCI_LINK_1:        return "container link (1)";
    default:                            return NULL;
    }
}

static void set_junction_xattr (const char *path)
{
    const char val[] = "1";
    if (lsetxattr (path, "trusted.windows.junction", val, 1, 0) < 0)
        fprintf (stderr, "Warning: could not set xattr on %s: %s\n", path, strerror (errno));
}

static void warn_skipping (struct portable_stat *pst, const char *path)
{
    if (S_ISCHR (pst->ustat.st_mode))
        fprintf (stderr, "Warning: skipping character device: %lu, %lu  %s\n", pst->dev_major, pst->dev_minor, path);
    else if (S_ISBLK (pst->ustat.st_mode))
        fprintf (stderr, "Warning: skipping block device: %lu, %lu  %s\n", pst->dev_major, pst->dev_minor, path);
    else if (S_ISFIFO (pst->ustat.st_mode))
        fprintf (stderr, "Warning: skipping FIFO: %s\n", path);
    else if (S_ISSOCK (pst->ustat.st_mode))
        fprintf (stderr, "Warning: skipping socket: %s\n", path);
    else if (S_ISLNK (pst->ustat.st_mode)) {
        const char *name = reparse_tag_name (pst->wattr.reparse_tag);
        if (name)
            fprintf (stderr, "Warning: skipping unsupported reparse point (%s): %s\n", name, path);
        else
            fprintf (stderr, "Warning: skipping unsupported reparse point (tag 0x%llx): %s\n", pst->wattr.reparse_tag, path);
    } else
        fprintf (stderr, "Warning: skipping unknown special file: %s\n", path);
}

static int confirm_overwrite (const char *path)
{
    char line[16];
    if (force_flag)
        return 1;
    fprintf (stderr, "Overwrite %s? (y/n) ", path);
    fflush (stderr);
    if (!fgets (line, sizeof (line), stdin))
        return 0;
    return (line[0] == 'y' || line[0] == 'Y');
}

struct loader_data {
    long total;
    int done;
    FILE *f;
    const char *fname;
};

static int filetool_sock_reader (struct action_callbacks *o, const unsigned char *buf, int buflen, unsigned long long filelen, char *errmsg)
{E_
    struct loader_data *ld;

    ld = (struct loader_data *) o->hook;

    if (fwrite (buf, 1, buflen, ld->f) != buflen || fflush (ld->f)) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "%s: Error writing to file: %s\n", ld->fname, get_sys_error (""));
        return -1;
    }
    progress (0, progress_file_count, buflen);

    return 0;
}

int filetool_copy_remote_to_local (const char *host, const char *remote_filename, const char *local_filename)
{E_
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct loader_data ld;
    struct action_callbacks o;
    struct remotefs *u;

    memset (&ld, '\0', sizeof (ld));
    ld.f = fopen (local_filename, "wb");
    ld.fname = local_filename;
    if (!ld.f) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "%s: Error opening file: %s\n", local_filename, get_sys_error (""));
        return 1;
    }
    progress (0, ++progress_file_count, 0);

    memset (&o, '\0', sizeof (o));

    o.hook = (void *) &ld;
    o.sock_reader = filetool_sock_reader;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_readfile) (u, &o, remote_filename, errmsg)) {
        fclose (ld.f);
        unlink (local_filename);
        fprintf (stderr, "%s: Failed trying to open file for reading: %s\n", remote_filename, errmsg);
        return 1;
    }

    if (fclose (ld.f)) {
        unlink (local_filename);
        fprintf (stderr, "%s: Error closing file: %s\n", local_filename, get_sys_error (""));
    }

    return 0;
}


struct saver_data {
    FILE *f;
    const char *remote_filename;
    unsigned long long totalwritten;
    unsigned long long filelen;
    int done;
};

static int filetool_sock_writer (struct action_callbacks *o, unsigned char *chunk, int *chunklen_, char *errmsg)
{E_
    struct saver_data *sd;
    int c;
    sd = (struct saver_data *) o->hook;

    if (sd->filelen == FILE_LEN_INDEFINITE) {
        if (sd->done) {
            *chunklen_ = 0;
            return 0;
        }
    } else {
        if (sd->done || sd->totalwritten >= sd->filelen) {
            strcpy (errmsg, "%s: Unknown error");
            return -1;
        }
    }

    c = fread (chunk, 1, *chunklen_, sd->f);
    if (c < 0) {
        snprintf (errmsg, REMOTEFS_ERR_MSG_LEN, "%s: Error writing to file: %s\n", sd->remote_filename, get_sys_error (""));
        return -1;
    }
    if (c < *chunklen_)
        sd->done = 1;
    sd->totalwritten += c;
    *chunklen_ = c;
    progress (0, progress_file_count, c);

    return 0;
}

int filetool_copy_local_to_remote (const char *local_filename, const char *host, const char *remote_filename)
{E_
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct saver_data sd;
    struct action_callbacks o;
    struct portable_stat st;
    struct remotefs *u;

    struct stat local_st;

    memset (&sd, '\0', sizeof (sd));

    sd.remote_filename = remote_filename;
    sd.f = fopen (local_filename, "rb");
    if (!sd.f) {
        perror (local_filename);
        return 1;
    }
    progress (0, ++progress_file_count, 0);

    if (fstat (fileno (sd.f), &local_st)) {
        fclose (sd.f);
        perror (local_filename);
        return 1;
    }

    memset (&o, '\0', sizeof (o));

    if (remotefs_check_indefinite_length (local_filename)) {
        sd.filelen = FILE_LEN_INDEFINITE;
    } else {
        sd.filelen = local_st.st_size;
    }

    o.hook = (void *) &sd;
    o.sock_writer = filetool_sock_writer;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_writefile) (u, &o, remote_filename, sd.filelen, SAVE_MODE, DEFAULT_CREATE_MODE, option_backup_ext, &st, errmsg)) {
        fprintf (stderr, "%s: Failed trying to write file: %s\n", remote_filename, errmsg);
        fclose (sd.f);
        return 1;
    }

    if (sd.filelen != FILE_LEN_INDEFINITE && sd.totalwritten != local_st.st_size) {
        fprintf (stderr, "%s: Error: Did not write all bytes: ", local_filename);
        fclose (sd.f);
        return 1;
    }

    fclose (sd.f);
    return 0;
}

int filetool_copy_local_to_local (const char *local_src_filename, const char *local_dst_filename)
{E_
    FILE *fsrc, *fdst;
    struct stat local_st;
    unsigned long long filelen;
    unsigned long long totalwritten = 0;
    unsigned char buf[65536];
    int indefinite;

    fsrc = fopen (local_src_filename, "rb");
    if (!fsrc) {
        perror (local_src_filename);
        return 1;
    }

    if (fstat (fileno (fsrc), &local_st)) {
        perror (local_src_filename);
        fclose (fsrc);
        return 1;
    }

    indefinite = remotefs_check_indefinite_length (local_src_filename);
    filelen = indefinite ? FILE_LEN_INDEFINITE : (unsigned long long) local_st.st_size;

    fdst = fopen (local_dst_filename, "wb");
    if (!fdst) {
        perror (local_dst_filename);
        fclose (fsrc);
        return 1;
    }
    progress (0, ++progress_file_count, 0);

    if (indefinite) {
        for (;;) {
            size_t c;
            c = fread (buf, 1, sizeof (buf), fsrc);
            if (!c)
                break;
            if (fwrite (buf, 1, c, fdst) != c) {
                perror (local_dst_filename);
                fclose (fsrc);
                fclose (fdst);
                return 1;
            }
            totalwritten += c;
            progress (0, progress_file_count, c);
        }
    } else {
        while (totalwritten < filelen) {
            size_t c;
            c = fread (buf, 1, sizeof (buf), fsrc);
            if (!c)
                break;
            if (fwrite (buf, 1, c, fdst) != c) {
                perror (local_dst_filename);
                fclose (fsrc);
                fclose (fdst);
                return 1;
            }
            totalwritten += c;
            progress (0, progress_file_count, c);
        }
        if (totalwritten != filelen) {
            fprintf (stderr, "%s: Error: Did not write all bytes\n", local_src_filename);
            fclose (fsrc);
            fclose (fdst);
            return 1;
        }
    }

    fclose (fsrc);
    fclose (fdst);
    return 0;
}

void filetool_usage(FILE *out, const char *prefix)
{
    fprintf(out, "%s\
--filetool [-f|--force] [-v|--verbose] [--] <src> [<src>...] <target>\n\
                                         scp-like remote copy with\n\
                                         recursive directory copy feature.\n\
                                         Uses remotefs / REMOTEFS.EXE /\n\
                                         remotefs.apk as a server.\n\
--filetool --ls|-ls [-adlrSt1] [<path>...]            list files in ls style\n\
                                         with Unix-compatible options\n", prefix);
}

static void usage_ (void)
{E_
    fprintf(stderr, "Usage\n");
    filetool_usage(stderr, "    cooledit ");
}

static void filetool_clean (void)
{
    remotefs_clean ();
    password_clean ();
}

static void usage_exit_error (void)
{E_
    usage_ ();
    filetool_clean ();
    exit (1);
}

static int contains_whitespace (const char *pass_)
{E_
    const unsigned char *p;
    p = (const unsigned char *) pass_;
    while (*p)
        if (*p++ <= ' ')
            return 1;
    return 0;
}

static enum remotfs_password_return password_remotfs_password_cb (void *user_data, int again, const char *host, int *crypto_enabled_, unsigned char *pass_, const char *user_msg, char *errmsg)
{E_
    char pass[REMOTEFS_MAX_PASSWORD_LEN] = "";
    char s[REMOTEFS_MAX_PASSWORD_LEN];
    int crypto_enabled = 1;
    int found;

    *crypto_enabled_ = 1;

    /* if the remote server doesn't support crypto, fall back to unencrypted */
    if (user_msg && strstr (user_msg, "crypto not supported")) {
        *crypto_enabled_ = 0;
        pass_[0] = '\0';
        return REMOTFS_PASSWORD_RETURN_SUCCESS;
    }

    if (!password_loaded) {
        password_loaded = 1;
        if (password_load_ ()) {
            fprintf (stderr, "Error Loading Passwords: ~%s: %s\n", PASSWORD_FILE, get_sys_error (""));
        }
    }

    assert (user_data == &dummy_data);
    found = !password_find (host, &crypto_enabled, (char *) pass, REMOTEFS_MAX_PASSWORD_LEN);
    if (!again && found) {
        *crypto_enabled_ = crypto_enabled;
        strcpy ((char *) pass_, pass);
        return REMOTFS_PASSWORD_RETURN_SUCCESS;
    }

    strcpy (pass, (const char *) pass_);

    for (;;) {
        if (user_msg && *user_msg)
            printf ("[%s]\nEnter a strong AES key for host %s. Whitespace is not allowed\n", user_msg, host);
        else
            printf ("Enter a strong AES key for host %s. Whitespace is not allowed\n", host);
        crypto_enabled = 1;
        if (!fgets(s, sizeof (s), stdin)) {
            strcpy (errmsg, "connection canceled by the user");
            return REMOTFS_PASSWORD_RETURN_USERCANCEL;
        }
        string_chomp (s);
        strcpy ((char *) pass, s);
        if (!pass[0] || contains_whitespace (pass)) {
            printf("Password Error: Whitespace characters are not allowed.\n");
            continue;
        }
        break;
    }

    if (password_save (host, crypto_enabled, pass))
        fprintf (stderr, "Error Saving Passwords: ~%s: %s\n", PASSWORD_FILE, get_sys_error (""));

    *crypto_enabled_ = crypto_enabled;
    strcpy ((char *) pass_, pass);

    return REMOTFS_PASSWORD_RETURN_SUCCESS;
}

static void filetool_password_init (void)
{E_
    remotefs_set_password_cb (password_remotfs_password_cb, &dummy_data);
}

/* --- recursive directory copy --- */

static int copy_dir_local_to_remote (const char *local_dir, const char *host, const char *remote_dir, int force)
{
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct file_entry *list = NULL;
    int i, cached = 0;
    struct remotefs *rfs;
    char sub_local[MAX_PATH_LEN], sub_remote[MAX_PATH_LEN];

    rfs = remotefs_lookup (host, NULL);

    /* create destination directory on remote */
    if ((*rfs->remotefs_mkdir) (rfs, remote_dir, 0777, errmsg)) {
        fprintf (stderr, "Error creating remote directory %s: %s\n", remote_dir, errmsg);
        goto err;;
    }
    progress (1, ++progress_file_count, 0);

    /* list local directory */
    {
        struct remotefs *local_rfs = the_remotefs_local;
        if ((*local_rfs->remotefs_listdir) (local_rfs, &cached, local_dir, FILELIST_ALL_FILES, "*", &list, errmsg)) {
            fprintf (stderr, "Error listing directory %s: %s\n", local_dir, errmsg);
            goto err;;
        }
    }

    for (i = 0; i < list->dl; i++) {
        if (!strcmp (list->d[i]->name, ".") || !strcmp (list->d[i]->name, ".."))
            continue;

        path_join (local_dir, list->d[i]->name, sub_local, sizeof (sub_local));
        path_join (remote_dir, list->d[i]->name, sub_remote, sizeof (sub_remote));

        if (S_ISDIR (list->d[i]->pstat.ustat.st_mode)) {
            if (copy_dir_local_to_remote (sub_local, host, sub_remote, force))
                goto err;;
        } else if (S_ISREG (list->d[i]->pstat.ustat.st_mode)) {
            if (filetool_copy_local_to_remote (sub_local, host, sub_remote))
                goto err;;
        } else if (S_ISLNK (list->d[i]->pstat.ustat.st_mode)) {
            if (list->d[i]->pstat.wattr.reparse_tag
                && list->d[i]->pstat.wattr.reparse_tag != REPARSE_TAG_SYMLINK
                && list->d[i]->pstat.wattr.reparse_tag != REPARSE_TAG_MOUNT_POINT) {
                warn_skipping (&list->d[i]->pstat, sub_local);
            } else if (list->d[i]->pstat.wattr.reparse_tag == REPARSE_TAG_MOUNT_POINT) {
                if ((*rfs->remotefs_junction) (rfs, list->d[i]->link_target, sub_remote, errmsg)) {
                    fprintf (stderr, "Error creating remote junction %s: %s\n", sub_remote, errmsg);
                    goto err;;
                }
            } else if ((*rfs->remotefs_symlink) (rfs, list->d[i]->link_target, sub_remote, errmsg)) {
                fprintf (stderr, "Error creating remote symlink %s: %s\n", sub_remote, errmsg);
                goto err;;
            }
        } else {
            warn_skipping (&list->d[i]->pstat, sub_local);
        }
    }

    file_array_free (list);
    return 0;

  err:
    file_array_free (list);
    return 1;
}

static int copy_dir_remote_to_local (const char *ip, const char *remote_dir, const char *local_dir, int force)
{
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct file_entry *list = NULL;
    int i, cached = 0;
    struct remotefs *rfs;
    char sub_remote[MAX_PATH_LEN], sub_local[MAX_PATH_LEN];

    rfs = ip[0] ? remotefs_lookup (ip, NULL) : the_remotefs_local;

    /* create destination directory locally */
    if (mkdir (local_dir, 0777) < 0 && errno != EEXIST) {
        fprintf (stderr, "Error creating directory %s: %s\n", local_dir, strerror (errno));
        goto err;
    }
    progress (1, ++progress_file_count, 0);

    /* list remote directory */
    if ((*rfs->remotefs_listdir) (rfs, &cached, remote_dir, FILELIST_ALL_FILES, "*", &list, errmsg)) {
        fprintf (stderr, "Error listing remote directory %s: %s\n", remote_dir, errmsg);
        goto err;
    }

    for (i = 0; i < list->dl; i++) {
        if (!strcmp (list->d[i]->name, ".") || !strcmp (list->d[i]->name, ".."))
            continue;

        path_join (remote_dir, list->d[i]->name, sub_remote, sizeof (sub_remote));
        path_join (local_dir, list->d[i]->name, sub_local, sizeof (sub_local));

        if (S_ISDIR (list->d[i]->pstat.ustat.st_mode)) {
            if (copy_dir_remote_to_local (ip, sub_remote, sub_local, force))
                goto err;
        } else if (S_ISREG (list->d[i]->pstat.ustat.st_mode)) {
            if (ip[0]
                ? filetool_copy_remote_to_local (ip, sub_remote, sub_local)
                : filetool_copy_local_to_local (sub_remote, sub_local))
                goto err;
        } else if (S_ISLNK (list->d[i]->pstat.ustat.st_mode)) {
            if (list->d[i]->pstat.wattr.reparse_tag
                && list->d[i]->pstat.wattr.reparse_tag != REPARSE_TAG_SYMLINK
                && list->d[i]->pstat.wattr.reparse_tag != REPARSE_TAG_MOUNT_POINT) {
                warn_skipping (&list->d[i]->pstat, sub_remote);
            } else if ((*the_remotefs_local->remotefs_symlink) (the_remotefs_local, list->d[i]->link_target, sub_local, errmsg)) {
                fprintf (stderr, "Error creating symlink %s: %s\n", sub_local, errmsg);
                goto err;
            } else if (list->d[i]->pstat.wattr.reparse_tag == REPARSE_TAG_MOUNT_POINT) {
                set_junction_xattr (sub_local);
            }
        } else {
            warn_skipping (&list->d[i]->pstat, sub_remote);
        }
    }

    file_array_free (list);
    return 0;

  err:
    file_array_free (list);
    return 1;
}

static int has_remote_prefix (const char *arg)
{
    return strchr (arg, ':') != NULL;
}

static int is_cross_remote (int nsrcs, char **srcs, const char *dst)
{
    int i, dst_remote = has_remote_prefix (dst);
    for (i = 0; i < nsrcs; i++)
        if (has_remote_prefix (srcs[i]) && dst_remote)
            return 1;
    return 0;
}

static int handle_single_source (const char *src, const char *dst)
{
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    char src_ip[256], src_path[MAX_PATH_LEN];
    char dst_ip[256], dst_path[MAX_PATH_LEN];
    struct portable_stat src_st, dst_st;
    int src_is_dir = 0, src_exists = 0, dst_is_dir = 0, dst_exists = 0;
    char target_path[MAX_PATH_LEN] = "";
    const char *target = NULL;
    int src_is_remote = 0, dst_is_remote = 0;
    int last_src_char_is_dir = 0, last_dst_char_is_dir = 0;

    *errmsg = '\0';

    parse_remote_path (src, src_ip, sizeof (src_ip), src_path, sizeof (src_path));

    parse_remote_path (dst, dst_ip, sizeof (dst_ip), dst_path, sizeof (dst_path));

    src_is_remote = (src_ip[0] != '\0');
    dst_is_remote = (dst_ip[0] != '\0');

    /* stat source */
    {
        char link_target[MAX_PATH_LEN] = "";
        if (path_stat (src_ip, src_path, &src_st, &src_is_dir, &src_exists, link_target, sizeof (link_target), errmsg)) {
            fprintf (stderr, "Error stating source %s: %s\n", src, errmsg);
            return 1;
        }

        strip_trailing_slash (src_path, src_st.os, &last_src_char_is_dir);

        if (last_src_char_is_dir && !src_is_dir) {
            fprintf (stderr, "Error: %s is not a directory\n", src);
            return 1;
        }

        /* stat destination */
        if (path_stat (dst_ip, dst_path, &dst_st, &dst_is_dir, &dst_exists, NULL, 0, errmsg)) {
            fprintf (stderr, "Error stating destination %s: %s\n", dst, errmsg);
            return 1;
        }

        strip_trailing_slash (dst_path, dst_st.os, &last_dst_char_is_dir);

        if (last_dst_char_is_dir && !dst_is_dir) {
            fprintf (stderr, "Error: %s is not a directory\n", dst);
            return 1;
        }

        if (link_target[0]) {
            if (dst_exists && dst_is_dir) {
                target = target_path;
                path_join (dst_path, my_basename (src_path, src_st.os), target_path, sizeof (target_path));
            } else {
                target = dst_path;
            }
            if (dst_exists && !dst_is_dir) {
                if (!confirm_overwrite (dst))
                    return 0;
            }
            if (dst_is_remote) {
                struct remotefs *rfs = remotefs_lookup (dst_ip, NULL);
                if (src_st.wattr.reparse_tag == REPARSE_TAG_MOUNT_POINT) {
                    if ((*rfs->remotefs_junction) (rfs, link_target, target, errmsg)) {
                        fprintf (stderr, "Error creating remote junction %s: %s\n", target, errmsg);
                        return 1;
                    }
                } else {
                    if ((*rfs->remotefs_symlink) (rfs, link_target, target, errmsg)) {
                        fprintf (stderr, "Error creating remote symlink %s: %s\n", target, errmsg);
                        return 1;
                    }
                }
            } else {
                if ((*the_remotefs_local->remotefs_symlink) (the_remotefs_local, link_target, target, errmsg)) {
                    fprintf (stderr, "Error creating symlink %s: %s\n", target, errmsg);
                    return 1;
                }
                if (src_st.wattr.reparse_tag == REPARSE_TAG_MOUNT_POINT)
                    set_junction_xattr (target);
            }
            return 0;
        }
    }

    if (!src_exists) {
        fprintf (stderr, "Error: source %s does not exist\n", src);
        return 1;
    }

    if (src_is_dir) {
        /* directory source: cases 5-10 */
        if (dst_exists && !dst_is_dir) {
            fprintf (stderr, "Error: cannot copy directory %s to a file %s\n", src, dst);
            return 1;
        }
        if (dst_exists && dst_is_dir) {
            target = target_path;
            path_join (dst_path, my_basename (src_path, src_st.os), target_path, sizeof (target_path));
        } else {
            target = dst_path;
        }

        if (!dst_is_remote) {
            if (copy_dir_remote_to_local (src_ip, src_path, target, force_flag))
                return 1;
        } else {
            if (copy_dir_local_to_remote (src_path, dst_ip, target, force_flag))
                return 1;
        }
    } else {
        /* file source: cases 1-4 */
        if (!S_ISREG (src_st.ustat.st_mode)) {
            warn_skipping (&src_st, src);
            return 0;
        }
        if (dst_exists && dst_is_dir) {
            target = target_path;
            path_join (dst_path, my_basename (src_path, src_st.os), target_path, sizeof (target_path));
        } else {
            target = dst_path;
        }

        if (dst_exists && !dst_is_dir) {
            if (!confirm_overwrite (dst))
                return 0;
        }

        if (src_is_remote) {
            if (filetool_copy_remote_to_local (src_ip, src_path, target))
                return 1;
        } else if (dst_is_remote) {
            if (filetool_copy_local_to_remote (src_path, dst_ip, target))
                return 1;
        } else {
            if (filetool_copy_local_to_local (src_path, target))
                return 1;
        }
    }
    return 0;
}

/* --- ls helpers --- */

static int ls_cmp (const void *a, const void *b)
{
    const struct file_item *fa = (const struct file_item *) a;
    const struct file_item *fb = (const struct file_item *) b;
    int r;
    if (ls_opt_S) {
        long long sa = (long long) fa->pstat.ustat.st_size;
        long long sb = (long long) fb->pstat.ustat.st_size;
        if (sa < sb) return ls_opt_r ? -1 : 1;
        if (sa > sb) return ls_opt_r ? 1 : -1;
    }
    if (ls_opt_t) {
        if (fa->pstat.ustat.st_mtime < fb->pstat.ustat.st_mtime)
            return ls_opt_r ? -1 : 1;
        if (fa->pstat.ustat.st_mtime > fb->pstat.ustat.st_mtime)
            return ls_opt_r ? 1 : -1;
    }
    r = strcmp (fa->name, fb->name);
    return ls_opt_r ? -r : r;
}

static int ls_cmp_ptr (const void *a, const void *b)
{
    return ls_cmp (*(const struct file_item **) a, *(const struct file_item **) b);
}

static void ls_print_long (struct file_item *e, const char *target)
{
    char mode[64];
    char timebuf[64];
    struct tm tm;
    time_t t, now;
    pstat_to_mode_string (&e->pstat, mode);
    t = (time_t) e->pstat.ustat.st_mtime;
    time (&now);
    localtime_r (&t, &tm);
    if (t < now - 180 * 24 * 3600 || t > now)
        strftime (timebuf, sizeof (timebuf), "%b %d  %Y", &tm);
    else
        strftime (timebuf, sizeof (timebuf), "%b %d %H:%M", &tm);
    if (S_ISBLK (e->pstat.ustat.st_mode) || S_ISCHR (e->pstat.ustat.st_mode))
        printf ("%-10s %4lu %5lu %5lu   %3lu, %3lu  %s %s%s%s\n",
            mode, (unsigned long) e->pstat.ustat.st_nlink,
            (unsigned long) e->pstat.ustat.st_uid,
            (unsigned long) e->pstat.ustat.st_gid,
            (unsigned long) major (e->pstat.ustat.st_rdev),
            (unsigned long) minor (e->pstat.ustat.st_rdev),
            timebuf, e->name,
            target ? " -> " : "",
            target ? target : "");
    else
        printf ("%-10s %4lu %5lu %5lu %11lld %s %s%s%s\n",
            mode, (unsigned long) e->pstat.ustat.st_nlink,
            (unsigned long) e->pstat.ustat.st_uid,
            (unsigned long) e->pstat.ustat.st_gid,
            (long long) e->pstat.ustat.st_size,
            timebuf, e->name,
            target ? " -> " : "",
            target ? target : "");
}

static void ls_print_entry (struct file_item *e, const char *target)
{
    if (ls_opt_l) ls_print_long (e, target);
    else printf ("%s\n", e->name);
}

static int do_ls (const char *path_)
{
    char ip[256], dir_path[MAX_PATH_LEN];
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct remotefs *rfs;
    struct file_entry *list = NULL;
    int i, cached = 0;
    struct portable_stat st;
    int is_dir, exists, last_char_is_dir = 0;
    char symlink_target[MAX_PATH_LEN];
    int is_symlink = 0;
    int stat_failed = 0;
    parse_remote_path (path_, ip, sizeof (ip), dir_path, sizeof (dir_path));
    rfs = ip[0] ? remotefs_lookup (ip, NULL) : the_remotefs_local;
    if (path_stat (ip, dir_path, &st, &is_dir, &exists, symlink_target, sizeof (symlink_target), errmsg)) {
        stat_failed = 1;
    } else {
        is_symlink = (symlink_target[0] != '\0');
    }
    if (!stat_failed)
        strip_trailing_slash (dir_path, st.os, &last_char_is_dir);
    if (is_symlink && (stat_failed || !exists)) {
        struct file_item *e;
        e = (struct file_item *) malloc (sizeof (*e));
        memset (e, '\0', sizeof (*e));
        e->name = Cstrdup (dir_path);
        e->pstat.ustat.st_mode = S_IFLNK | 0777;
        e->pstat.ustat.st_size = strlen (symlink_target);
        ls_print_entry (e, symlink_target);
        free (e->name);
        free (e);
        return 0;
    }
    if (stat_failed) {
        fprintf (stderr, "Error stating %s: %s\n", path_, errmsg);
        return 1;
    }
    if (last_char_is_dir && !is_dir && !is_symlink) {
        fprintf (stderr, "Error: %s is not a directory\n", path_);
        return 1;
    }
    if (!exists) {
        fprintf (stderr, "Error: %s does not exist\n", path_);
        return 1;
    }
    if (is_symlink && !ls_opt_d && !is_dir && (!ls_opt_l || last_char_is_dir)) {
        char resolved[MAX_PATH_LEN + MAX_PATH_LEN];
        struct portable_stat target_st;
        int target_is_dir, target_exists2;
        if (symlink_target[0] == '/') {
            strncpy (resolved, symlink_target, sizeof (resolved) - 1);
            resolved[sizeof (resolved) - 1] = '\0';
        } else {
            const char *last_slash = strrchr (dir_path, '/');
            if (st.os == OS_TYPE_WINDOWS) {
                const char *bs = strrchr (dir_path, '\\');
                if (!last_slash || (bs && bs > last_slash))
                    last_slash = bs;
            }
            int parent_len = last_slash ? (int)(last_slash - dir_path) : 0;
            snprintf (resolved, sizeof (resolved), "%.*s/%s", parent_len, dir_path, symlink_target);
        }
        if (!path_stat (ip, resolved, &target_st, &target_is_dir, &target_exists2, NULL, 0, errmsg)
            && target_exists2 && target_is_dir) {
            strncpy (dir_path, resolved, sizeof (dir_path) - 1);
            dir_path[sizeof (dir_path) - 1] = '\0';
            is_dir = 1;
        }
    }
    if (ls_opt_d || !is_dir || (is_symlink && ls_opt_l && !last_char_is_dir)) {
        struct file_item *e;
        e = (struct file_item *) malloc (sizeof (*e));
        memset (e, '\0', sizeof (*e));
        e->name = Cstrdup (dir_path);
        e->pstat = st;
        if (is_symlink)
            e->pstat.ustat.st_size = strlen (symlink_target);
        ls_print_entry (e, is_symlink ? symlink_target : NULL);
        free (e->name);
        free (e);
        return 0;
    }
    if ((*rfs->remotefs_listdir) (rfs, &cached, dir_path, FILELIST_ALL_FILES, "*", &list, errmsg)) {
        fprintf (stderr, "Error listing %s: %s\n", path_, errmsg);
        return 1;
    }
    qsort (list->d, list->dl, sizeof (struct file_item *), ls_cmp_ptr);
    for (i = 0; i < list->dl; i++) {
        if (!ls_opt_a && list->d[i]->name[0] == '.') continue;
        if (S_ISLNK (list->d[i]->pstat.ustat.st_mode))
            ls_print_entry (list->d[i], list->d[i]->link_target);
        else {
            ls_print_entry (list->d[i], NULL);
        }
    }
    file_array_free (list);
    return 0;
}

static int do_ls_multi (int npaths, char **paths)
{
    int i, ret = 0;
    if (npaths == 0) return do_ls (".");
    for (i = 0; i < npaths; i++) {
        int show_header = 0;
        if (npaths > 1 && !ls_opt_d) {
            int r;
            char ip[256], dir_path[MAX_PATH_LEN];
            struct portable_stat st;
            int is_dir, exists, last_char_is_dir = 0;
            char errmsg[REMOTEFS_ERR_MSG_LEN];
            parse_remote_path (paths[i], ip, sizeof (ip), dir_path, sizeof (dir_path));
            r = path_stat (ip, dir_path, &st, &is_dir, &exists, NULL, 0, errmsg);
            if (!r)
                strip_trailing_slash (dir_path, st.os, &last_char_is_dir);
            if (!r && exists && is_dir)
                show_header = 1;
            if (r) {
                fprintf (stderr, "Error: %s: %s\n", paths[i], errmsg);
                ret = 1;
            } else if (last_char_is_dir && !is_dir) {
                fprintf (stderr, "Error: %s is not a directory\n", paths[i]);
                ret = 1;
            }
        }
        if (show_header) printf ("%s:\n", paths[i]);
        if (do_ls (paths[i])) ret = 1;
        if (i < npaths - 1 && show_header) printf ("\n");
    }
    return ret;
}

static int filetool_process_args_ (int argc, char **argv);

/* returns 1 if the envocation is not --filetool */
int filetool_process_args (int argc, char **argv)
{E_
    int r;

    if (argc < 2)
        return 1;

    if (strcmp (argv[1], "--filetool"))
        return 1;

    r = filetool_process_args_ (argc - 2, argv + 2);
    
    filetool_clean ();
    exit (r);
}

static int filetool_process_args_ (int argc, char **argv)
{E_
    int i, j, nsrcs;
    char **srcs;
    const char *dst;
    int dst_is_dir, dst_exists;
    struct portable_stat dst_st;
    char errmsg[REMOTEFS_ERR_MSG_LEN];

    /* parse options */
    for (i = 0; i < argc; i++) {
        if (!strcmp (argv[i], "--")) {
            i++;
            break;
        }
        if (!strcmp (argv[i], "-f") || !strcmp (argv[i], "--force")) {
            force_flag = 1;
            continue;
        }
        if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--verbose")) {
            verbose_flag = 1;
            continue;
        }
        if (!strcmp (argv[i], "--ls") || !strcmp (argv[i], "-ls")) {
            ls_flag = 1;
            i++;
            break;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "Unknown option: %s\n", argv[i]);
            usage_exit_error ();
        }
        break;
    }

    /* parse ls options after --ls / -ls */
    while (ls_flag && i < argc && argv[i][0] == '-') {
        const char *p = argv[i] + 1;
        if (!*p)
            break;
        while (*p) {
            switch (*p) {
                case 'a': ls_opt_a = 1; break;
                case 'd': ls_opt_d = 1; break;
                case 'l': ls_opt_l = 1; break;
                case 'r': ls_opt_r = 1; break;
                case 'S': ls_opt_S = 1; break;
                case 't': ls_opt_t = 1; break;
                case '1': ls_opt_1 = 1; break;
                default:
                    fprintf (stderr, "Unknown ls option: %c\n", *p);
                    usage_exit_error ();
            }
            p++;
        }
        i++;
    }

    for (j = 0; j < argc; j++)
        if (!argv[j][0])
            usage_exit_error ();

    if (ls_flag) {
        int r;
        get_home_dir ();
        filetool_password_init ();
        r = do_ls_multi (argc - i, argv + i);
        filetool_clean ();
        exit (r);
    }

    if (i >= argc)
        usage_exit_error ();
    srcs = &argv[i];
    nsrcs = argc - i - 1;
    if (nsrcs < 1)
        usage_exit_error ();
    dst = argv[argc - 1];

    if (is_cross_remote (nsrcs, srcs, dst)) {
        fprintf (stderr, "Error: IP-to-IP copy is not supported\n");
        return 1;
    }

    get_home_dir ();
    filetool_password_init ();

    /* multi-source: destination must exist and be a directory */
    if (nsrcs > 1) {
        char ip[256], dir_path[MAX_PATH_LEN];
        parse_remote_path (dst, ip, sizeof (ip), dir_path, sizeof (dir_path));
        if (path_stat (ip, dir_path, &dst_st, &dst_is_dir, &dst_exists, NULL, 0, errmsg)) {
            fprintf (stderr, "Error stating destination %s: %s\n", dst, errmsg);
            return 1;
        }
        strip_trailing_slash (dir_path, dst_st.os, NULL);
        if (!dst_exists || !dst_is_dir) {
            fprintf (stderr, "Error: with multiple sources, destination %s must be an existing directory\n", dst);
            return 1;
        }
    }

    for (i = 0; i < nsrcs; i++) {
        if (handle_single_source (srcs[i], dst))
            return 1;
    }

    if (verbose_flag)
        printf ("\n");

    return 0;
}




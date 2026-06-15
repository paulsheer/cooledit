/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* filetool.c
   Copyright (C) 1996-2022 Paul Sheer
 */

#include <stdio.h>
#include <errno.h>

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

extern int option_save_mode;
extern char *option_backup_ext;

/* --- helpers for new CLI --- */

static void parse_remote_path (const char *arg, char *ip, int ip_len, char *path, int path_len, int *last_char_is_dir)
{
    const char *colon;
    int len;

    if (last_char_is_dir)
        *last_char_is_dir = 0;

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
    /* strip trailing slashes (Windows APIs reject them) */
    len = strlen (path);
    if (!last_char_is_dir) {
        /* other than the top-level, we can't get trailing / or \ */
    } else if (len > 1 && path[len - 1] == '/') {
        *last_char_is_dir = 1;
        while (len > 1 && (path[len - 1] == '/'))
            path[--len] = '\0';
    } else if (len > 1 && path[len - 1] == '\\') {
        *last_char_is_dir = 1;
        while (len > 1 && (path[len - 1] == '\\'))
            path[--len] = '\0';
    }
}

static int path_stat (const char *arg, struct portable_stat *st, int *is_dir, int *exists, char *errmsg)
{
    char ip[256], path[MAX_PATH_LEN];
    struct remotefs *rfs;
    remotefs_error_code_t error_code;
    int just_not_there = 0;
    int last_char_is_dir = 0;

    *exists = 0;
    *is_dir = 0;
    memset (st, 0, sizeof (*st));

    parse_remote_path (arg, ip, sizeof (ip), path, sizeof (path), &last_char_is_dir);
    rfs = ip[0] ? remotefs_lookup (ip, NULL) : the_remotefs_local;

    if ((*rfs->remotefs_stat) (rfs, NULL, path, st, &just_not_there, &error_code, errmsg))
        return -1;

    if (just_not_there)
        return 0;

    *exists = 1;
    *is_dir = S_ISDIR (st->ustat.st_mode);
    return 0;
}

static int path_readlink (const char *arg, char *target, int target_len, char *errmsg)
{
    char ip[256], path[MAX_PATH_LEN];
    struct remotefs *rfs;
    parse_remote_path (arg, ip, sizeof (ip), path, sizeof (path), NULL);
    rfs = ip[0] ? remotefs_lookup (ip, NULL) : the_remotefs_local;
    return (*rfs->remotefs_readlink) (rfs, path, target, target_len, errmsg);
}

static const char *my_basename (const char *path)
{
    const char *p = strrchr (path, '/');
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
    long totalwritten;
    long filelen;
    int done;
};

static int filetool_sock_writer (struct action_callbacks *o, unsigned char *chunk, int *chunklen_, char *errmsg)
{E_
    struct saver_data *sd;
    int c;
    sd = (struct saver_data *) o->hook;

    if (sd->done || sd->totalwritten >= sd->filelen) {
        strcpy (errmsg, "%s: Unknown error");
        return -1;
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

    if (fstat (fileno (sd.f), &local_st)) {
        fclose (sd.f);
        perror (local_filename);
        return 1;
    }

    memset (&o, '\0', sizeof (o));

    sd.filelen = local_st.st_size;

    o.hook = (void *) &sd;
    o.sock_writer = filetool_sock_writer;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_writefile) (u, &o, remote_filename, local_st.st_size, option_save_mode, DEFAULT_CREATE_MODE, option_backup_ext, &st, errmsg)) {
        fprintf (stderr, "%s: Failed trying to write file: %s\n", remote_filename, errmsg);
        fclose (sd.f);
        return 1;
    }

    if (sd.totalwritten != local_st.st_size) {
        fprintf (stderr, "%s: Error: Did not write all bytes: ", local_filename);
        fclose (sd.f);
        return 1;
    }

    fclose (sd.f);
    return 0;
}

void filetool_usage(FILE *out, const char *prefix)
{
    fprintf(out, "%s\
--filetool [-f|--force] <src> [<src>...] <target>      scp-like remote copy with\n\
                                         recursive directory copy feature. Uses\n\
                                         remotefs as a server.\n", prefix);
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
        if (!crypto_enabled && contains_whitespace (pass)) {
            pass[0] = '\0';
        } else if (contains_whitespace (pass)) {
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
    int n = 0, i, cached = 0;
    struct remotefs *rfs;
    char sub_local[MAX_PATH_LEN], sub_remote[MAX_PATH_LEN];

    rfs = remotefs_lookup (host, NULL);

    /* create destination directory on remote */
    if ((*rfs->remotefs_mkdir) (rfs, remote_dir, 0777, errmsg)) {
        fprintf (stderr, "Error creating remote directory %s: %s\n", remote_dir, errmsg);
        return 1;
    }

    /* list local directory */
    {
        struct remotefs *local_rfs = the_remotefs_local;
        if ((*local_rfs->remotefs_listdir) (local_rfs, &cached, local_dir, FILELIST_ALL_FILES, "*", &list, &n, errmsg)) {
            fprintf (stderr, "Error listing directory %s: %s\n", local_dir, errmsg);
            return 1;
        }
    }

    for (i = 0; i < n; i++) {
        if (list[i].options & FILELIST_LAST_ENTRY) break;
        if (!strcmp (list[i].name, ".") || !strcmp (list[i].name, ".."))
            continue;

        path_join (local_dir, list[i].name, sub_local, sizeof (sub_local));
        path_join (remote_dir, list[i].name, sub_remote, sizeof (sub_remote));

        if (S_ISDIR (list[i].pstat.ustat.st_mode)) {
            if (copy_dir_local_to_remote (sub_local, host, sub_remote, force))
                return 1;
        } else if (S_ISREG (list[i].pstat.ustat.st_mode)) {
            if (filetool_copy_local_to_remote (sub_local, host, sub_remote))
                return 1;
        } else if (S_ISLNK (list[i].pstat.ustat.st_mode)) {
            char link_target[MAX_PATH_LEN];
            ssize_t nlink;
            nlink = readlink (sub_local, link_target, sizeof (link_target) - 1);
            if (nlink < 0) {
                fprintf (stderr, "Error reading symlink %s: %s\n", sub_local, strerror (errno));
                return 1;
            }
            link_target[nlink] = '\0';
            if ((*rfs->remotefs_symlink) (rfs, link_target, sub_remote, errmsg)) {
                fprintf (stderr, "Error creating remote symlink %s: %s\n", sub_remote, errmsg);
                return 1;
            }
        }
    }

    free (list);
    return 0;
}

static int copy_dir_remote_to_local (const char *host, const char *remote_dir, const char *local_dir, int force)
{
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct file_entry *list = NULL;
    int n = 0, i, cached = 0;
    struct remotefs *rfs;
    char sub_remote[MAX_PATH_LEN], sub_local[MAX_PATH_LEN];

    rfs = remotefs_lookup (host, NULL);

    /* create destination directory locally */
    if (mkdir (local_dir, 0777) < 0 && errno != EEXIST) {
        fprintf (stderr, "Error creating directory %s: %s\n", local_dir, strerror (errno));
        return 1;
    }

    /* list remote directory */
    if ((*rfs->remotefs_listdir) (rfs, &cached, remote_dir, FILELIST_ALL_FILES, "*", &list, &n, errmsg)) {
        fprintf (stderr, "Error listing remote directory %s: %s\n", remote_dir, errmsg);
        return 1;
    }

    for (i = 0; i < n; i++) {
        if (list[i].options & FILELIST_LAST_ENTRY) break;
        if (!strcmp (list[i].name, ".") || !strcmp (list[i].name, ".."))
            continue;

        path_join (remote_dir, list[i].name, sub_remote, sizeof (sub_remote));
        path_join (local_dir, list[i].name, sub_local, sizeof (sub_local));

        if (S_ISDIR (list[i].pstat.ustat.st_mode)) {
            if (copy_dir_remote_to_local (host, sub_remote, sub_local, force))
                return 1;
        } else if (S_ISREG (list[i].pstat.ustat.st_mode)) {
            if (filetool_copy_remote_to_local (host, sub_remote, sub_local))
                return 1;
        } else if (S_ISLNK (list[i].pstat.ustat.st_mode)) {
            char link_target[MAX_PATH_LEN];
            if ((*rfs->remotefs_readlink) (rfs, sub_remote, link_target, sizeof (link_target), errmsg)) {
                fprintf (stderr, "Error reading remote symlink %s: %s\n", sub_remote, errmsg);
                return 1;
            }
            if ((*the_remotefs_local->remotefs_symlink) (the_remotefs_local, link_target, sub_local, errmsg)) {
                fprintf (stderr, "Error creating symlink %s: %s\n", sub_local, errmsg);
                return 1;
            }
        }
    }

    free (list);
    return 0;
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
    int src_is_dir, src_exists, dst_is_dir, dst_exists;
    char target_path[MAX_PATH_LEN];
    const char *target;
    int src_is_remote, dst_is_remote;
    int last_src_char_is_dir = 0, last_dst_char_is_dir = 0;

    *errmsg = '\0';
    parse_remote_path (src, src_ip, sizeof (src_ip), src_path, sizeof (src_path), &last_src_char_is_dir);
    parse_remote_path (dst, dst_ip, sizeof (dst_ip), dst_path, sizeof (dst_path), &last_dst_char_is_dir);

    src_is_remote = (src_ip[0] != '\0');
    dst_is_remote = (dst_ip[0] != '\0');

    /* stat source */
    if (path_stat (src, &src_st, &src_is_dir, &src_exists, errmsg)) {
        fprintf (stderr, "Error stating source %s: %s\n", src, errmsg);
        return 1;
    }

    if (last_src_char_is_dir && !src_is_dir) {
        fprintf (stderr, "Error %s is not a directory\n", src);
        return 1;
    }

    /* stat destination */
    if (path_stat (dst, &dst_st, &dst_is_dir, &dst_exists, errmsg)) {
        fprintf (stderr, "Error stating destination %s: %s\n", dst, errmsg);
        return 1;
    }

    if (last_dst_char_is_dir && !dst_is_dir) {
        fprintf (stderr, "Error %s is not a directory\n", dst);
        return 1;
    }

    /* Check if source is a symlink — symlinks are reproduced, not followed */
    {
        char link_target[MAX_PATH_LEN];
        if (!path_readlink (src, link_target, sizeof (link_target), errmsg)) {
            if (dst_exists && dst_is_dir)
                target = target_path, path_join (dst_path, my_basename (src_path), target_path, sizeof (target_path));
            else
                target = dst_path;
            if (dst_exists && !dst_is_dir) {
                if (!confirm_overwrite (dst))
                    return 0;
            }
            if (dst_is_remote) {
                struct remotefs *rfs = remotefs_lookup (dst_ip, NULL);
                if ((*rfs->remotefs_symlink) (rfs, link_target, target, errmsg)) {
                    fprintf (stderr, "Error creating remote symlink %s: %s\n", target, errmsg);
                    return 1;
                }
            } else {
                if ((*the_remotefs_local->remotefs_symlink) (the_remotefs_local, link_target, target, errmsg)) {
                    fprintf (stderr, "Error creating symlink %s: %s\n", target, errmsg);
                    return 1;
                }
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
        if (dst_exists && dst_is_dir)
            target = target_path, path_join (dst_path, my_basename (src_path), target_path, sizeof (target_path));
        else
            target = dst_path;

        if (!dst_is_remote) {
            if (copy_dir_remote_to_local (src_ip, src_path, target, force_flag))
                return 1;
        } else {
            if (copy_dir_local_to_remote (src_path, dst_ip, target, force_flag))
                return 1;
        }
    } else {
        /* file source: cases 1-4 */
        if (dst_exists && dst_is_dir)
            target = target_path, path_join (dst_path, my_basename (src_path), target_path, sizeof (target_path));
        else
            target = dst_path;

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
        }
    }
    return 0;
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
        if (!strcmp (argv[i], "-f") || !strcmp (argv[i], "--force")) {
            force_flag = 1;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "Unknown option: %s\n", argv[i]);
            usage_exit_error ();
        }
        break;
    }

    for (j = 0; j < argc; j++)
        if (!argv[j][0])
            usage_exit_error ();

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
        if (path_stat (dst, &dst_st, &dst_is_dir, &dst_exists, errmsg)) {
            fprintf (stderr, "Error stating destination %s: %s\n", dst, errmsg);
            return 1;
        }
        if (!dst_exists || !dst_is_dir) {
            fprintf (stderr, "Error: with multiple sources, destination %s must be an existing directory\n", dst);
            return 1;
        }
    }

    for (i = 0; i < nsrcs; i++)
        if (handle_single_source (srcs[i], dst))
            return 1;

    return 0;
}




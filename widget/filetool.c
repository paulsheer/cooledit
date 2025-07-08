/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* filetool.c
   Copyright (C) 1996-2022 Paul Sheer
 */

#include "inspect.h"
#include <config.h>
#include "stringtools.h"
#include "dirtools.h"
#include "remotefs.h"
#include "filetool.h"
#include "remotefspassword.h"

#include <stdio.h>


char *get_sys_error (const char *s);
void get_home_dir (void);

#define DEFAULT_CREATE_MODE            (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static int password_loaded = 0;
static int dummy_data;

extern int option_save_mode;
extern char *option_backup_ext;

struct loader_data {
    long total;
    int done;
    FILE *f;
    const char *fname;
};

static int filetool_sock_reader (struct action_callbacks *o, const unsigned char *buf, int buflen, long long filelen, char *errmsg)
{E_
    struct loader_data *ld;

    ld = (struct loader_data *) o->hook;

    if (ld->done) {
        strcpy (errmsg, "File size changed while loading");
        return -1;
    }

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
        exit (1);
    }

    memset (&o, '\0', sizeof (o));

    o.hook = (void *) &ld;
    o.sock_reader = filetool_sock_reader;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_readfile) (u, &o, remote_filename, errmsg)) {
        fprintf (stderr, "%s: Failed trying to open file for reading: %s\n", remote_filename, errmsg);
        return 1;
    }

    if (fclose (ld.f))
        fprintf (stderr, "%s: Error closing file: %s\n", local_filename, get_sys_error (""));

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
        exit (1);
    }

    if (fstat (fileno (sd.f), &local_st)) {
        perror (local_filename);
        exit (1);
    }

    memset (&o, '\0', sizeof (o));

    sd.filelen = local_st.st_size;

    o.hook = (void *) &sd;
    o.sock_writer = filetool_sock_writer;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_writefile) (u, &o, remote_filename, local_st.st_size, option_save_mode, DEFAULT_CREATE_MODE, option_backup_ext, &st, errmsg)) {
        fprintf (stderr, "%s: Failed trying to write file: %s\n", remote_filename, errmsg);
        return 1;
    }

    if (sd.totalwritten != local_st.st_size) {
        fprintf (stderr, "%s: Error: Did not write all bytes: ", local_filename);
        return 1;
    }

    return 0;
}

static void usage_ (void)
{E_
    fprintf(stderr, "Usage\n");
    fprintf(stderr, "    cooledit --filetool copy-from-remote <host-ip> <remote-file> <local-file>\n");
    fprintf(stderr, "    cooledit --filetool copy-to-remote <local-file> <host-ip> <remote-file>\n");
}

static void usage_exit_error (void)
{E_
    usage_ ();
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

    if (!password_loaded) {
        password_loaded = 1;
        if (password_load ()) {
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

int filetool_process_args (int argc, char **argv)
{E_
    int i;

    if (argc < 2)
        return 1;

    if (strcmp (argv[1], "--filetool"))
        return 1;

    if (argc < 3)
        usage_exit_error ();

    get_home_dir ();
    filetool_password_init ();

    for (i = 2; i < argc; i++) {
        if (!strcmp (argv[i], "copy-from-remote")) {
            if (argc != 6)
                usage_exit_error ();
            filetool_copy_remote_to_local (argv[i + 1], argv[i + 2], argv[i + 3]);
            return 0;
        }
        if (!strcmp (argv[i], "copy-to-remote")) {
            if (argc != 6)
                usage_exit_error ();
            filetool_copy_local_to_remote (argv[i + 1], argv[i + 2], argv[i + 3]);
            return 0;
        }
#warning here we should test very large directory sizes: they seem to fail
    }

    usage_exit_error ();
    return 1;
}





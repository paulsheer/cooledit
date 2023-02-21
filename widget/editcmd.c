/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* editcmd.c
   Copyright (C) 1996-2022 Paul Sheer
 */


/* #define PIPE_BLOCKS_SO_READ_BYTE_BY_BYTE */

#include "inspect.h"
#include <config.h>
#ifdef NEEDS_IO_H
#include <io.h>
#include <fcntl.h>
#endif
#include <assert.h>
#include <ctype.h>
#include "edit.h"
#include "stringtools.h"
#include "editcmddef.h"
#include "remotefs.h"

#ifndef MIDNIGHT
#include <X11/Xatom.h>
#ifndef GTK
#include "loadfile.h"
#endif
#endif

/* globals: */

/* queries on a save */
#ifdef MIDNIGHT
int edit_confirm_save = 1;
#else
int edit_confirm_save = 0;
#endif

#define NUM_REPL_ARGS 64
#define MAX_REPL_LEN            (256*1024)
#define SCANF_WORKSPACE         (256*1024)

#if defined(MIDNIGHT) || defined(GTK)

static inline int my_lower_case (int c)
{E_
    return tolower(c & 0xFF);
}

char *strcasechr (const unsigned char *s, int c)
{E_
    for (; my_lower_case ((int) *s) != my_lower_case (c); ++s)
	if (*s == '\0')
	    return 0;
    return (char *) s;
}

/* #define itoa MY_itoa  <---- this line is now in edit.h */
char *itoa (int i)
{E_
    static char t[14];
    char *s = t + 13;
    int j = i;
    *s-- = 0;
    do {
	*s-- = i % 10 + '0';
    } while ((i = i / 10));
    if (j < 0)
	*s-- = '-';
    return ++s;
}

/*
   This joins strings end on end and allocates memory for the result.
   The result is later automatically free'd and must not be free'd
   by the caller.
 */
char *catstrs (const char *first,...)
{E_
    static char *stacked[16] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static int i = 0;
    va_list ap;
    int len;
    char *data;

    if (!first)
	return 0;

    len = strlen (first);
    va_start (ap, first);

    while ((data = va_arg (ap, char *)) != 0)
	 len += strlen (data);

    len++;

    i = (i + 1) % 16;
    if (stacked[i])
	free (stacked[i]);

    stacked[i] = malloc (len);
    va_end (ap);
    va_start (ap, first);
    strcpy (stacked[i], first);
    while ((data = va_arg (ap, char *)) != 0)
	 strcat (stacked[i], data);
    va_end (ap);

    return stacked[i];
}
#endif

#ifdef MIDNIGHT

void edit_help_cmd (WEdit * edit)
{E_
    char *hlpdir = concat_dir_and_file (mc_home, "mc.hlp");
    interactive_display (hlpdir, "[Internal File Editor]");
    free (hlpdir);
    edit->force |= REDRAW_COMPLETELY;
}

void edit_refresh_cmd (WEdit * edit)
{E_
#ifndef HAVE_SLANG
    clr_scr();
    do_refresh();
#else
    {
	int fg, bg;
	edit_get_syntax_color (edit, -1, &fg, &bg);
    }
    touchwin(stdscr);
#endif
    mc_refresh();
    doupdate();
}

#else

void edit_help_cmd (WEdit * edit)
{E_
}

void edit_refresh_cmd (WEdit * edit)
{E_
    int fg, bg;
    edit_get_syntax_color (edit, -1, &fg, &bg);
    edit->force |= REDRAW_COMPLETELY;
}

void CRefreshEditor (WEdit * edit)
{E_
    edit_refresh_cmd (edit);
}

#endif

#define DEFAULT_CREATE_MODE		(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* three argument open */
static int open_create (const char *pathname, int flags, mode_t mode)
{E_
    int file;
    file = open ((char *) pathname, O_RDONLY, mode);
    if (file < 0 && (flags & O_CREAT))	/* must it be created ? */
	return creat ((char *) pathname, mode);
    if (file >= 0)
        close (file);
    return open ((char *) pathname, flags, mode);
}

/* "Oleg Yu. Repin" <repin@ssd.sscc.ru> added backup filenames
    ...thanks -paul */

/*  If 0 (quick save) then  a) create/truncate <filename> file,
			    b) save to <filename>;
    if 1 (safe save) then   a) save to <tempnam>,
			    b) rename <tempnam> to <filename>;
    if 2 (do backups) then  a) save to <tempnam>,
			    b) rename <filename> to <filename.backup_ext>,
			    c) rename <tempnam> to <filename>. */

/* returns 0 on error */

struct saver_data {
    const WEdit *edit;
    long totalwritten;
    long buf;
    long filelen;
    long curs2;
    unsigned char *p;
    int len;
    int step;
};

#undef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))

static int edit_sock_writer (struct action_callbacks *o, unsigned char *chunk, int *chunklen_, char *errmsg)
{E_
    struct saver_data *sd;
    const WEdit *edit;
    int chunklen;
    int avail;
    int c;

    chunklen = 0;
    avail = *chunklen_;

    sd = (struct saver_data *) o->hook;
    edit = sd->edit;

    switch (sd->step) {
    case 0:

#define COPYCHUNK(a) \
            c = MIN (avail, sd->len); \
            if (c > 0) \
                memcpy (chunk, sd->p, c); \
            sd->len -= c; \
            sd->p += c; \
            chunklen += c; \
            avail -= c; \
            chunk += c; \
            sd->totalwritten += c; \
            assert (sd->totalwritten <= sd->filelen); \
            if (!sd->len) { \
                a; \
                sd->p = NULL; \
            } \
            if (!avail) \
                goto out;

        while (sd->buf <= (edit->curs1 >> S_EDIT_BUF_SIZE) - 1) {
            if (!sd->p) {
                sd->p = edit->buffers1[sd->buf];
                sd->len = EDIT_BUF_SIZE;
            }
            COPYCHUNK (sd->buf++);
        }
        sd->step = 1;
        sd->p = NULL;
        sd->len = 0;
        /* fall through */

    case 1:
        if (!sd->p) {
            sd->buf = (edit->curs1 >> S_EDIT_BUF_SIZE);
            sd->p = edit->buffers1[sd->buf];
            sd->len = edit->curs1 & M_EDIT_BUF_SIZE;
        }
        COPYCHUNK (sd->step = 2);
        sd->step = 2;
        /* fall through */

    case 2:
        if (!edit->curs2) {
            sd->step = 10;
            goto out;
        }
        if (!sd->p) {
            sd->buf = (sd->curs2 >> S_EDIT_BUF_SIZE);
            sd->p = edit->buffers2[sd->buf] + EDIT_BUF_SIZE - (sd->curs2 & M_EDIT_BUF_SIZE) - 1;
            sd->len = 1 + (sd->curs2 & M_EDIT_BUF_SIZE);
        }
        COPYCHUNK (sd->step = 3);
        sd->step = 3;
        /* fall through */

    case 3:
        sd->buf = (sd->curs2 >> S_EDIT_BUF_SIZE) - 1;
        sd->step = 4;
        /* fall through */

    case 4:
        while (sd->buf >= 0) {
            if (!sd->p) {
                sd->p = edit->buffers2[sd->buf];
                sd->len = EDIT_BUF_SIZE;
            }
            COPYCHUNK (sd->buf--);
        }
        sd->step = 10;
        sd->p = NULL;
        sd->len = 0;
        /* fall through */

    case 10:
        /* done */
        break;
    }

  out:
    *chunklen_ = chunklen;

    return 0;
}

int edit_save_file (WEdit * edit, const char *host, const char *filename)
{E_
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct saver_data sd;
    struct action_callbacks o;
    struct portable_stat st;
    struct remotefs *u;

    if (!filename)
	return 0;
    if (!*filename)
	return 0;

    memset (&sd, '\0', sizeof (sd));
    memset (&o, '\0', sizeof (o));

    sd.edit = edit;
    sd.filelen = edit->last_byte;
    sd.curs2 = edit->curs2 - 1;

    o.hook = (void *) &sd;
    o.sock_writer = edit_sock_writer;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_writefile) (u, &o, filename, edit->last_byte, option_save_mode, DEFAULT_CREATE_MODE, option_backup_ext, &st, errmsg)) {
        edit_error_dialog (_(" Error "), catstrs (_(" Failed trying to write file: "), filename, " \n [", errmsg, "]", NULL));
        return 0;
    }

    if (sd.totalwritten != edit->last_byte) {
        edit_error_dialog (_(" Error "), catstrs (_(" Did not write all bytes: "), filename, NULL));
        return 0;
    }

    edit->stat = st;
    edit->test_file_on_disk_for_changes_m_time = st.ustat.st_mtime;

    return 1;
}

/* returns 0 on ignore.  for save_mode=2 returns 0 on cancel */
int edit_check_change_on_disk (WEdit * edit, int save_mode)
{E_
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    remotefs_error_code_t error_code;
    char *fullname;
    struct portable_stat st;
    int r = 0;
    struct remotefs *u;

    memset(&st, '\0', sizeof(st));
    if (!edit->filename || !*edit->filename || !edit->dir || !*edit->dir)
        return 0;
    fullname = (char *) strdup(catstrs (edit->dir, edit->filename, NULL));
    u = remotefs_lookup (edit->host, NULL);
#warning should abort on network error
    if (!(*u->remotefs_stat) (u, fullname, &st, NULL, &error_code, errmsg)) {
        if (st.ustat.st_mtime != (save_mode == EDIT_CHANGE_ON_DISK__ON_SAVE ? edit->stat.ustat.st_mtime : edit->test_file_on_disk_for_changes_m_time)) {
            const char *p;
            p = _ (" File has been modified on disk by another program. ");
            if (save_mode == EDIT_CHANGE_ON_DISK__ON_COMMAND) {
                int c;
                c = edit_query_dialog3 (_(" Warning "), p, _("Open"), _("Ignore"), _("Cancel"));
                if (c == 0) {
                    r = 1;
                    edit_load_cmd (edit);
                } else if (c == 1) {
                    edit->test_file_on_disk_for_changes_m_time = st.ustat.st_mtime;
                    /* edit->stat.st_mtime = st.st_mtime; ===>  [Ignore] after keypress should still result in a query on Save */
                } else {
                    r = 1;
                }
            } else if (save_mode == EDIT_CHANGE_ON_DISK__ON_SAVE) {
                if (!edit_query_dialog2 (_(" Warning "), p, _("Open"), _("Overwrite"))) {
                    r = 1;
                    edit_load_cmd (edit);
                } else {
                    edit->test_file_on_disk_for_changes_m_time = st.ustat.st_mtime;
                    edit->stat.ustat.st_mtime = st.ustat.st_mtime;
                }
            } else if (save_mode == EDIT_CHANGE_ON_DISK__ON_KEYPRESS) {
                if (!edit_query_dialog2 (_(" Warning "), p, _("Open"), _("Ignore"))) {
                    r = 1;
                    edit_load_cmd (edit);
                } else {
                    edit->test_file_on_disk_for_changes_m_time = st.ustat.st_mtime;
                    /* edit->stat.st_mtime = st.st_mtime; ===>  [Ignore] after keypress should still result in a query on Save */
                }
            }
        }
    }
    free(fullname);
    return r;
}

#ifdef MIDNIGHT
/*
   I changed this from Oleg's original routine so
   that option_backup_ext works with coolwidgets as well. This
   does mean there is a memory leak - paul.
 */
void menu_save_mode_cmd (void)
{E_
#define DLG_X 38
#define DLG_Y 10
    static char *str_result;
    static int save_mode_new;
    static char *str[] =
    {
	N_("Quick save "),
	N_("Safe save "),
	N_("Do backups -->")};
    static QuickWidget widgets[] =
    {
	{quick_button, 18, DLG_X, 7, DLG_Y, N_("&Cancel"), 0,
	 B_CANCEL, 0, 0, XV_WLAY_DONTCARE, "c"},
	{quick_button, 6, DLG_X, 7, DLG_Y, N_("&Ok"), 0,
	 B_ENTER, 0, 0, XV_WLAY_DONTCARE, "o"},
	{quick_input, 23, DLG_X, 5, DLG_Y, 0, 9,
	 0, 0, &str_result, XV_WLAY_DONTCARE, "i"},
	{quick_label, 22, DLG_X, 4, DLG_Y, N_("Extension:"), 0,
	 0, 0, 0, XV_WLAY_DONTCARE, "savemext"},
	{quick_radio, 4, DLG_X, 3, DLG_Y, "", 3,
	 0, &save_mode_new, str, XV_WLAY_DONTCARE, "t"},
	{0}};
    static QuickDialog dialog =
/* NLS ? */
    {DLG_X, DLG_Y, -1, -1, N_(" Edit Save Mode "), "[Edit Save Mode]",
     "esm", widgets};
    static int i18n_flag = 0;

    if (!i18n_flag) {
        int i;
        
        for (i = 0; i < 3; i++ )
            str[i] = _(str[i]);
        i18n_flag = 1;
    }

    widgets[2].text = option_backup_ext;
    widgets[4].value = option_save_mode;
    if (quick_dialog (&dialog) != B_ENTER)
	return;
    option_save_mode = save_mode_new;
    option_backup_ext = str_result;	/* this is a memory leak */
    option_backup_ext_int = 0;
    str_result[min (strlen (str_result), sizeof (int))] = '\0';
    memcpy ((char *) &option_backup_ext_int, str_result, strlen (option_backup_ext));
}

#endif

#ifdef MIDNIGHT

void edit_split_filename (WEdit * edit, char *f)
{E_
    if (edit->filename)
	free (edit->filename);
    edit->filename = (char *) strdup (f);
    if (edit->dir)
	free (edit->dir);
    edit->dir = (char *) strdup ("");
}

#else

#ifdef GTK

static char cwd[1040];

static char *canonicalize_pathname (char *p)
{E_
    char *q, *r;

    if (*p != '/') {
	if (strlen (cwd) == 0) {
#ifdef HAVE_GETCWD
	    getcwd (cwd, MAX_PATH_LEN);         /* M/G */
#else
	    getwd (cwd);                        /* M/G */
#endif
	}
	r = malloc (strlen (cwd) + strlen (p) + 2);
	strcpy (r, cwd);
	strcat (r, "/");
	strcat (r, p);
	p = r;
    }
    r = q = malloc (strlen (p) + 2);
    for (;;) {
	if (!*p) {
	    *q = '\0';
	    break;
	}
	if (*p != '/') {
	    *q++ = *p++;
	} else {
	    while (*p == '/') {
		*q = '/';
		if (!strncmp (p, "/./", 3) || !strcmp (p, "/."))
		    p++;
		else if (!strncmp (p, "/../", 4) || !strcmp (p, "/..")) {
		    p += 2;
		    *q = ' ';
		    q = strrchr (r, '/');
		    if (!q) {
			q = r;
			*q = '/';
		    }
		}
		p++;
	    }
	    q++;
	}
    }
/* get rid of trailing / */
    if (r[0] && r[1])
	if (*--q == '/')
	    *q = '\0';
    return r;
}

#endif		/* GTK */


int edit_split_filename (WEdit * edit, const char *host, const char *longname)
{E_
    char *exp, *p;
#if defined(MIDNIGHT) || defined(GTK)
    exp = canonicalize_pathname (longname);
#else
    char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
    exp = pathdup (host, longname, errmsg);	/* this ensures a full path */
    if (!exp) {
        char msg[REMOTEFS_ERR_MSG_LEN + 28];
        snprintf (msg, sizeof (msg), " Error connecting to %s: %s ", host, errmsg);
	edit_error_dialog (" Resolving Path ", msg);
        return 1;
    }
#endif
    if (edit->filename)
	free (edit->filename);
    if (edit->dir)
	free (edit->dir);
    if (edit->host)
	free (edit->host);
    p = strrchr (exp, '/');
    edit->filename = (char *) strdup (++p);
    *p = 0;
    edit->dir = (char *) strdup (exp);
    edit->host = (char *) strdup (host);
    free (exp);
    return 0;
}

#endif		/* ! MIDNIGHT */

/*  here we want to warn the user of overwriting an existing file, but only if they
   have made a change to the filename */
/* returns 1 on success */
int edit_save_as_cmd (WEdit * edit)
{E_
/* This heads the 'Save As' dialog box */
    char *exp = 0;
    int different_filename = 0;
    char host[256];
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    remotefs_error_code_t error_code;

    strcpy (host, edit->host);
    exp = edit_get_save_file (edit->dir, edit->filename, _(" Save As "), host);
    edit_push_action (edit, KEY_PRESS, edit->start_display);

    if (exp) {
	if (!*exp) {
	    free (exp);
	    edit->force |= REDRAW_COMPLETELY;
	    return 0;
	} else {
	    if (strcmp(catstrs (edit->dir, edit->filename, NULL), exp) || strcmp (edit->host, host)) {
                struct portable_stat st;
                struct remotefs *u;
		different_filename = 1;
                u = remotefs_lookup (host, NULL);
#warning should report on network error
                if (!(*u->remotefs_stat) (u, exp, &st, NULL, &error_code, errmsg)) {     /* the file exists */
		    if (edit_query_dialog2 (_(" Warning "), 
		    _(" A file already exists with this name. "), 
/* Push buttons to over-write the current file, or cancel the operation */
		    _("Overwrite"), _("Cancel"))) {
			edit->force |= REDRAW_COMPLETELY;
			return 0;
		    }
		}
	    }
	    if (edit_save_file (edit, host, exp)) {
                if (edit_split_filename (edit, host, exp)) {
		    free (exp);
		    edit->force |= REDRAW_COMPLETELY;
		    return 0;
                }
		free (exp);
		edit->modified = 0;
#if defined(MIDNIGHT) || defined(GTK)
	        edit->delete_file = 0;
#endif		
		if (different_filename && !edit->explicit_syntax)
		    edit_load_syntax (edit, 0, 0);
		edit->force |= REDRAW_COMPLETELY;
		return 1;
	    } else {
		free (exp);
		edit->force |= REDRAW_COMPLETELY;
		return 0;
	    }
	}
    }
    edit->force |= REDRAW_COMPLETELY;
    return 0;
}

/* {{{ Macro stuff starts here */

#ifdef MIDNIGHT
int raw_callback (struct Dlg_head *h, int key, int Msg)
{E_
    switch (Msg) {
    case DLG_DRAW:
	attrset (REVERSE_COLOR);
	dlg_erase (h);
	draw_box (h, 1, 1, h->lines - 2, h->cols - 2);

	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 1, 2);
	printw (h->title);
	break;

    case DLG_KEY:
	h->running = 0;
	h->ret_value = key;
	return 1;
    }
    return 0;
}

/* gets a raw key from the keyboard. Passing cancel = 1 draws
   a cancel button thus allowing c-c etc.. Alternatively, cancel = 0 
   will return the next key pressed */
int edit_raw_key_query (char *heading, char *query, int cancel)
{E_
    int w = strlen (query) + 7;
    struct Dlg_head *raw_dlg = create_dlg (0, 0, 7, w, dialog_colors,
/* NLS ? */
					 raw_callback, "[Raw Key Query]",
					   "raw_key_input",
					   DLG_CENTER | DLG_TRYUP);
    x_set_dialog_title (raw_dlg, heading);
    raw_dlg->raw = 1;		/* to return even a tab key */
    if (cancel)
	add_widget (raw_dlg, button_new (4, w / 2 - 5, B_CANCEL, NORMAL_BUTTON, _("Cancel"), 0, 0, 0));
    add_widget (raw_dlg, label_new (3 - cancel, 2, query, 0));
    add_widget (raw_dlg, input_new (3 - cancel, w - 5, INPUT_COLOR, 2, "", 0));
    run_dlg (raw_dlg);
    w = raw_dlg->ret_value;
    destroy_dlg (raw_dlg);
    if (cancel)
	if (w == XCTRL ('g') || w == XCTRL ('c') || w == ESC_CHAR || w == B_CANCEL)
	    return 0;
/* hence ctrl-a (=B_CANCEL), ctrl-g, ctrl-c, and Esc are cannot returned */
    return w;
}

#else

int edit_raw_key_query (const char *heading, const char *query, int cancel)
{E_
#ifdef GTK
    /* *** */
    return 0;
#else
    return CKeySymMod (CRawkeyQuery (0, 0, 0, heading, "%s", query));
#endif
}

#endif

/* creates a macro file if it doesn't exist */
static FILE *edit_open_macro_file (const char *r)
{E_
    char *filename;
    int file;
    filename = catstrs (local_home_dir, MACRO_FILE, NULL);
    if ((file = open_create (filename, O_CREAT | O_RDWR, DEFAULT_CREATE_MODE)) == -1)
	return 0;
    close (file);
    return fopen (filename, r);
}

#define MAX_MACROS 1024
static int saved_macro[MAX_MACROS + 1] =
{0, 0};
static int saved_macros_loaded = 0;

/*
   This is just to stop the macro file be loaded over and over for keys
   that aren't defined to anything. On slow systems this could be annoying.
 */
int macro_exists (int k)
{E_
    int i;
    for (i = 0; i < MAX_MACROS && saved_macro[i]; i++)
	if (saved_macro[i] == k)
	    return i;
    return -1;
}

static void macro_print_line (FILE *f, int s, struct macro_rec *macro)
{E_
    int i;
    fprintf (f, _("key '%d 0': "), s);
    for (i = 0; i < macro->macro_i; i++) {
	int j;
	fprintf (f, "%d", macro->macro[i].command);
	if (!macro->macro[i].ch.len)
	    fprintf (f, " -1");
	for (j = 0; j < macro->macro[i].ch.len; j++)
	    fprintf (f, " %d", (int) (unsigned char) macro->macro[i].ch.data[j]);
	fprintf (f, ", ");
    }
    fprintf (f, ";\n");
}

static int macro_scan_line (FILE * f, int *s, struct macro_rec *macro)
{E_
    int u;
    memset (macro, '\0', sizeof (*macro));
    u = fscanf (f, _("key '%d 0': "), s);
    if (!u || u == EOF)
	return 1;
    for (macro->macro_i = 0; macro->macro_i < MAX_MACRO_LENGTH; macro->macro_i++) {
	unsigned char ch[1024];
	int ch_n = 0;
	if (!(1 == fscanf (f, "%d", &macro->macro[macro->macro_i].command)))
	    break;
	for (;;) {
	    int v;
	    if (!(1 == fscanf (f, " %d", &v)))
		break;
	    if (v >= 0 && ch_n < 1024)
		ch[ch_n++] = (unsigned char) v;
	}
	u = fscanf (f, ", ");
	(void) u;
	macro->macro[macro->macro_i].ch = CStr_cpy ((const char *) ch, ch_n);
    }
    u = fscanf (f, ";\n");
    (void) u;
    return 0;
}

/* returns 1 on error */
int edit_delete_macro (WEdit * edit, int k)
{E_
    FILE *f, *g;
    int s, j = 0;

    if (saved_macros_loaded)
	if ((j = macro_exists (k)) < 0)
	    return 0;

    g = fopen (catstrs (local_home_dir, TEMP_FILE, NULL), "w");
    if (!g) {
/* This heads the delete macro error dialog box */
	edit_error_dialog (_(" Delete macro "),
/* 'Open' = load temp file */
		 get_sys_error (_(" Error trying to open temp file ")));
	return 1;
    }
    f = edit_open_macro_file ("r");
    if (!f) {
/* This heads the delete macro error dialog box */
	edit_error_dialog (_(" Delete macro "),
/* 'Open' = load temp file */
		get_sys_error (_(" Error trying to open macro file ")));
	fclose (g);
	return 1;
    }
    for (;;) {
        struct macro_rec m;
        if (macro_scan_line (f, &s, &m))
	    break;
	if (s != k)
            macro_print_line (g, s, &m);
        edit_clear_macro (&m);
    };
    fclose (f);
    fclose (g);
    if (rename (catstrs (local_home_dir, TEMP_FILE, NULL), catstrs (local_home_dir, MACRO_FILE, NULL)) == -1) {
/* This heads the delete macro error dialog box */
	edit_error_dialog (_(" Delete macro "),
	   get_sys_error (_(" Error trying to overwrite macro file ")));
	return 1;
    }
    if (saved_macros_loaded)
	Cmemmove (saved_macro + j, saved_macro + j + 1, sizeof (int) * (MAX_MACROS - j - 1));
    return 0;
}

/* returns 0 on error */
int edit_save_macro_cmd (WEdit * edit, struct macro_rec *macro)
{E_
    FILE *f;
    int s, i;

    edit_push_action (edit, KEY_PRESS, edit->start_display);
/* This heads the 'Macro' dialog box */
    s = edit_raw_key_query (_(" Macro "),
/* Input line for a single key press follows the ':' */
    _(" Press the macro's new hotkey: "), 1);
    edit->force |= REDRAW_COMPLETELY;
    if (s) {
	if (edit_delete_macro (edit, s))
	    return 0;
	f = edit_open_macro_file ("a+");
	if (f) {
            macro_print_line (f, s, macro);
	    fclose (f);
	    if (saved_macros_loaded) {
#warning backport this fix:
		for (i = 0; i < MAX_MACROS - 1 && saved_macro[i]; i++);
		saved_macro[i] = s;
		saved_macro[i + 1] = 0;
	    }
	    return 1;
	} else
/* This heads the 'Save Macro' dialog box */
	    edit_error_dialog (_(" Save macro "), get_sys_error (_(" Error trying to open macro file ")));
    }
    return 0;
}

void edit_delete_macro_cmd (WEdit * edit)
{E_
    int command;

#ifdef MIDNIGHT
    command = CK_Macro (edit_raw_key_query (_ (" Delete Macro "), _ (" Press macro hotkey: "), 1));
#else
/* This heads the 'Delete Macro' dialog box */
#ifdef GTK
/* *** */
    command = 0;
#else
    command = CKeySymMod (CRawkeyQuery (0, 0, 0, _ (" Delete Macro "), "%s",
/* Input line for a single key press follows the ':' */
					_ (" Press macro hotkey: ")));
#endif
#endif

    if (!command)
	return;

    edit_delete_macro (edit, command);
}

/* return 0 on error */
int edit_load_macro_cmd (WEdit * edit, struct macro_rec *macro, int k)
{E_
    FILE *f;
    int s, i = 0, found = 0;
    struct macro_rec m;

    memset (&m, '\0', sizeof (m));
    if (macro)
	memset (macro, '\0', sizeof (*macro));

    if (saved_macros_loaded)
	if (macro_exists (k) < 0)
	    return 0;

    if ((f = edit_open_macro_file ("r"))) {
	do {
	    if (!macro || found)
		macro = &m;
            if (macro_scan_line (f, &s, macro))
		break;
	    if (!saved_macros_loaded)
		saved_macro[i++] = s;
	    edit_clear_macro (&m);
	    if (s == k)
		found = 1;
	    if (!found)
		edit_clear_macro (macro);
	} while (!found || !saved_macros_loaded);
	if (!saved_macros_loaded) {
	    saved_macro[i] = 0;
	    saved_macros_loaded = 1;
	}
	fclose (f);
	return found;
    } else
/* This heads the 'Load Macro' dialog box */
	edit_error_dialog (_(" Load macro "),
		get_sys_error (_(" Error trying to open macro file ")));

    if (macro)
	edit_clear_macro (macro);

    return 0;
}

int edit_check_macro_exists (WEdit * edit, int k)
{E_
    if (!saved_macros_loaded)
	edit_load_macro_cmd (edit, 0, k);
    return macro_exists (k) >= 0;
}

/* }}} Macro stuff starts here */

/* returns 1 on success */
int edit_save_confirm_cmd (WEdit * edit)
{E_
    char *f;

    if (edit_confirm_save) {
#ifdef MIDNIGHT
	f = catstrs (_(" Confirm save file? : "), edit->filename, " ", NULL);
#else
	f = catstrs (_(" Confirm save file? : "), edit->dir, edit->filename, " ", NULL);
#endif
/* Buttons to 'Confirm save file' query */
	if (edit_query_dialog2 (_(" Save file "), f, _("Save"), _("Cancel")))
	    return 0;
    }
    return edit_save_cmd (edit);
}

/* returns 1 on success */
int edit_save_cmd (WEdit * edit)
{E_
    if (edit_check_change_on_disk (edit, EDIT_CHANGE_ON_DISK__ON_SAVE))
        return 0;
    if (!edit->filename || !*edit->filename || !edit_save_file (edit, edit->host, catstrs (edit->dir, edit->filename, NULL)))
	return edit_save_as_cmd (edit);
    edit->force |= REDRAW_COMPLETELY;
    edit->modified = 0;
#if defined(MIDNIGHT) || defined(GTK)
    edit->delete_file = 0;
#endif		

    return 1;
}

/* returns 1 on success */
int edit_save_query_cmd (WEdit * edit)
{E_
    if (edit_check_change_on_disk (edit, EDIT_CHANGE_ON_DISK__ON_COMMAND))
        return 0;
    if (!edit_save_file (edit, edit->host, catstrs (edit->dir, edit->filename, NULL)))
	return edit_save_as_cmd (edit);
    edit->force |= REDRAW_COMPLETELY;
    edit->modified = 0;
    return 1;
}

/* returns 1 on success */
int edit_new_cmd (WEdit * edit)
{E_
    if (edit->modified) {
	if (edit_query_dialog2 (_ (" Warning "), _ (" Current text was modified without a file save. \n Continue discards these changes. "), _ ("Continue"), _ ("Cancel"))) {
	    edit->force |= REDRAW_COMPLETELY;
	    return 0;
	}
    }
    edit->force |= REDRAW_COMPLETELY;
    edit->modified = 0;
    return edit_renew (edit);	/* if this gives an error, something has really screwed up */
}

/* returns 1 on error */
int edit_load_file_from_filename (WEdit * edit, const char *host, const char *exp)
{E_
    if (!edit_reload (edit, exp, 0, host, "", 0))
	return 1;
    if (edit_split_filename (edit, host, exp))
	return 1;
    edit->modified = 0;
    return 0;
}

int edit_load_cmd (WEdit * edit)
{E_
    char *exp;
    char host[256];

    strcpy (host, edit->host);

    if (edit->modified) {
	if (edit_query_dialog2 (_ (" Warning "), _ (" Current text was modified without a file save. \n Continue discards these changes. "), _ ("Continue"), _ ("Cancel"))) {
	    edit->force |= REDRAW_COMPLETELY;
	    return 0;
	}
    }

    exp = edit_get_load_file (edit->dir, edit->filename, _ (" Load "), host);

    if (exp) {
	if (*exp)
	    edit_load_file_from_filename (edit, host, exp);
	free (exp);
    }
    edit->force |= REDRAW_COMPLETELY;
    return 0;
}

/*
   if mark2 is -1 then marking is from mark1 to the cursor.
   Otherwise its between the markers. This handles this.
   Returns 1 if no text is marked.
 */
int eval_marks (WEdit * edit, long *start_mark, long *end_mark)
{E_
    if (edit->mark1 != edit->mark2) {
	if (edit->mark2 >= 0) {
	    *start_mark = min (edit->mark1, edit->mark2);
	    *end_mark = max (edit->mark1, edit->mark2);
	} else {
	    *start_mark = min (edit->mark1, edit->curs1);
	    *end_mark = max (edit->mark1, edit->curs1);
	    edit->column2 = edit->curs_col;
	}
	return 0;
    } else {
	*start_mark = *end_mark = 0;
	edit->column2 = edit->column1 = 0;
	return 1;
    }
}

/*Block copy, move and delete commands */
extern int column_highlighting;

#ifdef MIDNIGHT
#define space_width 1
#else
extern int space_width;
#endif

void edit_insert_column_of_text (WEdit * edit, unsigned char *data, int size, int width)
{E_
    long cursor;
    int i, col;
    cursor = edit->curs1;
    col = edit_get_col (edit);
    for (i = 0; i < size; i++) {
	if (data[i] == '\n') {	/* fill in and move to next line */
	    int l;
	    long p;
	    if (edit_get_byte (edit, edit->curs1) != '\n') {
		l = width - (edit_get_col (edit) - col);
		while (l > 0) {
		    edit_insert (edit, ' ');
		    l -= space_width;
		}
	    }
	    for (p = edit->curs1;; p++) {
		if (p == edit->last_byte) {
		    edit_cursor_move (edit, edit->last_byte - edit->curs1);
		    edit_insert_ahead (edit, '\n');
		    p++;
		    break;
		}
		if (edit_get_byte (edit, p) == '\n') {
		    p++;
		    break;
		}
	    }
	    edit_cursor_move (edit, edit_move_forward3 (edit, p, col, 0) - edit->curs1);
	    l = col - edit_get_col (edit);
	    while (l >= space_width) {
		edit_insert (edit, ' ');
		l -= space_width;
	    }
	    continue;
	}
	edit_insert (edit, data[i]);
    }
    edit_cursor_move (edit, cursor - edit->curs1);
}


void edit_block_copy_cmd (WEdit * edit)
{E_
    long start_mark, end_mark, current = edit->curs1;
    int size, x;
    unsigned char *copy_buf;

    if (eval_marks (edit, &start_mark, &end_mark))
	return;
    if (column_highlighting) {
	edit_update_curs_col (edit);
	x = edit->curs_col;
	if (start_mark <= edit->curs1 && end_mark >= edit->curs1)
	    if ((x > edit->column1 && x < edit->column2) || (x > edit->column2 && x < edit->column1))
		return;
    }

    copy_buf = edit_get_block (edit, start_mark, end_mark, &size);

/* all that gets pushed are deletes hence little space is used on the stack */

    edit_push_markers (edit);

    if (column_highlighting) {
	edit_insert_column_of_text (edit, copy_buf, size, abs (edit->column2 - edit->column1));
    } else {
	while (size--)
	    edit_insert_ahead (edit, copy_buf[size]);
    }

    free (copy_buf);
    edit_scroll_screen_over_cursor (edit);

    if (column_highlighting) {
	edit_set_markers (edit, 0, 0, 0, 0);
	edit_push_action (edit, COLUMN_ON, 0);
	column_highlighting = 0;
    } else if (start_mark < current && end_mark > current)
	edit_set_markers (edit, start_mark, end_mark + end_mark - start_mark, 0, 0);

    edit->force |= REDRAW_PAGE;
}


void edit_block_move_cmd (WEdit * edit)
{E_
    long count;
    long current;
    unsigned char *copy_buf;
    long start_mark, end_mark;
    int deleted = 0;
    int x = 0;

    if (eval_marks (edit, &start_mark, &end_mark))
	return;
    if (column_highlighting) {
	edit_update_curs_col (edit);
	x = edit->curs_col;
	if (start_mark <= edit->curs1 && end_mark >= edit->curs1)
	    if ((x > edit->column1 && x < edit->column2) || (x > edit->column2 && x < edit->column1))
		return;
    } else if (start_mark <= edit->curs1 && end_mark >= edit->curs1)
	return;

    if ((end_mark - start_mark) > option_max_undo / 2)
	if (edit_query_dialog2 (_ (" Warning "), _ (" Block is large, you may not be able to undo this action. "), _ ("Continue"), _ ("Cancel")))
	    return;

    edit_push_markers (edit);
    current = edit->curs1;
    if (column_highlighting) {
	int size, c1, c2, line;
	line = edit->curs_line;
	if (edit->mark2 < 0)
	    edit_mark_cmd (edit, 0);
	c1 = min (edit->column1, edit->column2);
	c2 = max (edit->column1, edit->column2);
	copy_buf = edit_get_block (edit, start_mark, end_mark, &size);
	if (x < c2) {
	    edit_block_delete_cmd (edit);
	    deleted = 1;
	}
	edit_move_to_line (edit, line);
	edit_cursor_move (edit, edit_move_forward3 (edit, edit_bol (edit, edit->curs1), x, 0) - edit->curs1);
	edit_insert_column_of_text (edit, copy_buf, size, c2 - c1);
	if (!deleted) {
	    line = edit->curs_line;
	    edit_update_curs_col (edit);
	    x = edit->curs_col;
	    edit_block_delete_cmd (edit);
	    edit_move_to_line (edit, line);
	    edit_cursor_move (edit, edit_move_forward3 (edit, edit_bol (edit, edit->curs1), x, 0) - edit->curs1);
	}
	edit_set_markers (edit, 0, 0, 0, 0);
	edit_push_action (edit, COLUMN_ON, 0);
	column_highlighting = 0;
    } else {
#warning backport this fix
	copy_buf = malloc (end_mark - start_mark + 1);
	edit_cursor_move (edit, start_mark - edit->curs1);
	edit_scroll_screen_over_cursor (edit);
	count = start_mark;
	while (count < end_mark) {
	    copy_buf[end_mark - count - 1] = edit_delete (edit);
	    count++;
	}
	edit_scroll_screen_over_cursor (edit);
	edit_cursor_move (edit, current - edit->curs1 - (((current - edit->curs1) > 0) ? end_mark - start_mark : 0));
	edit_scroll_screen_over_cursor (edit);
	while (count-- > start_mark)
	    edit_insert_ahead (edit, copy_buf[end_mark - count - 1]);
	edit_set_markers (edit, edit->curs1, edit->curs1 + end_mark - start_mark, 0, 0);
    }
    edit_scroll_screen_over_cursor (edit);
    free (copy_buf);
    edit->force |= REDRAW_PAGE;
}

void edit_cursor_to_bol (WEdit * edit);

void edit_delete_column_of_text (WEdit * edit)
{E_
    long p, q, r, m1, m2;
    int b, c, d;
    int n;

    eval_marks (edit, &m1, &m2);
    n = edit_move_forward (edit, m1, 0, m2) + 1;
    c = edit_move_forward3 (edit, edit_bol (edit, m1), 0, m1);
    d = edit_move_forward3 (edit, edit_bol (edit, m2), 0, m2);

    b = min (c, d);
    c = max (c, d);

    while (n--) {
	r = edit_bol (edit, edit->curs1);
	p = edit_move_forward3 (edit, r, b, 0);
	q = edit_move_forward3 (edit, r, c, 0);
	if (p < m1)
	    p = m1;
	if (q > m2)
	    q = m2;
	edit_cursor_move (edit, p - edit->curs1);
	while (q > p) {		/* delete line between margins */
	    if (edit_get_byte (edit, edit->curs1) != '\n')
		edit_delete (edit);
	    q--;
	}
	if (n)			/* move to next line except on the last delete */
	    edit_cursor_move (edit, edit_move_forward (edit, edit->curs1, 1, 0) - edit->curs1);
    }
}

int edit_block_delete (WEdit * edit)
{E_
    long count;
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    if (column_highlighting && edit->mark2 < 0)
	edit_mark_cmd (edit, 0);
    if ((end_mark - start_mark) > option_max_undo / 2)
/* Warning message with a query to continue or cancel the operation */
	if (edit_query_dialog2 (_ (" Warning "), _ (" Block is large, you may not be able to undo this action. "), _ (" Continue "), _ (" Cancel ")))
	    return 1;
    edit_push_markers (edit);
    edit_cursor_move (edit, start_mark - edit->curs1);
    edit_scroll_screen_over_cursor (edit);
    count = start_mark;
    if (start_mark < end_mark) {
	if (column_highlighting) {
	    if (edit->mark2 < 0)
		edit_mark_cmd (edit, 0);
	    edit_delete_column_of_text (edit);
	} else {
	    while (count < end_mark) {
		edit_delete (edit);
		count++;
	    }
	}
    }
    edit_set_markers (edit, 0, 0, 0, 0);
    edit->force |= REDRAW_PAGE;
    return 0;
}

/* returns 1 if canceelled by user */
int edit_block_delete_cmd (WEdit * edit)
{E_
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark)) {
	edit_delete_line (edit);
	return 0;
    }
    return edit_block_delete (edit);
}


#ifdef MIDNIGHT

#define INPUT_INDEX 9
#define SEARCH_DLG_WIDTH 58
#define SEARCH_DLG_HEIGHT 10
#define REPLACE_DLG_WIDTH 58
#define REPLACE_DLG_HEIGHT 15
#define CONFIRM_DLG_WIDTH 79
#define CONFIRM_DLG_HEIGTH 6
#define B_REPLACE_ALL B_USER+1
#define B_REPLACE_ONE B_USER+2
#define B_SKIP_REPLACE B_USER+3

int edit_replace_prompt (WEdit * edit, char *replace_text, int xpos, int ypos)
{E_
    QuickWidget quick_widgets[] =
    {
/* NLS  for hotkeys? */
	{quick_button, 63, CONFIRM_DLG_WIDTH, 3, CONFIRM_DLG_HEIGTH, N_ ("&Cancel"),
	 0, B_CANCEL, 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 50, CONFIRM_DLG_WIDTH, 3, CONFIRM_DLG_HEIGTH, N_ ("o&Ne"),
	 0, B_REPLACE_ONE, 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 37, CONFIRM_DLG_WIDTH, 3, CONFIRM_DLG_HEIGTH, N_ ("al&L"),
	 0, B_REPLACE_ALL, 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 21, CONFIRM_DLG_WIDTH, 3, CONFIRM_DLG_HEIGTH, N_ ("&Skip"),
	 0, B_SKIP_REPLACE, 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 4, CONFIRM_DLG_WIDTH, 3, CONFIRM_DLG_HEIGTH, N_ ("&Replace"),
	 0, B_ENTER, 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_label, 2, CONFIRM_DLG_WIDTH, 2, CONFIRM_DLG_HEIGTH, 0,
	 0, 0, 0, XV_WLAY_DONTCARE, 0},
	{0}};

    quick_widgets[4].text = catstrs (_ (" Replace with: "), replace_text, NULL);

    {
	QuickDialog Quick_input =
	{CONFIRM_DLG_WIDTH, CONFIRM_DLG_HEIGTH, 0, 0, N_ (" Confirm replace "),
	 "[Input Line Keys]", "quick_input", 0 /*quick_widgets */ };

	Quick_input.widgets = quick_widgets;

	Quick_input.xpos = xpos;
	Quick_input.ypos = ypos;
	return quick_dialog (&Quick_input);
    }
}

int edit_replace_dialog (WEdit * edit, CStr *search_text, CStr *replace_text, char **arg_order)
{E_
    int treplace_scanf = option_replace_scanf;
    int treplace_regexp = option_replace_regexp;
    int treplace_all = option_replace_all;
    int treplace_prompt = option_replace_prompt;
    int treplace_backwards = option_replace_backwards;
    int treplace_whole = option_replace_whole;
    int treplace_case = option_replace_case;

    char *tsearch_text;
    char *treplace_text;
    char *targ_order;
    QuickWidget quick_widgets[] =
    {
	{quick_button, 6, 10, 12, REPLACE_DLG_HEIGHT, N_("&Cancel"), 0, B_CANCEL, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 2, 10, 12, REPLACE_DLG_HEIGHT, N_("&Ok"), 0, B_ENTER, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 33, REPLACE_DLG_WIDTH, 11, REPLACE_DLG_HEIGHT, N_("scanf &Expression"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 33, REPLACE_DLG_WIDTH, 10, REPLACE_DLG_HEIGHT, N_("replace &All"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 33, REPLACE_DLG_WIDTH, 9, REPLACE_DLG_HEIGHT, N_("pr&Ompt on replace"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, REPLACE_DLG_WIDTH, 11, REPLACE_DLG_HEIGHT, N_("&Backwards"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, REPLACE_DLG_WIDTH, 10, REPLACE_DLG_HEIGHT, N_("&Regular expression"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, REPLACE_DLG_WIDTH, 9, REPLACE_DLG_HEIGHT, N_("&Whole words only"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, REPLACE_DLG_WIDTH, 8, REPLACE_DLG_HEIGHT, N_("case &Sensitive"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_input,    3, REPLACE_DLG_WIDTH, 7, REPLACE_DLG_HEIGHT, "", 52, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-argord"},
	{quick_label, 2, REPLACE_DLG_WIDTH, 6, REPLACE_DLG_HEIGHT, N_(" Enter replacement argument order eg. 3,2,1,4 "), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, REPLACE_DLG_WIDTH, 5, REPLACE_DLG_HEIGHT, "", 52, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-replace"},
	{quick_label, 2, REPLACE_DLG_WIDTH, 4, REPLACE_DLG_HEIGHT, N_(" Enter replacement string:"), 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, REPLACE_DLG_WIDTH, 3, REPLACE_DLG_HEIGHT, "", 52, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-search"},
	{quick_label, 2, REPLACE_DLG_WIDTH, 2, REPLACE_DLG_HEIGHT, N_(" Enter search string:"), 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{0}};

    quick_widgets[2].result = &treplace_scanf;
    quick_widgets[3].result = &treplace_all;
    quick_widgets[4].result = &treplace_prompt;
    quick_widgets[5].result = &treplace_backwards;
    quick_widgets[6].result = &treplace_regexp;
    quick_widgets[7].result = &treplace_whole;
    quick_widgets[8].result = &treplace_case;
    quick_widgets[9].str_result = &targ_order;
    quick_widgets[9].text = *arg_order;
    quick_widgets[11].str_result = &treplace_text;
    quick_widgets[11].text = *replace_text;
    quick_widgets[13].str_result = &tsearch_text;
    quick_widgets[13].text = *search_text;
    {
	QuickDialog Quick_input =
	{REPLACE_DLG_WIDTH, REPLACE_DLG_HEIGHT, -1, 0, N_(" Replace "),
	 "[Input Line Keys]", "quick_input", 0 /*quick_widgets */ };

	Quick_input.widgets = quick_widgets;

	if (quick_dialog (&Quick_input) != B_CANCEL) {
	    *arg_order = *(quick_widgets[INPUT_INDEX].str_result);
	    *replace_text = *(quick_widgets[INPUT_INDEX + 2].str_result);
	    *search_text = *(quick_widgets[INPUT_INDEX + 4].str_result);
	    option_replace_scanf = treplace_scanf;
	    option_replace_backwards = treplace_backwards;
	    option_replace_regexp = treplace_regexp;
	    option_replace_all = treplace_all;
	    option_replace_prompt = treplace_prompt;
	    option_replace_whole = treplace_whole;
	    option_replace_case = treplace_case;
	    return;
	} else {
	    *arg_order = NULL;
	    *replace_text = NULL;
	    *search_text = NULL;
	    return;
	}
    }
}


void edit_search_dialog (WEdit * edit, char **search_text)
{E_
    int treplace_scanf = option_replace_scanf;
    int treplace_regexp = option_replace_regexp;
    int treplace_whole = option_replace_whole;
    int treplace_case = option_replace_case;
    int treplace_backwards = option_replace_backwards;

    char *tsearch_text;
    QuickWidget quick_widgets[] =
    {
	{quick_button, 6, 10, 7, SEARCH_DLG_HEIGHT, N_("&Cancel"), 0, B_CANCEL, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 2, 10, 7, SEARCH_DLG_HEIGHT, N_("&Ok"), 0, B_ENTER, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 33, SEARCH_DLG_WIDTH, 6, SEARCH_DLG_HEIGHT, N_("scanf &Expression"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL },
	{quick_checkbox, 33, SEARCH_DLG_WIDTH, 5, SEARCH_DLG_HEIGHT, N_("&Backwards"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, SEARCH_DLG_WIDTH, 6, SEARCH_DLG_HEIGHT, N_("&Regular expression"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, SEARCH_DLG_WIDTH, 5, SEARCH_DLG_HEIGHT, N_("&Whole words only"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, SEARCH_DLG_WIDTH, 4, SEARCH_DLG_HEIGHT, N_("case &Sensitive"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_input, 3, SEARCH_DLG_WIDTH, 3, SEARCH_DLG_HEIGHT, "", 52, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-search"},
	{quick_label, 2, SEARCH_DLG_WIDTH, 2, SEARCH_DLG_HEIGHT, N_(" Enter search string:"), 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{0}};

    quick_widgets[2].result = &treplace_scanf;
    quick_widgets[3].result = &treplace_backwards;
    quick_widgets[4].result = &treplace_regexp;
    quick_widgets[5].result = &treplace_whole;
    quick_widgets[6].result = &treplace_case;
    quick_widgets[7].str_result = &tsearch_text;
    quick_widgets[7].text = *search_text;

    {
	QuickDialog Quick_input =
	{SEARCH_DLG_WIDTH, SEARCH_DLG_HEIGHT, -1, 0, N_(" Search "),
	 "[Input Line Keys]", "quick_input", 0 /*quick_widgets */ };

	Quick_input.widgets = quick_widgets;

	if (quick_dialog (&Quick_input) != B_CANCEL) {
	    *search_text = *(quick_widgets[7].str_result);
	    option_replace_scanf = treplace_scanf;
	    option_replace_backwards = treplace_backwards;
	    option_replace_regexp = treplace_regexp;
	    option_replace_whole = treplace_whole;
	    option_replace_case = treplace_case;
	    return;
	} else {
	    *search_text = NULL;
	    return;
	}
    }
}

#else

#define B_ENTER 0
#define B_SKIP_REPLACE 1
#define B_REPLACE_ALL 2
#define B_REPLACE_ONE 3
#define B_CANCEL 4

extern CWidget *wedit;

#ifndef GTK

void edit_backspace_tab (WEdit * edit, int whole_tabs_only);

static inline int my_is_blank (int c)
{E_
    return c == ' ' || c == '\t';
}

void edit_indent_left_right_paragraph (WEdit * edit)
{E_
    char id[33];
    int width = 0, lines, c;
    CState s;
    CWidget *w, *p = 0, *i;
    long start_mark, end_mark;
    strcpy (id, CIdentOf (edit->widget));
    strcat (id, ".text");
    w = CIdent (id);
    if (!w)
	return;
    if (eval_marks (edit, &start_mark, &end_mark)) {
	edit_error_dialog (_(" Error "),
			   _
			   (" No text highlighted - highlight text, run command again, then use arrow keys. "));
	return;
    }
    CBackupState (&s);
    CDisable ("*");
    p =
	CDrawText ("status_prompt", edit->widget->parentid, CXof (w), CYof (w), "%s",
		   _(" <---  ---> (this eats your undo stack) "));
    width = CWidthOf (p);
    i = CDrawTextInputP ("status_input", edit->widget->parentid, CXof (w) + width, CYof (w),
			CWidthOf (edit->widget) - width, AUTO_HEIGHT, 1, "");
    CFocus (i);
    edit_set_markers (edit, edit_bol (edit, start_mark), edit_eol (edit, end_mark), -1, -1);
    edit->force |= REDRAW_PAGE;
    edit_render_keypress (edit);
    edit_push_action (edit, KEY_PRESS, edit->start_display);

    for (;;) {
	XEvent xev;
	CEvent cev;
	CNextEvent (&xev, &cev);
	if (xev.type == KeyPress) {
	    if (eval_marks (edit, &start_mark, &end_mark))
		break;
	    lines = edit_count_lines (edit, start_mark, end_mark);
	    if (cev.command == CK_Right || cev.command == CK_Tab) {
		long s;
		for (c = 0, s = start_mark; c <= lines; s = edit_eol (edit, s) + 1, c++) {
		    while (my_is_blank (edit_get_byte (edit, s)) && s < edit->last_byte)
			s++;
		    edit_cursor_move (edit, s - edit->curs1);
		    edit_tab_cmd (edit);
		    s = edit->curs1;
		}
		edit->force |= REDRAW_PAGE;
		edit_render_keypress (edit);
		edit_push_action (edit, KEY_PRESS, edit->start_display);
	    } else if (cev.command == CK_Left || cev.command == CK_BackSpace) {
		long s;
		for (c = 0, s = start_mark; c <= lines; s = edit_eol (edit, s) + 1, c++) {
		    while (my_is_blank (edit_get_byte (edit, s)) && s < edit->last_byte)
			s++;
		    edit_cursor_move (edit, s - edit->curs1);
		    edit_backspace_tab (edit, 1);
		    s = edit->curs1;
		}
		edit->force |= REDRAW_PAGE;
		edit_render_keypress (edit);
		edit_push_action (edit, KEY_PRESS, edit->start_display);
	    } else {
		break;
	    }
	}
    }
    CDestroyWidget ("status_prompt");
    CDestroyWidget ("status_input");
    CRestoreState (&s);
    return;
}

extern struct look *look;

int edit_search_replace_dialog (Window parent, int x, int y, CStr *search_text,
				 CStr *replace_text, CStr *arg_order, const char *heading, int option)
{E_
    return (*look->search_replace_dialog) (parent, x, y, search_text, replace_text, arg_order, heading, option);
}

int edit_search_dialog (WEdit * edit, CStr *search_text)
{E_
/* Heads the 'Search' dialog box */
    return edit_search_replace_dialog (WIN_MESSAGES, search_text, 0, 0, _(" Search "), SEARCH_DIALOG_OPTION_BACKWARDS | SEARCH_DIALOG_OPTION_BOOKMARK);
}

int edit_replace_dialog (WEdit * edit, CStr *search_text, CStr *replace_text, CStr *arg_order)
{E_
/* Heads the 'Replace' dialog box */
    return edit_search_replace_dialog (WIN_MESSAGES, search_text, replace_text, arg_order, _(" Replace "), SEARCH_DIALOG_OPTION_BACKWARDS);
}

#else

#include <libgnomeui/gtkcauldron.h>
#include <libgnomeui/gnome-stock.h>

void edit_search_dialog (WEdit * edit, char **search_text)
{E_
    char *s;
    s = gtk_dialog_cauldron (
				"Search", GTK_CAULDRON_TOPLEVEL | GTK_CAULDRON_GRAB,
				" ( (Enter search text)d | %Eogxf )xf / ( ( %Cd // %Cd // %Cd ) || ( %Cd // %Cd )xf )xf / ( %Bxfgrq || %Bxfgq )f",
				search_text, "search",
				"&Whole word", &option_replace_whole,
				"Case &sensitive", &option_replace_case,
				"&Regular expression", &option_replace_regexp,
				"&Backwards", &option_replace_backwards,
				"Scanf &expression", &option_replace_scanf,
				GNOME_STOCK_BUTTON_OK,
				GNOME_STOCK_BUTTON_CANCEL
	);
    if (s == GTK_CAULDRON_ESCAPE || !s || s == GNOME_STOCK_BUTTON_CANCEL)
	*search_text = 0;
    return;
}

void edit_replace_dialog (WEdit * edit, char **search_text, char **replace_text, char **arg_order)
{E_
    char *s;
    s = gtk_dialog_cauldron (
				"Search", GTK_CAULDRON_TOPLEVEL | GTK_CAULDRON_GRAB,
				" ( (Enter search text)d | %Eogxf )xf / ( (Enter replace text)d | %Egxf )xf / ( (Enter argument order)d | %Egxf )xf / ( ( %Cd // %Cd // %Cd // %Cd ) || ( %Cd // %Cd // %Cd )xf )xf / ( %Bxfgrq || %Bxfgq )f",
				search_text, "search",
				replace_text, "replace",
				arg_order, "arg_order",
				"&Whole word", &option_replace_whole,
				"Case &sensitive", &option_replace_case,
				"&Regular expression", &option_replace_regexp,
				"&Backwards", &option_replace_backwards,
				"Pr&ompt on replace", &option_replace_prompt,
				"Replace &all", &option_replace_all,
				"Scanf &expression", &option_replace_scanf,
				GNOME_STOCK_BUTTON_OK,
				GNOME_STOCK_BUTTON_CANCEL
	);
    if (s == GTK_CAULDRON_ESCAPE || !s || s == GNOME_STOCK_BUTTON_CANCEL)
	*search_text = 0;
    return;
}

#endif

#ifdef GTK

int edit_replace_prompt (WEdit * edit, char *replace_text, int xpos, int ypos)
{E_
    char *s;
    s = gtk_dialog_cauldron (
		    "Replace", GTK_CAULDRON_TOPLEVEL | GTK_CAULDRON_GRAB,
				" ( (Replace with:)d %Ld )xf / ( %Bxfrq || %Bxfq || %Bxfq || %Bxfq || %Bxfgq )f",
				replace_text,
			 "Replace", "Skip", "Replace all", "Replace one",
				GNOME_STOCK_BUTTON_CANCEL
	);
    if (s == GTK_CAULDRON_ESCAPE || !s || s == GNOME_STOCK_BUTTON_CANCEL)
	return B_CANCEL;
    if (!strcmp (s, "Replace all"))
	return B_REPLACE_ALL;
    if (!strcmp (s, "Replace one"))
	return B_REPLACE_ONE;
    if (!strcmp (s, "Skip"))
	return B_SKIP_REPLACE;
    if (!strcmp (s, "Replace"))
	return B_ENTER;
}

#else

static char *text_to_nroff_out (CStr s);

static int edit_replace_prompt (WEdit * edit, CStr replace_text, int xpos, int ypos)
{E_
    int q, x[] =
    {
	B_CANCEL, B_ENTER, B_SKIP_REPLACE, B_REPLACE_ALL, B_REPLACE_ONE, B_CANCEL
    };
    q = CQueryDialog (WIN_MESSAGES + (edit->curs_line < 8 ? edit->num_widget_lines / 2 * FONT_PIX_PER_LINE + CYof (edit->widget) : 0),
		      _ (" Replace "), _ (" Replace with: "), CQUERYDIALOG_ADD_TEXTBOX, text_to_nroff_out (replace_text),
                      _ ("Replace"), _ ("Skip"), _ ("Replace all"), _ ("Replace one"), _ ("Cancel"), NULL);
    edit->force |= REDRAW_COMPLETELY;
    return x[q + 1];
}

#endif

#endif

struct sargs_s {
    int u[NUM_REPL_ARGS][256 / sizeof (int)];
};

struct sargs_s sargs_;

#define sargs sargs_.u

#define SCANF_ARGS sargs[0], sargs[1], sargs[2], sargs[3], \
		     sargs[4], sargs[5], sargs[6], sargs[7], \
		     sargs[8], sargs[9], sargs[10], sargs[11], \
		     sargs[12], sargs[13], sargs[14], sargs[15]

#define PRINTF_ARGS sargs[argord[0]], sargs[argord[1]], sargs[argord[2]], sargs[argord[3]], \
		     sargs[argord[4]], sargs[argord[5]], sargs[argord[6]], sargs[argord[7]], \
		     sargs[argord[8]], sargs[argord[9]], sargs[argord[10]], sargs[argord[11]], \
		     sargs[argord[12]], sargs[argord[13]], sargs[argord[14]], sargs[argord[15]]


/* This function is a modification of mc-3.2.10/src/view.c:regexp_view_search() */
/* returns -3 on error in pattern, -1 on not found, found_len = 0 if either */
int string_regexp_search (char *pattern, char *string, int len, int match_type, int match_bol, int icase, int *found_len, void *d)
{E_
    static regex_t r;
    static char *old_pattern = NULL;
    static int old_type, old_icase;
    regmatch_t *pmatch;
    static regmatch_t s[1];

    pmatch = (regmatch_t *) d;
    if (!pmatch)
	pmatch = s;

    if (!old_pattern || strcmp (old_pattern, pattern) || old_type != match_type || old_icase != icase) {
	if (old_pattern) {
	    regfree (&r);
	    free (old_pattern);
	    old_pattern = 0;
	}
	memset (&r, '\0', sizeof (r));
	if (regcomp (&r, pattern, REG_EXTENDED | (icase ? REG_ICASE : 0))) {
	    *found_len = 0;
	    return -3;
	}
	old_pattern = (char *) strdup (pattern);
	old_type = match_type;
	old_icase = icase;
    }
    if (regexec (&r, string, d ? NUM_REPL_ARGS : 1, pmatch, ((match_bol || match_type != match_normal) ? 0 : REG_NOTBOL)) != 0) {
	*found_len = 0;
	return -1;
    }
    *found_len = pmatch[0].rm_eo - pmatch[0].rm_so;
    return (pmatch[0].rm_so);
}

/* thanks to  Liviu Daia <daia@stoilow.imar.ro>  for getting this
   (and the above) routines to work properly - paul */
static long edit_find_string__ (unsigned char **work_space1, unsigned char **work_space2, long start, CStr exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data, int once_only, void *d)
{E_
    long p, q = 0;
    long l = exp.len, f = 0;
    int n = 0;

    memset(&sargs_, '\0', sizeof(sargs_));

    for (p = 0; p < l; p++)	/* count conversions... */
	if (exp.data[p] == '%')
	    if (exp.data[++p] != '%')	/* ...except for "%%" */
		n++;

    if (option_replace_scanf || option_replace_regexp) {
	int c;

	option_replace_scanf = (!option_replace_regexp);	/* can't have both */

	if (option_replace_scanf) {
	    unsigned char *e;
            unsigned char *expc;
	    unsigned char *buf;
	    unsigned char *mbuf;

	    buf = mbuf = *work_space1 = (unsigned char *) realloc (*work_space1, SCANF_WORKSPACE * 2 + 3);
            *mbuf = 0;

	    if (n >= NUM_REPL_ARGS)
		return -3;

            e = *work_space2 = realloc (*work_space2, SCANF_WORKSPACE + 1);

	    if (option_replace_case) {
		for (p = start; p < last_byte && p < start + SCANF_WORKSPACE; p++)
		    buf[p - start] = (*get_byte) (data, p);
	    } else {
		for (p = 0; p < exp.len; p++)
		    exp.data[p] = my_lower_case (exp.data[p]);
		for (p = start; p < last_byte && p < start + SCANF_WORKSPACE; p++) {
		    c = (*get_byte) (data, p);
		    buf[p - start] = my_lower_case (c);
		}
	    }

	    buf[(q = p - start)] = 0;
	    strcpy ((char *) e, (char *) exp.data);
	    strcat ((char *) e, "%n");
	    expc = e;

	    while (q) {
		*((int *) sargs[n]) = 0;	/* --> here was the problem - now fixed: good */
		if (n == sscanf ((char *) buf, (char *) expc, SCANF_ARGS)) {
		    if (*((int *) sargs[n])) {
			*len = *((int *) sargs[n]);
			return start;
		    }
		}
		if (once_only)
		    return -2;
		if (q + start < last_byte) {
		    if (option_replace_case) {
			buf[q] = (*get_byte) (data, q + start);
		    } else {
			c = (*get_byte) (data, q + start);
			buf[q] = my_lower_case (c);
		    }
		    q++;
		}
		buf[q] = 0;
		start++;
		buf++;		/* move the window along */
		if (buf >= mbuf + SCANF_WORKSPACE) {	/* the window is about to go past the end of array, so... */
		    Cmemmove (mbuf, buf, strlen ((char *) buf) + 1);	/* reset it */
		    buf = mbuf;
		}
		q--;
	    }
	} else {	/* regexp matching */
	    long offset = 0;
	    int found_start, match_bol; 
	    unsigned char *mbuf;
            long alloced = 8;

	    mbuf = *work_space1 = (unsigned char *) realloc (*work_space1, alloced + 8);
            *mbuf = 0;

	    while (start + offset < last_byte) {
	        unsigned char *buf;
		match_bol = (offset == 0 || (*get_byte) (data, start + offset - 1) == '\n');
	        p = start + offset;
		q = 0;
		for (; p < last_byte; p++, q++) {
                    if (q >= alloced) {
                        alloced *= 2;
                        mbuf = *work_space1 = (unsigned char *) realloc (*work_space1, alloced + 8);
                    }
		    mbuf[q] = (*get_byte) (data, p);
		    if (mbuf[q] == '\n') {
			q++;
			break;
                    }
		}
		offset += q;
		mbuf[q] = 0;

		buf = mbuf;
		while (q) {
		    found_start = string_regexp_search ((char *) exp.data, (char *) buf, q, match_normal, match_bol, !option_replace_case, len, d);

		    if (found_start <= -2) {	/* regcomp/regexec error */
			*len = 0;
			return -3;
		    }
		    else if (found_start == -1)	/* not found: try next line */
			break;
		    else if (*len == 0) { /* null pattern: try again at next character */
			q--;
			buf++;
			match_bol = 0;
			continue;
		    }
		    else	/* found */
			return (start + offset - q + found_start);
		}
		if (once_only)
		    return -2;
	    }
	}
    } else {
 	*len = exp.len;
	if (option_replace_case) {
	    for (p = start; p <= last_byte - l; p++) {
 		if ((*get_byte) (data, p) == (unsigned char)exp.data[0]) {	/* check if first char matches */
		    for (f = 0, q = 0; q < l && f < 1; q++)
 			if ((*get_byte) (data, q + p) != (unsigned char)exp.data[q])
			    f = 1;
		    if (f == 0)
			return p;
		}
		if (once_only)
		    return -2;
	    }
	} else {
	    for (p = 0; p < exp.len; p++)
		exp.data[p] = my_lower_case (exp.data[p]);

	    for (p = start; p <= last_byte - l; p++) {
		if (my_lower_case ((*get_byte) (data, p)) == (unsigned char)exp.data[0]) {
		    for (f = 0, q = 0; q < l && f < 1; q++)
			if (my_lower_case ((*get_byte) (data, q + p)) != (unsigned char)exp.data[q])
			    f = 1;
		    if (f == 0)
			return p;
		}
		if (once_only)
		    return -2;
	    }
	}
    }
    return -2;
}

static long edit_find_string_ (long start, CStr exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data, int once_only, void *d)
{E_
    long r;
    unsigned char *work_space1 = NULL, *work_space2 = NULL;
    r = edit_find_string__ (&work_space1, &work_space2, start, exp, len, last_byte, get_byte, data, once_only, d);
    free (work_space2);
    free (work_space1);
    return r;
}

long edit_find_string (long start, CStr exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data, int once_only, void *d)
{E_
    int r;
    CStr s;
    s = CStr_dupstr(exp);
    r = edit_find_string_ (start, s, len, last_byte, get_byte, data, once_only, d);
    CStr_free(&s);
    return r;
}

long edit_find_forwards (long search_start, CStr exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data, int once_only, void *d)
{				/*front end to find_string to check for
				   whole words */
    long p;
    p = search_start;

    while ((p = edit_find_string (p, exp, len, last_byte, get_byte, data, once_only, d)) >= 0) {
	if (option_replace_whole) {
/*If the bordering chars are not in option_whole_chars_search then word is whole */
	    if (!strcasechr (option_whole_chars_search, (*get_byte) (data, p - 1))
		&& !strcasechr (option_whole_chars_search, (*get_byte) (data, p + *len)))
		return p;
	    if (once_only)
		return -2;
	} else
	    return p;
	if (once_only)
	    break;
	p++;			/*not a whole word so continue search. */
    }
    return p;
}

long edit_find (long search_start, CStr exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data, void *d)
{E_
    long p;
    if (option_replace_backwards) {
	while (search_start >= 0) {
	    p = edit_find_forwards (search_start, exp, len, last_byte, get_byte, data, 1, d);
	    if (p == search_start)
		return p;
	    search_start--;
	}
    } else {
	return edit_find_forwards (search_start, exp, len, last_byte, get_byte, data, 0, d);
    }
    return -2;
}

#define is_digit(x) ((x) >= '0' && (x) <= '9')

#define snprintf(v) { \
		*p1++ = *p++; \
		*p1 = '\0'; \
		sprintf(s,q1,v); \
		s += strlen(s); \
	    }

/* this function uses the sprintf command to do a vprintf */
/* it takes pointers to arguments instead of the arguments themselves */
int sprintf_p (char *str, const char *fmt,...)
{E_
    va_list ap;
    int n;
    char *q, *p, *s = str;
    char q1[32];
    char *p1;

    va_start (ap, fmt);
    p = q = (char *) fmt;

    while ((p = strchr (p, '%'))) {
	n = (int) ((unsigned long) p - (unsigned long) q);
	strncpy (s, q, n);	/* copy stuff between format specifiers */
	s += n;
	*s = 0;
	q = p;
        (void) q;
	p1 = q1;
	*p1++ = *p++;
	if (*p == '%') {
	    p++;
	    *s++ = '%';
	    q = p;
	    continue;
	}
	if (*p == 'n') {
	    p++;
/* do nothing */
	    q = p;
	    continue;
	}
	if (*p == '#')
	    *p1++ = *p++;
	if (*p == '0')
	    *p1++ = *p++;
	if (*p == '-')
	    *p1++ = *p++;
	if (*p == '+')
	    *p1++ = *p++;
	if (*p == '*') {
	    p++;
	    strcpy (p1, itoa (*va_arg (ap, int *)));	/* replace field width with a number */
	    p1 += strlen (p1);
	} else {
	    while (is_digit (*p))
		*p1++ = *p++;
	}
	if (*p == '.')
	    *p1++ = *p++;
	if (*p == '*') {
	    p++;
	    strcpy (p1, itoa (*va_arg (ap, int *)));	/* replace precision with a number */
	    p1 += strlen (p1);
	} else {
	    while (is_digit (*p))
		*p1++ = *p++;
	}
/* flags done, now get argument */
	if (*p == 's') {
            snprintf (va_arg (ap, char *));
	} else if (*p == 'h') {
	    if (strchr ("diouxX", *p))
		snprintf (*va_arg (ap, short *));
	} else if (*p == 'l') {
	    *p1++ = *p++;
	    if (strchr ("diouxX", *p))
		snprintf (*va_arg (ap, long *));
	} else if (strchr ("cdiouxX", *p)) {
	    snprintf (*va_arg (ap, int *));
	} else if (*p == 'L') {
	    *p1++ = *p++;
	    if (strchr ("EefgG", *p))
		snprintf (*va_arg (ap, double *));	/* should be long double */
	} else if (strchr ("EefgG", *p)) {
	    snprintf (*va_arg (ap, double *));
	} else if (strchr ("DOU", *p)) {
	    snprintf (*va_arg (ap, long *));
	} else if (*p == 'p') {
	    snprintf (*va_arg (ap, void **));
	}
	q = p;
    }
    va_end (ap);
    strcpy (s, q);		/* print trailing leftover */
    s += strlen (s);
    return (s - str);
}

static void regexp_error (WEdit *edit)
{E_
/* "Error: Syntax error in regular expression, or scanf expression contained too many %'s */
    edit_error_dialog (_(" Error "), _(" Invalid regular expression, or scanf expression with to many conversions "));
}

/* call with edit = 0 before shutdown to close memory leaks */
void edit_replace_cmd (WEdit * edit, int again)
{E_
    int cancel = 0;
    static regmatch_t pmatch[NUM_REPL_ARGS];
    static CStr old1 = {NULL, 0};
    static CStr old2 = {NULL, 0};
    static CStr old3 = {NULL, 0};
    CStr exp1 = {"", 0};
    CStr exp2 = {"", 0};
    CStr exp3 = {"", 0};
    int replace_yes;
    int replace_continue;
    int treplace_prompt = 0;
    int i = 0;
    long times_replaced = 0, last_search;
    char fin_string[64];
    int argord[NUM_REPL_ARGS];

    if (!edit) {
        CStr_free(&old1);
        CStr_free(&old2);
        CStr_free(&old3);
	return;
    }

    if (!old1.data) {  /* initialize */
        old1 = CStr_dup("");
        old2 = CStr_dup("");
        old3 = CStr_dup("");
    }

    last_search = edit->last_byte;

    edit->force |= REDRAW_COMPLETELY;

    if (again)
	if (!old1.len)
	    return;

    exp1 = CStr_dupstr(old1);
    exp2 = CStr_dupstr(old2);
    exp3 = CStr_dupstr(old3);

    edit_push_action (edit, KEY_PRESS, edit->start_display);

    if (!again) {
	cancel = edit_replace_dialog (edit, &exp1, &exp2, &exp3);
	treplace_prompt = option_replace_prompt;
    }

    if (cancel || !exp1.len)
        goto out;

    CStr_free(&old1);
    CStr_free(&old2);
    CStr_free(&old3);
    old1 = CStr_dupstr(exp1);
    old2 = CStr_dupstr(exp2);
    old3 = CStr_dupstr(exp3);

/* #warning cleanup this block: */
    {
	char *s;
	int ord;
	while ((s = strchr (exp3.data, ' ')))
	    Cmemmove (s, s + 1, strlen (s));
	s = exp3.data;
	for (i = 0; i < NUM_REPL_ARGS; i++) {
	    if ((unsigned long) s != 1 && s < exp3.data + strlen (exp3.data)) {
		if ((ord = atoi (s)))
		    argord[i] = ord - 1;
		else
		    argord[i] = i;
		s = strchr (s, ',') + 1;
	    } else
		argord[i] = i;
	}
    }

    replace_continue = option_replace_all;

    if (edit->found_len && edit->search_start == edit->found_start + 1 && option_replace_backwards)
	edit->search_start--;

    if (edit->found_len && edit->search_start == edit->found_start - 1 && !option_replace_backwards)
	edit->search_start++;

    do {
	int len = 0;
	long new_start;
	new_start = edit_find (edit->search_start, exp1, &len, last_search,
	   (int (*)(void *, long)) edit_get_byte, (void *) edit, pmatch);
	if (new_start == -3) {
	    regexp_error (edit);
	    break;
	}
	edit->search_start = new_start;
	/*returns negative on not found or error in pattern */

	if (edit->search_start >= 0) {
	    edit->found_start = edit->search_start;
	    i = edit->found_len = len;

	    edit_cursor_move (edit, edit->search_start - edit->curs1);
	    edit_scroll_screen_over_cursor (edit);

	    replace_yes = 1;

	    if (treplace_prompt) {
		int l;
		l = edit->curs_row - edit->num_widget_lines / 3;
		if (l > 0)
		    edit_scroll_downward (edit, l);
		if (l < 0)
		    edit_scroll_upward (edit, -l);

		edit_scroll_screen_over_cursor (edit);
		edit->force |= REDRAW_PAGE;
		edit_render_keypress (edit);

		/*so that undo stops at each query */
		edit_push_key_press (edit);

		switch (edit_replace_prompt (edit, exp2,	/*and prompt 2/3 down */
					     edit->num_widget_columns / 2 - 33, edit->num_widget_lines * 2 / 3)) {
		case B_ENTER:
		    break;
		case B_SKIP_REPLACE:
		    replace_yes = 0;
		    break;
		case B_REPLACE_ALL:
		    treplace_prompt = 0;
		    replace_continue = 1;
		    break;
		case B_REPLACE_ONE:
		    replace_continue = 0;
		    break;
		case B_CANCEL:
		    replace_yes = 0;
		    replace_continue = 0;
		    break;
		}
	    }

	    if (replace_yes) {	/* delete then insert new */
		if (option_replace_scanf || option_replace_regexp) {
		    char *repl_str;
                    repl_str = (char *) malloc (MAX_REPL_LEN + 8);
		    if (option_replace_regexp) {	/* we need to fill in sargs just like with scanf */
			int k, j;
			for (k = 1; k < NUM_REPL_ARGS && pmatch[k].rm_eo >= 0; k++) {
			    unsigned char *t;
			    t = (unsigned char *) &sargs[k - 1][0];
			    for (j = 0; j < pmatch[k].rm_eo - pmatch[k].rm_so && j < 255; j++, t++)
				*t = (unsigned char) edit_get_byte (edit, edit->search_start - pmatch[0].rm_so + pmatch[k].rm_so + j);
			    *t = '\0';
			}
			for (; k <= NUM_REPL_ARGS; k++)
			    sargs[k - 1][0] = 0;
		    }
		    if (sprintf_p (repl_str, exp2.data, PRINTF_ARGS) >= 0) {
			times_replaced++;
			while (i--)
			    edit_delete (edit);
			while (repl_str[++i])
			    edit_insert (edit, repl_str[i]);
		    } else {
			edit_error_dialog (_ (" Replace "),
/* "Invalid regexp string or scanf string" */
			    _ (" Error in replacement format string. "));
			replace_continue = 0;
		    }
                    free (repl_str);
		} else {
		    times_replaced++;
		    while (i--)
			edit_delete (edit);
		    while (++i < exp2.len)
			edit_insert (edit, exp2.data[i]);
		}
		edit->found_len = i;
	    }
/* so that we don't find the same string again */
	    if (option_replace_backwards) {
		last_search = edit->search_start;
		edit->search_start--;
	    } else {
		edit->search_start += i;
		last_search = edit->last_byte;
	    }
	    edit_scroll_screen_over_cursor (edit);
	} else {
	    edit->search_start = edit->curs1;	/* try and find from right here for next search */
	    edit_update_curs_col (edit);

	    edit->force |= REDRAW_PAGE;
	    edit_render_keypress (edit);
	    if (times_replaced) {
		sprintf (fin_string, _ (" %ld replacements made. "), times_replaced);
		edit_message_dialog (_ (" Replace "), fin_string);
	    } else
		edit_message_dialog (_ (" Replace "), _ (" Search string not found. "));
	    replace_continue = 0;
	}
    } while (replace_continue);

  out:
    CStr_free(&exp1);
    CStr_free(&exp2);
    CStr_free(&exp3);
    edit->force = REDRAW_COMPLETELY;
    edit_scroll_screen_over_cursor (edit);
}




void edit_search_cmd (WEdit * edit, int again)
{E_
    int len = 0;
    int cancel = 0;
    static CStr old = {0, 0};
    CStr exp;

    if (!edit) {
	CStr_free(&old);
	return;
    }

    if (!old.data)  /* initialize */
        old = CStr_dup("");

    if (again)
	if (!old.len)
	    return;

    exp = CStr_dupstr(old);

    edit_push_action (edit, KEY_PRESS, edit->start_display);
    if (!again)
        cancel = edit_search_dialog (edit, &exp);

    if (cancel || !exp.len)
        goto out;

    CStr_free(&old);
    old = CStr_dupstr(exp);

	    if (option_search_create_bookmark) {
		int found = 0, books = 0;
		int l = 0, l_last = -1;
		long p, q = 0;
		for (;;) {
		    p = edit_find (q, exp, &len, edit->last_byte,
				   (int (*)(void *, long)) edit_get_byte, (void *) edit, 0);
		    if (p < 0)
			break;
		    found++;
		    l += edit_count_lines (edit, q, p);
		    if (l != l_last) {
			book_mark_insert (edit, l, BOOK_MARK_FOUND_COLOR, 0, 0, 0);
			books++;
		    }
		    l_last = l;
		    q = p + 1;
		}
		if (found) {
		    char fin_string[64];
/* in response to number of bookmarks added because of string being found %d times */
		    sprintf (fin_string, _ (" %d finds made, %d bookmarks added "), found, books);
		    edit_message_dialog (_ (" Search "), fin_string);
		} else {
		    edit_error_dialog (_ (" Search "), _ (" Search string not found. "));
		}
	    } else {

		if (edit->found_len && edit->search_start == edit->found_start && option_replace_backwards)
		    edit->search_start--;

		if (edit->found_len && edit->search_start == edit->found_start && !option_replace_backwards)
		    edit->search_start += edit->found_len;

		edit->search_start = edit_find (edit->search_start, exp, &len, edit->last_byte,
		(int (*)(void *, long)) edit_get_byte, (void *) edit, 0);

		if (edit->search_start >= 0) {
		    edit->found_start = edit->search_start;
		    edit->found_len = len;

		    edit_cursor_move (edit, edit->search_start - edit->curs1);
		    edit_scroll_screen_over_cursor (edit);
/*		    if (option_replace_backwards)
			edit->search_start--;
		    else
			edit->search_start++; */
		} else if (edit->search_start == -3) {
		    edit->search_start = edit->curs1;
		    regexp_error (edit);
		} else {
		    edit->search_start = edit->curs1;
		    edit_error_dialog (_ (" Search "), _ (" Search string not found. "));
		}
	    }


  out:
    CStr_free(&exp);
    edit->force |= REDRAW_COMPLETELY;
    edit_scroll_screen_over_cursor (edit);
}


/* Real edit only */
void edit_quit_cmd (WEdit * edit)
{E_
    edit_push_action (edit, KEY_PRESS, edit->start_display);

#ifndef MIDNIGHT
    if (edit->stopped)
	return;
#endif

    edit->force |= REDRAW_COMPLETELY;
    if (edit->modified) {
#ifdef GTK
	char *r;
	r = gtk_dialog_cauldron (_ (" Quit "), GTK_CAULDRON_TOPLEVEL | GTK_CAULDRON_GRAB, " [ ( %Lxf )xf ]xf / ( %Bgxfq || %Bgxfq || %Bgxfq ) ",
				     _ (" Current text was modified without a file save. \n Save with exit? "), GNOME_STOCK_BUTTON_CANCEL, GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO);
	if (!strcmp (r, GNOME_STOCK_BUTTON_YES)) {
	    edit_push_markers (edit);
	    edit_set_markers (edit, 0, 0, 0, 0);
	    if (!edit_save_cmd (edit))
		return;
	} else if (!strcmp (r, GNOME_STOCK_BUTTON_NO)) {
	    if (edit->delete_file)
		unlink (catstrs (edit->dir, edit->filename, NULL));
	} else {
	    return;
	}
#else
#ifdef MIDNIGHT
	switch (edit_query_dialog3 (_ (" Quit "), _ (" File was modified, Save with exit? "), _ ("Cancel quit"), _ ("&Yes"), _ ("&No"))) {
#else
/* Confirm 'Quit' dialog box */
	switch (edit_query_dialog3 (_ (" Quit "),
				    _ (" Current text was modified without a file save. \n Save with exit? "), _ (" &Cancel quit "), _ (" &Yes "), _ (" &No "))) {
#endif
	case 1:
	    edit_push_markers (edit);
	    edit_set_markers (edit, 0, 0, 0, 0);
	    if (!edit_save_cmd (edit))
		return;
	    break;
	case 2:
#ifdef MIDNIGHT
	    if (edit->delete_file)
		unlink (catstrs (edit->dir, edit->filename, NULL));
#endif
	    break;
	case 0:
	case -1:
	    return;
	}
#endif
    }
#if defined(MIDNIGHT) || defined(GTK)
    else if (edit->delete_file)
	unlink (catstrs (edit->dir, edit->filename, NULL));
#endif
#ifdef MIDNIGHT
    dlg_stop (edit->widget.parent);
#else
#ifdef GTK
    {
           extern char *edit_one_file;

           if (edit_one_file)
                   gtk_main_quit ();
    }
#endif
    edit->stopped = 1;
#endif
}

#define TEMP_BUF_LEN 1024

/* returns a null terminated length of text. Result must be free'd */
unsigned char *edit_get_block (WEdit * edit, long start, long finish, int *l)
{E_
    unsigned char *s, *r;
    r = s = malloc (finish - start + 1);
    if (column_highlighting) {
	*l = 0;
	while (start < finish) {	/* copy from buffer, excluding chars that are out of the column 'margins' */
	    int c, x;
	    x = edit_move_forward3 (edit, edit_bol (edit, start), 0, start);
	    c = edit_get_byte (edit, start);
	    if ((x >= edit->column1 && x < edit->column2)
	     || (x >= edit->column2 && x < edit->column1) || c == '\n') {
		*s++ = c;
		(*l)++;
	    }
	    start++;
	}
    } else {
	*l = finish - start;
	while (start < finish)
	    *s++ = edit_get_byte (edit, start++);
    }
    *s = 0;
    return r;
}

/* save block, returns 1 on success */
int edit_save_block (WEdit * edit, const char *filename, long start, long finish)
{E_
    int len, file;

    if ((file = open_create ((char *) filename, O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_CREATE_MODE)) == -1)
	return 0;

    if (column_highlighting) {
	unsigned char *block, *p;
	int r;
	p = block = edit_get_block (edit, start, finish, &len);
	while (len) {
	    r = write (file, p, len);
	    if (r < 0)
		break;
	    p += r;
	    len -= r;
	}
	free (block);
    } else {
	unsigned char *buf;
	int i = start, end;
	len = finish - start;
	buf = malloc (TEMP_BUF_LEN);
	while (start != finish) {
	    end = min (finish, start + TEMP_BUF_LEN);
	    for (; i < end; i++)
		buf[i - start] = edit_get_byte (edit, i);
	    len -= write (file, (char *) buf, end - start);
	    start = end;
	}
	free (buf);
    }
    close (file);
    if (len)
	return 0;
    return 1;
}

/* copies a block to clipboard file */
static int edit_save_block_to_clip_file (WEdit * edit, long start, long finish)
{E_
    return edit_save_block (edit, catstrs (local_home_dir, CLIP_FILE, NULL), start, finish);
}

#ifndef MIDNIGHT

void paste_text (WEdit * edit, unsigned char *data, unsigned int nitems)
{E_
    if (data) {
	data += nitems - 1;
	while (nitems--)
	    edit_insert_ahead (edit, *data--);
    }
    edit->force |= REDRAW_COMPLETELY;
}

static char *text_to_nroff_out (CStr s)
{E_
    static unsigned char t[1024];
    unsigned char *p = (unsigned char *) s.data;
    int i = 0;
    if (p) {
        int c, j;
	for (j = 0; j < s.len; j++) {
	    c = *p++;
	    if (c < ' ' || c == '\\') {
		t[i++] = '_';
		t[i++] = '\b';
		t[i++] = '\\';
		t[i++] = '_';
		t[i++] = '\b';
		switch (c) {
		case '\0':
		    t[i++] = '0';
		    break;
		case '\\':
		    t[i++] = '\\';
		    break;
		case '\a':
		    t[i++] = 'a';
		    break;
		case '\b':
		    t[i++] = 'b';
		    break;
		case '\t':
		    t[i++] = 't';
		    break;
		case '\n':
		    t[i++] = 'n';
		    break;
		case '\v':
		    t[i++] = 'v';
		    break;
		case '\f':
		    t[i++] = 'f';
		    break;
		case '\r':
		    t[i++] = 'r';
		    break;
		default:
		    i -= 3;
		    t[i++] = '.';
		    break;
		}
	    } else
		t[i++] = c;
	    if (i > 1000)
		break;
	}
    }
    t[i] = 0;
    return (char *) t;
}

char *selection_get_line (void *data, int line)
{E_
    CStr *s;
    s = (CStr *) data;
    return text_to_nroff_out (s[line]);
}

int edit_get_text_from_selection_history (Window parent, int x, int y, int cols, int lines, CStr *r)
{E_
    int i;
    CStr h[NUM_SELECTION_HISTORY];

#if 0
    for (i = 0; i < n_selection_history; i++)
        h[NUM_SELECTION_HISTORY - i - 1] = selection_history[i];
    for (; i < NUM_SELECTION_HISTORY; i++) {
        h[NUM_SELECTION_HISTORY - i - 1].text = (unsigned char *) "";
        h[NUM_SELECTION_HISTORY - i - 1].len = 0;
    }
#endif
    for (i = 0; i < n_selection_history; i++)
        h[i] = selection_history[n_selection_history - i - 1];

#ifdef GTK
    i = -1;
#else
    i = CListboxDialog (parent, x, y, cols, lines, 0, n_selection_history - lines, n_selection_history - 1, n_selection_history, selection_get_line, (void *) h);
#endif
    if (i < 0)
        return 1;
    *r = h[i];
    return 0;
}

void edit_paste_from_history (WEdit * edit)
{E_
    CStr s;
    edit_update_curs_col (edit);
    edit_update_curs_row (edit);
    if (!edit_get_text_from_selection_history (WIN_MESSAGES, max (20, edit->num_widget_columns - 5), 10, &s)) {
        paste_text (edit, (unsigned char *) s.data, s.len);
        edit->force |= REDRAW_COMPLETELY;
    }
}

/* copies a block to the XWindows buffer */
static int edit_XStore_block (WEdit * edit, long start, long finish)
{E_
    edit_get_selection (edit);
    if (edit_selection.len <= 512 * 1024) {	/* we don't want to fill up the server */
	XStoreBytes (CDisplay, (char *) edit_selection.data, (int) edit_selection.len);
	return 0;
    } else
	return 1;
}

int edit_copy_to_X_buf_cmd (WEdit * edit)
{E_
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    edit_XStore_block (edit, start_mark, end_mark);
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Copy to clipboard "), get_sys_error (_(" Unable to save to file. ")));
	return 1;
    }
#ifdef GTK
    gtk_selection_owner_set (GTK_WIDGET (edit->widget), GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
    edit->widget->editable.selection_start_pos = start_mark;
    edit->widget->editable.selection_end_pos = end_mark;
    edit->widget->editable.has_selection = TRUE;
#else
    XSetSelectionOwner (CDisplay, XA_PRIMARY, CWindowOf (edit->widget), CurrentTime);
    XSetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD, CWindowOf (edit->widget), CurrentTime);
#endif
    edit_mark_cmd (edit, 1);
    return 0;
}

int edit_cut_to_X_buf_cmd (WEdit * edit)
{E_
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    edit_XStore_block (edit, start_mark, end_mark);
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Cut to clipboard "), _(" Unable to save to file. "));
	return 1;
    }
    edit_block_delete_cmd (edit);
#ifdef GTK
    gtk_selection_owner_set (GTK_WIDGET (edit->widget), GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
    edit->widget->editable.selection_start_pos = start_mark;
    edit->widget->editable.selection_end_pos = end_mark;
    edit->widget->editable.has_selection = TRUE;
#else
    XSetSelectionOwner (CDisplay, XA_PRIMARY, CWindowOf (edit->widget), CurrentTime);
    XSetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD, CWindowOf (edit->widget), CurrentTime);
#endif
    edit_mark_cmd (edit, 1);
    return 0;
}

void selection_paste (WEdit * edit, Window win, unsigned prop, int delete);
void paste_convert_selection (Window w);

void edit_paste_from_X_buf_cmd (WEdit * edit)
{E_
    if (edit_selection.data && edit_selection.len)
	paste_text (edit, (unsigned char *) edit_selection.data, edit_selection.len);
    else if (!XGetSelectionOwner (CDisplay, XA_PRIMARY))
#ifdef GTK
/* *** */
	;
#else
	selection_paste (edit, CRoot, XA_CUT_BUFFER0, False);
#endif
    else
#ifdef GTK
       gtk_selection_convert (GTK_WIDGET (edit->widget), GDK_SELECTION_PRIMARY,
            gdk_atom_intern ("COMPOUND_TEXT", FALSE), GDK_CURRENT_TIME);
#else
        paste_convert_selection (CWindowOf (edit->widget));
#endif
    edit->force |= REDRAW_PAGE;
}

#else				/* MIDNIGHT */

void edit_paste_from_history (WEdit *edit)
{E_
}

int edit_copy_to_X_buf_cmd (WEdit * edit)
{E_
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Copy to clipboard "), get_sys_error (_(" Unable to save to file. ")));
	return 1;
    }
    edit_mark_cmd (edit, 1);
    return 0;
}

int edit_cut_to_X_buf_cmd (WEdit * edit)
{E_
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Cut to clipboard "), _(" Unable to save to file. "));
	return 1;
    }
    edit_block_delete_cmd (edit);
    edit_mark_cmd (edit, 1);
    return 0;
}

void edit_paste_from_X_buf_cmd (WEdit * edit)
{E_
    edit_insert_file (edit, catstrs (home_dir, CLIP_FILE, NULL));
}

#endif				/* MIDMIGHT */

void edit_goto_cmd (WEdit *edit)
{E_
    char *f;
    static int l = 0;
#ifdef MIDNIGHT
    char s[12];
    sprintf (s, "%d", l);
    f = input_dialog (_(" Goto line "), _(" Enter line: "), l ? s : "");
#else
#ifdef GTK
#if 0
    f = gtk_edit_dialog_input ("goto", 150, l ? itoa (l) : "", _(" Goto line "), _(" Enter line: "));
#else
    char s [12];

    sprintf (s, "%d", l);
    f = (char *) input_dialog (_(" Goto line "), _(" Enter line: "), l ? s : "");
#endif
#else
    f = CInputDialog ("goto", WIN_MESSAGES, 150, l ? itoa (l) : "", _(" Goto line "), _(" Enter line: "));
#endif
#endif
    if (f) {
	if (*f) {
	    l = atoi (f);
	    edit_move_display (edit, l - edit->num_widget_lines / 2 - 1);
	    edit_move_to_line (edit, l - 1);
	    edit->force |= REDRAW_COMPLETELY;
	    free (f);
	}
    }
}

/*returns 1 on success */
int edit_save_block_cmd (WEdit * edit)
{E_
    long start_mark, end_mark;
    char *exp;
    char host[256];

    strcpy (host, edit->host);

    if (eval_marks (edit, &start_mark, &end_mark))
	return 1;
    exp = edit_get_save_file (edit->dir, catstrs (local_home_dir, CLIP_FILE, NULL), _ (" Save Block "), host);
#warning handle host
    edit_push_action (edit, KEY_PRESS, edit->start_display);
    if (exp) {
	if (!*exp) {
	    free (exp);
	    return 0;
	} else {
	    if (edit_save_block (edit, exp, start_mark, end_mark)) {
		free (exp);
		edit->force |= REDRAW_COMPLETELY;
		return 1;
	    } else {
		free (exp);
		edit_error_dialog (_ (" Save Block "), get_sys_error (_ (" Error trying to save file. ")));
		edit->force |= REDRAW_COMPLETELY;
		return 0;
	    }
	}
    }
    edit->force |= REDRAW_COMPLETELY;
    return 0;
}


/* returns 1 on success */
int edit_insert_file_cmd (WEdit * edit)
{E_
    char host[256];
    char *exp;

    strcpy (host, edit->host);

    exp = edit_get_load_file (edit->dir, catstrs (local_home_dir, CLIP_FILE, NULL), _ (" Insert File "), host);
    edit_push_action (edit, KEY_PRESS, edit->start_display);
    if (exp) {
	if (!*exp) {
	    free (exp);
	    return 0;
	} else {
	    if (edit_insert_file (edit, exp)) {
		free (exp);
		edit->force |= REDRAW_COMPLETELY;
		return 1;
	    } else {
		free (exp);
		edit_error_dialog (_ (" Insert file "), get_sys_error (_ (" Error trying to insert file. ")));
		edit->force |= REDRAW_COMPLETELY;
		return 0;
	    }
	}
    }
    edit->force |= REDRAW_COMPLETELY;
    return 0;
}

#ifdef MIDNIGHT

/* sorts a block, returns -1 on system fail, 1 on cancel and 0 on success */
int edit_sort_cmd (WEdit * edit)
{E_
    static char *old = 0;
    char *exp;
    long start_mark, end_mark;
    int e;

    if (eval_marks (edit, &start_mark, &end_mark)) {
/* Not essential to translate */
	edit_error_dialog (_(" Sort block "), _(" You must first highlight a block of text. "));
	return 0;
    }
    edit_save_block (edit, catstrs (home_dir, BLOCK_FILE, NULL), start_mark, end_mark);

    exp = old ? old : "";

    exp = input_dialog (_(" Run Sort "), 
/* Not essential to translate */
    _(" Enter sort options (see manpage) separated by whitespace: "), "");

    if (!exp)
	return 1;
    if (old)
	free (old);
    old = exp;

    e = system (catstrs (" sort ", exp, " ", home_dir, BLOCK_FILE, " > ", home_dir, TEMP_FILE, NULL));
    if (e) {
	if (e == -1 || e == 127) {
	    edit_error_dialog (_(" Sort "), 
/* Not essential to translate */
	    get_sys_error (_(" Error trying to execute sort command ")));
	} else {
	    char q[8];
	    sprintf (q, "%d ", e);
	    edit_error_dialog (_(" Sort "), 
/* Not essential to translate */
	    catstrs (_(" Sort returned non-zero: "), q, NULL));
	}
	return -1;
    }

    edit->force |= REDRAW_COMPLETELY;

    if (edit_block_delete_cmd (edit))
	return 1;
    edit_insert_file (edit, catstrs (home_dir, TEMP_FILE, NULL));
    return 0;
}

/* if block is 1, a block must be highlighted and the shell command
   processes it. If block is 0 the shell command is a straight system
   command, that just produces some output which is to be inserted */
void edit_block_process_cmd (WEdit * edit, const char *shell_cmd, int block)
{E_
    long start_mark, end_mark;
    struct stat s;
    char *f = NULL, *b = NULL;

    if (block) {
	if (eval_marks (edit, &start_mark, &end_mark)) {
	    edit_error_dialog (_(" Process block "), 
/* Not essential to translate */
		_(" You must first highlight a block of text. "));
	    return;
	}
	edit_save_block (edit, b = catstrs (home_dir, BLOCK_FILE, NULL), start_mark, end_mark);
	my_system (0, shell, catstrs (home_dir, shell_cmd, NULL));
	edit_refresh_cmd (edit);
    } else {
	my_system (0, shell, shell_cmd);
	edit_refresh_cmd (edit);
    }

    edit->force |= REDRAW_COMPLETELY;

    f = catstrs (home_dir, ERROR_FILE, NULL);

    if (block) {
	if (stat (f, &s) == 0) {
	    if (!s.st_size) {	/* no error messages */
		if (edit_block_delete_cmd (edit))
		    return;
		edit_insert_file (edit, b);
		return;
	    } else {
		edit_insert_file (edit, f);
		return;
	    }
	} else {
/* Not essential to translate */
	    edit_error_dialog (_(" Process block "), 
/* Not essential to translate */
	    get_sys_error (_(" Error trying to stat file ")));
	    edit->force |= REDRAW_COMPLETELY;
	    return;
	}
    }
}

#endif

int edit_execute_cmd (WEdit * edit, int command, CStr char_for_insertion);

/* prints at the cursor */
/* returns the number of chars printed */
int edit_print_string (WEdit * e, const char *s)
{E_
    edit_execute_cmd (e, -1, CStr_const (s, strlen (s)));
    e->force |= REDRAW_COMPLETELY;
    edit_update_screen (e);
    return strlen (s);
}

int edit_printf (WEdit * e, const char *fmt,...)
{E_
    int i;
    va_list pa;
    char s[1024];
    va_start (pa, fmt);
    vsprintf (s, fmt, pa);
    i = edit_print_string (e, s);
    va_end (pa);
    return i;
}

#ifdef MIDNIGHT

/* FIXME: does this function break NT_OS2 ? */

static void pipe_mail (WEdit *edit, char *to, char *subject, char *cc)
{E_
    FILE *p;
    char *s;
    s = malloc (4096);
    sprintf (s, "mail -s \"%s\" -c \"%s\" \"%s\"", subject, cc, to);
    p = popen (s, "w");
    if (!p) {
	free (s);
	return;
    } else {
	long i;
	for (i = 0; i < edit->last_byte; i++)
	    fputc (edit_get_byte (edit, i), p);
	pclose (p);
    }
    free (s);
}

#define MAIL_DLG_HEIGHT 12

void edit_mail_dialog (WEdit * edit)
{E_
    char *tmail_to;
    char *tmail_subject;
    char *tmail_cc;

    static char *mail_cc_last = 0;
    static char *mail_subject_last = 0;
    static char *mail_to_last = 0;

    QuickDialog Quick_input =
    {50, MAIL_DLG_HEIGHT, -1, 0, N_(" Mail "),
/* NLS ? */
     "[Input Line Keys]", "quick_input", 0};

    QuickWidget quick_widgets[] =
    {
/* NLS ? */
	{quick_button, 6, 10, 9, MAIL_DLG_HEIGHT, N_("&Cancel"), 0, B_CANCEL, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 2, 10, 9, MAIL_DLG_HEIGHT, N_("&Ok"), 0, B_ENTER, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_input, 3, 50, 8, MAIL_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "mail-dlg-input"},
	{quick_label, 2, 50, 7, MAIL_DLG_HEIGHT, N_(" Copies to"), 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, 50, 6, MAIL_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "mail-dlg-input-2"},
	{quick_label, 2, 50, 5, MAIL_DLG_HEIGHT, N_(" Subject"), 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, 50, 4, MAIL_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "mail-dlg-input-3"},
	{quick_label, 2, 50, 3, MAIL_DLG_HEIGHT, N_(" To"), 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_label, 2, 50, 2, MAIL_DLG_HEIGHT, N_(" mail -s <subject> -c <cc> <to>"), 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{0}};

    quick_widgets[2].str_result = &tmail_cc;
    quick_widgets[2].text = mail_cc_last ? mail_cc_last : "";
    quick_widgets[4].str_result = &tmail_subject;
    quick_widgets[4].text = mail_subject_last ? mail_subject_last : "";
    quick_widgets[6].str_result = &tmail_to;
    quick_widgets[6].text = mail_to_last ? mail_to_last : "";

    Quick_input.widgets = quick_widgets;

    if (quick_dialog (&Quick_input) != B_CANCEL) {
	if (mail_cc_last)
	    free (mail_cc_last);
	if (mail_subject_last)
	    free (mail_subject_last);
	if (mail_to_last)
	    free (mail_to_last);
	mail_cc_last = *(quick_widgets[2].str_result);
	mail_subject_last = *(quick_widgets[4].str_result);
	mail_to_last = *(quick_widgets[6].str_result);
	pipe_mail (edit, mail_to_last, mail_subject_last, mail_cc_last);
    }
}

#endif


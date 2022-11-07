/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* dirtools.c - reads and creates a list of directory entries
   Copyright (C) 1996-2022 Paul Sheer
 */

#include "inspect.h"
#include "global.h"
#ifdef MSWIN
#include <config-mswin.h>
#else
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <my_string.h>
#include "stringtools.h"
#include <sys/types.h>

#include <my_string.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coolwidget.h"
#include "loadfile.h"
#include "pool.h"
#include "remotefs.h"

void pstat_to_mode_string (struct portable_stat *ps, char *mode)
{E_
    memset (mode, ' ', 64);

    if (ps->os == OS_TYPE_WINDOWS) {

/*

R        FILE_ATTRIBUTE_READONLY                 0x1
H        FILE_ATTRIBUTE_HIDDEN                   0x2
S        FILE_ATTRIBUTE_SYSTEM                   0x4
?        FILE_ATTRIBUTE_UNKNOWN_3                0x8
d        FILE_ATTRIBUTE_DIRECTORY                0x10
A        FILE_ATTRIBUTE_ARCHIVE                  0x20
D        FILE_ATTRIBUTE_DEVICE                   0x40
N        FILE_ATTRIBUTE_NORMAL                   0x80
T        FILE_ATTRIBUTE_TEMPORARY                0x100
s        FILE_ATTRIBUTE_SPARSE_FILE              0x200
r        FILE_ATTRIBUTE_REPARSE_POINT            0x400
C        FILE_ATTRIBUTE_COMPRESSED               0x800
O        FILE_ATTRIBUTE_OFFLINE                  0x1000
n        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED      0x2000
E        FILE_ATTRIBUTE_ENCRYPTED                0x4000
I        FILE_ATTRIBUTE_INTEGRITY_STREAM         0x8000
V        FILE_ATTRIBUTE_VIRTUAL                  0x10000
o        FILE_ATTRIBUTE_NO_SCRUB_DATA            0x20000
P        FILE_ATTRIBUTE_RECALL_ON_OPEN           0x40000
?        FILE_ATTRIBUTE_UNKNOWN_19               0x80000
?        FILE_ATTRIBUTE_UNKNOWN_20               0x100000
?        FILE_ATTRIBUTE_UNKNOWN_21               0x200000
a        FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS    0x400000
?        FILE_ATTRIBUTE_UNKNOWN_23               0x800000
?        FILE_ATTRIBUTE_UNKNOWN_24               0x1000000
?        FILE_ATTRIBUTE_UNKNOWN_25               0x2000000
?        FILE_ATTRIBUTE_UNKNOWN_26               0x4000000
?        FILE_ATTRIBUTE_UNKNOWN_27               0x8000000
?        FILE_ATTRIBUTE_UNKNOWN_28               0x10000000
?        FILE_ATTRIBUTE_UNKNOWN_29               0x20000000
?        FILE_ATTRIBUTE_UNKNOWN_30               0x40000000
?        FILE_ATTRIBUTE_UNKNOWN_31               0x80000000


*/

        const char *a = "RHS?dADNTsrCOnEIVoP???a?????????????????????????????????????????";
        const char *common = "RHSd";
        int c;
        char *q;
        assert (strlen (a) == 64);
        for (c = 0, q = mode; c < 64; c++)
            if ((ps->wattr.file_attributes & (1ULL << c)))
                *q++ = a[c];
            else if (strchr (common, a[c]))
                *q++ = '-';
        *q = '\0';
    } else {
        unsigned int m;
        m = ps->ustat.st_mode;
        mode[11] = 0;
        mode[0] = '?';
        switch ((unsigned int) m & S_IFMT) {
        case S_IFREG:
	    mode[0] = '-';
	    break;
        case S_IFLNK:
	    mode[0] = 'l';
	    break;
        case S_IFDIR:
	    mode[0] = 'd';
	    break;
        case S_IFCHR:
	    mode[0] = 'c';
	    break;
        case S_IFBLK:
	    mode[0] = 'b';
	    break;
        case S_IFIFO:
	    mode[0] = 'f';
	    break;
        case S_IFSOCK:
	    mode[0] = 's';
	    break;
        case 0110000:
	    mode[0] = 'n';
	    break;
        case 0150000:
	    mode[0] = 'D';
	    break;
        case 0160000:
	    mode[0] = 'w';
	    break;
        }
    
        mode[1] = m & S_IRUSR ? 'r' : '-';
        mode[2] = m & S_IWUSR ? 'w' : '-';
        mode[3] = m & S_ISUID ? (m & S_IXUSR ? 's' : 'S') : (m & S_IXUSR ? 'x' : '-');
    
        mode[4] = m & S_IRGRP ? 'r' : '-';
        mode[5] = m & S_IWGRP ? 'w' : '-';
        mode[6] = m & S_ISGID ? (m & S_IXGRP ? 's' : 'S') : (m & S_IXGRP ? 'x' : '-');
    
        mode[7] = m & S_IROTH ? 'r' : '-';
        mode[8] = m & S_IWOTH ? 'w' : '-';
        mode[9] = m & S_ISVTX ? (m & S_IXOTH ? 't' : 'T') : (m & S_IXOTH ? 'x' : '-');
    }
}

int compare_fileentries (struct file_entry *file_entry1, struct file_entry *file_entry2)
{E_
#if 0
    if (file_entry->options & FILELIST_SORT_...);
#endif
    return (strcmp (file_entry1->name, file_entry2->name));
}

struct file_entry *get_file_entry_list (const char *host, const char *directory, char *last_dir, unsigned long options, const char *filter, char *errmsg)
{E_
    struct remotefs *u;
    int n = 0;
    struct file_entry *list = NULL;
    errmsg[0] = '\0';
    char last_dir_[MAX_PATH_LEN];

    Cstrlcpy (last_dir_, directory, MAX_PATH_LEN);

    u = remotefs_lookup (host, last_dir_);
    if (!*directory)
        directory = last_dir_;
    if ((*u->remotefs_listdir) (u, directory, options, filter, &list, &n, errmsg))
        return NULL;

    if (last_dir)
        Cstrlcpy (last_dir, last_dir_, MAX_PATH_LEN);

    qsort((void *) list, n, sizeof (struct file_entry), (int (*) (const void *, const void *)) compare_fileentries);

    return list;
}

static char *get_a_line (void *data, int line)
{E_
    char **s;
    s = (char **) data;
    return s[line];
}

/* generate a list of search results, and query the user if the list is
longer that one: */
static char *do_user_file_list_search (Window parent, int x, int y, int lines, int columns, char *file_list, const char *base_name)
{E_
    char *p = file_list, *ret = NULL;
    char **l = NULL;
    int list_len = 0, item, i;
    if (!file_list)
	return NULL;
    while ((p = strstr (p, base_name))) {
	char left_word_border, right_word_border;
	left_word_border = (p > file_list) ? *(p - 1) : '\n';
	right_word_border = *(p + strlen (base_name));
	if (left_word_border == '/' && (right_word_border == '\n' || right_word_border == '\0')) {
	    char *eol, *bol, *r;
	    eol = p + strlen (base_name);
	    for (bol = p; bol > file_list && *(bol - 1) != '\n'; bol--);
	    r = (char *) malloc ((int) (eol - bol + 1));
	    strncpy (r, bol, (int) (eol - bol));
	    r[(int) (eol - bol)] = '\0';
	    list_len++;
	    l = (char **) realloc (l, sizeof (char *) * (list_len + 1));
	    l[list_len - 1] = r;
	    l[list_len] = NULL;
	    p = eol;
	    if (!*p)
		break;
	}
	p++;
	if (!*p)
	    break;
    }
    if (!list_len)
	return NULL;
    if (list_len == 1)
	item = 0;
    else
	item = CListboxDialog (parent, 20, 20, 60, list_len >= 15 ? 14 : list_len + 1,
			       _("Multiple Files Found - Please Select"), 0, 0, list_len, get_a_line,
			       (void *) l);
/* free all list entries except the one we are returning: */
    for (i = 0; i < list_len; i++) {
	if (i == item)
	    ret = l[i];
	else
	    free (l[i]);
    }
    free (l);
    return ret;
}

/* generate a list of search results, and query the user if the list is
longer that one: */
static char *do_user_file_list_complete (Window parent, int x, int y, int lines, int columns, char *file_list,
					 const char *search_str)
{E_
    POOL *pool;
    int c;
    char *p, *t, *r;
    pool = pool_init ();
    if (!file_list)
	return NULL;
    if (strlen (search_str) < 2)
	return NULL;
/* list files starting with the text string first */
    for (c = 0; c < 2; c++) {
	p = file_list;
	while ((p = strstr (p, search_str))) {
	    char *eol, *bol;
	    char left_word_border, right_word_border;
	    left_word_border = (p > file_list) ? *(p - 1) : '\n';
	    right_word_border = *(p + strcspn (p, "/\n"));
	    eol = p + strcspn (p, "\n");
	    for (bol = p; bol > file_list && *(bol - 1) != '\n'; bol--);
	    if ((left_word_border == '\n' || (left_word_border == '/' && right_word_border != '/')) ^ c) {
		pool_write (pool, (unsigned char *) bol, (int) (eol - bol));
		pool_write (pool, (unsigned char *) "\n", 1);
	    }
	    p = eol;
	    if (!*p)
		break;
	    p++;
	    if (!*p)
		break;
	}
    }
    pool_null (pool);
    t = (char *) pool_break (pool);
    r = CTrivialSelectionDialog (parent, x, y, lines, columns, t, 0, 0);
    free (t);
    return r;
}

static char *_user_file_list_search (Window parent, int x, int y, int lines, int columns,
				     const char *base_name, char *(*do_dialog) (Window, int, int, int, int,
										char *, const char *))
{E_
    static time_t last_stat_time = 0;
    static time_t last_change_time = 0;
    static char *whole_file = NULL;
    time_t t;
    struct stat st;
    if (!base_name)
	return NULL;
    time (&t);
    if (last_stat_time < t) {
	char *p;
	last_stat_time = t;
	p = (char *) malloc (strlen (local_home_dir) + strlen (FILELIST_FILE) + 2);
	strcpy (p, local_home_dir);
	strcat (p, FILELIST_FILE);
	if (stat (p, &st)) {
	    CErrorDialog (0, 0, 0, _(" Open Personal File List "),
			  get_sys_error (catstrs (_(" Error trying stat "), p, NULL)));
	    free (p);
	    if (whole_file) {
		free (whole_file);
		whole_file = NULL;
	    }
	    return NULL;
	}
	if (last_change_time && last_change_time == st.st_mtime) {
	    free (p);
	    return (*do_dialog) (parent, x, y, lines, columns, whole_file, base_name);
	}
	last_change_time = st.st_mtime;
	if (whole_file)
	    free (whole_file);
	whole_file = loadfile (p, NULL);
	free (p);
	if (!whole_file)
	    return NULL;
    }
    return (*do_dialog) (parent, x, y, lines, columns, whole_file, base_name);
}

char *user_file_list_search (Window parent, int x, int y, const char *base_name)
{E_
    return _user_file_list_search (parent, x, y, 0, 0, base_name, do_user_file_list_search);
}

char *user_file_list_complete (Window parent, int x, int y, int lines, int columns, const char *search_str)
{E_
    return _user_file_list_search (parent, x, y, lines, columns, search_str, do_user_file_list_complete);
}

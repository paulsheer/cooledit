/* cooledit.i - python module for builtin Python interpretor
   Copyright (C) 1998 Paul Sheer

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.
 */

%module cooledit

%{

#define GLOBAL_STARTUP_FILE	"global.py"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "lkeysym.h"

#include "stringtools.h"
#include "coolwidget.h"
#include "edit.h"
#include "editcmddef.h"
#include "editoptions.h"
#include "shell.h"		/* MAX_NUM_SCRIPTS */
#include "find.h"
#include "mad.h"


void coolpython_init (int argc, char **argv);
void coolpython_shut (void);
int coolpython_key (unsigned int state, unsigned int keycode, KeySym keysym);
void coolpython_command (WEdit * edit, int i);
void coolpython_typechange (Window win);

void initcooledit ();
static void coolpython_display_error (int set_sys_last_vars);


#define CHECK_CURRENT		if (!last_edit) return;
#define CHECK_CURRENT_RETURN(x)	if (!last_edit) return (x);

extern CWidget *edit[];
extern long current_edit;
extern long last_edit;

static PyObject *name_space = 0;

/* {{{ bindings */

#define MAX_GLOBAL_BINDINGS 1000
#define MAX_CURRENT_BINDINGS 1000

#define GLOBAL_BINDING(i) ((i) + MAX_CURRENT_BINDINGS)
#define CURRENT_BINDING(i) ((i) + 0)
#define BINDING_GLOBAL(i) ((i) - MAX_CURRENT_BINDINGS)
#define BINDING_CURRENT(i) ((i) - 0)

struct python_binding {
    PyObject *function;
    char *statement;
    KeySym keysym;
    int modifiers;
    struct python_binding *next;
};

static struct {
    char *statement;
    KeySym keysym;
    int modifiers;
} bindings[MAX_GLOBAL_BINDINGS];

int last_binding = 0;

static void free_current_bindings (void *x)
{
    struct python_binding *b, *n;
    for (b = (struct python_binding *) x; b; b = n) {
	n = b->next;
	if (b->statement)
	    free (b->statement);
	Py_XDECREF (b->function);
	free (b);
    }
}

/* returns non-zero on error: i.e. max bindings reached */
static int add_binding (PyObject * function, char *statement, KeySym keysym, int modifiers)
{
    struct python_binding **b, *p;
    int i;
    b = (struct python_binding **) &(edit[current_edit]->user);
    for (i = 0; *b; b = &((*b)->next), i++) {
	if (i >= MAX_CURRENT_BINDINGS)
	    return 1;
	p = *b;
	if (p->keysym == keysym && p->modifiers == modifiers) {
	    Py_XDECREF (p->function);
	    p->function = 0;
	    if (p->statement) {
		free (p->statement);
		p->statement = 0;
	    }
	    if (!function && !statement) {
		p = (*b)->next;
		free (*b);
		*b = p;
		return 0;
	    } else {
		goto no_alloc;
	    }
	}
    }
    p = *b = malloc (sizeof (struct python_binding));
    p->next = 0;
  no_alloc:
    if (function) {
	Py_INCREF (function);
	p->function = function;
	p->statement = 0;
    } else if (statement) {
	p->function = 0;
	p->statement = (char *) strdup (statement);
    } else {
	free (p);
	return 0;
    }
    p->keysym = keysym;
    p->modifiers = modifiers;
    edit[current_edit]->free_user = free_current_bindings;
    return 0;
}

static int add_bindings (PyObject *function, KeySym keysym1, KeySym keysym2, int modifiers)
{
    int r = 0;
    while (keysym1 <= keysym2)
	if ((r = add_binding (function, 0, keysym1++, modifiers)))
	    break;
    return r;
}

int coolpython_key (unsigned int state, unsigned int keycode, KeySym keysym)
{
    struct python_binding *b;
    int i;
    CWidget *w;
/* make sure we are really an editor since bindings should not apply in input/text/other widgets */
    w = CWidgetOfWindow (CGetFocus ());
    if (!w)
	return -1;
    if (w->kind != C_EDITOR_WIDGET)
	return -1;
/* check bindings for current editor */
    for (i = 0, b = (struct python_binding *) edit[current_edit]->user; b; b = b->next, i++)
	if (b->keysym == keysym && b->modifiers == state)
	    return CURRENT_BINDING (i);
/* check global bindings */
    for (i = 0; i < last_binding; i++)
	if (bindings[i].keysym == keysym && bindings[i].modifiers == state)
	    return GLOBAL_BINDING (i);
    return -1;
}

/* }}} bindings */

static void move (long i)
{
    CHECK_CURRENT;
    edit_cursor_move (edit[current_edit]->editor, i);
}

static void move_lines (long i)
{
    CHECK_CURRENT;
    if (i > 0)
	edit_move_up (edit[current_edit]->editor, i, 0);
    else if (i < 0)
	edit_move_down (edit[current_edit]->editor, i, 0);
}

static void move_to (long i)
{
    CHECK_CURRENT;
    edit_cursor_move (edit[current_edit]->editor, i - edit[current_edit]->editor->curs1);
}

static void insert (char *s)
{
    CHECK_CURRENT;
    for (; *s; s++)
	edit_insert (edit[current_edit]->editor, (long) *s);
}

static void delete (long i)
{
    CHECK_CURRENT;
    if (i > 0)
	while (i-- && edit[current_edit]->editor->curs2 > 0)
	    edit_delete (edit[current_edit]->editor);
}

static void back_space (long i)
{
    CHECK_CURRENT;
    if (i > 0)
	while (i-- && edit[current_edit]->editor->curs1 > 0)
	    edit_backspace (edit[current_edit]->editor);
}

static void insert_ahead (char *s)
{
    long l, r;
    CHECK_CURRENT;
    r = l = strlen (s);
    s += l - 1;
    for (; l >= 0; s--, l--)
	edit_insert_ahead (edit[current_edit]->editor, (long) *s);
}

static long current (void)
{
    CHECK_CURRENT_RETURN (-1);
    return edit[current_edit]->editor->curs1;
}

static long current_line (void)
{
    CHECK_CURRENT_RETURN (-1);
    return edit[current_edit]->editor->curs_line;
}

static long bol (long i)
{
    CHECK_CURRENT_RETURN (-1);
    if (i > edit[current_edit]->editor->total_lines || i < 0)
	return -1;
    return edit_find_line (edit[current_edit]->editor, i);
}

static long eol (long i)
{
    CHECK_CURRENT_RETURN (-1);
    if (i > edit[current_edit]->editor->total_lines || i < 0)
	return -1;
    return edit_eol (edit[current_edit]->editor, bol (i));
}

static long find_forwards (long from, char *s)
{
    CHECK_CURRENT_RETURN (-1);
    if (!*s)
	return -1;
    for (; from < edit[current_edit]->editor->last_byte; from++) {
	if (*s == edit_get_byte (edit[current_edit]->editor, from)) {
	    int i;
	    for (i = 1;; i++) {
		if (!s[i])
		    return from;
		if (s[i] == edit_get_byte (edit[current_edit]->editor, from + i))
		    continue;
		break;
	    }
	}
    }
    return -1;
}

static long find_backwards (long from, char *s)
{
    CHECK_CURRENT_RETURN (-1);
    if (!*s)
	return -1;
    for (; from >= 0; from--) {
	if (*s == edit_get_byte (edit[current_edit]->editor, from)) {
	    int i;
	    for (i = 1;; i++) {
		if (!s[i])
		    return from;
		if (s[i] == edit_get_byte (edit[current_edit]->editor, from + i))
		    continue;
		break;
	    }
	}
    }
    return -1;
}

static PyObject *edit__get_text (PyObject * self, PyObject * args)
{
    PyObject *result;
    char *r, *p;
    long i, j = -1, l;
    if (!PyArg_ParseTuple (args, "|ll:get_text", &i, &j))
	return NULL;
    if (i == -1)
	i = edit[current_edit]->editor->curs1;
    if (j == -1)
	j = i + 1;
    l = j - i;
    if (l < 0) {
	PyErr_SetString (PyExc_ValueError, _("second offset is less than first"));
	return NULL;
    }
    p = r = malloc (l + 1);
    if (i < 0)
	i = 0;
    if (j >= edit[current_edit]->editor->last_byte)
	j = edit[current_edit]->editor->last_byte;
    for (; i < j; i++)
	*p++ = edit_get_byte (edit[current_edit]->editor, i);
    *p = '\0';
    result = PyString_FromStringAndSize (r, l);
    free (r);
    return result;
}

%}

%native (get_text) PyObject *edit__get_text (PyObject * self, PyObject * args);

%{

static PyObject *edit__get_line (PyObject * self, PyObject * args)
{
    PyObject *result;
    char *r, *p;
    int i, j, l, l1 = -1, l2 = -1;
    if (!PyArg_ParseTuple (args, "|ii:get_line", &l1, &l2))
	return NULL;
    if (l1 == -1)
	l1 = edit[current_edit]->editor->curs_line;
    if (l2 == -1)
	l2 = l1;
    i = bol (l1);
    j = eol (l2);
    l = j - i;
    if (l < 0) {
	PyErr_SetString (PyExc_ValueError, _("second offset is less than first"));
	return NULL;
    }
    p = r = malloc (l + 1);
    if (i < 0)
	i = 0;
    if (j >= edit[current_edit]->editor->last_byte)
	j = edit[current_edit]->editor->last_byte;
    for (; i < j; i++)
	*p++ = edit_get_byte (edit[current_edit]->editor, i);
    *p = '\0';
    result = PyString_FromStringAndSize (r, l);
    free (r);
    return result;
}

%}

%native (get_line) PyObject *edit__get_line (PyObject * self, PyObject * args);

%{

static long line (long i)
{
    CHECK_CURRENT_RETURN (-1);
    if (i > edit[current_edit]->editor->curs1)
	return edit[current_edit]->editor->curs_line + edit_count_lines (edit[current_edit]->editor, edit[current_edit]->editor->curs1, i);
    return edit[current_edit]->editor->curs_line - edit_count_lines (edit[current_edit]->editor, i, edit[current_edit]->editor->curs1);
}

static void command (long i)
{
    CHECK_CURRENT;
    edit_execute_cmd (edit[current_edit]->editor, i, -1);
}

/* window functions */
static void focus (void)
{
    CHECK_CURRENT;
    CFocus (edit[current_edit]);
    XRaiseWindow (CDisplay, edit[current_edit]->parentid);
    CRaiseWindows ();
    current_to_top ();
}

static long set_editor (char *filename_with_path)
{
    char *f, *d, *p;
    int i, r = 1;
    d = pathdup (filename_with_path);
    p = strrchr (d, '/');
    if (p) {
	p++;
	f = (char *) strdup (p);
	*p = '\0';
    } else {
	free (d);
	return 1;
    }
    for (i = 0; i < last_edit; i++) {
	if (strcmp (f, edit[i]->editor->filename))
	    continue;
	if (strcmp (d, edit[i]->editor->dir))
	    continue;
	current_edit = i;
	r = 0;
	break;
    }
    free (d);
    free (f);
    return r;
}

static void close_window (long force)
{
    if (force)
	edit[current_edit]->editor->stopped = 1;
    else
	edit_execute_command (edit[current_edit]->editor, CK_Exit, -1);
}

static void new_window (void)
{
    new_window_callback (0);
}

static int load (char *filename)
{
    return edit_load_file_from_filename (edit[current_edit]->editor, filename);
}

static void status (char *text)
{
    char id[33];
    CWidget *w;
    strcpy (id, CIdentOf (edit[current_edit]));
    strcat (id, ".text");
    w = CIdent (id);
    free (w->text);
    w->text = (char *) strdup (text);
    render_status (w, 0);
}

static char *file (void)
{
    return edit[current_edit]->editor->filename;
}

static char *directory (void)
{
    return edit[current_edit]->editor->dir;
}

static long modified (void)
{
    return (long) edit[current_edit]->editor->modified;
}

static char *input_dialog (char *title, char *prompt, char *def)
{
    static char x[4096];
    char *p;
    p = CInputDialog (title, 0, 0, 0, 60 * FONT_MEAN_WIDTH, def, title, "%s", prompt);
    if (!p)
	return 0;
    else
	strncpy (x, p, 4095);
    free (p);
    return x;
}

static char *load_file_dialog (char *title, char *dir, char *def)
{
    static char x[4096];
    char *p;
    p = CGetLoadFile (0, 0, 0, dir, def, title);
    if (!p)
	return 0;
    else
	strncpy (x, p, 4095);
    free (p);
    return x;
}

static char *save_file_dialog (char *title, char *dir, char *def)
{
    static char x[4096];
    char *p;
    p = CGetSaveFile (0, 0, 0, dir, def, title);
    if (!p)
	return 0;
    else
	strncpy (x, p, 4095);
    free (p);
    return x;
}

static char *status_input (char *prompt, char *def)
{
    char id[33];
    int width = 0;
    XEvent xevent;
    KeySym k;
    static char t[260];
    char *r;
    CState s;
    CWidget *w, *p = 0, *i;
    r = 0;
    strcpy (id, CIdentOf (edit[current_edit]));
    strcat (id, ".text");
    CBackupState (&s);
    CDisable ("*");
    w = CIdent (id);
    if (*prompt) {
	p = CDrawText ("status_prompt", edit[current_edit]->parentid, CXof (w), CYof (w), "%s", prompt);
	width = CWidthOf (p);
    }
    i = CDrawTextInput ("status_input", edit[current_edit]->parentid, CXof (w) + width, CYof (w),
		 CWidthOf (edit[current_edit]) - width, AUTO_HEIGHT, 256, def);
    CFocus (i);
    for (;;) {
	CNextEvent (&xevent, 0);
	if (xevent.type == KeyPress) {
	    k = CKeySym (&xevent);
	    if (k == XK_KP_Enter || k == XK_Return) {
		strcpy (t, i->text);
		r = t;
		break;
	    }
	    if (k == XK_Escape)
		break;
	}
    }
    if (p)
	CDestroyWidget (p->ident);
    CDestroyWidget (i->ident);
    CRestoreState (&s);
    return r;
}

static void message_dialog (char *title, char *message)
{
    CMessageDialog (0, 0, 0, 0, title, "%s", message);
}

static void error_dialog (char *title, char *message)
{
    CErrorDialog (0, 0, 0, title, "%s", message);
}

static long tab_width (void)
{
    return (long) TAB_SIZE;
}

static long column_pixels (long i)
{
    return 0;
}

static long buffer_size (void)
{
    return edit[current_edit]->editor->last_byte;
}

static void key_press (void)
{
    edit_push_key_press (edit[current_edit]->editor);
}

static void redraw_page (void)
{
    edit[current_edit]->editor->force |= REDRAW_PAGE;
    edit_update_curs_row (edit[current_edit]->editor);
    edit_update_curs_col (edit[current_edit]->editor);
}

static int shell_output (char *title, char *s, char *name)
{
    return execute_background_display_output (title, s, name);
}

%}


long buffer_size (void);
void move (long i);
void move_lines (long i);
void move_to (long i);
void insert (char *s);
void delete (long i);
void back_space (long i);
void insert_ahead (char *s);
long current (void);
long current_line (void);
long bol (long i);
long eol (long i);
long find_forwards (long i, char *s);
long find_backwards (long from, char *s);
long line (long i);
void command (long i);
void focus (void);
long set_editor (char *filename_with_path);
void close_window (long force);
void new_window (void);
int load (char *filename);
void status (char *text);
int modified (void);
char *file (void);
char *directory (void);
char *input_dialog (char *title, char *prompt, char *def);
char *load_file_dialog (char *title, char *dir, char *def);
char *save_file_dialog (char *title, char *dir, char *def);
char *status_input (char *prompt, char *def);
void message_dialog (char *title, char *message);
void error_dialog (char *title, char *message);
long column_pixels (long i);
void key_press (void);
void redraw_page (void);
int shell_output (char *title, char *shell, char *magic);
static long tab_width (void);

%{

PyObject *edit__get_editors (PyObject * self, PyObject * args)
{
    PyObject *ret;
    int i;
    char *p;
    if (!PyArg_ParseTuple (args, ":get_editors"))
	return NULL;
    ret = PyTuple_New (last_edit);
    for (i = 0; i < last_edit; i++) {
	if (edit[i]->editor->filename && edit[i]->editor->dir) {
	    p = malloc (strlen (edit[i]->editor->filename) + strlen (edit[i]->editor->dir) + 1);
	    strcpy (p, edit[i]->editor->dir);
	    strcat (p, edit[i]->editor->filename);
	    PyTuple_SetItem (ret, i, PyString_FromString (p));
	    free (p);
	} else {
	    PyTuple_SetItem (ret, i, Py_None);
	}
    }
    return ret;
}

%}

%native (get_editors) PyObject *edit__get_editors (PyObject * self, PyObject * args);

%{

PyObject *edit__indent (PyObject * self, PyObject * args)
{
    int extra = 0;
    if (!PyArg_ParseTuple (args, "|i:indent", &extra))
	return NULL;
    edit_auto_indent (edit[current_edit]->editor, extra, 0);
    Py_INCREF (Py_None);
    return Py_None;
}

%}

%native (indent) PyObject *edit__indent (PyObject * self, PyObject * args);

%{

PyObject *edit__file_type (PyObject * self, PyObject * args)
{
    PyObject *ret;
    char *type = 0;
    if (!PyArg_ParseTuple (args, "|s:file_type", &type))
	return NULL;
    ret = PyString_FromString (edit[current_edit]->editor->syntax_type ? edit[current_edit]->editor->syntax_type : "");
    if (type) {
	edit_load_syntax (edit[current_edit]->editor, 0, type);
	edit[current_edit]->editor->explicit_syntax = 1;
	edit[current_edit]->editor->force |= REDRAW_PAGE;
    }
    return ret;
}

%}

%native (file_type) PyObject *edit__file_type (PyObject * self, PyObject * args);

%{

PyObject *edit__query_dialog (PyObject * self, PyObject * args)
{
    char *heading, *text;
    char *b1 = 0, *b2 = 0, *b3 = 0, *b4 = 0, *b5 = 0, *b6 = 0, *b7 = 0,
    *b8 = 0, *b9 = 0, *b10 = 0;
    int i;
    if (!PyArg_ParseTuple (args, "ss|ssssssssss:query_dialog", &heading, &text, &b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8, &b9, &b10))
	return NULL;
    i = CQueryDialog (0, 0, 0, heading, text, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, 0);
    return PyInt_FromLong (i);
}

%}

%native (query_dialog) PyObject *edit__query_dialog (PyObject * self, PyObject * args);

%{

PyObject *edit__overwrite (PyObject * self, PyObject * args)
{
    PyObject *ret;
    int i = -1;
    if (!PyArg_ParseTuple (args, "|i:overwrite", &i))
	return NULL;
    ret = PyInt_FromLong (edit[current_edit]->editor->overwrite);
    if (i != -1) {
	edit[current_edit]->editor->overwrite = (i != 0);
	CSetCursorColor (edit[current_edit]->editor->overwrite ? color_palette (24) : color_palette (19));
    }
    return ret;
}

%}

%native (overwrite) PyObject *edit__overwrite (PyObject * self, PyObject * args);

%{

PyObject *edit__generic_dialog (PyObject * self, PyObject * args)
{
    PyObject *a1 = 0, *a2 = 0, *a3 = 0, *a4 = 0, *a5 = 0, *a6 = 0, *a7 = 0, *result = 0;

    char **inputs;
    char **input_labels;
    char **input_names;
    char **input_tool_hints;
    char ***inputs_result;

    char **check_labels;
    char **check_tool_hints;
    static int *check_values;
    int **check_values_result;
    int options = 0;

    char *heading = 0;
    int r, n_inputs, n_checks, width = -1, i;

    if (!PyArg_ParseTuple (args, "sO!O!O!O!O!O!O!|ii:gtk_dialog_cauldron",
			   &heading,
			   &PyTuple_Type, &a1,
			   &PyTuple_Type, &a2,
			   &PyTuple_Type, &a3,
			   &PyTuple_Type, &a4,
			   &PyTuple_Type, &a5,
			   &PyTuple_Type, &a6,
			   &PyTuple_Type, &a7,
			   &width,
			   &options
	))
	return NULL;

    n_inputs = min (PyTuple_Size (a1), min (PyTuple_Size (a2), PyTuple_Size (a3)));
    n_checks = min (PyTuple_Size (a5), PyTuple_Size (a6));

    inputs = malloc ((n_inputs + 1) * sizeof (char *));
    input_labels = malloc ((n_inputs + 1) * sizeof (char *));
    input_names = malloc ((n_inputs + 1) * sizeof (char *));
    input_tool_hints = malloc ((n_inputs + 1) * sizeof (char *));
    inputs_result = malloc ((n_inputs + 1) * sizeof (char **));

    check_labels = malloc ((n_checks + 1) * sizeof (char *));
    check_tool_hints = malloc ((n_checks + 1) * sizeof (char *));
    check_values = malloc ((n_checks + 1) * sizeof (int));
    check_values_result = malloc ((n_checks + 1) * sizeof (int *));

    for (i = 0; i < n_inputs; i++) {
	PyObject *item;
	item = PyTuple_GetItem (a1, i);
	inputs[i] = PyString_Check (item) ? PyString_AsString (item) : "";
	item = PyTuple_GetItem (a2, i);
	input_labels[i] = PyString_Check (item) ? PyString_AsString (item) : "";
	item = PyTuple_GetItem (a3, i);
	input_names[i] = PyString_Check (item) ? PyString_AsString (item) : "";
	input_tool_hints[i] = 0;
	if (i < PyTuple_Size (a4)) {
	    item = PyTuple_GetItem (a4, i);
	    if (PyString_Check (item))
		input_tool_hints[i] = PyString_AsString (item);
	}
	inputs_result[i] = &inputs[i];
    }
    inputs_result[i] = 0;

    for (i = 0; i < n_checks; i++) {
	PyObject *item;
	item = PyTuple_GetItem (a5, i);
	check_values[i] = PyInt_Check (item) ? PyInt_AsLong (item) : 0;
	item = PyTuple_GetItem (a6, i);
	check_labels[i] = PyString_Check (item) ? PyString_AsString (item) : "";
	check_tool_hints[i] = 0;
	if (i < PyTuple_Size (a7)) {
	    item = PyTuple_GetItem (a7, i);
	    if (PyString_Check (item))
		check_tool_hints[i] = PyString_AsString (item);
	}
	check_values_result[i] = &check_values[i];
    }
    check_values_result[i] = 0;

    r = CInputsWithOptions (0, 0, 0, heading, inputs_result, input_labels, input_names, input_tool_hints, check_values_result, check_labels, check_tool_hints, options, max (width > 0 ? width : 60, 20));
    if (!r) {
	result = PyTuple_New (2);
	PyTuple_SetItem (result, 0, PyTuple_New (n_inputs));
	for (i = 0; i < n_inputs; i++)
	    PyTuple_SetItem (PyTuple_GetItem (result, 0), i, PyString_FromString (inputs[i]));
	PyTuple_SetItem (result, 1, PyTuple_New (n_checks));
	for (i = 0; i < n_checks; i++)
	    PyTuple_SetItem (PyTuple_GetItem (result, 1), i, PyInt_FromLong ((long) check_values[i]));
    } else {
	result = Py_None;
	Py_INCREF (result);
    }

    free (inputs);
    free (input_labels);
    free (input_names);
    free (input_tool_hints);
    free (inputs_result);
    free (check_labels);
    free (check_tool_hints);
    free (check_values);
    free (check_values_result);

    return result;
}

%}

%native (generic_dialog) PyObject *edit__generic_dialog (PyObject * self, PyObject * args);

%{

extern int column_highlighting;

PyObject *edit__markers (PyObject * self, PyObject * args)
{
    PyObject *ret;
    long m1 = -1000000000, m2 = -1000000000;
    int c1 = -1000000000, c2 = -1000000000;
    int column = -1;
    long r1, r2;
    if (!PyArg_ParseTuple (args, "|lliii:markers", &m1, &m2, &column, &c1, &c2))
	return NULL;
    if (eval_marks (edit[current_edit]->editor, &r1, &r2)) {
	Py_INCREF (Py_None);
	ret = Py_None;
    } else {
	ret = PyTuple_New (5);
	PyTuple_SetItem (ret, 0, PyInt_FromLong (r1));
	PyTuple_SetItem (ret, 1, PyInt_FromLong (r2));
	PyTuple_SetItem (ret, 3, PyInt_FromLong (column_highlighting));
	PyTuple_SetItem (ret, 4, PyInt_FromLong (min (edit[current_edit]->editor->column1, edit[current_edit]->editor->column2)));
	PyTuple_SetItem (ret, 5, PyInt_FromLong (max (edit[current_edit]->editor->column1, edit[current_edit]->editor->column2)));
    }
    if (m1 != -1000000000) {
	edit[current_edit]->editor->mark1 = m1;
	edit[current_edit]->editor->mark2 = m2 < 0 ? -1 : m2;
	if (column == 0 || column == -1000000000 || c1 == -1000000000 || c2 == -1000000000) {
	    if (column_highlighting)
		edit_push_action (edit[current_edit]->editor, COLUMN_ON);
	    edit_push_markers (edit[current_edit]->editor);
	    column_highlighting = 0;
	    edit[current_edit]->editor->column1 = edit[current_edit]->editor->column2 = 0;
	} else {
	    if (!column_highlighting)
		edit_push_action (edit[current_edit]->editor, COLUMN_OFF);
	    edit_push_markers (edit[current_edit]->editor);
	    column_highlighting = 1;
	    edit[current_edit]->editor->column1 = c1;
	    edit[current_edit]->editor->column2 = c2 < 0 ? -1 : c2;
	}
    }
    return ret;
}

%}

%native (markers) PyObject *edit__markers (PyObject * self, PyObject * args);

%{

PyObject *edit__get_key (PyObject * self, PyObject * args)
{
    PyObject *ret;
    XEvent xevent;
    CState s;
    KeySym k = NoSymbol;
    char *t;
    if (!PyArg_ParseTuple (args, ":get_key"))
	return NULL;
    CBackupState (&s);
    CDisable ("*");
    for (;;) {
	CNextEvent (&xevent, 0);
	if (xevent.type == KeyPress) {
	    k = CKeySym (&xevent);
	    if (k && !mod_type_key (k))
		break;
	}
    }
    CRestoreState (&s);
    ret = PyTuple_New (2);
    t = XKeysymToString (k);
    if (t)
	PyTuple_SetItem (ret, 0, PyString_FromString (t));
    else
	PyTuple_SetItem (ret, 0, Py_None);
    PyTuple_SetItem (ret, 1, PyInt_FromLong (xevent.xkey.state));
    return ret;
}

%}

%native (get_key) PyObject *edit__get_key (PyObject * self, PyObject * args);

%{

PyObject *edit__key (PyObject * self, PyObject * args)
{
    int i, found = 0;
    int modifiers;
    char *key = 0;
    KeySym keysym;
    char *statement = 0;
    if (!PyArg_ParseTuple (args, "si|s:key", &key, &modifiers, &statement))
	return NULL;
    if (!strncmp (key, "XK_", 3))
	key += 3;
    keysym = XStringToKeysym (key);
    if (keysym == NoSymbol) {
	char e[128];
	sprintf (e, _ ("No such key \"%s\".\n\tCheck the header file keysymdef.h on your\n\tsystem for a list of valid keys."), key);
	PyErr_SetString (PyExc_ValueError, e);
	return NULL;
    }
/* is the key already bound? */
    for (i = 0; i < last_binding; i++)
	if (bindings[i].modifiers == modifiers && bindings[i].keysym == keysym) {
	    found = 1;
	    if (bindings[i].statement)
		free (bindings[i].statement);
	    memset (&bindings[i], 0, sizeof (bindings[0]));
	    if (i == last_binding - 1)
		last_binding--;
	    break;
	}
/* check for an empty space in the array */
    if (!found && statement)
	for (i = 0; i < last_binding; i++)
	    if (!bindings[i].statement) {
		found = 1;
		break;
	    }
/* extend the array */
    if (!found && statement) {
	if (last_binding >= MAX_GLOBAL_BINDINGS) {
	    PyErr_SetString (PyExc_ValueError, _ ("maximum number of key bindings reached"));
	    return NULL;
	}
	i = last_binding;
    }
    if (statement) {
	bindings[i].statement = (char *) strdup (statement);
	bindings[i].modifiers = modifiers;
	bindings[i].keysym = keysym;
	if (i == last_binding)
	    last_binding++;
    }
    Py_INCREF (Py_None);
    return Py_None;
}

%}

%native (key) PyObject *edit__key (PyObject * self, PyObject * args);

%{

PyObject *edit__bind (PyObject * self, PyObject * args)
{
    char e[128];
    int modifiers;
    char *key1 = 0, *key2 = 0;
    KeySym keysym1, keysym2;
    PyObject *function = 0;
    if (!PyArg_ParseTuple (args, "ssi|O!:bind", &key1, &key2, &modifiers, &PyFunction_Type, &function))
	return NULL;
    if (!strncmp (key1, "XK_", 3))
	key1 += 3;
    if (!strncmp (key2, "XK_", 3))
	key2 += 3;
    keysym1 = XStringToKeysym (key1);
    keysym2 = XStringToKeysym (key2);
    if (keysym1 == NoSymbol) {
	sprintf (e, _ ("No such key \"%s\".\n\tCheck the header file keysymdef.h on your\n\tsystem for a list of valid keys."), key1);
	PyErr_SetString (PyExc_ValueError, e);
	return NULL;
    }
    if (keysym2 == NoSymbol) {
	sprintf (e, _ ("No such key \"%s\".\n\tCheck the header file keysymdef.h on your\n\tsystem for a list of valid keys."), key2);
	PyErr_SetString (PyExc_ValueError, e);
	return NULL;
    }
    if (add_bindings (function, keysym1, keysym2, modifiers)) {
	sprintf (e, _ ("Reached max number of bindings. Some key in range \"%s\" - \"%s\" not bound."), key1, key2);
	PyErr_SetString (PyExc_ValueError, e);
	return NULL;
    }
    Py_INCREF (Py_None);
    return Py_None;
}

%}

%native (bind) PyObject *edit__bind (PyObject * self, PyObject * args);

%{

static void coolpython_constants (PyObject * d)
{
    struct key_list *k;
    for (k = get_command_list (); *k->key_name; k++)
	if (k->command)
	    PyDict_SetItemString (d, k->key_name, PyInt_FromLong ((long) k->command));
    PyDict_SetItemString (d, "ShiftMask", PyInt_FromLong ((long) ShiftMask));
    PyDict_SetItemString (d, "LockMask", PyInt_FromLong ((long) LockMask));
    PyDict_SetItemString (d, "ControlMask", PyInt_FromLong ((long) ControlMask));
    PyDict_SetItemString (d, "AltMask", PyInt_FromLong ((long) MyAltMask));
    PyDict_SetItemString (d, "Mod1Mask", PyInt_FromLong ((long) Mod1Mask));
    PyDict_SetItemString (d, "Mod2Mask", PyInt_FromLong ((long) Mod2Mask));
    PyDict_SetItemString (d, "Mod3Mask", PyInt_FromLong ((long) Mod3Mask));
    PyDict_SetItemString (d, "Mod4Mask", PyInt_FromLong ((long) Mod4Mask));
    PyDict_SetItemString (d, "Mod5Mask", PyInt_FromLong ((long) Mod5Mask));
    PyDict_SetItemString (d, "LIBDIR", PyString_FromString (LIBDIR));

    PyDict_SetItemString (d, "Mod5Mask", PyInt_FromLong ((long) Mod5Mask));

    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_LOAD_1", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_LOAD_1));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_LOAD_2", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_LOAD_2));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_LOAD_3", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_LOAD_3));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_LOAD_4", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_LOAD_4));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_LOAD_5", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_LOAD_5));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_LOAD_6", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_LOAD_6));

    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_SAVE_1", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_SAVE_1));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_SAVE_2", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_SAVE_2));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_SAVE_3", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_SAVE_3));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_SAVE_4", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_SAVE_4));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_SAVE_5", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_SAVE_5));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_SAVE_6", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_SAVE_6));

    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_DIR_1", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_DIR_1));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_DIR_2", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_DIR_2));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_DIR_3", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_DIR_3));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_DIR_4", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_DIR_4));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_DIR_5", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_DIR_5));
    PyDict_SetItemString (d, "INPUTS_WITH_OPTIONS_BROWSE_DIR_6", PyInt_FromLong ((long) INPUTS_WITH_OPTIONS_BROWSE_DIR_6));
}

void coolpython_run_file (char *filename)
{
    FILE *f;
    PyObject *e;
    f = fopen (filename, "r");
    if (!f) {
	CErrorDialog (0, 0, 0, _("Python Error"), get_sys_error (_(" Error trying to open %s ")), filename);
	return;
    }
    e = PyRun_File (f, filename, Py_file_input, name_space, name_space);
    fclose (f);
    if (!e) {
	coolpython_display_error (1);
	return;
    }
    Py_DECREF (e);
}

static int coolpython_argc = 0;
static char **coolpython_argv = 0;

void coolpython_shut (void)
{
    int i;
#if 0
    Py_Finalize ();
#endif
    for (i = 0; i < last_binding; i++)
	if (bindings[i].statement) {
	    free (bindings[i].statement);
	    memset (&bindings[i], 0, sizeof (bindings[0]));
	}
    last_binding = 0;
}

void coolpython_init (int argc, char **argv)
{
    PyObject *e;
    char s[1024];
    int fd = -1;
    Py_Initialize ();
    if (argv) {
	PySys_SetArgv (coolpython_argc = argc, coolpython_argv = argv);
    } else {
	if (coolpython_argv)
	    PySys_SetArgv (coolpython_argc, coolpython_argv);
    }
#if 0
    Py_XDECREF (name_space);
#endif
    name_space = PyModule_GetDict (PyImport_AddModule ("__main__"));
    initcooledit ();
    sprintf (s, "from cooledit import *\n\
import sys\n\
sys.path.append('%s')\n\
sys.path.append('%s%s')\n\
", LIBDIR, home_dir, EDIT_DIR);
    e = PyRun_String (s, Py_file_input, name_space, name_space);
    if (!e) {
	PyErr_Print ();
	CErrorDialog (0, 0, 0, _ ("Python Error"), _ (" Error initialising Python "));
	return;
    }
    Py_DECREF (e);
    coolpython_constants (name_space);
    sprintf (s, "%s/%s", LIBDIR, GLOBAL_STARTUP_FILE);
    coolpython_run_file (s);
    sprintf (s, "%s%s/%s", home_dir, EDIT_DIR, GLOBAL_STARTUP_FILE);
    if ((fd = open (s, O_RDONLY)) >= 0)
	coolpython_run_file (s);
    if (fd >= 0)
	close (fd);
}

int coolpython_run_function (PyObject * function, char *key_name, int modifiers)
{
    PyObject *p, *ret;
    p = PyTuple_New (2);
    PyTuple_SetItem (p, 0, PyString_FromString (key_name));
    PyTuple_SetItem (p, 1, PyInt_FromLong ((long) modifiers));
    ret = PyObject_CallObject (function, p);
    edit[current_edit]->editor->prev_col = edit_get_col (edit[current_edit]->editor);
    Py_DECREF (p);
    if (ret == NULL) {
	coolpython_display_error (1);
	return -1;
    }
    Py_DECREF (ret);
    PyErr_Clear ();
    return 0;
}

int coolpython_run_statement (char *statement)
{
    PyObject *v;
    v = PyRun_String (statement, Py_file_input, name_space, name_space);
    if (v == NULL) {
	coolpython_display_error (1);
	return -1;
    }
    edit[current_edit]->editor->prev_col = edit_get_col (edit[current_edit]->editor);
    Py_DECREF (v);
    PyErr_Clear ();
    return 0;
}

static void type_change (int i)
{
    char s[256];
    sprintf (s, "type_change ('%s')", edit[i]->editor->syntax_type ? edit[i]->editor->syntax_type : "");
    free_current_bindings (edit[i]->user);
    edit[i]->user = 0;
    coolpython_run_statement (s);
}

void coolpython_typechange (Window win)
{
    int temp_current_edit;
    temp_current_edit = current_edit;
    for (current_edit = 0; current_edit < last_edit; current_edit++)
	if (win == edit[current_edit]->winid) {
	    type_change (current_edit);
	    break;
	}
    current_edit = temp_current_edit;
}

void menu_python_reload (unsigned long ignored)
{
    int temp_current_edit;
    coolpython_shut ();
    coolpython_init (0, 0);
    temp_current_edit = current_edit;
    for (current_edit = 0; current_edit < last_edit; current_edit++)
	type_change (current_edit);
    current_edit = temp_current_edit;
}

/* this is similar to the function edit__menu - if you change here, change there as well */
static PyObject *edit__replace_insert_menu (PyObject * self, PyObject * args, int insert)
{
    char *menu_name, *old_item = 0, *new_item = 0, *statement = 0;
    char ident[33];
    CWidget *w;
    if (!PyArg_ParseTuple (args, "ssss:replace_menu", &menu_name, &old_item, &new_item, &statement))
	return NULL;
    if (!strcasecmp (menu_name, "util")) {
	strcpy (ident, edit[current_edit]->ident);
	strcat (ident, ".util");
    } else if (!strcasecmp (menu_name, "command")) {
	strcpy (ident, "menu.commandmenu");
    } else if (!strcasecmp (menu_name, "readme")) {
	strcpy (ident, "menu.readme");
    } else if (!strcasecmp (menu_name, "search")) {
	strcpy (ident, "menu.searchmenu");
    } else if (!strcasecmp (menu_name, "edit")) {
	strcpy (ident, "menu.editmenu");
    } else if (!strcasecmp (menu_name, "file")) {
	strcpy (ident, "menu.filemenu");
    } else {
	char e[160];
	sprintf (e, "%s \"%s\". %s\n\"Util\", \"File\", \"Edit\", \"Search\", \"Command\", \"Options\" or \"Readme\".", _ ("No such menu:"), _ ("Possible menus are:"), menu_name);
	PyErr_SetString (PyExc_ValueError, e);
	return NULL;
    }
    {
	char *p;
	int i;
	w = CIdent (ident);
	if (w) {
	    if (insert)
		CInsertMenuItem (ident, old_item, new_item, '~', (callfn) coolpython_run_statement, 0);
	    else
		CReplaceMenuItem (ident, old_item, new_item, '~', (callfn) coolpython_run_statement, 0);
	    i = CHasMenuItem (ident, new_item);
	    if (i < 0) {
		char e[160];
		sprintf (e, "%s: %s", _ ("Menu replace failed: Check that there is such a menu item"), old_item);
		PyErr_SetString (PyExc_ValueError, e);
		return NULL;
	    }
/* hack: we want statement to free with destruction of the menu item */
	    p = w->menu[i].text;
	    w->menu[i].text = malloc (strlen (p) + strlen (statement) + 2);
	    strcpy (w->menu[i].text, p);
	    strcpy (w->menu[i].text + strlen (p) + 1, statement);
	    w->menu[i].data = (unsigned long) (w->menu[i].text + strlen (p) + 1);
	    free (p);
	}
    }
    Py_INCREF (Py_None);
    return Py_None;
}

PyObject *edit__insert_menu (PyObject * self, PyObject * args)
{
    return edit__replace_insert_menu (self, args, 1);
}

PyObject *edit__replace_menu (PyObject * self, PyObject * args)
{
    return edit__replace_insert_menu (self, args, 0);
}

/* this is similar to the function edit__replace_menu - if you change here, change there as well */
PyObject *edit__menu (PyObject * self, PyObject * args)
{
    char *menu_name, *menu_item = 0, *statement = 0;
    char ident[33];
    CWidget *w;
    if (!PyArg_ParseTuple (args, "s|ss:menu", &menu_name, &menu_item, &statement))
	return NULL;
    if (!strcasecmp (menu_name, "util")) {
	strcpy (ident, edit[current_edit]->ident);
	strcat (ident, ".util");
    } else if (!strcasecmp (menu_name, "command")) {
	strcpy (ident, "menu.commandmenu");
    } else if (!strcasecmp (menu_name, "readme")) {
	strcpy (ident, "menu.readme");
    } else if (!strcasecmp (menu_name, "search")) {
	strcpy (ident, "menu.searchmenu");
    } else if (!strcasecmp (menu_name, "edit")) {
	strcpy (ident, "menu.editmenu");
    } else if (!strcasecmp (menu_name, "file")) {
	strcpy (ident, "menu.filemenu");
    } else {
	char e[160];
	sprintf (e, "%s \"%s\". %s\n\"Util\", \"File\", \"Edit\", \"Search\", \"Command\", \"Options\" or \"Readme\".", _ ("No such menu:"), _ ("Possible menus are:"), menu_name);
	PyErr_SetString (PyExc_ValueError, e);
	return NULL;
    }
    if (!menu_item) {
	w = CIdent (ident);
	if (w) {
	    int i;
	    for (i = w->numlines - 1; i >= 0; i--)
		CRemoveMenuItemNumber (ident, i);
	}
    } else if (!statement) {
	CRemoveMenuItem (ident, menu_item);
    } else {
	char *p;
	int i;
	w = CIdent (ident);
	if (w) {
	    CAddMenuItem (ident, menu_item, '~', (callfn) coolpython_run_statement, 0);
	    i = w->numlines - 1;
/* hack: we want statement to free with destruction of the menu item */
	    p = w->menu[i].text;
	    w->menu[i].text = malloc (strlen (p) + strlen (statement) + 2);
	    strcpy (w->menu[i].text, p);
	    strcpy (w->menu[i].text + strlen (p) + 1, statement);
	    w->menu[i].data = (unsigned long) (w->menu[i].text + strlen (p) + 1);
	    free (p);
	}
    }
    Py_INCREF (Py_None);
    return Py_None;
}

PyObject *edit__gettext (PyObject * self, PyObject * args)
{
    char *s;
    if (!PyArg_ParseTuple (args, "s:gettext", &s))
	return NULL;
    return PyString_FromString ((char *) _ ((char *) s));
}

%}

%native (menu) PyObject *edit__menu (PyObject * self, PyObject * args);
%native (replace_menu) PyObject *edit__replace_menu (PyObject * self, PyObject * args);
%native (insert_menu) PyObject *edit__insert_menu (PyObject * self, PyObject * args);
%native (gettext) PyObject *edit__gettext (PyObject * self, PyObject * args);
%native (_) PyObject *edit__gettext (PyObject * self, PyObject * args);

%{

void coolpython_command (WEdit * edit, int i)
{
    int k;
    struct python_binding *b;
    if (i >= GLOBAL_BINDING (0) && i < GLOBAL_BINDING (MAX_GLOBAL_BINDINGS)) {
/* lcc breaks here */
	int j;
	j = BINDING_GLOBAL (i);
	i = j;
	if (i >= 0 || i < last_binding)
	    coolpython_run_statement (bindings[i].statement);
    } else if (i >= CURRENT_BINDING (0) && i < CURRENT_BINDING (MAX_CURRENT_BINDINGS)) {
	i = BINDING_CURRENT (i);
	for (k = 0, b = (struct python_binding *) edit->widget->user; b && k <= i; b = b->next, k++) {
	    if (i == k) {
		if (b->statement)
		    coolpython_run_statement (b->statement);
		else
		    coolpython_run_function (b->function, XKeysymToString (b->keysym), b->modifiers);
	    }
	}
    }
}

static void coolpython_display_error (int set_sys_last_vars)
{
    PyObject *e;
    char t[1024];
    PyObject *exception, *v, *tb, *d;
    PyErr_Fetch (&exception, &v, &tb);
    PyErr_NormalizeException (&exception, &v, &tb);
    if (exception == NULL)
	return;
    if (set_sys_last_vars) {
	PySys_SetObject ("last_type", exception);
	PySys_SetObject ("last_value", v);
	PySys_SetObject ("last_traceback", tb);
    }
    d = PyModule_GetDict (PyImport_AddModule ("__main__"));
    PyDict_SetItemString (d, "exc_type", exception);
    PyDict_SetItemString (d, "exc_value", v);
    PyDict_SetItemString (d, "exc_traceback", tb ? tb : Py_None);
    sprintf (t, "\n\
import cooledit\n\
import traceback\n\
s = \"\"\n\
for filename, line, function, text in traceback.extract_tb(exc_traceback):\n\
    s = s + ' File \"%%s\", line %%d, in %%s\\n    %%s' %% (filename, line, function, text)\n\
    if s[-1] != '\\n':\n\
	s = s + '\\n'\n\
for l in traceback.format_exception_only(exc_type, exc_value):\n\
    s = s + ' ' + l\n\
    if s[-1] != '\\n':\n\
	s = s + '\\n'\n\
cooledit.error_dialog (\"%s\", s)\n\
", _("Python Interpretor Error"));
    e = PyRun_String (t, Py_file_input, d, d);
    if (!e) {
	PyErr_Print();
	CErrorDialog (0, 0, 0, _("Python Error"), _(" Error trying to display traceback. \n Message dumped to stderr "));
    }
    Py_XDECREF (e);
    Py_XDECREF (d);
    Py_XDECREF (exception);
    Py_XDECREF (v);
    Py_XDECREF (tb);
}

%}



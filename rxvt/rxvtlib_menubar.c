/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	menubar.c
 *----------------------------------------------------------------------*
 * $Id: menubar.c,v 1.27 1999/01/23 14:26:36 mason Exp $
 *
 * Copyright (C) 1997,1998  mj olesen <olesen@me.QueensU.CA>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*
 * refer.html (or refer.txt) contains up-to-date documentation.  The
 * summary that appears at the end of this file was taken from there.
 *----------------------------------------------------------------------*/

#ifdef MENUBAR

	/* okay to alter menu? */

/* (l)eft, (u)p, (d)own, (r)ight *//* str[0] = strlen (str+1) */

	/* currently active menu */
#endif

/*}}} */

#ifdef MENUBAR
/*
 * find an item called NAME in MENU
 */
/* INTPROTO */
menuitem_t     *menuitem_find (const menu_t * menu, const char *name)
{E_
    menuitem_t     *item;

    assert (name != NULL);
    assert (menu != NULL);

/* find the last item in the menu, this is good for separators */
    for (item = menu->tail; item != NULL; item = item->prev) {
	if (item->entry.type == MenuSubMenu) {
	    if (!strcmp (name, (item->entry.submenu.menu)->name))
		break;
	} else if ((isSeparator (name) && isSeparator (item->name))
		   || !strcmp (name, item->name))
	    break;
    }
    return item;
}
#endif

#ifdef MENUBAR
/*
 * unlink ITEM from its MENU and free its memory
 */
/* INTPROTO */
void            rxvtlib_menuitem_free (rxvtlib *o, menu_t * menu, menuitem_t * item)
{E_
/* disconnect */
    menuitem_t     *prev, *next;

    assert (menu != NULL);

    prev = item->prev;
    next = item->next;
    if (prev != NULL)
	prev->next = next;
    if (next != NULL)
	next->prev = prev;

/* new head, tail */
    if (menu->tail == item)
	menu->tail = prev;
    if (menu->head == item)
	menu->head = next;

    switch (item->entry.type) {
    case MenuAction:
    case MenuTerminalAction:
	FREE (item->entry.action.str);
	break;
    case MenuSubMenu:
	(void)rxvtlib_menu_delete (o, item->entry.submenu.menu);
	break;
    }
    if (item->name != NULL)
	FREE (item->name);
    if (item->name2 != NULL)
	FREE (item->name2);
    FREE (item);
}
#endif

#ifdef MENUBAR
/*
 * sort command vs. terminal actions and
 * remove the first character of STR if it's '\0'
 */
/* INTPROTO */
int             action_type (action_t * action, unsigned char *str)
{E_
    unsigned int    len;

#if defined (DEBUG_MENU) || defined (DEBUG_MENUARROWS)
    len = strlen (str);
    fprintf (stderr, "(len %d) = %s\n", len, str);
#else
    len = Str_escaped ((char *)str);
#endif

    if (!len)
	return -1;

/* sort command vs. terminal actions */
    action->type = MenuAction;
    if (str[0] == '\0') {
	/* the functional equivalent: memmove (str, str+1, len); */
	unsigned char  *dst = (str);
	unsigned char  *src = (str + 1);
	unsigned char  *end = (str + len);

	while (src <= end)
	    *dst++ = *src++;

	len--;			/* decrement length */
	if (str[0] != '\0')
	    action->type = MenuTerminalAction;
    }
    action->str = str;
    action->len = len;

    return 0;
}
#endif

#ifdef MENUBAR
/* INTPROTO */
int             rxvtlib_action_dispatch (rxvtlib *o, action_t * action)
{E_
    switch (action->type) {
    case MenuTerminalAction:
	rxvtlib_cmd_write (o, action->str, action->len);
	break;

    case MenuAction:
	rxvtlib_tt_write (o, action->str, action->len);
	break;

    default:
	return -1;
	break;
    }
    return 0;
}
#endif

#ifdef MENUBAR
/* return the arrow index corresponding to NAME */
/* INTPROTO */
int             rxvtlib_menuarrow_find (rxvtlib *o, char name)
{E_
    int             i;

    for (i = 0; i < NARROWS; i++)
	if (name == o->Arrows[i].name)
	    return i;
    return -1;
}
#endif

#ifdef MENUBAR
/* free the memory associated with arrow NAME of the current menubar */
/* INTPROTO */
void            rxvtlib_menuarrow_free (rxvtlib *o, char name)
{E_
    int             i;

    if (name) {
	i = rxvtlib_menuarrow_find (o, name);
	if (i >= 0) {
	    action_t       *act = &(o->CurrentBar->arrows[i]);

	    switch (act->type) {
	    case MenuAction:
	    case MenuTerminalAction:
		FREE (act->str);
		act->str = NULL;
		act->len = 0;
		break;
	    }
	    act->type = MenuLabel;
	}
    } else {
	for (i = 0; i < NARROWS; i++)
	    rxvtlib_menuarrow_free (o, o->Arrows[i].name);
    }
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menuarrow_add (rxvtlib *o, char *string)
{E_
    int             i;
    unsigned        xtra_len;
    char           *p;

    struct {
	char           *str;
	int             len;
    } beg = {
    NULL, 0}
    , end = {
    NULL, 0}
    , *cur, parse[NARROWS];

    MEMSET (parse, 0, sizeof (parse));

/* fprintf(stderr, "add arrows = `%s'\n", string); */
    for (p = string; p != NULL && *p; string = p) {
	p = (string + 3);
	/* fprintf(stderr, "parsing at %s\n", string); */
	switch (string[1]) {
	case 'b':
	    cur = &beg;
	    break;
	case 'e':
	    cur = &end;
	    break;

	default:
	    i = rxvtlib_menuarrow_find (o, string[1]);
	    if (i >= 0)
		cur = &(parse[i]);
	    else
		continue;	/* not found */
	    break;
	}

	string = p;
	cur->str = string;
	cur->len = 0;

	if (cur == &end) {
	    p = strchr (string, '\0');
	} else {
	    char           *next = string;

	    while (1) {
		p = strchr (next, '<');
		if (p != NULL) {
		    if (p[1] && p[2] == '>')
			break;
		    /* parsed */
		} else {
		    if (beg.str == NULL)	/* no end needed */
			p = strchr (next, '\0');
		    break;
		}
		next = (p + 1);
	    }
	}

	if (p == NULL)
	    return;
	cur->len = (p - string);
    }

#ifdef DEBUG_MENUARROWS
    cur = &beg;
    fprintf (stderr, "<b>(len %d) = %.*s\n",
	     cur->len, cur->len, (cur->str ? cur->str : ""));
    for (i = 0; i < NARROWS; i++) {
	cur = &(parse[i]);
	fprintf (stderr, "<%c>(len %d) = %.*s\n",
		 o->Arrows[i].name,
		 cur->len, cur->len, (cur->str ? cur->str : ""));
    }
    cur = &end;
    fprintf (stderr, "<e>(len %d) = %.*s\n",
	     cur->len, cur->len, (cur->str ? cur->str : ""));
#endif

    xtra_len = (beg.len + end.len);
    for (i = 0; i < NARROWS; i++) {
	if (xtra_len || parse[i].len)
	    rxvtlib_menuarrow_free (o, o->Arrows[i].name);
    }

    for (i = 0; i < NARROWS; i++) {
	unsigned char  *str;
	unsigned int    len;

	if (!parse[i].len)
	    continue;

	str = MALLOC (parse[i].len + xtra_len + 1);
	if (str == NULL)
	    continue;

	len = 0;
	if (beg.len) {
	    STRNCPY (str + len, beg.str, beg.len);
	    len += beg.len;
	}
	STRNCPY (str + len, parse[i].str, parse[i].len);
	len += parse[i].len;

	if (end.len) {
	    STRNCPY (str + len, end.str, end.len);
	    len += end.len;
	}
	str[len] = '\0';

#ifdef DEBUG_MENUARROWS
	fprintf (stderr, "<%c>(len %d) = %s\n", o->Arrows[i].name, len, str);
#endif
	if (action_type (&(o->CurrentBar->arrows[i]), str) < 0)
	    FREE (str);
    }
}
#endif

#ifdef MENUBAR
/* INTPROTO */
menuitem_t     *rxvtlib_menuitem_add (rxvtlib *o, menu_t * menu, const char *name,
			      const char *name2, const char *action)
{E_
    menuitem_t     *item;
    unsigned int    len;

    assert (name != NULL);
    assert (action != NULL);

    if (menu == NULL)
	return NULL;

    if (isSeparator (name)) {
	/* add separator, no action */
	name = "";
	action = "";
    } else {
	/*
	 * add/replace existing menu item
	 */
	item = menuitem_find (menu, name);
	if (item != NULL) {
	    if (item->name2 != NULL && name2 != NULL) {
		FREE (item->name2);
		item->len2 = 0;
		item->name2 = NULL;
	    }
	    switch (item->entry.type) {
	    case MenuAction:
	    case MenuTerminalAction:
		FREE (item->entry.action.str);
		item->entry.action.str = NULL;
		break;
	    }
	    goto Item_Found;
	}
    }
/* allocate a new itemect */
    if ((item = (menuitem_t *) MALLOC (sizeof (menuitem_t))) == NULL)
	return NULL;

    item->len2 = 0;
    item->name2 = NULL;

    len = strlen (name);
    item->name = MALLOC (len + 1);
    if (item->name != NULL) {
	STRCPY (item->name, name);
	if (name[0] == '.' && name[1] != '.')
	    len = 0;		/* hidden menu name */
    } else {
	FREE (item);
	return NULL;
    }
    item->len = len;

/* add to tail of list */
    item->prev = menu->tail;
    item->next = NULL;

    if (menu->tail != NULL)
	(menu->tail)->next = item;
    menu->tail = item;
/* fix head */
    if (menu->head == NULL)
	menu->head = item;

/*
 * add action
 */
  Item_Found:
    if (name2 != NULL && item->name2 == NULL) {
	len = strlen (name2);
	if (len == 0 || (item->name2 = MALLOC (len + 1)) == NULL) {
	    len = 0;
	    item->name2 = NULL;
	} else {
	    STRCPY (item->name2, name2);
	}
	item->len2 = len;
    }
    item->entry.type = MenuLabel;
    len = strlen (action);

    if (len == 0 && item->name2 != NULL) {
	action = item->name2;
	len = item->len2;
    }
    if (len) {
	unsigned char  *str = MALLOC (len + 1);

	if (str == NULL) {
	    rxvtlib_menuitem_free (o, menu, item);
	    return NULL;
	}
	STRCPY (str, action);

	if (action_type (&(item->entry.action), str) < 0)
	    FREE (str);
    }
/* new item and a possible increase in width */
    if (menu->width < (item->len + item->len2))
	menu->width = (item->len + item->len2);

    return item;
}
#endif

#ifdef MENUBAR
/*
 * search for the base starting menu for NAME.
 * return a pointer to the portion of NAME that remains
 */
/* INTPROTO */
char           *rxvtlib_menu_find_base (rxvtlib *o, menu_t ** menu, char *path)
{E_
    menu_t         *m = NULL;
    menuitem_t     *item;

    assert (menu != NULL);
    assert (o->CurrentBar != NULL);

    if (path[0] == '\0')
	return path;

    if (strchr (path, '/') != NULL) {
	register char  *p = path;

	while ((p = strchr (p, '/')) != NULL) {
	    p++;
	    if (*p == '/')
		path = p;
	}
	if (path[0] == '/') {
	    path++;
	    *menu = NULL;
	}
	while ((p = strchr (path, '/')) != NULL) {
	    p[0] = '\0';
	    if (path[0] == '\0')
		return NULL;
	    if (!strcmp (path, DOT)) {
		/* nothing to do */
	    } else if (!strcmp (path, DOTS)) {
		if (*menu != NULL)
		    *menu = (*menu)->parent;
	    } else {
		path = rxvtlib_menu_find_base (o, menu, path);
		if (path[0] != '\0') {	/* not found */
		    p[0] = '/';	/* fix-up name again */
		    return path;
		}
	    }

	    path = (p + 1);
	}
    }
    if (!strcmp (path, DOTS)) {
	path += strlen (DOTS);
	if (*menu != NULL)
	    *menu = (*menu)->parent;
	return path;
    }
/* find this menu */
    if (*menu == NULL) {
	for (m = o->CurrentBar->tail; m != NULL; m = m->prev) {
	    if (!strcmp (path, m->name))
		break;
	}
    } else {
	/* find this menu */
	for (item = (*menu)->tail; item != NULL; item = item->prev) {
	    if (item->entry.type == MenuSubMenu
		&& !strcmp (path, (item->entry.submenu.menu)->name)) {
		m = (item->entry.submenu.menu);
		break;
	    }
	}
    }
    if (m != NULL) {
	*menu = m;
	path += strlen (path);
    }
    return path;
}
#endif

#ifdef MENUBAR
/*
 * delete this entire menu
 */
/* INTPROTO */
menu_t         *rxvtlib_menu_delete (rxvtlib *o, menu_t * menu)
{E_
    menu_t         *parent = NULL, *prev, *next;
    menuitem_t     *item;

    assert (o->CurrentBar != NULL);

/* delete the entire menu */
    if (menu == NULL)
	return NULL;

    parent = menu->parent;

/* unlink MENU */
    prev = menu->prev;
    next = menu->next;
    if (prev != NULL)
	prev->next = next;
    if (next != NULL)
	next->prev = prev;

/* fix the index */
    if (parent == NULL) {
	const int       len = (menu->len + HSPACE);

	if (o->CurrentBar->tail == menu)
	    o->CurrentBar->tail = prev;
	if (o->CurrentBar->head == menu)
	    o->CurrentBar->head = next;

	for (next = menu->next; next != NULL; next = next->next)
	    next->x -= len;
    } else {
	for (item = parent->tail; item != NULL; item = item->prev) {
	    if (item->entry.type == MenuSubMenu
		&& item->entry.submenu.menu == menu) {
		item->entry.submenu.menu = NULL;
		rxvtlib_menuitem_free (o, menu->parent, item);
		break;
	    }
	}
    }

    item = menu->tail;
    while (item != NULL) {
	menuitem_t     *p = item->prev;

	rxvtlib_menuitem_free (o, menu, item);
	item = p;
    }

    if (menu->name != NULL)
	FREE (menu->name);
    FREE (menu);

    return parent;
}
#endif

#ifdef MENUBAR
/* INTPROTO */
menu_t         *rxvtlib_menu_add (rxvtlib *o, menu_t * parent, char *path)
{E_
    menu_t         *menu;

    assert (o->CurrentBar != NULL);

    if (strchr (path, '/') != NULL) {
	register char  *p;

	if (path[0] == '/') {
	    /* shouldn't happen */
	    path++;
	    parent = NULL;
	}
	while ((p = strchr (path, '/')) != NULL) {
	    p[0] = '\0';
	    if (path[0] == '\0')
		return NULL;

	    parent = rxvtlib_menu_add (o, parent, path);
	    path = (p + 1);
	}
    }
    if (!strcmp (path, DOTS))
	return (parent != NULL ? parent->parent : parent);

    if (!strcmp (path, DOT) || path[0] == '\0')
	return parent;

/* allocate a new menu */
    if ((menu = (menu_t *) MALLOC (sizeof (menu_t))) == NULL)
	return parent;

    menu->width = 0;
    menu->parent = parent;
    menu->len = strlen (path);
    menu->name = MALLOC ((menu->len + 1));
    if (menu->name == NULL) {
	FREE (menu);
	return parent;
    }
    STRCPY (menu->name, path);

/* initialize head/tail */
    menu->head = menu->tail = NULL;
    menu->prev = menu->next = NULL;

    menu->win = None;
    menu->x = menu->y = menu->w = menu->h = 0;
    menu->item = NULL;

/* add to tail of list */
    if (parent == NULL) {
	menu->prev = o->CurrentBar->tail;
	if (o->CurrentBar->tail != NULL)
	    o->CurrentBar->tail->next = menu;
	o->CurrentBar->tail = menu;
	if (o->CurrentBar->head == NULL)
	    o->CurrentBar->head = menu;	/* fix head */
	if (menu->prev)
	    menu->x = (menu->prev->x + menu->prev->len + HSPACE);
    } else {
	menuitem_t     *item;

	item = rxvtlib_menuitem_add (o, parent, path, "", "");
	if (item == NULL) {
	    FREE (menu);
	    return parent;
	}
	assert (item->entry.type == MenuLabel);
	item->entry.type = MenuSubMenu;
	item->entry.submenu.menu = menu;
    }

    return menu;
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_drawbox_menubar (rxvtlib *o, int x, int len, int state)
{E_
    GC              top, bot;

    x = Width2Pixel (x);
    len = Width2Pixel (len + HSPACE);
    if (x >= o->TermWin.width)
	return;
    else if (x + len >= o->TermWin.width)
	len = (TermWin_TotalWidth () - x);

#ifdef MENUBAR_SHADOW_IN
    state = -state;
#endif
    switch (state) {
    case +1:
	top = o->topShadowGC;
	bot = o->botShadowGC;
	break;			/* SHADOW_OUT */
    case -1:
	top = o->botShadowGC;
	bot = o->topShadowGC;
	break;			/* SHADOW_IN */
    default:
	top = bot = o->neutralGC;
	break;			/* neutral */
    }

    rxvtlib_Draw_Shadow (o, o->menuBar.win, top, bot, x, 0, len, menuBar_TotalHeight ());
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_drawtriangle (rxvtlib *o, int x, int y, int state)
{E_
    GC              top, bot;
    int             w;

#ifdef MENU_SHADOW_IN
    state = -state;
#endif
    switch (state) {
    case +1:
	top = o->topShadowGC;
	bot = o->botShadowGC;
	break;			/* SHADOW_OUT */
    case -1:
	top = o->botShadowGC;
	bot = o->topShadowGC;
	break;			/* SHADOW_IN */
    default:
	top = bot = o->neutralGC;
	break;			/* neutral */
    }

    w = Height2Pixel (1) - 2 * SHADOW;

    x -= SHADOW + (3 * w / 2);
    y += SHADOW * 3;

    rxvtlib_Draw_Triangle (o, o->ActiveMenu->win, top, bot, x, y, w, 'r');
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_drawbox_menuitem (rxvtlib *o, int y, int state)
{E_
    GC              top, bot;

#ifdef MENU_SHADOW_IN
    state = -state;
#endif
    switch (state) {
    case +1:
	top = o->topShadowGC;
	bot = o->botShadowGC;
	break;			/* SHADOW_OUT */
    case -1:
	top = o->botShadowGC;
	bot = o->topShadowGC;
	break;			/* SHADOW_IN */
    default:
	top = bot = o->neutralGC;
	break;			/* neutral */
    }

    rxvtlib_Draw_Shadow (o, o->ActiveMenu->win, top, bot,
		 SHADOW + 0,
		 SHADOW + y,
		 o->ActiveMenu->w - 2 * (SHADOW), HEIGHT_TEXT + 2 * SHADOW);
    XFlush (o->Xdisplay);
}
#endif

#ifdef DEBUG_MENU_LAYOUT
#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_print_menu_ancestors (rxvtlib *o, menu_t * menu)
{E_
    if (menu == NULL) {
	fprintf (stderr, "Top Level menu\n");
	return;
    }
    fprintf (stderr, "menu %s ", menu->name);
    if (menu->parent != NULL) {
	menuitem_t     *item;

	for (item = menu->parent->head; item != NULL; item = item->next) {
	    if (item->entry.type == MenuSubMenu
		&& item->entry.submenu.menu == menu) {
		break;
	    }
	}
	if (item == NULL) {
	    fprintf (stderr, "is an orphan!\n");
	    return;
	}
    }
    fprintf (stderr, "\n");
    rxvtlib_print_menu_ancestors (o, menu->parent);
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_print_menu_descendants (rxvtlib *o, menu_t * menu)
{E_
    menuitem_t     *item;
    menu_t         *parent;
    int             i, level = 0;

    parent = menu;
    do {
	level++;
	parent = parent->parent;
    }
    while (parent != NULL);

    for (i = 0; i < level; i++)
	fprintf (stderr, ">");
    fprintf (stderr, "%s\n", menu->name);

    for (item = menu->head; item != NULL; item = item->next) {
	if (item->entry.type == MenuSubMenu) {
	    if (item->entry.submenu.menu == NULL)
		fprintf (stderr, "> %s == NULL\n", item->name);
	    else
		rxvtlib_print_menu_descendants (o, item->entry.submenu.menu);
	} else {
	    for (i = 0; i < level; i++)
		fprintf (stderr, "+");
	    if (item->entry.type == MenuLabel)
		fprintf (stderr, "label: ");
	    fprintf (stderr, "%s\n", item->name);
	}
    }

    for (i = 0; i < level; i++)
	fprintf (stderr, "<");
    fprintf (stderr, "\n");
}
#endif
#endif

#ifdef MENUBAR
/* pop up/down the current menu and redraw the menuBar button */
/* INTPROTO */
void            rxvtlib_menu_show (rxvtlib *o)
{E_
    int             x, y, xright;
    menuitem_t     *item;

    if (o->ActiveMenu == NULL)
	return;

    x = o->ActiveMenu->x;
    if (o->ActiveMenu->parent == NULL) {
	register int    h;

	rxvtlib_drawbox_menubar (o, x, o->ActiveMenu->len, -1);
	x = Width2Pixel (x);

	o->ActiveMenu->y = 1;
	o->ActiveMenu->w = Menu_PixelWidth (o->ActiveMenu);

	if ((x + o->ActiveMenu->w) >= o->TermWin.width)
	    x = (TermWin_TotalWidth () - o->ActiveMenu->w);

	/* find the height */
	for (h = 0, item = o->ActiveMenu->head; item != NULL; item = item->next)
	    h += isSeparator (item->name) ? HEIGHT_SEPARATOR
		: HEIGHT_TEXT + 2 * SHADOW;
	o->ActiveMenu->h = h + 2 * SHADOW;
    }
    if (o->ActiveMenu->win == None) {
	o->ActiveMenu->win = XCreateSimpleWindow (o->Xdisplay, o->TermWin.vt,
					       x,
					       o->ActiveMenu->y,
					       o->ActiveMenu->w,
					       o->ActiveMenu->h,
					       0,
					       o->PixColors[Color_fg],
					       o->PixColors[Color_scroll]);
	XMapWindow (o->Xdisplay, o->ActiveMenu->win);
    }
    rxvtlib_Draw_Shadow (o, o->ActiveMenu->win,
		 o->topShadowGC, o->botShadowGC, 0, 0, o->ActiveMenu->w, o->ActiveMenu->h);

/* determine the correct right-alignment */
    for (xright = 0, item = o->ActiveMenu->head; item != NULL; item = item->next)
	if (item->len2 > xright)
	    xright = item->len2;

    for (y = 0, item = o->ActiveMenu->head; item != NULL; item = item->next) {
	const int       xoff = (SHADOW + Width2Pixel (HSPACE) / 2);
	register int    h;
	GC              gc = o->menubarGC;

	if (isSeparator (item->name)) {
	    rxvtlib_Draw_Shadow (o, o->ActiveMenu->win,
			 o->topShadowGC, o->botShadowGC,
			 SHADOW, y + SHADOW + 1, o->ActiveMenu->w - 2 * SHADOW,
			 0);
	    h = HEIGHT_SEPARATOR;
	} else {
	    char           *name = item->name;
	    int             len = item->len;

	    if (item->entry.type == MenuLabel) {
		gc = o->botShadowGC;
	    } else if (item->entry.type == MenuSubMenu) {
		int             x1, y1;
		menuitem_t     *it;
		menu_t         *menu = item->entry.submenu.menu;

		rxvtlib_drawtriangle (o, o->ActiveMenu->w, y, +1);

		name = menu->name;
		len = menu->len;

		y1 = o->ActiveMenu->y + y;

		menu->w = Menu_PixelWidth (menu);

		/* place sub-menu at midpoint of parent menu */
		x1 = o->ActiveMenu->w / 2;
		if (x1 > menu->w)	/* right-flush menu if too small */
		    x1 += (x1 - menu->w);
		x1 += x;

		/* find the height of this submenu */
		for (h = 0, it = menu->head; it != NULL; it = it->next)
		    h += isSeparator (it->name) ? HEIGHT_SEPARATOR
			: HEIGHT_TEXT + 2 * SHADOW;
		menu->h = h + 2 * SHADOW;

		/* ensure menu is in window limits */
		if ((x1 + menu->w) >= o->TermWin.width)
		    x1 = (TermWin_TotalWidth () - menu->w);

		if ((y1 + menu->h) >= o->TermWin.height)
		    y1 = (TermWin_TotalHeight () - menu->h);

		menu->x = (x1 < 0 ? 0 : x1);
		menu->y = (y1 < 0 ? 0 : y1);
	    } else if (item->name2 && !strcmp (name, item->name2))
		name = NULL;

	    if (len && name)
		XDrawString (o->Xdisplay,
			     o->ActiveMenu->win, gc,
			     xoff,
			     2 * SHADOW + y + o->TermWin.font->ascent + 1,
			     name, len);

	    len = item->len2;
	    name = item->name2;
	    if (len && name)
		XDrawString (o->Xdisplay,
			     o->ActiveMenu->win, gc,
			     o->ActiveMenu->w - (xoff + Width2Pixel (xright)),
			     2 * SHADOW + y + o->TermWin.font->ascent + 1,
			     name, len);

	    h = HEIGHT_TEXT + 2 * SHADOW;
	}
	y += h;
    }
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menu_display (rxvtlib *o, void (*update) (rxvtlib *))
{E_
    if (o->ActiveMenu == NULL)
	return;
    if (o->ActiveMenu->win != None)
	XDestroyWindow (o->Xdisplay, o->ActiveMenu->win);
    o->ActiveMenu->win = None;
    o->ActiveMenu->item = NULL;

    if (o->ActiveMenu->parent == NULL)
	rxvtlib_drawbox_menubar (o, o->ActiveMenu->x, o->ActiveMenu->len, +1);
    o->ActiveMenu = o->ActiveMenu->parent;
    update (o);
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menu_hide_all (rxvtlib *o)
{E_
    rxvtlib_menu_display (o, rxvtlib_menu_hide_all);
}

#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menu_hide (rxvtlib *o)
{E_
    rxvtlib_menu_display (o, rxvtlib_menu_show);
}

#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menu_clear (rxvtlib *o, menu_t * menu)
{E_
    if (menu != NULL) {
	menuitem_t     *item = menu->tail;

	while (item != NULL) {
	    rxvtlib_menuitem_free (o, menu, item);
	    /* it didn't get freed ... why? */
	    if (item == menu->tail)
		return;
	    item = menu->tail;
	}
	menu->width = 0;
    }
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menubar_clear (rxvtlib *o)
{E_
    if (o->CurrentBar != NULL) {
	menu_t         *menu = o->CurrentBar->tail;

	while (menu != NULL) {
	    menu_t         *prev = menu->prev;

	    rxvtlib_menu_delete (o, menu);
	    menu = prev;
	}
	o->CurrentBar->head = o->CurrentBar->tail = o->ActiveMenu = NULL;

	if (o->CurrentBar->title) {
	    FREE (o->CurrentBar->title);
	    o->CurrentBar->title = NULL;
	}
	rxvtlib_menuarrow_free (o, 0);	/* remove all arrow functions */
    }
    o->ActiveMenu = NULL;
}
#endif

#if (MENUBAR_MAX > 1)
#ifdef MENUBAR
/* find if menu already exists */
/* INTPROTO */
bar_t          *rxvtlib_menubar_find (rxvtlib *o, const char *name)
{E_
    bar_t          *bar = o->CurrentBar;

#ifdef DEBUG_MENUBAR_STACKING
    fprintf (stderr, "looking for [menu:%s] ...", name ? name : "(nil)");
#endif
    if (bar == NULL || name == NULL)
	return NULL;

    if (strlen (name) && strcmp (name, "*")) {
	do {
	    if (!strcmp (bar->name, name)) {
#ifdef DEBUG_MENUBAR_STACKING
		fprintf (stderr, " found!\n");
#endif
		return bar;
	    }
	    bar = bar->next;
	}
	while (bar != o->CurrentBar);
	bar = NULL;
    }
#ifdef DEBUG_MENUBAR_STACKING
    fprintf (stderr, "%s found!\n", (bar ? "" : " NOT"));
#endif

    return bar;
}
#endif

#ifdef MENUBAR
/* INTPROTO */
int             rxvtlib_menubar_push (rxvtlib *o, const char *name)
{E_
    int             ret = 1;
    bar_t          *bar;

    if (o->CurrentBar == NULL) {
	/* allocate first one */
	bar = (bar_t *) MALLOC (sizeof (bar_t));

	if (bar == NULL)
	    return 0;

	MEMSET (bar, 0, sizeof (bar_t));
	/* circular linked-list */
	bar->next = bar->prev = bar;
	bar->head = bar->tail = NULL;
	bar->title = NULL;
	o->CurrentBar = bar;
	o->Nbars++;

	rxvtlib_menubar_clear (o);
    } else {
	/* find if menu already exists */
	bar = rxvtlib_menubar_find (o, name);
	if (bar != NULL) {
	    /* found it, use it */
	    o->CurrentBar = bar;
	} else {
	    /* create if needed, or reuse the existing empty menubar */
	    if (o->CurrentBar->head != NULL) {
		/* need to malloc another one */
		if (o->Nbars < MENUBAR_MAX)
		    bar = (bar_t *) MALLOC (sizeof (bar_t));
		else
		    bar = NULL;

		/* malloc failed or too many menubars, reuse another */
		if (bar == NULL) {
		    bar = o->CurrentBar->next;
		    ret = -1;
		} else {
		    bar->head = bar->tail = NULL;
		    bar->title = NULL;

		    bar->next = o->CurrentBar->next;
		    o->CurrentBar->next = bar;
		    bar->prev = o->CurrentBar;
		    bar->next->prev = bar;

		    o->Nbars++;
		}
		o->CurrentBar = bar;

	    }
	    rxvtlib_menubar_clear (o);
	}
    }

/* give menubar this name */
    STRNCPY (o->CurrentBar->name, name, MAXNAME);
    o->CurrentBar->name[MAXNAME - 1] = '\0';

    return ret;
}
#endif

#ifdef MENUBAR
/* switch to a menu called NAME and remove it */
/* INTPROTO */
void            rxvtlib_menubar_remove (rxvtlib *o, const char *name)
{E_
    bar_t          *bar;

    if ((bar = rxvtlib_menubar_find (o, name)) == NULL)
	return;
    o->CurrentBar = bar;

    do {
	rxvtlib_menubar_clear (o);
	/*
	 * pop a menubar, clean it up first
	 */
	if (o->CurrentBar != NULL) {
	    bar_t          *prev = o->CurrentBar->prev;
	    bar_t          *next = o->CurrentBar->next;

	    if (prev == next && prev == o->CurrentBar) {	/* only 1 left */
		prev = NULL;
		o->Nbars = 0;	/* safety */
	    } else {
		next->prev = prev;
		prev->next = next;
		o->Nbars--;
	    }

	    FREE (o->CurrentBar);
	    o->CurrentBar = prev;
	}
    }
    while (o->CurrentBar && !strcmp (name, "*"));
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            action_decode (FILE * fp, action_t * act)
{E_
    unsigned char  *str;
    short           len;

    if (act == NULL || (len = act->len) == 0 || (str = act->str) == NULL)
	return;

    if (act->type == MenuTerminalAction) {
	fprintf (fp, "^@");
	/* can strip trailing ^G from XTerm sequence */
	if (str[0] == 033 && str[1] == ']' && str[len - 1] == 007)
	    len--;
    } else if (str[0] == 033) {
	switch (str[1]) {
	case '[':
	case ']':
	    break;

	case 'x':
	    /* can strip trailing '\r' from M-x sequence */
	    if (str[len - 1] == '\r')
		len--;
	    /* FALLTHROUGH */

	default:
	    fprintf (fp, "M-");	/* meta prefix */
	    str++;
	    len--;
	    break;
	}
    }
/*
 * control character form is preferred, since backslash-escaping
 * can be really ugly looking when the backslashes themselves also
 * have to be escaped to avoid Shell (or whatever scripting
 * language) interpretation
 */
    while (len > 0) {
	unsigned char   ch = *str++;

	switch (ch) {
	case 033:
	    fprintf (fp, "\\E");
	    break;		/* escape */
	case '\r':
	    fprintf (fp, "\\r");
	    break;		/* carriage-return */
	case '\\':
	    fprintf (fp, "\\\\");
	    break;		/* backslash */
	case '^':
	    fprintf (fp, "\\^");
	    break;		/* caret */
	case 127:
	    fprintf (fp, "^?");
	default:
	    if (ch <= 31)
		fprintf (fp, "^%c", ('@' + ch));
	    else if (ch > 127)
		fprintf (fp, "\\%o", ch);
	    else
		fprintf (fp, "%c", ch);
	    break;
	}
	len--;
    }
    fprintf (fp, "\n");
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menu_dump (rxvtlib *o, FILE * fp, menu_t * menu)
{E_
    menuitem_t     *item;

/* create a new menu and clear it */
    fprintf (fp, (menu->parent ? "./%s/*\n" : "/%s/*\n"), menu->name);

    for (item = menu->head; item != NULL; item = item->next) {
	switch (item->entry.type) {
	case MenuSubMenu:
	    if (item->entry.submenu.menu == NULL)
		fprintf (fp, "> %s == NULL\n", item->name);
	    else
		rxvtlib_menu_dump (o, fp, item->entry.submenu.menu);
	    break;

	case MenuLabel:
	    fprintf (fp, "{%s}\n", (strlen (item->name) ? item->name : "-"));
	    break;

	case MenuTerminalAction:
	case MenuAction:
	    fprintf (fp, "{%s}", item->name);
	    if (item->name2 != NULL && strlen (item->name2))
		fprintf (fp, "{%s}", item->name2);
	    fprintf (fp, "\t");
	    action_decode (fp, &(item->entry.action));
	    break;
	}
    }

    fprintf (fp, (menu->parent ? "../\n" : "/\n\n"));
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menubar_dump (rxvtlib *o, FILE * fp)
{E_
    bar_t          *bar = o->CurrentBar;
    time_t          t;

    if (bar == NULL || fp == NULL)
	return;
    time (&t);

    fprintf (fp,
	     "# " APL_SUBCLASS " (%s)  Pid: %u\n# Date: %s\n\n",
	     o->rs[Rs_name], (unsigned int)getpid (), ctime (&t));

/* dump in reverse order */
    bar = o->CurrentBar->prev;
    do {
	menu_t         *menu;
	int             i;

	fprintf (fp, "[menu:%s]\n", bar->name);

	if (bar->title != NULL)
	    fprintf (fp, "[title:%s]\n", bar->title);

	for (i = 0; i < NARROWS; i++) {
	    switch (bar->arrows[i].type) {
	    case MenuTerminalAction:
	    case MenuAction:
		fprintf (fp, "<%c>", o->Arrows[i].name);
		action_decode (fp, &(bar->arrows[i]));
		break;
	    }
	}
	fprintf (fp, "\n");

	for (menu = bar->head; menu != NULL; menu = menu->next)
	    rxvtlib_menu_dump (o, fp, menu);

	fprintf (fp, "\n[done:%s]\n\n", bar->name);
	bar = bar->prev;
    }
    while (bar != o->CurrentBar->prev);
}
#endif
#endif				/* (MENUBAR_MAX > 1) */

/*
 * read in menubar commands from FILENAME
 * ignore all input before the tag line [menu] or [menu:???]
 *
 * Note that since File_find () is used, FILENAME can be semi-colon
 * delimited such that the second part can refer to a tag
 * so that a large `database' of menus can be collected together
 *
 * FILENAME = "file"
 * FILENAME = "file;"
 *      read `file' starting with first [menu] or [menu:???] line
 *
 * FILENAME = "file;tag"
 *      read `file' starting with [menu:tag]
 */
/* EXTPROTO */
void            rxvtlib_menubar_read (rxvtlib *o, const char *filename)
{E_
#ifdef MENUBAR
/* read in a menu from a file */
    FILE           *fp;
    char            buffer[256];
    char           *p, *file, *tag = NULL;

    file = (char *)rxvtlib_File_find (o, filename, ".menu");
    if (file == NULL)
	return;
    fp = fopen (file, "rb");
    FREE (file);
    if (fp == NULL)
	return;

#if (MENUBAR_MAX > 1)
/* semi-colon delimited */
    if ((tag = strchr (filename, ';')) != NULL) {
	tag++;
	if (*tag == '\0')
	    tag = NULL;
    }
#endif				/* (MENUBAR_MAX > 1) */
#ifdef DEBUG_MENU
    fprintf (stderr, "[read:%s]\n", p);
    if (tag)
	fprintf (stderr, "looking for [menu:%s]\n", tag);
#endif

    while ((p = fgets (buffer, sizeof (buffer), fp)) != NULL) {
	int             n;

	if ((n = Str_match (p, "[menu")) != 0) {
	    if (tag) {
		/* looking for [menu:tag] */
		if (p[n] == ':' && p[n + 1] != ']') {
		    n++;
		    n += Str_match (p + n, tag);
		    if (p[n] == ']') {
#ifdef DEBUG_MENU
			fprintf (stderr, "[menu:%s]\n", tag);
#endif
			break;
		    }
		}
	    } else if (p[n] == ':' || p[n] == ']')
		break;
	}
    }

/* found [menu], [menu:???] tag */
    while (p != NULL) {
	int             n;

#ifdef DEBUG_MENU
	fprintf (stderr, "read line = %s\n", p);
#endif

	/* looking for [done:tag] or [done:] */
	if ((n = Str_match (p, "[done")) != 0) {
	    if (p[n] == ']') {
		o->menu_readonly = 1;
		break;
	    } else if (p[n] == ':') {
		n++;
		if (p[n] == ']') {
		    o->menu_readonly = 1;
		    break;
		} else if (tag) {
		    n += Str_match (p + n, tag);
		    if (p[n] == ']') {
#ifdef DEBUG_MENU
			fprintf (stderr, "[done:%s]\n", tag);
#endif
			o->menu_readonly = 1;
			break;
		    }
		} else {
		    /* what? ... skip this line */
		    p[0] = COMMENT_CHAR;
		}
	    }
	}
	/*
	 * remove leading/trailing space
	 * and strip-off leading/trailing quotes
	 * skip blank or comment lines
	 */
	(void)Str_trim (p);
	if (*p && *p != '#') {
	    o->menu_readonly = 0;	/* if case we read another file */
	    rxvtlib_menubar_dispatch (o, p);
	}
	/* get another line */
	p = fgets (buffer, sizeof (buffer), fp);
    }

    fclose (fp);
#endif
}

/*
 * user interface for building/deleting and otherwise managing menus
 */
/* EXTPROTO */
void            rxvtlib_menubar_dispatch (rxvtlib *o, char *str)
{E_
#ifdef MENUBAR
    int             n, cmd;
    char           *path, *name, *name2;

    if (menubar_visible () && o->ActiveMenu != NULL)
	rxvtlib_menubar_expose (o);
    else
	o->ActiveMenu = NULL;

    cmd = *str;
    switch (cmd) {
    case '.':
    case '/':			/* absolute & relative path */
    case MENUITEM_BEG:		/* menuitem */
	/* add `+' prefix for these cases */
	cmd = '+';
	break;

    case '+':
    case '-':
	str++;			/* skip cmd character */
	break;

    case '<':
#if (MENUBAR_MAX > 1)
	if (o->CurrentBar == NULL)
	    break;
#endif				/* (MENUBAR_MAX > 1) */
	if (str[1] && str[2] == '>')	/* arrow commands */
	    rxvtlib_menuarrow_add (o, str);
	break;

    case '[':			/* extended command */
	while (str[0] == '[') {
	    char           *next = (++str);	/* skip leading '[' */

	    if (str[0] == ':') {	/* [:command:] */
		do {
		    next++;
		    if ((next = strchr (next, ':')) == NULL)
			return;	/* parse error */
		}
		while (next[1] != ']');
		/* remove and skip ':]' */
		*next = '\0';
		next += 2;
	    } else {
		if ((next = strchr (next, ']')) == NULL)
		    return;	/* parse error */
		/* remove and skip ']' */
		*next = '\0';
		next++;
	    }

	    if (str[0] == ':') {
		int             saved;

		/* try and dispatch it, regardless of read/write status */
		saved = o->menu_readonly;
		o->menu_readonly = 0;
		rxvtlib_menubar_dispatch (o, str + 1);
		o->menu_readonly = saved;
	    }
	    /* these ones don't require menu stacking */
	    else if (!strcmp (str, "clear")) {
		rxvtlib_menubar_clear (o);
	    } else if (!strcmp (str, "done") || Str_match (str, "done:")) {
		o->menu_readonly = 1;
	    } else if (!strcmp (str, "show")) {
		rxvtlib_map_menuBar (o, 1);
		o->menu_readonly = 1;
	    } else if (!strcmp (str, "hide")) {
		rxvtlib_map_menuBar (o, 0);
		o->menu_readonly = 1;
	    } else if ((n = Str_match (str, "read:")) != 0) {
		/* read in a menu from a file */
		str += n;
		rxvtlib_menubar_read (o, str);
	    } else if ((n = Str_match (str, "title:")) != 0) {
		str += n;
		if (o->CurrentBar != NULL && !o->menu_readonly) {
		    if (*str) {
			name = REALLOC (o->CurrentBar->title, strlen (str) + 1);
			if (name != NULL) {
			    STRCPY (name, str);
			    o->CurrentBar->title = name;
			}
			rxvtlib_menubar_expose (o);
		    } else {
			FREE (o->CurrentBar->title);
			o->CurrentBar->title = NULL;
		    }
		}
	    } else if ((n = Str_match (str, "pixmap:")) != 0) {
		str += n;
		rxvtlib_xterm_seq (o, XTerm_Pixmap, str);
	    }
#if (MENUBAR_MAX > 1)
	    else if ((n = Str_match (str, "rm")) != 0) {
		str += n;
		switch (str[0]) {
		case ':':
		    str++;
		    rxvtlib_menubar_remove (o, str);
		    break;

		case '\0':
		    rxvtlib_menubar_remove (o, str);
		    break;

		case '*':
		    rxvtlib_menubar_remove (o, str);
		    break;
		}
		o->menu_readonly = 1;
	    } else if ((n = Str_match (str, "menu")) != 0) {
		str += n;
		switch (str[0]) {
		case ':':
		    str++;
		    /* add/access menuBar */
		    if (*str != '\0' && *str != '*')
			rxvtlib_menubar_push (o, str);
		    break;
		default:
		    if (o->CurrentBar == NULL) {
			rxvtlib_menubar_push (o, "default");
		    }
		}

		if (o->CurrentBar != NULL)
		    o->menu_readonly = 0;	/* allow menu build commands */
	    } else if (!strcmp (str, "dump")) {
		/* dump current menubars to a file */
		FILE           *fp;

		/* enough space to hold the results */
		char            buffer[32];

		sprintf (buffer, "/tmp/" APL_SUBCLASS "-%u",
			 (unsigned int)getpid ());

		if ((fp = fopen (buffer, "wb")) != NULL) {
		    rxvtlib_xterm_seq (o, XTerm_title, buffer);
		    rxvtlib_menubar_dump (o, fp);
		    fclose (fp);
		}
	    } else if (!strcmp (str, "next")) {
		if (o->CurrentBar) {
		    o->CurrentBar = o->CurrentBar->next;
		    o->menu_readonly = 1;
		}
	    } else if (!strcmp (str, "prev")) {
		if (o->CurrentBar) {
		    o->CurrentBar = o->CurrentBar->prev;
		    o->menu_readonly = 1;
		}
	    } else if (!strcmp (str, "swap")) {
		/* swap the top 2 menus */
		if (o->CurrentBar) {
		    bar_t          *prev = o->CurrentBar->prev;
		    bar_t          *next = o->CurrentBar->next;

		    prev->next = next;
		    next->prev = prev;

		    o->CurrentBar->next = prev;
		    o->CurrentBar->prev = prev->prev;

		    prev->prev->next = o->CurrentBar;
		    prev->prev = o->CurrentBar;

		    o->CurrentBar = prev;
		    o->menu_readonly = 1;
		}
	    }
#endif				/* (MENUBAR_MAX > 1) */
	    str = next;

	    o->BuildMenu = o->ActiveMenu = NULL;
	    rxvtlib_menubar_expose (o);
#ifdef DEBUG_MENUBAR_STACKING
	    fprintf (stderr, "menus are read%s\n",
		     o->menu_readonly ? "only" : "/write");
#endif
	}
	return;
	break;
    }

#if (MENUBAR_MAX > 1)
    if (o->CurrentBar == NULL)
	return;
    if (o->menu_readonly) {
#ifdef DEBUG_MENUBAR_STACKING
	fprintf (stderr, "menus are read%s\n",
		 o->menu_readonly ? "only" : "/write");
#endif
	return;
    }
#endif				/* (MENUBAR_MAX > 1) */

    switch (cmd) {
    case '+':
    case '-':
	path = name = str;

	name2 = NULL;
	/* parse STR, allow spaces inside (name)  */
	if (path[0] != '\0') {
	    name = strchr (path, MENUITEM_BEG);
	    str = strchr (path, MENUITEM_END);
	    if (name != NULL || str != NULL) {
		if (name == NULL || str == NULL || str <= (name + 1)
		    || (name > path && name[-1] != '/')) {
		    print_error ("menu error <%s>\n", path);
		    break;
		}
		if (str[1] == MENUITEM_BEG) {
		    name2 = (str + 2);
		    str = strchr (name2, MENUITEM_END);

		    if (str == NULL) {
			print_error ("menu error <%s>\n", path);
			break;
		    }
		    name2[-2] = '\0';	/* remove prev MENUITEM_END */
		}
		if (name > path && name[-1] == '/')
		    name[-1] = '\0';

		*name++ = '\0';	/* delimit */
		*str++ = '\0';	/* delimit */

		while (isspace (*str))
		    str++;	/* skip space */
	    }
#ifdef DEBUG_MENU
	    fprintf (stderr,
		     "`%c' path = <%s>, name = <%s>, name2 = <%s>, action = <%s>\n",
		     cmd, (path ? path : "(nil)"), (name ? name : "(nil)"),
		     (name2 ? name2 : "(nil)"), (str ? str : "(nil)")
		);
#endif
	}
	/* process the different commands */
	switch (cmd) {
	case '+':		/* add/replace existing menu or menuitem */
	    if (path[0] != '\0') {
		int             len;

		path = rxvtlib_menu_find_base (o, &o->BuildMenu, path);
		len = strlen (path);

		/* don't allow menus called `*' */
		if (path[0] == '*') {
		    rxvtlib_menu_clear (o, o->BuildMenu);
		    break;
		} else if (len >= 2 && !strcmp ((path + len - 2), "/*")) {
		    path[len - 2] = '\0';
		}
		if (path[0] != '\0')
		    o->BuildMenu = rxvtlib_menu_add (o, o->BuildMenu, path);
	    }
	    if (name != NULL && name[0] != '\0')
		rxvtlib_menuitem_add (o, o->BuildMenu,
			      (strcmp (name, SEPARATOR_NAME) ? name : ""),
			      name2, str);
	    break;

	case '-':		/* delete menu entry */
	    if (!strcmp (path, "/*") && (name == NULL || name[0] == '\0')) {
		rxvtlib_menubar_clear (o);
		o->BuildMenu = NULL;
		rxvtlib_menubar_expose (o);
		break;
	    } else if (path[0] != '\0') {
		int             len;
		menu_t         *menu = o->BuildMenu;

		path = rxvtlib_menu_find_base (o, &menu, path);
		len = strlen (path);

		/* submenu called `*' clears all menu items */
		if (path[0] == '*') {
		    rxvtlib_menu_clear (o, menu);
		    break;	/* done */
		} else if (len >= 2 && !strcmp (&path[len - 2], "/*")) {
		    /* done */
		    break;
		} else if (path[0] != '\0') {
		    o->BuildMenu = NULL;
		    break;
		} else {
		    o->BuildMenu = menu;
		}
	    }
	    if (o->BuildMenu != NULL) {
		if (name == NULL || name[0] == '\0') {
		    o->BuildMenu = rxvtlib_menu_delete (o, o->BuildMenu);
		} else {
		    menuitem_t     *item;

		    item = menuitem_find (o->BuildMenu,
					  strcmp (name, SEPARATOR_NAME) ? name
					  : "");

		    if (item != NULL && item->entry.type != MenuSubMenu) {
			rxvtlib_menuitem_free (o, o->BuildMenu, item);

			/* fix up the width */
			o->BuildMenu->width = 0;
			for (item = o->BuildMenu->head;
			     item != NULL; item = item->next) {
			    if (o->BuildMenu->width < (item->len + item->len2))
				o->BuildMenu->width = (item->len + item->len2);
			}
		    }
		}
		rxvtlib_menubar_expose (o);
	    }
	    break;
	}
	break;
    }
#endif
}

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_draw_Arrows (rxvtlib *o, int name, int state)
{E_
    GC              top, bot;

    int             i;

#ifdef MENU_SHADOW_IN
    state = -state;
#endif
    switch (state) {
    case +1:
	top = o->topShadowGC;
	bot = o->botShadowGC;
	break;			/* SHADOW_OUT */
    case -1:
	top = o->botShadowGC;
	bot = o->topShadowGC;
	break;			/* SHADOW_IN */
    default:
	top = bot = o->neutralGC;
	break;			/* neutral */
    }

    if (!o->Arrows_x)
	return;

    for (i = 0; i < NARROWS; i++) {
	const int       w = Width2Pixel (1);
	const int       y = (menuBar_TotalHeight () - w) / 2;
	int             x = o->Arrows_x + (5 * Width2Pixel (i)) / 4;

	if (!name || name == o->Arrows[i].name)
	    rxvtlib_Draw_Triangle (o, o->menuBar.win, top, bot, x, y, w, o->Arrows[i].name);
    }
    XFlush (o->Xdisplay);
}
#endif

/* EXTPROTO */
void            rxvtlib_menubar_expose (rxvtlib *o)
{E_
#ifdef MENUBAR
    menu_t         *menu;
    int             x;

    if (!menubar_visible () || o->menuBar.win == 0)
	return;

    if (o->menubarGC == None) {
	/* Create the graphics context */
	XGCValues       gcvalue;

	gcvalue.font = o->TermWin.font->fid;

	gcvalue.foreground = (o->Xdepth <= 2 ?
			      o->PixColors[Color_fg] : o->PixColors[Color_Black]);
	o->menubarGC = XCreateGC (o->Xdisplay, o->menuBar.win,
			       GCForeground | GCFont, &gcvalue);

	gcvalue.foreground = o->PixColors[Color_scroll];
	o->neutralGC = XCreateGC (o->Xdisplay, o->menuBar.win, GCForeground, &gcvalue);

	gcvalue.foreground = o->PixColors[Color_bottomShadow];
	o->botShadowGC = XCreateGC (o->Xdisplay, o->menuBar.win,
				 GCForeground | GCFont, &gcvalue);

	gcvalue.foreground = o->PixColors[Color_topShadow];
	o->topShadowGC =
	    XCreateGC (o->Xdisplay, o->menuBar.win, GCForeground, &gcvalue);
    }
/* make sure the font is correct */
    XSetFont (o->Xdisplay, o->menubarGC, o->TermWin.font->fid);
    XSetFont (o->Xdisplay, o->botShadowGC, o->TermWin.font->fid);
    XClearWindow (o->Xdisplay, o->menuBar.win);

    rxvtlib_menu_hide_all (o);

    x = 0;
    if (o->CurrentBar != NULL) {
	for (menu = o->CurrentBar->head; menu != NULL; menu = menu->next) {
	    int             len = menu->len;

	    x = (menu->x + menu->len + HSPACE);

#ifdef DEBUG_MENU_LAYOUT
	    rxvtlib_print_menu_descendants (o, menu);
#endif

	    if (x >= o->TermWin.ncol)
		len = (o->TermWin.ncol - (menu->x + HSPACE));

	    rxvtlib_drawbox_menubar (o, menu->x, len, +1);

	    XDrawString (o->Xdisplay,
			 o->menuBar.win, o->menubarGC,
			 (Width2Pixel (menu->x) + Width2Pixel (HSPACE) / 2),
			 menuBar_height () - SHADOW, menu->name, len);

	    if (x >= o->TermWin.ncol)
		break;
	}
    }
    rxvtlib_drawbox_menubar (o, x, o->TermWin.ncol, (o->CurrentBar ? +1 : -1));

/* add the menuBar title, if it exists and there's plenty of room */
    o->Arrows_x = 0;
    if (x < o->TermWin.ncol) {
	const char     *str;
	int             len, ncol = o->TermWin.ncol;
	char            title[256];

	if (x < (ncol - (NARROWS + 1))) {
	    ncol -= (NARROWS + 1);
	    o->Arrows_x = Width2Pixel (ncol);
	}
	rxvtlib_draw_Arrows (o, 0, +1);

	str = (o->CurrentBar && o->CurrentBar->title) ? o->CurrentBar->title : "%n-%v";
	for (len = 0; str[0] && len < sizeof (title) - 1; str++) {
	    const char     *s = NULL;

	    switch (str[0]) {
	    case '%':
		str++;
		switch (str[0]) {
		case 'n':
		    s = o->rs[Rs_name];
		    break;	/* resource name */
		case 'v':
		    s = VERSION;
		    break;	/* version number */
		case '%':
		    s = "%";
		    break;	/* literal '%' */
		}
		if (s != NULL)
		    while (*s && len < sizeof (title) - 1)
			title[len++] = *s++;
		break;

	    default:
		title[len++] = str[0];
		break;
	    }
	}
	title[len] = '\0';

	ncol -= (x + len + HSPACE);
	if (len > 0 && ncol >= 0)
	    XDrawString (o->Xdisplay,
			 o->menuBar.win, o->menubarGC,
			 Width2Pixel (x) + Width2Pixel (ncol + HSPACE) / 2,
			 menuBar_height () - SHADOW, title, len);
    }
#endif
}

/* EXTPROTO */
int             rxvtlib_menubar_mapping (rxvtlib *o, int map)
{E_
#ifdef MENUBAR
    int             change = 0;

    if (map && !menubar_visible ()) {
	o->menuBar.state = 1;
	if (o->menuBar.win == 0)
	    return 0;
	XMapWindow (o->Xdisplay, o->menuBar.win);
	change = 1;
    } else if (!map && menubar_visible ()) {
	rxvtlib_menubar_expose (o);
	o->menuBar.state = 0;
	XUnmapWindow (o->Xdisplay, o->menuBar.win);
	change = 1;
    } else
	rxvtlib_menubar_expose (o);

    return change;
#else
    return 0;
#endif
}

#ifdef MENUBAR
/* INTPROTO */
int             rxvtlib_menu_select (rxvtlib *o, XButtonEvent * ev)
{E_
    menuitem_t     *thisitem, *item = NULL;
    int             this_y, y;

    Window          unused_root, unused_child;
    int             unused_root_x, unused_root_y;
    unsigned int    unused_mask;

    if (o->ActiveMenu == NULL)
	return 0;

    XQueryPointer (o->Xdisplay, o->ActiveMenu->win,
		   &unused_root, &unused_child,
		   &unused_root_x, &unused_root_y,
		   &(ev->x), &(ev->y), &unused_mask);

    if (o->ActiveMenu->parent != NULL && (ev->x < 0 || ev->y < 0)) {
	rxvtlib_menu_hide (o);
	return 1;
    }
/* determine the menu item corresponding to the Y index */
    y = SHADOW;
    if (ev->x >= 0 && ev->x <= (o->ActiveMenu->w - SHADOW)) {
	for (item = o->ActiveMenu->head; item != NULL; item = item->next) {
	    int             h = HEIGHT_TEXT + 2 * SHADOW;

	    if (isSeparator (item->name))
		h = HEIGHT_SEPARATOR;
	    else if (ev->y >= y && ev->y < (y + h))
		break;
	    y += h;
	}
    }
    if (item == NULL && ev->type == ButtonRelease) {
	rxvtlib_menu_hide_all (o);
	return 0;
    }
    thisitem = item;
    this_y = y - SHADOW;

/* erase the last item */
    if (o->ActiveMenu->item != NULL) {
	if (o->ActiveMenu->item != thisitem) {
	    for (y = 0, item = o->ActiveMenu->head;
		 item != NULL; item = item->next) {
		int             h;

		if (isSeparator (item->name))
		    h = HEIGHT_SEPARATOR;
		else if (item == o->ActiveMenu->item) {
		    /* erase old menuitem */
		    rxvtlib_drawbox_menuitem (o, y, 0);	/* No Shadow */
		    if (item->entry.type == MenuSubMenu)
			rxvtlib_drawtriangle (o, o->ActiveMenu->w, y, +1);
		    break;
		} else
		    h = HEIGHT_TEXT + 2 * SHADOW;
		y += h;
	    }
	} else {
	    switch (ev->type) {
	    case ButtonRelease:
		switch (item->entry.type) {
		case MenuLabel:
		case MenuSubMenu:
		    rxvtlib_menu_hide_all (o);
		    break;

		case MenuAction:
		case MenuTerminalAction:
		    rxvtlib_drawbox_menuitem (o, this_y, -1);
		    /*
		     * use select for timing
		     * remove menu before sending keys to the application
		     */
		    {
			struct timeval  tv;

			tv.tv_sec = 0;
			tv.tv_usec = MENU_DELAY_USEC;
			select (0, NULL, NULL, NULL, &tv);
		    }
		    rxvtlib_menu_hide_all (o);
#ifndef DEBUG_MENU
		    rxvtlib_action_dispatch (o, &(item->entry.action));
#else				/* DEBUG_MENU */
		    fprintf (stderr, "%s: %s\n", item->name,
			     item->entry.action.str);
#endif				/* DEBUG_MENU */
		    break;
		}
		break;

	    default:
		if (item->entry.type == MenuSubMenu)
		    goto DoMenu;
		break;
	    }
	    return 0;
	}
    }
  DoMenu:
    o->ActiveMenu->item = thisitem;
    y = this_y;
    if (thisitem != NULL) {
	item = o->ActiveMenu->item;
	if (item->entry.type != MenuLabel)
	    rxvtlib_drawbox_menuitem (o, y, +1);
	if (item->entry.type == MenuSubMenu) {
	    int             x;

	    rxvtlib_drawtriangle (o, o->ActiveMenu->w, y, -1);

	    x = ev->x + (o->ActiveMenu->parent ?
			 o->ActiveMenu->x : Width2Pixel (o->ActiveMenu->x));

	    if (x >= item->entry.submenu.menu->x) {
		o->ActiveMenu = item->entry.submenu.menu;
		rxvtlib_menu_show (o);
		return 1;
	    }
	}
    }
    return 0;
}
#endif

#ifdef MENUBAR
/* INTPROTO */
void            rxvtlib_menubar_select (rxvtlib *o, XButtonEvent * ev)
{E_
    menu_t         *menu = NULL;

/* determine the pulldown menu corresponding to the X index */
    if (ev->y >= 0 && ev->y <= menuBar_height () && o->CurrentBar != NULL) {
	for (menu = o->CurrentBar->head; menu != NULL; menu = menu->next) {
	    int             x = Width2Pixel (menu->x);
	    int             w = Width2Pixel (menu->len + HSPACE);

	    if ((ev->x >= x && ev->x < x + w))
		break;
	}
    }
    switch (ev->type) {
    case ButtonRelease:
	rxvtlib_menu_hide_all (o);
	break;

    case ButtonPress:
	if (menu == NULL && o->Arrows_x && ev->x >= o->Arrows_x) {
	    int             i;

	    for (i = 0; i < NARROWS; i++) {
		if (ev->x >= (o->Arrows_x + (Width2Pixel (4 * i + i)) / 4)
		    && ev->x < (o->Arrows_x + (Width2Pixel (4 * i + i + 4)) / 4)) {
		    rxvtlib_draw_Arrows (o, o->Arrows[i].name, -1);
		    /*
		     * use select for timing
		     */
		    {
			struct timeval  tv;

			tv.tv_sec = 0;
			tv.tv_usec = MENU_DELAY_USEC;
			select (0, NULL, NULL, NULL, &tv);
		    }
		    rxvtlib_draw_Arrows (o, o->Arrows[i].name, +1);
#ifdef DEBUG_MENUARROWS
		    fprintf (stderr, "'%c': ", o->Arrows[i].name);

		    if (o->CurrentBar == NULL
			|| (o->CurrentBar->arrows[i].type != MenuAction
			    && o->CurrentBar->arrows[i].type !=
			    MenuTerminalAction)) {
			if (o->Arrows[i].str != NULL && o->Arrows[i].str[0])
			    fprintf (stderr, "(default) \\033%s\n",
				     &(o->Arrows[i].str[2]));
		    } else {
			fprintf (stderr, "%s\n", o->CurrentBar->arrows[i].str);
		    }
#else				/* DEBUG_MENUARROWS */
		    if (o->CurrentBar == NULL
			|| rxvtlib_action_dispatch (o, &(o->CurrentBar->arrows[i]))) {
			if (o->Arrows[i].str != NULL && o->Arrows[i].str[0] != 0)
			    rxvtlib_tt_write (o, o->Arrows[i].str + 1, o->Arrows[i].str[0]);
		    }
#endif				/* DEBUG_MENUARROWS */
		    return;
		}
	    }
	}
	/* FALLTHROUGH */

    default:
	/*
	 * press menubar or move to a new entry
	 */
	if (menu != NULL && menu != o->ActiveMenu) {
	    rxvtlib_menu_hide_all (o);	/* pop down old menu */
	    o->ActiveMenu = menu;
	    rxvtlib_menu_show (o);	/* pop up new menu */
	}
	break;
    }
}
#endif

/*
 * general dispatch routine,
 * it would be nice to have `sticky' menus
 */
/* EXTPROTO */
void            rxvtlib_menubar_control (rxvtlib *o, XButtonEvent * ev)
{E_
#ifdef MENUBAR
    switch (ev->type) {
    case ButtonPress:
	if (ev->button == Button1)
	    rxvtlib_menubar_select (o, ev);
	break;

    case ButtonRelease:
	if (ev->button == Button1)
	    rxvtlib_menu_select (o, ev);
	break;

    case MotionNotify:
	while (XCheckTypedWindowEvent (o->Xdisplay, o->TermWin.parent[0],
				       MotionNotify, (XEvent *) ev)) ;

	if (o->ActiveMenu)
	    while (rxvtlib_menu_select (o, ev)) ;
	else
	    ev->y = -1;
	if (ev->y < 0) {
	    Window          unused_root, unused_child;
	    int             unused_root_x, unused_root_y;
	    unsigned int    unused_mask;

	    XQueryPointer (o->Xdisplay, o->menuBar.win,
			   &unused_root, &unused_child,
			   &unused_root_x, &unused_root_y,
			   &(ev->x), &(ev->y), &unused_mask);
	    rxvtlib_menubar_select (o, ev);
	}
	break;
    }
#endif
}

/* EXTPROTO */
void            rxvtlib_map_menuBar (rxvtlib *o, int map)
{E_
#ifdef MENUBAR
    if (rxvtlib_menubar_mapping (o, map))
	rxvtlib_resize_all_windows (o);
#endif
}

/* EXTPROTO */
void            rxvtlib_create_menuBar (rxvtlib *o, Cursor cursor)
{E_
#ifdef MENUBAR
/* menuBar: size doesn't matter */
    o->menuBar.win = XCreateSimpleWindow (o->Xdisplay, o->TermWin.parent[0],
				       0, 0,
				       1, 1,
				       0,
				       o->PixColors[Color_fg],
				       o->PixColors[Color_scroll]);
    XDefineCursor (o->Xdisplay, o->menuBar.win, cursor);
    XSelectInput (o->Xdisplay, o->menuBar.win,
		  (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		   Button1MotionMask));

#endif
}

/* EXTPROTO */
void            rxvtlib_Resize_menuBar (rxvtlib *o, int x, int y, unsigned int width,
				unsigned int height)
{E_
#ifdef MENUBAR
    XMoveResizeWindow (o->Xdisplay, o->menuBar.win, x, y, width, height);
#endif
}

/*----------------------- end-of-file (C source) -----------------------*/

/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"
#include "stringtools.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	xdefaults.c
 *----------------------------------------------------------------------*
 * $Id: xdefaults.c,v 1.34.2.3 1999/07/28 20:22:39 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 *				- original version
 * Copyright (C) 1997,1998 mj olesen <olesen@me.queensu.ca>
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
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 * get resources from ~/.Xdefaults or ~/.Xresources with the memory-saving
 * default or with XGetDefault() (#define USE_XGETDEFAULT)
 *
 * Coding style:
 *	resource strings are indicated by an `Rs_' prefix followed by
 *	the resource name.
 *	eg, `Rs_saveLines' is the resource string corresponding to
 *	    the `saveLines' resource
 *----------------------------------------------------------------------*/

/* #define DEBUG_RESOURCES */

/*{{{ monolithic option/resource structure: */
/*
 * `string' options MUST have a usage argument
 * `switch' and `boolean' options have no argument
 * if there's no desc(ription), it won't appear in usage()
 */

/* INFO() - descriptive information only */

/* STRG() - command-line option, with/without resource */

/* RSTRG() - resource/long-option */

/* BOOL() - regular boolean `-/+' flag */

/* SWCH() - `-' flag */

/* convenient macros */

/* Option flag *//* data pointer *//* keyword *//* option *//* argument *//* description */

/* short form *//* short form *//* short form */
/* fonts: command-line option = resource name */
#ifdef MULTICHAR_SET
/* fonts: command-line option = resource name */
#endif				/* MULTICHAR_SET */
/* short form */
#ifndef NO_CURSORCOLOR
/* command-line option = resource name */
#endif				/* NO_CURSORCOLOR */

/*}}} */

#define optList_strlen(i)						\
    (optList[i].flag ? 0 : (optList[i].arg ? strlen (optList[i].arg) : 1))
#define optList_isBool(i)						\
    (optList[i].flag & Opt_Boolean)
#define optList_isReverse(i)						\
    (optList[i].flag & Opt_Reverse)
#define optList_size()							\
    (sizeof(optList) / sizeof(optList[0]))

struct _optList {
    const unsigned long flag;	
    int dp;		
    const char     *kw;		
    const char     *opt;	
    const char     *arg;	
    const char     *desc;	
};

static struct _optList optList[] = {
    STRG (Rs_display_name, NULL, "d", NULL, NULL),	
    STRG (Rs_display_name, NULL, "display", "string", "X server to contact"),
    STRG (Rs_term_name, "termName", "tn", "string",
	  "value of the TERM environment variable"),
    STRG (Rs_geometry, NULL, "g", NULL, NULL),	
    STRG (Rs_geometry, "geometry", "geometry", "geometry",
	  "size (in characters) and position"),
    SWCH ("C", Opt_console, "intercept console messages"),
    SWCH ("iconic", Opt_iconic, "start iconic"),
    SWCH ("ic", Opt_iconic, NULL),	
    BOOL (Rs_reverseVideo, "reverseVideo", "rv", Opt_reverseVideo,
	  "reverse video"),
    BOOL (Rs_loginShell, "loginShell", "ls", Opt_loginShell, "login shell"),
    BOOL (Rs_scrollBar, "scrollBar", "sb", Opt_scrollBar, "scrollbar"),
    BOOL (Rs_scrollBar_right, "scrollBar_right", "sr", Opt_scrollBar_right,
	  "scrollbar right"),
    BOOL (Rs_scrollBar_floating, "scrollBar_floating", "st",
	  Opt_scrollBar_floating, "scrollbar without a trough"),
    BOOL (Rs_scrollTtyOutput, "scrollTtyOutput", NULL,
	  Opt_scrollTtyOutput, NULL),
    BOOL (Rs_scrollTtyOutput, NULL, "si", Opt_Reverse | Opt_scrollTtyOutput,
 	 "scroll-on-tty-output inhibit"),
    BOOL (Rs_scrollKeypress, "scrollTtyKeypress", "sk",
	  Opt_scrollKeypress, "scroll-on-keypress"),
#ifdef TRANSPARENT
    BOOL (Rs_transparent, "inheritPixmap", "ip",
	  Opt_transparent, "inherit parent pixmap"),
    SWCH ("tr", Opt_transparent, NULL),
#endif
    BOOL (Rs_utmpInhibit, "utmpInhibit", "ut", Opt_utmpInhibit,
	  "utmp inhibit"),
    BOOL (Rs_visualBell, "visualBell", "vb", Opt_visualBell, "visual bell"),
#if ! defined(NO_MAPALERT) && defined(MAPALERT_OPTION)
    BOOL (Rs_mapAlert, "mapAlert", NULL, Opt_mapAlert, NULL),
#endif
#ifdef META8_OPTION
    BOOL (Rs_meta8, "meta8", NULL, Opt_meta8, NULL),
#endif
    STRG (Rs_color + Color_bg, "background", "bg", "color",
	  "background color"),
    STRG (Rs_color + Color_fg, "foreground", "fg", "color",
	  "foreground color"),
    RSTRG (Rs_color + minCOLOR + 0, "color0", "color"),
    RSTRG (Rs_color + minCOLOR + 1, "color1", "color"),
    RSTRG (Rs_color + minCOLOR + 2, "color2", "color"),
    RSTRG (Rs_color + minCOLOR + 3, "color3", "color"),
    RSTRG (Rs_color + minCOLOR + 4, "color4", "color"),
    RSTRG (Rs_color + minCOLOR + 5, "color5", "color"),
    RSTRG (Rs_color + minCOLOR + 6, "color6", "color"),
    RSTRG (Rs_color + minCOLOR + 7, "color7", "color"),
#ifndef NO_BRIGHTCOLOR
    RSTRG (Rs_color + minBrightCOLOR + 0, "color8", "color"),
    RSTRG (Rs_color + minBrightCOLOR + 1, "color9", "color"),
    RSTRG (Rs_color + minBrightCOLOR + 2, "color10", "color"),
    RSTRG (Rs_color + minBrightCOLOR + 3, "color11", "color"),
    RSTRG (Rs_color + minBrightCOLOR + 4, "color12", "color"),
    RSTRG (Rs_color + minBrightCOLOR + 5, "color13", "color"),
    RSTRG (Rs_color + minBrightCOLOR + 6, "color14", "color"),
    RSTRG (Rs_color + minBrightCOLOR + 7, "color15", "color"),
#endif				/* NO_BRIGHTCOLOR */
#ifndef NO_BOLDUNDERLINE
    RSTRG (Rs_color + Color_BD, "colorBD", "color"),
    RSTRG (Rs_color + Color_UL, "colorUL", "color"),
#endif				/* NO_BOLDUNDERLINE */
#ifdef KEEP_SCROLLCOLOR
    RSTRG (Rs_color + Color_scroll, "scrollColor", "color"),
    RSTRG (Rs_color + Color_trough, "troughColor", "color"),
#endif				/* KEEP_SCROLLCOLOR */
#if defined (XPM_BACKGROUND) || (MENUBAR_MAX)
    RSTRG (Rs_path, "path", "search path"),
#endif				/* defined (XPM_BACKGROUND) || (MENUBAR_MAX) */
#ifdef XPM_BACKGROUND
    STRG (Rs_backgroundPixmap, "backgroundPixmap",
	  "pixmap", "file[;geom]", "background pixmap"),
#endif				/* XPM_BACKGROUND */
#if (MENUBAR_MAX)
    RSTRG (Rs_menu, "menu", "name[;tag]"),
#endif
#ifndef NO_BOLDFONT
    STRG (Rs_boldFont, "boldFont", "fb", "fontname", "bold text font"),
#endif
    STRG (Rs_font, "font", "fn", "fontname", "normal text font"),
#if NFONTS > 1
    RSTRG (Rs_font + 1, "font1", "fontname"),
#endif
#if NFONTS > 2
    RSTRG (Rs_font + 2, "font2", "fontname"),
#endif
#if NFONTS > 3
    RSTRG (Rs_font + 3, "font3", "fontname"),
#endif
#if NFONTS > 4
    RSTRG (Rs_font + 4, "font4", "fontname"),
#endif
#if NFONTS > 5
    RSTRG (Rs_font + 5, "font5", "fontname"),
#endif
#if NFONTS > 6
    RSTRG (Rs_font + 6, "font6", "fontname"),
#endif
#if NFONTS > 7
    RSTRG (Rs_font + 7, "font7", "fontname"),
#endif
#ifdef MULTICHAR_SET
    STRG (Rs_mfont, "mfont", "fm", "fontname", "multichar font"),
# if NFONTS > 1
    RSTRG (Rs_mfont + 1, "mfont1", "fontname"),
# endif
# if NFONTS > 2
    RSTRG (Rs_mfont + 2, "mfont2", "fontname"),
# endif
# if NFONTS > 3
    RSTRG (Rs_mfont + 3, "mfont3", "fontname"),
# endif
# if NFONTS > 4
    RSTRG (Rs_mfont + 4, "mfont4", "fontname"),
# endif
# if NFONTS > 5
    RSTRG (Rs_mfont + 5, "mfont5", "fontname"),
# endif
# if NFONTS > 6
    RSTRG (Rs_mfont + 6, "mfont6", "fontname"),
# endif
# if NFONTS > 7
    RSTRG (Rs_mfont + 7, "mfont7", "fontname"),
# endif
#endif				/* MULTICHAR_SET */
#ifdef MULTICHAR_SET
    STRG (Rs_multichar_encoding, "multichar_encoding", "km", "mode",
	  "multiple-character font encoding; mode = eucj | sjis | big5 | gb"),
#endif				/* MULTICHAR_SET */
#ifdef USE_XIM
    STRG (Rs_preeditType, "preeditType", "pt", "style",
	  "input style of input method; style = OverTheSpot | OffTheSpot | Root"),
    STRG (Rs_inputMethod, "inputMethod", "im", "name",
	  "name of input method"),
#endif				/* USE_XIM */
#ifdef GREEK_SUPPORT
    STRG (Rs_greek_keyboard, "greek_keyboard", "grk", "mode",
	  "greek keyboard mapping; mode = iso | ibm"),
#endif
    STRG (Rs_name, NULL, "name", "string",
	  "client instance, icon, and title strings"),
    STRG (Rs_title, "title", "title", "string", "title name for window"),
    STRG (Rs_title, NULL, "T", NULL, NULL),	
    STRG (Rs_iconName, "iconName", "n", "string", "icon name for window"),
#ifndef NO_CURSORCOLOR
    STRG (Rs_color + Color_cursor, "cursorColor", "cr", "color",
	  "cursor color"),
    RSTRG (Rs_color + Color_cursor2, "cursorColor2", "color"),
#endif				/* NO_CURSORCOLOR */
    STRG (Rs_color + Color_pointer, "pointerColor", "pr", "color",
	  "pointer color"),
    STRG (Rs_color + Color_border, "borderColor", "bd", "color",
	  "border color"),
    STRG (Rs_saveLines, "saveLines", "sl", "number",
	  "number of scrolled lines to save"),
#ifndef NO_BACKSPACE_KEY
    RSTRG (Rs_backspace_key, "backspacekey", "string"),
#endif
#ifndef NO_DELETE_KEY
    RSTRG (Rs_delete_key, "deletekey", "string"),
#endif
    RSTRG (Rs_selectstyle, "selectstyle", "string"),
#ifdef PRINTPIPE
    RSTRG (Rs_print_pipe, "print-pipe", "string"),
#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    RSTRG (Rs_bigfont_key, "bigfont_key", "keysym"),
    RSTRG (Rs_smallfont_key, "smallfont_key", "keysym"),
#endif
    STRG (Rs_modifier, "modifier", "mod", "modifier",
	  "alt | meta | hyper | super | mod1 | ... | mod5"),
#ifdef CUTCHAR_RESOURCE
    RSTRG (Rs_cutchars, "cutchars", "string"),
#endif				/* CUTCHAR_RESOURCE */
    INFO ("e", "command arg ...", "command to execute")
};


/*{{{ usage: */
/*----------------------------------------------------------------------*/
/* EXTPROTO */
void            rxvtlib_usage (rxvtlib *o, int type)
{E_
    int             i, col;

    fprintf (stderr, "\nUsage v%s : (", VERSION);
#ifdef XPM_BACKGROUND
# ifdef XPM_BUFFERING
    fprintf (stderr, "XPM-buffer,");
# else
    fprintf (stderr, "XPM,");
# endif
#endif
#ifdef UTMP_SUPPORT
    fprintf (stderr, "utmp,");
#endif
#ifdef MENUBAR
    fprintf (stderr, "menubar,");
#endif
#ifdef KANJI
    fprintf (stderr, "Kanji,");
#endif
#ifdef ZH
    fprintf (stderr, "Chinese,");
#endif
#ifdef ZHCN
    fprintf (stderr, "Chinese(GB),");
#endif
#ifdef XTERM_SCROLLBAR
    fprintf (stderr, "XTerm-scrollbar,");
#endif
#ifdef GREEK_SUPPORT
    fprintf (stderr, "Greek,");
#endif
#ifdef RXVT_GRAPHICS
    fprintf (stderr, "graphics,");
#endif
#ifdef NO_BACKSPACE_KEY
    fprintf (stderr, "no backspace,");
#endif
#ifdef NO_DELETE_KEY
    fprintf (stderr, "no delete,");
#endif
#ifdef NO_RESOURCES
    fprintf (stderr, "NoResources");
#else
# ifdef USE_XGETDEFAULT
    fprintf (stderr, "XGetDefaults");
# else
    fprintf (stderr, ".Xdefaults");
# endif
#endif

    fprintf (stderr, ")\n%s", APL_NAME);
    switch (type) {
    case 0:			/* brief listing */
	fprintf (stderr, " [-help]\n");
	col = 3;
	for (i = 0; i < optList_size (); i++) {
	    if (optList[i].desc != NULL) {
		int             len = 2;

		if (!optList_isBool (i)) {
		    len = optList_strlen (i);
		    if (len > 0)
			len++;	/* account for space */
		}
		len += 4 + strlen (optList[i].opt);

		col += len;
		if (col > 79) {	/* assume regular width */
		    fprintf (stderr, "\n");
		    col = 3 + len;
		}
		fprintf (stderr, " [-");
		if (optList_isBool (i))
		    fprintf (stderr, "/+");
		fprintf (stderr, "%s", optList[i].opt);
		if (optList_strlen (i))
		    fprintf (stderr, " %s]", optList[i].arg);
		else
		    fprintf (stderr, "]");
	    }
	}
	fprintf (stderr, "\n\n");
	break;

    case 1:			/* full command-line listing */
	fprintf (stderr,
		 " [options] [-e command args]\n\n"
		 "where options include:\n");

	for (i = 0; i < optList_size (); i++)
	    if (optList[i].desc != NULL)
		fprintf (stderr, "    %s%s %-*s%s%s\n",
			 (optList_isBool (i) ? "-/+" : "-"),
			 optList[i].opt, (int) (INDENT - strlen (optList[i].opt)
					  + (optList_isBool (i) ? 0 : 2)),
			 (optList[i].arg ? optList[i].arg : ""),
			 (optList_isBool (i) ? "turn on/off " : ""),
			 optList[i].desc);
	fprintf (stderr, "\n    --help to list long-options\n\n");
	break;

    case 2:			/* full resource listing */
	fprintf (stderr,
		 " [options] [-e command args]\n\n"
		 "where resources (long-options) include:\n");

	for (i = 0; i < optList_size (); i++)
	    if (optList[i].kw != NULL)
		fprintf (stderr, "    %s: %*s\n",
			 optList[i].kw,
			 (int) (INDENT - strlen (optList[i].kw)),
			 (optList_isBool (i) ? "boolean" : optList[i].arg));

#ifdef KEYSYM_RESOURCE
	fprintf (stderr, "    " "keysym.sym" ": %*s\n",
		 (int) (INDENT - strlen ("keysym.sym")), "keysym");
#endif
	fprintf (stderr, "\n    -help to list options\n\n");
	break;
    }
    o->killed = EXIT_FAILURE | DO_EXIT;
    /* NOTREACHED */
}
/*}}} */

/*{{{ get command-line options before getting resources */
/* EXTPROTO */
void            rxvtlib_get_options (rxvtlib *o, int argc, char *const *argv)
{E_
    int             i, bad_option = 0;
    static const char On[3] = "ON", Off[4] = "OFF";

    for (i = 1; i < argc; i++) {
	int             entry, longopt = 0;
	const char     *flag, *opt;

	opt = argv[i];
#ifdef DEBUG_RESOURCES
	fprintf (stderr, "argv[%d] = %s: ", i, opt);
#endif
	if (*opt == '-') {
	    flag = On;
	    if (*++opt == '-')
		longopt = *opt++;	/* long option */
	} else if (*opt == '+') {
	    flag = Off;
	    if (*++opt == '+')
		longopt = *opt++;	/* long option */
	} else {
	    bad_option = 1;
	    print_error ("bad option \"%s\"", opt);
	    continue;
	}

	if (!strcmp (opt, "help"))
	    rxvtlib_usage (o, longopt ? 2 : 1);
	if (o->killed)
	    return;
	if (!strcmp (opt, "h"))
	    rxvtlib_usage (o, 0);
	if (o->killed)
	    return;

	/* feature: always try to match long-options */
	for (entry = 0; entry < optList_size (); entry++)
	    if ((optList[entry].kw && !strcmp (opt, optList[entry].kw))
		|| (!longopt
		    && optList[entry].opt
		    && !strcmp (opt, optList[entry].opt))) break;

	if (entry < optList_size ()) {
	    if (optList_isReverse (entry))
		flag = flag == On ? Off : On;
	    if (optList_strlen (entry)) {	/* string value */
		const char     *str = argv[++i];

#ifdef DEBUG_RESOURCES
		fprintf (stderr, "string (%s,%s) = ",
			 optList[entry].opt ? optList[entry].opt : "nil",
			 optList[entry].kw ? optList[entry].kw : "nil");
#endif
		if (flag == On && str && optList[entry].dp != 1) { 
#ifdef DEBUG_RESOURCES
		    fprintf (stderr, "\"%s\"\n", str);
#endif
		    *(&(o->rs[optList[entry].dp])) = (char *) str;

		    /* special cases are handled in main.c:main() to allow
		     * X resources to set these values before we settle for
		     * default values
		     */
		}
#ifdef DEBUG_RESOURCES
		else
		    fprintf (stderr, "???\n");
#endif
	    } else {		/* boolean value */
#ifdef DEBUG_RESOURCES
		fprintf (stderr, "boolean (%s,%s) = %s\n",
			 optList[entry].opt, optList[entry].kw, flag);
#endif
		if (flag == On)
		    o->Options |= (optList[entry].flag);
		else
		    o->Options &= ~(optList[entry].flag);

		if (optList[entry].dp != 1)
		    *(&(o->rs[optList[entry].dp])) = (char *) flag;
	    }
	} else
#ifdef KEYSYM_RESOURCE
	    /* if (!strncmp (opt, "keysym.", strlen ("keysym."))) */
	if (Str_match (opt, "keysym.")) {
	    const char     *str = argv[++i];

	    /*
	     * '7' is strlen("keysym.")
	     */
	    if (str != NULL)
		rxvtlib_parse_keysym (o, opt + 7, str);
	} else
#endif
	{
	    /* various old-style options, just ignore
	     * Obsolete since about Jan 96,
	     * so they can probably eventually be removed
	     */
	    const char     *msg = "bad";

	    if (longopt) {
		opt--;
		bad_option = 1;
	    } else if (!strcmp (opt, "7") || !strcmp (opt, "8")
#ifdef GREEK_SUPPORT
		       /* obsolete 12 May 1996 (v2.17) */
		       || !Str_match (opt, "grk")
#endif
		)
		msg = "obsolete";
	    else
		bad_option = 1;

	    print_error ("%s option \"%s\"", msg, --opt);
	}
    }

    if (bad_option)
	rxvtlib_usage (o, 0);
}
/*}}} */

#ifndef NO_RESOURCES
/*----------------------------------------------------------------------*/

# ifdef KEYSYM_RESOURCE
/*
 * Define key from XrmEnumerateDatabase.
 *   quarks will be something like
 *      "rxvt" "keysym" "0xFF01"
 *   value will be a string
 */
/* ARGSUSED */
/* INTPROTO */
Bool            rxvtlib_define_key (rxvtlib *o, XrmDatabase * database, XrmBindingList bindings,
			    XrmQuarkList quarks, XrmRepresentation * type,
			    XrmValue * value, XPointer closure)
{E_
    int             last;

    for (last = 0; quarks[last] != NULLQUARK; last++)	/* look for last quark in list */
	;
    last--;
    rxvtlib_parse_keysym (o, XrmQuarkToString (quarks[last]), (char *)value->addr);
    return False;
}

/*
 * look for something like this (XK_Delete)
 * rxvt*keysym.0xFFFF: "\177"
 *
 * arg will be
 *      NULL for ~/.Xdefaults and
 *      non-NULL for command-line options (need to allocate)
 */
/* INTPROTO */
int             rxvtlib_parse_keysym (rxvtlib *o, const char *str, const char *arg)
{E_
    int             n, sym;
    char           *key_string, *newarg = NULL;
    char            newargstr[NEWARGLIM];

    if (arg == NULL) {
	if ((n = Str_match (str, "keysym.")) == 0)
	    return 0;
	str += n;		/* skip `keysym.' */
    }
/* some scanf() have trouble with a 0x prefix */
    if (isdigit (str[0])) {
	if (str[0] == '0' && toupper (str[1]) == 'X')
	    str += 2;
	if (arg) {
	    if (sscanf (str, (strchr (str, ':') ? "%x:" : "%x"), &sym) != 1)
		return -1;
	} else {
	    if (sscanf (str, "%x:", &sym) != 1)
		return -1;

	    /* cue to ':', it's there since sscanf() worked */
	    STRNCPY (newargstr, strchr (str, ':') + 1, NEWARGLIM - 1);
	    newargstr[NEWARGLIM - 1] = '\0';
	    newarg = Str_trim (newargstr);
	}
    } else {
	/*
	 * convert keysym name to keysym number
	 */
	STRNCPY (newargstr, str, NEWARGLIM - 1);
	newargstr[NEWARGLIM - 1] = '\0';
	if (arg == NULL) {
	    if ((newarg = strchr (newargstr, ':')) == NULL)
		return -1;
	    *newarg++ = '\0';	/* terminate keysym name */
	    (void)Str_trim (newarg);
	}
	if ((sym = XStringToKeysym (newargstr)) == None)
	    return -1;
    }

    if (sym < 0xFF00 || sym > 0xFFFF)	/* we only do extended keys */
	return -1;
    sym &= 0xFF;
    if (o->KeySym_map[sym] != NULL)	/* already set ? */
	return -1;

    if (newarg == NULL) {
	STRNCPY (newargstr, arg, NEWARGLIM - 1);
	newargstr[NEWARGLIM - 1] = '\0';
	newarg = newargstr;
    }
    if (strlen (newargstr) == 0 || (n = Str_escaped (newargstr)) == 0)
	return -1;
    MIN_IT (n, 255);
    key_string = MALLOC ((n + 1) * sizeof (char));

    key_string[0] = n;
    STRNCPY (key_string + 1, newarg, n);
    o->KeySym_map[sym] = (unsigned char *)key_string;

    return 1;
}
# endif				/* KEYSYM_RESOURCE */

# ifndef USE_XGETDEFAULT
/*{{{ get_xdefaults() */
/*
 * the matching algorithm used for memory-save fake resources
 */
/* INTPROTO */
void            rxvtlib_get_xdefaults (rxvtlib *o, FILE * stream, const char *name)
{E_
    unsigned int    len;
    char           *str, buffer[256];

    if (stream == NULL)
	return;
    len = strlen (name);
    while ((str = fgets (buffer, sizeof (buffer), stream)) != NULL) {
	unsigned int    entry, n;

	while (*str && isspace (*str))
	    str++;		/* leading whitespace */

	if ((str[len] != '*' && str[len] != '.')
	    || (len && strncmp (str, name, len)))
	    continue;
	str += (len + 1);	/* skip `name*' or `name.' */

# ifdef KEYSYM_RESOURCE
	if (!rxvtlib_parse_keysym (o, str, NULL))
# endif				/* KEYSYM_RESOURCE */
	    for (entry = 0; entry < optList_size (); entry++) {
		const char     *kw = optList[entry].kw;

		if (kw == NULL)
		    continue;
		n = strlen (kw);
		if (str[n] == ':' && Str_match (str, kw)) {
		    /* skip `keyword:' */
		    str += (n + 1);
		    (void)Str_trim (str);
		    n = strlen (str);
		    if (n && *(&(o->rs[optList[entry].dp])) == NULL) {
			/* not already set */
			int             s;
			char           *p = MALLOC ((n + 1) * sizeof (char));

			STRCPY (p, str);
			*(&(o->rs[optList[entry].dp])) = *(&(o->rs_free[optList[entry].dp])) = p;
			if (optList_isBool (entry)) {
			    s = Cstrcasecmp (p, "TRUE") == 0
				|| Cstrcasecmp (p, "YES") == 0
				|| Cstrcasecmp (p, "ON") == 0
				|| Cstrcasecmp (p, "1") == 0;
			    if (optList_isReverse (entry))
				s = !s;
			    if (s)
				o->Options |= (optList[entry].flag);
			    else
				o->Options &= ~(optList[entry].flag);
			}
		    }
		    break;
		}
	    }
    }
    rewind (stream);
}
/*}}} */
# endif				/* ! USE_XGETDEFAULT */
#endif				/* NO_RESOURCES */

/*{{{ read the resources files */
/*
 * using XGetDefault() or the hand-rolled replacement
 */
/* ARGSUSED */
/* EXTPROTO */
void            rxvtlib_extract_resources (rxvtlib *o, Display * display, const char *name)
{E_
#ifndef NO_RESOURCES
# ifdef USE_XGETDEFAULT
/*
 * get resources using the X library function
 */
    int             entry;

#  ifdef XrmEnumOneLevel
    XrmName         name_prefix[3];
    XrmClass        class_prefix[3];
    char           *displayResource;
    XrmDatabase     database;
    char           *screenResource;
    XrmDatabase     screenDatabase;

/*
 * Get screen-specific resources (X11R5) and merge into common resources.
 */
    database = NULL;
    screenDatabase = NULL;
    displayResource = XResourceManagerString (display);
    if (displayResource != NULL)
	database = XrmGetStringDatabase (displayResource);
    screenResource = XScreenResourceString (DefaultScreenOfDisplay (display));
    if (screenResource != NULL)
	screenDatabase = XrmGetStringDatabase (screenResource);
    XrmMergeDatabases (screenDatabase, &database);
    XrmSetDatabase (display, database);
#  endif

    for (entry = 0; entry < optList_size (); entry++) {
	int             s;
	char           *p;
	const char     *kw = optList[entry].kw;

	if (kw == NULL || *(&(o->rs[optList[entry].dp])) != NULL)
	    continue;		/* previously set */
	if ((p = XGetDefault (display, name, kw)) != NULL
	    || (p = XGetDefault (display, APL_SUBCLASS, kw)) != NULL
	    || (p = XGetDefault (display, APL_CLASS, kw)) != NULL) {
	    *(&(o->rs[optList[entry].dp])) = p;

	    if (optList_isBool (entry)) {
		s = Cstrcasecmp (p, "TRUE") == 0
		    || Cstrcasecmp (p, "YES") == 0
		    || Cstrcasecmp (p, "ON") == 0
		    || Cstrcasecmp (p, "1") == 0;
		if (optList_isReverse (entry))
		    s = !s;
		if (s)
		    o->Options |= (optList[entry].flag);
		else
		    o->Options &= ~(optList[entry].flag);
	    }
	}
    }

/*
 * [R5 or later]: enumerate the resource database
 */
#  ifdef XrmEnumOneLevel
#   ifdef KEYSYM_RESOURCE
    name_prefix[0] = XrmStringToName (name);
    name_prefix[1] = XrmStringToName ("keysym");
    name_prefix[2] = NULLQUARK;
    class_prefix[0] = XrmStringToName (APL_SUBCLASS);
    class_prefix[1] = XrmStringToName ("Keysym");
    class_prefix[2] = NULLQUARK;
    XrmEnumerateDatabase (XrmGetDatabase (display),
			  name_prefix,
			  class_prefix, XrmEnumOneLevel, rxvtlib_define_key, , NULL);
    name_prefix[0] = XrmStringToName (APL_CLASS);
    name_prefix[1] = XrmStringToName ("keysym");
    class_prefix[0] = XrmStringToName (APL_CLASS);
    class_prefix[1] = XrmStringToName ("Keysym");
    XrmEnumerateDatabase (XrmGetDatabase (display),
			  name_prefix,
			  class_prefix, XrmEnumOneLevel, rxvtlib_define_key, , NULL);
#   endif
#  endif

# else				/* USE_XGETDEFAULT */
/* get resources the hard way, but save lots of memory */
    const char     *const fname[] = { ".Xdefaults", ".Xresources" };
    FILE           *fd = NULL;
    char           *home;

    if ((home = getenv ("HOME")) != NULL) {
	int             i, len = strlen (home) + 2;
	char           *f = NULL;

	for (i = 0; i < (sizeof (fname) / sizeof (fname[0])); i++) {
	    f = REALLOC (f, (len + strlen (fname[i])) * sizeof (char));

	    sprintf (f, "%s/%s", home, fname[i]);

	    if ((fd = fopen (f, "r")) != NULL)
		break;
	}
	FREE (f);
    }
/*
 * The normal order to match resources is the following:
 * @ global resources (partial match, ~/.Xdefaults)
 * @ application file resources (XAPPLOADDIR/Rxvt)
 * @ class resources (~/.Xdefaults)
 * @ private resources (~/.Xdefaults)
 *
 * However, for the hand-rolled resources, the matching algorithm
 * checks if a resource string value has already been allocated
 * and won't overwrite it with (in this case) a less specific
 * resource value.
 *
 * This avoids multiple allocation.  Also, when we've called this
 * routine command-line string options have already been applied so we
 * needn't to allocate for those resources.
 *
 * So, search in resources from most to least specific.
 *
 * Also, use a special sub-class so that we can use either or both of
 * "XTerm" and "Rxvt" as class names.
 */

    rxvtlib_get_xdefaults (o, fd, name);
    rxvtlib_get_xdefaults (o, fd, APL_SUBCLASS);

#  ifdef XAPPLOADDIR
    {
	FILE           *ad = fopen (XAPPLOADDIR "/" APL_SUBCLASS, "r");

	if (ad != NULL) {
	    rxvtlib_get_xdefaults (o, ad, "");
	    fclose (ad);
	}
    }
#  endif			/* XAPPLOADDIR */

    rxvtlib_get_xdefaults (o, fd, APL_CLASS);
    rxvtlib_get_xdefaults (o, fd, "");	/* partial match */
    if (fd != NULL)
	fclose (fd);
# endif				/* USE_XGETDEFAULT */
#endif				/* NO_RESOURCES */

/*
 * even without resources, at least do this setup for command-line
 * options and command-line long options
 */
#ifdef MULTICHAR_SET
    rxvtlib_set_multichar_encoding (o, o->rs[Rs_multichar_encoding]);
#endif
#ifdef GREEK_SUPPORT
/* this could be a function in grkelot.c */
/* void set_greek_keyboard (const char * str); */
    if (o->rs[Rs_greek_keyboard]) {
	if (!strcmp (o->rs[Rs_greek_keyboard], "iso"))
	    greek_setmode (GREEK_ELOT928);	/* former -grk9 */
	else if (!strcmp (o->rs[Rs_greek_keyboard], "ibm"))
	    greek_setmode (GREEK_IBM437);	/* former -grk4 */
    }
#endif				/* GREEK_SUPPORT */

#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    {
	KeySym          sym;

	if (o->rs[Rs_bigfont_key]
	    && ((sym = XStringToKeysym (o->rs[Rs_bigfont_key])) != 0))
	    o->ks_bigfont = sym;
	if (o->rs[Rs_smallfont_key]
	    && ((sym = XStringToKeysym (o->rs[Rs_smallfont_key])) != 0))
	    o->ks_smallfont = sym;
    }
#endif
}
/*}}} */
/*----------------------- end-of-file (C source) -----------------------*/

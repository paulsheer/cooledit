/* mancmd.c - runs man and displays the output 
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include "edit.h"
extern Window main_window;

CWidget *CManpageDialog (Window in, int x, int y, int width, int height, const char *manpage);

int edit_man_page_cmd (WEdit * edit)
{
    int i;
    long start_mark, end_mark;
    char q[64];
    char *p, *man;
    CWidget *w;
    if (eval_marks (edit, &start_mark, &end_mark)) {
	w = CManpageDialog (0, 0, 0, 80, 25, 0);
	if (w) {
	    w->position |= WINDOW_UNMOVEABLE;
	    CFocus (CIdent ("mandisplayfile.text"));
	} else {
	    p = CInputDialog ("manpage", 0, 0, 0, 200, TEXTINPUT_LAST_INPUT, _(" Open Man Page "),
			      _(" No man page name is highlighted, \n" \
			      " or other man page open. " \
			      " Enter a man page : "));
	    if (p) {
		w = CManpageDialog (0, 0, 0, 80, 25, p);
		free (p);
		if (w) {
		    w->position |= WINDOW_UNMOVEABLE;
		    CFocus (CIdent ("mandisplayfile.text"));
		}
	    }
	}
    } else {
	man = q;
	for (i = 0; i < 63; i++)
	    man[i] = edit_get_byte (edit, start_mark + i);
	man[min (end_mark - start_mark, 63)] = 0;
	man += strspn (man, "\n\t()[];,\" ");
	p = man + strcspn (man, "\n\t()[];,\" ");
	*p = 0;
	w = CManpageDialog (0, 0, 0, 80, 25, man);
	if (w) {
	    w->position |= WINDOW_UNMOVEABLE;
	    CFocus (CIdent ("mandisplayfile.text"));
	}
    }
    return 1;
}



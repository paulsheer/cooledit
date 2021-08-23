/* disable.c - disabling and enabling widgets
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"

#include "coollocal.h"
#include "regex.h"


extern char *regtools_old_pattern;


void CSetDisable (const char *ident, int disable)
{
    int i = last_widget + 1;
    if (strcmp (ident, "*")) {
	while (--i)
	    if (CIndex (i))
		switch (regexp_match ((char *) ident, CIndex (i)->ident, match_file)) {
		case 1:
		    CIndex (i)->disabled = disable;
		    break;
		case -1:
/* NLS ? */
		    CFatalErrorDialog (20, 20, " Invalid regular expression in call to CDisable() ");
		    break;
		}
    } else {
	while (--i)
	    if (CIndex (i))
		CIndex (i)->disabled = disable;
    }
}

void CDisable (const char *ident)
{
    if (!ident) {
	if (regtools_old_pattern) {
	    free (regtools_old_pattern);
	    regtools_old_pattern = 0;
	}
    } else
	CSetDisable (ident, 1);
}

void CEnable (const char *ident)
{
    CSetDisable (ident, 0);
}

/* These are to save or restore the present state of the widgets enablement. */
/* You may create or destroy widget between save and restores, but
do not destroy _and_ create, because these won't remember properly which
are have gone and which are new. */

void CBackupState(CState *s)
{
    int i = last_widget + 1;
    memset(s, 0, sizeof(CState));
    while (--i) {
	if (CIndex (i)) {
	    s->mask[i/32] |= (0x1L << (i % 32));
	    if(CIndex (i)->disabled != 0)
		s->state[i/32] |= (0x1L << (i % 32));
	}
    }
}

void CRestoreState (CState * s)
{
    int i = last_widget + 1;
    while (--i)
	if (CIndex (i))
	    if (((u_32bit_t) s->mask[i / 32] & (0x1L << (i % 32))))
		CIndex (i)->disabled = ((s->state[i / 32] & (0x1L << (i % 32))) != 0);
}




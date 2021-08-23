/* cmdlineopt.c and cmdlineopt.h are for processing command line options.
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmdlineopt.h"

void get_cmdline_options_free_list (struct cmdline_option_free *fl)
{
    while(fl->free_list_len) {
        fl->free_list_len--;
	free (fl->free_list[fl->free_list_len]);
    }
}

int get_cmdline_options (int argc, char **argv, struct prog_options *args, struct cmdline_option_free *fl)
{
    int i = 0, j, c;
    int other = 0;
    fl->free_list_len = 0;
    for (i = 1; i < argc; i++) {
	if (*argv[i] != '-') {	/* something that is not an option */
	    for (j = 0; args[j].type; j++)
		if (args[j].char_opt == ' ') {
		    fl->free_list[fl->free_list_len++] = args[j].strs[other] = malloc (strlen (argv[i]) + 1);
		    strcpy (args[j].strs[other], argv[i]);
		    other++;
		    goto cont;
		}
	    return i;
	}
	c = 0;
	while (++c > 0) {	/* try each letter in a combined option eg 'tar -xvzf' */
	    for (j = 0; args[j].type; j++) {
		if (!strcmp (args[j].long_opt, argv[i]) || !strcmp (args[j].short_opt, argv[i])) {
		    c = -1;	/* not a combined option */
		    goto valid_opt;
		}
		if (argv[i][0] == '-' && argv[i][c] == args[j].char_opt) {
		    if (!argv[i][c + 1])	/* this must be the last letter in the combined option */
			c = -1;
		    goto valid_opt;
		}
		continue;

	      valid_opt:;
		switch (args[j].type) {
		case ARG_SET:{
			int *t;
			t = (int *) args[j].option;
			*t = 1;
			goto next;
		    }
		case ARG_CLEAR:{
			int *t;
			t = (int *) args[j].option;
			*t = 0;
			goto next;
		    }
		case ARG_IGNORE:
		    /* do nothing with this option */
		    goto next;
		}

		if (i + 1 != argc && argv[i + 1]
		    && c < 0	/* must be the last option if a combined option */
		    ) {
		    ++i;
		    switch (args[j].type) {
			int *t;
			double *f;
		    case ARG_ON_OFF:
			if (strcmp (argv[i], "on") == 0) {
			    t = (int *) args[j].option;
			    *t = 1;
			} else if (strcmp (argv[++i], "off") == 0) {
			    t = (int *) args[j].option;
			    *t = 0;
			} else
			    return i;
			goto next;
		    case ARG_YES_NO:
			if (strcmp (argv[i], "yes") == 0) {
			    t = (int *) args[j].option;
			    *t = 1;
			} else if (strcmp (argv[++i], "no") == 0) {
			    t = (int *) args[j].option;
			    *t = 0;
			} else
			    return i;
			goto next;
		    case ARG_STRING:
			fl->free_list[fl->free_list_len++] = *(args[j].str) = malloc (strlen (argv[i]) + 1);
			strcpy (*(args[j].str), argv[i]);
			goto next;
		    case ARG_STRINGS:{
			    /* get all argv's after this option until we reach another option */
			    int k = 0;
			    while (i < argc && *argv[i] != '-') {
				args[j].strs[k] = malloc (strlen (argv[i]) + 1);
				strcpy (args[j].strs[k], argv[i]);
				k++;
				i++;
			    }
			    i--;	/* will be incremented at end of loop */
			    goto next;
			}
		    case ARG_INT:
			t = (int *) args[j].option;
			*t = atoi (argv[i]);
			goto next;
		    case ARG_DOUBLE:
			f = (double *) args[j].option;
			*f = atof (argv[i]);
			goto next;
		    }
		    i--;
		}
		return i;	/* option parameter not found */
	    }			/* j */
	    return i;		/* option not found */
	  next:;
	}			/* c */
      cont:;
    }

    return 0;
}

/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* cmdlineopt.c and cmdlineopt.h are for processing command line options.
   Copyright (C) 1996-2022 Paul Sheer
 */


/*
    The kind of options it processes are summaries as follows:

	1. on-off switch: -X|--longX on|off
	    eg
	    --sound on
	    --sound off
	    -s on
	    -s off
	
	2. yes-no switch: (same as on-off)
	
	3. Ordinary option: -X|--longX
	    eg
	    --verbose
	    -v
	
	4. String options: -X|--longX <some_string>
	    eg
	    --file program.c
	    -f program.c
	
	5. Integer or float options: -X|--longX N
	    eg
	    --count 20
	    -C 20
	
	6. String list options: -X|--longX <file1> <file2> <file3> ...
	    eg
	    --files hello.c whereis.c prog.c
	    -f hello.c whereis.c prog.c
	
	an example of usage follows:
	
	Suppose your program requires the following info to be set on the command line:
	    1. a list of files
	    2. a list of libraries
	    3. a device filename
	    4. an integer to repeat the operation N times.
	    5. whether verbose output is needed.
	    6. whether case sensitivity is on.
	    7. whether debugging mode is needed.

	You would store this data as global variables. The defaults would
	be set at compile time:

	char *libraries[] =
	    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	char *files[] =
	    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	char *device = "";
	int option_verbose_output = 0;
	int option_debug = 1;
	int option_case_sensitive = 1;
	int option_repeat = 1;
	
	Then create a prog_args structure. Each entry contains an abbreviated
	option, and then two synonyms that my be used for the options.
	(See the prog_options structure below).
	
	struct prog_options arg_opts[] =
	{
	    {' ', "", "", ARG_STRINGS, 0, files, 0},
	    {'L', "", "--libraries", ARG_STRINGS, 0, libraries, 0},
	    {'d', "", "--device", ARG_STRING, &device, 0, 0},
	    {'v', "-verb", "--verbose", ARG_SET, 0, 0, &option_verbose_output},
	    {'b', "-debug", "--debug", ARG_CLEAR, 0, 0, &option_debug},
	    {0, "-cs", "--case-sensitivity", ARG_ON_OFF, &option_case_sensitive, 0, 0},
	    {'r', "-rep", "--repeat", ARG_INT, 0, 0, &count},
	    {0, 0, 0, 0, 0, 0, 0}
	};
	
	
	and call get_cmdline_options as follows:
	
	main (int argc, char **argv)
	{
	    int i;
	    i = get_cmdline_options (argc, argv, arg_opts);
	.
	.
	.

	}

	On return, 'i' will be zero if there were no errors, or it will
	be set to the number of the parameter that the error occured at.


	Notes:
	------

	The type member indicates the kind of option.
	The type member ARG_SET causes the 'option_verbose_output' to be
	set to 1 if the switch is encountered. ARG_CLEAR causes it
	to be set to zero. ARG_ON_OFF causes 'option_case_sensitivity'
	to be set to 1 or 0, depending if its parameter
	is 'yes' or 'no' respectively. ARG_INT uses 'atoi' to get
	the integer after the option into 'count'.

	If the char_opt member is set to ' ' (as above) it means that any
	isolated parameters on the command line (strings that are not
	options or option parameters) must be recorded into
	the specified list. Hence in the above (crude) example, any thing
	on the command line that is not an option, or does not belong
	to an option, is considered to be a filename, and is put into 
	the list 'files[]'.

	Libraries is the list of parameters that come after a '-L' or
	'--libraries'. A list is specified with ARG_STRINGS. A list is
	terminated by any parameter beginning with '-'.

	The char_opt member contains the abbreviated option. eg if
	char_opt contains 'x' then the command line parameter is '-x'.
	It is not the same as the short_opt member, as multiple of these
	can be used together like in the tar command: 'tar -xvzf <file>' etc.

	Usually 'short_opt' would just contain a two letter version
	of the option if a one letter version isn't possible
	(in this case char_opt would be set to zero). You do
	not have to have both a long, a short and a character option.


	an example command line would be:
	    progname file1 file2 file3 -L lib1 lib2 lib3 -cs on -vbd /dev/par1 -r 5
	which would be the same as
	    progname file1 --libraries lib1 lib2 lib3 --repeat 5
		--device /dev/par1 file2 --verbose --case-sensitivity on file3 --debug

	This library cannot be used to process any kind of command line,
	put it should be useful for most applications.
*/

#ifndef _COMMAND_LINE_H
#define _COMMAND_LINE_H

#define ARG_ON_OFF	1
#define ARG_STRING	2
#define ARG_STRINGS	3
#define ARG_SET		4
#define ARG_CLEAR	5
#define ARG_YES_NO	6

#define ARG_IGNORE	7
    /* ARG_IGNORE causes an option to be ignored. */

#define ARG_INT		8
#define ARG_DOUBLE	9


struct prog_options {
    char char_opt;
    char *short_opt;
    char *long_opt;
    int type;			/* one of the #define's above */
    char **str;			/* pointer to a single string */
    char **strs;		/* pointer to an array of strings */
    void *option;		/* an integer or double */
#if 0
    char *dummy_opt;		/* for the help text */
    char *help;			/* help line */
#endif
};

struct cmdline_option_free {
    char *free_list[256];
    int free_list_len;
};

int get_cmdline_options (int argc, char **argv, struct prog_options *args, struct cmdline_option_free *fl);
void get_cmdline_options_free_list (struct cmdline_option_free *fl);

#if 0
void print_cmdline_options (char *progname, struct prog_options *args);
#endif

#endif				/* !_COMMAND_LINE_H */

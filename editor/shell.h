/* shell.h
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include "edit.h"
#include "loadfile.h"
#include "coolwidget.h"

#define MAX_NUM_SCRIPTS 100

/* things to do before running the script */
#define SHELL_OPTION_SAVE_BLOCK					(1<<1)
#define SHELL_OPTION_SAVE_EDITOR_FILE				(1<<2)
#define SHELL_OPTION_REQUEST_ARGUMENTS				(1<<3)

/* things to do during the running of the script. If either
   of these are set, the shell runs in the background */
#define SHELL_OPTION_DISPLAY_STDOUT_CONTINUOUS			(1<<4)
#define SHELL_OPTION_DISPLAY_STDERR_CONTINUOUS			(1<<5)

/* things to do on completion of the script */
#define SHELL_OPTION_DELETE_BLOCK				(1<<6)
#define SHELL_OPTION_INSERT_TEMP_FILE				(1<<7)
#define SHELL_OPTION_INSERT_BLOCK_FILE				(1<<8)
#define SHELL_OPTION_INSERT_CLIP_FILE				(1<<9)
#define SHELL_OPTION_INSERT_STDOUT				(1<<10)
#define SHELL_OPTION_INSERT_STDERR				(1<<11)
/* displays after executing script */
#define SHELL_OPTION_DISPLAY_ERROR_FILE				(1<<12)
/* doesn't change anything if error file is empty after executing script */
#define SHELL_OPTION_CHECK_ERROR_FILE				(1<<13)
#define SHELL_OPTION_RUN_IN_BACKGROUND				(1<<14)
#define SHELL_OPTION_ANNOTATED_BOOKMARKS			(1<<15)
#define SHELL_OPTION_GOTO_FILE_LINE_COLUMN                      (1<<16)
#define SHELL_OPTION_COMPLETE_WORD                              (1<<17)

/* the following options cannot coexist:

(INSERT_STDOUT or INSERT_STDERR) with
		    (DISPLAY_STDOUT_CONTINUOUS or DISPLAY_STDERR_CONTINUOUS)
*/

/* Before the script runs the following substitutions are made
   via a percent sign. You can thus use the % to refer to certain
   strings, eg %t gets replaced with the name of the temporary 
   file. A '%%' is replaced with a literal '%' */

/* current directory without trailing slash */
#define SHELL_SUBS_CURRENT_DIRECTORY				'd'

/* full filename without path */
#define SHELL_SUBS_EDITOR_FILE					'f'

/* name only without path, extension or last dot */
#define SHELL_SUBS_EDITOR_FILE_NAME				'n'

/* current cursor line number */
#define SHELL_SUBS_EDITOR_FILE_LINE				'L'

/* current cursor column position (assume tabs as 1 char, and 1-indexed) */
#define SHELL_SUBS_EDITOR_FILE_COLUMN				'C'

/* extension only with dot */
#define SHELL_SUBS_EDITOR_FILE_EXTENSION			'x'

/* full path without trailing slash or filename */
#define SHELL_SUBS_EDITOR_FILE_PATH				'p'

/* name of temporary file you can use in the shell */
#define SHELL_SUBS_TEMP_FILE					't'

/* name of block file that is saved */
#define SHELL_SUBS_BLOCK_FILE					'b'

/* name of clip file */
#define SHELL_SUBS_CLIP_FILE					'c'

/* name of error file to display */
#define SHELL_SUBS_ERROR_FILE					'e'

/* arguments given by user */
#define SHELL_SUBS_ARGUMENTS					'a'

/* arguments given by user */
#define SHELL_SUBS_FONT_FIXED					'F'

/* arguments given by user */
#define SHELL_SUBS_FONT						'O'

/* hence '%n.%x' == '%f' */

struct shell_cmd {
    char name[40];		/* name that appears on the menu */
    char menu[40];
    char menu_hot_key;	/* hotkey to underline on menu */
    KeySym key;			/* key code for hot key */
    int alt_modifier;	                /* 1 or 0:  is the alt modifier pressed? used for default scripts */
    unsigned long keyboard_state;	/* keyboard state to execute */
    char prompt[160];		/* prompt string if arguments are requested */
    unsigned int options;	/* flags #define'd about */
    char *last_options;		/* last arguments typed typed by user */
    char *script;		/* shell script to run on commence (malloc'ed) */
};


/* for external use */
void shell_set_alt_modifier (unsigned long modifier);
void load_scripts (void);
void update_script_menu_items (void);
void execute_script (WEdit *edit, int i);
int get_script_number_from_key (unsigned int state, KeySym keysym);


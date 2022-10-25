
#ifdef HAVE_TERMIOS_H
typedef struct termios ttymode_t;
#else
typedef struct _ttymode_t ttymode_t;
#endif				/* HAVE_TERMIOS_H */

struct cterminal {
    char *ttydev;
    short changettyowner;
    struct stat ttyfd_stat;
    pid_t cmd_pid;
    int erase_char;
#define RXVTLIB_MAX_ENVVAR      32
    char *envvar[RXVTLIB_MAX_ENVVAR];
    int n_envvar;
};

struct cterminal_config {
    const char *color_env;
    const char *display_env_var;
    unsigned long term_win_id;
    const char *term_name;
    const char *colorterm_name;
    int col;
    int row;
    int login_shell;
    int do_sleep;
    int charset_8bit;
    int env_fg;
    int env_bg;
};

#define CTERMINAL_IGNORE                0
#define CTERMINAL_SAVE	                's'
#define CTERMINAL_RESTORE               'r'

#define DO_EXIT                         ((int) 1 << 30)
#ifndef EXIT_SUCCESS		        /* missing from <stdlib.h> */
# define EXIT_SUCCESS		        0	/* exit function success */
# define EXIT_FAILURE		        1	/* exit function failure */
#endif

void cterminal_tt_winsize (struct cterminal *o, int fd, int col, int row);
void cterminal_cleanup (struct cterminal *o);
int cterminal_get_pty (struct cterminal *o);
int cterminal_run_command (struct cterminal *o, struct cterminal_config *config, char *const argv[]);


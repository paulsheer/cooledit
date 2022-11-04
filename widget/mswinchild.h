

#undef MSWIN_HANDLE
typedef void *MSWIN_HANDLE;
#define MSWIN_INVALID_HANDLE_VALUE      ((void *) (__int64) -1)

struct cterminal_config {
    char display_env_var[128];
    char term_name[32];
    char colorterm_name[32];
    unsigned long term_win_id;
    int col;
    int row;
    int login_shell;
    int do_sleep;
    int charset_8bit;
    int env_fg;
    int env_bg;
    int erase_char; /* return value */
    unsigned long cmd_pid; /* return value */
    MSWIN_HANDLE process_handle;
#define CTERMINAL_TTYDEV_SZ     64
    char ttydev[CTERMINAL_TTYDEV_SZ]; /* return value */
};

struct cterminal {
    int cmd_pid;
    MSWIN_HANDLE process_handle;
    MSWIN_HANDLE cmd_fd_stdin;
    MSWIN_HANDLE cmd_fd_stdout;
};

#define CTERMINAL_ERR_MSG_LEN           384

void cterminal_cleanup (struct cterminal *c);

int cterminal_run_command (struct cterminal *o, struct cterminal_config *config, int dumb_terminal, const char *log_origin_host, char *const argv[], char *errmsg);;

int kill_child_get_exit_status (int pid, MSWIN_HANDLE h, unsigned long *exit_status);



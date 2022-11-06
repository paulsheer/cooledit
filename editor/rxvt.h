

struct _rxvtlib;
typedef struct _rxvtlib rxvtlib;

struct rxvt_startup_options {
    int term_8bit;
    int large_font;
    int backspace_ctrl_h;
    int backspace_127;
    char host[256];
};

extern struct rxvt_startup_options rxvt_startup_options;

rxvtlib *rxvt_start (const char *host, Window win, char **argv, int do_sleep, unsigned long rxvt_options);

int rxvt_have_pid (const char *host, pid_t pid);
void rxvt_set_input_context (rxvtlib *o, XIMStyle input_style);
int rxvt_event (XEvent * xevent);
void rxvt_init (void);
void rxvtlib_shutall (void);
void rxvt_kill (pid_t p);
void rxvt_get_tty_name (rxvtlib * rxvt, char *p);
void rxvt_get_pid (rxvtlib * rxvt, pid_t *pid, const char *host);
int rxvt_startup_dialog (const char *host);


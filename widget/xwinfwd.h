
#ifdef XWIN_FWD

#define XFWDSTATUS_DATA                 1
#define XFWDSTATUS_SHUTDOWN             2

typedef int (*add_watch_cb_f) (char *file, int line, int sock, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how, void *data);
typedef void (*remove_watch_cb_f) (int sock, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how);

struct xwinclient_data;
struct xwinfwd_data;
struct sock_data;
struct remotefs;

void xwinclient_set_watch (add_watch_cb_f a, remove_watch_cb_f b);
struct xwinclient_data *xwinclient_alloc (int xwin_fd);
int xwinclient_new_client (struct remotefs *rfs, struct xwinclient_data *x, unsigned long xwin_client_id);
void xwinclient_write (struct xwinclient_data *x, unsigned long xwin_client_id, const char *buf, int buflen);
void xwinclient_freeall (struct xwinclient_data *x);
void xwinclient_kill (struct xwinclient_data *x, unsigned long xwin_client_id);
struct xwinfwd_data *xwinfwd_alloc (void);
void xwinfwd_prep_sockets (struct xwinfwd_data *x, fd_set * rd, fd_set * wr, int *n);
int xwinfwd_process_sockets (struct sock_data *sock_data, struct xwinfwd_data *x, fd_set * rd, fd_set * wr);
int xwinfwd_display_port (struct xwinfwd_data *x);
int xwinfwd_listen_socket (struct xwinfwd_data *x);
int xwinfwd_new_client (struct xwinfwd_data *x);
void xwinfwd_write (struct xwinfwd_data *x, unsigned long xwin_client_id, const char *buf, int buflen);
void xwinfwd_kill (struct xwinfwd_data *x, unsigned long xwin_client_id);
void xwinfwd_freeall (struct xwinfwd_data *x);

#endif



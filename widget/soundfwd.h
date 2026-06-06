
#ifdef SOUND_FWD

#define SOUNDSTATUS_DATA                 3
#define SOUNDSTATUS_SHUTDOWN             4

typedef int (*add_watch_cb_f) (char *file, int line, int sock, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how, void *data);
typedef void (*remove_watch_cb_f) (int sock, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how);

struct soundclient_data;
struct soundfwd_data;
struct sock_data;
struct remotefs;

void soundclient_set_watch (add_watch_cb_f a, remove_watch_cb_f b);
struct soundclient_data *soundclient_alloc (const char *sound_env_var);
int soundclient_new_client (struct remotefs *rfs, struct soundclient_data *x, unsigned long sound_client_id);
void soundclient_write (struct soundclient_data *x, unsigned long sound_client_id, const char *buf, int buflen);
void soundclient_freeall (struct soundclient_data *x);
void soundclient_kill (struct soundclient_data *x, unsigned long sound_client_id);
struct soundfwd_data *soundfwd_alloc (void);
void soundfwd_prep_sockets (struct soundfwd_data *x, fd_set * rd, fd_set * wr, int *n);
int soundfwd_process_sockets (struct sock_data *sock_data, struct soundfwd_data *x, fd_set * rd, fd_set * wr);
void soundfwd_construct_envvar (struct soundfwd_data *x, char *e1, int l1, char *e2, int l2);
int soundfwd_listen_socket (struct soundfwd_data *x);
int soundfwd_new_client (struct soundfwd_data *x);
void soundfwd_write (struct soundfwd_data *x, unsigned long sound_client_id, const char *buf, int buflen);
void soundfwd_kill (struct soundfwd_data *x, unsigned long sound_client_id);
void soundfwd_freeall (struct soundfwd_data *x);

#endif



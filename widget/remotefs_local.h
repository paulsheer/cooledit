
#ifdef MSWIN

#undef WSAEVENT
typedef void *WSAEVENT;

#define makedev(a,b)            (((a) << 8) | ((b) & 0xFF))
#define random()                rand()
#define rename(a,b)             windows_rename((a),(b))

#undef INVALID_HANDLE_VALUE
#define HANDLE                  int
#define INVALID_HANDLE_VALUE    (-1)

#define socklen_t               int

#else

#define _O_BINARY               0
#define SOCKET                  int
#define INVALID_SOCKET          (-1)
#define SOCKET_ERROR            (-1)
#define HANDLE                  int
#define INVALID_HANDLE_VALUE    (-1)
#define ioctlsocket(a,b,c)      ioctl(a,b,c)
#define closesocket(s)          close(s)
#define socklen_t               unsigned int

#endif



#define REMOTEFS_ACTION_NOTIMPLEMENTED          0
#define REMOTEFS_ACTION_READDIR                 1
#define REMOTEFS_ACTION_READFILE                2
#define REMOTEFS_ACTION_WRITEFILE               3
#define REMOTEFS_ACTION_CHECKORDINARYFILEACCESS 4
#define REMOTEFS_ACTION_STAT                    5
#define REMOTEFS_ACTION_CHDIR                   6
#define REMOTEFS_ACTION_REALPATHIZE             7
#define REMOTEFS_ACTION_GETHOMEDIR              8
#define REMOTEFS_ACTION_ENABLECRYPTO            9
#define REMOTEFS_ACTION_SHELLCMD                10
#define REMOTEFS_ACTION_SHELLRESIZE             11
#define REMOTEFS_ACTION_SHELLREAD               12
#define REMOTEFS_ACTION_SHELLWRITE              13
#define REMOTEFS_ACTION_SHELLKILL               14
#define REMOTEFS_ACTION_SHELLSIGNAL             15


#define CONNCHECK_SUCCESS       0
#define CONNCHECK_WAITING       1
#define CONNCHECK_ERROR         2


// #define LEGACY_IP4_ONLY

struct remotefs_sockaddr_s_ {
#ifdef LEGACY_IP4_ONLY
    struct sockaddr ss;
#else
    struct sockaddr_storage ss;
#endif
};

struct sock_data;
typedef struct remotefs_sockaddr_s_ remotefs_sockaddr_t;

SOCKET remotefs_listen_socket (const char *listen_address, int listen_port);
int remotefs_connection_check (const SOCKET s, int write_set);
int remotefs_sockaddr_t_addressfamily (remotefs_sockaddr_t *a);
int send_blind_message (struct sock_data *sock_data, int action, unsigned long multiplex, int xfwdstatus, char *data1, int l1, char *data2, int l2);
struct sock_data *remotefs_get_sock_data (struct remotefs *rfs);



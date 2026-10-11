/*
 * Standalone getaddrinfo()/freeaddrinfo() substitution.
 *
 * On Solaris (and some other platforms) getaddrinfo() lives only in the
 * shared libsocket.so / libnsl.so with no static archive, so a fully
 * static link cannot resolve it.  This file provides a self-contained
 * implementation that resolves names with inet_pton(), then /etc/hosts,
 * then a raw UDP DNS query against the nameservers listed in
 * /etc/resolv.conf.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DNS_PORT          53
#define DNS_HDR_SIZE      12
#define DNS_MAX_PKT       4096

#define DNS_RCODE_NOERROR 0

#define DNS_TYPE_A        1
#define DNS_TYPE_AAAA     28

#define DNS_CLASS_IN      1

#define MAX_ADDRS         32
#define MAX_NAMESERVERS   8

typedef struct {
    int             family;
    struct in_addr  v4;
    struct in6_addr v6;
} addr_entry_t;

typedef struct {
    int                     family;
    struct sockaddr_storage ss;
    socklen_t               len;
} ns_entry_t;

static uint16_t rd16 (const unsigned char *p)
{
    return (uint16_t) ((p[0] << 8) | p[1]);
}

static char *dupstr (const char *s)
{
    size_t n = strlen (s) + 1;
    char *p = malloc (n);
    if (p)
        memcpy (p, s, n);
    return p;
}

static int collect_addr (addr_entry_t * list, int *count, int family, const void *addr)
{
    int i;

    if (*count >= MAX_ADDRS)
        return 0;
    for (i = 0; i < *count; i++) {
        if (list[i].family == family) {
            if (family == AF_INET && memcmp (&list[i].v4, addr, 4) == 0)
                return 0;
            if (family == AF_INET6 && memcmp (&list[i].v6, addr, 16) == 0)
                return 0;
        }
    }
    list[*count].family = family;
    if (family == AF_INET)
        memcpy (&list[*count].v4, addr, 4);
    else
        memcpy (&list[*count].v6, addr, 16);
    (*count)++;
    return 1;
}

static int proto_matches (const char *prot, int socktype)
{
    if (socktype == 0)
        return 1;
    if (socktype == SOCK_STREAM)
        return strcmp (prot, "tcp") == 0;
    if (socktype == SOCK_DGRAM)
        return strcmp (prot, "udp") == 0;
    return 0;
}

/* Resolve a service name to a host-order port number. */
static int service_to_port (const char *service, int socktype, int *port_out)
{
    int port = 0;

    if (!service || !service[0]) {
        *port_out = 0;
        return 0;
    }

    if (isdigit ((unsigned char) service[0])) {
        char *end;
        long v = strtol (service, &end, 10);
        if (*end != '\0' || v < 0 || v > 65535)
            return -1;
        *port_out = (int) v;
        return 0;
    }

    {
        FILE *f = fopen ("/etc/services", "r");
        char line[512];
        int ok = -1;

        if (!f)
            return -1;
        while (fgets (line, sizeof (line), f)) {
            char *h = strchr (line, '#');
            char name[64], prot[16];
            int p;

            if (h)
                *h = '\0';
            if (sscanf (line, "%63s %d/%15s", name, &p, prot) == 3) {
                if (strcasecmp (name, service) == 0 && proto_matches (prot, socktype)) {
                    port = p;
                    ok = 0;
                    break;
                }
            }
        }
        fclose (f);
        if (ok != 0)
            return -1;
        *port_out = port;
        return 0;
    }
}

/* Scan /etc/hosts; return number of matching lines found (>=0). */
static int hosts_lookup (const char *node, int family, addr_entry_t * list, int *count)
{
    FILE *f = fopen ("/etc/hosts", "r");
    char line[1024];
    int found = 0;

    if (!f)
        return 0;
    while (fgets (line, sizeof (line), f)) {
        char *h = strchr (line, '#');
        char *p = line;
        char *tok[32];
        int nt = 0, i, matches = 0;
        struct in_addr a4;
        struct in6_addr a6;

        if (h)
            *h = '\0';
        while (nt < 32) {
            while (*p && isspace ((unsigned char) *p))
                p++;
            if (!*p)
                break;
            tok[nt++] = p;
            while (*p && !isspace ((unsigned char) *p))
                p++;
            if (*p)
                *p++ = '\0';
        }
        if (nt < 2)
            continue;
        for (i = 1; i < nt; i++)
            if (strcasecmp (tok[i], node) == 0) {
                matches = 1;
                break;
            }
        if (!matches)
            continue;
        if (inet_pton (AF_INET, tok[0], &a4) == 1) {
            if (family == AF_UNSPEC || family == AF_INET)
                collect_addr (list, count, AF_INET, &a4);
            found = 1;
        } else if (inet_pton (AF_INET6, tok[0], &a6) == 1) {
            if (family == AF_UNSPEC || family == AF_INET6)
                collect_addr (list, count, AF_INET6, &a6);
            found = 1;
        }
    }
    fclose (f);
    return found;
}

/* Extract nameserver addresses from /etc/resolv.conf. */
static int parse_resolv_conf (ns_entry_t * out)
{
    FILE *f = fopen ("/etc/resolv.conf", "r");
    char line[512];
    int n = 0;

    if (!f)
        return 0;
    while (fgets (line, sizeof (line), f) && n < MAX_NAMESERVERS) {
        char *h = strpbrk (line, "#;");
        char kw[64], addr[64];
        struct in_addr a4;
        struct in6_addr a6;

        if (h)
            *h = '\0';
        if (sscanf (line, "%63s %63s", kw, addr) != 2)
            continue;
        if (strcmp (kw, "nameserver") != 0)
            continue;
        if (inet_pton (AF_INET, addr, &a4) == 1) {
            struct sockaddr_in *sin = (struct sockaddr_in *) &out[n].ss;
            memset (sin, 0, sizeof (*sin));
            sin->sin_family = AF_INET;
            sin->sin_port = htons (DNS_PORT);
            sin->sin_addr = a4;
            out[n].family = AF_INET;
            out[n].len = sizeof (*sin);
            n++;
        } else if (inet_pton (AF_INET6, addr, &a6) == 1) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) &out[n].ss;
            memset (sin6, 0, sizeof (*sin6));
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons (DNS_PORT);
            sin6->sin6_addr = a6;
            out[n].family = AF_INET6;
            out[n].len = sizeof (*sin6);
            n++;
        }
    }
    fclose (f);
    return n;
}

static int dns_encode_qname (const char *name, unsigned char *pkt, int pktsize, int off)
{
    const char *p = name;
    int o = off;

    while (*p) {
        const char *dot = strchr (p, '.');
        int l = dot ? (int) (dot - p) : (int) strlen (p);

        if (l == 0) {
            p++;
            continue;
        }
        if (l > 63 || o + 1 + l + 1 > pktsize)
            return -1;
        pkt[o++] = (unsigned char) l;
        memcpy (pkt + o, p, l);
        o += l;
        p += l;
        if (*p == '.')
            p++;
    }
    if (o + 1 > pktsize)
        return -1;
    pkt[o++] = 0;
    return o;
}

/* Advance *off past a (possibly compressed) DNS name. */
static int dns_skip_name (const unsigned char *pkt, int len, int *off)
{
    int o = *off;

    while (o < len) {
        unsigned char c = pkt[o];
        if (c == 0) {
            o++;
            break;
        }
        if ((c & 0xC0) == 0xC0) {       /* compression pointer */
            if (o + 1 >= len)
                return -1;
            o += 2;
            break;
        }
        if (c > 63 || o + 1 + c > len)
            return -1;
        o += 1 + c;
    }
    *off = o;
    return 0;
}

static int dns_parse_response (const unsigned char *pkt, int len, int want_family,
                               addr_entry_t * list, int *count)
{
    int off = DNS_HDR_SIZE;
    int qd, an, i;

    if (len < DNS_HDR_SIZE)
        return -1;
    if ((pkt[3] & 0x0f) != DNS_RCODE_NOERROR)
        return -1;
    qd = rd16 (pkt + 4);
    an = rd16 (pkt + 6);

    for (i = 0; i < qd; i++) {
        if (dns_skip_name (pkt, len, &off) < 0)
            return -1;
        off += 4;               /* QTYPE + QCLASS */
        if (off > len)
            return -1;
    }

    for (i = 0; i < an; i++) {
        uint16_t type, rdlen;

        if (dns_skip_name (pkt, len, &off) < 0)
            return -1;
        if (off + 10 > len)
            return -1;
        type = rd16 (pkt + off);
        rdlen = rd16 (pkt + off + 8);
        off += 10;              /* TYPE + CLASS + TTL + RDLENGTH */
        if (off + rdlen > len)
            return -1;
        if (type == DNS_TYPE_A && rdlen == 4 && want_family == AF_INET) {
            struct in_addr a;
            memcpy (&a, pkt + off, 4);
            collect_addr (list, count, AF_INET, &a);
        } else if (type == DNS_TYPE_AAAA && rdlen == 16 && want_family == AF_INET6) {
            struct in6_addr a;
            memcpy (&a, pkt + off, 16);
            collect_addr (list, count, AF_INET6, &a);
        }
        off += rdlen;
    }
    return 0;
}

static int dns_query_ns (const unsigned char *query, int qlen,
                         const ns_entry_t * ns, int want_family,
                         addr_entry_t * list, int *count)
{
    int fd, r, n;
    unsigned char resp[DNS_MAX_PKT];
    struct timeval tv;
    fd_set rfds;
    socklen_t fromlen;
    struct sockaddr_storage from;

    fd = socket (ns->family, SOCK_DGRAM, 0);
    if (fd < 0)
        return -1;

    r = sendto (fd, query, qlen, 0, (const struct sockaddr *) &ns->ss, ns->len);
    if (r < 0) {
        close (fd);
        return -1;
    }

    tv.tv_sec = 2;
    tv.tv_usec = 0;
    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);
    r = select (fd + 1, &rfds, NULL, NULL, &tv);
    if (r <= 0) {
        close (fd);
        return -1;
    }

    fromlen = sizeof (from);
    n = recvfrom (fd, resp, sizeof (resp), 0, (struct sockaddr *) &from, &fromlen);
    close (fd);
    if (n < DNS_HDR_SIZE)
        return -1;
    if (rd16 (resp) != rd16 (query))       /* mismatched transaction id */
        return -1;
    return dns_parse_response (resp, n, want_family, list, count);
}

static int dns_lookup (const char *node, int family, addr_entry_t * list, int *count)
{
    ns_entry_t nss[MAX_NAMESERVERS];
    int n_ns = parse_resolv_conf (nss);
    uint16_t qtypes[2];
    int nqt = 0, i, j;
    uint16_t id = (uint16_t) (getpid () ^ 0x1234);

    if (family == AF_UNSPEC || family == AF_INET)
        qtypes[nqt++] = DNS_TYPE_A;
    if (family == AF_UNSPEC || family == AF_INET6)
        qtypes[nqt++] = DNS_TYPE_AAAA;

    for (j = 0; j < nqt; j++) {
        int before = *count;
        int want = (qtypes[j] == DNS_TYPE_A) ? AF_INET : AF_INET6;

        for (i = 0; i < n_ns; i++) {
            unsigned char q[DNS_MAX_PKT];
            int qlen;

            memset (q, 0, DNS_HDR_SIZE);
            q[0] = (unsigned char) (id >> 8);
            q[1] = (unsigned char) id;
            q[2] = 0x01;        /* RD = 1 */
            q[3] = 0x00;
            q[4] = 0x00;
            q[5] = 0x01;        /* QDCOUNT = 1 */
            qlen = dns_encode_qname (node, q, sizeof (q), DNS_HDR_SIZE);
            if (qlen < 0)
                continue;
            q[qlen++] = (unsigned char) (qtypes[j] >> 8);
            q[qlen++] = (unsigned char) qtypes[j];
            q[qlen++] = 0x00;
            q[qlen++] = 0x01;   /* QCLASS = IN */

            dns_query_ns (q, qlen, &nss[i], want, list, count);
            if (*count > before)
                break;          /* first responding server wins */
        }
    }
    return *count;
}

static int build_result (const addr_entry_t * list, int count, int family,
                         int socktype, int protocol, int flags, const char *node,
                         int port, struct addrinfo **res)
{
    struct addrinfo *head = NULL, *tail = NULL;
    int i, emitted = 0;
    int proto = protocol;

    if (proto == 0)
        proto = (socktype == SOCK_DGRAM) ? IPPROTO_UDP : IPPROTO_TCP;

    for (i = 0; i < count; i++) {
        struct addrinfo *ai;
        int fam = list[i].family;

        if (family != AF_UNSPEC && fam != family)
            continue;

        ai = calloc (1, sizeof (*ai));
        if (!ai) {
            freeaddrinfo (head);
            return EAI_MEMORY;
        }
        ai->ai_family = fam;
        ai->ai_socktype = socktype;
        ai->ai_protocol = proto;
        ai->ai_flags = flags;

        if (fam == AF_INET) {
            struct sockaddr_in *sin = calloc (1, sizeof (*sin));
            if (!sin) {
                free (ai);
                freeaddrinfo (head);
                return EAI_MEMORY;
            }
            sin->sin_family = AF_INET;
            sin->sin_port = htons ((unsigned short) port);
            sin->sin_addr = list[i].v4;
            ai->ai_addr = (struct sockaddr *) sin;
            ai->ai_addrlen = sizeof (*sin);
        } else {
            struct sockaddr_in6 *sin6 = calloc (1, sizeof (*sin6));
            if (!sin6) {
                free (ai);
                freeaddrinfo (head);
                return EAI_MEMORY;
            }
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons ((unsigned short) port);
            sin6->sin6_addr = list[i].v6;
            ai->ai_addr = (struct sockaddr *) sin6;
            ai->ai_addrlen = sizeof (*sin6);
        }

        if ((flags & AI_CANONNAME) && node && emitted == 0)
            ai->ai_canonname = dupstr (node);

        if (!head)
            head = tail = ai;
        else {
            tail->ai_next = ai;
            tail = ai;
        }
        emitted++;
    }

    if (emitted == 0)
        return EAI_NONAME;
    *res = head;
    return 0;
}

/* AF_UNIX: node is a filesystem path; service and IP protocols do not apply. */
static int unix_lookup (const char *node, int socktype, int flags,
                        struct addrinfo **res)
{
    struct addrinfo *ai;
    struct sockaddr_un *sun;
    size_t namelen;

    if (!node || !node[0])
        return EAI_NONAME;

    if (socktype != 0 && socktype != SOCK_STREAM && socktype != SOCK_DGRAM
        && socktype != SOCK_SEQPACKET)
        return EAI_SOCKTYPE;

    namelen = strlen (node);
    if (namelen >= sizeof (((struct sockaddr_un *) 0)->sun_path))
        return EAI_NONAME;

    ai = calloc (1, sizeof (*ai));
    if (!ai)
        return EAI_MEMORY;

    sun = calloc (1, sizeof (*sun));
    if (!sun) {
        free (ai);
        return EAI_MEMORY;
    }

    sun->sun_family = AF_UNIX;
    memcpy (sun->sun_path, node, namelen + 1);

    ai->ai_family = AF_UNIX;
    ai->ai_socktype = (socktype == 0) ? SOCK_STREAM : socktype;
    ai->ai_protocol = 0;
    ai->ai_flags = flags;
    ai->ai_addr = (struct sockaddr *) sun;
    ai->ai_addrlen = (socklen_t) (offsetof (struct sockaddr_un, sun_path)
                                  + namelen + 1);

    if (flags & AI_CANONNAME)
        ai->ai_canonname = dupstr (node);

    *res = ai;
    return 0;
}

int getaddrinfo (const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int family = AF_UNSPEC, socktype = 0, protocol = 0, flags = 0;
    int port = 0;
    int is_passive = 0;
    addr_entry_t list[MAX_ADDRS];
    int count = 0;
    struct in_addr a4;
    struct in6_addr a6;

    if (!res)
        return EAI_FAIL;
    *res = NULL;

    if (hints) {
        family = hints->ai_family;
        socktype = hints->ai_socktype;
        protocol = hints->ai_protocol;
        flags = hints->ai_flags;
        if (flags & AI_PASSIVE)
            is_passive = 1;
        if (family != AF_UNSPEC && family != AF_INET && family != AF_INET6
            && family != AF_UNIX)
            return EAI_FAMILY;
    }

    if (family == AF_UNIX)
        return unix_lookup (node, socktype, flags, res);

    if (socktype != 0 && socktype != SOCK_STREAM && socktype != SOCK_DGRAM)
        return EAI_SOCKTYPE;

    if ((flags & AI_NUMERICSERV) && service && service[0]
        && !isdigit ((unsigned char) service[0]))
        return EAI_NONAME;

    if (service_to_port (service, socktype, &port) < 0)
        return EAI_SERVICE;

    if (!node || !node[0]) {
        if (is_passive) {
            if (family == AF_UNSPEC || family == AF_INET) {
                struct in_addr z;
                z.s_addr = htonl (INADDR_ANY);
                collect_addr (list, &count, AF_INET, &z);
            }
            if (family == AF_UNSPEC || family == AF_INET6)
                collect_addr (list, &count, AF_INET6, &in6addr_any);
        } else {
            if (family == AF_UNSPEC || family == AF_INET) {
                struct in_addr lo;
                lo.s_addr = htonl (INADDR_LOOPBACK);
                collect_addr (list, &count, AF_INET, &lo);
            }
            if (family == AF_UNSPEC || family == AF_INET6)
                collect_addr (list, &count, AF_INET6, &in6addr_loopback);
        }
    } else if (inet_pton (AF_INET, node, &a4) == 1) {
        if (family == AF_UNSPEC || family == AF_INET)
            collect_addr (list, &count, AF_INET, &a4);
    } else if (inet_pton (AF_INET6, node, &a6) == 1) {
        if (family == AF_UNSPEC || family == AF_INET6)
            collect_addr (list, &count, AF_INET6, &a6);
    } else if (flags & AI_NUMERICHOST) {
        return EAI_NONAME;
    } else {
        if (hosts_lookup (node, family, list, &count) == 0)
            dns_lookup (node, family, list, &count);
    }

    if (count == 0)
        return EAI_NONAME;

    return build_result (list, count, family, socktype, protocol, flags,
                         node, port, res);
}

void freeaddrinfo (struct addrinfo *res)
{
    struct addrinfo *p = res;

    while (p) {
        struct addrinfo *next = p->ai_next;
        free (p->ai_addr);
        free (p->ai_canonname);
        free (p);
        p = next;
    }
}

#ifdef STANDALONE

/*
 * Self-test for the standalone getaddrinfo()/freeaddrinfo() substitute.
 *
 * Built only when -DSTANDALONE is defined (see tests.c and `make tests`).
 * Real domains are resolved and their returned address sets are checked
 * against the answers produced by /usr/bin/host.
 */

#define MAX_IPS 64

typedef struct {
    int             family;     /* AF_INET or AF_INET6 */
    struct in_addr  v4;
    struct in6_addr v6;
} tip_t;

static int g_pass, g_fail;

static void report (int ok, const char *what)
{
    if (ok) {
        g_pass++;
        printf ("ok   %s\n", what);
    } else {
        g_fail++;
        printf ("FAIL %s\n", what);
    }
}

static void tip_str (const tip_t * ip, char *buf, size_t len)
{
    if (ip->family == AF_INET)
        inet_ntop (AF_INET, &ip->v4, buf, len);
    else
        inet_ntop (AF_INET6, &ip->v6, buf, len);
}

/* Run /usr/bin/host -t <qtype> <name> and collect every IP it reports. */
static int host_collect (const char *qtype, const char *name, tip_t * out, int *n)
{
    char cmd[512], line[512];
    FILE *fp;
    int count = 0;

    snprintf (cmd, sizeof (cmd), "/usr/bin/host -t %s %s 2>/dev/null", qtype, name);
    fp = popen (cmd, "r");
    if (!fp)
        return -1;
    while (fgets (line, sizeof (line), fp)) {
        char *tok;
        for (tok = strtok (line, " \t\r\n"); tok; tok = strtok (NULL, " \t\r\n")) {
            struct in_addr a4;
            struct in6_addr a6;
            if (inet_pton (AF_INET, tok, &a4) == 1) {
                if (count < MAX_IPS) {
                    out[count].family = AF_INET;
                    out[count].v4 = a4;
                    count++;
                }
            } else if (inet_pton (AF_INET6, tok, &a6) == 1) {
                if (count < MAX_IPS) {
                    out[count].family = AF_INET6;
                    out[count].v6 = a6;
                    count++;
                }
            }
        }
    }
    pclose (fp);
    *n = count;
    return 0;
}

/* Resolve via our getaddrinfo and collect the addresses it returns. */
static int gai_collect (const char *node, const char *service, int family,
                        int flags, tip_t * out, int *n)
{
    struct addrinfo hints, *res, *p;
    int count = 0, rc;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = flags;

    rc = getaddrinfo (node, service, &hints, &res);
    if (rc != 0)
        return rc;
    for (p = res; p && count < MAX_IPS; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            out[count].family = AF_INET;
            out[count].v4 = ((struct sockaddr_in *) p->ai_addr)->sin_addr;
            count++;
        } else if (p->ai_family == AF_INET6) {
            out[count].family = AF_INET6;
            out[count].v6 = ((struct sockaddr_in6 *) p->ai_addr)->sin6_addr;
            count++;
        }
    }
    freeaddrinfo (res);
    *n = count;
    return 0;
}

static int tip_in (const tip_t * ip, const tip_t * list, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (list[i].family != ip->family)
            continue;
        if (ip->family == AF_INET && memcmp (&ip->v4, &list[i].v4, 4) == 0)
            return 1;
        if (ip->family == AF_INET6 && memcmp (&ip->v6, &list[i].v6, 16) == 0)
            return 1;
    }
    return 0;
}

static int tip_count (const tip_t * list, int n, int family)
{
    int i, c = 0;
    for (i = 0; i < n; i++)
        if (list[i].family == family)
            c++;
    return c;
}

static void test_domain (const char *name)
{
    tip_t gai[MAX_IPS], hv4[MAX_IPS], hv6[MAX_IPS];
    int ngai = 0, nv4 = 0, nv6 = 0, rc, i;
    char buf[INET6_ADDRSTRLEN], m[256];

    rc = gai_collect (name, NULL, AF_UNSPEC, 0, gai, &ngai);
    if (rc != 0) {
        snprintf (m, sizeof (m), "%s resolves (rc=%d)", name, rc);
        report (0, m);
        return;
    }
    host_collect ("A", name, hv4, &nv4);
    host_collect ("AAAA", name, hv6, &nv6);

    if (tip_count (gai, ngai, AF_INET) != nv4) {
        snprintf (m, sizeof (m), "%s A: got %d want %d",
                  name, tip_count (gai, ngai, AF_INET), nv4);
        report (0, m);
    }
    for (i = 0; i < nv4; i++)
        if (!tip_in (&hv4[i], gai, ngai)) {
            tip_str (&hv4[i], buf, sizeof (buf));
            snprintf (m, sizeof (m), "%s A: missing %s", name, buf);
            report (0, m);
        }

    if (tip_count (gai, ngai, AF_INET6) != nv6) {
        snprintf (m, sizeof (m), "%s AAAA: got %d want %d",
                  name, tip_count (gai, ngai, AF_INET6), nv6);
        report (0, m);
    }
    for (i = 0; i < nv6; i++)
        if (!tip_in (&hv6[i], gai, ngai)) {
            tip_str (&hv6[i], buf, sizeof (buf));
            snprintf (m, sizeof (m), "%s AAAA: missing %s", name, buf);
            report (0, m);
        }

    snprintf (m, sizeof (m), "%s: %d A + %d AAAA", name, nv4, nv6);
    report (1, m);
}

static void test_family (const char *name, int family)
{
    tip_t gai[MAX_IPS], hv[MAX_IPS];
    int ngai = 0, nhv = 0, rc, i;
    char buf[INET6_ADDRSTRLEN], m[256];

    rc = gai_collect (name, NULL, family, 0, gai, &ngai);
    if (rc != 0) {
        snprintf (m, sizeof (m), "%s family %d: rc=%d", name, family, rc);
        report (0, m);
        return;
    }
    host_collect (family == AF_INET ? "A" : "AAAA", name, hv, &nhv);

    for (i = 0; i < ngai; i++)
        if (gai[i].family != family) {
            snprintf (m, sizeof (m), "%s family %d: wrong family returned",
                      name, family);
            report (0, m);
            return;
        }
    if (ngai != nhv) {
        snprintf (m, sizeof (m), "%s family %d: got %d want %d",
                  name, family, ngai, nhv);
        report (0, m);
        return;
    }
    for (i = 0; i < nhv; i++)
        if (!tip_in (&hv[i], gai, ngai)) {
            tip_str (&hv[i], buf, sizeof (buf));
            snprintf (m, sizeof (m), "%s family %d: missing %s",
                      name, family, buf);
            report (0, m);
            return;
        }
    snprintf (m, sizeof (m), "%s family %d: %d addrs", name, family, ngai);
    report (1, m);
}

static void test_raw (const char *literal, int family)
{
    tip_t ips[MAX_IPS];
    int n = 0, rc;
    char m[256];

    rc = gai_collect (literal, NULL, AF_UNSPEC, AI_NUMERICHOST, ips, &n);
    if (rc != 0 || n != 1 || ips[0].family != family) {
        snprintf (m, sizeof (m), "raw %s: rc=%d n=%d", literal, rc, n);
        report (0, m);
        return;
    }
    snprintf (m, sizeof (m), "raw %s", literal);
    report (1, m);
}

static void test_raw_wrong_family (const char *literal, int family)
{
    tip_t ips[MAX_IPS];
    int n = 0, rc;
    char m[256];

    rc = gai_collect (literal, NULL, family, 0, ips, &n);
    snprintf (m, sizeof (m), "raw %s with family %d -> EAI_NONAME", literal, family);
    report (rc == EAI_NONAME, m);
}

int main (void)
{
    struct addrinfo hints, *res, *p;
    int rc;

    printf ("getaddrinfo standalone self-test\n\n");

    test_raw ("8.8.8.8", AF_INET);
    test_raw ("2001:4860:4860::8888", AF_INET6);
    test_raw_wrong_family ("8.8.8.8", AF_INET6);
    test_raw_wrong_family ("2001:4860:4860::8888", AF_INET);

    test_domain ("protobalance.com");
    test_domain ("ipv6.google.com");
    test_domain ("google.com");

    test_family ("google.com", AF_INET);
    test_family ("google.com", AF_INET6);

    /* service: numeric and /etc/services, resolved via /etc/hosts */
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo ("localhost", "http", &hints, &res);
    if (rc == 0) {
        int ok = 0;
        for (p = res; p; p = p->ai_next)
            if (p->ai_family == AF_INET
                && ((struct sockaddr_in *) p->ai_addr)->sin_port == htons (80))
                ok = 1;
        freeaddrinfo (res);
        report (ok, "localhost:http -> 127.0.0.1:80");
    } else
        report (0, "localhost:http resolution");

    rc = getaddrinfo ("localhost", "8080", &hints, &res);
    if (rc == 0) {
        int ok = 0;
        for (p = res; p; p = p->ai_next)
            if (p->ai_family == AF_INET
                && ((struct sockaddr_in *) p->ai_addr)->sin_port == htons (8080))
                ok = 1;
        freeaddrinfo (res);
        report (ok, "localhost:8080 numeric service");
    } else
        report (0, "localhost:8080 resolution");

    /* AI_PASSIVE wildcard */
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    rc = getaddrinfo (NULL, "80", &hints, &res);
    if (rc == 0) {
        int ok4 = 0, ok6 = 0;
        for (p = res; p; p = p->ai_next) {
            if (p->ai_family == AF_INET) {
                struct sockaddr_in *s = (struct sockaddr_in *) p->ai_addr;
                if (s->sin_addr.s_addr == 0 && s->sin_port == htons (80))
                    ok4 = 1;
            } else if (p->ai_family == AF_INET6) {
                struct sockaddr_in6 *s6 = (struct sockaddr_in6 *) p->ai_addr;
                if (IN6_IS_ADDR_UNSPECIFIED (&s6->sin6_addr)
                    && s6->sin6_port == htons (80))
                    ok6 = 1;
            }
        }
        freeaddrinfo (res);
        report (ok4 && ok6, "AI_PASSIVE wildcard 0.0.0.0 and ::");
    } else
        report (0, "AI_PASSIVE resolution");

    /* NXDOMAIN */
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo ("this-does-not-exist-xyz.invalid", NULL, &hints, &res);
    report (rc == EAI_NONAME, "NXDOMAIN -> EAI_NONAME");

    /* AF_UNIX */
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNIX;
    hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo ("/tmp/gai-test.sock", NULL, &hints, &res);
    if (rc == 0) {
        struct sockaddr_un *su = (struct sockaddr_un *) res->ai_addr;
        report (su->sun_family == AF_UNIX
                && strcmp (su->sun_path, "/tmp/gai-test.sock") == 0,
                "AF_UNIX path");
        freeaddrinfo (res);
    } else
        report (0, "AF_UNIX path");

    printf ("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}

#endif /* STANDALONE */

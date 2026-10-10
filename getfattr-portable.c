#include <stdio.h>
#include <string.h>
#include <errno.h>
#if defined(__FreeBSD__)
#include <sys/extattr.h>
#elif defined(__sun) || defined(__sun__)
#include <fcntl.h>
#include <unistd.h>
#else
#include <sys/xattr.h>
#endif

static int print_value (const char *path, const char *attr)
{
    char buf[4096];
    const char *shortname = attr;
    ssize_t n = -1;

    if (!strncmp (attr, "trusted.", 8))
        shortname = attr + 8;

#if defined(__FreeBSD__)
    n = extattr_get_link (path, EXTATTR_NAMESPACE_USER, shortname, buf, sizeof (buf));
#elif defined(__sun) || defined(__sun__)
    {
        int fd = attropen (path, shortname, O_RDONLY);
        if (fd >= 0) {
            n = read (fd, buf, sizeof (buf));
            close (fd);
        }
    }
#else
    n = lgetxattr (path, attr, buf, sizeof (buf));
#endif

    if (n < 0) {
        fprintf (stderr, "getfattr-portable: %s: %s: %s\n", path, attr, strerror (errno));
        return 1;
    }
    if (n > 0 && fwrite (buf, 1, n, stdout) != (size_t) n)
        return 1;
    return 0;
}

int main (int argc, char **argv)
{
    const char *attr = NULL;
    const char *path = NULL;
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp (argv[i], "-n") && i + 1 < argc) {
            attr = argv[++i];
        } else if (!strcmp (argv[i], "--only-values")) {
            ;
        } else if (argv[i][0] != '-') {
            path = argv[i];
        }
    }

    if (!attr || !path) {
        fprintf (stderr, "Usage: getfattr-portable -n <name> <path> [--only-values]\n");
        return 2;
    }

    return print_value (path, attr);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#if defined(__FreeBSD__)
#include <sys/extattr.h>
#elif defined(__sun) || defined(__sun__)
#include <fcntl.h>
#else
#include <sys/xattr.h>
#endif

int main (int argc, char **argv)
{
    int force = 0;
    const char *target, *linkpath;
    const char val[] = "1";

    if (argc < 2) {
        fprintf (stderr, "Usage: mkjunction [-f] <target> <linkpath>\n");
        return 1;
    }

    if (!strcmp (argv[1], "-f")) {
        if (argc != 4) {
            fprintf (stderr, "Usage: mkjunction [-f] <target> <linkpath>\n");
            return 1;
        }
        force = 1;
        target = argv[2];
        linkpath = argv[3];
    } else {
        if (argc != 3) {
            fprintf (stderr, "Usage: mkjunction [-f] <target> <linkpath>\n");
            return 1;
        }
        target = argv[1];
        linkpath = argv[2];
    }

    if (force)
        unlink (linkpath);

    if (symlink (target, linkpath) < 0) {
        fprintf (stderr, "mkjunction: %s: %s\n", linkpath, strerror (errno));
        return 1;
    }

#if defined(__FreeBSD__)
    if (extattr_set_link (linkpath, EXTATTR_NAMESPACE_USER, "windows.junction", val, 1) < 0)
        fprintf (stderr, "mkjunction: warning: could not set xattr on %s: %s\n", linkpath, strerror (errno));
#elif defined(__sun) || defined(__sun__)
    {
        int fd = attropen (linkpath, "windows.junction", O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            fprintf (stderr, "mkjunction: warning: could not set xattr on %s: %s\n", linkpath, strerror (errno));
            return 1;
        }
        if (write (fd, val, 1) != 1)
            fprintf (stderr, "mkjunction: warning: could not set xattr on %s: %s\n", linkpath, strerror (errno));
        close (fd);
    }
#else
    if (lsetxattr (linkpath, "trusted.windows.junction", val, 1, 0) < 0)
        fprintf (stderr, "mkjunction: warning: could not set xattr on %s: %s\n", linkpath, strerror (errno));
#endif

    return 0;
}

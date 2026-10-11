/*
 * Standalone getgrnam() substitution.
 *
 * Reads /etc/group directly and returns a struct group whose gr_gid comes
 * from the third colon-separated field.  This bypasses the NSS stack and
 * any shared libraries it may pull in, mirroring the plain-file lookups
 * used elsewhere in static-c-subst.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <grp.h>

/* Split a colon-separated line into fields, preserving empty fields
 * (strtok would collapse consecutive colons, which /etc/group allows). */
static int split_colons (char *line, char **f, int max)
{
    int n = 0;
    char *p = line;

    f[n++] = p;
    while (*p && n < max) {
        if (*p == ':') {
            *p = '\0';
            f[n++] = p + 1;
        }
        p++;
    }
    return n;
}

struct group *getgrnam (const char *name)
{
    static struct group gr;
    FILE *f;
    char line[1024];

    f = fopen ("/etc/group", "r");
    if (!f)
        return NULL;

    while (fgets (line, sizeof (line), f)) {
        char *fld[4];
        char *end;
        int n;
        unsigned long g;

        line[strcspn (line, "\r\n")] = '\0';
        n = split_colons (line, fld, 4);
        if (n < 3)              /* need through the gid field */
            continue;

        if (strcmp (fld[0], name) != 0)
            continue;

        g = strtoul (fld[2], &end, 10);
        if (*end != '\0')
            continue;

        gr.gr_gid = (gid_t) g;

        fclose (f);
        return &gr;
    }

    fclose (f);
    return NULL;
}

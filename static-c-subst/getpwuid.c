/*
 * Standalone getpwuid() substitution.
 *
 * Reads /etc/passwd directly and returns a struct passwd whose pw_name and
 * pw_dir come from the first and sixth colon-separated fields respectively.
 * This bypasses the NSS stack and any shared libraries it may pull in,
 * mirroring the plain-file lookups used elsewhere in static-c-subst.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

/* Split a colon-separated line into fields, preserving empty fields
 * (strtok would collapse consecutive colons, which /etc/passwd allows). */
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

struct passwd *getpwuid (uid_t uid)
{
    static struct passwd pw;
    static char name[256], dir[256];
    FILE *f;
    char line[1024];

    f = fopen ("/etc/passwd", "r");
    if (!f)
        return NULL;

    while (fgets (line, sizeof (line), f)) {
        char *fld[7];
        char *end;
        int n;
        unsigned long u;

        line[strcspn (line, "\r\n")] = '\0';
        n = split_colons (line, fld, 7);
        if (n < 6)              /* need through the home-directory field */
            continue;

        u = strtoul (fld[2], &end, 10);
        if (*end != '\0' || u != (unsigned long) uid)
            continue;

        snprintf (name, sizeof (name), "%s", fld[0]);
        snprintf (dir,  sizeof (dir),  "%s", fld[5]);

        pw.pw_name = name;
        pw.pw_uid  = (uid_t) u;
        pw.pw_dir  = dir;

        fclose (f);
        return &pw;
    }

    fclose (f);
    return NULL;
}

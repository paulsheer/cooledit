/* loadfile.c - loads a file into memory
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "my_string.h"


int readall (int fd, char *buf, int len);

/*Loads a file into memory */
/*Returns the size if the file in filelen and a pointer to the actual file
   which must be free'd. Returns NULL on error. */
/*The returned data is terminated by a null character.*/
char *loadfile (const char *filename, long *filelen)
{
    long filel;
    int file;
    struct stat info;
    char *data;

    if (!filelen)
	filelen = &filel;

    if (stat (filename, &info))
	return NULL;

    if (S_ISDIR (info.st_mode) || S_ISSOCK (info.st_mode)
	|| S_ISFIFO (info.st_mode) || S_ISCHR (info.st_mode)
	|| S_ISBLK (info.st_mode)) {
	return NULL;
    }
    *filelen = info.st_size;
    if ((data = malloc ((*filelen) + 2)) == NULL)
	return NULL;
    if ((file = open (filename, O_RDONLY)) < 0) {
	free (data);
	return NULL;
    }
    if (readall (file, data, *filelen) < *filelen) {
	close (file);
	free (data);
	return NULL;
    }
    data[*filelen] = 0;
    close (file);
    return data;
}


int savefile (const char *filename, const char *data, int len, int permissions)
{
    int file;
    int count = len;
    if ((file = creat (filename, permissions)) < 0)
	return -1;
    while (count > 0) {
	int bytes;
	bytes = write (file, data + (len - count), count);
	if (bytes == -1) {
	    close (file);
	    return -1;
	}
	count -= bytes;
    }
    return close (file);
}



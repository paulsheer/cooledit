#ifndef LOAD_FILE_H
#define LOAD_FILE_H

/* Loads a file into memory */
/* Returns the size if the file in filelen and a pointer to the actual file
   which must be free'd. Returns NULL on error. */
/* The returned data is terminated by a null character.*/
char *loadfile (const char *filename, long *filelen);

/* save a file, returns 0 on success, -1 on error: errno set for open or write */
int savefile (const char *filename, const char *data, int len, int permissions);

#endif

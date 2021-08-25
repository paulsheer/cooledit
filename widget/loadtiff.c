/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* loadtiff.c - loads a tiff file into memory
   Copyright (C) 1996-2022 Paul Sheer
 */


/* this file is highly dependent on a long int being 4 bytes */

/* NLS through this whole file ? */

#include "inspect.h"
#include <config.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <math.h>

#include "stringtools.h"
#include "app_glob.c"

#include "coolwidget.h"



#define TPRINTF tiffprintf
/*tiffprintf*/

/* #define ABORT_ON_ERROR */



short highbytefirst = 0;

short fgetshort(FILE *f)
{E_
if(highbytefirst)
    return (getc(f)<<8) + getc(f);
else
    return getc(f) + (getc(f) << 8);
}


long fgetlong (FILE * f)
{E_
    if (highbytefirst)
	return (getc (f) << 24) + (getc (f) << 16) + (getc (f) << 8) + getc (f);
    else
	return getc (f) + (getc (f) << 8) + (getc (f) << 16) + (getc (f) << 24);
}

/*
Gets an array of unsigned char at offset 'offset' from the beginning
of the file, of length 'length' bytes. It returns a pointer to the data
although the data is malloced from within the function, the function
checks on each call to free previous mallocs, and does so
if necessary. Therefore DO NOT FREE the pointer returned by this function.
Also copy the results before the next call to fgetstring.
*/

unsigned char *fgetstring (FILE * f, long offset, long length)
{E_
    static unsigned char *str = NULL;

    if (str)
	free (str);
    if ((str = malloc (length + 1)) == NULL) {
/* Not essential to translate */
	printf(_("Could not allocate memory in fgetstring.\n"));
	abort();
    }

    fseek (f, offset, SEEK_SET);
    fread (str, length, 1, f);
    str[length] = 0;

    return str;
}

void tiffprintf(const char *str, ...)
{E_

}


struct IFDentry {
    short tag;
    short fieldtype;
    long length;
    long valueoffset;
    short shortvalue;		/*this is duplicate of the lower numbered
				   two bytes of valueoffset, for when valueoffset contains a 
				   short value */
};


/*gets the value of an IFD entry. If the value is small enough
   to fit in the valueoffset it gets it from there.
   Handles correctly both arrays and single numbers */
void getvalue (FILE * fp, struct IFDentry *entry, long *retval, long numtoget, int size)
{E_
    int j;
    if (size == 2 || (entry->fieldtype == 3 && size == 0)) {
	TPRINTF (" (short) ");
	if (numtoget <= 2) {
	    retval[0] = entry->shortvalue;
	    if (numtoget == 2)
		if (highbytefirst)
		    retval[1] = entry->valueoffset && 0xFFFF;
		else
		    retval[1] = entry->valueoffset >> 16;
	} else {
	    fseek (fp, entry->valueoffset, SEEK_SET);
	    for (j = 0; j < numtoget; j++)
		retval[j] = fgetshort (fp);
	}
    }
    if (size == 4 || (entry->fieldtype == 4 && size == 0)) {
	TPRINTF (" (long) ");
	if (numtoget <= 1) {
	    retval[0] = entry->valueoffset;
	} else {
	    fseek (fp, entry->valueoffset, SEEK_SET);
	    for (j = 0; j < numtoget; j++)
		retval[j] = fgetlong (fp);
	}
    }
}





void tifferror (const char *errmessage)
{E_
  fprintf(stderr, errmessage); /* OR for the application:  */
#ifdef ABORT_ON_ERROR
abort();
#endif
/*    CError (errmessage);  */
/********/
}

/*
The return value must be free'd by the calling application
since the function malloc's it.

loads an uncompressed class G tiff file. This is a greyscale file.
Presently, this routine only supports, topdown orientation (Orientation = 1)
1 sample per pixel of 8 bits. It uses the greyresponse curve and assumes the
monitor to have a linear response.
It will interprete PhotometricInterpretation of 0 or 1 (reverse or normal).
It can handle multiple strips of any size and reads and displays most of the
important fields (although it only actually interprets class G fields).

rowstart and rowend are used to specify what part of the image to load
for very large files. If they go past the end of a file. The calling
application must check the size of the returned data by looking
at the height returned. To load the whole file,set rowstart = 0,
rowend = 2^31 - 1.
loadgreytiff returns contigous rows, 1 byte/pixel (0-255, black-white),
exclusive of rowend. Hence rowend-rowstart rows/scanlines are returned.
The first row/scanline is number zero, the last is numbered height - 1.
*/


unsigned char *loadgreytiff (const char *fname, long *width, long *height, long rowstart, long rowend, float gamma)
{E_

    FILE *fp = NULL;
    int i, j, c;
    long IFDoffset;
    struct IFDentry *theIFD = NULL;
    short numberofIFDentries;
    int numinterpreted;
    long *greyresponsecurve = NULL;
    long *stripbytecounts = NULL, *stripoffsets = NULL;
    long numstrips;
    float xresolution, yresolution;
    long bitspersample[3] =
    {8, 8, 8};
    long index;
    long maxgrey, mingrey;

    unsigned char *pic8 = NULL;
    unsigned char *pp = NULL;

    int numgreylevels = 0;	/*number of entries in the grey response curve array */

/*options: */
    int photointerp = 1, spp = 1, comptype = 1, fillorder = 1, planarconfig = 1,
     orient = 1;
    long rps = 0x7FFFFFFF;


    if ((fp = fopen (fname, "r")) == NULL) {
/* NLS ? */
	tifferror ("Cannot open tiff image file.\n");
	goto freeall;
    }

    if ((c = fgetshort (fp)) != 'I' + 256 * 'I')
	highbytefirst = 1;
    TPRINTF ("00; %c%c\n", (unsigned char) c, (unsigned char) c);
    if ((c = fgetshort (fp)) != 42) {
	tifferror ("Not a recognised (meaning of life) tiff file.\n");
	goto freeall;
    }
    TPRINTF ("02; %d\n", c);
    IFDoffset = fgetlong (fp);
    if (IFDoffset < 8) {
	tifferror ("Not a recognised tiff file.\n");
	goto freeall;
    }
    TPRINTF ("04; %ld\n", IFDoffset);

    if (fseek (fp, IFDoffset, SEEK_SET)) {
	tifferror ("Tiff IFD passed end of file.\n");
	goto freeall;
    }
/*now read the Image File Directory (IFD) */

    numberofIFDentries = fgetshort (fp);
    TPRINTF ("\nNumber IFD entries = %d\n", numberofIFDentries);

    if ((theIFD = malloc (numberofIFDentries * sizeof (struct IFDentry))) == NULL) {
	tifferror ("Cannot allocate memory for tiff file.\n");
	goto freeall;
    }
    for (i = 0; i < numberofIFDentries; i++) {
/*TPRINTF's are for debug */
/*      TPRINTF ("\n"); */
	theIFD[i].tag = fgetshort (fp);
/*      TPRINTF ("IFD %ld;%d\n", IFDoffset + 0 + 12 * i, theIFD[i].tag); */
	theIFD[i].fieldtype = fgetshort (fp);
/*      TPRINTF ("IFD %ld;%d\n", IFDoffset + 2 + 12 * i, theIFD[i].fieldtype); */
	theIFD[i].length = fgetlong (fp);
/*      TPRINTF ("IFD %ld;%ld\n", IFDoffset + 4 + 12 * i, theIFD[i].length); */
	theIFD[i].valueoffset = fgetlong (fp);
/*      TPRINTF ("IFD %ld;%ld\n", IFDoffset + 8 + 12 * i, theIFD[i].valueoffset); */
	if (!highbytefirst)
	    theIFD[i].shortvalue = (long) theIFD[i].valueoffset & 0xFFFF;
	else
	    theIFD[i].shortvalue = (long) theIFD[i].valueoffset >> 16;
    }


/*now loop through the IFD and check tags */

    TPRINTF ("\n");

    numinterpreted = numberofIFDentries;

    for (i = 0; i < numberofIFDentries; i++) {
	switch (theIFD[i].tag) {
	case 254:
	    TPRINTF ("NewSubfileType = %ld\n", theIFD[i].valueoffset);
	    break;
	case 256:
	    getvalue (fp, theIFD + i, width, 1, 0);
	    TPRINTF ("Image width = %ld\n", *width);
	    break;
	case 257:
	    getvalue (fp, theIFD + i, height, 1, 0);
	    TPRINTF ("Image length = %ld\n", *height);
	    break;
	case 258:
	    getvalue (fp, theIFD + i, bitspersample, theIFD[i].length, 2);
	    TPRINTF ("BitsPerSample\n");
	    for (j = 0; j < theIFD[i].length; j++)
		TPRINTF ("%ld ", bitspersample[j]);
	    TPRINTF ("\n");
	    break;
	case 259:
	    TPRINTF ("Compressing is type %d\n", theIFD[i].shortvalue);
	    comptype = theIFD[i].shortvalue;
	    break;
	case 262:
	    TPRINTF ("Photometric interpretation = %d\n", (int) theIFD[i].shortvalue);
	    photointerp = theIFD[i].shortvalue;
	    break;
	case 266:
	    TPRINTF ("Fillorder = %d\n", theIFD[i].shortvalue);
	    fillorder = theIFD[i].shortvalue;
	    break;
	case 269:
	    TPRINTF ("DocumentName: %s\n", fgetstring (fp, theIFD[i].valueoffset, theIFD[i].length));
	    break;
	case 270:
	    TPRINTF ("ImageDescription: %s\n", fgetstring (fp, theIFD[i].valueoffset, theIFD[i].length));
	    break;
	case 271:
	    TPRINTF ("Make: %s\n", fgetstring (fp, theIFD[i].valueoffset, theIFD[i].length));
	    break;
	case 272:
	    TPRINTF ("Model: %s\n", fgetstring (fp, theIFD[i].valueoffset, theIFD[i].length));
	    break;
	case 273:
	    numstrips = theIFD[i].length;

	    if ((stripoffsets = malloc (numstrips * sizeof (long))) == NULL) {
		tifferror ("Cannot allocate memory for tiff file.\n");
		goto freeall;
	    }
	    getvalue (fp, theIFD + i, stripoffsets, numstrips, 0);
	    TPRINTF ("StripOffsets: %ld\n", numstrips);
	    for (j = 0; j < numstrips; j++)
		TPRINTF ("%ld ", stripoffsets[j]);
	    TPRINTF ("\n");
	    break;
	case 274:
	    TPRINTF ("Orientation = %d\n", theIFD[i].shortvalue);
	    orient = theIFD[i].shortvalue;
	    break;
	case 277:
	    TPRINTF ("SamplesPerPixel = %d\n", theIFD[i].shortvalue);
	    spp = theIFD[i].shortvalue;
	    break;
	case 278:
	    getvalue (fp, theIFD + i, &rps, 1, 0);
	    TPRINTF ("RowsPerStrip = %ld\n", rps);
	    break;
	case 279:
	    numstrips = theIFD[i].length;
	    if ((stripbytecounts = malloc (numstrips * sizeof (long))) == NULL) {
		tifferror ("Cannot allocate memory for tiff file.\n");
		goto freeall;
	    }
	    getvalue (fp, theIFD + i, stripbytecounts, numstrips, 0);
	    TPRINTF ("StripByteCounts: %ld\n", numstrips);
	    for (j = 0; j < numstrips; j++)
		TPRINTF ("%ld ", stripbytecounts[j]);
	    TPRINTF ("\n");
	    break;
	case 282:
	    TPRINTF ("Xresolution at %ld\n", theIFD[i].valueoffset);
	    fseek (fp, theIFD[i].valueoffset, SEEK_SET);
	    xresolution = (float) fgetlong (fp) / fgetlong (fp);
	    TPRINTF ("  %f\n", xresolution);
	    break;
	case 283:
	    TPRINTF ("Yresolution at %ld\n", theIFD[i].valueoffset);
	    fseek (fp, theIFD[i].valueoffset, SEEK_SET);
	    yresolution = (float) fgetlong (fp) / fgetlong (fp);
	    TPRINTF ("  %f\n", yresolution);
	    break;
	case 284:
	    TPRINTF ("PlanarConfiguration = %d\n", theIFD[i].shortvalue);
	    planarconfig = theIFD[i].shortvalue;
	    break;
	case 285:
	    TPRINTF ("PageName: %s\n", fgetstring (fp, theIFD[i].valueoffset, theIFD[i].length));
	    break;
	case 290:
	    TPRINTF ("GrayResponseUnit = %d\n", theIFD[i].shortvalue);
	    break;
	case 291:
	    TPRINTF ("GrayResponseCurve at %ld, length %ld\n", theIFD[i].valueoffset, theIFD[i].length);
	    numgreylevels = theIFD[i].length;

	    if ((greyresponsecurve = malloc (numgreylevels * sizeof (long))) == NULL) {
abort();
		tifferror ("Cannot allocate memory for tiff file.\n");
		goto freeall;
	    }
	    getvalue (fp, theIFD + i, greyresponsecurve, numgreylevels, 2);
	    for (j = 0; j < numgreylevels; j++)
		TPRINTF ("%ld ", greyresponsecurve[j]);
	    TPRINTF ("\n");
	    break;
	case 296:
	    TPRINTF ("ResolutionUnit = %d\n", theIFD[i].shortvalue);
	    break;
	case 301:
	    TPRINTF ("Color response curve present at %ld, length %ld\n", theIFD[i].valueoffset, theIFD[i].length);
	    break;
	case 320:
	    TPRINTF ("Colormap present at %ld\n", theIFD[i].valueoffset);
	    break;
	default:
	    TPRINTF ("Tag %d unread.\n", theIFD[i].tag);
	    numinterpreted--;
	}
    }

    TPRINTF ("Number of fields interpreted = %d\n\n", numinterpreted);

/*now that we've read most of what we might want to know, we can't
   scratch all images that are too tedious to interpret. What we
   are looking for is a class G greyscale with no compression */

    if (spp != 1 || bitspersample[0] != 8 || comptype != 1 || fillorder != 1
	|| orient != 1) {
	tifferror ("This kind of tiff file is not supported.\n");
	goto freeall;
    }

/*printf("rowstart = %ld, rowend = %ld.\n", rowstart, rowend);
*/

/*check that rowstart and rowend are ok*/

    if(rowstart > rowend) {
	tifferror("Tiff called with rowstart > rowend.\n");
	goto freeall;
    }

    if(rowstart > *height) rowstart = *height; 

    if(rowstart < 0) rowstart = 0;

    if(rowend > *height) rowend = *height;

/*printf("width = %ld\n", *width);
printf("changeto rowstart = %ld, rowend = %ld.\n", rowstart, rowend);
*/

    if ((  pic8 = malloc ((rowend-rowstart) * *width + 1)  ) == NULL) {
	tifferror ("Cannot allocate memory for tiff file.\n");
	goto freeall;
    }

    index = 0;
    if(gamma == 0) gamma = 1.5;

    maxgrey = 0; mingrey = 1 << 30;
    if(numgreylevels) {
	for(i=0;i<numgreylevels;i++) {
	    maxgrey = max(maxgrey, greyresponsecurve[i]);
	    mingrey = min(mingrey, greyresponsecurve[i]);
	}
	for(i=0;i<numgreylevels;i++)
	    greyresponsecurve[i] = (double) 255 * pow((double) 1 - (double) ((double) greyresponsecurve[i] - mingrey) / (maxgrey - mingrey), (double) 1 / gamma);
    }

    if(rowend > rowstart)    
    for(i=rowstart;i<rowend;i++) {
		/*:loop through all rows*/

	pp = fgetstring (fp, stripoffsets[i/rps] + (i%rps) * *width, *width);
		/*:load the row*/

/*OR  pp = fdecodeLZW (fp, stripoffsets[i/rps] + (i%rps) * *width, *width); <-- for later*/

	if(numgreylevels) {
	    for(j=0;j<*width;j++)
		pic8[index++] = greyresponsecurve[pp[j]];
	} else {
	    if(photointerp == 1)
		for(j=0;j<*width;j++)
		    pic8[index++] = pp[j];
	    else
		for(j=0;j<*width;j++)
		    pic8[index++] = 255 - pp[j];
	}
    if(index > (rowend-rowstart) * *width) {
printf("Index past end of array\n\n");
abort();
}
    }

  freeall:


    if (greyresponsecurve)
	free (greyresponsecurve);
    if (stripbytecounts)
	free (stripbytecounts);
    if (stripoffsets)
	free (stripoffsets);
    if (theIFD)
	free (theIFD);

    if(fp) fclose(fp);

    return pic8;
}



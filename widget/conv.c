
/*
#!/usr/bin/python


import math

dens = 20

r = 1.8
s = 0.0
q = 1.8
# it is a coincidence that q are r are the same

for j in range(3):
    for i in xrange(5):
	e = 0.0
	for k in range(3):
	    for yi in range (dens):
		for xi in range (dens):
		    x = float (i - 2)               - 0.5 + float (xi + 0.5) / float (dens)
		    y = float ((j - 1) * 3 + k - 1) - 0.5 + float (yi + 0.5) / float (dens)
		    e = e + math.exp (-(x * x + y * y) / q)
	e = e / (dens * dens * 3) / r
	s = s + int (e * 255.0)
	print '#define G%d%d\t%d' % (i + 1, j + 1, int (e * 255.0))
    print
print s
*/

#if 0

#define G11     4
#define G21     8
#define G31     11
#define G41     8
#define G51     4

#define G12     21
#define G22     44
#define G32     56
#define G42     44
#define G52     21

#define G13     4
#define G23     8
#define G33     11
#define G43     8
#define G53     4

#elif 0

#define G11     1
#define G21     4
#define G31     6
#define G41     4
#define G51     1

#define G12     13
#define G22     55
#define G32     88
#define G42     55
#define G52     13

#define G13     1
#define G23     4
#define G33     6
#define G43     4
#define G53     1

#else

/* above two are too blured, this looks ok though: */

#define G11     0
#define G21     3
#define G31     6
#define G41     3
#define G51     0

#define G12     12
#define G22     57
#define G32     94
#define G42     57
#define G52     12

#define G13     0
#define G23     3
#define G33     6
#define G43     3
#define G53     0

#endif


#define S1M(x,y)   ((unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line]      )

#define S2M(x,y)  (((unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line] << 8 )  \
		+   (unsigned long) source[(x) * bytes_per_pixel + 1 + (y) * source_bytes_per_line]      )

#define S3M(x,y)  (((unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line] << 16)  \
		+  ((unsigned long) source[(x) * bytes_per_pixel + 1 + (y) * source_bytes_per_line] << 8 )  \
		+   (unsigned long) source[(x) * bytes_per_pixel + 2 + (y) * source_bytes_per_line])

#define S4M(x,y)  (((unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line] << 24)  \
		+  ((unsigned long) source[(x) * bytes_per_pixel + 1 + (y) * source_bytes_per_line] << 16)  \
		+  ((unsigned long) source[(x) * bytes_per_pixel + 2 + (y) * source_bytes_per_line] << 8 )  \
		+   (unsigned long) source[(x) * bytes_per_pixel + 3 + (y) * source_bytes_per_line]      )


#define S1L(x,y)   ((unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line]      )

#define S2L(x,y)  (((unsigned long) source[(x) * bytes_per_pixel + 1 + (y) * source_bytes_per_line] << 8 )  \
		+   (unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line]      )

#define S3L(x,y)  (((unsigned long) source[(x) * bytes_per_pixel + 2 + (y) * source_bytes_per_line] << 16)  \
		+  ((unsigned long) source[(x) * bytes_per_pixel + 1 + (y) * source_bytes_per_line] << 8 )  \
		+   (unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line]      )

#define S4L(x,y)  (((unsigned long) source[(x) * bytes_per_pixel + 3 + (y) * source_bytes_per_line] << 24)  \
		+  ((unsigned long) source[(x) * bytes_per_pixel + 2 + (y) * source_bytes_per_line] << 16)  \
		+  ((unsigned long) source[(x) * bytes_per_pixel + 1 + (y) * source_bytes_per_line] << 8 )  \
		+   (unsigned long) source[(x) * bytes_per_pixel +     (y) * source_bytes_per_line]      )

#define S1LR(x,y) ((S1L(x,y) >> red_shift) & red_mask)
#define S1LG(x,y) ((S1L(x,y) >> green_shift) & green_mask)
#define S1LB(x,y) ((S1L(x,y) >> blue_shift) & blue_mask)

#define S2LR(x,y) ((S2L(x,y) >> red_shift) & red_mask)
#define S2LG(x,y) ((S2L(x,y) >> green_shift) & green_mask)
#define S2LB(x,y) ((S2L(x,y) >> blue_shift) & blue_mask)

#define S3LR(x,y) ((S3L(x,y) >> red_shift) & red_mask)
#define S3LG(x,y) ((S3L(x,y) >> green_shift) & green_mask)
#define S3LB(x,y) ((S3L(x,y) >> blue_shift) & blue_mask)

#define S4LR(x,y) ((S4L(x,y) >> red_shift) & red_mask)
#define S4LG(x,y) ((S4L(x,y) >> green_shift) & green_mask)
#define S4LB(x,y) ((S4L(x,y) >> blue_shift) & blue_mask)


#define S1MR(x,y) ((S1M(x,y) >> red_shift) & red_mask)
#define S1MG(x,y) ((S1M(x,y) >> green_shift) & green_mask)
#define S1MB(x,y) ((S1M(x,y) >> blue_shift) & blue_mask)

#define S2MR(x,y) ((S2M(x,y) >> red_shift) & red_mask)
#define S2MG(x,y) ((S2M(x,y) >> green_shift) & green_mask)
#define S2MB(x,y) ((S2M(x,y) >> blue_shift) & blue_mask)

#define S3MR(x,y) ((S3M(x,y) >> red_shift) & red_mask)
#define S3MG(x,y) ((S3M(x,y) >> green_shift) & green_mask)
#define S3MB(x,y) ((S3M(x,y) >> blue_shift) & blue_mask)

#define S4MR(x,y) ((S4M(x,y) >> red_shift) & red_mask)
#define S4MG(x,y) ((S4M(x,y) >> green_shift) & green_mask)
#define S4MB(x,y) ((S4M(x,y) >> blue_shift) & blue_mask)

int n, m, f, g;

int G[3][5] = {
    {G11, G21, G31, G41, G51},
    {G12, G22, G32, G42, G52},
    {G13, G23, G33, G43, G53}
};

red = 0;
green = 0;
blue = 0;
if (byte_order == MSBFirst) {
    if (rgb_order == RedFirst) {
	switch (bytes_per_pixel) {
	case 1:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S1MR (f, g - 1) + S1MR (f, g) + S1MR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S1MG (f, g - 1) + S1MG (f, g) + S1MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S1MB (f, g - 1) + S1MB (f, g) + S1MB (f, g + 1)) * G[n][m]));
	    break;
	case 2:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S2MR (f, g - 1) + S2MR (f, g) + S2MR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S2MG (f, g - 1) + S2MG (f, g) + S2MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S2MB (f, g - 1) + S2MB (f, g) + S2MB (f, g + 1)) * G[n][m]));
	    break;
	case 3:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S3MR (f, g - 1) + S3MR (f, g) + S3MR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S3MG (f, g - 1) + S3MG (f, g) + S3MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S3MB (f, g - 1) + S3MB (f, g) + S3MB (f, g + 1)) * G[n][m]));
	    break;
	case 4:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S4MR (f, g - 1) + S4MR (f, g) + S4MR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S4MG (f, g - 1) + S4MG (f, g) + S4MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S4MB (f, g - 1) + S4MB (f, g) + S4MB (f, g + 1)) * G[n][m]));
	    break;
	}
    } else {
	switch (bytes_per_pixel) {
	case 1:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S1MB (f, g - 1) + S1MB (f, g) + S1MB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S1MG (f, g - 1) + S1MG (f, g) + S1MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S1MR (f, g - 1) + S1MR (f, g) + S1MR (f, g + 1)) * G[n][m]));
	    break;
	case 2:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S2MB (f, g - 1) + S2MB (f, g) + S2MB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S2MG (f, g - 1) + S2MG (f, g) + S2MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S2MR (f, g - 1) + S2MR (f, g) + S2MR (f, g + 1)) * G[n][m]));
	    break;
	case 3:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S3MB (f, g - 1) + S3MB (f, g) + S3MB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S3MG (f, g - 1) + S3MG (f, g) + S3MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S3MR (f, g - 1) + S3MR (f, g) + S3MR (f, g + 1)) * G[n][m]));
	    break;
	case 4:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S4MB (f, g - 1) + S4MB (f, g) + S4MB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S4MG (f, g - 1) + S4MG (f, g) + S4MG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S4MR (f, g - 1) + S4MR (f, g) + S4MR (f, g + 1)) * G[n][m]));
	    break;
	}
    }
} else {
    if (rgb_order == RedFirst) {
	switch (bytes_per_pixel) {
	case 1:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S1LR (f, g - 1) + S1LR (f, g) + S1LR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S1LG (f, g - 1) + S1LG (f, g) + S1LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S1LB (f, g - 1) + S1LB (f, g) + S1LB (f, g + 1)) * G[n][m]));
	    break;
	case 2:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S2LR (f, g - 1) + S2LR (f, g) + S2LR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S2LG (f, g - 1) + S2LG (f, g) + S2LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S2LB (f, g - 1) + S2LB (f, g) + S2LB (f, g + 1)) * G[n][m]));
	    break;
	case 3:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S3LR (f, g - 1) + S3LR (f, g) + S3LR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S3LG (f, g - 1) + S3LG (f, g) + S3LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S3LB (f, g - 1) + S3LB (f, g) + S3LB (f, g + 1)) * G[n][m]));
	    break;
	case 4:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S4LR (f, g - 1) + S4LR (f, g) + S4LR (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S4LG (f, g - 1) + S4LG (f, g) + S4LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S4LB (f, g - 1) + S4LB (f, g) + S4LB (f, g + 1)) * G[n][m]));
	    break;
	}
    } else {
	switch (bytes_per_pixel) {
	case 1:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S1LB (f, g - 1) + S1LB (f, g) + S1LB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S1LG (f, g - 1) + S1LG (f, g) + S1LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S1LR (f, g - 1) + S1LR (f, g) + S1LR (f, g + 1)) * G[n][m]));
	    break;
	case 2:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S2LB (f, g - 1) + S2LB (f, g) + S2LB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S2LG (f, g - 1) + S2LG (f, g) + S2LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S2LR (f, g - 1) + S2LR (f, g) + S2LR (f, g + 1)) * G[n][m]));
	    break;
	case 3:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S3LB (f, g - 1) + S3LB (f, g) + S3LB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S3LG (f, g - 1) + S3LG (f, g) + S3LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S3LR (f, g - 1) + S3LR (f, g) + S3LR (f, g + 1)) * G[n][m]));
	    break;
	case 4:
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    blue += (((S4LB (f, g - 1) + S4LB (f, g) + S4LB (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    green += (((S4LG (f, g - 1) + S4LG (f, g) + S4LG (f, g + 1)) * G[n][m]));
	    i++;
	    for (n = 0, g = j - 3; g <= j + 3; g += 3, n++)
		for (m = 0, f = i - 2; f <= i + 2; f++, m++)
		    red += (((S4LR (f, g - 1) + S4LR (f, g) + S4LR (f, g + 1)) * G[n][m]));
	    break;
	}
    }
}


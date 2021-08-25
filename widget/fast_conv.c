/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

if (byte_order == MSBFirst) {
    if (rgb_order == RedFirst) {
	switch (bytes_per_pixel) {
	case 1:
	    red =
		(((S1MR (i - 2, j - 4) + S1MR (i - 2, j - 3) + S1MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S1MR (i - 1, j - 4) + S1MR (i - 1, j - 3) + S1MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S1MR (i    , j - 4) + S1MR (i    , j - 3) + S1MR (i    , j - 2)) * G31) >> 8 +
		 ((S1MR (i + 1, j - 4) + S1MR (i + 1, j - 3) + S1MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S1MR (i + 2, j - 4) + S1MR (i + 2, j - 3) + S1MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S1MR (i - 2, j - 1) + S1MR (i - 2, j    ) + S1MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S1MR (i - 1, j - 1) + S1MR (i - 1, j    ) + S1MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S1MR (i    , j - 1) + S1MR (i    , j    ) + S1MR (i    , j + 1)) * G32) >> 8 +
		 ((S1MR (i + 1, j - 1) + S1MR (i + 1, j    ) + S1MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S1MR (i + 2, j - 1) + S1MR (i + 2, j    ) + S1MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S1MR (i - 2, j + 2) + S1MR (i - 2, j + 3) + S1MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S1MR (i - 1, j + 2) + S1MR (i - 1, j + 3) + S1MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S1MR (i    , j + 2) + S1MR (i    , j + 3) + S1MR (i    , j + 4)) * G33) >> 8 +
		 ((S1MR (i + 1, j + 2) + S1MR (i + 1, j + 3) + S1MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S1MR (i + 2, j + 2) + S1MR (i + 2, j + 3) + S1MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S1MG (i - 2, j - 4) + S1MG (i - 2, j - 3) + S1MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S1MG (i - 1, j - 4) + S1MG (i - 1, j - 3) + S1MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S1MG (i    , j - 4) + S1MG (i    , j - 3) + S1MG (i    , j - 2)) * G31) >> 8 +
		 ((S1MG (i + 1, j - 4) + S1MG (i + 1, j - 3) + S1MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S1MG (i + 2, j - 4) + S1MG (i + 2, j - 3) + S1MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S1MG (i - 2, j - 1) + S1MG (i - 2, j    ) + S1MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S1MG (i - 1, j - 1) + S1MG (i - 1, j    ) + S1MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S1MG (i    , j - 1) + S1MG (i    , j    ) + S1MG (i    , j + 1)) * G32) >> 8 +
		 ((S1MG (i + 1, j - 1) + S1MG (i + 1, j    ) + S1MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S1MG (i + 2, j - 1) + S1MG (i + 2, j    ) + S1MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S1MG (i - 2, j + 2) + S1MG (i - 2, j + 3) + S1MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S1MG (i - 1, j + 2) + S1MG (i - 1, j + 3) + S1MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S1MG (i    , j + 2) + S1MG (i    , j + 3) + S1MG (i    , j + 4)) * G33) >> 8 +
		 ((S1MG (i + 1, j + 2) + S1MG (i + 1, j + 3) + S1MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S1MG (i + 2, j + 2) + S1MG (i + 2, j + 3) + S1MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S1MB (i - 2, j - 4) + S1MB (i - 2, j - 3) + S1MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S1MB (i - 1, j - 4) + S1MB (i - 1, j - 3) + S1MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S1MB (i    , j - 4) + S1MB (i    , j - 3) + S1MB (i    , j - 2)) * G31) >> 8 +
		 ((S1MB (i + 1, j - 4) + S1MB (i + 1, j - 3) + S1MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S1MB (i + 2, j - 4) + S1MB (i + 2, j - 3) + S1MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S1MB (i - 2, j - 1) + S1MB (i - 2, j    ) + S1MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S1MB (i - 1, j - 1) + S1MB (i - 1, j    ) + S1MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S1MB (i    , j - 1) + S1MB (i    , j    ) + S1MB (i    , j + 1)) * G32) >> 8 +
		 ((S1MB (i + 1, j - 1) + S1MB (i + 1, j    ) + S1MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S1MB (i + 2, j - 1) + S1MB (i + 2, j    ) + S1MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S1MB (i - 2, j + 2) + S1MB (i - 2, j + 3) + S1MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S1MB (i - 1, j + 2) + S1MB (i - 1, j + 3) + S1MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S1MB (i    , j + 2) + S1MB (i    , j + 3) + S1MB (i    , j + 4)) * G33) >> 8 +
		 ((S1MB (i + 1, j + 2) + S1MB (i + 1, j + 3) + S1MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S1MB (i + 2, j + 2) + S1MB (i + 2, j + 3) + S1MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 2:
	    red =
		(((S2MR (i - 2, j - 4) + S2MR (i - 2, j - 3) + S2MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S2MR (i - 1, j - 4) + S2MR (i - 1, j - 3) + S2MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S2MR (i    , j - 4) + S2MR (i    , j - 3) + S2MR (i    , j - 2)) * G31) >> 8 +
		 ((S2MR (i + 1, j - 4) + S2MR (i + 1, j - 3) + S2MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S2MR (i + 2, j - 4) + S2MR (i + 2, j - 3) + S2MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S2MR (i - 2, j - 1) + S2MR (i - 2, j    ) + S2MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S2MR (i - 1, j - 1) + S2MR (i - 1, j    ) + S2MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S2MR (i    , j - 1) + S2MR (i    , j    ) + S2MR (i    , j + 1)) * G32) >> 8 +
		 ((S2MR (i + 1, j - 1) + S2MR (i + 1, j    ) + S2MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S2MR (i + 2, j - 1) + S2MR (i + 2, j    ) + S2MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S2MR (i - 2, j + 2) + S2MR (i - 2, j + 3) + S2MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S2MR (i - 1, j + 2) + S2MR (i - 1, j + 3) + S2MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S2MR (i    , j + 2) + S2MR (i    , j + 3) + S2MR (i    , j + 4)) * G33) >> 8 +
		 ((S2MR (i + 1, j + 2) + S2MR (i + 1, j + 3) + S2MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S2MR (i + 2, j + 2) + S2MR (i + 2, j + 3) + S2MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S2MG (i - 2, j - 4) + S2MG (i - 2, j - 3) + S2MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S2MG (i - 1, j - 4) + S2MG (i - 1, j - 3) + S2MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S2MG (i    , j - 4) + S2MG (i    , j - 3) + S2MG (i    , j - 2)) * G31) >> 8 +
		 ((S2MG (i + 1, j - 4) + S2MG (i + 1, j - 3) + S2MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S2MG (i + 2, j - 4) + S2MG (i + 2, j - 3) + S2MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S2MG (i - 2, j - 1) + S2MG (i - 2, j    ) + S2MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S2MG (i - 1, j - 1) + S2MG (i - 1, j    ) + S2MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S2MG (i    , j - 1) + S2MG (i    , j    ) + S2MG (i    , j + 1)) * G32) >> 8 +
		 ((S2MG (i + 1, j - 1) + S2MG (i + 1, j    ) + S2MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S2MG (i + 2, j - 1) + S2MG (i + 2, j    ) + S2MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S2MG (i - 2, j + 2) + S2MG (i - 2, j + 3) + S2MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S2MG (i - 1, j + 2) + S2MG (i - 1, j + 3) + S2MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S2MG (i    , j + 2) + S2MG (i    , j + 3) + S2MG (i    , j + 4)) * G33) >> 8 +
		 ((S2MG (i + 1, j + 2) + S2MG (i + 1, j + 3) + S2MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S2MG (i + 2, j + 2) + S2MG (i + 2, j + 3) + S2MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S2MB (i - 2, j - 4) + S2MB (i - 2, j - 3) + S2MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S2MB (i - 1, j - 4) + S2MB (i - 1, j - 3) + S2MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S2MB (i    , j - 4) + S2MB (i    , j - 3) + S2MB (i    , j - 2)) * G31) >> 8 +
		 ((S2MB (i + 1, j - 4) + S2MB (i + 1, j - 3) + S2MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S2MB (i + 2, j - 4) + S2MB (i + 2, j - 3) + S2MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S2MB (i - 2, j - 1) + S2MB (i - 2, j    ) + S2MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S2MB (i - 1, j - 1) + S2MB (i - 1, j    ) + S2MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S2MB (i    , j - 1) + S2MB (i    , j    ) + S2MB (i    , j + 1)) * G32) >> 8 +
		 ((S2MB (i + 1, j - 1) + S2MB (i + 1, j    ) + S2MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S2MB (i + 2, j - 1) + S2MB (i + 2, j    ) + S2MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S2MB (i - 2, j + 2) + S2MB (i - 2, j + 3) + S2MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S2MB (i - 1, j + 2) + S2MB (i - 1, j + 3) + S2MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S2MB (i    , j + 2) + S2MB (i    , j + 3) + S2MB (i    , j + 4)) * G33) >> 8 +
		 ((S2MB (i + 1, j + 2) + S2MB (i + 1, j + 3) + S2MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S2MB (i + 2, j + 2) + S2MB (i + 2, j + 3) + S2MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 3:
	    red =
		(((S3MR (i - 2, j - 4) + S3MR (i - 2, j - 3) + S3MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S3MR (i - 1, j - 4) + S3MR (i - 1, j - 3) + S3MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S3MR (i    , j - 4) + S3MR (i    , j - 3) + S3MR (i    , j - 2)) * G31) >> 8 +
		 ((S3MR (i + 1, j - 4) + S3MR (i + 1, j - 3) + S3MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S3MR (i + 2, j - 4) + S3MR (i + 2, j - 3) + S3MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S3MR (i - 2, j - 1) + S3MR (i - 2, j    ) + S3MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S3MR (i - 1, j - 1) + S3MR (i - 1, j    ) + S3MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S3MR (i    , j - 1) + S3MR (i    , j    ) + S3MR (i    , j + 1)) * G32) >> 8 +
		 ((S3MR (i + 1, j - 1) + S3MR (i + 1, j    ) + S3MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S3MR (i + 2, j - 1) + S3MR (i + 2, j    ) + S3MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S3MR (i - 2, j + 2) + S3MR (i - 2, j + 3) + S3MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S3MR (i - 1, j + 2) + S3MR (i - 1, j + 3) + S3MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S3MR (i    , j + 2) + S3MR (i    , j + 3) + S3MR (i    , j + 4)) * G33) >> 8 +
		 ((S3MR (i + 1, j + 2) + S3MR (i + 1, j + 3) + S3MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S3MR (i + 2, j + 2) + S3MR (i + 2, j + 3) + S3MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S3MG (i - 2, j - 4) + S3MG (i - 2, j - 3) + S3MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S3MG (i - 1, j - 4) + S3MG (i - 1, j - 3) + S3MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S3MG (i    , j - 4) + S3MG (i    , j - 3) + S3MG (i    , j - 2)) * G31) >> 8 +
		 ((S3MG (i + 1, j - 4) + S3MG (i + 1, j - 3) + S3MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S3MG (i + 2, j - 4) + S3MG (i + 2, j - 3) + S3MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S3MG (i - 2, j - 1) + S3MG (i - 2, j    ) + S3MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S3MG (i - 1, j - 1) + S3MG (i - 1, j    ) + S3MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S3MG (i    , j - 1) + S3MG (i    , j    ) + S3MG (i    , j + 1)) * G32) >> 8 +
		 ((S3MG (i + 1, j - 1) + S3MG (i + 1, j    ) + S3MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S3MG (i + 2, j - 1) + S3MG (i + 2, j    ) + S3MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S3MG (i - 2, j + 2) + S3MG (i - 2, j + 3) + S3MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S3MG (i - 1, j + 2) + S3MG (i - 1, j + 3) + S3MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S3MG (i    , j + 2) + S3MG (i    , j + 3) + S3MG (i    , j + 4)) * G33) >> 8 +
		 ((S3MG (i + 1, j + 2) + S3MG (i + 1, j + 3) + S3MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S3MG (i + 2, j + 2) + S3MG (i + 2, j + 3) + S3MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S3MB (i - 2, j - 4) + S3MB (i - 2, j - 3) + S3MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S3MB (i - 1, j - 4) + S3MB (i - 1, j - 3) + S3MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S3MB (i    , j - 4) + S3MB (i    , j - 3) + S3MB (i    , j - 2)) * G31) >> 8 +
		 ((S3MB (i + 1, j - 4) + S3MB (i + 1, j - 3) + S3MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S3MB (i + 2, j - 4) + S3MB (i + 2, j - 3) + S3MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S3MB (i - 2, j - 1) + S3MB (i - 2, j    ) + S3MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S3MB (i - 1, j - 1) + S3MB (i - 1, j    ) + S3MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S3MB (i    , j - 1) + S3MB (i    , j    ) + S3MB (i    , j + 1)) * G32) >> 8 +
		 ((S3MB (i + 1, j - 1) + S3MB (i + 1, j    ) + S3MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S3MB (i + 2, j - 1) + S3MB (i + 2, j    ) + S3MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S3MB (i - 2, j + 2) + S3MB (i - 2, j + 3) + S3MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S3MB (i - 1, j + 2) + S3MB (i - 1, j + 3) + S3MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S3MB (i    , j + 2) + S3MB (i    , j + 3) + S3MB (i    , j + 4)) * G33) >> 8 +
		 ((S3MB (i + 1, j + 2) + S3MB (i + 1, j + 3) + S3MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S3MB (i + 2, j + 2) + S3MB (i + 2, j + 3) + S3MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 4:
	    red =
		(((S4MR (i - 2, j - 4) + S4MR (i - 2, j - 3) + S4MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S4MR (i - 1, j - 4) + S4MR (i - 1, j - 3) + S4MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S4MR (i    , j - 4) + S4MR (i    , j - 3) + S4MR (i    , j - 2)) * G31) >> 8 +
		 ((S4MR (i + 1, j - 4) + S4MR (i + 1, j - 3) + S4MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S4MR (i + 2, j - 4) + S4MR (i + 2, j - 3) + S4MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S4MR (i - 2, j - 1) + S4MR (i - 2, j    ) + S4MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S4MR (i - 1, j - 1) + S4MR (i - 1, j    ) + S4MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S4MR (i    , j - 1) + S4MR (i    , j    ) + S4MR (i    , j + 1)) * G32) >> 8 +
		 ((S4MR (i + 1, j - 1) + S4MR (i + 1, j    ) + S4MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S4MR (i + 2, j - 1) + S4MR (i + 2, j    ) + S4MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S4MR (i - 2, j + 2) + S4MR (i - 2, j + 3) + S4MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S4MR (i - 1, j + 2) + S4MR (i - 1, j + 3) + S4MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S4MR (i    , j + 2) + S4MR (i    , j + 3) + S4MR (i    , j + 4)) * G33) >> 8 +
		 ((S4MR (i + 1, j + 2) + S4MR (i + 1, j + 3) + S4MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S4MR (i + 2, j + 2) + S4MR (i + 2, j + 3) + S4MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S4MG (i - 2, j - 4) + S4MG (i - 2, j - 3) + S4MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S4MG (i - 1, j - 4) + S4MG (i - 1, j - 3) + S4MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S4MG (i    , j - 4) + S4MG (i    , j - 3) + S4MG (i    , j - 2)) * G31) >> 8 +
		 ((S4MG (i + 1, j - 4) + S4MG (i + 1, j - 3) + S4MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S4MG (i + 2, j - 4) + S4MG (i + 2, j - 3) + S4MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S4MG (i - 2, j - 1) + S4MG (i - 2, j    ) + S4MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S4MG (i - 1, j - 1) + S4MG (i - 1, j    ) + S4MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S4MG (i    , j - 1) + S4MG (i    , j    ) + S4MG (i    , j + 1)) * G32) >> 8 +
		 ((S4MG (i + 1, j - 1) + S4MG (i + 1, j    ) + S4MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S4MG (i + 2, j - 1) + S4MG (i + 2, j    ) + S4MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S4MG (i - 2, j + 2) + S4MG (i - 2, j + 3) + S4MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S4MG (i - 1, j + 2) + S4MG (i - 1, j + 3) + S4MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S4MG (i    , j + 2) + S4MG (i    , j + 3) + S4MG (i    , j + 4)) * G33) >> 8 +
		 ((S4MG (i + 1, j + 2) + S4MG (i + 1, j + 3) + S4MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S4MG (i + 2, j + 2) + S4MG (i + 2, j + 3) + S4MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S4MB (i - 2, j - 4) + S4MB (i - 2, j - 3) + S4MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S4MB (i - 1, j - 4) + S4MB (i - 1, j - 3) + S4MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S4MB (i    , j - 4) + S4MB (i    , j - 3) + S4MB (i    , j - 2)) * G31) >> 8 +
		 ((S4MB (i + 1, j - 4) + S4MB (i + 1, j - 3) + S4MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S4MB (i + 2, j - 4) + S4MB (i + 2, j - 3) + S4MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S4MB (i - 2, j - 1) + S4MB (i - 2, j    ) + S4MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S4MB (i - 1, j - 1) + S4MB (i - 1, j    ) + S4MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S4MB (i    , j - 1) + S4MB (i    , j    ) + S4MB (i    , j + 1)) * G32) >> 8 +
		 ((S4MB (i + 1, j - 1) + S4MB (i + 1, j    ) + S4MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S4MB (i + 2, j - 1) + S4MB (i + 2, j    ) + S4MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S4MB (i - 2, j + 2) + S4MB (i - 2, j + 3) + S4MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S4MB (i - 1, j + 2) + S4MB (i - 1, j + 3) + S4MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S4MB (i    , j + 2) + S4MB (i    , j + 3) + S4MB (i    , j + 4)) * G33) >> 8 +
		 ((S4MB (i + 1, j + 2) + S4MB (i + 1, j + 3) + S4MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S4MB (i + 2, j + 2) + S4MB (i + 2, j + 3) + S4MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	}
    } else {
	switch (bytes_per_pixel) {
	case 1:
	    blue =
		(((S1MB (i - 2, j - 4) + S1MB (i - 2, j - 3) + S1MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S1MB (i - 1, j - 4) + S1MB (i - 1, j - 3) + S1MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S1MB (i    , j - 4) + S1MB (i    , j - 3) + S1MB (i    , j - 2)) * G31) >> 8 +
		 ((S1MB (i + 1, j - 4) + S1MB (i + 1, j - 3) + S1MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S1MB (i + 2, j - 4) + S1MB (i + 2, j - 3) + S1MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S1MB (i - 2, j - 1) + S1MB (i - 2, j    ) + S1MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S1MB (i - 1, j - 1) + S1MB (i - 1, j    ) + S1MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S1MB (i    , j - 1) + S1MB (i    , j    ) + S1MB (i    , j + 1)) * G32) >> 8 +
		 ((S1MB (i + 1, j - 1) + S1MB (i + 1, j    ) + S1MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S1MB (i + 2, j - 1) + S1MB (i + 2, j    ) + S1MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S1MB (i - 2, j + 2) + S1MB (i - 2, j + 3) + S1MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S1MB (i - 1, j + 2) + S1MB (i - 1, j + 3) + S1MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S1MB (i    , j + 2) + S1MB (i    , j + 3) + S1MB (i    , j + 4)) * G33) >> 8 +
		 ((S1MB (i + 1, j + 2) + S1MB (i + 1, j + 3) + S1MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S1MB (i + 2, j + 2) + S1MB (i + 2, j + 3) + S1MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S1MG (i - 2, j - 4) + S1MG (i - 2, j - 3) + S1MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S1MG (i - 1, j - 4) + S1MG (i - 1, j - 3) + S1MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S1MG (i    , j - 4) + S1MG (i    , j - 3) + S1MG (i    , j - 2)) * G31) >> 8 +
		 ((S1MG (i + 1, j - 4) + S1MG (i + 1, j - 3) + S1MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S1MG (i + 2, j - 4) + S1MG (i + 2, j - 3) + S1MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S1MG (i - 2, j - 1) + S1MG (i - 2, j    ) + S1MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S1MG (i - 1, j - 1) + S1MG (i - 1, j    ) + S1MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S1MG (i    , j - 1) + S1MG (i    , j    ) + S1MG (i    , j + 1)) * G32) >> 8 +
		 ((S1MG (i + 1, j - 1) + S1MG (i + 1, j    ) + S1MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S1MG (i + 2, j - 1) + S1MG (i + 2, j    ) + S1MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S1MG (i - 2, j + 2) + S1MG (i - 2, j + 3) + S1MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S1MG (i - 1, j + 2) + S1MG (i - 1, j + 3) + S1MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S1MG (i    , j + 2) + S1MG (i    , j + 3) + S1MG (i    , j + 4)) * G33) >> 8 +
		 ((S1MG (i + 1, j + 2) + S1MG (i + 1, j + 3) + S1MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S1MG (i + 2, j + 2) + S1MG (i + 2, j + 3) + S1MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S1MR (i - 2, j - 4) + S1MR (i - 2, j - 3) + S1MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S1MR (i - 1, j - 4) + S1MR (i - 1, j - 3) + S1MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S1MR (i    , j - 4) + S1MR (i    , j - 3) + S1MR (i    , j - 2)) * G31) >> 8 +
		 ((S1MR (i + 1, j - 4) + S1MR (i + 1, j - 3) + S1MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S1MR (i + 2, j - 4) + S1MR (i + 2, j - 3) + S1MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S1MR (i - 2, j - 1) + S1MR (i - 2, j    ) + S1MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S1MR (i - 1, j - 1) + S1MR (i - 1, j    ) + S1MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S1MR (i    , j - 1) + S1MR (i    , j    ) + S1MR (i    , j + 1)) * G32) >> 8 +
		 ((S1MR (i + 1, j - 1) + S1MR (i + 1, j    ) + S1MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S1MR (i + 2, j - 1) + S1MR (i + 2, j    ) + S1MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S1MR (i - 2, j + 2) + S1MR (i - 2, j + 3) + S1MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S1MR (i - 1, j + 2) + S1MR (i - 1, j + 3) + S1MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S1MR (i    , j + 2) + S1MR (i    , j + 3) + S1MR (i    , j + 4)) * G33) >> 8 +
		 ((S1MR (i + 1, j + 2) + S1MR (i + 1, j + 3) + S1MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S1MR (i + 2, j + 2) + S1MR (i + 2, j + 3) + S1MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 2:
	    blue =
		(((S2MB (i - 2, j - 4) + S2MB (i - 2, j - 3) + S2MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S2MB (i - 1, j - 4) + S2MB (i - 1, j - 3) + S2MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S2MB (i    , j - 4) + S2MB (i    , j - 3) + S2MB (i    , j - 2)) * G31) >> 8 +
		 ((S2MB (i + 1, j - 4) + S2MB (i + 1, j - 3) + S2MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S2MB (i + 2, j - 4) + S2MB (i + 2, j - 3) + S2MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S2MB (i - 2, j - 1) + S2MB (i - 2, j    ) + S2MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S2MB (i - 1, j - 1) + S2MB (i - 1, j    ) + S2MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S2MB (i    , j - 1) + S2MB (i    , j    ) + S2MB (i    , j + 1)) * G32) >> 8 +
		 ((S2MB (i + 1, j - 1) + S2MB (i + 1, j    ) + S2MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S2MB (i + 2, j - 1) + S2MB (i + 2, j    ) + S2MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S2MB (i - 2, j + 2) + S2MB (i - 2, j + 3) + S2MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S2MB (i - 1, j + 2) + S2MB (i - 1, j + 3) + S2MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S2MB (i    , j + 2) + S2MB (i    , j + 3) + S2MB (i    , j + 4)) * G33) >> 8 +
		 ((S2MB (i + 1, j + 2) + S2MB (i + 1, j + 3) + S2MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S2MB (i + 2, j + 2) + S2MB (i + 2, j + 3) + S2MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S2MG (i - 2, j - 4) + S2MG (i - 2, j - 3) + S2MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S2MG (i - 1, j - 4) + S2MG (i - 1, j - 3) + S2MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S2MG (i    , j - 4) + S2MG (i    , j - 3) + S2MG (i    , j - 2)) * G31) >> 8 +
		 ((S2MG (i + 1, j - 4) + S2MG (i + 1, j - 3) + S2MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S2MG (i + 2, j - 4) + S2MG (i + 2, j - 3) + S2MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S2MG (i - 2, j - 1) + S2MG (i - 2, j    ) + S2MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S2MG (i - 1, j - 1) + S2MG (i - 1, j    ) + S2MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S2MG (i    , j - 1) + S2MG (i    , j    ) + S2MG (i    , j + 1)) * G32) >> 8 +
		 ((S2MG (i + 1, j - 1) + S2MG (i + 1, j    ) + S2MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S2MG (i + 2, j - 1) + S2MG (i + 2, j    ) + S2MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S2MG (i - 2, j + 2) + S2MG (i - 2, j + 3) + S2MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S2MG (i - 1, j + 2) + S2MG (i - 1, j + 3) + S2MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S2MG (i    , j + 2) + S2MG (i    , j + 3) + S2MG (i    , j + 4)) * G33) >> 8 +
		 ((S2MG (i + 1, j + 2) + S2MG (i + 1, j + 3) + S2MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S2MG (i + 2, j + 2) + S2MG (i + 2, j + 3) + S2MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S2MR (i - 2, j - 4) + S2MR (i - 2, j - 3) + S2MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S2MR (i - 1, j - 4) + S2MR (i - 1, j - 3) + S2MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S2MR (i    , j - 4) + S2MR (i    , j - 3) + S2MR (i    , j - 2)) * G31) >> 8 +
		 ((S2MR (i + 1, j - 4) + S2MR (i + 1, j - 3) + S2MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S2MR (i + 2, j - 4) + S2MR (i + 2, j - 3) + S2MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S2MR (i - 2, j - 1) + S2MR (i - 2, j    ) + S2MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S2MR (i - 1, j - 1) + S2MR (i - 1, j    ) + S2MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S2MR (i    , j - 1) + S2MR (i    , j    ) + S2MR (i    , j + 1)) * G32) >> 8 +
		 ((S2MR (i + 1, j - 1) + S2MR (i + 1, j    ) + S2MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S2MR (i + 2, j - 1) + S2MR (i + 2, j    ) + S2MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S2MR (i - 2, j + 2) + S2MR (i - 2, j + 3) + S2MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S2MR (i - 1, j + 2) + S2MR (i - 1, j + 3) + S2MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S2MR (i    , j + 2) + S2MR (i    , j + 3) + S2MR (i    , j + 4)) * G33) >> 8 +
		 ((S2MR (i + 1, j + 2) + S2MR (i + 1, j + 3) + S2MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S2MR (i + 2, j + 2) + S2MR (i + 2, j + 3) + S2MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 3:
	    blue =
		(((S3MB (i - 2, j - 4) + S3MB (i - 2, j - 3) + S3MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S3MB (i - 1, j - 4) + S3MB (i - 1, j - 3) + S3MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S3MB (i    , j - 4) + S3MB (i    , j - 3) + S3MB (i    , j - 2)) * G31) >> 8 +
		 ((S3MB (i + 1, j - 4) + S3MB (i + 1, j - 3) + S3MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S3MB (i + 2, j - 4) + S3MB (i + 2, j - 3) + S3MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S3MB (i - 2, j - 1) + S3MB (i - 2, j    ) + S3MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S3MB (i - 1, j - 1) + S3MB (i - 1, j    ) + S3MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S3MB (i    , j - 1) + S3MB (i    , j    ) + S3MB (i    , j + 1)) * G32) >> 8 +
		 ((S3MB (i + 1, j - 1) + S3MB (i + 1, j    ) + S3MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S3MB (i + 2, j - 1) + S3MB (i + 2, j    ) + S3MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S3MB (i - 2, j + 2) + S3MB (i - 2, j + 3) + S3MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S3MB (i - 1, j + 2) + S3MB (i - 1, j + 3) + S3MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S3MB (i    , j + 2) + S3MB (i    , j + 3) + S3MB (i    , j + 4)) * G33) >> 8 +
		 ((S3MB (i + 1, j + 2) + S3MB (i + 1, j + 3) + S3MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S3MB (i + 2, j + 2) + S3MB (i + 2, j + 3) + S3MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S3MG (i - 2, j - 4) + S3MG (i - 2, j - 3) + S3MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S3MG (i - 1, j - 4) + S3MG (i - 1, j - 3) + S3MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S3MG (i    , j - 4) + S3MG (i    , j - 3) + S3MG (i    , j - 2)) * G31) >> 8 +
		 ((S3MG (i + 1, j - 4) + S3MG (i + 1, j - 3) + S3MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S3MG (i + 2, j - 4) + S3MG (i + 2, j - 3) + S3MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S3MG (i - 2, j - 1) + S3MG (i - 2, j    ) + S3MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S3MG (i - 1, j - 1) + S3MG (i - 1, j    ) + S3MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S3MG (i    , j - 1) + S3MG (i    , j    ) + S3MG (i    , j + 1)) * G32) >> 8 +
		 ((S3MG (i + 1, j - 1) + S3MG (i + 1, j    ) + S3MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S3MG (i + 2, j - 1) + S3MG (i + 2, j    ) + S3MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S3MG (i - 2, j + 2) + S3MG (i - 2, j + 3) + S3MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S3MG (i - 1, j + 2) + S3MG (i - 1, j + 3) + S3MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S3MG (i    , j + 2) + S3MG (i    , j + 3) + S3MG (i    , j + 4)) * G33) >> 8 +
		 ((S3MG (i + 1, j + 2) + S3MG (i + 1, j + 3) + S3MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S3MG (i + 2, j + 2) + S3MG (i + 2, j + 3) + S3MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S3MR (i - 2, j - 4) + S3MR (i - 2, j - 3) + S3MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S3MR (i - 1, j - 4) + S3MR (i - 1, j - 3) + S3MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S3MR (i    , j - 4) + S3MR (i    , j - 3) + S3MR (i    , j - 2)) * G31) >> 8 +
		 ((S3MR (i + 1, j - 4) + S3MR (i + 1, j - 3) + S3MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S3MR (i + 2, j - 4) + S3MR (i + 2, j - 3) + S3MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S3MR (i - 2, j - 1) + S3MR (i - 2, j    ) + S3MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S3MR (i - 1, j - 1) + S3MR (i - 1, j    ) + S3MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S3MR (i    , j - 1) + S3MR (i    , j    ) + S3MR (i    , j + 1)) * G32) >> 8 +
		 ((S3MR (i + 1, j - 1) + S3MR (i + 1, j    ) + S3MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S3MR (i + 2, j - 1) + S3MR (i + 2, j    ) + S3MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S3MR (i - 2, j + 2) + S3MR (i - 2, j + 3) + S3MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S3MR (i - 1, j + 2) + S3MR (i - 1, j + 3) + S3MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S3MR (i    , j + 2) + S3MR (i    , j + 3) + S3MR (i    , j + 4)) * G33) >> 8 +
		 ((S3MR (i + 1, j + 2) + S3MR (i + 1, j + 3) + S3MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S3MR (i + 2, j + 2) + S3MR (i + 2, j + 3) + S3MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 4:
	    blue =
		(((S4MB (i - 2, j - 4) + S4MB (i - 2, j - 3) + S4MB (i - 2, j - 2)) * G11) >> 8 +
		 ((S4MB (i - 1, j - 4) + S4MB (i - 1, j - 3) + S4MB (i - 1, j - 2)) * G21) >> 8 +
		 ((S4MB (i    , j - 4) + S4MB (i    , j - 3) + S4MB (i    , j - 2)) * G31) >> 8 +
		 ((S4MB (i + 1, j - 4) + S4MB (i + 1, j - 3) + S4MB (i + 1, j - 2)) * G41) >> 8 +
		 ((S4MB (i + 2, j - 4) + S4MB (i + 2, j - 3) + S4MB (i + 2, j - 2)) * G51) >> 8 +
		 ((S4MB (i - 2, j - 1) + S4MB (i - 2, j    ) + S4MB (i - 2, j + 1)) * G12) >> 8 +
		 ((S4MB (i - 1, j - 1) + S4MB (i - 1, j    ) + S4MB (i - 1, j + 1)) * G22) >> 8 +
		 ((S4MB (i    , j - 1) + S4MB (i    , j    ) + S4MB (i    , j + 1)) * G32) >> 8 +
		 ((S4MB (i + 1, j - 1) + S4MB (i + 1, j    ) + S4MB (i + 1, j + 1)) * G42) >> 8 +
		 ((S4MB (i + 2, j - 1) + S4MB (i + 2, j    ) + S4MB (i + 2, j + 1)) * G52) >> 8 +
		 ((S4MB (i - 2, j + 2) + S4MB (i - 2, j + 3) + S4MB (i - 2, j + 4)) * G13) >> 8 +
		 ((S4MB (i - 1, j + 2) + S4MB (i - 1, j + 3) + S4MB (i - 1, j + 4)) * G23) >> 8 +
		 ((S4MB (i    , j + 2) + S4MB (i    , j + 3) + S4MB (i    , j + 4)) * G33) >> 8 +
		 ((S4MB (i + 1, j + 2) + S4MB (i + 1, j + 3) + S4MB (i + 1, j + 4)) * G43) >> 8 +
		 ((S4MB (i + 2, j + 2) + S4MB (i + 2, j + 3) + S4MB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S4MG (i - 2, j - 4) + S4MG (i - 2, j - 3) + S4MG (i - 2, j - 2)) * G11) >> 8 +
		 ((S4MG (i - 1, j - 4) + S4MG (i - 1, j - 3) + S4MG (i - 1, j - 2)) * G21) >> 8 +
		 ((S4MG (i    , j - 4) + S4MG (i    , j - 3) + S4MG (i    , j - 2)) * G31) >> 8 +
		 ((S4MG (i + 1, j - 4) + S4MG (i + 1, j - 3) + S4MG (i + 1, j - 2)) * G41) >> 8 +
		 ((S4MG (i + 2, j - 4) + S4MG (i + 2, j - 3) + S4MG (i + 2, j - 2)) * G51) >> 8 +
		 ((S4MG (i - 2, j - 1) + S4MG (i - 2, j    ) + S4MG (i - 2, j + 1)) * G12) >> 8 +
		 ((S4MG (i - 1, j - 1) + S4MG (i - 1, j    ) + S4MG (i - 1, j + 1)) * G22) >> 8 +
		 ((S4MG (i    , j - 1) + S4MG (i    , j    ) + S4MG (i    , j + 1)) * G32) >> 8 +
		 ((S4MG (i + 1, j - 1) + S4MG (i + 1, j    ) + S4MG (i + 1, j + 1)) * G42) >> 8 +
		 ((S4MG (i + 2, j - 1) + S4MG (i + 2, j    ) + S4MG (i + 2, j + 1)) * G52) >> 8 +
		 ((S4MG (i - 2, j + 2) + S4MG (i - 2, j + 3) + S4MG (i - 2, j + 4)) * G13) >> 8 +
		 ((S4MG (i - 1, j + 2) + S4MG (i - 1, j + 3) + S4MG (i - 1, j + 4)) * G23) >> 8 +
		 ((S4MG (i    , j + 2) + S4MG (i    , j + 3) + S4MG (i    , j + 4)) * G33) >> 8 +
		 ((S4MG (i + 1, j + 2) + S4MG (i + 1, j + 3) + S4MG (i + 1, j + 4)) * G43) >> 8 +
		 ((S4MG (i + 2, j + 2) + S4MG (i + 2, j + 3) + S4MG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S4MR (i - 2, j - 4) + S4MR (i - 2, j - 3) + S4MR (i - 2, j - 2)) * G11) >> 8 +
		 ((S4MR (i - 1, j - 4) + S4MR (i - 1, j - 3) + S4MR (i - 1, j - 2)) * G21) >> 8 +
		 ((S4MR (i    , j - 4) + S4MR (i    , j - 3) + S4MR (i    , j - 2)) * G31) >> 8 +
		 ((S4MR (i + 1, j - 4) + S4MR (i + 1, j - 3) + S4MR (i + 1, j - 2)) * G41) >> 8 +
		 ((S4MR (i + 2, j - 4) + S4MR (i + 2, j - 3) + S4MR (i + 2, j - 2)) * G51) >> 8 +
		 ((S4MR (i - 2, j - 1) + S4MR (i - 2, j    ) + S4MR (i - 2, j + 1)) * G12) >> 8 +
		 ((S4MR (i - 1, j - 1) + S4MR (i - 1, j    ) + S4MR (i - 1, j + 1)) * G22) >> 8 +
		 ((S4MR (i    , j - 1) + S4MR (i    , j    ) + S4MR (i    , j + 1)) * G32) >> 8 +
		 ((S4MR (i + 1, j - 1) + S4MR (i + 1, j    ) + S4MR (i + 1, j + 1)) * G42) >> 8 +
		 ((S4MR (i + 2, j - 1) + S4MR (i + 2, j    ) + S4MR (i + 2, j + 1)) * G52) >> 8 +
		 ((S4MR (i - 2, j + 2) + S4MR (i - 2, j + 3) + S4MR (i - 2, j + 4)) * G13) >> 8 +
		 ((S4MR (i - 1, j + 2) + S4MR (i - 1, j + 3) + S4MR (i - 1, j + 4)) * G23) >> 8 +
		 ((S4MR (i    , j + 2) + S4MR (i    , j + 3) + S4MR (i    , j + 4)) * G33) >> 8 +
		 ((S4MR (i + 1, j + 2) + S4MR (i + 1, j + 3) + S4MR (i + 1, j + 4)) * G43) >> 8 +
		 ((S4MR (i + 2, j + 2) + S4MR (i + 2, j + 3) + S4MR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	}
    }
} else {
    if (rgb_order == RedFirst) {
	switch (bytes_per_pixel) {
	case 1:
	    red =
		(((S1LR (i - 2, j - 4) + S1LR (i - 2, j - 3) + S1LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S1LR (i - 1, j - 4) + S1LR (i - 1, j - 3) + S1LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S1LR (i    , j - 4) + S1LR (i    , j - 3) + S1LR (i    , j - 2)) * G31) >> 8 +
		 ((S1LR (i + 1, j - 4) + S1LR (i + 1, j - 3) + S1LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S1LR (i + 2, j - 4) + S1LR (i + 2, j - 3) + S1LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S1LR (i - 2, j - 1) + S1LR (i - 2, j    ) + S1LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S1LR (i - 1, j - 1) + S1LR (i - 1, j    ) + S1LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S1LR (i    , j - 1) + S1LR (i    , j    ) + S1LR (i    , j + 1)) * G32) >> 8 +
		 ((S1LR (i + 1, j - 1) + S1LR (i + 1, j    ) + S1LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S1LR (i + 2, j - 1) + S1LR (i + 2, j    ) + S1LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S1LR (i - 2, j + 2) + S1LR (i - 2, j + 3) + S1LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S1LR (i - 1, j + 2) + S1LR (i - 1, j + 3) + S1LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S1LR (i    , j + 2) + S1LR (i    , j + 3) + S1LR (i    , j + 4)) * G33) >> 8 +
		 ((S1LR (i + 1, j + 2) + S1LR (i + 1, j + 3) + S1LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S1LR (i + 2, j + 2) + S1LR (i + 2, j + 3) + S1LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S1LG (i - 2, j - 4) + S1LG (i - 2, j - 3) + S1LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S1LG (i - 1, j - 4) + S1LG (i - 1, j - 3) + S1LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S1LG (i    , j - 4) + S1LG (i    , j - 3) + S1LG (i    , j - 2)) * G31) >> 8 +
		 ((S1LG (i + 1, j - 4) + S1LG (i + 1, j - 3) + S1LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S1LG (i + 2, j - 4) + S1LG (i + 2, j - 3) + S1LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S1LG (i - 2, j - 1) + S1LG (i - 2, j    ) + S1LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S1LG (i - 1, j - 1) + S1LG (i - 1, j    ) + S1LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S1LG (i    , j - 1) + S1LG (i    , j    ) + S1LG (i    , j + 1)) * G32) >> 8 +
		 ((S1LG (i + 1, j - 1) + S1LG (i + 1, j    ) + S1LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S1LG (i + 2, j - 1) + S1LG (i + 2, j    ) + S1LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S1LG (i - 2, j + 2) + S1LG (i - 2, j + 3) + S1LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S1LG (i - 1, j + 2) + S1LG (i - 1, j + 3) + S1LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S1LG (i    , j + 2) + S1LG (i    , j + 3) + S1LG (i    , j + 4)) * G33) >> 8 +
		 ((S1LG (i + 1, j + 2) + S1LG (i + 1, j + 3) + S1LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S1LG (i + 2, j + 2) + S1LG (i + 2, j + 3) + S1LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S1LB (i - 2, j - 4) + S1LB (i - 2, j - 3) + S1LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S1LB (i - 1, j - 4) + S1LB (i - 1, j - 3) + S1LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S1LB (i    , j - 4) + S1LB (i    , j - 3) + S1LB (i    , j - 2)) * G31) >> 8 +
		 ((S1LB (i + 1, j - 4) + S1LB (i + 1, j - 3) + S1LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S1LB (i + 2, j - 4) + S1LB (i + 2, j - 3) + S1LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S1LB (i - 2, j - 1) + S1LB (i - 2, j    ) + S1LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S1LB (i - 1, j - 1) + S1LB (i - 1, j    ) + S1LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S1LB (i    , j - 1) + S1LB (i    , j    ) + S1LB (i    , j + 1)) * G32) >> 8 +
		 ((S1LB (i + 1, j - 1) + S1LB (i + 1, j    ) + S1LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S1LB (i + 2, j - 1) + S1LB (i + 2, j    ) + S1LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S1LB (i - 2, j + 2) + S1LB (i - 2, j + 3) + S1LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S1LB (i - 1, j + 2) + S1LB (i - 1, j + 3) + S1LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S1LB (i    , j + 2) + S1LB (i    , j + 3) + S1LB (i    , j + 4)) * G33) >> 8 +
		 ((S1LB (i + 1, j + 2) + S1LB (i + 1, j + 3) + S1LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S1LB (i + 2, j + 2) + S1LB (i + 2, j + 3) + S1LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 2:
	    red =
		(((S2LR (i - 2, j - 4) + S2LR (i - 2, j - 3) + S2LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S2LR (i - 1, j - 4) + S2LR (i - 1, j - 3) + S2LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S2LR (i    , j - 4) + S2LR (i    , j - 3) + S2LR (i    , j - 2)) * G31) >> 8 +
		 ((S2LR (i + 1, j - 4) + S2LR (i + 1, j - 3) + S2LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S2LR (i + 2, j - 4) + S2LR (i + 2, j - 3) + S2LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S2LR (i - 2, j - 1) + S2LR (i - 2, j    ) + S2LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S2LR (i - 1, j - 1) + S2LR (i - 1, j    ) + S2LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S2LR (i    , j - 1) + S2LR (i    , j    ) + S2LR (i    , j + 1)) * G32) >> 8 +
		 ((S2LR (i + 1, j - 1) + S2LR (i + 1, j    ) + S2LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S2LR (i + 2, j - 1) + S2LR (i + 2, j    ) + S2LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S2LR (i - 2, j + 2) + S2LR (i - 2, j + 3) + S2LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S2LR (i - 1, j + 2) + S2LR (i - 1, j + 3) + S2LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S2LR (i    , j + 2) + S2LR (i    , j + 3) + S2LR (i    , j + 4)) * G33) >> 8 +
		 ((S2LR (i + 1, j + 2) + S2LR (i + 1, j + 3) + S2LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S2LR (i + 2, j + 2) + S2LR (i + 2, j + 3) + S2LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S2LG (i - 2, j - 4) + S2LG (i - 2, j - 3) + S2LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S2LG (i - 1, j - 4) + S2LG (i - 1, j - 3) + S2LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S2LG (i    , j - 4) + S2LG (i    , j - 3) + S2LG (i    , j - 2)) * G31) >> 8 +
		 ((S2LG (i + 1, j - 4) + S2LG (i + 1, j - 3) + S2LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S2LG (i + 2, j - 4) + S2LG (i + 2, j - 3) + S2LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S2LG (i - 2, j - 1) + S2LG (i - 2, j    ) + S2LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S2LG (i - 1, j - 1) + S2LG (i - 1, j    ) + S2LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S2LG (i    , j - 1) + S2LG (i    , j    ) + S2LG (i    , j + 1)) * G32) >> 8 +
		 ((S2LG (i + 1, j - 1) + S2LG (i + 1, j    ) + S2LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S2LG (i + 2, j - 1) + S2LG (i + 2, j    ) + S2LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S2LG (i - 2, j + 2) + S2LG (i - 2, j + 3) + S2LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S2LG (i - 1, j + 2) + S2LG (i - 1, j + 3) + S2LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S2LG (i    , j + 2) + S2LG (i    , j + 3) + S2LG (i    , j + 4)) * G33) >> 8 +
		 ((S2LG (i + 1, j + 2) + S2LG (i + 1, j + 3) + S2LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S2LG (i + 2, j + 2) + S2LG (i + 2, j + 3) + S2LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S2LB (i - 2, j - 4) + S2LB (i - 2, j - 3) + S2LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S2LB (i - 1, j - 4) + S2LB (i - 1, j - 3) + S2LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S2LB (i    , j - 4) + S2LB (i    , j - 3) + S2LB (i    , j - 2)) * G31) >> 8 +
		 ((S2LB (i + 1, j - 4) + S2LB (i + 1, j - 3) + S2LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S2LB (i + 2, j - 4) + S2LB (i + 2, j - 3) + S2LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S2LB (i - 2, j - 1) + S2LB (i - 2, j    ) + S2LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S2LB (i - 1, j - 1) + S2LB (i - 1, j    ) + S2LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S2LB (i    , j - 1) + S2LB (i    , j    ) + S2LB (i    , j + 1)) * G32) >> 8 +
		 ((S2LB (i + 1, j - 1) + S2LB (i + 1, j    ) + S2LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S2LB (i + 2, j - 1) + S2LB (i + 2, j    ) + S2LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S2LB (i - 2, j + 2) + S2LB (i - 2, j + 3) + S2LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S2LB (i - 1, j + 2) + S2LB (i - 1, j + 3) + S2LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S2LB (i    , j + 2) + S2LB (i    , j + 3) + S2LB (i    , j + 4)) * G33) >> 8 +
		 ((S2LB (i + 1, j + 2) + S2LB (i + 1, j + 3) + S2LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S2LB (i + 2, j + 2) + S2LB (i + 2, j + 3) + S2LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 3:
	    red =
		(((S3LR (i - 2, j - 4) + S3LR (i - 2, j - 3) + S3LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S3LR (i - 1, j - 4) + S3LR (i - 1, j - 3) + S3LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S3LR (i    , j - 4) + S3LR (i    , j - 3) + S3LR (i    , j - 2)) * G31) >> 8 +
		 ((S3LR (i + 1, j - 4) + S3LR (i + 1, j - 3) + S3LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S3LR (i + 2, j - 4) + S3LR (i + 2, j - 3) + S3LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S3LR (i - 2, j - 1) + S3LR (i - 2, j    ) + S3LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S3LR (i - 1, j - 1) + S3LR (i - 1, j    ) + S3LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S3LR (i    , j - 1) + S3LR (i    , j    ) + S3LR (i    , j + 1)) * G32) >> 8 +
		 ((S3LR (i + 1, j - 1) + S3LR (i + 1, j    ) + S3LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S3LR (i + 2, j - 1) + S3LR (i + 2, j    ) + S3LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S3LR (i - 2, j + 2) + S3LR (i - 2, j + 3) + S3LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S3LR (i - 1, j + 2) + S3LR (i - 1, j + 3) + S3LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S3LR (i    , j + 2) + S3LR (i    , j + 3) + S3LR (i    , j + 4)) * G33) >> 8 +
		 ((S3LR (i + 1, j + 2) + S3LR (i + 1, j + 3) + S3LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S3LR (i + 2, j + 2) + S3LR (i + 2, j + 3) + S3LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S3LG (i - 2, j - 4) + S3LG (i - 2, j - 3) + S3LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S3LG (i - 1, j - 4) + S3LG (i - 1, j - 3) + S3LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S3LG (i    , j - 4) + S3LG (i    , j - 3) + S3LG (i    , j - 2)) * G31) >> 8 +
		 ((S3LG (i + 1, j - 4) + S3LG (i + 1, j - 3) + S3LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S3LG (i + 2, j - 4) + S3LG (i + 2, j - 3) + S3LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S3LG (i - 2, j - 1) + S3LG (i - 2, j    ) + S3LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S3LG (i - 1, j - 1) + S3LG (i - 1, j    ) + S3LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S3LG (i    , j - 1) + S3LG (i    , j    ) + S3LG (i    , j + 1)) * G32) >> 8 +
		 ((S3LG (i + 1, j - 1) + S3LG (i + 1, j    ) + S3LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S3LG (i + 2, j - 1) + S3LG (i + 2, j    ) + S3LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S3LG (i - 2, j + 2) + S3LG (i - 2, j + 3) + S3LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S3LG (i - 1, j + 2) + S3LG (i - 1, j + 3) + S3LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S3LG (i    , j + 2) + S3LG (i    , j + 3) + S3LG (i    , j + 4)) * G33) >> 8 +
		 ((S3LG (i + 1, j + 2) + S3LG (i + 1, j + 3) + S3LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S3LG (i + 2, j + 2) + S3LG (i + 2, j + 3) + S3LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S3LB (i - 2, j - 4) + S3LB (i - 2, j - 3) + S3LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S3LB (i - 1, j - 4) + S3LB (i - 1, j - 3) + S3LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S3LB (i    , j - 4) + S3LB (i    , j - 3) + S3LB (i    , j - 2)) * G31) >> 8 +
		 ((S3LB (i + 1, j - 4) + S3LB (i + 1, j - 3) + S3LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S3LB (i + 2, j - 4) + S3LB (i + 2, j - 3) + S3LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S3LB (i - 2, j - 1) + S3LB (i - 2, j    ) + S3LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S3LB (i - 1, j - 1) + S3LB (i - 1, j    ) + S3LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S3LB (i    , j - 1) + S3LB (i    , j    ) + S3LB (i    , j + 1)) * G32) >> 8 +
		 ((S3LB (i + 1, j - 1) + S3LB (i + 1, j    ) + S3LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S3LB (i + 2, j - 1) + S3LB (i + 2, j    ) + S3LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S3LB (i - 2, j + 2) + S3LB (i - 2, j + 3) + S3LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S3LB (i - 1, j + 2) + S3LB (i - 1, j + 3) + S3LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S3LB (i    , j + 2) + S3LB (i    , j + 3) + S3LB (i    , j + 4)) * G33) >> 8 +
		 ((S3LB (i + 1, j + 2) + S3LB (i + 1, j + 3) + S3LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S3LB (i + 2, j + 2) + S3LB (i + 2, j + 3) + S3LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 4:
	    red =
		(((S4LR (i - 2, j - 4) + S4LR (i - 2, j - 3) + S4LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S4LR (i - 1, j - 4) + S4LR (i - 1, j - 3) + S4LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S4LR (i    , j - 4) + S4LR (i    , j - 3) + S4LR (i    , j - 2)) * G31) >> 8 +
		 ((S4LR (i + 1, j - 4) + S4LR (i + 1, j - 3) + S4LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S4LR (i + 2, j - 4) + S4LR (i + 2, j - 3) + S4LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S4LR (i - 2, j - 1) + S4LR (i - 2, j    ) + S4LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S4LR (i - 1, j - 1) + S4LR (i - 1, j    ) + S4LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S4LR (i    , j - 1) + S4LR (i    , j    ) + S4LR (i    , j + 1)) * G32) >> 8 +
		 ((S4LR (i + 1, j - 1) + S4LR (i + 1, j    ) + S4LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S4LR (i + 2, j - 1) + S4LR (i + 2, j    ) + S4LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S4LR (i - 2, j + 2) + S4LR (i - 2, j + 3) + S4LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S4LR (i - 1, j + 2) + S4LR (i - 1, j + 3) + S4LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S4LR (i    , j + 2) + S4LR (i    , j + 3) + S4LR (i    , j + 4)) * G33) >> 8 +
		 ((S4LR (i + 1, j + 2) + S4LR (i + 1, j + 3) + S4LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S4LR (i + 2, j + 2) + S4LR (i + 2, j + 3) + S4LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S4LG (i - 2, j - 4) + S4LG (i - 2, j - 3) + S4LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S4LG (i - 1, j - 4) + S4LG (i - 1, j - 3) + S4LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S4LG (i    , j - 4) + S4LG (i    , j - 3) + S4LG (i    , j - 2)) * G31) >> 8 +
		 ((S4LG (i + 1, j - 4) + S4LG (i + 1, j - 3) + S4LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S4LG (i + 2, j - 4) + S4LG (i + 2, j - 3) + S4LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S4LG (i - 2, j - 1) + S4LG (i - 2, j    ) + S4LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S4LG (i - 1, j - 1) + S4LG (i - 1, j    ) + S4LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S4LG (i    , j - 1) + S4LG (i    , j    ) + S4LG (i    , j + 1)) * G32) >> 8 +
		 ((S4LG (i + 1, j - 1) + S4LG (i + 1, j    ) + S4LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S4LG (i + 2, j - 1) + S4LG (i + 2, j    ) + S4LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S4LG (i - 2, j + 2) + S4LG (i - 2, j + 3) + S4LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S4LG (i - 1, j + 2) + S4LG (i - 1, j + 3) + S4LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S4LG (i    , j + 2) + S4LG (i    , j + 3) + S4LG (i    , j + 4)) * G33) >> 8 +
		 ((S4LG (i + 1, j + 2) + S4LG (i + 1, j + 3) + S4LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S4LG (i + 2, j + 2) + S4LG (i + 2, j + 3) + S4LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    blue =
		(((S4LB (i - 2, j - 4) + S4LB (i - 2, j - 3) + S4LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S4LB (i - 1, j - 4) + S4LB (i - 1, j - 3) + S4LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S4LB (i    , j - 4) + S4LB (i    , j - 3) + S4LB (i    , j - 2)) * G31) >> 8 +
		 ((S4LB (i + 1, j - 4) + S4LB (i + 1, j - 3) + S4LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S4LB (i + 2, j - 4) + S4LB (i + 2, j - 3) + S4LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S4LB (i - 2, j - 1) + S4LB (i - 2, j    ) + S4LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S4LB (i - 1, j - 1) + S4LB (i - 1, j    ) + S4LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S4LB (i    , j - 1) + S4LB (i    , j    ) + S4LB (i    , j + 1)) * G32) >> 8 +
		 ((S4LB (i + 1, j - 1) + S4LB (i + 1, j    ) + S4LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S4LB (i + 2, j - 1) + S4LB (i + 2, j    ) + S4LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S4LB (i - 2, j + 2) + S4LB (i - 2, j + 3) + S4LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S4LB (i - 1, j + 2) + S4LB (i - 1, j + 3) + S4LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S4LB (i    , j + 2) + S4LB (i    , j + 3) + S4LB (i    , j + 4)) * G33) >> 8 +
		 ((S4LB (i + 1, j + 2) + S4LB (i + 1, j + 3) + S4LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S4LB (i + 2, j + 2) + S4LB (i + 2, j + 3) + S4LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	}
    } else {
	switch (bytes_per_pixel) {
	case 1:
	    blue =
		(((S1LB (i - 2, j - 4) + S1LB (i - 2, j - 3) + S1LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S1LB (i - 1, j - 4) + S1LB (i - 1, j - 3) + S1LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S1LB (i    , j - 4) + S1LB (i    , j - 3) + S1LB (i    , j - 2)) * G31) >> 8 +
		 ((S1LB (i + 1, j - 4) + S1LB (i + 1, j - 3) + S1LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S1LB (i + 2, j - 4) + S1LB (i + 2, j - 3) + S1LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S1LB (i - 2, j - 1) + S1LB (i - 2, j    ) + S1LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S1LB (i - 1, j - 1) + S1LB (i - 1, j    ) + S1LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S1LB (i    , j - 1) + S1LB (i    , j    ) + S1LB (i    , j + 1)) * G32) >> 8 +
		 ((S1LB (i + 1, j - 1) + S1LB (i + 1, j    ) + S1LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S1LB (i + 2, j - 1) + S1LB (i + 2, j    ) + S1LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S1LB (i - 2, j + 2) + S1LB (i - 2, j + 3) + S1LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S1LB (i - 1, j + 2) + S1LB (i - 1, j + 3) + S1LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S1LB (i    , j + 2) + S1LB (i    , j + 3) + S1LB (i    , j + 4)) * G33) >> 8 +
		 ((S1LB (i + 1, j + 2) + S1LB (i + 1, j + 3) + S1LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S1LB (i + 2, j + 2) + S1LB (i + 2, j + 3) + S1LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S1LG (i - 2, j - 4) + S1LG (i - 2, j - 3) + S1LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S1LG (i - 1, j - 4) + S1LG (i - 1, j - 3) + S1LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S1LG (i    , j - 4) + S1LG (i    , j - 3) + S1LG (i    , j - 2)) * G31) >> 8 +
		 ((S1LG (i + 1, j - 4) + S1LG (i + 1, j - 3) + S1LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S1LG (i + 2, j - 4) + S1LG (i + 2, j - 3) + S1LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S1LG (i - 2, j - 1) + S1LG (i - 2, j    ) + S1LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S1LG (i - 1, j - 1) + S1LG (i - 1, j    ) + S1LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S1LG (i    , j - 1) + S1LG (i    , j    ) + S1LG (i    , j + 1)) * G32) >> 8 +
		 ((S1LG (i + 1, j - 1) + S1LG (i + 1, j    ) + S1LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S1LG (i + 2, j - 1) + S1LG (i + 2, j    ) + S1LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S1LG (i - 2, j + 2) + S1LG (i - 2, j + 3) + S1LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S1LG (i - 1, j + 2) + S1LG (i - 1, j + 3) + S1LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S1LG (i    , j + 2) + S1LG (i    , j + 3) + S1LG (i    , j + 4)) * G33) >> 8 +
		 ((S1LG (i + 1, j + 2) + S1LG (i + 1, j + 3) + S1LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S1LG (i + 2, j + 2) + S1LG (i + 2, j + 3) + S1LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S1LR (i - 2, j - 4) + S1LR (i - 2, j - 3) + S1LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S1LR (i - 1, j - 4) + S1LR (i - 1, j - 3) + S1LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S1LR (i    , j - 4) + S1LR (i    , j - 3) + S1LR (i    , j - 2)) * G31) >> 8 +
		 ((S1LR (i + 1, j - 4) + S1LR (i + 1, j - 3) + S1LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S1LR (i + 2, j - 4) + S1LR (i + 2, j - 3) + S1LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S1LR (i - 2, j - 1) + S1LR (i - 2, j    ) + S1LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S1LR (i - 1, j - 1) + S1LR (i - 1, j    ) + S1LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S1LR (i    , j - 1) + S1LR (i    , j    ) + S1LR (i    , j + 1)) * G32) >> 8 +
		 ((S1LR (i + 1, j - 1) + S1LR (i + 1, j    ) + S1LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S1LR (i + 2, j - 1) + S1LR (i + 2, j    ) + S1LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S1LR (i - 2, j + 2) + S1LR (i - 2, j + 3) + S1LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S1LR (i - 1, j + 2) + S1LR (i - 1, j + 3) + S1LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S1LR (i    , j + 2) + S1LR (i    , j + 3) + S1LR (i    , j + 4)) * G33) >> 8 +
		 ((S1LR (i + 1, j + 2) + S1LR (i + 1, j + 3) + S1LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S1LR (i + 2, j + 2) + S1LR (i + 2, j + 3) + S1LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 2:
	    blue =
		(((S2LB (i - 2, j - 4) + S2LB (i - 2, j - 3) + S2LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S2LB (i - 1, j - 4) + S2LB (i - 1, j - 3) + S2LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S2LB (i    , j - 4) + S2LB (i    , j - 3) + S2LB (i    , j - 2)) * G31) >> 8 +
		 ((S2LB (i + 1, j - 4) + S2LB (i + 1, j - 3) + S2LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S2LB (i + 2, j - 4) + S2LB (i + 2, j - 3) + S2LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S2LB (i - 2, j - 1) + S2LB (i - 2, j    ) + S2LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S2LB (i - 1, j - 1) + S2LB (i - 1, j    ) + S2LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S2LB (i    , j - 1) + S2LB (i    , j    ) + S2LB (i    , j + 1)) * G32) >> 8 +
		 ((S2LB (i + 1, j - 1) + S2LB (i + 1, j    ) + S2LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S2LB (i + 2, j - 1) + S2LB (i + 2, j    ) + S2LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S2LB (i - 2, j + 2) + S2LB (i - 2, j + 3) + S2LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S2LB (i - 1, j + 2) + S2LB (i - 1, j + 3) + S2LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S2LB (i    , j + 2) + S2LB (i    , j + 3) + S2LB (i    , j + 4)) * G33) >> 8 +
		 ((S2LB (i + 1, j + 2) + S2LB (i + 1, j + 3) + S2LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S2LB (i + 2, j + 2) + S2LB (i + 2, j + 3) + S2LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S2LG (i - 2, j - 4) + S2LG (i - 2, j - 3) + S2LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S2LG (i - 1, j - 4) + S2LG (i - 1, j - 3) + S2LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S2LG (i    , j - 4) + S2LG (i    , j - 3) + S2LG (i    , j - 2)) * G31) >> 8 +
		 ((S2LG (i + 1, j - 4) + S2LG (i + 1, j - 3) + S2LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S2LG (i + 2, j - 4) + S2LG (i + 2, j - 3) + S2LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S2LG (i - 2, j - 1) + S2LG (i - 2, j    ) + S2LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S2LG (i - 1, j - 1) + S2LG (i - 1, j    ) + S2LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S2LG (i    , j - 1) + S2LG (i    , j    ) + S2LG (i    , j + 1)) * G32) >> 8 +
		 ((S2LG (i + 1, j - 1) + S2LG (i + 1, j    ) + S2LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S2LG (i + 2, j - 1) + S2LG (i + 2, j    ) + S2LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S2LG (i - 2, j + 2) + S2LG (i - 2, j + 3) + S2LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S2LG (i - 1, j + 2) + S2LG (i - 1, j + 3) + S2LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S2LG (i    , j + 2) + S2LG (i    , j + 3) + S2LG (i    , j + 4)) * G33) >> 8 +
		 ((S2LG (i + 1, j + 2) + S2LG (i + 1, j + 3) + S2LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S2LG (i + 2, j + 2) + S2LG (i + 2, j + 3) + S2LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S2LR (i - 2, j - 4) + S2LR (i - 2, j - 3) + S2LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S2LR (i - 1, j - 4) + S2LR (i - 1, j - 3) + S2LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S2LR (i    , j - 4) + S2LR (i    , j - 3) + S2LR (i    , j - 2)) * G31) >> 8 +
		 ((S2LR (i + 1, j - 4) + S2LR (i + 1, j - 3) + S2LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S2LR (i + 2, j - 4) + S2LR (i + 2, j - 3) + S2LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S2LR (i - 2, j - 1) + S2LR (i - 2, j    ) + S2LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S2LR (i - 1, j - 1) + S2LR (i - 1, j    ) + S2LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S2LR (i    , j - 1) + S2LR (i    , j    ) + S2LR (i    , j + 1)) * G32) >> 8 +
		 ((S2LR (i + 1, j - 1) + S2LR (i + 1, j    ) + S2LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S2LR (i + 2, j - 1) + S2LR (i + 2, j    ) + S2LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S2LR (i - 2, j + 2) + S2LR (i - 2, j + 3) + S2LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S2LR (i - 1, j + 2) + S2LR (i - 1, j + 3) + S2LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S2LR (i    , j + 2) + S2LR (i    , j + 3) + S2LR (i    , j + 4)) * G33) >> 8 +
		 ((S2LR (i + 1, j + 2) + S2LR (i + 1, j + 3) + S2LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S2LR (i + 2, j + 2) + S2LR (i + 2, j + 3) + S2LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 3:
	    blue =
		(((S3LB (i - 2, j - 4) + S3LB (i - 2, j - 3) + S3LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S3LB (i - 1, j - 4) + S3LB (i - 1, j - 3) + S3LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S3LB (i    , j - 4) + S3LB (i    , j - 3) + S3LB (i    , j - 2)) * G31) >> 8 +
		 ((S3LB (i + 1, j - 4) + S3LB (i + 1, j - 3) + S3LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S3LB (i + 2, j - 4) + S3LB (i + 2, j - 3) + S3LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S3LB (i - 2, j - 1) + S3LB (i - 2, j    ) + S3LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S3LB (i - 1, j - 1) + S3LB (i - 1, j    ) + S3LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S3LB (i    , j - 1) + S3LB (i    , j    ) + S3LB (i    , j + 1)) * G32) >> 8 +
		 ((S3LB (i + 1, j - 1) + S3LB (i + 1, j    ) + S3LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S3LB (i + 2, j - 1) + S3LB (i + 2, j    ) + S3LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S3LB (i - 2, j + 2) + S3LB (i - 2, j + 3) + S3LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S3LB (i - 1, j + 2) + S3LB (i - 1, j + 3) + S3LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S3LB (i    , j + 2) + S3LB (i    , j + 3) + S3LB (i    , j + 4)) * G33) >> 8 +
		 ((S3LB (i + 1, j + 2) + S3LB (i + 1, j + 3) + S3LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S3LB (i + 2, j + 2) + S3LB (i + 2, j + 3) + S3LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S3LG (i - 2, j - 4) + S3LG (i - 2, j - 3) + S3LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S3LG (i - 1, j - 4) + S3LG (i - 1, j - 3) + S3LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S3LG (i    , j - 4) + S3LG (i    , j - 3) + S3LG (i    , j - 2)) * G31) >> 8 +
		 ((S3LG (i + 1, j - 4) + S3LG (i + 1, j - 3) + S3LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S3LG (i + 2, j - 4) + S3LG (i + 2, j - 3) + S3LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S3LG (i - 2, j - 1) + S3LG (i - 2, j    ) + S3LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S3LG (i - 1, j - 1) + S3LG (i - 1, j    ) + S3LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S3LG (i    , j - 1) + S3LG (i    , j    ) + S3LG (i    , j + 1)) * G32) >> 8 +
		 ((S3LG (i + 1, j - 1) + S3LG (i + 1, j    ) + S3LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S3LG (i + 2, j - 1) + S3LG (i + 2, j    ) + S3LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S3LG (i - 2, j + 2) + S3LG (i - 2, j + 3) + S3LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S3LG (i - 1, j + 2) + S3LG (i - 1, j + 3) + S3LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S3LG (i    , j + 2) + S3LG (i    , j + 3) + S3LG (i    , j + 4)) * G33) >> 8 +
		 ((S3LG (i + 1, j + 2) + S3LG (i + 1, j + 3) + S3LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S3LG (i + 2, j + 2) + S3LG (i + 2, j + 3) + S3LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S3LR (i - 2, j - 4) + S3LR (i - 2, j - 3) + S3LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S3LR (i - 1, j - 4) + S3LR (i - 1, j - 3) + S3LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S3LR (i    , j - 4) + S3LR (i    , j - 3) + S3LR (i    , j - 2)) * G31) >> 8 +
		 ((S3LR (i + 1, j - 4) + S3LR (i + 1, j - 3) + S3LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S3LR (i + 2, j - 4) + S3LR (i + 2, j - 3) + S3LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S3LR (i - 2, j - 1) + S3LR (i - 2, j    ) + S3LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S3LR (i - 1, j - 1) + S3LR (i - 1, j    ) + S3LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S3LR (i    , j - 1) + S3LR (i    , j    ) + S3LR (i    , j + 1)) * G32) >> 8 +
		 ((S3LR (i + 1, j - 1) + S3LR (i + 1, j    ) + S3LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S3LR (i + 2, j - 1) + S3LR (i + 2, j    ) + S3LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S3LR (i - 2, j + 2) + S3LR (i - 2, j + 3) + S3LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S3LR (i - 1, j + 2) + S3LR (i - 1, j + 3) + S3LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S3LR (i    , j + 2) + S3LR (i    , j + 3) + S3LR (i    , j + 4)) * G33) >> 8 +
		 ((S3LR (i + 1, j + 2) + S3LR (i + 1, j + 3) + S3LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S3LR (i + 2, j + 2) + S3LR (i + 2, j + 3) + S3LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	case 4:
	    blue =
		(((S4LB (i - 2, j - 4) + S4LB (i - 2, j - 3) + S4LB (i - 2, j - 2)) * G11) >> 8 +
		 ((S4LB (i - 1, j - 4) + S4LB (i - 1, j - 3) + S4LB (i - 1, j - 2)) * G21) >> 8 +
		 ((S4LB (i    , j - 4) + S4LB (i    , j - 3) + S4LB (i    , j - 2)) * G31) >> 8 +
		 ((S4LB (i + 1, j - 4) + S4LB (i + 1, j - 3) + S4LB (i + 1, j - 2)) * G41) >> 8 +
		 ((S4LB (i + 2, j - 4) + S4LB (i + 2, j - 3) + S4LB (i + 2, j - 2)) * G51) >> 8 +
		 ((S4LB (i - 2, j - 1) + S4LB (i - 2, j    ) + S4LB (i - 2, j + 1)) * G12) >> 8 +
		 ((S4LB (i - 1, j - 1) + S4LB (i - 1, j    ) + S4LB (i - 1, j + 1)) * G22) >> 8 +
		 ((S4LB (i    , j - 1) + S4LB (i    , j    ) + S4LB (i    , j + 1)) * G32) >> 8 +
		 ((S4LB (i + 1, j - 1) + S4LB (i + 1, j    ) + S4LB (i + 1, j + 1)) * G42) >> 8 +
		 ((S4LB (i + 2, j - 1) + S4LB (i + 2, j    ) + S4LB (i + 2, j + 1)) * G52) >> 8 +
		 ((S4LB (i - 2, j + 2) + S4LB (i - 2, j + 3) + S4LB (i - 2, j + 4)) * G13) >> 8 +
		 ((S4LB (i - 1, j + 2) + S4LB (i - 1, j + 3) + S4LB (i - 1, j + 4)) * G23) >> 8 +
		 ((S4LB (i    , j + 2) + S4LB (i    , j + 3) + S4LB (i    , j + 4)) * G33) >> 8 +
		 ((S4LB (i + 1, j + 2) + S4LB (i + 1, j + 3) + S4LB (i + 1, j + 4)) * G43) >> 8 +
		 ((S4LB (i + 2, j + 2) + S4LB (i + 2, j + 3) + S4LB (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    green =
		(((S4LG (i - 2, j - 4) + S4LG (i - 2, j - 3) + S4LG (i - 2, j - 2)) * G11) >> 8 +
		 ((S4LG (i - 1, j - 4) + S4LG (i - 1, j - 3) + S4LG (i - 1, j - 2)) * G21) >> 8 +
		 ((S4LG (i    , j - 4) + S4LG (i    , j - 3) + S4LG (i    , j - 2)) * G31) >> 8 +
		 ((S4LG (i + 1, j - 4) + S4LG (i + 1, j - 3) + S4LG (i + 1, j - 2)) * G41) >> 8 +
		 ((S4LG (i + 2, j - 4) + S4LG (i + 2, j - 3) + S4LG (i + 2, j - 2)) * G51) >> 8 +
		 ((S4LG (i - 2, j - 1) + S4LG (i - 2, j    ) + S4LG (i - 2, j + 1)) * G12) >> 8 +
		 ((S4LG (i - 1, j - 1) + S4LG (i - 1, j    ) + S4LG (i - 1, j + 1)) * G22) >> 8 +
		 ((S4LG (i    , j - 1) + S4LG (i    , j    ) + S4LG (i    , j + 1)) * G32) >> 8 +
		 ((S4LG (i + 1, j - 1) + S4LG (i + 1, j    ) + S4LG (i + 1, j + 1)) * G42) >> 8 +
		 ((S4LG (i + 2, j - 1) + S4LG (i + 2, j    ) + S4LG (i + 2, j + 1)) * G52) >> 8 +
		 ((S4LG (i - 2, j + 2) + S4LG (i - 2, j + 3) + S4LG (i - 2, j + 4)) * G13) >> 8 +
		 ((S4LG (i - 1, j + 2) + S4LG (i - 1, j + 3) + S4LG (i - 1, j + 4)) * G23) >> 8 +
		 ((S4LG (i    , j + 2) + S4LG (i    , j + 3) + S4LG (i    , j + 4)) * G33) >> 8 +
		 ((S4LG (i + 1, j + 2) + S4LG (i + 1, j + 3) + S4LG (i + 1, j + 4)) * G43) >> 8 +
		 ((S4LG (i + 2, j + 2) + S4LG (i + 2, j + 3) + S4LG (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    red =
		(((S4LR (i - 2, j - 4) + S4LR (i - 2, j - 3) + S4LR (i - 2, j - 2)) * G11) >> 8 +
		 ((S4LR (i - 1, j - 4) + S4LR (i - 1, j - 3) + S4LR (i - 1, j - 2)) * G21) >> 8 +
		 ((S4LR (i    , j - 4) + S4LR (i    , j - 3) + S4LR (i    , j - 2)) * G31) >> 8 +
		 ((S4LR (i + 1, j - 4) + S4LR (i + 1, j - 3) + S4LR (i + 1, j - 2)) * G41) >> 8 +
		 ((S4LR (i + 2, j - 4) + S4LR (i + 2, j - 3) + S4LR (i + 2, j - 2)) * G51) >> 8 +
		 ((S4LR (i - 2, j - 1) + S4LR (i - 2, j    ) + S4LR (i - 2, j + 1)) * G12) >> 8 +
		 ((S4LR (i - 1, j - 1) + S4LR (i - 1, j    ) + S4LR (i - 1, j + 1)) * G22) >> 8 +
		 ((S4LR (i    , j - 1) + S4LR (i    , j    ) + S4LR (i    , j + 1)) * G32) >> 8 +
		 ((S4LR (i + 1, j - 1) + S4LR (i + 1, j    ) + S4LR (i + 1, j + 1)) * G42) >> 8 +
		 ((S4LR (i + 2, j - 1) + S4LR (i + 2, j    ) + S4LR (i + 2, j + 1)) * G52) >> 8 +
		 ((S4LR (i - 2, j + 2) + S4LR (i - 2, j + 3) + S4LR (i - 2, j + 4)) * G13) >> 8 +
		 ((S4LR (i - 1, j + 2) + S4LR (i - 1, j + 3) + S4LR (i - 1, j + 4)) * G23) >> 8 +
		 ((S4LR (i    , j + 2) + S4LR (i    , j + 3) + S4LR (i    , j + 4)) * G33) >> 8 +
		 ((S4LR (i + 1, j + 2) + S4LR (i + 1, j + 3) + S4LR (i + 1, j + 4)) * G43) >> 8 +
		 ((S4LR (i + 2, j + 2) + S4LR (i + 2, j + 3) + S4LR (i + 2, j + 4)) * G53) >> 8);
	    i++;
	    break;
	}
    }
}

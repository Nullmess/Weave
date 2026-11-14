/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT file system module  R0.12c                             /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2017, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/----------------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"

#if _FATFS != 68300
#error Wrong include file (ff.h).
#endif

/* Plages DBCS et tables majuscules SBCS */

#if _CODE_PAGE == 932	/* Japonais Shift-JIS */
#define _DF1S	0x81
#define _DF1E	0x9F
#define _DF2S	0xE0
#define _DF2E	0xFC
#define _DS1S	0x40
#define _DS1E	0x7E
#define _DS2S	0x80
#define _DS2E	0xFC

#elif _CODE_PAGE == 936	/* Chinois simplifié GBK */
#define _DF1S	0x81
#define _DF1E	0xFE
#define _DS1S	0x40
#define _DS1E	0x7E
#define _DS2S	0x80
#define _DS2E	0xFE

#elif _CODE_PAGE == 949	/* Coréen */
#define _DF1S	0x81
#define _DF1E	0xFE
#define _DS1S	0x41
#define _DS1E	0x5A
#define _DS2S	0x61
#define _DS2E	0x7A
#define _DS3S	0x81
#define _DS3E	0xFE

#elif _CODE_PAGE == 950	/* Chinois traditionnel Big5 */
#define _DF1S	0x81
#define _DF1E	0xFE
#define _DS1S	0x40
#define _DS1E	0x7E
#define _DS2S	0xA1
#define _DS2E	0xFE

#elif _CODE_PAGE == 437	/* U.S. */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x45,0x41,0x8E,0x41,0x8F,0x80,0x45,0x45,0x45,0x49,0x49,0x49,0x8E,0x8F, \
				0x90,0x92,0x92,0x4F,0x99,0x4F,0x55,0x55,0x59,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 720	/* Arabe */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 737	/* Grec */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0x90,0x92,0x92,0x93,0x94,0x95,0x96,0x97,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87, \
				0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0xAA,0x92,0x93,0x94,0x95,0x96, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0x97,0xEA,0xEB,0xEC,0xE4,0xED,0xEE,0xEF,0xF5,0xF0,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 771	/* KBL */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDC,0xDE,0xDE, \
				0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xF0,0xF0,0xF2,0xF2,0xF4,0xF4,0xF6,0xF6,0xF8,0xF8,0xFA,0xFA,0xFC,0xFC,0xFE,0xFF}

#elif _CODE_PAGE == 775	/* Balte */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x91,0xA0,0x8E,0x95,0x8F,0x80,0xAD,0xED,0x8A,0x8A,0xA1,0x8D,0x8E,0x8F, \
				0x90,0x92,0x92,0xE2,0x99,0x95,0x96,0x97,0x97,0x99,0x9A,0x9D,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xE0,0xA3,0xA3,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xB5,0xB6,0xB7,0xB8,0xBD,0xBE,0xC6,0xC7,0xA5,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE5,0xE5,0xE6,0xE3,0xE8,0xE8,0xEA,0xEA,0xEE,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 850	/* Latin 1 */
#define _DF1S	0
#define _EXCVT {0x43,0x55,0x45,0x41,0x41,0x41,0x41,0x43,0x45,0x45,0x45,0x49,0x49,0x49,0x41,0x41, \
				0x45,0x92,0x92,0x4F,0x4F,0x4F,0x55,0x55,0x59,0x4F,0x55,0x4F,0x9C,0x4F,0x9E,0x9F, \
				0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0x41,0x41,0x41,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0x41,0x41,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD1,0xD1,0x45,0x45,0x45,0x49,0x49,0x49,0x49,0xD9,0xDA,0xDB,0xDC,0xDD,0x49,0xDF, \
				0x4F,0xE1,0x4F,0x4F,0x4F,0x4F,0xE6,0xE8,0xE8,0x55,0x55,0x55,0x59,0x59,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 852	/* Latin 2 */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0xB6,0x8E,0xDE,0x8F,0x80,0x9D,0xD3,0x8A,0x8A,0xD7,0x8D,0x8E,0x8F, \
				0x90,0x91,0x91,0xE2,0x99,0x95,0x95,0x97,0x97,0x99,0x9A,0x9B,0x9B,0x9D,0x9E,0xAC, \
				0xB5,0xD6,0xE0,0xE9,0xA4,0xA4,0xA6,0xA6,0xA8,0xA8,0xAA,0x8D,0xAC,0xB8,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBD,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC6,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD1,0xD1,0xD2,0xD3,0xD2,0xD5,0xD6,0xD7,0xB7,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE3,0xD5,0xE6,0xE6,0xE8,0xE9,0xE8,0xEB,0xED,0xED,0xDD,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xEB,0xFC,0xFC,0xFE,0xFF}

#elif _CODE_PAGE == 855	/* Cyrillique */
#define _DF1S	0
#define _EXCVT {0x81,0x81,0x83,0x83,0x85,0x85,0x87,0x87,0x89,0x89,0x8B,0x8B,0x8D,0x8D,0x8F,0x8F, \
				0x91,0x91,0x93,0x93,0x95,0x95,0x97,0x97,0x99,0x99,0x9B,0x9B,0x9D,0x9D,0x9F,0x9F, \
				0xA1,0xA1,0xA3,0xA3,0xA5,0xA5,0xA7,0xA7,0xA9,0xA9,0xAB,0xAB,0xAD,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB6,0xB6,0xB8,0xB8,0xB9,0xBA,0xBB,0xBC,0xBE,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC7,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD1,0xD1,0xD3,0xD3,0xD5,0xD5,0xD7,0xD7,0xDD,0xD9,0xDA,0xDB,0xDC,0xDD,0xE0,0xDF, \
				0xE0,0xE2,0xE2,0xE4,0xE4,0xE6,0xE6,0xE8,0xE8,0xEA,0xEA,0xEC,0xEC,0xEE,0xEE,0xEF, \
				0xF0,0xF2,0xF2,0xF4,0xF4,0xF6,0xF6,0xF8,0xF8,0xFA,0xFA,0xFC,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 857	/* Turc */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0xB6,0x8E,0xB7,0x8F,0x80,0xD2,0xD3,0xD4,0xD8,0xD7,0x49,0x8E,0x8F, \
				0x90,0x92,0x92,0xE2,0x99,0xE3,0xEA,0xEB,0x98,0x99,0x9A,0x9D,0x9C,0x9D,0x9E,0x9E, \
				0xB5,0xD6,0xE0,0xE9,0xA5,0xA5,0xA6,0xA6,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC7,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0x49,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE5,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xDE,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 860	/* Portugais */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0x8F,0x8E,0x91,0x86,0x80,0x89,0x89,0x92,0x8B,0x8C,0x98,0x8E,0x8F, \
				0x90,0x91,0x92,0x8C,0x99,0xA9,0x96,0x9D,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x86,0x8B,0x9F,0x96,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 861	/* Islandais */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0x41,0x8E,0x41,0x8F,0x80,0x45,0x45,0x45,0x8B,0x8B,0x8D,0x8E,0x8F, \
				0x90,0x92,0x92,0x4F,0x99,0x8D,0x55,0x97,0x97,0x99,0x9A,0x9D,0x9C,0x9D,0x9E,0x9F, \
				0xA4,0xA5,0xA6,0xA7,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 862	/* Hébreu */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 863	/* Français canadien */
#define _DF1S	0
#define _EXCVT {0x43,0x55,0x45,0x41,0x41,0x41,0x86,0x43,0x45,0x45,0x45,0x49,0x49,0x8D,0x41,0x8F, \
				0x45,0x45,0x45,0x4F,0x45,0x49,0x55,0x55,0x98,0x4F,0x55,0x9B,0x9C,0x55,0x55,0x9F, \
				0xA0,0xA1,0x4F,0x55,0xA4,0xA5,0xA6,0xA7,0x49,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 864	/* Arabe */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x45,0x41,0x8E,0x41,0x8F,0x80,0x45,0x45,0x45,0x49,0x49,0x49,0x8E,0x8F, \
				0x90,0x92,0x92,0x4F,0x99,0x4F,0x55,0x55,0x59,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 865	/* Nordique */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0x41,0x8E,0x41,0x8F,0x80,0x45,0x45,0x45,0x49,0x49,0x49,0x8E,0x8F, \
				0x90,0x92,0x92,0x4F,0x99,0x4F,0x55,0x55,0x59,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, \
				0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 866	/* Russe */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xF0,0xF0,0xF2,0xF2,0xF4,0xF4,0xF6,0xF6,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 869	/* Grec 2 */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, \
				0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x86,0x9C,0x8D,0x8F,0x90, \
				0x91,0x90,0x92,0x95,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, \
				0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, \
				0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xA4,0xA5,0xA6,0xD9,0xDA,0xDB,0xDC,0xA7,0xA8,0xDF, \
				0xA9,0xAA,0xAC,0xAD,0xB5,0xB6,0xB7,0xB8,0xBD,0xBE,0xC6,0xC7,0xCF,0xCF,0xD0,0xEF, \
				0xF0,0xF1,0xD1,0xD2,0xD3,0xF5,0xD4,0xF7,0xF8,0xF9,0xD5,0x96,0x95,0x98,0xFE,0xFF}

#elif _CODE_PAGE == 1	/* ASCII uniquement en configuration sans LFN */
#if _USE_LFN != 0
#error Cannot enable LFN without valid code page.
#endif
#define _DF1S	0

#else
#error Unknown code page

#endif

#define IsUpper(c)	(((c)>='A')&&((c)<='Z'))
#define IsLower(c)	(((c)>='a')&&((c)<='z'))
#define IsDigit(c)	(((c)>='0')&&((c)<='9'))

#if _DF1S != 0	/* Code page DBCS */

#ifdef _DF2S	/* Deux plages pour le premier byte */
#define IsDBCS1(c)	(((BYTE)(c) >= _DF1S && (BYTE)(c) <= _DF1E) || ((BYTE)(c) >= _DF2S && (BYTE)(c) <= _DF2E))
#else			/* Une plage pour le premier byte */
#define IsDBCS1(c)	((BYTE)(c) >= _DF1S && (BYTE)(c) <= _DF1E)
#endif

#ifdef _DS3S	/* Trois plages pour le second byte */
#define IsDBCS2(c)	(((BYTE)(c) >= _DS1S && (BYTE)(c) <= _DS1E) || ((BYTE)(c) >= _DS2S && (BYTE)(c) <= _DS2E) || ((BYTE)(c) >= _DS3S && (BYTE)(c) <= _DS3E))
#else			/* Deux plages pour le second byte */
#define IsDBCS2(c)	(((BYTE)(c) >= _DS1S && (BYTE)(c) <= _DS1E) || ((BYTE)(c) >= _DS2S && (BYTE)(c) <= _DS2E))
#endif

#else			/* Code page SBCS */

#define IsDBCS1(c)	0
#define IsDBCS2(c)	0

#endif

#define	AM_VOL		0x08	/* Label volume */
#define AM_LFN		0x0F	/* Entrée LFN */
#define AM_MASK		0x3F	/* Masque des bits définis */

#define FA_SEEKEND	0x20	/* Seek en fin de fichier à l’ouverture */
#define FA_MODIFIED	0x40	/* Fichier modifié */
#define FA_DIRTY	0x80	/* FIL.buf[] à écrire sur disque */

#define NSFLAG		11		/* Index byte status du nom */
#define NS_LOSS		0x01	/* Hors format 8.3 */
#define NS_LFN		0x02	/* Création forcée d’entrée LFN */
#define NS_LAST		0x04	/* Dernier segment */
#define NS_BODY		0x08	/* Nom en minuscules */
#define NS_EXT		0x10	/* Extension en minuscules */
#define NS_DOT		0x20	/* Entrée point */
#define NS_NOLFN	0x40	/* Ignorer LFN */
#define NS_NONAME	0x80	/* Non suivi */

#define MAX_DIR		0x200000		/* Taille maximale répertoire FAT */
#define MAX_DIR_EX	0x10000000		/* Taille maximale répertoire exFAT */
#define MAX_FAT12	0xFF5			/* Limite FAT12 adaptée au comportement DOS/Windows, différente des specs */
#define	MAX_FAT16	0xFFF5			/* Limite FAT16 adaptée au comportement DOS/Windows, différente des specs */
#define	MAX_FAT32	0x0FFFFFF5		/* Limite pratique FAT32, non spécifiée */
#define	MAX_EXFAT	0x7FFFFFFD		/* Limite exFAT propre à l’implémentation, différente des specs */

/* FAT en bytes : structures C non compatibles entre plateformes */

#define BS_JmpBoot			0		/* Instruction jump x86 de 3 bytes */
#define BS_OEMName			3		/* Nom OEM de 8 bytes */
#define BPB_BytsPerSec		11		/* Taille secteur en bytes, WORD */
#define BPB_SecPerClus		13		/* Taille cluster en secteurs, BYTE */
#define BPB_RsvdSecCnt		14		/* Zone réservée en secteurs, WORD */
#define BPB_NumFATs			16		/* Nombre de FAT, BYTE */
#define BPB_RootEntCnt		17		/* Racine FAT12/16 en entrées, WORD */
#define BPB_TotSec16		19		/* Taille volume 16 bits en secteurs, WORD */
#define BPB_Media			21		/* Descripteur media, BYTE */
#define BPB_FATSz16			22		/* Taille FAT 16 bits en secteurs, WORD */
#define BPB_SecPerTrk		24		/* Taille piste int13h en secteurs, WORD */
#define BPB_NumHeads		26		/* Nombre de têtes int13h, WORD */
#define BPB_HiddSec			28		/* Offset volume depuis début lecteur, DWORD */
#define BPB_TotSec32		32		/* Taille volume 32 bits en secteurs, DWORD */
#define BS_DrvNum			36		/* Lecteur physique int13h, BYTE */
#define BS_NTres			37		/* Flag erreur, BYTE */
#define BS_BootSig			38		/* Signature boot étendue, BYTE */
#define BS_VolID			39		/* Numéro série volume, DWORD */
#define BS_VolLab			43		/* Label volume sur 8 bytes */
#define BS_FilSysType		54		/* Type filesystem sur 8 bytes */
#define BS_BootCode			62		/* Code boot sur 448 bytes */
#define BS_55AA				510		/* Signature WORD */

#define BPB_FATSz32			36		/* FAT32 : taille FAT en secteurs, DWORD */
#define BPB_ExtFlags32		40		/* FAT32 : flags étendus, WORD */
#define BPB_FSVer32			42		/* FAT32 : version filesystem, WORD */
#define BPB_RootClus32		44		/* FAT32 : cluster répertoire racine, DWORD */
#define BPB_FSInfo32		48		/* FAT32 : offset secteur FSINFO, WORD */
#define BPB_BkBootSec32		50		/* FAT32 : offset secteur boot de secours, WORD */
#define BS_DrvNum32			64		/* FAT32 : lecteur physique int13h, BYTE */
#define BS_NTres32			65		/* FAT32 : flag erreur, BYTE */
#define BS_BootSig32		66		/* FAT32 : signature boot étendue, BYTE */
#define BS_VolID32			67		/* FAT32 : numéro série volume, DWORD */
#define BS_VolLab32			71		/* FAT32 : label volume sur 8 bytes */
#define BS_FilSysType32		82		/* FAT32 : type filesystem sur 8 bytes */
#define BS_BootCode32		90		/* FAT32 : code boot sur 420 bytes */

#define BPB_ZeroedEx		11		/* exFAT : champ MBZ sur 53 bytes */
#define BPB_VolOfsEx		64		/* exFAT : offset volume depuis lecteur en secteurs, QWORD */
#define BPB_TotSecEx		72		/* exFAT : taille volume en secteurs, QWORD */
#define BPB_FatOfsEx		80		/* exFAT : offset FAT depuis volume en secteurs, DWORD */
#define BPB_FatSzEx			84		/* exFAT : taille FAT en secteurs, DWORD */
#define BPB_DataOfsEx		88		/* exFAT : offset données depuis volume en secteurs, DWORD */
#define BPB_NumClusEx		92		/* exFAT : nombre de clusters, DWORD */
#define BPB_RootClusEx		96		/* exFAT : premier cluster répertoire racine, DWORD */
#define BPB_VolIDEx			100		/* exFAT : numéro série volume, DWORD */
#define BPB_FSVerEx			104		/* exFAT : version filesystem, WORD */
#define BPB_VolFlagEx		106		/* exFAT : flags volume, BYTE */
#define BPB_ActFatEx		107		/* exFAT : flags FAT active, BYTE */
#define BPB_BytsPerSecEx	108		/* exFAT : log2 taille secteur en bytes, BYTE */
#define BPB_SecPerClusEx	109		/* exFAT : log2 taille cluster en secteurs, BYTE */
#define BPB_NumFATsEx		110		/* exFAT : nombre de FAT, BYTE */
#define BPB_DrvNumEx		111		/* exFAT : lecteur physique int13h, BYTE */
#define BPB_PercInUseEx		112		/* exFAT : pourcentage utilisé, BYTE */
#define	BPB_RsvdEx			113		/* exFAT : 7 bytes réservés */
#define BS_BootCodeEx		120		/* exFAT : code boot sur 390 bytes */

#define	DIR_Name			0		/* Nom court sur 11 bytes */
#define	DIR_Attr			11		/* Attribut BYTE */
#define	DIR_NTres			12		/* Flag minuscules, BYTE */
#define DIR_CrtTime10		13		/* Création : fraction de seconde, BYTE */
#define	DIR_CrtTime			14		/* Timestamp création, DWORD */
#define DIR_LstAccDate		18		/* Date dernier accès, WORD */
#define	DIR_FstClusHI		20		/* Premier cluster : 16 bits supérieurs, WORD */
#define	DIR_ModTime			22		/* Timestamp modification, DWORD */
#define	DIR_FstClusLO		26		/* Premier cluster : 16 bits inférieurs, WORD */
#define	DIR_FileSize		28		/* Taille fichier, DWORD */
#define	LDIR_Ord			0		/* LFN : ordre et flag LLE, BYTE */
#define	LDIR_Attr			11		/* LFN : attribut, BYTE */
#define	LDIR_Type			12		/* LFN : type entrée, BYTE */
#define	LDIR_Chksum			13		/* LFN : checksum du SFN, BYTE */
#define	LDIR_FstClusLO		26		/* LFN : champ MBZ, WORD */
#define	XDIR_Type			0		/* exFAT : type entrée répertoire, BYTE */
#define	XDIR_NumLabel		1		/* exFAT : nombre caractères du label volume, BYTE */
#define	XDIR_Label			2		/* exFAT : label volume sur 11 WORD */
#define	XDIR_CaseSum		4		/* exFAT : checksum table conversion casse, DWORD */
#define	XDIR_NumSec			1		/* exFAT : nombre entrées secondaires, BYTE */
#define	XDIR_SetSum			2		/* exFAT : checksum ensemble entrées répertoire, WORD */
#define	XDIR_Attr			4		/* exFAT : attribut fichier, WORD */
#define	XDIR_CrtTime		8		/* exFAT : timestamp création, DWORD */
#define	XDIR_ModTime		12		/* exFAT : timestamp modification, DWORD */
#define	XDIR_AccTime		16		/* exFAT : timestamp dernier accès, DWORD */
#define	XDIR_CrtTime10		20		/* exFAT : création fraction de seconde, BYTE */
#define	XDIR_ModTime10		21		/* exFAT : modification fraction de seconde, BYTE */
#define	XDIR_CrtTZ			22		/* exFAT : timezone création, BYTE */
#define	XDIR_ModTZ			23		/* exFAT : timezone modification, BYTE */
#define	XDIR_AccTZ			24		/* exFAT : timezone dernier accès, BYTE */
#define	XDIR_GenFlags		33		/* exFAT : flags secondaires généraux, WORD */
#define	XDIR_NumName		35		/* exFAT : nombre caractères nom fichier, BYTE */
#define	XDIR_NameHash		36		/* exFAT : hash nom fichier, WORD */
#define XDIR_ValidFileSize	40		/* exFAT : taille valide fichier, QWORD */
#define	XDIR_FstClus		52		/* exFAT : premier cluster données fichier, DWORD */
#define	XDIR_FileSize		56		/* exFAT : taille fichier/répertoire, QWORD */

#define	SZDIRE				32		/* Taille entrée répertoire */
#define	DDEM				0xE5	/* Marque entrée supprimée dans DIR_Name[0] */
#define	RDDEM				0x05	/* Remplacement du caractère confondu avec DDEM */
#define	LLEF				0x40	/* Flag dernière entrée longue dans LDIR_Ord */

#define	FSI_LeadSig			0		/* FSI FAT32 : signature initiale, DWORD */
#define	FSI_StrucSig		484		/* FSI FAT32 : signature structure, DWORD */
#define	FSI_Free_Count		488		/* FSI FAT32 : clusters libres, DWORD */
#define	FSI_Nxt_Free		492		/* FSI FAT32 : dernier cluster alloué, DWORD */

#define MBR_Table			446		/* MBR : offset table partitions */
#define	SZ_PTE				16		/* MBR : taille entrée table partitions */
#define PTE_Boot			0		/* MBR PTE : indicateur boot */
#define PTE_StHead			1		/* MBR PTE : tête début */
#define PTE_StSec			2		/* MBR PTE : secteur début */
#define PTE_StCyl			3		/* MBR PTE : cylindre début */
#define PTE_System			4		/* MBR PTE : ID système */
#define PTE_EdHead			5		/* MBR PTE : tête fin */
#define PTE_EdSec			6		/* MBR PTE : secteur fin */
#define PTE_EdCyl			7		/* MBR PTE : cylindre fin */
#define PTE_StLba			8		/* MBR PTE : début en LBA */
#define PTE_SizLba			12		/* MBR PTE : taille en LBA */

#define	ABORT(fs, res)		{ fp->err = (BYTE)(res); LEAVE_FF(fs, res); }

#if _FS_REENTRANT
#if _USE_LFN == 1
#error Static LFN work area cannot be used at thread-safe configuration
#endif
#define	ENTER_FF(fs)		{ if (!lock_fs(fs)) return FR_TIMEOUT; }
#define	LEAVE_FF(fs, res)	{ unlock_fs(fs, res); return res; }
#else
#define	ENTER_FF(fs)
#define LEAVE_FF(fs, res)	return res
#endif

#if _MULTI_PARTITION
#define LD2PD(vol) VolToPart[vol].pd
#define LD2PT(vol) VolToPart[vol].pt
#else
#define LD2PD(vol) (BYTE)(vol)	/* Chaque lecteur logique associé au même numéro physique */
#define LD2PT(vol) 0			/* Première partition valide ou SFD */
#endif

#if (_MAX_SS < _MIN_SS) || (_MAX_SS != 512 && _MAX_SS != 1024 && _MAX_SS != 2048 && _MAX_SS != 4096) || (_MIN_SS != 512 && _MIN_SS != 1024 && _MIN_SS != 2048 && _MIN_SS != 4096)
#error Wrong sector size configuration
#endif
#if _MAX_SS == _MIN_SS
#define	SS(fs)	((UINT)_MAX_SS)	/* Secteurs de taille fixe */
#else
#define	SS(fs)	((fs)->ssize)	/* Secteurs de taille variable */
#endif

#if _FS_NORTC == 1
#if _NORTC_YEAR < 1980 || _NORTC_YEAR > 2107 || _NORTC_MON < 1 || _NORTC_MON > 12 || _NORTC_MDAY < 1 || _NORTC_MDAY > 31
#error Invalid _FS_NORTC settings
#endif
#define GET_FATTIME()	((DWORD)(_NORTC_YEAR - 1980) << 25 | (DWORD)_NORTC_MON << 21 | (DWORD)_NORTC_MDAY << 16)
#else
#define GET_FATTIME()	get_fattime()
#endif

#if _FS_LOCK != 0
#if _FS_READONLY
#error _FS_LOCK must be 0 at read-only configuration
#endif
typedef struct {
	FATFS *fs;		/* Object ID 1 : volume ; NULL entrée vide */
	DWORD clu;		/* Object ID 2 : répertoire parent ; 0 racine */
	DWORD ofs;		/* Object ID 3 : offset répertoire */
	WORD ctr;		/* Open count : 0 aucun, 1..255 lectures, 0x100 écriture */
} FILESEM;
#endif

/* Variables sans initializer : zéro/NULL garantis au démarrage C */

#if _VOLUMES < 1 || _VOLUMES > 10
#error Wrong _VOLUMES setting
#endif
static FATFS *FatFs[_VOLUMES];
static WORD Fsid;

#if _FS_RPATH != 0 && _VOLUMES >= 2
static BYTE CurrVol;
#endif

#if _FS_LOCK != 0
static FILESEM Files[_FS_LOCK];
#endif

#if _USE_LFN == 0		/* Configuration sans LFN */
#define	DEF_NAMBUF
#define INIT_NAMBUF(fs)
#define	FREE_NAMBUF()

#else					/* Configuration LFN */
#if _MAX_LFN < 12 || _MAX_LFN > 255
#error Wrong _MAX_LFN value
#endif
#define MAXDIRB(nc)	((nc + 44U) / 15 * SZDIRE)

#if _USE_LFN == 1		/* LFN : buffer statique */
#if _FS_EXFAT
static BYTE	DirBuf[MAXDIRB(_MAX_LFN)];	/* Buffer temporaire du bloc d’entrées répertoire */
#endif
static WCHAR LfnBuf[_MAX_LFN + 1];	/* LFN : buffer statique */
#define	DEF_NAMBUF
#define INIT_NAMBUF(fs)
#define	FREE_NAMBUF()

#elif _USE_LFN == 2 	/* LFN : buffer dynamique stack */
#if _FS_EXFAT
#define	DEF_NAMBUF		WCHAR lbuf[_MAX_LFN+1]; BYTE dbuf[MAXDIRB(_MAX_LFN)];
#define INIT_NAMBUF(fs)	{ (fs)->lfnbuf = lbuf; (fs)->dirbuf = dbuf; }
#define	FREE_NAMBUF()
#else
#define	DEF_NAMBUF		WCHAR lbuf[_MAX_LFN+1];
#define INIT_NAMBUF(fs)	{ (fs)->lfnbuf = lbuf; }
#define	FREE_NAMBUF()
#endif

#elif _USE_LFN == 3 	/* LFN : buffer dynamique heap */
#if _FS_EXFAT
#define	DEF_NAMBUF		WCHAR *lfn;
#define INIT_NAMBUF(fs)	{ lfn = ff_memalloc((_MAX_LFN+1)*2 + MAXDIRB(_MAX_LFN)); if (!lfn) LEAVE_FF(fs, FR_NOT_ENOUGH_CORE); (fs)->lfnbuf = lfn; (fs)->dirbuf = (BYTE*)(lfn+_MAX_LFN+1); }
#define	FREE_NAMBUF()	ff_memfree(lfn)
#else
#define	DEF_NAMBUF		WCHAR *lfn;
#define INIT_NAMBUF(fs)	{ lfn = ff_memalloc((_MAX_LFN+1)*2); if (!lfn) LEAVE_FF(fs, FR_NOT_ENOUGH_CORE); (fs)->lfnbuf = lfn; }
#define	FREE_NAMBUF()	ff_memfree(lfn)
#endif

#else
#error Wrong _USE_LFN setting

#endif
#endif

#ifdef _EXCVT
static const BYTE ExCvt[] = _EXCVT;	/* Table majuscules des caractères étendus SBCS */
#endif

static
WORD ld_word (const BYTE* ptr)	/* Lecture WORD little endian */
{
	WORD rv;

	rv = ptr[1];
	rv = rv << 8 | ptr[0];
	return rv;
}

static
DWORD ld_dword (const BYTE* ptr)	/* Lecture DWORD little endian */
{
	DWORD rv;

	rv = ptr[3];
	rv = rv << 8 | ptr[2];
	rv = rv << 8 | ptr[1];
	rv = rv << 8 | ptr[0];
	return rv;
}

#if _FS_EXFAT
static
QWORD ld_qword (const BYTE* ptr)	/* Lecture QWORD little endian */
{
	QWORD rv;

	rv = ptr[7];
	rv = rv << 8 | ptr[6];
	rv = rv << 8 | ptr[5];
	rv = rv << 8 | ptr[4];
	rv = rv << 8 | ptr[3];
	rv = rv << 8 | ptr[2];
	rv = rv << 8 | ptr[1];
	rv = rv << 8 | ptr[0];
	return rv;
}
#endif

#if !_FS_READONLY
static
void st_word (BYTE* ptr, WORD val)	/* Écriture WORD little endian */
{
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val;
}

static
void st_dword (BYTE* ptr, DWORD val)	/* Écriture DWORD little endian */
{
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val;
}

#if _FS_EXFAT
static
void st_qword (BYTE* ptr, QWORD val)	/* Écriture QWORD little endian */
{
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val; val >>= 8;
	*ptr++ = (BYTE)val;
}
#endif
#endif

static
void mem_cpy (void* dst, const void* src, UINT cnt) {
	BYTE *d = (BYTE*)dst;
	const BYTE *s = (const BYTE*)src;

	if (cnt) {
		do {
			*d++ = *s++;
		} while (--cnt);
	}
}

static
void mem_set (void* dst, int val, UINT cnt) {
	BYTE *d = (BYTE*)dst;

	do {
		*d++ = (BYTE)val;
	} while (--cnt);
}

static
int mem_cmp (const void* dst, const void* src, UINT cnt) {	/* ZR égal, NZ différent */
	const BYTE *d = (const BYTE *)dst, *s = (const BYTE *)src;
	int r = 0;

	do {
		r = *d++ - *s++;
	} while (--cnt && r == 0);

	return r;
}

static
int chk_chr (const char* str, int chr) {	/* NZ contenu, ZR absent */
	while (*str && *str != chr) str++;
	return *str;
}

#if _FS_REENTRANT

static
int lock_fs (
	FATFS* fs		/* Objet système de fichiers */
)
{
	return (fs && ff_req_grant(fs->sobj)) ? 1 : 0;
}

static
void unlock_fs (
	FATFS* fs,		/* Objet système de fichiers */
	FRESULT res
)
{
	if (fs && res != FR_NOT_ENABLED && res != FR_INVALID_DRIVE && res != FR_TIMEOUT) {
		ff_rel_grant(fs->sobj);
	}
}

#endif

#if _FS_LOCK != 0

static
FRESULT chk_lock (
	DIR* dp,
	int acc			/* Accès : 0 lecture, 1 écriture, 2 delete/rename */
)
{
	UINT i, be;

	for (i = be = 0; i < _FS_LOCK; i++) {
		if (Files[i].fs) {
			if (Files[i].fs == dp->obj.fs &&
				Files[i].clu == dp->obj.sclust &&
				Files[i].ofs == dp->dptr) break;
		} else {
			be = 1;
		}
	}
	if (i == _FS_LOCK) {
		return (be || acc == 2) ? FR_OK : FR_TOO_MANY_OPEN_FILES;
	}

	/* Refuser ouverture sur fichier en écriture et ouvertures write */
	return (acc || Files[i].ctr == 0x100) ? FR_LOCKED : FR_OK;
}

static
int enq_lock (void)
{
	UINT i;

	for (i = 0; i < _FS_LOCK && Files[i].fs; i++) ;
	return (i == _FS_LOCK) ? 0 : 1;
}

static
UINT inc_lock (	/* Retour index semaphore ; 0 erreur interne */
	DIR* dp,
	int acc		/* Accès : 0 lecture, 1 écriture, 2 delete/rename */
)
{
	UINT i;

	for (i = 0; i < _FS_LOCK; i++) {
		if (Files[i].fs == dp->obj.fs &&
			Files[i].clu == dp->obj.sclust &&
			Files[i].ofs == dp->dptr) break;
	}

	if (i == _FS_LOCK) {
		for (i = 0; i < _FS_LOCK && Files[i].fs; i++) ;
		if (i == _FS_LOCK) return 0;	/* Aucune entrée libre : erreur interne */
		Files[i].fs = dp->obj.fs;
		Files[i].clu = dp->obj.sclust;
		Files[i].ofs = dp->dptr;
		Files[i].ctr = 0;
	}

	if (acc && Files[i].ctr) return 0;	/* Violation d’accès : erreur interne */

	Files[i].ctr = acc ? 0x100 : Files[i].ctr + 1;

	return i + 1;
}

static
FRESULT dec_lock (
	UINT i			/* Index semaphore depuis 1 */
)
{
	WORD n;
	FRESULT res;

	if (--i < _FS_LOCK) {	/* Conversion index vers origine 0 */
		n = Files[i].ctr;
		if (n == 0x100) n = 0;
		if (n > 0) n--;
		Files[i].ctr = n;
		if (n == 0) Files[i].fs = 0;
		res = FR_OK;
	} else {
		res = FR_INT_ERR;			/* Index invalide */
	}
	return res;
}

static
void clear_lock (
	FATFS *fs
)
{
	UINT i;

	for (i = 0; i < _FS_LOCK; i++) {
		if (Files[i].fs == fs) Files[i].fs = 0;
	}
}

#endif

#if !_FS_READONLY
static
FRESULT sync_window (	/* Retour FR_OK ou FR_DISK_ERROR */
	FATFS* fs			/* Objet système de fichiers */
)
{
	DWORD wsect;
	UINT nf;
	FRESULT res = FR_OK;

	if (fs->wflag) {	/* Writeback uniquement si secteur dirty */
		wsect = fs->winsect;
		if (disk_write(fs->drv, fs->win, wsect, 1) != RES_OK) {
			res = FR_DISK_ERR;
		} else {
			fs->wflag = 0;
			if (wsect - fs->fatbase < fs->fsize) {
				for (nf = fs->n_fats; nf >= 2; nf--) {	/* Propagation à toutes les copies FAT */
					wsect += fs->fsize;
					disk_write(fs->drv, fs->win, wsect, 1);
				}
			}
		}
	}
	return res;
}
#endif

static
FRESULT move_window (	/* Retour FR_OK ou FR_DISK_ERROR */
	FATFS* fs,			/* Objet système de fichiers */
	DWORD sector		/* Secteur à charger dans fs->win[] */
)
{
	FRESULT res = FR_OK;

	if (sector != fs->winsect) {
#if !_FS_READONLY
		res = sync_window(fs);
#endif
		if (res == FR_OK) {
			if (disk_read(fs->drv, fs->win, sector, 1) != RES_OK) {
				sector = 0xFFFFFFFF;	/* Invalider la fenêtre si données non fiables */
				res = FR_DISK_ERR;
			}
			fs->winsect = sector;
		}
	}
	return res;
}

#if !_FS_READONLY

static
FRESULT sync_fs (	/* FR_OK succès ; sinon erreur */
	FATFS* fs		/* Objet système de fichiers */
)
{
	FRESULT res;

	res = sync_window(fs);
	if (res == FR_OK) {

		if (fs->fs_type == FS_FAT32 && fs->fsi_flag == 1) {

			mem_set(fs->win, 0, SS(fs));
			st_word(fs->win + BS_55AA, 0xAA55);
			st_dword(fs->win + FSI_LeadSig, 0x41615252);
			st_dword(fs->win + FSI_StrucSig, 0x61417272);
			st_dword(fs->win + FSI_Free_Count, fs->free_clst);
			st_dword(fs->win + FSI_Nxt_Free, fs->last_clst);

			fs->winsect = fs->volbase + 1;
			disk_write(fs->drv, fs->win, fs->winsect, 1);
			fs->fsi_flag = 0;
		}
		/* Attente fin des writes pending sur lecteur physique */
		if (disk_ioctl(fs->drv, CTRL_SYNC, 0) != RES_OK) res = FR_DISK_ERR;
	}

	return res;
}

#endif

static
DWORD clust2sect (	/* Retour secteur ; 0 cluster invalide */
	FATFS* fs,		/* Objet système de fichiers */
	DWORD clst
)
{
	clst -= 2;
	if (clst >= fs->n_fatent - 2) return 0;		/* Cluster invalide */
	return clst * fs->csize + fs->database;
}

static
DWORD get_fat (	/* 0xFFFFFFFF erreur disque, 1 interne, 2..0x7FFFFFFF état cluster */
	_FDID* obj,
	DWORD clst
)
{
	UINT wc, bc;
	DWORD val;
	FATFS *fs = obj->fs;

	if (clst < 2 || clst >= fs->n_fatent) {
		val = 1;

	} else {
		val = 0xFFFFFFFF;	/* Valeur par défaut pour erreur disque */

		switch (fs->fs_type) {
		case FS_FAT12 :
			bc = (UINT)clst; bc += bc / 2;
			if (move_window(fs, fs->fatbase + (bc / SS(fs))) != FR_OK) break;
			wc = fs->win[bc++ % SS(fs)];
			if (move_window(fs, fs->fatbase + (bc / SS(fs))) != FR_OK) break;
			wc |= fs->win[bc % SS(fs)] << 8;
			val = (clst & 1) ? (wc >> 4) : (wc & 0xFFF);
			break;

		case FS_FAT16 :
			if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 2))) != FR_OK) break;
			val = ld_word(fs->win + clst * 2 % SS(fs));
			break;

		case FS_FAT32 :
			if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 4))) != FR_OK) break;
			val = ld_dword(fs->win + clst * 4 % SS(fs)) & 0x0FFFFFFF;
			break;
#if _FS_EXFAT
		case FS_EXFAT :
			if (obj->objsize) {
				DWORD cofs = clst - obj->sclust;	/* Offset depuis cluster initial */
				DWORD clen = (DWORD)((obj->objsize - 1) / SS(fs)) / fs->csize;	/* Nombre de clusters moins 1 */

				if (obj->stat == 2) {	/* Aucune chaîne FAT valide */
					if (cofs <= clen) {
						val = (cofs == clen) ? 0x7FFFFFFF : clst + 1;
						break;
					}
				}
				if (obj->stat == 3 && cofs < obj->n_cont) {	/* Premier fragment */
					val = clst + 1;
					break;
				}
				if (obj->stat != 2) {	/* Lecture FAT uniquement si chaîne valide */
					if (obj->n_frag != 0) {	/* Bord de croissance */
						val = 0x7FFFFFFF;	/* Génération EOC */
					} else {
						if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 4))) != FR_OK) break;
						val = ld_dword(fs->win + clst * 4 % SS(fs)) & 0x7FFFFFFF;
					}
					break;
				}
			}
			__attribute__ ((fallthrough));
#endif
		default:
			val = 1;
		}
	}

	return val;
}

#if !_FS_READONLY

static
FRESULT put_fat (	/* FR_OK(0) succès ; sinon erreur */
	FATFS* fs,
	DWORD clst,
	DWORD val
)
{
	UINT bc;
	BYTE *p;
	FRESULT res = FR_INT_ERR;

	if (clst >= 2 && clst < fs->n_fatent) {
		switch (fs->fs_type) {
		case FS_FAT12 :	/* Champs de bits */
			bc = (UINT)clst; bc += bc / 2;
			res = move_window(fs, fs->fatbase + (bc / SS(fs)));
			if (res != FR_OK) break;
			p = fs->win + bc++ % SS(fs);
			*p = (clst & 1) ? ((*p & 0x0F) | ((BYTE)val << 4)) : (BYTE)val;
			fs->wflag = 1;
			res = move_window(fs, fs->fatbase + (bc / SS(fs)));
			if (res != FR_OK) break;
			p = fs->win + bc % SS(fs);
			*p = (clst & 1) ? (BYTE)(val >> 4) : ((*p & 0xF0) | ((BYTE)(val >> 8) & 0x0F));
			fs->wflag = 1;
			break;

		case FS_FAT16 :	/* Champs alignés WORD */
			res = move_window(fs, fs->fatbase + (clst / (SS(fs) / 2)));
			if (res != FR_OK) break;
			st_word(fs->win + clst * 2 % SS(fs), (WORD)val);
			fs->wflag = 1;
			break;

		case FS_FAT32 :	/* Champs alignés DWORD */
#if _FS_EXFAT
		case FS_EXFAT :
#endif
			res = move_window(fs, fs->fatbase + (clst / (SS(fs) / 4)));
			if (res != FR_OK) break;
			if (!_FS_EXFAT || fs->fs_type != FS_EXFAT) {
				val = (val & 0x0FFFFFFF) | (ld_dword(fs->win + clst * 4 % SS(fs)) & 0xF0000000);
			}
			st_dword(fs->win + clst * 4 % SS(fs), val);
			fs->wflag = 1;
			break;
		}
	}
	return res;
}

#endif

#if _FS_EXFAT && !_FS_READONLY

/* exFAT : FAT et allocation bitmap */

static
DWORD find_bitmap (	/* Retour 0 absent, >=2 cluster trouvé, 0xFFFFFFFF erreur disque */
	FATFS* fs,	/* Objet système de fichiers */
	DWORD clst,
	DWORD ncl	/* Nombre de clusters contigus requis, depuis 1 */
)
{
	BYTE bm, bv;
	UINT i;
	DWORD val, scl, ctr;

	clst -= 2;	/* Premier bit bitmap associé au cluster 2 */
	if (clst >= fs->n_fatent - 2) clst = 0;
	scl = val = clst; ctr = 0;
	for (;;) {
		if (move_window(fs, fs->database + val / 8 / SS(fs)) != FR_OK) return 0xFFFFFFFF;	/* Hypothèse : bitmap au début du cluster heap */
		i = val / 8 % SS(fs); bm = 1 << (val % 8);
		do {
			do {
				bv = fs->win[i] & bm; bm <<= 1;
				if (++val >= fs->n_fatent - 2) {	/* Cluster suivant avec wrap-around */
					val = 0; bm = 0; i = SS(fs);
				}
				if (!bv) {
					if (++ctr == ncl) return scl + 2;
				} else {
					scl = val; ctr = 0;		/* Cluster occupé : reprise du scan */
				}
				if (val == clst) return 0;
			} while (bm);
			bm = 1;
		} while (++i < SS(fs));
	}
}

static
FRESULT change_bitmap (
	FATFS* fs,	/* Objet système de fichiers */
	DWORD clst,
	DWORD ncl,
	int bv		/* Bit à fixer : 0 ou 1 */
)
{
	BYTE bm;
	UINT i;
	DWORD sect;

	clst -= 2;	/* Premier bit associé au cluster 2 */
	sect = fs->database + clst / 8 / SS(fs);	/* Secteur bitmap supposé au début du cluster heap */
	i = clst / 8 % SS(fs);						/* Offset byte dans le secteur */
	bm = 1 << (clst % 8);						/* Masque bit dans le byte */
	for (;;) {
		if (move_window(fs, sect++) != FR_OK) return FR_DISK_ERR;
		do {
			do {
				if (bv == (int)((fs->win[i] & bm) != 0)) return FR_INT_ERR;
				fs->win[i] ^= bm;
				fs->wflag = 1;
				if (--ncl == 0) return FR_OK;
			} while (bm <<= 1);
			bm = 1;
		} while (++i < SS(fs));
		i = 0;
	}
}

static
FRESULT fill_first_frag (
	_FDID* obj
)
{
	FRESULT res;
	DWORD cl, n;

	if (obj->stat == 3) {
		for (cl = obj->sclust, n = obj->n_cont; n; cl++, n--) {
			res = put_fat(obj->fs, cl, cl + 1);
			if (res != FR_OK) return res;
		}
		obj->stat = 0;	/* État modifié : chaîne FAT valide */
	}
	return FR_OK;
}

static
FRESULT fill_last_frag (
	_FDID* obj,
	DWORD lcl,	/* Dernier cluster du fragment */
	DWORD term	/* Valeur de la dernière entrée FAT */
)
{
	FRESULT res;

	while (obj->n_frag > 0) {
		res = put_fat(obj->fs, lcl - obj->n_frag + 1, (obj->n_frag > 1) ? lcl - obj->n_frag + 2 : term);
		if (res != FR_OK) return res;
		obj->n_frag--;
	}
	return FR_OK;
}

#endif

#if !_FS_READONLY

static
FRESULT remove_chain (	/* FR_OK(0) succès ; sinon erreur */
	_FDID* obj,
	DWORD clst,
	DWORD pclst			/* Cluster précédent ; 0 chaîne entière */
)
{
	FRESULT res = FR_OK;
	DWORD nxt;
	FATFS *fs = obj->fs;
#if _FS_EXFAT || _USE_TRIM
	DWORD scl = clst, ecl = clst;
#endif
#if _USE_TRIM
	DWORD rt[2];
#endif

	if (clst < 2 || clst >= fs->n_fatent) return FR_INT_ERR;

	/* Marquer cluster précédent EOC s’il existe */
	if (pclst && (!_FS_EXFAT || fs->fs_type != FS_EXFAT || obj->stat != 2)) {
		res = put_fat(fs, pclst, 0xFFFFFFFF);
		if (res != FR_OK) return res;
	}

	do {
		nxt = get_fat(obj, clst);
		if (nxt == 0) break;				/* Cluster vide */
		if (nxt == 1) return FR_INT_ERR;	/* Erreur interne */
		if (nxt == 0xFFFFFFFF) return FR_DISK_ERR;	/* Erreur disque */
		if (!_FS_EXFAT || fs->fs_type != FS_EXFAT) {
			res = put_fat(fs, clst, 0);		/* Marquer cluster libre dans FAT */
			if (res != FR_OK) return res;
		}
		if (fs->free_clst < fs->n_fatent - 2) {
			fs->free_clst++;
			fs->fsi_flag |= 1;
		}
#if _FS_EXFAT || _USE_TRIM
		if (ecl + 1 == nxt) {
			ecl = nxt;
		} else {				/* Fin du bloc de clusters contigus */
#if _FS_EXFAT
			if (fs->fs_type == FS_EXFAT) {
				res = change_bitmap(fs, scl, ecl - scl + 1, 0);	/* Marquer bloc clusters libre dans bitmap */
				if (res != FR_OK) return res;
			}
#endif
#if _USE_TRIM
			rt[0] = clust2sect(fs, scl);					/* Secteur début */
			rt[1] = clust2sect(fs, ecl) + fs->csize - 1;	/* Secteur fin */
			disk_ioctl(fs->drv, CTRL_TRIM, rt);				/* Notification device : bloc effaçable */
#endif
			scl = ecl = nxt;
		}
#endif
		clst = nxt;
	} while (clst < fs->n_fatent);	/* Parcours jusqu’au dernier lien */

#if _FS_EXFAT
	if (fs->fs_type == FS_EXFAT) {
		if (pclst == 0) {
			obj->stat = 0;		/* État objet réinitialisé */
		} else {
			if (obj->stat == 3 && pclst >= obj->sclust && pclst <= obj->sclust + obj->n_cont) {
				obj->stat = 2;	/* État objet contigu */
			}
		}
	}
#endif
	return FR_OK;
}

static
DWORD create_chain (	/* Retour 0 plein, 1 erreur interne, 0xFFFFFFFF disque, >=2 cluster */
	_FDID* obj,
	DWORD clst			/* Cluster à étendre ; 0 crée une nouvelle chaîne */
)
{
	DWORD cs, ncl, scl;
	FRESULT res;
	FATFS *fs = obj->fs;

	if (clst == 0) {
		scl = fs->last_clst;
		if (scl == 0 || scl >= fs->n_fatent) scl = 1;
	}
	else {
		cs = get_fat(obj, clst);
		if (cs < 2) return 1;				/* Valeur FAT invalide */
		if (cs == 0xFFFFFFFF) return cs;	/* Erreur disque */
		if (cs < fs->n_fatent) return cs;	/* Cluster suivant déjà existant */
		scl = clst;
	}

#if _FS_EXFAT
	if (fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
		ncl = find_bitmap(fs, scl, 1);
		if (ncl == 0 || ncl == 0xFFFFFFFF) return ncl;	/* Aucun cluster libre ou erreur hardware */
		res = change_bitmap(fs, ncl, 1, 1);			/* Marquer cluster occupé */
		if (res == FR_INT_ERR) return 1;
		if (res == FR_DISK_ERR) return 0xFFFFFFFF;
		if (clst == 0) {
			obj->stat = 2;							/* État contigu */
		} else {									/* Chaîne étendue */
			if (obj->stat == 2 && ncl != scl + 1) {
				obj->n_cont = scl - obj->sclust;	/* Taille de la partie contiguë */
				obj->stat = 3;						/* État nouvellement fragmenté */
			}
		}
		if (obj->stat != 2) {
			if (ncl == clst + 1) {
				obj->n_frag = obj->n_frag ? obj->n_frag + 1 : 2;	/* Extension du dernier fragment */
			} else {				/* Nouveau fragment */
				if (obj->n_frag == 0) obj->n_frag = 1;
				res = fill_last_frag(obj, clst, ncl);	/* Dernier fragment FAT lié au nouveau */
				if (res == FR_OK) obj->n_frag = 1;
			}
		}
	} else
#endif
	{	/* Sur volume FAT12/16/32 */
		ncl = scl;
		for (;;) {
			ncl++;
			if (ncl >= fs->n_fatent) {
				ncl = 2;
				if (ncl > scl) return 0;	/* Aucun cluster libre */
			}
			cs = get_fat(obj, ncl);
			if (cs == 0) break;
			if (cs == 1 || cs == 0xFFFFFFFF) return cs;
			if (ncl == scl) return 0;		/* Aucun cluster libre */
		}
		res = put_fat(fs, ncl, 0xFFFFFFFF);	/* Marquer nouveau cluster EOC */
		if (res == FR_OK && clst != 0) {
			res = put_fat(fs, clst, ncl);	/* Lien depuis cluster précédent si nécessaire */
		}
	}

	if (res == FR_OK) {			/* Mise à jour FSINFO uniquement si succès */
		fs->last_clst = ncl;
		if (fs->free_clst <= fs->n_fatent - 2) fs->free_clst--;
		fs->fsi_flag |= 1;
	} else {
		ncl = (res == FR_DISK_ERR) ? 0xFFFFFFFF : 1;	/* Génération status erreur après échec */
	}

	return ncl;		/* Retour nouveau cluster ou status erreur */
}

#endif

#if _USE_FASTSEEK

static
DWORD clmt_clust (	/* Retour <2 erreur ; >=2 numéro cluster */
	FIL* fp,		/* Pointeur objet fichier */
	FSIZE_t ofs		/* Offset fichier à convertir en cluster */
)
{
	DWORD cl, ncl, *tbl;
	FATFS *fs = fp->obj.fs;

	tbl = fp->cltbl + 1;
	cl = (DWORD)(ofs / SS(fs) / fs->csize);
	for (;;) {
		ncl = *tbl++;			/* Nombre de clusters dans le fragment */
		if (ncl == 0) return 0;	/* Fin de table : erreur */
		if (cl < ncl) break;
		cl -= ncl; tbl++;
	}
	return cl + *tbl;
}

#endif

static
FRESULT dir_sdi (	/* FR_OK(0) succès ; sinon erreur */
	DIR* dp,
	DWORD ofs
)
{
	DWORD csz, clst;
	FATFS *fs = dp->obj.fs;

	if (ofs >= (DWORD)((_FS_EXFAT && fs->fs_type == FS_EXFAT) ? MAX_DIR_EX : MAX_DIR) || ofs % SZDIRE) {	/* Validation offset et alignement */
		return FR_INT_ERR;
	}
	dp->dptr = ofs;
	clst = dp->obj.sclust;		/* Cluster initial de table ; 0 racine */
	if (clst == 0 && fs->fs_type >= FS_FAT32) {	/* Remplacer cluster 0 par cluster racine */
		clst = fs->dirbase;
		if (_FS_EXFAT) dp->obj.stat = 0;	/* exFAT : répertoire racine doté d’une chaîne FAT */
	}

	if (clst == 0) {	/* Table statique racine FAT12/16 */
		if (ofs / SZDIRE >= fs->n_rootdir)	return FR_INT_ERR;	/* Index hors limites */
		dp->sect = fs->dirbase;

	} else {			/* Table dynamique : sous-répertoire ou racine FAT32+ */
		csz = (DWORD)fs->csize * SS(fs);	/* Bytes par cluster */
		while (ofs >= csz) {
			clst = get_fat(&dp->obj, clst);
			if (clst == 0xFFFFFFFF) return FR_DISK_ERR;	/* Erreur disque */
			if (clst < 2 || clst >= fs->n_fatent) return FR_INT_ERR;	/* Fin de table ou erreur interne */
			ofs -= csz;
		}
		dp->sect = clust2sect(fs, clst);
	}
	dp->clust = clst;
	if (!dp->sect) return FR_INT_ERR;
	dp->sect += ofs / SS(fs);			/* Secteur de l’entrée répertoire */
	dp->dir = fs->win + (ofs % SS(fs));	/* Pointer d’entrée dans win[] */

	return FR_OK;
}

static
FRESULT dir_next (	/* FR_OK succès, FR_NO_FILE fin de table, FR_DENIED extension refusée */
	DIR* dp,		/* Pointeur objet répertoire */
	int stretch		/* 0 interdit extension table ; 1 autorise si nécessaire */
)
{
	DWORD ofs, clst;
	FATFS *fs = dp->obj.fs;
#if !_FS_READONLY
	UINT n;
#endif

	ofs = dp->dptr + SZDIRE;
	if (!dp->sect || ofs >= (DWORD)((_FS_EXFAT && fs->fs_type == FS_EXFAT) ? MAX_DIR_EX : MAX_DIR)) return FR_NO_FILE;	/* EOT si offset atteint la valeur maximale */

	if (ofs % SS(fs) == 0) {
		dp->sect++;

		if (!dp->clust) {
			if (ofs / SZDIRE >= fs->n_rootdir) {	/* EOT à la fin de la table statique */
				dp->sect = 0; return FR_NO_FILE;
			}
		}
		else {
			if ((ofs / SS(fs) & (fs->csize - 1)) == 0) {
				clst = get_fat(&dp->obj, dp->clust);
				if (clst <= 1) return FR_INT_ERR;
				if (clst == 0xFFFFFFFF) return FR_DISK_ERR;		/* Erreur disque */
				if (clst >= fs->n_fatent) {
#if !_FS_READONLY
					if (!stretch) {								/* EOT si extension interdite */
						dp->sect = 0; return FR_NO_FILE;
					}
					clst = create_chain(&dp->obj, dp->clust);
					if (clst == 0) return FR_DENIED;			/* Aucun cluster libre */
					if (clst == 1) return FR_INT_ERR;
					if (clst == 0xFFFFFFFF) return FR_DISK_ERR;	/* Erreur disque */
					/* Nettoyage de la table étendue */
					if (_FS_EXFAT) dp->obj.stat |= 4;			/* Répertoire à mettre à jour */
					if (sync_window(fs) != FR_OK) return FR_DISK_ERR;
					mem_set(fs->win, 0, SS(fs));
					for (n = 0, fs->winsect = clust2sect(fs, clst); n < fs->csize; n++, fs->winsect++) {	/* Nouveau cluster rempli de zéros */
						fs->wflag = 1;
						if (sync_window(fs) != FR_OK) return FR_DISK_ERR;
					}
					fs->winsect -= n;
#else
					if (!stretch) dp->sect = 0;
					dp->sect = 0; return FR_NO_FILE;			/* Signalement EOT */
#endif
				}
				dp->clust = clst;
				dp->sect = clust2sect(fs, clst);
			}
		}
	}
	dp->dptr = ofs;
	dp->dir = fs->win + ofs % SS(fs);	/* Pointer d’entrée dans win[] */

	return FR_OK;
}

#if !_FS_READONLY

static
FRESULT dir_alloc (	/* FR_OK(0) succès ; sinon erreur */
	DIR* dp,		/* Pointeur objet répertoire */
	UINT nent		/* Nombre d’entrées contiguës à allouer */
)
{
	FRESULT res;
	UINT n;
	FATFS *fs = dp->obj.fs;

	res = dir_sdi(dp, 0);
	if (res == FR_OK) {
		n = 0;
		do {
			res = move_window(fs, dp->sect);
			if (res != FR_OK) break;
#if _FS_EXFAT
			if ((fs->fs_type == FS_EXFAT) ? (int)((dp->dir[XDIR_Type] & 0x80) == 0) : (int)(dp->dir[DIR_Name] == DDEM || dp->dir[DIR_Name] == 0)) {
#else
			if (dp->dir[DIR_Name] == DDEM || dp->dir[DIR_Name] == 0) {
#endif
				if (++n == nent) break;
			} else {
				n = 0;					/* Entrée occupée : reprise recherche */
			}
			res = dir_next(dp, 1);
		} while (res == FR_OK);	/* Entrée suivante, extension table autorisée */
	}

	if (res == FR_NO_FILE) res = FR_DENIED;	/* Aucune entrée répertoire disponible */
	return res;
}

#endif

static
DWORD ld_clust (	/* Retour cluster initial de l’entrée SFN */
	FATFS* fs,
	const BYTE* dir
)
{
	DWORD cl;

	cl = ld_word(dir + DIR_FstClusLO);
	if (fs->fs_type == FS_FAT32) {
		cl |= (DWORD)ld_word(dir + DIR_FstClusHI) << 16;
	}

	return cl;
}

#if !_FS_READONLY
static
void st_clust (
	FATFS* fs,
	BYTE* dir,
	DWORD cl
)
{
	st_word(dir + DIR_FstClusLO, (WORD)cl);
	if (fs->fs_type == FS_FAT32) {
		st_word(dir + DIR_FstClusHI, (WORD)(cl >> 16));
	}
}
#endif

#if _USE_LFN != 0

static
const BYTE LfnOfs[] = {1,3,5,7,9,14,16,18,20,22,24,28,30};	/* Offset caractères LFN dans entrée répertoire */

static
int cmp_lfn (				/* 1 correspondance ; 0 incompatibilité */
	const WCHAR* lfnbuf,
	BYTE* dir
)
{
	UINT i, s;
	WCHAR wc, uc;

	if (ld_word(dir + LDIR_FstClusLO) != 0) return 0;

	i = ((dir[LDIR_Ord] & 0x3F) - 1) * 13;	/* Offset dans le buffer LFN */

	for (wc = 1, s = 0; s < 13; s++) {
		uc = ld_word(dir + LfnOfs[s]);
		if (wc) {
			if (i >= _MAX_LFN || ff_wtoupper(uc) != ff_wtoupper(lfnbuf[i++])) {
				return 0;
			}
			wc = uc;
		} else {
			if (uc != 0xFFFF) return 0;
		}
	}

	if ((dir[LDIR_Ord] & LLEF) && wc && lfnbuf[i]) return 0;	/* Dernier segment correspondant mais longueur incohérente */

	return 1;
}

#if _FS_MINIMIZE <= 1 || _FS_RPATH >= 2 || _USE_LABEL || _FS_EXFAT

static
int pick_lfn (			/* 1 succès ; 0 overflow buffer ou entrée LFN invalide */
	WCHAR* lfnbuf,
	BYTE* dir
)
{
	UINT i, s;
	WCHAR wc, uc;

	if (ld_word(dir + LDIR_FstClusLO) != 0) return 0;	/* LDIR_FstClusLO obligatoirement zéro */

	i = ((dir[LDIR_Ord] & ~LLEF) - 1) * 13;	/* Offset dans le buffer LFN */

	for (wc = 1, s = 0; s < 13; s++) {
		uc = ld_word(dir + LfnOfs[s]);
		if (wc) {
			if (i >= _MAX_LFN) return 0;
			lfnbuf[i++] = wc = uc;
		} else {
			if (uc != 0xFFFF) return 0;
		}
	}

	if (dir[LDIR_Ord] & LLEF) {				/* Terminator uniquement dans la dernière partie LFN */
		if (i >= _MAX_LFN) return 0;
		lfnbuf[i] = 0;
	}

	return 1;
}
#endif

#if !_FS_READONLY

static
void put_lfn (
	const WCHAR* lfn,
	BYTE* dir,
	BYTE ord,			/* Ordre LFN de 1 à 20 */
	BYTE sum			/* Checksum du SFN associé */
)
{
	UINT i, s;
	WCHAR wc;

	dir[LDIR_Chksum] = sum;
	dir[LDIR_Attr] = AM_LFN;
	dir[LDIR_Type] = 0;
	st_word(dir + LDIR_FstClusLO, 0);

	i = (ord - 1) * 13;
	s = wc = 0;
	do {
		if (wc != 0xFFFF) wc = lfn[i++];
		st_word(dir + LfnOfs[s], wc);
		if (wc == 0) wc = 0xFFFF;		/* Padding des emplacements restants */
	} while (++s < 13);
	if (wc == 0xFFFF || !lfn[i]) ord |= LLEF;	/* Dernière partie LFN au début de la séquence */
	dir[LDIR_Ord] = ord;
}

#endif
#endif

#if _USE_LFN != 0 && !_FS_READONLY

static
void gen_numname (
	BYTE* dst,
	const BYTE* src,
	const WCHAR* lfn,
	UINT seq
)
{
	BYTE ns[8], c;
	UINT i, j;
	WCHAR wc;
	DWORD sr;

	mem_cpy(dst, src, 11);

	if (seq > 5) {	/* Collisions nombreuses : hash plutôt que numéro séquentiel */
		sr = seq;
		while (*lfn) {
			wc = *lfn++;
			for (i = 0; i < 16; i++) {
				sr = (sr << 1) + (wc & 1);
				wc >>= 1;
				if (sr & 0x10000) sr ^= 0x11021;
			}
		}
		seq = (UINT)sr;
	}

	i = 7;
	do {
		c = (BYTE)((seq % 16) + '0');
		if (c > '9') c += 7;
		ns[i--] = c;
		seq /= 16;
	} while (seq);
	ns[i] = '~';

	for (j = 0; j < i && dst[j] != ' '; j++) {
		if (IsDBCS1(dst[j])) {
			if (j == i - 1) break;
			j++;
		}
	}
	do {
		dst[j++] = (i < 8) ? ns[i++] : ' ';
	} while (j < 8);
}
#endif

#if _USE_LFN != 0

static
BYTE sum_sfn (
	const BYTE* dir
)
{
	BYTE sum = 0;
	UINT n = 11;

	do {
		sum = (sum >> 1) + (sum << 7) + *dir++;
	} while (--n);
	return sum;
}

#endif

#if _FS_EXFAT

static
WORD xdir_sum (
	const BYTE* dir
)
{
	UINT i, szblk;
	WORD sum;

	szblk = (dir[XDIR_NumSec] + 1) * SZDIRE;
	for (i = sum = 0; i < szblk; i++) {
		if (i == XDIR_SetSum) {	/* Champ checksum exclu du calcul */
			i++;
		} else {
			sum = ((sum & 1) ? 0x8000 : 0) + (sum >> 1) + dir[i];
		}
	}
	return sum;
}

static
WORD xname_sum (
	const WCHAR* name
)
{
	WCHAR chr;
	WORD sum = 0;

	while ((chr = *name++) != 0) {
		chr = ff_wtoupper(chr);		/* Hash du nom sans distinction de casse */
		sum = ((sum & 1) ? 0x8000 : 0) + (sum >> 1) + (chr & 0xFF);
		sum = ((sum & 1) ? 0x8000 : 0) + (sum >> 1) + (chr >> 8);
	}
	return sum;
}

#if !_FS_READONLY && _USE_MKFS
static
DWORD xsum32 (
	BYTE  dat,
	DWORD sum
)
{
	sum = ((sum & 1) ? 0x80000000 : 0) + (sum >> 1) + dat;
	return sum;
}
#endif

#if _FS_MINIMIZE <= 1 || _FS_RPATH >= 2

static
void get_xdir_info (
	BYTE* dirb,			/* Bloc entrées répertoire 85+C0+C1s */
	FILINFO* fno
)
{
	UINT di, si;
	WCHAR w;
#if !_LFN_UNICODE
	UINT nc;
#endif

	di = 0;
#if _LFN_UNICODE
	for (si = SZDIRE * 2; di < dirb[XDIR_NumName]; si += 2, di++) {
		if ((si % SZDIRE) == 0) si += 2;		/* Ignorer champ type entrée */
		w = ld_word(dirb + si);
		if (di >= _MAX_LFN) { di = 0; break; }	/* Overflow buffer : nom objet inaccessible */
		fno->fname[di] = w;
	}
#else
	for (si = SZDIRE * 2, nc = 0; nc < dirb[XDIR_NumName]; si += 2, nc++) {
		if ((si % SZDIRE) == 0) si += 2;		/* Ignorer champ type entrée */
		w = ff_convert(ld_word(dirb + si), 0);
		if (_DF1S && w >= 0x100) {				/* DBCS : deux bytes ; toujours false en SBCS */
			fno->fname[di++] = (char)(w >> 8);
		}
		if (w == 0 || di >= _MAX_LFN) { di = 0; break; }	/* Caractère invalide ou overflow : nom inaccessible */
		fno->fname[di++] = (char)w;
	}
#endif
	if (di == 0) fno->fname[di++] = '?';
	fno->fname[di] = 0;

	fno->altname[0] = 0;							/* Aucun SFN */
	fno->fattrib = dirb[XDIR_Attr];
	fno->fsize = (fno->fattrib & AM_DIR) ? 0 : ld_qword(dirb + XDIR_FileSize);
	fno->ftime = ld_word(dirb + XDIR_ModTime + 0);
	fno->fdate = ld_word(dirb + XDIR_ModTime + 2);
}

#endif

static
FRESULT load_xdir (	/* FR_INT_ERR : bloc d’entrées invalide */
	DIR* dp			/* Répertoire de lecture positionné sur entrée 85 */
)
{
	FRESULT res;
	UINT i, sz_ent;
	BYTE* dirb = dp->obj.fs->dirbuf;	/* Bloc mémoire d’entrées 85+C0+C1s */

	/* Chargement entrée 85 */
	res = move_window(dp->obj.fs, dp->sect);
	if (res != FR_OK) return res;
	if (dp->dir[XDIR_Type] != 0x85) return FR_INT_ERR;
	mem_cpy(dirb + 0, dp->dir, SZDIRE);
	sz_ent = (dirb[XDIR_NumSec] + 1) * SZDIRE;
	if (sz_ent < 3 * SZDIRE || sz_ent > 19 * SZDIRE) return FR_INT_ERR;

	/* Chargement entrée C0 */
	res = dir_next(dp, 0);
	if (res != FR_OK) return res;
	res = move_window(dp->obj.fs, dp->sect);
	if (res != FR_OK) return res;
	if (dp->dir[XDIR_Type] != 0xC0) return FR_INT_ERR;
	mem_cpy(dirb + SZDIRE, dp->dir, SZDIRE);
	if (MAXDIRB(dirb[XDIR_NumName]) > sz_ent) return FR_INT_ERR;

	/* Chargement entrées C1 */
	i = SZDIRE * 2;	/* Offset C1 */
	do {
		res = dir_next(dp, 0);
		if (res != FR_OK) return res;
		res = move_window(dp->obj.fs, dp->sect);
		if (res != FR_OK) return res;
		if (dp->dir[XDIR_Type] != 0xC1) return FR_INT_ERR;
		if (i < MAXDIRB(_MAX_LFN)) mem_cpy(dirb + i, dp->dir, SZDIRE);
	} while ((i += SZDIRE) < sz_ent);

	/* Validation seulement si nom objet accessible */
	if (i <= MAXDIRB(_MAX_LFN)) {
		if (xdir_sum(dirb) != ld_word(dirb + XDIR_SetSum)) return FR_INT_ERR;
	}
	return FR_OK;
}

#if !_FS_READONLY || _FS_RPATH != 0

static
FRESULT load_obj_dir (
	DIR* dp,
	const _FDID* obj
)
{
	FRESULT res;

	dp->obj.fs = obj->fs;
	dp->obj.sclust = obj->c_scl;
	dp->obj.stat = (BYTE)obj->c_size;
	dp->obj.objsize = obj->c_size & 0xFFFFFF00;
	dp->blk_ofs = obj->c_ofs;

	res = dir_sdi(dp, dp->blk_ofs);
	if (res == FR_OK) {
		res = load_xdir(dp);
	}
	return res;
}
#endif

#if !_FS_READONLY

static
FRESULT store_xdir (
	DIR* dp				/* Pointeur objet répertoire */
)
{
	FRESULT res;
	UINT nent;
	BYTE* dirb = dp->obj.fs->dirbuf;	/* Bloc entrées répertoire 85+C0+C1s */

	st_word(dirb + XDIR_SetSum, xdir_sum(dirb));
	nent = dirb[XDIR_NumSec] + 1;

	res = dir_sdi(dp, dp->blk_ofs);
	while (res == FR_OK) {
		res = move_window(dp->obj.fs, dp->sect);
		if (res != FR_OK) break;
		mem_cpy(dp->dir, dirb, SZDIRE);
		dp->obj.fs->wflag = 1;
		if (--nent == 0) break;
		dirb += SZDIRE;
		res = dir_next(dp, 0);
	}
	return (res == FR_OK || res == FR_DISK_ERR) ? res : FR_INT_ERR;
}

static
void create_xdir (
	BYTE* dirb,
	const WCHAR* lfn	/* Nom fichier terminé par NUL */
)
{
	UINT i;
	BYTE nb, nc;
	WCHAR chr;

	/* Création entrée 85+C0 */
	mem_set(dirb, 0, 2 * SZDIRE);
	dirb[XDIR_Type] = 0x85;
	dirb[XDIR_Type + SZDIRE] = 0xC0;

	/* Création entrées C1 */
	nc = 0; nb = 1; chr = 1; i = SZDIRE * 2;
	do {
		dirb[i++] = 0xC1; dirb[i++] = 0;	/* Type entrée C1 */
		do {
			if (chr && (chr = lfn[nc]) != 0) nc++;
			st_word(dirb + i, chr);
		} while ((i += 2) % SZDIRE != 0);
		nb++;
	} while (lfn[nc]);

	dirb[XDIR_NumName] = nc;
	dirb[XDIR_NumSec] = nb;
	st_word(dirb + XDIR_NameHash, xname_sum(lfn));
}

#endif
#endif

#if _FS_MINIMIZE <= 1 || _FS_RPATH >= 2 || _USE_LABEL || _FS_EXFAT

static
FRESULT dir_read (
	DIR* dp,		/* Pointeur objet répertoire */
	int vol			/* Filtre : 0 fichier/répertoire, 1 label volume */
)
{
	FRESULT res = FR_NO_FILE;
	FATFS *fs = dp->obj.fs;
	BYTE a, c;
#if _USE_LFN != 0
	BYTE ord = 0xFF, sum = 0xFF;
#endif

	while (dp->sect) {
		res = move_window(fs, dp->sect);
		if (res != FR_OK) break;
		c = dp->dir[DIR_Name];
		if (c == 0) {
			res = FR_NO_FILE; break;
		}
#if _FS_EXFAT
		if (fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
			if (_USE_LABEL && vol) {
				if (c == 0x83) break;
			} else {
				if (c == 0x85) {
					dp->blk_ofs = dp->dptr;
					res = load_xdir(dp);
					if (res == FR_OK) {
						dp->obj.attr = fs->dirbuf[XDIR_Attr] & AM_MASK;
					}
					break;
				}
			}
		} else
#endif
		{	/* Sur volume FAT12/16/32 */
			dp->obj.attr = a = dp->dir[DIR_Attr] & AM_MASK;
#if _USE_LFN != 0	/* Configuration LFN */
			if (c == DDEM || c == '.' || (int)((a & ~AM_ARC) == AM_VOL) != vol) {	/* Entrée sans données valides */
				ord = 0xFF;
			} else {
				if (a == AM_LFN) {			/* Entrée LFN détectée */
					if (c & LLEF) {
						sum = dp->dir[LDIR_Chksum];
						c &= (BYTE)~LLEF; ord = c;
						dp->blk_ofs = dp->dptr;
					}

					ord = (c == ord && sum == dp->dir[LDIR_Chksum] && pick_lfn(fs->lfnbuf, dp->dir)) ? ord - 1 : 0xFF;
				} else {					/* Entrée SFN détectée */
					if (ord || sum != sum_sfn(dp->dir)) {
						dp->blk_ofs = 0xFFFFFFFF;			/* Aucun LFN */
					}
					break;
				}
			}
#else		/* Configuration sans LFN */
			if (c != DDEM && c != '.' && a != AM_LFN && (int)((a & ~AM_ARC) == AM_VOL) == vol) {
				break;
			}
#endif
		}
		res = dir_next(dp, 0);
		if (res != FR_OK) break;
	}

	if (res != FR_OK) dp->sect = 0;		/* Erreur ou EOT : arrêt de lecture */
	return res;
}

#endif

static
FRESULT dir_find (	/* FR_OK(0) succès ; sinon erreur */
	DIR* dp
)
{
	FRESULT res;
	FATFS *fs = dp->obj.fs;
	BYTE c;
#if _USE_LFN != 0
	BYTE a, ord, sum;
#endif

	res = dir_sdi(dp, 0);
	if (res != FR_OK) return res;
#if _FS_EXFAT
	if (fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
		BYTE nc;
		UINT di, ni;
		WORD hash = xname_sum(fs->lfnbuf);

		while ((res = dir_read(dp, 0)) == FR_OK) {
#if _MAX_LFN < 255
			if (fs->dirbuf[XDIR_NumName] > _MAX_LFN) continue;			/* Ignorer comparaison si nom inaccessible */
#endif
			if (ld_word(fs->dirbuf + XDIR_NameHash) != hash) continue;	/* Ignorer comparaison si hash différent */
			for (nc = fs->dirbuf[XDIR_NumName], di = SZDIRE * 2, ni = 0; nc; nc--, di += 2, ni++) {
				if ((di % SZDIRE) == 0) di += 2;
				if (ff_wtoupper(ld_word(fs->dirbuf + di)) != ff_wtoupper(fs->lfnbuf[ni])) break;
			}
			if (nc == 0 && !fs->lfnbuf[ni]) break;
		}
		return res;
	}
#endif
	/* Sur volume FAT12/16/32 */
#if _USE_LFN != 0
	ord = sum = 0xFF; dp->blk_ofs = 0xFFFFFFFF;
#endif
	do {
		res = move_window(fs, dp->sect);
		if (res != FR_OK) break;
		c = dp->dir[DIR_Name];
		if (c == 0) { res = FR_NO_FILE; break; }
#if _USE_LFN != 0	/* Configuration LFN */
		dp->obj.attr = a = dp->dir[DIR_Attr] & AM_MASK;
		if (c == DDEM || ((a & AM_VOL) && a != AM_LFN)) {	/* Entrée sans données valides */
			ord = 0xFF; dp->blk_ofs = 0xFFFFFFFF;
		} else {
			if (a == AM_LFN) {			/* Entrée LFN détectée */
				if (!(dp->fn[NSFLAG] & NS_NOLFN)) {
					if (c & LLEF) {
						sum = dp->dir[LDIR_Chksum];
						c &= (BYTE)~LLEF; ord = c;	/* Ordre initial du LFN */
						dp->blk_ofs = dp->dptr;	/* Offset initial du LFN */
					}

					ord = (c == ord && sum == dp->dir[LDIR_Chksum] && cmp_lfn(fs->lfnbuf, dp->dir)) ? ord - 1 : 0xFF;
				}
			} else {					/* Entrée SFN détectée */
				if (!ord && sum == sum_sfn(dp->dir)) break;
				if (!(dp->fn[NSFLAG] & NS_LOSS) && !mem_cmp(dp->dir, dp->fn, 11)) break;
				ord = 0xFF; dp->blk_ofs = 0xFFFFFFFF;
			}
		}
#else		/* Configuration sans LFN */
		dp->obj.attr = dp->dir[DIR_Attr] & AM_MASK;
		if (!(dp->dir[DIR_Attr] & AM_VOL) && !mem_cmp(dp->dir, dp->fn, 11)) break;
#endif
		res = dir_next(dp, 0);
	} while (res == FR_OK);

	return res;
}

#if !_FS_READONLY

static
FRESULT dir_register (	/* FR_OK succès, FR_DENIED entrées/collisions SFN, FR_DISK_ERR disque */
	DIR* dp
)
{
	FRESULT res;
	FATFS *fs = dp->obj.fs;
#if _USE_LFN != 0	/* Configuration LFN */
	UINT n, nlen, nent;
	BYTE sn[12], sum;

	if (dp->fn[NSFLAG] & (NS_DOT | NS_NONAME)) return FR_INVALID_NAME;
	for (nlen = 0; fs->lfnbuf[nlen]; nlen++) ;

#if _FS_EXFAT
	if (fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
		DIR dj;

		nent = (nlen + 14) / 15 + 2;	/* Nombre d’entrées à allouer : 85+C0+C1s */
		res = dir_alloc(dp, nent);
		if (res != FR_OK) return res;
		dp->blk_ofs = dp->dptr - SZDIRE * (nent - 1);

		if (dp->obj.sclust != 0 && (dp->obj.stat & 4)) {
			dp->obj.objsize += (DWORD)fs->csize * SS(fs);	/* Extension taille répertoire d’un cluster */
			res = fill_first_frag(&dp->obj);
			if (res != FR_OK) return res;
			res = fill_last_frag(&dp->obj, dp->clust, 0xFFFFFFFF);
			if (res != FR_OK) return res;
			res = load_obj_dir(&dj, &dp->obj);
			if (res != FR_OK) return res;
			st_qword(fs->dirbuf + XDIR_FileSize, dp->obj.objsize);
			st_qword(fs->dirbuf + XDIR_ValidFileSize, dp->obj.objsize);
			fs->dirbuf[XDIR_GenFlags] = dp->obj.stat | 1;
			res = store_xdir(&dj);
			if (res != FR_OK) return res;
		}

		create_xdir(fs->dirbuf, fs->lfnbuf);	/* Bloc répertoire mémoire écrit ultérieurement */
		return FR_OK;
	}
#endif
	/* Sur volume FAT12/16/32 */
	mem_cpy(sn, dp->fn, 12);
	if (sn[NSFLAG] & NS_LOSS) {			/* LFN hors format 8.3 : création nom numéroté */
		dp->fn[NSFLAG] = NS_NOLFN;
		for (n = 1; n < 100; n++) {
			gen_numname(dp->fn, sn, fs->lfnbuf, n);
			res = dir_find(dp);
			if (res != FR_OK) break;
		}
		if (n == 100) return FR_DENIED;		/* Abandon si trop de collisions */
		if (res != FR_NO_FILE) return res;	/* Abandon sauf résultat sans collision */
		dp->fn[NSFLAG] = sn[NSFLAG];
	}

	nent = (sn[NSFLAG] & NS_LFN) ? (nlen + 12) / 13 + 1 : 1;
	res = dir_alloc(dp, nent);
	if (res == FR_OK && --nent) {
		res = dir_sdi(dp, dp->dptr - nent * SZDIRE);
		if (res == FR_OK) {
			sum = sum_sfn(dp->fn);	/* Checksum SFN lié au LFN */
			do {					/* Écriture entrées LFN en ordre inverse */
				res = move_window(fs, dp->sect);
				if (res != FR_OK) break;
				put_lfn(fs->lfnbuf, dp->dir, (BYTE)nent, sum);
				fs->wflag = 1;
				res = dir_next(dp, 0);
			} while (res == FR_OK && --nent);
		}
	}

#else	/* Configuration sans LFN */
	res = dir_alloc(dp, 1);

#endif

	if (res == FR_OK) {
		res = move_window(fs, dp->sect);
		if (res == FR_OK) {
			mem_set(dp->dir, 0, SZDIRE);
			mem_cpy(dp->dir + DIR_Name, dp->fn, 11);
#if _USE_LFN != 0
			dp->dir[DIR_NTres] = dp->fn[NSFLAG] & (NS_BODY | NS_EXT);
#endif
			fs->wflag = 1;
		}
	}

	return res;
}

#endif

#if !_FS_READONLY && _FS_MINIMIZE == 0

static
FRESULT dir_remove (	/* FR_OK succès ; FR_DISK_ERR erreur disque */
	DIR* dp
)
{
	FRESULT res;
	FATFS *fs = dp->obj.fs;
#if _USE_LFN != 0	/* Configuration LFN */
	DWORD last = dp->dptr;

	res = (dp->blk_ofs == 0xFFFFFFFF) ? FR_OK : dir_sdi(dp, dp->blk_ofs);	/* Début du bloc si LFN présent */
	if (res == FR_OK) {
		do {
			res = move_window(fs, dp->sect);
			if (res != FR_OK) break;
			/* Entrée marquée supprimée */
			if (_FS_EXFAT && fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
				dp->dir[XDIR_Type] &= 0x7F;
			} else {									/* Sur volume FAT12/16/32 */
				dp->dir[DIR_Name] = DDEM;
			}
			fs->wflag = 1;
			if (dp->dptr >= last) break;	/* Dernière entrée atteinte : objet entièrement supprimé */
			res = dir_next(dp, 0);
		} while (res == FR_OK);
		if (res == FR_NO_FILE) res = FR_INT_ERR;
	}
#else			/* Configuration sans LFN */

	res = move_window(fs, dp->sect);
	if (res == FR_OK) {
		dp->dir[DIR_Name] = DDEM;
		fs->wflag = 1;
	}
#endif

	return res;
}

#endif

#if _FS_MINIMIZE <= 1 || _FS_RPATH >= 2

static
void get_fileinfo (
	DIR* dp,			/* Pointeur objet répertoire */
	FILINFO* fno
)
{
	UINT i, j;
	TCHAR c;
	DWORD tm;
#if _USE_LFN != 0
	WCHAR w, lfv;
	FATFS *fs = dp->obj.fs;
#endif

	fno->fname[0] = 0;		/* Invalidation des informations fichier */
	if (!dp->sect) return;	/* Sortie si read pointer à la fin du répertoire */

#if _USE_LFN != 0	/* Configuration LFN */
#if _FS_EXFAT
	if (fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
		get_xdir_info(fs->dirbuf, fno);
		return;
	} else
#endif
	{	/* Sur volume FAT12/16/32 */
		if (dp->blk_ofs != 0xFFFFFFFF) {
			i = j = 0;
			while ((w = fs->lfnbuf[j++]) != 0) {
#if !_LFN_UNICODE
				w = ff_convert(w, 0);		/* Unicode vers OEM */
				if (w == 0) { i = 0; break; }	/* LFN absent si conversion impossible */
				if (_DF1S && w >= 0x100) {	/* Premier byte DBC, jamais en configuration SBCS */
					fno->fname[i++] = (char)(w >> 8);
				}
#endif
				if (i >= _MAX_LFN) { i = 0; break; }	/* LFN absent si overflow buffer */
				fno->fname[i++] = (TCHAR)w;
			}
			fno->fname[i] = 0;
		}
	}

	i = j = 0;
	lfv = fno->fname[i];	/* LFN présent si non nul */
	while (i < 11) {
		c = (TCHAR)dp->dir[i++];
		if (c == ' ') continue;				/* Ignorer espaces de padding */
		if (c == RDDEM) c = (TCHAR)DDEM;	/* Restauration du caractère DDEM remplacé */
		if (i == 9) {						/* Point inséré si extension présente */
			if (!lfv) fno->fname[j] = '.';
			fno->altname[j++] = '.';
		}
#if _LFN_UNICODE
		if (IsDBCS1(c) && i != 8 && i != 11 && IsDBCS2(dp->dir[i])) {
			c = c << 8 | dp->dir[i++];
		}
		c = ff_convert(c, 1);	/* OEM vers Unicode */
		if (!c) c = '?';
#endif
		fno->altname[j] = c;
		if (!lfv) {
			if (IsUpper(c) && (dp->dir[DIR_NTres] & ((i >= 9) ? NS_EXT : NS_BODY))) {
				c += 0x20;
			}
			fno->fname[j] = c;
		}
		j++;
	}
	if (!lfv) {
		fno->fname[j] = 0;
		if (!dp->dir[DIR_NTres]) j = 0;	/* Altname inutile sans LFN ni information de casse */
	}
	fno->altname[j] = 0;

#else	/* Configuration sans LFN */
	i = j = 0;
	while (i < 11) {
		c = (TCHAR)dp->dir[i++];
		if (c == ' ') continue;				/* Ignorer espaces de padding */
		if (c == RDDEM) c = (TCHAR)DDEM;	/* Restauration du caractère DDEM remplacé */
		if (i == 9) fno->fname[j++] = '.';	/* Point inséré si extension présente */
		fno->fname[j++] = c;
	}
	fno->fname[j] = 0;
#endif

	fno->fattrib = dp->dir[DIR_Attr];
	fno->fsize = ld_dword(dp->dir + DIR_FileSize);
	tm = ld_dword(dp->dir + DIR_ModTime);
	fno->ftime = (WORD)tm; fno->fdate = (WORD)(tm >> 16);
}

#endif

#if _USE_FIND && _FS_MINIMIZE <= 1

static
WCHAR get_achar (		/* Avancement pointer de 1 ou 2 caractères */
	const TCHAR** ptr	/* Pointer de chaîne SBCS/DBCS/Unicode */
)
{
#if !_LFN_UNICODE
	WCHAR chr;

	chr = (BYTE)*(*ptr)++;
	if (IsLower(chr)) chr -= 0x20;
#ifdef _EXCVT
	if (chr >= 0x80) chr = ExCvt[chr - 0x80];
#else
	if (IsDBCS1(chr) && IsDBCS2(**ptr)) {		/* Deuxième byte DBC si nécessaire */
		chr = chr << 8 | (BYTE)*(*ptr)++;
	}
#endif
	return chr;
#else
	return ff_wtoupper(*(*ptr)++);
#endif
}

static
int pattern_matching (	/* 0 incompatibilité, 1 correspondance */
	const TCHAR* pat,
	const TCHAR* nam,
	int skip,			/* Nombre de caractères à ignorer, selon les ? */
	int inf				/* Wildcard * : recherche sans limite */
)
{
	const TCHAR *pp, *np;
	WCHAR pc, nc;
	int nm, nx;

	while (skip--) {
		if (!get_achar(&nam)) return 0;	/* Branche incompatible si trop peu de caractères */
	}
	if (!*pat && inf) return 1;

	do {
		pp = pat; np = nam;
		for (;;) {
			if (*pp == '?' || *pp == '*') {
				nm = nx = 0;
				do {
					if (*pp++ == '?') nm++; else nx = 1;
				} while (*pp == '?' || *pp == '*');
				if (pattern_matching(pp, np, nm, nx)) return 1;	/* Récursion limitée au nombre de blocs wildcard */
				nc = *np; break;
			}
			pc = get_achar(&pp);
			nc = get_achar(&np);
			if (pc != nc) break;
			if (pc == 0) return 1;	/* Correspondance à la fin des deux chaînes */
		}
		get_achar(&nam);
	} while (inf && nc);			/* Wildcard * : retry jusqu’à fin du nom */

	return 0;
}

#endif

static
FRESULT create_name (	/* FR_OK succès ; FR_INVALID_NAME création impossible */
	DIR* dp,			/* Pointeur objet répertoire */
	const TCHAR** path	/* Pointer du segment dans la chaîne chemin */
)
{
#if _USE_LFN != 0	/* Configuration LFN */
	BYTE b, cf;
	WCHAR w, *lfn;
	UINT i, ni, si, di;
	const TCHAR *p;

	/* LFN créé en Unicode */
	p = *path; lfn = dp->obj.fs->lfnbuf; si = di = 0;
	for (;;) {
		w = p[si++];
		if (w < ' ') break;
		if (w == '/' || w == '\\') {
			while (p[si] == '/' || p[si] == '\\') si++;	/* Séparateurs dupliqués ignorés */
			break;
		}
		if (di >= _MAX_LFN) return FR_INVALID_NAME;	/* Nom trop long refusé */
#if !_LFN_UNICODE
		w &= 0xFF;
		if (IsDBCS1(w)) {				/* Premier byte DBC ; toujours false en SBCS */
			b = (BYTE)p[si++];
			w = (w << 8) + b;
			if (!IsDBCS2(b)) return FR_INVALID_NAME;	/* Séquence invalide refusée */
		}
		w = ff_convert(w, 1);			/* Conversion ANSI/OEM vers Unicode */
		if (!w) return FR_INVALID_NAME;	/* Code invalide refusé */
#endif
		if (w < 0x80 && chk_chr("\"*:<>\?|\x7F", w)) return FR_INVALID_NAME;	/* Caractères LFN interdits refusés */
		lfn[di++] = w;
	}
	*path = &p[si];						/* Retour pointer vers segment suivant */
	cf = (w < ' ') ? NS_LAST : 0;		/* Dernier segment marqué si fin du chemin */
#if _FS_RPATH != 0
	if ((di == 1 && lfn[di - 1] == '.') ||
		(di == 2 && lfn[di - 1] == '.' && lfn[di - 2] == '.')) {
		lfn[di] = 0;
		for (i = 0; i < 11; i++)
			dp->fn[i] = (i < di) ? '.' : ' ';
		dp->fn[i] = cf | NS_DOT;		/* Entrée point */
		return FR_OK;
	}
#endif
	while (di) {						/* Suppression espaces et points terminaux */
		w = lfn[di - 1];
		if (w != ' ' && w != '.') break;
		di--;
	}
	lfn[di] = 0;
	if (di == 0) return FR_INVALID_NAME;	/* Nom vide refusé */

	mem_set(dp->fn, ' ', 11);
	for (si = 0; lfn[si] == ' ' || lfn[si] == '.'; si++) ;	/* Suppression espaces et points initiaux */
	if (si) cf |= NS_LOSS | NS_LFN;
	while (di && lfn[di - 1] != '.') di--;	/* Aucune extension si di <=si */

	i = b = 0; ni = 8;
	for (;;) {
		w = lfn[si++];
		if (!w) break;
		if (w == ' ' || (w == '.' && si != di)) {
			cf |= NS_LOSS | NS_LFN; continue;
		}

		if (i >= ni || si == di) {
			if (ni == 11) {
				cf |= NS_LOSS | NS_LFN; break;
			}
			if (si != di) cf |= NS_LOSS | NS_LFN;	/* Hors format 8.3 */
			if (si > di) break;
			si = di; i = 8; ni = 11;
			b <<= 2; continue;
		}

		if (w >= 0x80) {
#ifdef _EXCVT
			w = ff_convert(w, 0);		/* Unicode vers OEM */
			if (w) w = ExCvt[w - 0x80];	/* Conversion étendue vers majuscules SBCS */
#else
			w = ff_convert(ff_wtoupper(w), 0);	/* Unicode majuscule vers OEM */
#endif
			cf |= NS_LFN;				/* Création entrée LFN forcée */
		}

		if (_DF1S && w >= 0x100) {		/* DBCS ; toujours false en SBCS */
			if (i >= ni - 1) {
				cf |= NS_LOSS | NS_LFN; i = ni; continue;
			}
			dp->fn[i++] = (BYTE)(w >> 8);
		} else {						/* SBC */
			if (!w || chk_chr("+,;=[]", w)) {	/* Remplacement des caractères interdits SFN */
				w = '_'; cf |= NS_LOSS | NS_LFN;/* Conversion avec perte */
			} else {
				if (IsUpper(w)) {		/* ASCII majuscule */
					b |= 2;
				} else {
					if (IsLower(w)) {	/* ASCII minuscule */
						b |= 1; w -= 0x20;
					}
				}
			}
		}
		dp->fn[i++] = (BYTE)w;
	}

	if (dp->fn[0] == DDEM) dp->fn[0] = RDDEM;	/* Premier caractère DDEM remplacé par RDDEM */

	if (ni == 8) b <<= 2;
	if ((b & 0x0C) == 0x0C || (b & 0x03) == 0x03) cf |= NS_LFN;	/* Casse mixte : création entrée LFN */
	if (!(cf & NS_LFN)) {						/* Format 8.3 sans caractère étendu : création flags NT */
		if ((b & 0x03) == 0x01) cf |= NS_EXT;	/* Flag NT : extension entièrement minuscule */
		if ((b & 0x0C) == 0x04) cf |= NS_BODY;	/* Flag NT : nom entièrement minuscule */
	}

	dp->fn[NSFLAG] = cf;

	return FR_OK;

#else	/* Configuration sans LFN */
	BYTE c, d, *sfn;
	UINT ni, si, i;
	const char *p;

	p = *path; sfn = dp->fn;
	mem_set(sfn, ' ', 11);
	si = i = 0; ni = 8;
#if _FS_RPATH != 0
	if (p[si] == '.') {
		for (;;) {
			c = (BYTE)p[si++];
			if (c != '.' || si >= 3) break;
			sfn[i++] = c;
		}
		if (c != '/' && c != '\\' && c > ' ') return FR_INVALID_NAME;
		*path = p + si;								/* Retour pointer vers segment suivant */
		sfn[NSFLAG] = (c <= ' ') ? NS_LAST | NS_DOT : NS_DOT;	/* Dernier segment marqué si fin du chemin */
		return FR_OK;
	}
#endif
	for (;;) {
		c = (BYTE)p[si++];
		if (c <= ' ') break;
		if (c == '/' || c == '\\') {
			while (p[si] == '/' || p[si] == '\\') si++;	/* Séparateurs dupliqués ignorés */
			break;
		}
		if (c == '.' || i >= ni) {
			if (ni == 11 || c != '.') return FR_INVALID_NAME;	/* Dépassement de taille ou point invalide */
			i = 8; ni = 11;
			continue;
		}
		if (c >= 0x80) {
#ifdef _EXCVT
			c = ExCvt[c - 0x80];		/* Caractères étendus en majuscules SBCS */
#else
#if !_DF1S
			return FR_INVALID_NAME;		/* Configuration ASCII : caractères étendus refusés */
#endif
#endif
		}
		if (IsDBCS1(c)) {				/* Premier byte DBC ; toujours false en SBCS */
			d = (BYTE)p[si++];
			if (!IsDBCS2(d) || i >= ni - 1) return FR_INVALID_NAME;	/* DBC invalide refusé */
			sfn[i++] = c;
			sfn[i++] = d;
		} else {						/* SBC */
			if (chk_chr("\"*+,:;<=>\?[]|\x7F", c)) return FR_INVALID_NAME;	/* Caractères SFN interdits refusés */
			if (IsLower(c)) c -= 0x20;
			sfn[i++] = c;
		}
	}
	*path = p + si;						/* Retour pointer vers segment suivant */
	if (i == 0) return FR_INVALID_NAME;	/* Chaîne vide refusée */

	if (sfn[0] == DDEM) sfn[0] = RDDEM;	/* Premier caractère DDEM remplacé par RDDEM */
	sfn[NSFLAG] = (c <= ' ') ? NS_LAST : 0;		/* Dernier segment marqué si fin du chemin */

	return FR_OK;
#endif
}

static
FRESULT follow_path (	/* FR_OK(0) succès ; sinon code erreur */
	DIR* dp,
	const TCHAR* path	/* Chemin complet du fichier/répertoire */
)
{
	FRESULT res;
	BYTE ns;
	_FDID *obj = &dp->obj;
	FATFS *fs = obj->fs;

#if _FS_RPATH != 0
	if (*path != '/' && *path != '\\') {	/* Sans séparateur initial */
		obj->sclust = fs->cdir;				/* Départ depuis le répertoire courant */
	} else
#endif
	{										/* Avec séparateur initial */
		while (*path == '/' || *path == '\\') path++;
		obj->sclust = 0;					/* Départ depuis la racine */
	}
#if _FS_EXFAT
	obj->n_frag = 0;	/* Invalidation compteur dernier fragment */
#if _FS_RPATH != 0
	if (fs->fs_type == FS_EXFAT && obj->sclust) {
		DIR dj;

		obj->c_scl = fs->cdc_scl;
		obj->c_size = fs->cdc_size;
		obj->c_ofs = fs->cdc_ofs;
		res = load_obj_dir(&dj, obj);
		if (res != FR_OK) return res;
		obj->objsize = ld_dword(fs->dirbuf + XDIR_FileSize);
		obj->stat = fs->dirbuf[XDIR_GenFlags] & 2;
	}
#endif
#endif

	if ((UINT)*path < ' ') {				/* Chemin vide : répertoire d’origine */
		dp->fn[NSFLAG] = NS_NONAME;
		res = dir_sdi(dp, 0);

	} else {
		for (;;) {
			res = create_name(dp, &path);
			if (res != FR_OK) break;
			res = dir_find(dp);
			ns = dp->fn[NSFLAG];
			if (res != FR_OK) {
				if (res == FR_NO_FILE) {
					if (_FS_RPATH && (ns & NS_DOT)) {	/* Entrée point absente : conserver répertoire courant */
						if (!(ns & NS_LAST)) continue;
						dp->fn[NSFLAG] = NS_NONAME;
						res = FR_OK;
					} else {
						if (!(ns & NS_LAST)) res = FR_NO_PATH;	/* Code erreur adapté si segment non terminal */
					}
				}
				break;
			}
			if (ns & NS_LAST) break;

			if (!(obj->attr & AM_DIR)) {		/* Objet non-répertoire : poursuite impossible */
				res = FR_NO_PATH; break;
			}
#if _FS_EXFAT
			if (fs->fs_type == FS_EXFAT) {		/* Informations du répertoire parent conservées pour la suite */
				obj->c_scl = obj->sclust;
				obj->c_size = ((DWORD)obj->objsize & 0xFFFFFF00) | obj->stat;
				obj->c_ofs = dp->blk_ofs;
				obj->sclust = ld_dword(fs->dirbuf + XDIR_FstClus);
				obj->stat = fs->dirbuf[XDIR_GenFlags] & 2;
				obj->objsize = ld_qword(fs->dirbuf + XDIR_FileSize);
			} else
#endif
			{
				obj->sclust = ld_clust(fs, fs->win + dp->dptr % SS(fs));
			}
		}
	}

	return res;
}

static
int get_ldnumber (		/* Numéro lecteur logique ; -1 invalide */
	const TCHAR** path
)
{
	const TCHAR *tp, *tt;
	UINT i;
	int vol = -1;
#if _STR_VOLUME_ID
	static const char* const volid[] = {_VOLUME_STRS};
	const char *sp;
	char c;
	TCHAR tc;
#endif

	if (*path) {
		for (tt = *path; (UINT)*tt >= (_USE_LFN ? ' ' : '!') && *tt != ':'; tt++) ;
		if (*tt == ':') {
			tp = *path;
			i = *tp++ - '0';
			if (i < 10 && tp == tt) {
				if (i < _VOLUMES) {	/* Drive ID reconnu : retrait du préfixe */
					vol = (int)i;
					*path = ++tt;
				}
			}
#if _STR_VOLUME_ID
			 else {	/* Drive ID non numérique : recherche ID string */
				i = 0; tt++;
				do {
					sp = volid[i]; tp = *path;
					do {
						c = *sp++; tc = *tp++;
						if (IsLower(tc)) tc -= 0x20;
					} while (c && (TCHAR)c == tc);
				} while ((c || tp != tt) && ++i < _VOLUMES);
				if (i < _VOLUMES) {	/* Drive ID reconnu : retrait du préfixe */
					vol = (int)i;
					*path = tt;
				}
			}
#endif
			return vol;
		}
#if _FS_RPATH != 0 && _VOLUMES >= 2
		vol = CurrVol;
#else
		vol = 0;
#endif
	}
	return vol;
}

static
BYTE check_fs (	/* 0 FAT, 1 exFAT, 2 boot non-FAT, 3 non-boot, 4 erreur disque */
	FATFS* fs,	/* Objet système de fichiers */
	DWORD sect	/* Secteur LBA à vérifier comme FAT-VBR */
)
{
	fs->wflag = 0; fs->winsect = 0xFFFFFFFF;
	if (move_window(fs, sect) != FR_OK) return 4;

	if (ld_word(fs->win + BS_55AA) != 0xAA55) return 3;	/* Signature boot au même offset, même si secteur >512 bytes */

	if (fs->win[BS_JmpBoot] == 0xE9 || (fs->win[BS_JmpBoot] == 0xEB && fs->win[BS_JmpBoot + 2] == 0x90)) {
		if ((ld_dword(fs->win + BS_FilSysType) & 0xFFFFFF) == 0x544146) return 0;
		if (ld_dword(fs->win + BS_FilSysType32) == 0x33544146) return 0;
	}
#if _FS_EXFAT
	if (!mem_cmp(fs->win + BS_JmpBoot, "\xEB\x76\x90" "EXFAT   ", 11)) return 1;
#endif
	return 2;
}

static
FRESULT find_volume (	/* FR_OK(0) succès ; sinon erreur */
	const TCHAR** path,
	FATFS** rfs,
	BYTE mode			/* Valeur non nulle : vérifier protection pour écriture */
)
{
	BYTE fmt, *pt;
	int vol;
	DSTATUS stat;
	DWORD bsect, fasize, tsect, sysect, nclst, szbfat, br[4];
	WORD nrsv;
	FATFS *fs;
	UINT i;

	*rfs = 0;
	vol = get_ldnumber(path);
	if (vol < 0) return FR_INVALID_DRIVE;

	fs = FatFs[vol];
	if (!fs) return FR_NOT_ENABLED;

	ENTER_FF(fs);
	*rfs = fs;

	mode &= (BYTE)~FA_READ;				/* Mode d’accès souhaité : écriture ou non */
	if (fs->fs_type) {
		stat = disk_status(fs->drv);
		if (!(stat & STA_NOINIT)) {
			if (!_FS_READONLY && mode && (stat & STA_PROTECT)) {	/* Protection en écriture vérifiée si nécessaire */
				return FR_WRITE_PROTECTED;
			}
			return FR_OK;
		}
	}

	fs->fs_type = 0;
	fs->drv = LD2PD(vol);
	stat = disk_initialize(fs->drv);
	if (stat & STA_NOINIT) {
		return FR_NOT_READY;			/* Initialisation échouée : aucun support ou erreur hardware */
	}
	if (!_FS_READONLY && mode && (stat & STA_PROTECT)) {
		return FR_WRITE_PROTECTED;
	}
#if _MAX_SS != _MIN_SS					/* Taille secteur seulement si configuration variable */
	if (disk_ioctl(fs->drv, GET_SECTOR_SIZE, &SS(fs)) != RES_OK) return FR_DISK_ERR;
	if (SS(fs) > _MAX_SS || SS(fs) < _MIN_SS || (SS(fs) & (SS(fs) - 1))) return FR_DISK_ERR;
#endif

	/* Partitions génériques uniquement : FDISK ou SFD */
	bsect = 0;
	fmt = check_fs(fs, bsect);			/* Secteur 0 vérifié comme FAT-VBR SFD */
	if (fmt == 2 || (fmt < 2 && LD2PT(vol) != 0)) {	/* Non FAT-VBR ou numéro partition forcé */
		for (i = 0; i < 4; i++) {
			pt = fs->win + (MBR_Table + i * SZ_PTE);
			br[i] = pt[PTE_System] ? ld_dword(pt + PTE_StLba) : 0;
		}
		i = LD2PT(vol);					/* Partition : 0 auto, 1..4 forcée */
		if (i) i--;
		do {
			bsect = br[i];
			fmt = bsect ? check_fs(fs, bsect) : 3;
		} while (LD2PT(vol) == 0 && fmt >= 2 && ++i < 4);
	}
	if (fmt == 4) return FR_DISK_ERR;		/* Erreur disk I/O */
	if (fmt >= 2) return FR_NO_FILESYSTEM;	/* Aucun volume FAT */

#if _FS_EXFAT
	if (fmt == 1) {
		QWORD maxlba;

		for (i = BPB_ZeroedEx; i < BPB_ZeroedEx + 53 && fs->win[i] == 0; i++) ;
		if (i < BPB_ZeroedEx + 53) return FR_NO_FILESYSTEM;

		if (ld_word(fs->win + BPB_FSVerEx) != 0x100) return FR_NO_FILESYSTEM;	/* Version exFAT obligatoirement 1.0 */

		if (1 << fs->win[BPB_BytsPerSecEx] != SS(fs)) {	/* BPB_BytsPerSecEx égal à la taille secteur physique */
			return FR_NO_FILESYSTEM;
		}

		maxlba = ld_qword(fs->win + BPB_TotSecEx) + bsect;	/* Dernier LBA du volume +1 */
		if (maxlba >= 0x100000000) return FR_NO_FILESYSTEM;	/* LBA incompatible avec 32 bits */

		fs->fsize = ld_dword(fs->win + BPB_FatSzEx);	/* Nombre de secteurs par FAT */

		fs->n_fats = fs->win[BPB_NumFATsEx];			/* Nombre de FAT */
		if (fs->n_fats != 1) return FR_NO_FILESYSTEM;	/* Une seule FAT supportée */

		fs->csize = 1 << fs->win[BPB_SecPerClusEx];		/* Taille cluster */
		if (fs->csize == 0)	return FR_NO_FILESYSTEM;	/* Valeur obligatoire dans [1,32768] */

		nclst = ld_dword(fs->win + BPB_NumClusEx);		/* Nombre de clusters */
		if (nclst > MAX_EXFAT) return FR_NO_FILESYSTEM;	/* Trop de clusters */
		fs->n_fatent = nclst + 2;

		fs->volbase = bsect;
		fs->database = bsect + ld_dword(fs->win + BPB_DataOfsEx);
		fs->fatbase = bsect + ld_dword(fs->win + BPB_FatOfsEx);
		if (maxlba < (QWORD)fs->database + nclst * fs->csize) return FR_NO_FILESYSTEM;	/* Volume au moins aussi grand que la taille requise */
		fs->dirbase = ld_dword(fs->win + BPB_RootClusEx);

		/* Bitmap supposé dans le premier cluster */
		if (move_window(fs, clust2sect(fs, fs->dirbase)) != FR_OK) return FR_DISK_ERR;
		for (i = 0; i < SS(fs); i += SZDIRE) {
			if (fs->win[i] == 0x81 && ld_dword(fs->win + i + 20) == 2) break;	/* Entrée 81 associée au cluster 2 */
		}
		if (i == SS(fs)) return FR_NO_FILESYSTEM;
#if !_FS_READONLY
		fs->last_clst = fs->free_clst = 0xFFFFFFFF;
#endif
		fmt = FS_EXFAT;			/* Sous-type FAT */
	} else
#endif
	{
		if (ld_word(fs->win + BPB_BytsPerSec) != SS(fs)) return FR_NO_FILESYSTEM;	/* BPB_BytsPerSec égal à la taille secteur physique */

		fasize = ld_word(fs->win + BPB_FATSz16);		/* Nombre de secteurs par FAT */
		if (fasize == 0) fasize = ld_dword(fs->win + BPB_FATSz32);
		fs->fsize = fasize;

		fs->n_fats = fs->win[BPB_NumFATs];				/* Nombre de FAT */
		if (fs->n_fats != 1 && fs->n_fats != 2) return FR_NO_FILESYSTEM;	/* Valeur obligatoirement 1 ou 2 */
		fasize *= fs->n_fats;							/* Nombre de secteurs zone FAT */

		fs->csize = fs->win[BPB_SecPerClus];			/* Taille cluster */
		if (fs->csize == 0 || (fs->csize & (fs->csize - 1))) return FR_NO_FILESYSTEM;	/* Puissance de 2 obligatoire */

		fs->n_rootdir = ld_word(fs->win + BPB_RootEntCnt);	/* Nombre entrées répertoire racine */
		if (fs->n_rootdir % (SS(fs) / SZDIRE)) return FR_NO_FILESYSTEM;	/* Alignement secteur obligatoire */

		tsect = ld_word(fs->win + BPB_TotSec16);		/* Nombre secteurs volume */
		if (tsect == 0) tsect = ld_dword(fs->win + BPB_TotSec32);

		nrsv = ld_word(fs->win + BPB_RsvdSecCnt);		/* Nombre secteurs réservés */
		if (nrsv == 0) return FR_NO_FILESYSTEM;			/* Valeur zéro interdite */

		sysect = nrsv + fasize + fs->n_rootdir / (SS(fs) / SZDIRE);	/* RSV + FAT + DIR */
		if (tsect < sysect) return FR_NO_FILESYSTEM;	/* Taille volume invalide */
		nclst = (tsect - sysect) / fs->csize;			/* Nombre de clusters */
		if (nclst == 0) return FR_NO_FILESYSTEM;		/* Taille volume invalide */
		fmt = FS_FAT32;
		if (nclst <= MAX_FAT16) fmt = FS_FAT16;
		if (nclst <= MAX_FAT12) fmt = FS_FAT12;

		fs->n_fatent = nclst + 2;						/* Nombre entrées FAT */
		fs->volbase = bsect;							/* Secteur début volume */
		fs->fatbase = bsect + nrsv; 					/* Secteur début FAT */
		fs->database = bsect + sysect;					/* Secteur début données */
		if (fmt == FS_FAT32) {
			if (ld_word(fs->win + BPB_FSVer32) != 0) return FR_NO_FILESYSTEM;	/* FAT32 version 0.0 obligatoire */
			if (fs->n_rootdir) return FR_NO_FILESYSTEM;	/* BPB_RootEntCnt obligatoirement zéro */
			fs->dirbase = ld_dword(fs->win + BPB_RootClus32);
			szbfat = fs->n_fatent * 4;					/* Taille FAT requise */
		} else {
			if (fs->n_rootdir == 0)	return FR_NO_FILESYSTEM;/* BPB_RootEntCnt non nul requis */
			fs->dirbase = fs->fatbase + fasize;
			szbfat = (fmt == FS_FAT16) ?				/* Taille FAT requise */
				fs->n_fatent * 2 : fs->n_fatent * 3 / 2 + (fs->n_fatent & 1);
		}
		if (fs->fsize < (szbfat + (SS(fs) - 1)) / SS(fs)) return FR_NO_FILESYSTEM;	/* BPB_FATSz au moins égal à la taille requise */

#if !_FS_READONLY

		fs->last_clst = fs->free_clst = 0xFFFFFFFF;
		fs->fsi_flag = 0x80;
#if (_FS_NOFSINFO & 3) != 3
		if (fmt == FS_FAT32				/* FSINFO activé uniquement en FAT32 avec BPB_FSInfo32=1 */
			&& ld_word(fs->win + BPB_FSInfo32) == 1
			&& move_window(fs, bsect + 1) == FR_OK)
		{
			fs->fsi_flag = 0;
			if (ld_word(fs->win + BS_55AA) == 0xAA55
				&& ld_dword(fs->win + FSI_LeadSig) == 0x41615252
				&& ld_dword(fs->win + FSI_StrucSig) == 0x61417272)
			{
#if (_FS_NOFSINFO & 1) == 0
				fs->free_clst = ld_dword(fs->win + FSI_Free_Count);
#endif
#if (_FS_NOFSINFO & 2) == 0
				fs->last_clst = ld_dword(fs->win + FSI_Nxt_Free);
#endif
			}
		}
#endif
#endif
	}

	fs->fs_type = fmt;		/* Sous-type FAT */
	fs->id = ++Fsid;
#if _USE_LFN == 1
	fs->lfnbuf = LfnBuf;	/* Buffer LFN statique */
#if _FS_EXFAT
	fs->dirbuf = DirBuf;	/* Buffer temporaire bloc répertoire statique */
#endif
#endif
#if _FS_RPATH != 0
	fs->cdir = 0;
#endif
#if _FS_LOCK != 0
	clear_lock(fs);
#endif
	return FR_OK;
}

static
FRESULT validate (	/* Retour FR_OK ou FR_INVALID_OBJECT */
	_FDID* obj,		/* _OBJ obligatoirement premier membre de FIL/DIR */
	FATFS** fs
)
{
	FRESULT res = FR_INVALID_OBJECT;

	if (obj && obj->fs && obj->fs->fs_type && obj->id == obj->fs->id) {
#if _FS_REENTRANT
		if (lock_fs(obj->fs)) {
			if (!(disk_status(obj->fs->drv) & STA_NOINIT)) {
				res = FR_OK;
			} else {
				unlock_fs(obj->fs, FR_OK);
			}
		} else {
			res = FR_TIMEOUT;
		}
#else
		if (!(disk_status(obj->fs->drv) & STA_NOINIT)) {
			res = FR_OK;
		}
#endif
	}
	*fs = (res == FR_OK) ? obj->fs : 0;
	return res;
}

FRESULT f_mount (
	FATFS* fs,			/* Filesystem NULL : démontage */
	const TCHAR* path,
	BYTE opt			/* Mount : 0 différé, 1 immédiat */
)
{
	FATFS *cfs;
	int vol;
	FRESULT res;
	const TCHAR *rp = path;

	vol = get_ldnumber(&rp);
	if (vol < 0) return FR_INVALID_DRIVE;
	cfs = FatFs[vol];

	if (cfs) {
#if _FS_LOCK != 0
		clear_lock(cfs);
#endif
#if _FS_REENTRANT
		if (!ff_del_syncobj(cfs->sobj)) return FR_INT_ERR;
#endif
		cfs->fs_type = 0;
	}

	if (fs) {
		fs->fs_type = 0;
#if _FS_REENTRANT
		if (!ff_cre_syncobj((BYTE)vol, &fs->sobj)) return FR_INT_ERR;
#endif
	}
	FatFs[vol] = fs;

	if (!fs || opt != 1) return FR_OK;	/* Mount différé */

	res = find_volume(&path, &fs, 0);
	LEAVE_FF(fs, res);
}

FRESULT f_open (
	FIL* fp,
	const TCHAR* path,
	BYTE mode
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
#if !_FS_READONLY
	DWORD dw, cl, bcs, clst, sc;
	FSIZE_t ofs;
#endif
	DEF_NAMBUF

	if (!fp) return FR_INVALID_OBJECT;

	/* Recherche lecteur logique */
	mode &= _FS_READONLY ? FA_READ : FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_OPEN_ALWAYS | FA_OPEN_APPEND | FA_SEEKEND;
	res = find_volume(&path, &fs, mode);
	if (res == FR_OK) {
		dj.obj.fs = fs;
		INIT_NAMBUF(fs);
		res = follow_path(&dj, path);	/* Résolution chemin fichier */
#if !_FS_READONLY
		if (res == FR_OK) {
			if (dj.fn[NSFLAG] & NS_NONAME) {
				res = FR_INVALID_NAME;
			}
#if _FS_LOCK != 0
			else {
				res = chk_lock(&dj, (mode & ~FA_READ) ? 1 : 0);
			}
#endif
		}

		if (mode & (FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW)) {
			if (res != FR_OK) {
				if (res == FR_NO_FILE) {
#if _FS_LOCK != 0
					res = enq_lock() ? dir_register(&dj) : FR_TOO_MANY_OPEN_FILES;
#else
					res = dir_register(&dj);
#endif
				}
				mode |= FA_CREATE_ALWAYS;
			}
			else {
				if (dj.obj.attr & (AM_RDO | AM_DIR)) {	/* Overwrite interdit sur R/O ou répertoire */
					res = FR_DENIED;
				} else {
					if (mode & FA_CREATE_NEW) res = FR_EXIST;	/* Création nouvelle interdite */
				}
			}
			if (res == FR_OK && (mode & FA_CREATE_ALWAYS)) {	/* Truncate si mode overwrite */
				dw = GET_FATTIME();
#if _FS_EXFAT
				if (fs->fs_type == FS_EXFAT) {

					fp->obj.fs = fs;
					fp->obj.sclust = ld_dword(fs->dirbuf + XDIR_FstClus);
					fp->obj.objsize = ld_qword(fs->dirbuf + XDIR_FileSize);
					fp->obj.stat = fs->dirbuf[XDIR_GenFlags] & 2;
					fp->obj.n_frag = 0;

					st_dword(fs->dirbuf + XDIR_CrtTime, dw);
					fs->dirbuf[XDIR_CrtTime10] = 0;
					st_dword(fs->dirbuf + XDIR_ModTime, dw);
					fs->dirbuf[XDIR_ModTime10] = 0;
					fs->dirbuf[XDIR_Attr] = AM_ARC;
					st_dword(fs->dirbuf + XDIR_FstClus, 0);
					st_qword(fs->dirbuf + XDIR_FileSize, 0);
					st_qword(fs->dirbuf + XDIR_ValidFileSize, 0);
					fs->dirbuf[XDIR_GenFlags] = 1;
					res = store_xdir(&dj);
					if (res == FR_OK && fp->obj.sclust) {
						res = remove_chain(&fp->obj, fp->obj.sclust, 0);
						fs->last_clst = fp->obj.sclust - 1;		/* Réutilisation du cluster libéré */
					}
				} else
#endif
				{

					st_dword(dj.dir + DIR_CrtTime, dw);
					st_dword(dj.dir + DIR_ModTime, dw);
					dj.dir[DIR_Attr] = AM_ARC;
					cl = ld_clust(fs, dj.dir);
					st_clust(fs, dj.dir, 0);
					st_dword(dj.dir + DIR_FileSize, 0);
					fs->wflag = 1;

					if (cl) {
						dw = fs->winsect;
						res = remove_chain(&dj.obj, cl, 0);
						if (res == FR_OK) {
							res = move_window(fs, dw);
							fs->last_clst = cl - 1;		/* Réutilisation du cluster libéré */
						}
					}
				}
			}
		}
		else {
			if (res == FR_OK) {
				if (dj.obj.attr & AM_DIR) {		/* Objet répertoire */
					res = FR_NO_FILE;
				} else {
					if ((mode & FA_WRITE) && (dj.obj.attr & AM_RDO)) { /* Violation lecture seule */
						res = FR_DENIED;
					}
				}
			}
		}
		if (res == FR_OK) {
			if (mode & FA_CREATE_ALWAYS)		/* Flag change si créé ou overwritten */
				mode |= FA_MODIFIED;
			fp->dir_sect = fs->winsect;
			fp->dir_ptr = dj.dir;
#if _FS_LOCK != 0
			fp->obj.lockid = inc_lock(&dj, (mode & ~FA_READ) ? 1 : 0);
			if (!fp->obj.lockid) res = FR_INT_ERR;
#endif
		}
#else
		if (res == FR_OK) {
			if (dj.fn[NSFLAG] & NS_NONAME) {
				res = FR_INVALID_NAME;
			} else {
				if (dj.obj.attr & AM_DIR) {		/* Objet répertoire */
					res = FR_NO_FILE;
				}
			}
		}
#endif

		if (res == FR_OK) {
#if _FS_EXFAT
			if (fs->fs_type == FS_EXFAT) {
				fp->obj.c_scl = dj.obj.sclust;
				fp->obj.c_size = ((DWORD)dj.obj.objsize & 0xFFFFFF00) | dj.obj.stat;
				fp->obj.c_ofs = dj.blk_ofs;
				fp->obj.sclust = ld_dword(fs->dirbuf + XDIR_FstClus);
				fp->obj.objsize = ld_qword(fs->dirbuf + XDIR_FileSize);
				fp->obj.stat = fs->dirbuf[XDIR_GenFlags] & 2;
			} else
#endif
			{
				fp->obj.sclust = ld_clust(fs, dj.dir);
				fp->obj.objsize = ld_dword(dj.dir + DIR_FileSize);
			}
#if _USE_FASTSEEK
			fp->cltbl = 0;			/* Fast seek désactivé */
#endif
			fp->obj.fs = fs;
			fp->obj.id = fs->id;
			fp->flag = mode;
			fp->err = 0;
			fp->sect = 0;
			fp->fptr = 0;
#if !_FS_READONLY
#if !_FS_TINY
			mem_set(fp->buf, 0, _MAX_SS);
#endif
			if ((mode & FA_SEEKEND) && fp->obj.objsize > 0) {	/* Seek final si FA_OPEN_APPEND */
				fp->fptr = fp->obj.objsize;
				bcs = (DWORD)fs->csize * SS(fs);	/* Taille cluster en bytes */
				clst = fp->obj.sclust;
				for (ofs = fp->obj.objsize; res == FR_OK && ofs > bcs; ofs -= bcs) {
					clst = get_fat(&fp->obj, clst);
					if (clst <= 1) res = FR_INT_ERR;
					if (clst == 0xFFFFFFFF) res = FR_DISK_ERR;
				}
				fp->clust = clst;
				if (res == FR_OK && ofs % SS(fs)) {	/* Buffer rempli si position hors frontière secteur */
					if ((sc = clust2sect(fs, clst)) == 0) {
						res = FR_INT_ERR;
					} else {
						fp->sect = sc + (DWORD)(ofs / SS(fs));
#if !_FS_TINY
						if (disk_read(fs->drv, fp->buf, fp->sect, 1) != RES_OK) res = FR_DISK_ERR;
#endif
					}
				}
			}
#endif
		}

		FREE_NAMBUF();
	}

	if (res != FR_OK) fp->obj.fs = 0;	/* Objet fichier invalidé après erreur */

	LEAVE_FF(fs, res);
}

FRESULT f_read (
	FIL* fp, 	/* Pointeur objet fichier */
	void* buff,
	UINT btr,	/* Nombre de bytes à lire */
	UINT* br	/* Nombre de bytes effectivement lus en sortie */
)
{
	FRESULT res;
	FATFS *fs;
	DWORD clst, sect;
	FSIZE_t remain;
	UINT rcnt, cc, csect;
	BYTE *rbuff = (BYTE*)buff;

	*br = 0;
	res = validate(&fp->obj, &fs);				/* Validation objet fichier */
	if (res != FR_OK || (res = (FRESULT)fp->err) != FR_OK) LEAVE_FF(fs, res);
	if (!(fp->flag & FA_READ)) LEAVE_FF(fs, FR_DENIED);
	remain = fp->obj.objsize - fp->fptr;
	if (btr > remain) btr = (UINT)remain;		/* Btr limité aux bytes restants */

	for ( ;  btr;
		rbuff += rcnt, fp->fptr += rcnt, *br += rcnt, btr -= rcnt) {
		if (fp->fptr % SS(fs) == 0) {
			csect = (UINT)(fp->fptr / SS(fs) & (fs->csize - 1));	/* Offset secteur dans cluster */
			if (csect == 0) {
				if (fp->fptr == 0) {
					clst = fp->obj.sclust;		/* Chaîne cluster suivie depuis l’origine */
				} else {						/* Milieu ou fin du fichier */
#if _USE_FASTSEEK
					if (fp->cltbl) {
						clst = clmt_clust(fp, fp->fptr);
					} else
#endif
					{
						clst = get_fat(&fp->obj, fp->clust);
					}
				}
				if (clst < 2) ABORT(fs, FR_INT_ERR);
				if (clst == 0xFFFFFFFF) ABORT(fs, FR_DISK_ERR);
				fp->clust = clst;
			}
			sect = clust2sect(fs, fp->clust);
			if (!sect) ABORT(fs, FR_INT_ERR);
			sect += csect;
			cc = btr / SS(fs);					/* Bytes restants au moins égaux à un secteur */
			if (cc) {							/* Lecture directe du maximum de secteurs contigus */
				if (csect + cc > fs->csize) {	/* Limitation à la frontière cluster */
					cc = fs->csize - csect;
				}
				if (disk_read(fs->drv, rbuff, sect, cc) != RES_OK) ABORT(fs, FR_DISK_ERR);
#if !_FS_READONLY && _FS_MINIMIZE <= 2			/* Secteur dirty : priorité aux données en cache */
#if _FS_TINY
				if (fs->wflag && fs->winsect - sect < cc) {
					mem_cpy(rbuff + ((fs->winsect - sect) * SS(fs)), fs->win, SS(fs));
				}
#else
				if ((fp->flag & FA_DIRTY) && fp->sect - sect < cc) {
					mem_cpy(rbuff + ((fp->sect - sect) * SS(fs)), fp->buf, SS(fs));
				}
#endif
#endif
				rcnt = SS(fs) * cc;				/* Nombre de bytes transférés */
				continue;
			}
#if !_FS_TINY
			if (fp->sect != sect) {
#if !_FS_READONLY
				if (fp->flag & FA_DIRTY) {		/* Writeback du cache secteur dirty */
					if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
					fp->flag &= (BYTE)~FA_DIRTY;
				}
#endif
				if (disk_read(fs->drv, fp->buf, sect, 1) != RES_OK)	ABORT(fs, FR_DISK_ERR);
			}
#endif
			fp->sect = sect;
		}
		rcnt = SS(fs) - (UINT)fp->fptr % SS(fs);	/* Bytes restants dans secteur */
		if (rcnt > btr) rcnt = btr;					/* Limitation à btr si nécessaire */
#if _FS_TINY
		if (move_window(fs, fp->sect) != FR_OK) ABORT(fs, FR_DISK_ERR);
		mem_cpy(rbuff, fs->win + fp->fptr % SS(fs), rcnt);
#else
		mem_cpy(rbuff, fp->buf + fp->fptr % SS(fs), rcnt);
#endif
	}

	LEAVE_FF(fs, FR_OK);
}

#if !_FS_READONLY

FRESULT f_write (
	FIL* fp,			/* Pointeur objet fichier */
	const void* buff,
	UINT btw,			/* Nombre de bytes à écrire */
	UINT* bw			/* Nombre de bytes effectivement écrits en sortie */
)
{
	FRESULT res;
	FATFS *fs;
	DWORD clst, sect;
	UINT wcnt, cc, csect;
	const BYTE *wbuff = (const BYTE*)buff;

	*bw = 0;
	res = validate(&fp->obj, &fs);			/* Validation objet fichier */
	if (res != FR_OK || (res = (FRESULT)fp->err) != FR_OK) LEAVE_FF(fs, res);
	if (!(fp->flag & FA_WRITE)) LEAVE_FF(fs, FR_DENIED);

	/* FATxx : taille fichier strictement inférieure à 4 GiB */
	if ((!_FS_EXFAT || fs->fs_type != FS_EXFAT) && (DWORD)(fp->fptr + btw) < (DWORD)fp->fptr) {
		btw = (UINT)(0xFFFFFFFF - (DWORD)fp->fptr);
	}

	for ( ;  btw;
		wbuff += wcnt, fp->fptr += wcnt, fp->obj.objsize = (fp->fptr > fp->obj.objsize) ? fp->fptr : fp->obj.objsize, *bw += wcnt, btw -= wcnt) {
		if (fp->fptr % SS(fs) == 0) {
			csect = (UINT)(fp->fptr / SS(fs)) & (fs->csize - 1);	/* Offset secteur dans cluster */
			if (csect == 0) {
				if (fp->fptr == 0) {
					clst = fp->obj.sclust;
					if (clst == 0) {		/* Aucun cluster alloué */
						clst = create_chain(&fp->obj, 0);	/* Création d’une nouvelle chaîne cluster */
					}
				} else {					/* Milieu ou fin du fichier */
#if _USE_FASTSEEK
					if (fp->cltbl) {
						clst = clmt_clust(fp, fp->fptr);
					} else
#endif
					{
						clst = create_chain(&fp->obj, fp->clust);
					}
				}
				if (clst == 0) break;		/* Allocation cluster impossible : disque plein */
				if (clst == 1) ABORT(fs, FR_INT_ERR);
				if (clst == 0xFFFFFFFF) ABORT(fs, FR_DISK_ERR);
				fp->clust = clst;
				if (fp->obj.sclust == 0) fp->obj.sclust = clst;	/* Cluster initial renseigné à la première écriture */
			}
#if _FS_TINY
			if (fs->winsect == fp->sect && sync_window(fs) != FR_OK) ABORT(fs, FR_DISK_ERR);	/* Writeback du cache secteur */
#else
			if (fp->flag & FA_DIRTY) {		/* Writeback du cache secteur */
				if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
				fp->flag &= (BYTE)~FA_DIRTY;
			}
#endif
			sect = clust2sect(fs, fp->clust);
			if (!sect) ABORT(fs, FR_INT_ERR);
			sect += csect;
			cc = btw / SS(fs);				/* Bytes restants au moins égaux à un secteur */
			if (cc) {						/* Écriture directe du maximum de secteurs contigus */
				if (csect + cc > fs->csize) {	/* Limitation à la frontière cluster */
					cc = fs->csize - csect;
				}
				if (disk_write(fs->drv, wbuff, sect, cc) != RES_OK) ABORT(fs, FR_DISK_ERR);
#if _FS_MINIMIZE <= 2
#if _FS_TINY
				if (fs->winsect - sect < cc) {	/* Cache rechargé si invalidé par écriture directe */
					mem_cpy(fs->win, wbuff + ((fs->winsect - sect) * SS(fs)), SS(fs));
					fs->wflag = 0;
				}
#else
				if (fp->sect - sect < cc) { /* Cache rechargé si invalidé par écriture directe */
					mem_cpy(fp->buf, wbuff + ((fp->sect - sect) * SS(fs)), SS(fs));
					fp->flag &= (BYTE)~FA_DIRTY;
				}
#endif
#endif
				wcnt = SS(fs) * cc;		/* Nombre de bytes transférés */
				continue;
			}
#if _FS_TINY
			if (fp->fptr >= fp->obj.objsize) {	/* Bord de croissance : éviter chargement cache inutile */
				if (sync_window(fs) != FR_OK) ABORT(fs, FR_DISK_ERR);
				fs->winsect = sect;
			}
#else
			if (fp->sect != sect &&
				fp->fptr < fp->obj.objsize &&
				disk_read(fs->drv, fp->buf, sect, 1) != RES_OK) {
					ABORT(fs, FR_DISK_ERR);
			}
#endif
			fp->sect = sect;
		}
		wcnt = SS(fs) - (UINT)fp->fptr % SS(fs);	/* Bytes restants dans secteur */
		if (wcnt > btw) wcnt = btw;					/* Limitation à btw si nécessaire */
#if _FS_TINY
		if (move_window(fs, fp->sect) != FR_OK) ABORT(fs, FR_DISK_ERR);
		mem_cpy(fs->win + fp->fptr % SS(fs), wbuff, wcnt);	/* Données ajustées au secteur */
		fs->wflag = 1;
#else
		mem_cpy(fp->buf + fp->fptr % SS(fs), wbuff, wcnt);	/* Données ajustées au secteur */
		fp->flag |= FA_DIRTY;
#endif
	}

	fp->flag |= FA_MODIFIED;

	LEAVE_FF(fs, FR_OK);
}

FRESULT f_sync (
	FIL* fp		/* Pointeur objet fichier */
)
{
	FRESULT res;
	FATFS *fs;
	DWORD tm;
	BYTE *dir;
#if _FS_EXFAT
	DIR dj;
	DEF_NAMBUF
#endif

	res = validate(&fp->obj, &fs);	/* Validation objet fichier */
	if (res == FR_OK) {
		if (fp->flag & FA_MODIFIED) {
#if !_FS_TINY
			if (fp->flag & FA_DIRTY) {	/* Writeback des données en cache si nécessaire */
				if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) LEAVE_FF(fs, FR_DISK_ERR);
				fp->flag &= (BYTE)~FA_DIRTY;
			}
#endif

			tm = GET_FATTIME();
#if _FS_EXFAT
			if (fs->fs_type == FS_EXFAT) {
				res = fill_first_frag(&fp->obj);
				if (res == FR_OK) {
					res = fill_last_frag(&fp->obj, fp->clust, 0xFFFFFFFF);
				}
				if (res == FR_OK) {
					INIT_NAMBUF(fs);
					res = load_obj_dir(&dj, &fp->obj);
					if (res == FR_OK) {
						fs->dirbuf[XDIR_Attr] |= AM_ARC;				/* Activation bit archive */
						fs->dirbuf[XDIR_GenFlags] = fp->obj.stat | 1;
						st_dword(fs->dirbuf + XDIR_FstClus, fp->obj.sclust);
						st_qword(fs->dirbuf + XDIR_FileSize, fp->obj.objsize);
						st_qword(fs->dirbuf + XDIR_ValidFileSize, fp->obj.objsize);
						st_dword(fs->dirbuf + XDIR_ModTime, tm);
						fs->dirbuf[XDIR_ModTime10] = 0;
						st_dword(fs->dirbuf + XDIR_AccTime, 0);
						res = store_xdir(&dj);
						if (res == FR_OK) {
							res = sync_fs(fs);
							fp->flag &= (BYTE)~FA_MODIFIED;
						}
					}
					FREE_NAMBUF();
				}
			} else
#endif
			{
				res = move_window(fs, fp->dir_sect);
				if (res == FR_OK) {
					dir = fp->dir_ptr;
					dir[DIR_Attr] |= AM_ARC;						/* Activation bit archive */
					st_clust(fp->obj.fs, dir, fp->obj.sclust);
					st_dword(dir + DIR_FileSize, (DWORD)fp->obj.objsize);
					st_dword(dir + DIR_ModTime, tm);
					st_word(dir + DIR_LstAccDate, 0);
					fs->wflag = 1;
					res = sync_fs(fs);
					fp->flag &= (BYTE)~FA_MODIFIED;
				}
			}
		}
	}

	LEAVE_FF(fs, res);
}

#endif

FRESULT f_close (
	FIL* fp
)
{
	FRESULT res;
	FATFS *fs;

#if !_FS_READONLY
	res = f_sync(fp);
	if (res == FR_OK)
#endif
	{
		res = validate(&fp->obj, &fs);
		if (res == FR_OK) {
#if _FS_LOCK != 0
			res = dec_lock(fp->obj.lockid);
			if (res == FR_OK)
#endif
			{
				fp->obj.fs = 0;
			}
#if _FS_REENTRANT
			unlock_fs(fs, FR_OK);
#endif
		}
	}
	return res;
}

#if _FS_RPATH >= 1

#if _VOLUMES >= 2
FRESULT f_chdrive (
	const TCHAR* path
)
{
	int vol;

	vol = get_ldnumber(&path);
	if (vol < 0) return FR_INVALID_DRIVE;

	CurrVol = (BYTE)vol;

	return FR_OK;
}
#endif

FRESULT f_chdir (
	const TCHAR* path
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
	DEF_NAMBUF

	/* Recherche lecteur logique */
	res = find_volume(&path, &fs, 0);
	if (res == FR_OK) {
		dj.obj.fs = fs;
		INIT_NAMBUF(fs);
		res = follow_path(&dj, path);
		if (res == FR_OK) {
			if (dj.fn[NSFLAG] & NS_NONAME) {
				fs->cdir = dj.obj.sclust;	/* Répertoire d’origine */
#if _FS_EXFAT
				if (fs->fs_type == FS_EXFAT) {
					fs->cdc_scl = dj.obj.c_scl;
					fs->cdc_size = dj.obj.c_size;
					fs->cdc_ofs = dj.obj.c_ofs;
				}
#endif
			} else {
				if (dj.obj.attr & AM_DIR) {
#if _FS_EXFAT
					if (fs->fs_type == FS_EXFAT) {
						fs->cdir = ld_dword(fs->dirbuf + XDIR_FstClus);
						fs->cdc_scl = dj.obj.sclust;						/* Informations du répertoire parent conservées */
						fs->cdc_size = ((DWORD)dj.obj.objsize & 0xFFFFFF00) | dj.obj.stat;
						fs->cdc_ofs = dj.blk_ofs;
					} else
#endif
					{
						fs->cdir = ld_clust(fs, dj.dir);
					}
				} else {
					res = FR_NO_PATH;		/* Chemin atteint un fichier */
				}
			}
		}
		FREE_NAMBUF();
		if (res == FR_NO_FILE) res = FR_NO_PATH;
	}

	LEAVE_FF(fs, res);
}

#if _FS_RPATH >= 2
FRESULT f_getcwd (
	TCHAR* buff,
	UINT len		/* Taille chemin */
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
	UINT i, n;
	DWORD ccl;
	TCHAR *tp;
	FILINFO fno;
	DEF_NAMBUF

	*buff = 0;
	/* Recherche lecteur logique */
	res = find_volume((const TCHAR**)&buff, &fs, 0);
	if (res == FR_OK) {
		dj.obj.fs = fs;
		INIT_NAMBUF(fs);
		i = len;			/* Fond du buffer : base stack répertoire */
		if (!_FS_EXFAT || fs->fs_type != FS_EXFAT) {	/* exFAT : getcwd indisponible, retour chemin racine */
			dj.obj.sclust = fs->cdir;
			while ((ccl = dj.obj.sclust) != 0) {	/* Parcours tant que répertoire courant non racine */
				res = dir_sdi(&dj, 1 * SZDIRE);
				if (res != FR_OK) break;
				res = move_window(fs, dj.sect);
				if (res != FR_OK) break;
				dj.obj.sclust = ld_clust(fs, dj.dir);
				res = dir_sdi(&dj, 0);
				if (res != FR_OK) break;
				do {
					res = dir_read(&dj, 0);
					if (res != FR_OK) break;
					if (ccl == ld_clust(fs, dj.dir)) break;
					res = dir_next(&dj, 0);
				} while (res == FR_OK);
				if (res == FR_NO_FILE) res = FR_INT_ERR;/* Entrée introuvable impossible ici */
				if (res != FR_OK) break;
				get_fileinfo(&dj, &fno);
				for (n = 0; fno.fname[n]; n++) ;
				if (i < n + 3) {
					res = FR_NOT_ENOUGH_CORE; break;
				}
				while (n) buff[--i] = fno.fname[--n];
				buff[--i] = '/';
			}
		}
		tp = buff;
		if (res == FR_OK) {
#if _VOLUMES >= 2
			*tp++ = '0' + CurrVol;
			*tp++ = ':';
#endif
			if (i == len) {
				*tp++ = '/';
			} else {
				do
					*tp++ = buff[i++];
				while (i < len);
			}
		}
		*tp = 0;
		FREE_NAMBUF();
	}

	LEAVE_FF(fs, res);
}

#endif
#endif

#if _FS_MINIMIZE <= 2

FRESULT f_lseek (
	FIL* fp,		/* Pointeur objet fichier */
	FSIZE_t ofs
)
{
	FRESULT res;
	FATFS *fs;
	DWORD clst, bcs, nsect;
	FSIZE_t ifptr;
#if _USE_FASTSEEK
	DWORD cl, pcl, ncl, tcl, dsc, tlen, ulen, *tbl;
#endif

	res = validate(&fp->obj, &fs);		/* Validation objet fichier */
	if (res == FR_OK) res = (FRESULT)fp->err;
#if _FS_EXFAT && !_FS_READONLY
	if (res == FR_OK && fs->fs_type == FS_EXFAT) {
		res = fill_last_frag(&fp->obj, fp->clust, 0xFFFFFFFF);
	}
#endif
	if (res != FR_OK) LEAVE_FF(fs, res);

#if _USE_FASTSEEK
	if (fp->cltbl) {
		if (ofs == CREATE_LINKMAP) {
			tbl = fp->cltbl;
			tlen = *tbl++; ulen = 2;	/* Taille table fournie et taille requise */
			cl = fp->obj.sclust;
			if (cl) {
				do {

					tcl = cl; ncl = 0; ulen += 2;	/* Début, longueur et items utilisés */
					do {
						pcl = cl; ncl++;
						cl = get_fat(&fp->obj, cl);
						if (cl <= 1) ABORT(fs, FR_INT_ERR);
						if (cl == 0xFFFFFFFF) ABORT(fs, FR_DISK_ERR);
					} while (cl == pcl + 1);
					if (ulen <= tlen) {
						*tbl++ = ncl; *tbl++ = tcl;
					}
				} while (cl < fs->n_fatent);
			}
			*fp->cltbl = ulen;
			if (ulen <= tlen) {
				*tbl = 0;
			} else {
				res = FR_NOT_ENOUGH_CORE;	/* Table fournie trop petite */
			}
		} else {
			if (ofs > fp->obj.objsize) ofs = fp->obj.objsize;	/* Offset limité à la taille fichier */
			fp->fptr = ofs;
			if (ofs) {
				fp->clust = clmt_clust(fp, ofs - 1);
				dsc = clust2sect(fs, fp->clust);
				if (!dsc) ABORT(fs, FR_INT_ERR);
				dsc += (DWORD)((ofs - 1) / SS(fs)) & (fs->csize - 1);
				if (fp->fptr % SS(fs) && dsc != fp->sect) {
#if !_FS_TINY
#if !_FS_READONLY
					if (fp->flag & FA_DIRTY) {		/* Writeback du cache secteur dirty */
						if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
						fp->flag &= (BYTE)~FA_DIRTY;
					}
#endif
					if (disk_read(fs->drv, fp->buf, dsc, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
#endif
					fp->sect = dsc;
				}
			}
		}
	} else
#endif

	{
#if _FS_EXFAT
		if (fs->fs_type != FS_EXFAT && ofs >= 0x100000000) ofs = 0xFFFFFFFF;	/* FATxx : limite 4 GiB-1 */
#endif
		if (ofs > fp->obj.objsize && (_FS_READONLY || !(fp->flag & FA_WRITE))) {	/* Lecture seule : offset limité à la taille fichier */
			ofs = fp->obj.objsize;
		}
		ifptr = fp->fptr;
		fp->fptr = nsect = 0;
		if (ofs) {
			bcs = (DWORD)fs->csize * SS(fs);	/* Taille cluster en bytes */
			if (ifptr > 0 &&
				(ofs - 1) / bcs >= (ifptr - 1) / bcs) {	/* Seek au même cluster ou suivant */
				fp->fptr = (ifptr - 1) & ~(FSIZE_t)(bcs - 1);	/* Parcours depuis cluster courant */
				ofs -= fp->fptr;
				clst = fp->clust;
			} else {									/* Seek vers cluster antérieur */
				clst = fp->obj.sclust;					/* Parcours depuis premier cluster */
#if !_FS_READONLY
				if (clst == 0) {						/* Chaîne absente : création d’une nouvelle */
					clst = create_chain(&fp->obj, 0);
					if (clst == 1) ABORT(fs, FR_INT_ERR);
					if (clst == 0xFFFFFFFF) ABORT(fs, FR_DISK_ERR);
					fp->obj.sclust = clst;
				}
#endif
				fp->clust = clst;
			}
			if (clst != 0) {
				while (ofs > bcs) {
					ofs -= bcs; fp->fptr += bcs;
#if !_FS_READONLY
					if (fp->flag & FA_WRITE) {
						if (_FS_EXFAT && fp->fptr > fp->obj.objsize) {	/* Sans chaîne FAT : objsize requis pour générer valeur FAT */
							fp->obj.objsize = fp->fptr;
							fp->flag |= FA_MODIFIED;
						}
						clst = create_chain(&fp->obj, clst);	/* Parcours avec extension forcée */
						if (clst == 0) {				/* Disque plein : taille fichier limitée */
							ofs = 0; break;
						}
					} else
#endif
					{
						clst = get_fat(&fp->obj, clst);	/* Hors mode write : parcours chaîne cluster */
					}
					if (clst == 0xFFFFFFFF) ABORT(fs, FR_DISK_ERR);
					if (clst <= 1 || clst >= fs->n_fatent) ABORT(fs, FR_INT_ERR);
					fp->clust = clst;
				}
				fp->fptr += ofs;
				if (ofs % SS(fs)) {
					nsect = clust2sect(fs, clst);	/* Secteur courant */
					if (!nsect) ABORT(fs, FR_INT_ERR);
					nsect += (DWORD)(ofs / SS(fs));
				}
			}
		}
		if (!_FS_READONLY && fp->fptr > fp->obj.objsize) {		/* Flag change si taille fichier étendue */
			fp->obj.objsize = fp->fptr;
			fp->flag |= FA_MODIFIED;
		}
		if (fp->fptr % SS(fs) && nsect != fp->sect) {
#if !_FS_TINY
#if !_FS_READONLY
			if (fp->flag & FA_DIRTY) {			/* Writeback du cache secteur dirty */
				if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
				fp->flag &= (BYTE)~FA_DIRTY;
			}
#endif
			if (disk_read(fs->drv, fp->buf, nsect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
#endif
			fp->sect = nsect;
		}
	}

	LEAVE_FF(fs, res);
}

#if _FS_MINIMIZE <= 1

FRESULT f_opendir (
	DIR* dp,
	const TCHAR* path
)
{
	FRESULT res;
	FATFS *fs;
	_FDID *obj;
	DEF_NAMBUF

	if (!dp) return FR_INVALID_OBJECT;

	/* Recherche lecteur logique */
	obj = &dp->obj;
	res = find_volume(&path, &fs, 0);
	if (res == FR_OK) {
		obj->fs = fs;
		INIT_NAMBUF(fs);
		res = follow_path(dp, path);
		if (res == FR_OK) {
			if (!(dp->fn[NSFLAG] & NS_NONAME)) {
				if (obj->attr & AM_DIR) {
#if _FS_EXFAT
					if (fs->fs_type == FS_EXFAT) {
						obj->c_scl = obj->sclust;
						obj->c_size = ((DWORD)obj->objsize & 0xFFFFFF00) | obj->stat;
						obj->c_ofs = dp->blk_ofs;
						obj->sclust = ld_dword(fs->dirbuf + XDIR_FstClus);
						obj->objsize = ld_qword(fs->dirbuf + XDIR_FileSize);
						obj->stat = fs->dirbuf[XDIR_GenFlags] & 2;
					} else
#endif
					{
						obj->sclust = ld_clust(fs, dp->dir);
					}
				} else {
					res = FR_NO_PATH;
				}
			}
			if (res == FR_OK) {
				obj->id = fs->id;
				res = dir_sdi(dp, 0);
#if _FS_LOCK != 0
				if (res == FR_OK) {
					if (obj->sclust) {
						obj->lockid = inc_lock(dp, 0);
						if (!obj->lockid) res = FR_TOO_MANY_OPEN_FILES;
					} else {
						obj->lockid = 0;	/* Racine : aucun lock requis */
					}
				}
#endif
			}
		}
		FREE_NAMBUF();
		if (res == FR_NO_FILE) res = FR_NO_PATH;
	}
	if (res != FR_OK) obj->fs = 0;		/* Objet répertoire invalidé en cas d’échec */

	LEAVE_FF(fs, res);
}

FRESULT f_closedir (
	DIR *dp
)
{
	FRESULT res;
	FATFS *fs;

	res = validate(&dp->obj, &fs);			/* Validation objet fichier */
	if (res == FR_OK) {
#if _FS_LOCK != 0
		if (dp->obj.lockid) {
			res = dec_lock(dp->obj.lockid);
		}
		if (res == FR_OK)
#endif
		{
			dp->obj.fs = 0;
		}
#if _FS_REENTRANT
		unlock_fs(fs, FR_OK);
#endif
	}
	return res;
}

FRESULT f_readdir (
	DIR* dp,
	FILINFO* fno
)
{
	FRESULT res;
	FATFS *fs;
	DEF_NAMBUF

	res = validate(&dp->obj, &fs);
	if (res == FR_OK) {
		if (!fno) {
			res = dir_sdi(dp, 0);
		} else {
			INIT_NAMBUF(fs);
			res = dir_read(dp, 0);
			if (res == FR_NO_FILE) res = FR_OK;	/* Fin de répertoire ignorée */
			if (res == FR_OK) {
				get_fileinfo(dp, fno);
				res = dir_next(dp, 0);
				if (res == FR_NO_FILE) res = FR_OK;	/* Fin de répertoire ignorée pour le moment */
			}
			FREE_NAMBUF();
		}
	}
	LEAVE_FF(fs, res);
}

#if _USE_FIND

FRESULT f_findnext (
	DIR* dp,
	FILINFO* fno
)
{
	FRESULT res;

	for (;;) {
		res = f_readdir(dp, fno);
		if (res != FR_OK || !fno || !fno->fname[0]) break;	/* Arrêt si erreur ou fin de répertoire */
		if (pattern_matching(dp->pat, fno->fname, 0, 0)) break;
#if _USE_LFN != 0 && _USE_FIND == 2
		if (pattern_matching(dp->pat, fno->altname, 0, 0)) break;
#endif
	}
	return res;
}

FRESULT f_findfirst (
	DIR* dp,
	FILINFO* fno,
	const TCHAR* path,
	const TCHAR* pattern
)
{
	FRESULT res;

	dp->pat = pattern;		/* Conservation du pointer vers pattern */
	res = f_opendir(dp, path);
	if (res == FR_OK) {
		res = f_findnext(dp, fno);
	}
	return res;
}

#endif

#if _FS_MINIMIZE == 0

FRESULT f_stat (
	const TCHAR* path,
	FILINFO* fno
)
{
	FRESULT res;
	DIR dj;
	DEF_NAMBUF

	/* Recherche lecteur logique */
	res = find_volume(&path, &dj.obj.fs, 0);
	if (res == FR_OK) {
		INIT_NAMBUF(dj.obj.fs);
		res = follow_path(&dj, path);	/* Résolution chemin fichier */
		if (res == FR_OK) {
			if (dj.fn[NSFLAG] & NS_NONAME) {
				res = FR_INVALID_NAME;
			} else {
				if (fno) get_fileinfo(&dj, fno);
			}
		}
		FREE_NAMBUF();
	}

	LEAVE_FF(dj.obj.fs, res);
}

#if !_FS_READONLY

FRESULT f_getfree (
	const TCHAR* path,
	DWORD* nclst,
	FATFS** fatfs
)
{
	FRESULT res;
	FATFS *fs;
	DWORD nfree, clst, sect, stat;
	UINT i;
	BYTE *p;
	_FDID obj;

	/* Recherche lecteur logique */
	res = find_volume(&path, &fs, 0);
	if (res == FR_OK) {
		*fatfs = fs;
		/* Free_clst valide : retour sans scan complet */
		if (fs->free_clst <= fs->n_fatent - 2) {
			*nclst = fs->free_clst;
		} else {

			nfree = 0;
			if (fs->fs_type == FS_FAT12) {	/* FAT12 : entrées FAT non alignées secteur */
				clst = 2; obj.fs = fs;
				do {
					stat = get_fat(&obj, clst);
					if (stat == 0xFFFFFFFF) { res = FR_DISK_ERR; break; }
					if (stat == 1) { res = FR_INT_ERR; break; }
					if (stat == 0) nfree++;
				} while (++clst < fs->n_fatent);
			} else {
#if _FS_EXFAT
				if (fs->fs_type == FS_EXFAT) {	/* exFAT : scan bitmap */
					BYTE bm;
					UINT b;

					clst = fs->n_fatent - 2;
					sect = fs->database;
					i = 0;
					do {
						if (i == 0 && (res = move_window(fs, sect++)) != FR_OK) break;
						for (b = 8, bm = fs->win[i]; b && clst; b--, clst--) {
							if (!(bm & 1)) nfree++;
							bm >>= 1;
						}
						i = (i + 1) % SS(fs);
					} while (clst);
				} else
#endif
				{	/* FAT16/32 : entrées FAT alignées secteur */
					clst = fs->n_fatent; sect = fs->fatbase;
					i = 0; p = 0;
					do {
						if (i == 0) {
							res = move_window(fs, sect++);
							if (res != FR_OK) break;
							p = fs->win;
							i = SS(fs);
						}
						if (fs->fs_type == FS_FAT16) {
							if (ld_word(p) == 0) nfree++;
							p += 2; i -= 2;
						} else {
							if ((ld_dword(p) & 0x0FFFFFFF) == 0) nfree++;
							p += 4; i -= 4;
						}
					} while (--clst);
				}
			}
			*nclst = nfree;
			fs->free_clst = nfree;
			fs->fsi_flag |= 1;		/* Mise à jour FSINFO requise */
		}
	}

	LEAVE_FF(fs, res);
}

FRESULT f_truncate (
	FIL* fp		/* Pointeur objet fichier */
)
{
	FRESULT res;
	FATFS *fs;
	DWORD ncl;

	res = validate(&fp->obj, &fs);	/* Validation objet fichier */
	if (res != FR_OK || (res = (FRESULT)fp->err) != FR_OK) LEAVE_FF(fs, res);
	if (!(fp->flag & FA_WRITE)) LEAVE_FF(fs, FR_DENIED);

	if (fp->fptr < fp->obj.objsize) {	/* Truncate uniquement si fptr !=eof */
		if (fp->fptr == 0) {	/* Taille zéro : suppression complète de la chaîne cluster */
			res = remove_chain(&fp->obj, fp->obj.sclust, 0);
			fp->obj.sclust = 0;
		} else {				/* Truncate partiel : suppression des clusters restants */
			ncl = get_fat(&fp->obj, fp->clust);
			res = FR_OK;
			if (ncl == 0xFFFFFFFF) res = FR_DISK_ERR;
			if (ncl == 1) res = FR_INT_ERR;
			if (res == FR_OK && ncl < fs->n_fatent) {
				res = remove_chain(&fp->obj, ncl, fp->clust);
			}
		}
		fp->obj.objsize = fp->fptr;	/* Taille fichier fixée à la position read/write courante */
		fp->flag |= FA_MODIFIED;
#if !_FS_TINY
		if (res == FR_OK && (fp->flag & FA_DIRTY)) {
			if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) {
				res = FR_DISK_ERR;
			} else {
				fp->flag &= (BYTE)~FA_DIRTY;
			}
		}
#endif
		if (res != FR_OK) ABORT(fs, res);
	}

	LEAVE_FF(fs, res);
}

FRESULT f_unlink (
	const TCHAR* path
)
{
	FRESULT res;
	DIR dj, sdj;
	DWORD dclst = 0;
	FATFS *fs;
#if _FS_EXFAT
	_FDID obj;
#endif
	DEF_NAMBUF

	/* Recherche lecteur logique */
	res = find_volume(&path, &fs, FA_WRITE);
	dj.obj.fs = fs;
	if (res == FR_OK) {
		INIT_NAMBUF(fs);
		res = follow_path(&dj, path);		/* Résolution chemin fichier */
		if (_FS_RPATH && res == FR_OK && (dj.fn[NSFLAG] & NS_DOT)) {
			res = FR_INVALID_NAME;			/* Suppression entrée point interdite */
		}
#if _FS_LOCK != 0
		if (res == FR_OK) res = chk_lock(&dj, 2);
#endif
		if (res == FR_OK) {
			if (dj.fn[NSFLAG] & NS_NONAME) {
				res = FR_INVALID_NAME;		/* Suppression répertoire d’origine interdite */
			} else {
				if (dj.obj.attr & AM_RDO) {
					res = FR_DENIED;		/* Suppression objet R/O interdite */
				}
			}
			if (res == FR_OK) {
#if _FS_EXFAT
				obj.fs = fs;
				if (fs->fs_type == FS_EXFAT) {
					obj.sclust = dclst = ld_dword(fs->dirbuf + XDIR_FstClus);
					obj.objsize = ld_qword(fs->dirbuf + XDIR_FileSize);
					obj.stat = fs->dirbuf[XDIR_GenFlags] & 2;
				} else
#endif
				{
					dclst = ld_clust(fs, dj.dir);
				}
				if (dj.obj.attr & AM_DIR) {
#if _FS_RPATH != 0
					if (dclst == fs->cdir) {
						res = FR_DENIED;
					} else
#endif
					{
						sdj.obj.fs = fs;
						sdj.obj.sclust = dclst;
#if _FS_EXFAT
						if (fs->fs_type == FS_EXFAT) {
							sdj.obj.objsize = obj.objsize;
							sdj.obj.stat = obj.stat;
						}
#endif
						res = dir_sdi(&sdj, 0);
						if (res == FR_OK) {
							res = dir_read(&sdj, 0);
							if (res == FR_OK) res = FR_DENIED;
							if (res == FR_NO_FILE) res = FR_OK;
						}
					}
				}
			}
			if (res == FR_OK) {
				res = dir_remove(&dj);
				if (res == FR_OK && dclst) {
#if _FS_EXFAT
					res = remove_chain(&obj, dclst, 0);
#else
					res = remove_chain(&dj.obj, dclst, 0);
#endif
				}
				if (res == FR_OK) res = sync_fs(fs);
			}
		}
		FREE_NAMBUF();
	}

	LEAVE_FF(fs, res);
}

FRESULT f_mkdir (
	const TCHAR* path
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
	BYTE *dir;
	UINT n;
	DWORD dsc, dcl, pcl, tm;
	DEF_NAMBUF

	/* Recherche lecteur logique */
	res = find_volume(&path, &fs, FA_WRITE);
	dj.obj.fs = fs;
	if (res == FR_OK) {
		INIT_NAMBUF(fs);
		res = follow_path(&dj, path);			/* Résolution chemin fichier */
		if (res == FR_OK) res = FR_EXIST;
		if (_FS_RPATH && res == FR_NO_FILE && (dj.fn[NSFLAG] & NS_DOT)) {
			res = FR_INVALID_NAME;
		}
		if (res == FR_NO_FILE) {
			dcl = create_chain(&dj.obj, 0);
			dj.obj.objsize = (DWORD)fs->csize * SS(fs);
			res = FR_OK;
			if (dcl == 0) res = FR_DENIED;		/* Aucun espace pour allouer un cluster */
			if (dcl == 1) res = FR_INT_ERR;
			if (dcl == 0xFFFFFFFF) res = FR_DISK_ERR;
			if (res == FR_OK) res = sync_window(fs);
			tm = GET_FATTIME();
			if (res == FR_OK) {
				dsc = clust2sect(fs, dcl);
				dir = fs->win;
				mem_set(dir, 0, SS(fs));
				if (!_FS_EXFAT || fs->fs_type != FS_EXFAT) {
					mem_set(dir + DIR_Name, ' ', 11);
					dir[DIR_Name] = '.';
					dir[DIR_Attr] = AM_DIR;
					st_dword(dir + DIR_ModTime, tm);
					st_clust(fs, dir, dcl);
					mem_cpy(dir + SZDIRE, dir, SZDIRE);
					dir[SZDIRE + 1] = '.'; pcl = dj.obj.sclust;
					if (fs->fs_type == FS_FAT32 && pcl == fs->dirbase) pcl = 0;
					st_clust(fs, dir + SZDIRE, pcl);
				}
				for (n = fs->csize; n; n--) {
					fs->winsect = dsc++;
					fs->wflag = 1;
					res = sync_window(fs);
					if (res != FR_OK) break;
					mem_set(dir, 0, SS(fs));
				}
			}
			if (res == FR_OK) {
				res = dir_register(&dj);
			}
			if (res == FR_OK) {
#if _FS_EXFAT
				if (fs->fs_type == FS_EXFAT) {
					st_dword(fs->dirbuf + XDIR_ModTime, tm);
					st_dword(fs->dirbuf + XDIR_FstClus, dcl);
					st_dword(fs->dirbuf + XDIR_FileSize, (DWORD)dj.obj.objsize);	/* Taille fichier obligatoirement valide */
					st_dword(fs->dirbuf + XDIR_ValidFileSize, (DWORD)dj.obj.objsize);
					fs->dirbuf[XDIR_GenFlags] = 3;				/* Objet initialement contigu */
					fs->dirbuf[XDIR_Attr] = AM_DIR;
					res = store_xdir(&dj);
				} else
#endif
				{
					dir = dj.dir;
					st_dword(dir + DIR_ModTime, tm);
					st_clust(fs, dir, dcl);
					dir[DIR_Attr] = AM_DIR;
					fs->wflag = 1;
				}
				if (res == FR_OK) {
					res = sync_fs(fs);
				}
			} else {
				remove_chain(&dj.obj, dcl, 0);		/* Échec register : suppression chaîne cluster */
			}
		}
		FREE_NAMBUF();
	}

	LEAVE_FF(fs, res);
}

FRESULT f_rename (
	const TCHAR* path_old,
	const TCHAR* path_new
)
{
	FRESULT res;
	DIR djo, djn;
	FATFS *fs;
	BYTE buf[_FS_EXFAT ? SZDIRE * 2 : 24], *dir;
	DWORD dw;
	DEF_NAMBUF

	get_ldnumber(&path_new);						/* Préfixe lecteur du nouveau nom supprimé */
	res = find_volume(&path_old, &fs, FA_WRITE);
	if (res == FR_OK) {
		djo.obj.fs = fs;
		INIT_NAMBUF(fs);
		res = follow_path(&djo, path_old);
		if (res == FR_OK && (djo.fn[NSFLAG] & (NS_DOT | NS_NONAME))) res = FR_INVALID_NAME;
#if _FS_LOCK != 0
		if (res == FR_OK) {
			res = chk_lock(&djo, 2);
		}
#endif
		if (res == FR_OK) {
#if _FS_EXFAT
			if (fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
				BYTE nf, nn;
				WORD nh;

				mem_cpy(buf, fs->dirbuf, SZDIRE * 2);	/* Conservation ancienne entrée 85+C0 */
				mem_cpy(&djn, &djo, sizeof djo);
				res = follow_path(&djn, path_new);
				if (res == FR_OK) {
					res = (djn.obj.sclust == djo.obj.sclust && djn.dptr == djo.dptr) ? FR_NO_FILE : FR_EXIST;
				}
				if (res == FR_NO_FILE) {
					res = dir_register(&djn);
					if (res == FR_OK) {
						nf = fs->dirbuf[XDIR_NumSec]; nn = fs->dirbuf[XDIR_NumName];
						nh = ld_word(fs->dirbuf + XDIR_NameHash);
						mem_cpy(fs->dirbuf, buf, SZDIRE * 2);
						fs->dirbuf[XDIR_NumSec] = nf; fs->dirbuf[XDIR_NumName] = nn;
						st_word(fs->dirbuf + XDIR_NameHash, nh);
/* Section critique : interruption susceptible de créer un cross-link */
						res = store_xdir(&djn);
					}
				}
			} else
#endif
			{	/* FAT12/FAT16/FAT32 */
				mem_cpy(buf, djo.dir + DIR_Attr, 21);	/* Conservation informations objet hors nom */
				mem_cpy(&djn, &djo, sizeof (DIR));
				res = follow_path(&djn, path_new);
				if (res == FR_OK) {
					res = (djn.obj.sclust == djo.obj.sclust && djn.dptr == djo.dptr) ? FR_NO_FILE : FR_EXIST;
				}
				if (res == FR_NO_FILE) {
					res = dir_register(&djn);
					if (res == FR_OK) {
						dir = djn.dir;					/* Copie informations objet hors nom */
						mem_cpy(dir + 13, buf + 2, 19);
						dir[DIR_Attr] = buf[0] | AM_ARC;
						fs->wflag = 1;
						if ((dir[DIR_Attr] & AM_DIR) && djo.obj.sclust != djn.obj.sclust) {	/* Mise à jour entrée parent .. si nécessaire */
							dw = clust2sect(fs, ld_clust(fs, dir));
							if (!dw) {
								res = FR_INT_ERR;
							} else {
/* Section critique : interruption susceptible de créer un cross-link */
								res = move_window(fs, dw);
								dir = fs->win + SZDIRE * 1;	/* Pointer vers entrée parent .. */
								if (res == FR_OK && dir[1] == '.') {
									st_clust(fs, dir, djn.obj.sclust);
									fs->wflag = 1;
								}
							}
						}
					}
				}
			}
			if (res == FR_OK) {
				res = dir_remove(&djo);
				if (res == FR_OK) {
					res = sync_fs(fs);
				}
			}
/* Fin de section critique */
		}
		FREE_NAMBUF();
	}

	LEAVE_FF(fs, res);
}

#endif
#endif
#endif
#endif

#if _USE_CHMOD && !_FS_READONLY

FRESULT f_chmod (
	const TCHAR* path,
	BYTE attr,
	BYTE mask
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
	DEF_NAMBUF

	res = find_volume(&path, &fs, FA_WRITE);	/* Recherche lecteur logique */
	dj.obj.fs = fs;
	if (res == FR_OK) {
		INIT_NAMBUF(fs);
		res = follow_path(&dj, path);	/* Résolution chemin fichier */
		if (res == FR_OK && (dj.fn[NSFLAG] & (NS_DOT | NS_NONAME))) res = FR_INVALID_NAME;
		if (res == FR_OK) {
			mask &= AM_RDO|AM_HID|AM_SYS|AM_ARC;
#if _FS_EXFAT
			if (fs->fs_type == FS_EXFAT) {
				fs->dirbuf[XDIR_Attr] = (attr & mask) | (fs->dirbuf[XDIR_Attr] & (BYTE)~mask);
				res = store_xdir(&dj);
			} else
#endif
			{
				dj.dir[DIR_Attr] = (attr & mask) | (dj.dir[DIR_Attr] & (BYTE)~mask);
				fs->wflag = 1;
			}
			if (res == FR_OK) {
				res = sync_fs(fs);
			}
		}
		FREE_NAMBUF();
	}

	LEAVE_FF(fs, res);
}

FRESULT f_utime (
	const TCHAR* path,
	const FILINFO* fno
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
	DEF_NAMBUF

	res = find_volume(&path, &fs, FA_WRITE);	/* Recherche lecteur logique */
	dj.obj.fs = fs;
	if (res == FR_OK) {
		INIT_NAMBUF(fs);
		res = follow_path(&dj, path);	/* Résolution chemin fichier */
		if (res == FR_OK && (dj.fn[NSFLAG] & (NS_DOT | NS_NONAME))) res = FR_INVALID_NAME;
		if (res == FR_OK) {
#if _FS_EXFAT
			if (fs->fs_type == FS_EXFAT) {
				st_dword(fs->dirbuf + XDIR_ModTime, (DWORD)fno->fdate << 16 | fno->ftime);
				res = store_xdir(&dj);
			} else
#endif
			{
				st_dword(dj.dir + DIR_ModTime, (DWORD)fno->fdate << 16 | fno->ftime);
				fs->wflag = 1;
			}
			if (res == FR_OK) {
				res = sync_fs(fs);
			}
		}
		FREE_NAMBUF();
	}

	LEAVE_FF(fs, res);
}

#endif

#if _USE_LABEL

FRESULT f_getlabel (
	const TCHAR* path,
	TCHAR* label,
	DWORD* vsn
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
	UINT si, di;
#if _LFN_UNICODE || _FS_EXFAT
	WCHAR w;
#endif

	/* Recherche lecteur logique */
	res = find_volume(&path, &fs, 0);

	if (res == FR_OK && label) {
		dj.obj.fs = fs; dj.obj.sclust = 0;
		res = dir_sdi(&dj, 0);
		if (res == FR_OK) {
		 	res = dir_read(&dj, 1);
		 	if (res == FR_OK) {
#if _FS_EXFAT
				if (fs->fs_type == FS_EXFAT) {
					for (si = di = 0; si < dj.dir[XDIR_NumLabel]; si++) {	/* Label volume extrait de l’entrée 83 */
						w = ld_word(dj.dir + XDIR_Label + si * 2);
#if _LFN_UNICODE
						label[di++] = w;
#else
						w = ff_convert(w, 0);	/* Unicode vers OEM */
						if (w == 0) w = '?';
						if (_DF1S && w >= 0x100) label[di++] = (char)(w >> 8);
						label[di++] = (char)w;
#endif
					}
					label[di] = 0;
				} else
#endif
				{
					si = di = 0;		/* Label volume extrait d’AM_VOL avec conversion code */
					do {
#if _LFN_UNICODE
						w = (si < 11) ? dj.dir[si++] : ' ';
						if (IsDBCS1(w) && si < 11 && IsDBCS2(dj.dir[si])) {
							w = w << 8 | dj.dir[si++];
						}
						label[di++] = ff_convert(w, 1);	/* OEM vers Unicode */
#else
						label[di++] = dj.dir[si++];
#endif
					} while (di < 11);
					do {
						label[di] = 0;
						if (di == 0) break;
					} while (label[--di] == ' ');
				}
			}
		}
		if (res == FR_NO_FILE) {	/* Aucun label : retour chaîne vide */
			label[0] = 0;
			res = FR_OK;
		}
	}

	if (res == FR_OK && vsn) {
		res = move_window(fs, fs->volbase);
		if (res == FR_OK) {
			switch (fs->fs_type) {
			case FS_EXFAT:
				di = BPB_VolIDEx; break;

			case FS_FAT32:
				di = BS_VolID32; break;

			default:
				di = BS_VolID;
			}
			*vsn = ld_dword(fs->win + di);
		}
	}

	LEAVE_FF(fs, res);
}

#if !_FS_READONLY

FRESULT f_setlabel (
	const TCHAR* label
)
{
	FRESULT res;
	DIR dj;
	FATFS *fs;
	BYTE dirvn[22];
	UINT i, j, slen;
	WCHAR w;
	static const char badchr[] = "\"*+,.:;<=>\?[]|\x7F";

	/* Recherche lecteur logique */
	res = find_volume(&label, &fs, FA_WRITE);
	if (res != FR_OK) LEAVE_FF(fs, res);
	dj.obj.fs = fs;

	for (slen = 0; (UINT)label[slen] >= ' '; slen++) ;

#if _FS_EXFAT
	if (fs->fs_type == FS_EXFAT) {	/* Sur volume exFAT */
		for (i = j = 0; i < slen; ) {
			w = label[i++];
#if !_LFN_UNICODE
			if (IsDBCS1(w)) {
				w = (i < slen && IsDBCS2(label[i])) ? w << 8 | (BYTE)label[i++] : 0;
			}
			w = ff_convert(w, 1);
#endif
			if (w == 0 || chk_chr(badchr, w) || j == 22) {
				LEAVE_FF(fs, FR_INVALID_NAME);
			}
			st_word(dirvn + j, w); j += 2;
		}
		slen = j;
	} else
#endif
	{	/* Sur volume FAT12/16/32 */
		for ( ; slen && label[slen - 1] == ' '; slen--) ;
		if (slen) {
			dirvn[0] = 0; i = j = 0;
			do {
#if _LFN_UNICODE
				w = ff_convert(ff_wtoupper(label[i++]), 0);
#else
				w = (BYTE)label[i++];
				if (IsDBCS1(w)) {
					w = (j < 10 && i < slen && IsDBCS2(label[i])) ? w << 8 | (BYTE)label[i++] : 0;
				}
#if _USE_LFN != 0
				w = ff_convert(ff_wtoupper(ff_convert(w, 1)), 0);
#else
				if (IsLower(w)) w -= 0x20;
#ifdef _EXCVT
				if (w >= 0x80) w = ExCvt[w - 0x80];	/* Caractères étendus en majuscules SBCS */
#else
				if (!_DF1S && w >= 0x80) w = 0;		/* Configuration ASCII : caractères étendus refusés */
#endif
#endif
#endif
				if (w == 0 || chk_chr(badchr, w) || j >= (UINT)((w >= 0x100) ? 10 : 11)) {	/* Caractères interdits pour label volume refusés */
					LEAVE_FF(fs, FR_INVALID_NAME);
				}
				if (w >= 0x100) dirvn[j++] = (BYTE)(w >> 8);
				dirvn[j++] = (BYTE)w;
			} while (i < slen);
			while (j < 11) dirvn[j++] = ' ';
			if (dirvn[0] == DDEM) LEAVE_FF(fs, FR_INVALID_NAME);	/* DDEM initial : nom interdit */
		}
	}

	dj.obj.sclust = 0;
	res = dir_sdi(&dj, 0);
	if (res == FR_OK) {
		res = dir_read(&dj, 1);
		if (res == FR_OK) {
			if (_FS_EXFAT && fs->fs_type == FS_EXFAT) {
				dj.dir[XDIR_NumLabel] = (BYTE)(slen / 2);
				mem_cpy(dj.dir + XDIR_Label, dirvn, slen);
			} else {
				if (slen) {
					mem_cpy(dj.dir, dirvn, 11);
				} else {
					dj.dir[DIR_Name] = DDEM;
				}
			}
			fs->wflag = 1;
			res = sync_fs(fs);
		} else {
			if (res == FR_NO_FILE) {
				res = FR_OK;
				if (slen) {
					res = dir_alloc(&dj, 1);
					if (res == FR_OK) {
						mem_set(dj.dir, 0, SZDIRE);
						if (_FS_EXFAT && fs->fs_type == FS_EXFAT) {
							dj.dir[XDIR_Type] = 0x83;		/* Création entrée 83 */
							dj.dir[XDIR_NumLabel] = (BYTE)(slen / 2);
							mem_cpy(dj.dir + XDIR_Label, dirvn, slen);
						} else {
							dj.dir[DIR_Attr] = AM_VOL;
							mem_cpy(dj.dir, dirvn, 11);
						}
						fs->wflag = 1;
						res = sync_fs(fs);
					}
				}
			}
		}
	}

	LEAVE_FF(fs, res);
}

#endif
#endif

#if _USE_EXPAND && !_FS_READONLY

FRESULT f_expand (
	FIL* fp,		/* Pointeur objet fichier */
	FSIZE_t fsz,	/* Taille fichier à atteindre */
	BYTE opt		/* Mode 0 recherche/préparation, 1 recherche/allocation */
)
{
	FRESULT res;
	FATFS *fs;
	DWORD n, clst, stcl, scl, ncl, tcl, lclst;

	res = validate(&fp->obj, &fs);		/* Validation objet fichier */
	if (res != FR_OK || (res = (FRESULT)fp->err) != FR_OK) LEAVE_FF(fs, res);
	if (fsz == 0 || fp->obj.objsize != 0 || !(fp->flag & FA_WRITE)) LEAVE_FF(fs, FR_DENIED);
#if _FS_EXFAT
	if (fs->fs_type != FS_EXFAT && fsz >= 0x100000000) LEAVE_FF(fs, FR_DENIED);
#endif
	n = (DWORD)fs->csize * SS(fs);	/* Taille cluster */
	tcl = (DWORD)(fsz / n) + ((fsz & (n - 1)) ? 1 : 0);
	stcl = fs->last_clst; lclst = 0;
	if (stcl < 2 || stcl >= fs->n_fatent) stcl = 2;

#if _FS_EXFAT
	if (fs->fs_type == FS_EXFAT) {
		scl = find_bitmap(fs, stcl, tcl);
		if (scl == 0) res = FR_DENIED;				/* Aucun bloc cluster contigu trouvé */
		if (scl == 0xFFFFFFFF) res = FR_DISK_ERR;
		if (res == FR_OK) {
			if (opt) {
				res = change_bitmap(fs, scl, tcl, 1);	/* Bloc cluster marqué occupé */
				lclst = scl + tcl - 1;
			} else {		/* Début suggéré pour la prochaine allocation */
				lclst = scl - 1;
			}
		}
	} else
#endif
	{
		scl = clst = stcl; ncl = 0;
		for (;;) {
			n = get_fat(&fp->obj, clst);
			if (++clst >= fs->n_fatent) clst = 2;
			if (n == 1) { res = FR_INT_ERR; break; }
			if (n == 0xFFFFFFFF) { res = FR_DISK_ERR; break; }
			if (n == 0) {
				if (++ncl == tcl) break;
			} else {
				scl = clst; ncl = 0;		/* Cluster occupé */
			}
			if (clst == stcl) { res = FR_DENIED; break; }
		}
		if (res == FR_OK) {
			if (opt) {
				for (clst = scl, n = tcl; n; clst++, n--) {
					res = put_fat(fs, clst, (n == 1) ? 0xFFFFFFFF : clst + 1);
					if (res != FR_OK) break;
					lclst = clst;
				}
			} else {		/* Début suggéré pour la prochaine allocation */
				lclst = scl - 1;
			}
		}
	}

	if (res == FR_OK) {
		fs->last_clst = lclst;		/* Début suggéré pour la prochaine allocation */
		if (opt) {
			fp->obj.sclust = scl;
			fp->obj.objsize = fsz;
			if (_FS_EXFAT) fp->obj.stat = 2;	/* État chaîne contiguë */
			fp->flag |= FA_MODIFIED;
			if (fs->free_clst <= fs->n_fatent - 2) {
				fs->free_clst -= tcl;
				fs->fsi_flag |= 1;
			}
		}
	}

	LEAVE_FF(fs, res);
}

#endif

#if _USE_FORWARD

FRESULT f_forward (
	FIL* fp, 						/* Pointeur objet fichier */
	UINT (*func)(const BYTE*,UINT),
	UINT btf,						/* Nombre de bytes à transférer */
	UINT* bf						/* Nombre de bytes transférés en sortie */
)
{
	FRESULT res;
	FATFS *fs;
	DWORD clst, sect;
	FSIZE_t remain;
	UINT rcnt, csect;
	BYTE *dbuf;

	*bf = 0;
	res = validate(&fp->obj, &fs);		/* Validation objet fichier */
	if (res != FR_OK || (res = (FRESULT)fp->err) != FR_OK) LEAVE_FF(fs, res);
	if (!(fp->flag & FA_READ)) LEAVE_FF(fs, FR_DENIED);

	remain = fp->obj.objsize - fp->fptr;
	if (btf > remain) btf = (UINT)remain;			/* Btf limité aux bytes restants */

	for ( ;  btf && (*func)(0, 0);					/* Transfert jusqu’à fin des données ou stream occupé */
		fp->fptr += rcnt, *bf += rcnt, btf -= rcnt) {
		csect = (UINT)(fp->fptr / SS(fs) & (fs->csize - 1));	/* Offset secteur dans cluster */
		if (fp->fptr % SS(fs) == 0) {
			if (csect == 0) {
				clst = (fp->fptr == 0) ?
					fp->obj.sclust : get_fat(&fp->obj, fp->clust);
				if (clst <= 1) ABORT(fs, FR_INT_ERR);
				if (clst == 0xFFFFFFFF) ABORT(fs, FR_DISK_ERR);
				fp->clust = clst;
			}
		}
		sect = clust2sect(fs, fp->clust);
		if (!sect) ABORT(fs, FR_INT_ERR);
		sect += csect;
#if _FS_TINY
		if (move_window(fs, sect) != FR_OK) ABORT(fs, FR_DISK_ERR);
		dbuf = fs->win;
#else
		if (fp->sect != sect) {
#if !_FS_READONLY
			if (fp->flag & FA_DIRTY) {		/* Writeback du cache secteur dirty */
				if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
				fp->flag &= (BYTE)~FA_DIRTY;
			}
#endif
			if (disk_read(fs->drv, fp->buf, sect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
		}
		dbuf = fp->buf;
#endif
		fp->sect = sect;
		rcnt = SS(fs) - (UINT)fp->fptr % SS(fs);	/* Bytes restants dans secteur */
		if (rcnt > btf) rcnt = btf;					/* Limitation à btr si nécessaire */
		rcnt = (*func)(dbuf + ((UINT)fp->fptr % SS(fs)), rcnt);
		if (!rcnt) ABORT(fs, FR_INT_ERR);
	}

	LEAVE_FF(fs, FR_OK);
}
#endif

#if _USE_MKFS && !_FS_READONLY

FRESULT f_mkfs (
	const TCHAR* path,
	BYTE opt,
	DWORD au,			/* Taille unité allocation en bytes */
	void* work,
	UINT len			/* Taille buffer de travail */
)
{
	const UINT n_fats = 1;		/* Nombre de FAT : 1 ou 2 en FAT12/16/32 */
	const UINT n_rootdir = 512;	/* Nombre entrées racine FAT12/16 */
	static const WORD cst[] = {1, 4, 16, 64, 256, 512, 0};	/* Limite taille cluster FAT12/16 en unités de 4 KiB */
	static const WORD cst32[] = {1, 2, 4, 8, 16, 32, 0};	/* Limite taille cluster FAT32 en unités de 128 KiB */
	BYTE fmt, sys, *buf, *pte, pdrv, part;
	WORD ss;
	DWORD szb_buf, sz_buf, sz_blk, n_clst, pau, sect, nsect, n;
	DWORD b_vol, b_fat, b_data;				/* LBA de base volume/FAT/données */
	DWORD sz_vol, sz_rsv, sz_fat, sz_dir;	/* Taille volume/FAT/répertoire/données */
	UINT i;
	int vol;
	DSTATUS stat;
#if _USE_TRIM || _FS_EXFAT
	DWORD tbl[3];
#endif

	vol = get_ldnumber(&path);
	if (vol < 0) return FR_INVALID_DRIVE;
	if (FatFs[vol]) FatFs[vol]->fs_type = 0;
	pdrv = LD2PD(vol);
	part = LD2PT(vol);	/* Partition : 0 nouvelle, 1..4 depuis table */

	stat = disk_initialize(pdrv);
	if (stat & STA_NOINIT) return FR_NOT_READY;
	if (stat & STA_PROTECT) return FR_WRITE_PROTECTED;
	if (disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_blk) != RES_OK || !sz_blk || sz_blk > 32768 || (sz_blk & (sz_blk - 1))) sz_blk = 1;	/* Alignement données sur erase block */
#if _MAX_SS != _MIN_SS		/* Taille support uniquement si secteurs variables */
	if (disk_ioctl(pdrv, GET_SECTOR_SIZE, &ss) != RES_OK) return FR_DISK_ERR;
	if (ss > _MAX_SS || ss < _MIN_SS || (ss & (ss - 1))) return FR_DISK_ERR;
#else
	ss = _MAX_SS;
#endif
	if ((au != 0 && au < ss) || au > 0x1000000 || (au & (au - 1))) return FR_INVALID_PARAMETER;
	au /= ss;	/* Taille cluster en secteurs */

	buf = (BYTE*)work;
	sz_buf = len / ss;		/* Taille buffer de travail en secteurs */
	szb_buf = sz_buf * ss;	/* Taille buffer de travail en bytes */
	if (!szb_buf) return FR_MKFS_ABORTED;

	if (_MULTI_PARTITION && part != 0) {

		if (disk_read(pdrv, buf, 0, 1) != RES_OK) return FR_DISK_ERR;
		if (ld_word(buf + BS_55AA) != 0xAA55) return FR_MKFS_ABORTED;
		pte = buf + (MBR_Table + (part - 1) * SZ_PTE);
		if (!pte[PTE_System]) return FR_MKFS_ABORTED;	/* Aucune partition */
		b_vol = ld_dword(pte + PTE_StLba);
		sz_vol = ld_dword(pte + PTE_SizLba);
	} else {
		/* Création d’une partition unique */
		if (disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_vol) != RES_OK) return FR_DISK_ERR;
		b_vol = (opt & FM_SFD) ? 0 : 63;		/* Secteur début volume */
		if (sz_vol < b_vol) return FR_MKFS_ABORTED;
		sz_vol -= b_vol;						/* Taille volume */
	}
	if (sz_vol < 128) return FR_MKFS_ABORTED;	/* Volume au moins égal à 128 secteurs */

	do {
		if (_FS_EXFAT && (opt & FM_EXFAT)) {
			if ((opt & FM_ANY) == FM_EXFAT || sz_vol >= 0x4000000 || au > 128) {	/* exFAT seul, volume >=64 Mi secteurs ou cluster >128 secteurs */
				fmt = FS_EXFAT; break;
			}
		}
		if (au > 128) return FR_INVALID_PARAMETER;	/* Unité allocation trop grande pour FAT/FAT32 */
		if (opt & FM_FAT32) {
			if ((opt & FM_ANY) == FM_FAT32 || !(opt & FM_FAT)) {	/* FAT32 uniquement ou sans FAT */
				fmt = FS_FAT32; break;
			}
		}
		if (!(opt & FM_FAT)) return FR_INVALID_PARAMETER;
		fmt = FS_FAT16;
	} while (0);

#if _FS_EXFAT
	if (fmt == FS_EXFAT) {
		DWORD szb_bit, szb_case, sum, nb, cl;
		WCHAR ch, si;
		UINT j, st;
		BYTE b;

		if (sz_vol < 0x1000) return FR_MKFS_ABORTED;	/* Volume trop petit */
#if _USE_TRIM
		tbl[0] = b_vol; tbl[1] = b_vol + sz_vol - 1;	/* Notification device : volume effaçable */
		disk_ioctl(pdrv, CTRL_TRIM, tbl);
#endif

		if (!au) {	/* Sélection automatique de au */
			au = 8;
			if (sz_vol >= 0x80000) au = 64;		/* Au moins 512 Ki secteurs */
			if (sz_vol >= 0x4000000) au = 256;	/* Au moins 64 Mi secteurs */
		}
		b_fat = b_vol + 32;										/* Début FAT à l’offset 32 */
		sz_fat = ((sz_vol / au + 2) * 4 + ss - 1) / ss;			/* Nombre secteurs FAT */
		b_data = (b_fat + sz_fat + sz_blk - 1) & ~(sz_blk - 1);	/* Données alignées sur erase block */
		if (b_data >= sz_vol / 2) return FR_MKFS_ABORTED;		/* Volume trop petit */
		n_clst = (sz_vol - (b_data - b_vol)) / au;				/* Nombre de clusters */
		if (n_clst <16) return FR_MKFS_ABORTED;					/* Trop peu de clusters */
		if (n_clst > MAX_EXFAT) return FR_MKFS_ABORTED;			/* Trop de clusters */

		szb_bit = (n_clst + 7) / 8;						/* Taille allocation bitmap */
		tbl[0] = (szb_bit + au * ss - 1) / (au * ss);	/* Nombre de clusters bitmap */

		/* Création table majuscules compressée */
		sect = b_data + au * tbl[0];	/* Secteur début table */
		sum = 0;						/* Checksum table stocké dans entrée 82 */
		st = si = i = j = szb_case = 0;
		do {
			switch (st) {
			case 0:
				ch = ff_wtoupper(si);
				if (ch != si) {
					si++; break;
				}
				for (j = 1; (WCHAR)(si + j) && (WCHAR)(si + j) == ff_wtoupper((WCHAR)(si + j)); j++) ;
				if (j >= 128) {
					ch = 0xFFFF; st = 2; break;	/* Bloc sans changement de casse compressé si longueur >=128 */
				}
				st = 1;			/* Séquences courtes non compressées */
				__attribute__ ((fallthrough));
			case 1:
				ch = si++;
				if (--j == 0) st = 0;
				break;

			default:
				ch = (WCHAR)j; si += j;	/* Nombre caractères à ignorer */
				st = 0;
			}
			sum = xsum32(buf[i + 0] = (BYTE)ch, sum);
			sum = xsum32(buf[i + 1] = (BYTE)(ch >> 8), sum);
			i += 2; szb_case += 2;
			if (!si || i == szb_buf) {		/* Écriture si buffer plein ou fin de traitement */
				n = (i + ss - 1) / ss;
				if (disk_write(pdrv, buf, sect, n) != RES_OK) return FR_DISK_ERR;
				sect += n; i = 0;
			}
		} while (si);
		tbl[1] = (szb_case + au * ss - 1) / (au * ss);	/* Nombre clusters table majuscules */
		tbl[2] = 1;										/* Nombre clusters répertoire racine */

		sect = b_data; nsect = (szb_bit + ss - 1) / ss;	/* Début bitmap et nombre secteurs */
		nb = tbl[0] + tbl[1] + tbl[2];					/* Clusters utilisés par le système */
		do {
			mem_set(buf, 0, szb_buf);
			for (i = 0; nb >= 8 && i < szb_buf; buf[i++] = 0xFF, nb -= 8) ;
			for (b = 1; nb && i < szb_buf; buf[i] |= b, b <<= 1, nb--) ;
			n = (nsect > sz_buf) ? sz_buf : nsect;
			if (disk_write(pdrv, buf, sect, n) != RES_OK) return FR_DISK_ERR;
			sect += n; nsect -= n;
		} while (nsect);

		sect = b_fat; nsect = sz_fat;	/* Début FAT et nombre secteurs FAT */
		j = nb = cl = 0;
		do {
			mem_set(buf, 0, szb_buf); i = 0;
			if (cl == 0) {	/* Initialisation entrées 0 et 1 */
				st_dword(buf + i, 0xFFFFFFF8); i += 4; cl++;
				st_dword(buf + i, 0xFFFFFFFF); i += 4; cl++;
			}
			do {			/* Chaînes bitmap/table majuscules/racine */
				while (nb && i < szb_buf) {
					st_dword(buf + i, (nb > 1) ? cl + 1 : 0xFFFFFFFF);
					i += 4; cl++; nb--;
				}
				if (!nb && j < 3) nb = tbl[j++];
			} while (nb && i < szb_buf);
			n = (nsect > sz_buf) ? sz_buf : nsect;
			if (disk_write(pdrv, buf, sect, n) != RES_OK) return FR_DISK_ERR;
			sect += n; nsect -= n;
		} while (nsect);

		mem_set(buf, 0, szb_buf);
		buf[SZDIRE * 0 + 0] = 0x83;		/* Entrée 83 : label volume */
		buf[SZDIRE * 1 + 0] = 0x81;		/* Entrée 81 : allocation bitmap */
		st_dword(buf + SZDIRE * 1 + 20, 2);
		st_dword(buf + SZDIRE * 1 + 24, szb_bit);
		buf[SZDIRE * 2 + 0] = 0x82;		/* Entrée 82 : table majuscules */
		st_dword(buf + SZDIRE * 2 + 4, sum);
		st_dword(buf + SZDIRE * 2 + 20, 2 + tbl[0]);
		st_dword(buf + SZDIRE * 2 + 24, szb_case);
		sect = b_data + au * (tbl[0] + tbl[1]);	nsect = au;	/* Début racine et nombre secteurs */
		do {
			n = (nsect > sz_buf) ? sz_buf : nsect;
			if (disk_write(pdrv, buf, sect, n) != RES_OK) return FR_DISK_ERR;
			mem_set(buf, 0, ss);
			sect += n; nsect -= n;
		} while (nsect);

		/* Création des deux ensembles de VBR exFAT */
		sect = b_vol;
		for (n = 0; n < 2; n++) {
			/* Enregistrement principal à +0 */
			mem_set(buf, 0, ss);
			mem_cpy(buf + BS_JmpBoot, "\xEB\x76\x90" "EXFAT   ", 11);	/* Jump boot x86 et nom OEM */
			st_dword(buf + BPB_VolOfsEx, b_vol);					/* Offset volume dans lecteur physique, en secteurs */
			st_dword(buf + BPB_TotSecEx, sz_vol);					/* Taille volume en secteurs */
			st_dword(buf + BPB_FatOfsEx, b_fat - b_vol);			/* Offset FAT en secteurs */
			st_dword(buf + BPB_FatSzEx, sz_fat);					/* Taille FAT en secteurs */
			st_dword(buf + BPB_DataOfsEx, b_data - b_vol);			/* Offset données en secteurs */
			st_dword(buf + BPB_NumClusEx, n_clst);					/* Nombre de clusters */
			st_dword(buf + BPB_RootClusEx, 2 + tbl[0] + tbl[1]);	/* Cluster répertoire racine */
			st_dword(buf + BPB_VolIDEx, GET_FATTIME());				/* VSN */
			st_word(buf + BPB_FSVerEx, 0x100);						/* Version filesystem 1.00 */
			for (buf[BPB_BytsPerSecEx] = 0, i = ss; i >>= 1; buf[BPB_BytsPerSecEx]++) ;	/* Log2 taille secteur en bytes */
			for (buf[BPB_SecPerClusEx] = 0, i = au; i >>= 1; buf[BPB_SecPerClusEx]++) ;	/* Log2 taille cluster en secteurs */
			buf[BPB_NumFATsEx] = 1;					/* Nombre de FAT */
			buf[BPB_DrvNumEx] = 0x80;				/* Numéro lecteur pour int13 */
			st_word(buf + BS_BootCodeEx, 0xFEEB);	/* Code boot x86 */
			st_word(buf + BS_55AA, 0xAA55);			/* Offset signature fixe, quelle que soit taille secteur */
			for (i = sum = 0; i < ss; i++) {		/* Checksum VBR */
				if (i != BPB_VolFlagEx && i != BPB_VolFlagEx + 1 && i != BPB_PercInUseEx) sum = xsum32(buf[i], sum);
			}
			if (disk_write(pdrv, buf, sect++, 1) != RES_OK) return FR_DISK_ERR;
			/* Bootstraps étendus à +1..+8 */
			mem_set(buf, 0, ss);
			st_word(buf + ss - 2, 0xAA55);	/* Signature à la fin du secteur */
			for (j = 1; j < 9; j++) {
				for (i = 0; i < ss; sum = xsum32(buf[i++], sum)) ;	/* Checksum VBR */
				if (disk_write(pdrv, buf, sect++, 1) != RES_OK) return FR_DISK_ERR;
			}
			/* Enregistrements OEM/réservés à +9..+10 */
			mem_set(buf, 0, ss);
			for ( ; j < 11; j++) {
				for (i = 0; i < ss; sum = xsum32(buf[i++], sum)) ;	/* Checksum VBR */
				if (disk_write(pdrv, buf, sect++, 1) != RES_OK) return FR_DISK_ERR;
			}
			/* Enregistrement checksum à +11 */
			for (i = 0; i < ss; i += 4) st_dword(buf + i, sum);		/* Remplissage par valeur checksum */
			if (disk_write(pdrv, buf, sect++, 1) != RES_OK) return FR_DISK_ERR;
		}

	} else
#endif
	{
		do {
			pau = au;

			if (fmt == FS_FAT32) {	/* Volume FAT32 */
				if (!pau) {	/* Sélection automatique de au */
					n = sz_vol / 0x20000;	/* Taille volume en unités de 128 Ki secteurs */
					for (i = 0, pau = 1; cst32[i] && cst32[i] <= n; i++, pau <<= 1) ;
				}
				n_clst = sz_vol / pau;	/* Nombre de clusters */
				sz_fat = (n_clst * 4 + 8 + ss - 1) / ss;	/* Taille FAT en secteurs */
				sz_rsv = 32;	/* Nombre secteurs réservés */
				sz_dir = 0;		/* Aucun répertoire statique */
				if (n_clst <= MAX_FAT16 || n_clst > MAX_FAT32) return FR_MKFS_ABORTED;
			} else {				/* Volume FAT12/16 */
				if (!pau) {	/* Sélection automatique de au */
					n = sz_vol / 0x1000;	/* Taille volume en unités de 4 Ki secteurs */
					for (i = 0, pau = 1; cst[i] && cst[i] <= n; i++, pau <<= 1) ;
				}
				n_clst = sz_vol / pau;
				if (n_clst > MAX_FAT12) {
					n = n_clst * 2 + 4;		/* Taille FAT en bytes */
				} else {
					fmt = FS_FAT12;
					n = (n_clst * 3 + 1) / 2 + 3;	/* Taille FAT en bytes */
				}
				sz_fat = (n + ss - 1) / ss;		/* Taille FAT en secteurs */
				sz_rsv = 1;						/* Nombre secteurs réservés */
				sz_dir = (DWORD)n_rootdir * SZDIRE / ss;	/* Taille répertoire racine en secteurs */
			}
			b_fat = b_vol + sz_rsv;						/* Base FAT */
			b_data = b_fat + sz_fat * n_fats + sz_dir;	/* Base données */

			/* Media Flash : base données alignée sur erase block */
			n = ((b_data + sz_blk - 1) & ~(sz_blk - 1)) - b_data;	/* Erase block suivant le plus proche */
			if (fmt == FS_FAT32) {		/* FAT32 : déplacement base FAT */
				sz_rsv += n; b_fat += n;
			} else {					/* FAT12/16 : extension taille FAT */
				sz_fat += n / n_fats;
			}

			if (sz_vol < b_data + pau * 16 - b_vol) return FR_MKFS_ABORTED;	/* Volume trop petit */
			n_clst = (sz_vol - sz_rsv - sz_fat * n_fats - sz_dir) / pau;
			if (fmt == FS_FAT32) {
				if (n_clst <= MAX_FAT16) {	/* Trop peu de clusters pour FAT32 */
					if (!au && (au = pau / 2) != 0) continue;	/* Taille cluster ajustée puis retry */
					return FR_MKFS_ABORTED;
				}
			}
			if (fmt == FS_FAT16) {
				if (n_clst > MAX_FAT16) {	/* Trop de clusters pour FAT16 */
					if (!au && (pau * 2) <= 64) {
						au = pau * 2; continue;		/* Taille cluster ajustée puis retry */
					}
					if ((opt & FM_FAT32)) {
						fmt = FS_FAT32; continue;	/* Passage au type FAT32 puis retry */
					}
					if (!au && (au = pau * 2) <= 128) continue;	/* Taille cluster ajustée puis retry */
					return FR_MKFS_ABORTED;
				}
				if  (n_clst <= MAX_FAT12) {	/* Trop peu de clusters pour FAT16 */
					if (!au && (au = pau * 2) <= 128) continue;	/* Taille cluster ajustée puis retry */
					return FR_MKFS_ABORTED;
				}
			}
			if (fmt == FS_FAT12 && n_clst > MAX_FAT12) return FR_MKFS_ABORTED;	/* Trop de clusters pour FAT12 */

			break;
		} while (1);

#if _USE_TRIM
		tbl[0] = b_vol; tbl[1] = b_vol + sz_vol - 1;	/* Notification device : volume effaçable */
		disk_ioctl(pdrv, CTRL_TRIM, tbl);
#endif

		mem_set(buf, 0, ss);
		mem_cpy(buf + BS_JmpBoot, "\xEB\xFE\x90" "MSDOS5.0", 11);/* Jump boot x86 et nom OEM */
		st_word(buf + BPB_BytsPerSec, ss);				/* Taille secteur en bytes */
		buf[BPB_SecPerClus] = (BYTE)pau;				/* Taille cluster en secteurs */
		st_word(buf + BPB_RsvdSecCnt, (WORD)sz_rsv);	/* Taille zone réservée */
		buf[BPB_NumFATs] = (BYTE)n_fats;				/* Nombre de FAT */
		st_word(buf + BPB_RootEntCnt, (WORD)((fmt == FS_FAT32) ? 0 : n_rootdir));	/* Nombre entrées répertoire racine */
		if (sz_vol < 0x10000) {
			st_word(buf + BPB_TotSec16, (WORD)sz_vol);	/* Taille volume en LBA 16 bits */
		} else {
			st_dword(buf + BPB_TotSec32, sz_vol);		/* Taille volume en LBA 32 bits */
		}
		buf[BPB_Media] = 0xF8;							/* Byte descripteur media */
		st_word(buf + BPB_SecPerTrk, 63);				/* Nombre secteurs par piste pour int13 */
		st_word(buf + BPB_NumHeads, 255);				/* Nombre têtes pour int13 */
		st_dword(buf + BPB_HiddSec, b_vol);				/* Offset volume dans lecteur physique, en secteurs */
		if (fmt == FS_FAT32) {
			st_dword(buf + BS_VolID32, GET_FATTIME());	/* VSN */
			st_dword(buf + BPB_FATSz32, sz_fat);		/* Taille FAT en secteurs */
			st_dword(buf + BPB_RootClus32, 2);			/* Cluster racine numéro 2 */
			st_word(buf + BPB_FSInfo32, 1);				/* Offset FSINFO : VBR+1 */
			st_word(buf + BPB_BkBootSec32, 6);			/* VBR de secours à VBR+6 */
			buf[BS_DrvNum32] = 0x80;					/* Numéro lecteur pour int13 */
			buf[BS_BootSig32] = 0x29;					/* Signature boot étendue */
			mem_cpy(buf + BS_VolLab32, "NO NAME    " "FAT32   ", 19);	/* Label volume et signature FAT */
		} else {
			st_dword(buf + BS_VolID, GET_FATTIME());	/* VSN */
			st_word(buf + BPB_FATSz16, (WORD)sz_fat);	/* Taille FAT en secteurs */
			buf[BS_DrvNum] = 0x80;						/* Numéro lecteur pour int13 */
			buf[BS_BootSig] = 0x29;						/* Signature boot étendue */
			mem_cpy(buf + BS_VolLab, "NO NAME    " "FAT     ", 19);	/* Label volume et signature FAT */
		}
		st_word(buf + BS_55AA, 0xAA55);					/* Offset signature fixe, quelle que soit taille secteur */
		if (disk_write(pdrv, buf, b_vol, 1) != RES_OK) return FR_DISK_ERR;

		if (fmt == FS_FAT32) {
			disk_write(pdrv, buf, b_vol + 6, 1);		/* Écriture VBR de secours à VBR+6 */
			mem_set(buf, 0, ss);
			st_dword(buf + FSI_LeadSig, 0x41615252);
			st_dword(buf + FSI_StrucSig, 0x61417272);
			st_dword(buf + FSI_Free_Count, n_clst - 1);
			st_dword(buf + FSI_Nxt_Free, 2);			/* Dernier cluster alloué */
			st_word(buf + BS_55AA, 0xAA55);
			disk_write(pdrv, buf, b_vol + 7, 1);		/* Écriture FSINFO de secours à VBR+7 */
			disk_write(pdrv, buf, b_vol + 1, 1);		/* Écriture FSINFO original à VBR+1 */
		}

		mem_set(buf, 0, (UINT)szb_buf);
		sect = b_fat;		/* Secteur début FAT */
		for (i = 0; i < n_fats; i++) {
			if (fmt == FS_FAT32) {
				st_dword(buf + 0, 0xFFFFFFF8);
				st_dword(buf + 4, 0xFFFFFFFF);
				st_dword(buf + 8, 0x0FFFFFFF);	/* Entrée 2 : répertoire racine */
			} else {
				st_dword(buf + 0, (fmt == FS_FAT12) ? 0xFFFFF8 : 0xFFFFFFF8);
			}
			nsect = sz_fat;		/* Nombre secteurs FAT */
			do {
				n = (nsect > sz_buf) ? sz_buf : nsect;
				if (disk_write(pdrv, buf, sect, (UINT)n) != RES_OK) return FR_DISK_ERR;
				mem_set(buf, 0, ss);
				sect += n; nsect -= n;
			} while (nsect);
		}

		/* Répertoire racine initialisé à zéro */
		nsect = (fmt == FS_FAT32) ? pau : sz_dir;	/* Nombre secteurs répertoire racine */
		do {
			n = (nsect > sz_buf) ? sz_buf : nsect;
			if (disk_write(pdrv, buf, sect, (UINT)n) != RES_OK) return FR_DISK_ERR;
			sect += n; nsect -= n;
		} while (nsect);
	}

	if (_FS_EXFAT && fmt == FS_EXFAT) {
		sys = 0x07;			/* HPFS/NTFS/exFAT */
	} else {
		if (fmt == FS_FAT32) {
			sys = 0x0C;		/* FAT32X */
		} else {
			if (sz_vol >= 0x10000) {
				sys = 0x06;	/* FAT12/16 : au moins 64 Ki secteurs */
			} else {
				sys = (fmt == FS_FAT16) ? 0x04 : 0x01;	/* FAT16 ou FAT12 : moins de 64 Ki secteurs */
			}
		}
	}

	if (_MULTI_PARTITION && part != 0) {	/* Création dans la partition existante */

		if (disk_read(pdrv, buf, 0, 1) != RES_OK) return FR_DISK_ERR;
		buf[MBR_Table + (part - 1) * SZ_PTE + PTE_System] = sys;
		if (disk_write(pdrv, buf, 0, 1) != RES_OK) return FR_DISK_ERR;
	} else {								/* Création d’une nouvelle partition unique */
		if (!(opt & FM_SFD)) {	/* Table partitions créée en format FDISK */
			mem_set(buf, 0, ss);
			st_word(buf + BS_55AA, 0xAA55);		/* Signature MBR */
			pte = buf + MBR_Table;
			pte[PTE_Boot] = 0;					/* Indicateur boot */
			pte[PTE_StHead] = 1;				/* Tête début */
			pte[PTE_StSec] = 1;					/* Secteur début */
			pte[PTE_StCyl] = 0;					/* Cylindre début */
			pte[PTE_System] = sys;				/* Type système */
			n = (b_vol + sz_vol) / (63 * 255);	/* CHS final potentiellement invalide */
			pte[PTE_EdHead] = 254;				/* Tête fin */
			pte[PTE_EdSec] = (BYTE)(n >> 2 | 63);	/* Secteur fin */
			pte[PTE_EdCyl] = (BYTE)n;			/* Cylindre fin */
			st_dword(pte + PTE_StLba, b_vol);	/* Offset initial en LBA */
			st_dword(pte + PTE_SizLba, sz_vol);	/* Taille en secteurs */
			if (disk_write(pdrv, buf, 0, 1) != RES_OK) return FR_DISK_ERR;
		}
	}

	if (disk_ioctl(pdrv, CTRL_SYNC, 0) != RES_OK) return FR_DISK_ERR;

	return FR_OK;
}

#if _MULTI_PARTITION

FRESULT f_fdisk (
	BYTE pdrv,			/* Numéro du lecteur physique */
	const DWORD* szt,	/* Pointer table des tailles de partitions */
	void* work
)
{
	UINT i, n, sz_cyl, tot_cyl, b_cyl, e_cyl, p_cyl;
	BYTE s_hd, e_hd, *p, *buf = (BYTE*)work;
	DSTATUS stat;
	DWORD sz_disk, sz_part, s_part;

	stat = disk_initialize(pdrv);
	if (stat & STA_NOINIT) return FR_NOT_READY;
	if (stat & STA_PROTECT) return FR_WRITE_PROTECTED;
	if (disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_disk)) return FR_DISK_ERR;

	/* CHS calculé indépendamment de la géométrie lecteur */
	for (n = 16; n < 256 && sz_disk / n / 63 > 1024; n *= 2) ;
	if (n == 256) n--;
	e_hd = n - 1;
	sz_cyl = 63 * n;
	tot_cyl = sz_disk / sz_cyl;

	mem_set(buf, 0, _MAX_SS);
	p = buf + MBR_Table; b_cyl = 0;
	for (i = 0; i < 4; i++, p += SZ_PTE) {
		p_cyl = (szt[i] <= 100U) ? (DWORD)tot_cyl * szt[i] / 100 : szt[i] / sz_cyl;	/* Nombre cylindres */
		if (!p_cyl) continue;
		s_part = (DWORD)sz_cyl * b_cyl;
		sz_part = (DWORD)sz_cyl * p_cyl;
		if (i == 0) {	/* Première piste du cylindre 0 exclue */
			s_hd = 1;
			s_part += 63; sz_part -= 63;
		} else {
			s_hd = 0;
		}
		e_cyl = b_cyl + p_cyl - 1;	/* Cylindre fin */
		if (e_cyl >= tot_cyl) return FR_INVALID_PARAMETER;

		p[1] = s_hd;						/* Tête début */
		p[2] = (BYTE)((b_cyl >> 2) + 1);	/* Secteur début */
		p[3] = (BYTE)b_cyl;					/* Cylindre début */
		p[4] = 0x07;						/* Type système provisoire */
		p[5] = e_hd;						/* Tête fin */
		p[6] = (BYTE)((e_cyl >> 2) + 63);	/* Secteur fin */
		p[7] = (BYTE)e_cyl;					/* Cylindre fin */
		st_dword(p + 8, s_part);			/* Secteur début en LBA */
		st_dword(p + 12, sz_part);			/* Nombre de secteurs */

		b_cyl += p_cyl;
	}
	st_word(p, 0xAA55);

	return (disk_write(pdrv, buf, 0, 1) != RES_OK || disk_ioctl(pdrv, CTRL_SYNC, 0) != RES_OK) ? FR_DISK_ERR : FR_OK;
}

#endif
#endif

#if _USE_STRFUNC

TCHAR* f_gets (
	TCHAR* buff,
	int len,		/* Taille buffer string en caractères */
	FIL* fp			/* Pointeur objet fichier */
)
{
	int n = 0;
	TCHAR c, *p = buff;
	BYTE s[2];
	UINT rc;

	while (n < len - 1) {
#if _LFN_UNICODE
#if _STRF_ENCODE == 3		/* Lecture caractère UTF-8 */
		f_read(fp, s, 1, &rc);
		if (rc != 1) break;
		c = s[0];
		if (c >= 0x80) {
			if (c < 0xC0) continue;	/* Ignorer continuation UTF-8 isolée */
			if (c < 0xE0) {			/* Séquence 2 bytes : 0x80..0x7FF */
				f_read(fp, s, 1, &rc);
				if (rc != 1) break;
				c = (c & 0x1F) << 6 | (s[0] & 0x3F);
				if (c < 0x80) c = '?';	/* Plage de code invalide refusée */
			} else {
				if (c < 0xF0) {		/* Séquence 3 bytes : 0x800..0xFFFF */
					f_read(fp, s, 2, &rc);
					if (rc != 2) break;
					c = c << 12 | (s[0] & 0x3F) << 6 | (s[1] & 0x3F);
					if (c < 0x800) c = '?';	/* Plage de code invalide refusée */
				} else {			/* Séquence UTF-8 de 4 bytes refusée */
					c = '?';
				}
			}
		}
#elif _STRF_ENCODE == 2		/* Lecture caractère UTF-16BE */
		f_read(fp, s, 2, &rc);
		if (rc != 2) break;
		c = s[1] + (s[0] << 8);
#elif _STRF_ENCODE == 1		/* Lecture caractère UTF-16LE */
		f_read(fp, s, 2, &rc);
		if (rc != 2) break;
		c = s[0] + (s[1] << 8);
#else						/* Lecture caractère ANSI/OEM */
		f_read(fp, s, 1, &rc);
		if (rc != 1) break;
		c = s[0];
		if (IsDBCS1(c)) {
			f_read(fp, s, 1, &rc);
			if (rc != 1) break;
			c = (c << 8) + s[0];
		}
		c = ff_convert(c, 1);	/* OEM vers Unicode */
		if (!c) c = '?';
#endif
#else						/* Lecture caractère sans conversion */
		f_read(fp, s, 1, &rc);
		if (rc != 1) break;
		c = s[0];
#endif
		if (_USE_STRFUNC == 2 && c == '\r') continue;	/* Suppression CR */
		*p++ = c;
		n++;
		if (c == '\n') break;
	}
	*p = 0;
	return n ? buff : 0;			/* Aucune donnée lue, EOF ou erreur : retour erreur */
}

#if !_FS_READONLY
#include <stdarg.h>

typedef struct {
	FIL *fp;
	int idx, nchr;	/* Index write buf[] ; -1 erreur ; nombre caractères écrits */
	BYTE buf[64];
} putbuff;

static
void putc_bfd (		/* Écriture bufferisée avec conversion code */
	putbuff* pb,
	TCHAR c
)
{
	UINT bw;
	int i;

	if (_USE_STRFUNC == 2 && c == '\n') {	 /* Conversion LF vers CRLF */
		putc_bfd(pb, '\r');
	}

	i = pb->idx;
	if (i < 0) return;

#if _LFN_UNICODE
#if _STRF_ENCODE == 3			/* Écriture caractère UTF-8 */
	if (c < 0x80) {				/* 7 bits */
		pb->buf[i++] = (BYTE)c;
	} else {
		if (c < 0x800) {		/* 11 bits */
			pb->buf[i++] = (BYTE)(0xC0 | c >> 6);
		} else {				/* 16 bits */
			pb->buf[i++] = (BYTE)(0xE0 | c >> 12);
			pb->buf[i++] = (BYTE)(0x80 | (c >> 6 & 0x3F));
		}
		pb->buf[i++] = (BYTE)(0x80 | (c & 0x3F));
	}
#elif _STRF_ENCODE == 2			/* Écriture caractère UTF-16BE */
	pb->buf[i++] = (BYTE)(c >> 8);
	pb->buf[i++] = (BYTE)c;
#elif _STRF_ENCODE == 1			/* Écriture caractère UTF-16LE */
	pb->buf[i++] = (BYTE)c;
	pb->buf[i++] = (BYTE)(c >> 8);
#else							/* Écriture caractère ANSI/OEM */
	c = ff_convert(c, 0);	/* Unicode vers OEM */
	if (!c) c = '?';
	if (c >= 0x100)
		pb->buf[i++] = (BYTE)(c >> 8);
	pb->buf[i++] = (BYTE)c;
#endif
#else							/* Écriture caractère sans conversion */
	pb->buf[i++] = (BYTE)c;
#endif

	if (i >= (int)(sizeof pb->buf) - 3) {
		f_write(pb->fp, pb->buf, (UINT)i, &bw);
		i = (bw == (UINT)i) ? 0 : -1;
	}
	pb->idx = i;
	pb->nchr++;
}

static
int putc_flush (
	putbuff* pb
)
{
	UINT nw;

	if (   pb->idx >= 0
		&& f_write(pb->fp, pb->buf, (UINT)pb->idx, &nw) == FR_OK
		&& (UINT)pb->idx == nw) return pb->nchr;
	return EOF;
}

static
void putc_init (
	putbuff* pb,
	FIL* fp
)
{
	pb->fp = fp;
	pb->nchr = pb->idx = 0;
}

int f_putc (
	TCHAR c,
	FIL* fp		/* Pointeur objet fichier */
)
{
	putbuff pb;

	putc_init(&pb, fp);
	putc_bfd(&pb, c);
	return putc_flush(&pb);
}

int f_puts (
	const TCHAR* str,
	FIL* fp				/* Pointeur objet fichier */
)
{
	putbuff pb;

	putc_init(&pb, fp);
	while (*str) putc_bfd(&pb, *str++);
	return putc_flush(&pb);
}

int f_printf (
	FIL* fp,			/* Pointeur objet fichier */
	const TCHAR* fmt,
	...
)
{
	va_list arp;
	putbuff pb;
	BYTE f, r;
	UINT i, j, w;
	DWORD v;
	TCHAR c, d, str[32], *p;

	putc_init(&pb, fp);

	va_start(arp, fmt);

	for (;;) {
		c = *fmt++;
		if (c == 0) break;
		if (c != '%') {				/* Caractère non échappé */
			putc_bfd(&pb, c);
			continue;
		}
		w = f = 0;
		c = *fmt++;
		if (c == '0') {				/* Flag padding zéro */
			f = 1; c = *fmt++;
		} else {
			if (c == '-') {			/* Flag justification gauche */
				f = 2; c = *fmt++;
			}
		}
		while (IsDigit(c)) {		/* Précision */
			w = w * 10 + c - '0';
			c = *fmt++;
		}
		if (c == 'l' || c == 'L') {	/* Préfixe taille long int */
			f |= 4; c = *fmt++;
		}
		if (!c) break;
		d = c;
		if (IsLower(d)) d -= 0x20;
		switch (d) {
		case 'S' :
			p = va_arg(arp, TCHAR*);
			for (j = 0; p[j]; j++) ;
			if (!(f & 2)) {
				while (j++ < w) putc_bfd(&pb, ' ');
			}
			while (*p) putc_bfd(&pb, *p++);
			while (j++ < w) putc_bfd(&pb, ' ');
			continue;

		case 'C' :
			putc_bfd(&pb, (TCHAR)va_arg(arp, int)); continue;

		case 'B' :
			r = 2; break;

		case 'O' :
			r = 8; break;

		case 'D' :					/* Décimal signé */
		case 'U' :					/* Décimal non signé */
			r = 10; break;

		case 'X' :					/* Hexadécimal */
			r = 16; break;

		default:					/* Type inconnu transmis tel quel */
			putc_bfd(&pb, c); continue;
		}

		v = (f & 4) ? (DWORD)va_arg(arp, long) : ((d == 'D') ? (DWORD)(long)va_arg(arp, int) : (DWORD)va_arg(arp, unsigned int));
		if (d == 'D' && (v & 0x80000000)) {
			v = 0 - v;
			f |= 8;
		}
		i = 0;
		do {
			d = (TCHAR)(v % r); v /= r;
			if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
			str[i++] = d + '0';
		} while (v && i < sizeof str / sizeof str[0]);
		if (f & 8) str[i++] = '-';
		j = i; d = (f & 1) ? '0' : ' ';
		while (!(f & 2) && j++ < w) putc_bfd(&pb, d);
		do {
			putc_bfd(&pb, str[--i]);
		} while (i);
		while (j++ < w) putc_bfd(&pb, d);
	}

	va_end(arp);

	return putc_flush(&pb);
}

#endif
#endif

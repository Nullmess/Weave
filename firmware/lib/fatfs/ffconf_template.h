/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT file system module  R0.12c                             /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2017, ChaN, all right reserved.
/ Portions Copyright (C) STMicroelectronics, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:

/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/----------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module configuration file
/---------------------------------------------------------------------------*/

#define _FFCONF 68300

#define _FS_READONLY	0
/* 0 read/write ; 1 lecture seule, APIs d’écriture retirées */

#define _FS_MINIMIZE	0
/* 0 : toutes les APIs ; 1 : retrait stat/free/unlink/mkdir/truncate/rename */
/* 2 : retrait opendir/readdir/closedir en plus ; 3 : retrait lseek */

#define	_USE_STRFUNC	0
/* String I/O : 0 désactivé, 1 sans conversion, 2 LF vers CRLF */

#define _USE_FIND		0
/* Find : 0 désactivé, 1 activé, 2 inclut altname[] */

#define	_USE_MKFS		1
/* f_mkfs : 0 désactivé, 1 activé */

#define	_USE_FASTSEEK	1
/* Fast seek : 0 désactivé, 1 activé */

#define	_USE_EXPAND		0
/* f_expand : 0 désactivé, 1 activé */

#define _USE_CHMOD		0
/* chmod/utime : 1 active ; _FS_READONLY=0 requis */

#define _USE_LABEL		0
/* Label volume : 0 désactivé, 1 activé */

#define	_USE_FORWARD	0
/* f_forward : 0 désactivé, 1 activé */

#define _CODE_PAGE	850
/* Code page OEM ; mauvaise sélection : échec possible d’ouverture */

#define	_USE_LFN	3
#define	_MAX_LFN	255
/* LFN : 0 désactivé, 1 BSS non thread-safe, 2 stack, 3 heap */
/* _MAX_LFN dans [12,255] ; 255 pour support complet */
/* Buffer : (_MAX_LFN+1)*2 bytes, +608 en exFAT */
/* Unicode.c requis ; heap : ff_memalloc/free requis ; surveiller stack */

#define	_LFN_UNICODE	0
/* API : 0 ANSI/OEM, 1 UTF-16 ; LFN requis pour Unicode */

#define _STRF_ENCODE	3
/* Fichier Unicode : 0 ANSI/OEM, 1 UTF-16LE, 2 UTF-16BE, 3 UTF-8 */

#define _FS_RPATH	0
/* Path relatif : 0 désactivé, 1 chdir/chdrive, 2 ajoute getcwd */

#define _VOLUMES	2
/* Nombre de volumes logiques */

#define _STR_VOLUME_ID	0
#define _VOLUME_STRS	"RAM","NAND","CF","SD","SD2","USB","USB2","USB3"
/* ID volume string activé si _STR_VOLUME_ID=1 */
/* _VOLUME_STRS : _VOLUMES entrées, caractères A-Z et 0-9 */

#define	_MULTI_PARTITION	0
/* 0 : lecteur logique associé au même lecteur physique */
/* 1 : partitions arbitraires via VolToPart[], f_fdisk disponible */

#define	_MIN_SS		512
#define	_MAX_SS		512
/* Secteurs supportés : 512, 1024, 2048 ou 4096 bytes */
/* _MAX_SS >_MIN_SS : disk_ioctl GET_SECTOR_SIZE obligatoire */

#define	_USE_TRIM	0
/* TRIM activé si 1 ; disk_ioctl CTRL_TRIM obligatoire */

#define _FS_NOFSINFO	0
/* Bit 0=1 : ignorer nombre de clusters libres FSINFO, scan FAT */
/* Bit 1=1 : ignorer dernier cluster alloué FSINFO */

#define	_FS_TINY	0
/* Mode tiny : buffer FATFS partagé, FIL réduit de _MAX_SS bytes */

#define _FS_EXFAT	0
/* exFAT activé si 1 ; LFN requis, incompatible C89 */

#define _FS_NORTC	0
#define _NORTC_MON	1
#define _NORTC_MDAY	1
#define _NORTC_YEAR	2016

#define	_FS_LOCK	2
/* Sans lock : éviter opérations illégales sur objets ouverts */
/* File locks indépendants de la réentrance */

#define _FS_REENTRANT	1

#if _FS_REENTRANT
#include "cmsis_os.h"
#define _FS_TIMEOUT		1000
#define	_SYNC_t         osSemaphoreId
#endif
/* Réentrance par volume si 1 ; sync handlers utilisateur requis */
/* Volumes distincts réentrants ; mount/mkfs/fdisk jamais réentrants */

#if _USE_LFN == 3
#if !defined(ff_malloc) || !defined(ff_free)
#include <stdlib.h>
#endif

#if !defined(ff_malloc)
#define ff_malloc malloc
#endif

#if !defined(ff_free)
#define ff_free free
#endif
#endif

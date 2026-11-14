/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT file system module  R0.12c                             /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2017, ChaN, all right reserved.
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

#ifndef _FATFS
#define _FATFS	68300

#ifdef __cplusplus
extern "C" {
#endif

#include "integer.h"
#include "ffconf.h"

#if _FATFS != _FFCONF
#error Wrong configuration file (ffconf.h).
#endif

#if _MULTI_PARTITION
typedef struct {
	BYTE pd;	/* Numéro du lecteur physique */
	BYTE pt;	/* Partition : 0 auto, 1..4 imposée */
} PARTITION;
extern PARTITION VolToPart[];
#endif

#if _LFN_UNICODE			/* Chaîne Unicode UTF-16 */
#if _USE_LFN == 0
#error _LFN_UNICODE must be 0 at non-LFN cfg.
#endif
#ifndef _INC_TCHAR
typedef WCHAR TCHAR;
#define _T(x) L ## x
#define _TEXT(x) L ## x
#endif
#else						/* Chaîne ANSI/OEM */
#ifndef _INC_TCHAR
typedef char TCHAR;
#define _T(x) x
#define _TEXT(x) x
#endif
#endif

#if _FS_EXFAT
#if _USE_LFN == 0
#error LFN must be enabled when enable exFAT
#endif
typedef QWORD FSIZE_t;
#else
typedef DWORD FSIZE_t;
#endif

typedef struct {
	BYTE	fs_type;		/* Type filesystem : 0 non applicable */
	BYTE	drv;			/* Numéro du lecteur physique */
	BYTE	n_fats;			/* Une ou deux FAT */
	BYTE	wflag;			/* win[] : bit 0 dirty */
	BYTE	fsi_flag;		/* FSINFO : bit 7 désactivé, bit 0 dirty */
	WORD	id;
	WORD	n_rootdir;		/* Entrées racine FAT12/16 */
	WORD	csize;			/* Taille cluster en secteurs */
#if _MAX_SS != _MIN_SS
	WORD	ssize;			/* Secteurs : 512, 1024, 2048 ou 4096 bytes */
#endif
#if _USE_LFN != 0
	WCHAR*	lfnbuf;
#endif
#if _FS_EXFAT
	BYTE*	dirbuf;			/* Buffer temporaire du bloc d’entrées répertoire */
#endif
#if _FS_REENTRANT
	_SYNC_t	sobj;
#endif
#if !_FS_READONLY
	DWORD	last_clst;
	DWORD	free_clst;
#endif
#if _FS_RPATH != 0
	DWORD	cdir;			/* Premier cluster du répertoire courant ; 0 racine */
#if _FS_EXFAT
	DWORD	cdc_scl;		/* Premier cluster parent ; invalide si cdir=0 */
	DWORD	cdc_size;		/* Bits 31..8 : taille parent ; 7..0 : état chaîne */
	DWORD	cdc_ofs;		/* Offset parent invalide si cdir=0 */
#endif
#endif
	DWORD	n_fatent;		/* Entrées FAT : nombre de clusters +2 */
	DWORD	fsize;			/* Taille FAT en secteurs */
	DWORD	volbase;
	DWORD	fatbase;
	DWORD	dirbase;
	DWORD	database;
	DWORD	winsect;		/* Secteur actuellement chargé dans win[] */
	BYTE	win[_MAX_SS];	/* Fenêtre disque : répertoire/FAT/données si mode tiny */
} FATFS;

typedef struct {
	FATFS*	fs;			/* Pointeur vers le filesystem propriétaire */
	WORD	id;			/* Mount ID du filesystem propriétaire */
	BYTE	attr;
	BYTE	stat;		/* Chaîne 0 fragmentée, 2 contiguë sans FAT, 3 fragmentée ici ; bit2 étendu */
	DWORD	sclust;		/* Premier cluster ; 0 absent ou racine */
	FSIZE_t	objsize;	/* Taille objet valide si sclust !=0 */
#if _FS_EXFAT
	DWORD	n_cont;		/* Premier fragment : clusters-1, valide si stat=3 */
	DWORD	n_frag;		/* Dernier fragment à écrire, valide si non nul */
	DWORD	c_scl;		/* Cluster parent valide si sclust !=0 */
	DWORD	c_size;		/* Taille parent bits31..8, état chaîne bits7..0 ; c_scl !=0 */
	DWORD	c_ofs;		/* Offset parent valide si sclust !=0 et objet non-répertoire */
#endif
#if _FS_LOCK != 0
	UINT	lockid;		/* Lock ID depuis 1 ; index semaphore Files[] */
#endif
} _FDID;

typedef struct {
	_FDID	obj;			/* Identifiant obligatoirement premier membre pour validation des pointers */
	BYTE	flag;
	BYTE	err;			/* Flag abort contenant le code erreur */
	FSIZE_t	fptr;			/* Position read/write initialisée à zéro à l’ouverture */
	DWORD	clust;			/* Cluster courant invalide si fptr=0 */
	DWORD	sect;			/* Secteur chargé dans buf[] ; 0 invalide */
#if !_FS_READONLY
	DWORD	dir_sect;
	BYTE*	dir_ptr;		/* Pointer d’entrée dans win[] */
#endif
#if _USE_FASTSEEK
	DWORD*	cltbl;			/* CLMT NULL à l’ouverture, renseigné par l’application */
#endif
#if !_FS_TINY
	BYTE	buf[_MAX_SS];	/* Fenêtre read/write privée du fichier */
#endif
} FIL;

typedef struct {
	_FDID	obj;
	DWORD	dptr;
	DWORD	clust;
	DWORD	sect;			/* Secteur courant ; 0 marque fin de lecture */
	BYTE*	dir;			/* Pointer d’item dans win[] */
	BYTE	fn[12];			/* SFN : nom[8], extension[3], status[1] */
#if _USE_LFN != 0
	DWORD	blk_ofs;		/* Offset du bloc courant ; 0xFFFFFFFF invalide */
#endif
#if _USE_FIND
	const TCHAR* pat;
#endif
} DIR;

typedef struct {
	FSIZE_t	fsize;
	WORD	fdate;
	WORD	ftime;
	BYTE	fattrib;
#if _USE_LFN != 0
	TCHAR	altname[13];
	TCHAR	fname[_MAX_LFN + 1];
#else
	TCHAR	fname[13];
#endif
} FILINFO;

typedef enum {
	FR_OK = 0,
	FR_DISK_ERR,			/* Erreur hardware du disk I/O bas niveau */
	FR_INT_ERR,				/* Assertion échouée */
	FR_NOT_READY,			/* Lecteur physique indisponible */
	FR_NO_FILE,
	FR_NO_PATH,
	FR_INVALID_NAME,		/* Format chemin invalide */
	FR_DENIED,				/* Accès interdit ou répertoire plein */
	FR_EXIST,				/* Accès interdit */
	FR_INVALID_OBJECT,		/* Objet fichier/répertoire invalide */
	FR_WRITE_PROTECTED,		/* Lecteur protégé en écriture */
	FR_INVALID_DRIVE,		/* Numéro lecteur logique invalide */
	FR_NOT_ENABLED,			/* Volume sans zone de travail */
	FR_NO_FILESYSTEM,		/* Aucun volume FAT valide */
	FR_MKFS_ABORTED,		/* f_mkfs interrompu */
	FR_TIMEOUT,				/* Timeout acquisition accès au volume */
	FR_LOCKED,				/* Rejet par politique de partage des fichiers */
	FR_NOT_ENOUGH_CORE,		/* Allocation buffer LFN impossible */
	FR_TOO_MANY_OPEN_FILES,	/* Nombre de fichiers ouverts >_FS_LOCK */
	FR_INVALID_PARAMETER	/* Paramètre invalide */
} FRESULT;

FRESULT f_open (FIL* fp, const TCHAR* path, BYTE mode);
FRESULT f_close (FIL* fp);
FRESULT f_read (FIL* fp, void* buff, UINT btr, UINT* br);
FRESULT f_write (FIL* fp, const void* buff, UINT btw, UINT* bw);
FRESULT f_lseek (FIL* fp, FSIZE_t ofs);
FRESULT f_truncate (FIL* fp);
FRESULT f_sync (FIL* fp);
FRESULT f_opendir (DIR* dp, const TCHAR* path);
FRESULT f_closedir (DIR* dp);
FRESULT f_readdir (DIR* dp, FILINFO* fno);
FRESULT f_findfirst (DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);
FRESULT f_findnext (DIR* dp, FILINFO* fno);
FRESULT f_mkdir (const TCHAR* path);
FRESULT f_unlink (const TCHAR* path);
FRESULT f_rename (const TCHAR* path_old, const TCHAR* path_new);
FRESULT f_stat (const TCHAR* path, FILINFO* fno);
FRESULT f_chmod (const TCHAR* path, BYTE attr, BYTE mask);
FRESULT f_utime (const TCHAR* path, const FILINFO* fno);
FRESULT f_chdir (const TCHAR* path);
FRESULT f_chdrive (const TCHAR* path);
FRESULT f_getcwd (TCHAR* buff, UINT len);
FRESULT f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs);
FRESULT f_getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn);
FRESULT f_setlabel (const TCHAR* label);
FRESULT f_forward (FIL* fp, UINT(*func)(const BYTE*,UINT), UINT btf, UINT* bf);
FRESULT f_expand (FIL* fp, FSIZE_t szf, BYTE opt);					/* Allocation de blocs contigus au fichier */
FRESULT f_mount (FATFS* fs, const TCHAR* path, BYTE opt);
FRESULT f_mkfs (const TCHAR* path, BYTE opt, DWORD au, void* work, UINT len);
FRESULT f_fdisk (BYTE pdrv, const DWORD* szt, void* work);
int f_putc (TCHAR c, FIL* fp);
int f_puts (const TCHAR* str, FIL* cp);
int f_printf (FIL* fp, const TCHAR* str, ...);
TCHAR* f_gets (TCHAR* buff, int len, FIL* fp);

#define f_eof(fp) ((int)((fp)->fptr == (fp)->obj.objsize))
#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->obj.objsize)
#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)
#define f_rmdir(path) f_unlink(path)

#ifndef EOF
#define EOF (-1)
#endif

#if !_FS_READONLY && !_FS_NORTC
DWORD get_fattime (void);
#endif

#if _USE_LFN != 0
WCHAR ff_convert (WCHAR chr, UINT dir);
WCHAR ff_wtoupper (WCHAR chr);
#if _USE_LFN == 3
void* ff_memalloc (UINT msize);
void ff_memfree (void* mblock);
#endif
#endif

#if _FS_REENTRANT
int ff_cre_syncobj (BYTE vol, _SYNC_t* sobj);
int ff_req_grant (_SYNC_t sobj);
void ff_rel_grant (_SYNC_t sobj);
int ff_del_syncobj (_SYNC_t sobj);
#endif

/* Flags d’accès : troisième argument de f_open */
#define	FA_READ				0x01
#define	FA_WRITE			0x02
#define	FA_OPEN_EXISTING	0x00
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define	FA_OPEN_APPEND		0x30

/* Fast seek : deuxième argument de f_lseek */
#define CREATE_LINKMAP	((FSIZE_t)0 - 1)

/* Options format : deuxième argument de f_mkfs */
#define FM_FAT		0x01
#define FM_FAT32	0x02
#define FM_EXFAT	0x04
#define FM_ANY		0x07
#define FM_SFD		0x08

/* Type filesystem dans FATFS.fs_type */
#define FS_FAT12	1
#define FS_FAT16	2
#define FS_FAT32	3
#define FS_EXFAT	4

/* Attributs répertoire dans FILINFO.fattrib */
#define	AM_RDO	0x01	/* Lecture seule */
#define	AM_HID	0x02	/* Caché */
#define	AM_SYS	0x04	/* Système */
#define AM_DIR	0x10	/* Répertoire */
#define AM_ARC	0x20	/* Archive */

#ifdef __cplusplus
}
#endif

#endif

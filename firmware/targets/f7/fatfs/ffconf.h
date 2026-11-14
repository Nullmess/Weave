/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  *  FatFs - Generic FAT file system module  R0.12c (C)ChaN, 2017
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef _FFCONF
#define _FFCONF 68300

#ifdef FURI_RAM_EXEC
#define _FS_READONLY 1 /* 0 : lecture/écriture ; 1 : lecture seule */
#else
#define _FS_READONLY 0 /* 0 : lecture/écriture ; 1 : lecture seule */
#endif
/* Lecture seule : toutes les API écriture retirées */

#define _FS_MINIMIZE 0 /* Niveaux 0 à 3 */
/* 0 : toutes les API disponibles */
/* 1 : retrait stat/getfree/unlink/mkdir/truncate/rename */
/* 2 : retrait supplémentaire opendir/readdir/closedir */
/* 3 : retrait supplémentaire lseek */

#define _USE_STRFUNC 0 /* 0 : désactivé ; 1/2 : activé */
/* 0 : fonctions string désactivées */
/* 1 : sans conversion LF/CRLF */
/* 2 : conversion LF vers CRLF */

#define _USE_FIND 0
/* findfirst/findnext : 0 off ; 1 on ; 2 inclut altname[] */

#ifdef FURI_RAM_EXEC
#define _USE_MKFS 0
#else
#define _USE_MKFS 1
#endif

#define _USE_FASTSEEK 1

#define _USE_EXPAND 0

#define _USE_CHMOD 0
/* chmod/utime nécessitent _FS_READONLY=0 */

#define _USE_LABEL 1

#define _USE_FORWARD 0

#define _CODE_PAGE 850
/* Code page OEM erronée : ouverture impossible */
/* 1 : ASCII sans LFN ; 437 : US ; 720/864 : arabe */
/* 737/869 : grec ; 771 : KBL ; 775 : baltique */
/* 850/852 : latin ; 855/866 : cyrillique */
/* 857 : turc ; 860 : portugais ; 861 : islandais */
/* 862 : hébreu ; 863 : français canadien ; 865 : nordique */
/* 932 : japonais ; 936 : chinois simplifié */
/* 949 : coréen ; 950 : chinois traditionnel */

#define _USE_LFN 2 /* Niveaux 0 à 3 */
#define _MAX_LFN 255 /* LFN : 12 à 255 caractères */
/* _USE_LFN : 0 off ; 1 BSS ; 2 stack ; 3 heap */
/* Mode 1 non thread-safe ; mode 2 : risque stack overflow */
/* Unicode requis : option/unicode.c */
/* Buffer : (_MAX_LFN+1)*2 octets ; exFAT ajoute 608 octets */
/* _MAX_LFN : 12..255 ; 255 pour support complet */
/* Heap : ff_memalloc/ff_memfree requis */

#define _LFN_UNICODE 0 /* 0 : ANSI/OEM ; 1 : Unicode */
/* UTF-16 : LFN et _LFN_UNICODE=1 requis, y compris string I/O */

#define _STRF_ENCODE 0
/* _LFN_UNICODE=1 : encodage fichier pour string I/O */
/* 0 ANSI/OEM ; 1 UTF-16LE ; 2 UTF-16BE ; 3 UTF-8 */
/* Sans effet si _LFN_UNICODE=0 */

#define _FS_RPATH 0 /* Niveaux 0 à 2 */
/* 0 : chemins relatifs désactivés */
/* 1 : chdir/chdrive disponibles ; 2 : ajoute getcwd */

#define _VOLUMES 1

/* USER CODE BEGIN Volumes */
#define _STR_VOLUME_ID 0 /* Volume ID : 0 chiffres 0..9 ; 1 strings */
#define _VOLUME_STRS   "SD"
/* _STR_VOLUME_ID=1 : strings _VOLUME_STRS utilisables comme volumes */
/* Une entrée par _VOLUMES ; caractères A-Z/0-9 */

/* USER CODE END Volumes */

#define _MULTI_PARTITION 0 /* 0 : partition unique ; 1 : partitions multiples */
/* 0 : volume logique lié au disque homonyme, FAT unique */
/* 1 : mapping disque/partition via VolToPart[], f_fdisk disponible */

#define _MIN_SS          512 /* 512, 1024, 2048 ou 4096 */
#define _MAX_SS          512 /* 512, 1024, 2048 ou 4096 */
/* Secteurs : 512/1024/2048/4096 octets ; SD/disques : 512 */
/* _MAX_SS>_MIN_SS : GET_SECTOR_SIZE requis dans disk_ioctl */

#define _USE_TRIM 0
/* ATA-TRIM nécessite CTRL_TRIM dans disk_ioctl */

#define _FS_NOFSINFO 0 /* 0, 1, 2 ou 3 */
/* bit0=0 : compteur FSINFO ; bit0=1 : scan FAT complet */
/* bit1=0 : dernier cluster FSINFO ; bit1=1 : ignorer */

#define _FS_TINY 0 /* 0 : normal ; 1 : tiny */
/* Tiny : buffer FATFS partagé ; FIL réduit de _MAX_SS octets */

#define _FS_EXFAT 1
/* exFAT nécessite _USE_LFN>=1 ; incompatible C89 */

#define _FS_NORTC 0
/* _FS_NORTC=1 : horodatage local fixe via _NORTC_* */
/* _FS_NORTC=0 : get_fattime requis ; _NORTC_* ignorés */
/* Sans effet en lecture seule */

#define _FS_LOCK 0 /* 0 : désactivé ; >=1 : activé */
/* _FS_LOCK=0 obligatoire en lecture seule */
/* Sans lock : éviter suppression/renommage/ouvertures incompatibles */
/* >0 : nombre maximal de fichiers/dossiers verrouillés */
/* Lock indépendant de la réentrance */

#define _FS_REENTRANT 0 /* 0 : désactivé ; 1 : activé */
#define _FS_TIMEOUT   1000 /* Timeout en ticks */
#define _SYNC_t       FuriMutex*
/* Volumes distincts toujours réentrants */
/* mount/mkfs/fdisk jamais réentrants */
/* _FS_REENTRANT protège les accès au même volume */
/* Mode 1 : ff_req_grant/rel_grant/del_syncobj/cre_syncobj requis */
/* Mode 0 : _FS_TIMEOUT et _SYNC_t ignorés */
/* _FS_TIMEOUT en ticks ; _SYNC_t dépend du système */
/* Inclure les définitions système dans le scope de ff.h */

/* ff_malloc/ff_free vers malloc/free standard */
#if !defined(ff_malloc) && !defined(ff_free)
#include <stdlib.h>
#define ff_malloc malloc
#define ff_free   free
#endif

#endif

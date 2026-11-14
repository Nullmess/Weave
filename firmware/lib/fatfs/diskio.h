/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2014          /
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#define _USE_WRITE	1	/* 1 active disk_write */
#define _USE_IOCTL	1	/* 1 active disk_ioctl */

#include "integer.h"

typedef BYTE	DSTATUS;

typedef enum {
	RES_OK = 0,
	RES_ERROR,		/* Erreur read/write */
	RES_WRPRT,		/* Protégé en écriture */
	RES_NOTRDY,		/* Non prêt */
	RES_PARERR		/* Paramètre invalide */
} DRESULT;

DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);

#define STA_NOINIT		0x01	/* Lecteur non initialisé */
#define STA_NODISK		0x02	/* Aucun support dans le lecteur */
#define STA_PROTECT		0x04	/* Protégé en écriture */

#define CTRL_SYNC		0	/* Flush des writes pending requis si _FS_READONLY=0 */
#define GET_SECTOR_COUNT	1	/* Taille support requise si _USE_MKFS=1 */
#define GET_SECTOR_SIZE		2	/* Taille secteur requise si _MAX_SS !=_MIN_SS */
#define GET_BLOCK_SIZE		3	/* Taille erase block requise si _USE_MKFS=1 */
#define CTRL_TRIM		4	/* Notification blocs inutilisés requise si _USE_TRIM=1 */

/* Commandes génériques inutilisées par FatFs */
#define CTRL_POWER			5
#define CTRL_LOCK			6	/* Verrouillage de l’éjection du support */
#define CTRL_EJECT			7
#define CTRL_FORMAT			8

#define MMC_GET_TYPE		10
#define MMC_GET_CSD			11
#define MMC_GET_CID			12
#define MMC_GET_OCR			13
#define MMC_GET_SDSTAT		14

#define ATA_GET_REV			20
#define ATA_GET_MODEL		21
#define ATA_GET_SN			22

#ifdef __cplusplus
}
#endif

#endif

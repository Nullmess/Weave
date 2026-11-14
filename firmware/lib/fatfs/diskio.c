/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2017        */
/*                                                                       */
/*   Portions COPYRIGHT 2017 STMicroelectronics                          */
/*   Portions Copyright (C) 2017, ChaN, all right reserved               */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various existing      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "diskio.h"
#include "ff_gen_drv.h"

#if defined ( __GNUC__ )
#ifndef __weak
#define __weak __attribute__((weak))
#endif
#endif

extern Disk_drvTypeDef  disk;

/* Pdrv : numéro de lecteur physique depuis 0 */
DSTATUS disk_status (
	BYTE pdrv		/* Numéro du lecteur physique */
)
{
  DSTATUS stat;

  stat = disk.drv[pdrv]->disk_status(disk.lun[pdrv]);
  return stat;
}

/* Pdrv : numéro de lecteur physique depuis 0 */
DSTATUS disk_initialize (
	BYTE pdrv				/* Numéro du lecteur physique */
)
{
  DSTATUS stat = RES_OK;

  if(disk.is_initialized[pdrv] == 0)
  {
    disk.is_initialized[pdrv] = 1;
    stat = disk.drv[pdrv]->disk_initialize(disk.lun[pdrv]);
  }
  return stat;
}

/* Secteur en LBA ; lecture de 1..128 secteurs */
DRESULT disk_read (
	BYTE pdrv,		/* Numéro du lecteur physique */
	BYTE *buff,
	DWORD sector,	        /* Adresse secteur en LBA */
	UINT count		/* Nombre de secteurs à lire */
)
{
  DRESULT res;

  res = disk.drv[pdrv]->disk_read(disk.lun[pdrv], buff, sector, count);
  return res;
}

/* Secteur en LBA ; écriture de 1..128 secteurs */
#if _USE_WRITE == 1
DRESULT disk_write (
	BYTE pdrv,		/* Numéro du lecteur physique */
	const BYTE *buff,
	DWORD sector,		/* Adresse secteur en LBA */
	UINT count        	/* Nombre de secteurs à écrire */
)
{
  DRESULT res;

  res = disk.drv[pdrv]->disk_write(disk.lun[pdrv], buff, sector, count);
  return res;
}
#endif

#if _USE_IOCTL == 1
DRESULT disk_ioctl (
	BYTE pdrv,		/* Numéro lecteur physique depuis 0 */
	BYTE cmd,
	void *buff
)
{
  DRESULT res;

  res = disk.drv[pdrv]->disk_ioctl(disk.lun[pdrv], cmd, buff);
  return res;
}
#endif

/* Timestamp RTC en DWORD */
__weak DWORD get_fattime (void)
{
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

// For definition of OS_FileSystem, has to be included after the global header
#include "include/OS_FileSystem.h"

#include "ff.h"
#include "diskio.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

#include <string.h>

// Default configuration for FatFs
#define FATFS_DEFAULT_N_FAT 1
#define FATFS_DEFAULT_FMT FM_FAT
static const OS_FileSystem_Format_t fatFs_defaultConfig =
{
    .fatFs = {
        .createPartition = true,
        .sectorSize = 512,
        .blockSize = 16,
        .clusterSize = 128,
    }
};

// Private Functions -----------------------------------------------------------

static DSTATUS
storage_initialize(
    void* ctx,
    BYTE pdrv)
{
    return RES_OK;;
}

static DSTATUS
storage_status (
    void* ctx,
    BYTE pdrv)
{
    return RES_OK;
}

static DRESULT
storage_read(
    void* ctx,
    BYTE pdrv,
    BYTE* buff,
    LBA_t sector,
    UINT count)
{
    OS_Error_t err;
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) ctx;
    size_t sectorSize = self->cfg.format->fatFs.sectorSize;
    size_t read, addr, size;

    size = sectorSize * count;
    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        return RES_PARERR;
    }

    addr = sectorSize * sector;
    if ((err = self->cfg.storage.read(addr, size, &read)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("read() failed with %i", err);
        return RES_ERROR;
    }

    if (read != size)
    {
        Debug_LOG_ERROR("read() requested to read %zu bytes but got %zu bytes",
                        size, read);
        return RES_ERROR;
    }

    memcpy(buff, OS_Dataport_getBuf(self->cfg.storage.dataport), read);

    return RES_OK;
}

static DRESULT
storage_write (
    void* ctx,
    BYTE pdrv,
    const BYTE* buff,
    LBA_t sector,
    UINT count)
{
    OS_Error_t err;
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) ctx;
    size_t sectorSize = self->cfg.format->fatFs.sectorSize;
    size_t written, addr, size;

    size = sectorSize * count;
    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        return RES_PARERR;
    }

    memcpy(OS_Dataport_getBuf(self->cfg.storage.dataport), buff, size);

    addr = sectorSize * sector;
    if ((err = self->cfg.storage.write(addr, size, &written)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("write() failed with %i", err);
        return RES_ERROR;
    }

    if (written != size)
    {
        Debug_LOG_ERROR("write() requested to write %zu bytes but got %zu bytes",
                        size, written);
        return RES_ERROR;
    }

    return RES_OK;
}

static DRESULT
storage_ioctl (
    void* ctx,
    BYTE pdrv,
    BYTE cmd,
    void* buff)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) ctx;
    size_t sectorSize = self->cfg.format->fatFs.sectorSize;
    size_t blockSize = self->cfg.format->fatFs.blockSize;

    switch (cmd)
    {
    case GET_SECTOR_COUNT:
        (*(LBA_t*) buff) = (LBA_t) (self->cfg.size / sectorSize);
        return RES_OK;
    case GET_SECTOR_SIZE:
        (*(WORD*) buff) = (WORD) sectorSize;
        return RES_OK;
    case GET_BLOCK_SIZE:
        (*(DWORD*) buff) = (DWORD) blockSize;
        return RES_OK;
    case CTRL_SYNC:
    case CTRL_TRIM:
        return RES_OK;
    }

    return RES_ERROR;
}

// Public Functions ------------------------------------------------------------

OS_Error_t
FatFs_init(
    OS_FileSystem_Handle_t self)
{
    OS_FileSystem_Config_t* cfg = &self->cfg;

    if  (NULL == cfg->format)
    {
        cfg->format = &fatFs_defaultConfig;
    }

    // Check we are aligned with the sector size
    if (cfg->size % cfg->format->fatFs.sectorSize)
    {
        Debug_LOG_ERROR("Storage size of %zu bytes is not aligned with "
                        "sector size of %i bytes",
                        cfg->size, cfg->format->fatFs.blockSize);
        return OS_ERROR_INVALID_PARAMETER;
    }

    // Assign callbacks
    self->fs.fatFs.dio.disk_initialize = storage_initialize;
    self->fs.fatFs.dio.disk_status = storage_status;
    self->fs.fatFs.dio.disk_read = storage_read;
    self->fs.fatFs.dio.disk_write = storage_write;
    self->fs.fatFs.dio.disk_ioctl = storage_ioctl;

    // Assign context so we can get it in the callbacks
    self->fs.fatFs.dio.ctx = (void*) self;

    // Assign dio structure so it is part of the fctx
    self->fs.fatFs.fctx.dio = &self->fs.fatFs.dio;

    return OS_SUCCESS;
}

OS_Error_t
FatFs_free(
    OS_FileSystem_Handle_t self)
{
    return OS_SUCCESS;
}

OS_Error_t
FatFs_format(
    OS_FileSystem_Handle_t self)
{
    MKFS_PARM parms =
    {
        .n_fat   = FATFS_DEFAULT_N_FAT,
        .fmt     = FATFS_DEFAULT_FMT,
        // Cluster size determines the FAT12/FAT16/FAT32 selection
        .au_size = self->cfg.format->fatFs.clusterSize,

    };
    FRESULT rc;

    //
    // From the documentation:
    // "There are two disk formats, FDISK and SFD. The FDISK format is usually
    //  used for harddisk, MMC, SDC, CFC and U Disk. It can divide a physical
    //  drive into one or more partitions with a partition table on the MBR
    //  (master boot record, the first sector of the physical drive). The SFD
    //  (super-floppy disk) is non-partitioned disk format. The FAT volume starts
    //  at the first sector of the physical drive without any disk partitioning.
    //  It is usually used for floppy disk, Microdrive, optical disk and most
    //  types of super-floppy media. Some systems support only either one of
    //  two formats and other is not supported.
    //
    //  When FM_SFD is not specified (the volume is bound to a physical drive),
    //  a primary partition occupies whole drive space is created and then the
    //  FAT volume is created in it. When FM_SFD is specified, the FAT volume
    //  occupies from the first sector of the drive is created."
    //
    if (!self->cfg.format->fatFs.createPartition)
    {
        parms.fmt |= FM_SFD;
    }

    if ((rc = f_mkfs(&self->fs.fatFs.fctx, "", &parms,
                     self->fs.fatFs.buffer,
                     sizeof(self->fs.fatFs.buffer))) != FR_OK)
    {
        Debug_LOG_ERROR("f_mkfs() failed with %i", rc);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
FatFs_mount(
    OS_FileSystem_Handle_t self)
{
    BYTE mountNow = 1;
    FRESULT rc;

    // We want to mount the fs immediately so we can detect broken file systems
    // or those that are not properly formatted
    if ((rc = f_mount(&self->fs.fatFs.fctx,
                      &self->fs.fatFs.fs, "", mountNow)) != FR_OK)
    {
        Debug_LOG_ERROR("f_mount() failed with %i", rc);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
FatFs_unmount(
    OS_FileSystem_Handle_t self)
{
    FRESULT rc;

    if ((rc = f_mount(&self->fs.fatFs.fctx, NULL, "", 0)) != FR_OK)
    {
        Debug_LOG_ERROR("f_mount() failed with %i", rc);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}
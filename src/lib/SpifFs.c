/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"
#include "OS_FileSystem_int.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

#include <string.h>


#include "spiffs.h"
#include "spiffs_config.h"


/* Defines -------------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static const OS_FileSystem_Format_t SpifFs_defaultConfig =
{
    // the spiffs page size has to be smaller than the spiffs block size
    .spifFs = {
        .physEraseBlock = 4096,
        .logBlockSize   = 4096,
        .logPageSize    = 256,
        .physAddr       = 0,
        .cachePages     = 16,
    }
};


// Private Functions -----------------------------------------------------------


static int32_t spiffsRead(struct spiffs_t *fs, uint32_t addr, uint32_t size,
                          uint8_t* dst)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t)     fs->user_data;
    OS_Error_t err;
    size_t read;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        return OS_ERROR_BUFFER_TOO_SMALL;
    }

    if ((err = self->cfg.storage.read(addr, size, &read)) != OS_SUCCESS)
    {
        return err;
    }

    if (read != size)
    {
        return OS_ERROR_ABORTED;
    }

    memcpy(dst, OS_Dataport_getBuf(self->cfg.storage.dataport), read);

    return OS_SUCCESS;
}

static int32_t spiffsWrite(struct spiffs_t *fs, uint32_t addr, uint32_t size,
                           uint8_t* src)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t)     fs->user_data;
    OS_Error_t err;
    size_t written;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        return OS_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(OS_Dataport_getBuf(self->cfg.storage.dataport), src, size);

    if ((err = self->cfg.storage.write(addr, size, &written)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("write() failed with %d", err);
        return err;
    }

    if (written != size)
    {
        Debug_LOG_ERROR("write() requested to write %d bytes but got %zu bytes",
                        size, written);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

static int32_t spiffsErase(struct spiffs_t *fs, uint32_t addr, uint32_t size)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) fs->user_data;
    OS_Error_t err;
    size_t erased;

    if ((err = self->cfg.storage.erase(addr, size, &erased)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("erase() failed with %d", err);
        return err;
    }

    if (erased != size)
    {
        Debug_LOG_ERROR("erase() requested to erase %zu bytes but erased %zu bytes",
                        size, erased);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}


// Public Functions ------------------------------------------------------------

OS_Error_t
SpifFs_init(
    OS_FileSystem_Handle_t self)
{
    OS_Error_t err;
    OS_FileSystem_Config_t* cfg = &self->cfg;
    size_t pageSz, blockSz;

    if  (NULL == cfg->format)
    {
        cfg->format = &SpifFs_defaultConfig;
    }

    // We need to ensure that page size is smaller than block size
    pageSz  = cfg->format->spifFs.logPageSize;
    blockSz = cfg->format->spifFs.logBlockSize;
    if (pageSz >= blockSz)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    self->fs.spifFs.cfg.phys_size        = cfg->size;
    self->fs.spifFs.cfg.phys_addr        = cfg->format->spifFs.physAddr;
    self->fs.spifFs.cfg.phys_erase_block = cfg->format->spifFs.physEraseBlock;
    self->fs.spifFs.cfg.log_block_size   = blockSz;
    self->fs.spifFs.cfg.log_page_size    = pageSz;
    self->fs.spifFs.cfg.hal_read_f       = spiffsRead;
    self->fs.spifFs.cfg.hal_write_f      = spiffsWrite;
    self->fs.spifFs.cfg.hal_erase_f      = spiffsErase;

    // These size calculations are taken from SPIFFS test code
    self->fs.spifFs.cacheSize = (cfg->format->spifFs.cachePages * (sizeof(
                                     spiffs_cache_page) + pageSz)) + sizeof(spiffs_cache);

    if ((self->fs.spifFs.spiffs_cache_buf =
             malloc(self->fs.spifFs.cacheSize)) == NULL)
    {
        return OS_ERROR_INSUFFICIENT_SPACE;
    }
    if ((self->fs.spifFs.spiffs_work_buf =
             malloc(pageSz * 2)) == NULL)
    {
        err = OS_ERROR_INSUFFICIENT_SPACE;
        goto err0;
    }

    self->fs.spifFs.fs.user_data = (void*) self;

    return OS_SUCCESS;

err0:
    free(self->fs.spifFs.spiffs_cache_buf);
    return err;
}

OS_Error_t
SpifFs_free(
    OS_FileSystem_Handle_t self)
{
    free(self->fs.spifFs.spiffs_cache_buf);
    free(self->fs.spifFs.spiffs_work_buf);

    return OS_SUCCESS;
}

OS_Error_t
SpifFs_format(
    OS_FileSystem_Handle_t self)
{
    spiffs* fs         = &self->fs.spifFs.fs;
    spiffs_config* cfg = &self->fs.spifFs.cfg;
    int rc;

    // SPIFFS_format needs to be called with an initalized spiffs structure,
    // the initialization of which happens in SPIFFS_mount.

    rc = SPIFFS_mount(fs, cfg, self->fs.spifFs.spiffs_work_buf,
                      self->fs.spifFs.spiffs_fds,
                      sizeof(self->fs.spifFs.spiffs_fds),
                      self->fs.spifFs.spiffs_cache_buf,
                      self->fs.spifFs.cacheSize, 0);

    if (rc == SPIFFS_OK)
    {
        SPIFFS_unmount (fs);
    }

    if ((rc = SPIFFS_format(fs)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_format() failed with %d", rc);
    }

    return (rc < 0) ? OS_ERROR_GENERIC : OS_SUCCESS;
}

OS_Error_t
SpifFs_mount(
    OS_FileSystem_Handle_t self)
{
    spiffs* fs         = &self->fs.spifFs.fs;
    spiffs_config* cfg = &self->fs.spifFs.cfg;
    int rc;

    rc = SPIFFS_mount(fs, cfg, self->fs.spifFs.spiffs_work_buf,
                      self->fs.spifFs.spiffs_fds,
                      sizeof(self->fs.spifFs.spiffs_fds),
                      self->fs.spifFs.spiffs_cache_buf,
                      self->fs.spifFs.cacheSize, 0);

    return (rc == SPIFFS_OK) ? OS_SUCCESS : OS_ERROR_GENERIC;
}

OS_Error_t
SpifFs_unmount(
    OS_FileSystem_Handle_t self)
{
    spiffs* fs         = &self->fs.spifFs.fs;

    // SPIFFS_unmount does not return an error code.
    SPIFFS_unmount (fs);

    return OS_SUCCESS;
}


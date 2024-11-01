/*
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#include "OS_FileSystem.h"
#include "OS_FileSystem_int.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "lib_debug/Debug.h"

#include "lfs.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// Default configuration for LittleFS
#define LITTLEFS_DEFAULT_CACHE_SIZE 4096
#define LITTLEFS_DEFAULT_LOOKAHEAD_SIZE 16
static const OS_FileSystem_Format_t littleFs_defaultConfig =
{
    .littleFs = {
        .readSize = 16,
        .writeSize = 16,
        .blockSize = 4096,
        .blockCycles = 500,
    }
};

// Private Functions -----------------------------------------------------------

static int
storage_read(
    const struct lfs_config* c,
    lfs_block_t              block,
    lfs_off_t                off,
    void*                    buffer,
    lfs_size_t               size)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) c->context;
    OS_Error_t err;
    off_t addr;
    size_t read;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        self->ioError = OS_ERROR_BUFFER_TOO_SMALL;
        return self->ioError;
    }

    addr = off + (c->block_size * block);
    if ((err = self->cfg.storage.read(addr, size, &read)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("read() failed with %d", err);
        self->ioError = err;
        return self->ioError;
    }

    if (read != size)
    {
        Debug_LOG_ERROR("read() requested to read %d bytes but got %zu bytes",
                        size, read);
        self->ioError = OS_ERROR_ABORTED;
        return self->ioError;
    }

    memcpy(buffer, OS_Dataport_getBuf(self->cfg.storage.dataport), read);

    self->ioError = OS_SUCCESS;
    return 0;
}

static int
storage_prog(
    const struct lfs_config* c,
    lfs_block_t              block,
    lfs_off_t                off,
    const void*              buffer,
    lfs_size_t               size)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) c->context;
    OS_Error_t err;
    off_t addr;
    size_t written;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        self->ioError = OS_ERROR_BUFFER_TOO_SMALL;
        return self->ioError;
    }

    memcpy(OS_Dataport_getBuf(self->cfg.storage.dataport), buffer, size);

    addr = off + (c->block_size * block);
    if ((err = self->cfg.storage.write(addr, size, &written)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("write() failed with %d", err);
        self->ioError = err;
        return self->ioError;
    }

    if (written != size)
    {
        Debug_LOG_ERROR("write() requested to write %u bytes but got %zu bytes",
                        size, written);
        self->ioError = OS_ERROR_ABORTED;
        return self->ioError;
    }

    self->ioError = OS_SUCCESS;
    return 0;
}

static int
storage_erase(
    const struct lfs_config* c,
    lfs_block_t              block)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) c->context;
    OS_Error_t err;
    off_t addr;
    off_t erased, size;

    addr = c->block_size * block;
    size = c->block_size;
    if ((err = self->cfg.storage.erase(addr, size, &erased)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("erase() failed with %d", err);
        self->ioError = err;
        return self->ioError;
    }

    if (erased != size)
    {
        Debug_LOG_ERROR(
            "erase() requested to erase %" PRIiMAX " bytes "
            "but erased %" PRIiMAX " bytes",
            size,
            erased);
        self->ioError = OS_ERROR_ABORTED;
        return self->ioError;
    }

    self->ioError = OS_SUCCESS;
    return 0;
}

static int
storage_sync(
    const struct lfs_config* c)
{
    /*
     * For now, we just assume writes are never cached so a sync() is not
     * really necessary..
     */
    (void) c;
    return 0;
}

// Public Functions -----------------------------------------------------------

OS_Error_t
LittleFs_init(
    OS_FileSystem_Handle_t self)
{
    OS_FileSystem_Config_t* cfg = &self->cfg;
    struct lfs_config* lfsCfg = &self->fs.littleFs.cfg;

    // If user doesn't give us anything, we load some defaults
    if  (NULL == cfg->format)
    {
        cfg->format = &littleFs_defaultConfig;
    }

    // Set storage specific options
    lfsCfg->cache_size     = LITTLEFS_DEFAULT_CACHE_SIZE;
    lfsCfg->lookahead_size = LITTLEFS_DEFAULT_LOOKAHEAD_SIZE;
    lfsCfg->read_size      = cfg->format->littleFs.readSize;
    lfsCfg->prog_size      = cfg->format->littleFs.writeSize;
    lfsCfg->block_size     = cfg->format->littleFs.blockSize;
    lfsCfg->block_cycles   = cfg->format->littleFs.blockCycles;

    // Compute the block count based on the overall size of the storage, but
    // make sure it is aligned with the block size
    if (cfg->size % cfg->format->littleFs.blockSize)
    {
        Debug_LOG_ERROR("Storage size of %" PRIiMAX " bytes is not aligned "
                        "with block size of %d bytes",
                        cfg->size, cfg->format->littleFs.blockSize);
        return OS_ERROR_INVALID_PARAMETER;
    }
    lfsCfg->block_count = cfg->size / cfg->format->littleFs.blockSize;

    Debug_LOG_INFO("Using LITTLEFS ("
                   "cache_size = %u, "
                   "lookahead_size = %u, "
                   "read_size = %u, "
                   "prog_size = %u, "
                   "block_size = %u, "
                   "block_cycles = %u)",
                   lfsCfg->cache_size,
                   lfsCfg->lookahead_size,
                   lfsCfg->read_size,
                   lfsCfg->prog_size,
                   lfsCfg->block_size,
                   lfsCfg->block_cycles);

    // Set callbacks
    lfsCfg->read  = storage_read;
    lfsCfg->prog  = storage_prog;
    lfsCfg->erase = storage_erase;
    lfsCfg->sync  = storage_sync;

    // Set pointer to our own context
    lfsCfg->context = (void*) self;

    return OS_SUCCESS;
}

OS_Error_t
LittleFs_free(
    OS_FileSystem_Handle_t self)
{
    (void) self;
    return OS_SUCCESS;
}

OS_Error_t
LittleFs_format(
    OS_FileSystem_Handle_t self)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    struct lfs_config* cfg = &self->fs.littleFs.cfg;
    int rc;

    if ((rc = lfs_format(fs, cfg)) < 0)
    {
        Debug_LOG_ERROR("lfs_format() failed with %d", rc);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
LittleFs_mount(
    OS_FileSystem_Handle_t self)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    struct lfs_config* cfg = &self->fs.littleFs.cfg;
    int rc;

    if ((rc = lfs_mount(fs, cfg)) < 0)
    {
        Debug_LOG_ERROR("lfs_mount() failed with %d", rc);
        // If we have an ioError, return that; otherwise check if LittleFS
        // complained about corruption, which we interpret as "wrong fs" and
        // return NOT_FOUND; otherwise return GENERIC.
        return (self->ioError != OS_SUCCESS)
               ? self->ioError : (rc == LFS_ERR_CORRUPT)
               ? OS_ERROR_NOT_FOUND : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
LittleFs_unmount(
    OS_FileSystem_Handle_t self)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    int rc;

    if ((rc = lfs_unmount(fs)) < 0)
    {
        Debug_LOG_ERROR("lfs_unmount() failed with %d", rc);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

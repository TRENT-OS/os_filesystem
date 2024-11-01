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

#include <string.h>
#include <inttypes.h>

#include "spiffs.h"
#include "spiffs_config.h"

// This is an offset into the spiffs file system structure, which we do not want
// the user to set manually. It can be done via the StorageServer.
#define SPIFFS_DEFAULT_PHYS_ADDR 0
static const OS_FileSystem_Format_t SpifFs_defaultConfig =
    {
        .spifFs = {
            .eraseBlockSize = 4096,
            .logicalBlockSize = 4096,
            .logicalPageSize = 256,
            .cachePages = 16,
        }};

// Private Functions -----------------------------------------------------------

static int32_t
storage_read(
    struct spiffs_t *fs,
    uint32_t addr,
    uint32_t size,
    uint8_t *dst)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t)fs->user_data;
    OS_Error_t err;
    size_t read;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        self->ioError = OS_ERROR_BUFFER_TOO_SMALL;
        return self->ioError;
    }

    if ((err = self->cfg.storage.read(addr, size, &read)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("read() failed with %d", err);
        self->ioError = err;
        return self->ioError;
    }

    if (read != size)
    {
        Debug_LOG_ERROR("read() requested to read %u bytes but got %zu bytes",
                        size, read);
        self->ioError = OS_ERROR_ABORTED;
        return self->ioError;
    }

    memcpy(dst, OS_Dataport_getBuf(self->cfg.storage.dataport), read);

    self->ioError = OS_SUCCESS;
    return OS_SUCCESS;
}

static int32_t
storage_write(
    struct spiffs_t *fs,
    uint32_t addr,
    uint32_t size,
    uint8_t *src)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t)fs->user_data;
    OS_Error_t err;
    size_t written;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        self->ioError = OS_ERROR_BUFFER_TOO_SMALL;
        return self->ioError;
    }

    memcpy(OS_Dataport_getBuf(self->cfg.storage.dataport), src, size);

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
    return OS_SUCCESS;
}

static int32_t
storage_erase(
    struct spiffs_t *fs,
    uint32_t addr,
    uint32_t size)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t)fs->user_data;
    OS_Error_t err;
    off_t erased;

    if ((err = self->cfg.storage.erase(addr, size, &erased)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("erase() failed with %d", err);
        self->ioError = err;
        return self->ioError;
    }

    if (erased != size)
    {
        Debug_LOG_ERROR(
            "erase() requested to erase %u bytes but erased %" PRIiMAX " bytes",
            size,
            erased);
        self->ioError = OS_ERROR_ABORTED;
        return self->ioError;
    }

    self->ioError = OS_SUCCESS;
    return OS_SUCCESS;
}

// Public Functions ------------------------------------------------------------

OS_Error_t
SpifFs_init(
    OS_FileSystem_Handle_t self)
{
    OS_Error_t err;
    OS_FileSystem_Config_t *cfg = &self->cfg;
    size_t pageSz, blockSz;

    if (NULL == cfg->format)
    {
        cfg->format = &SpifFs_defaultConfig;
    }

    // We need to ensure that page size is smaller than block size
    pageSz = cfg->format->spifFs.logicalPageSize;
    blockSz = cfg->format->spifFs.logicalBlockSize;
    if (pageSz >= blockSz)
    {
        Debug_LOG_ERROR("Block size (%zu bytes) is smaller than page size "
                        "(%zu bytes)",
                        blockSz, pageSz);
        return OS_ERROR_INVALID_PARAMETER;
    }

    self->fs.spifFs.cfg.phys_addr = SPIFFS_DEFAULT_PHYS_ADDR;
    self->fs.spifFs.cfg.phys_size = cfg->size;
    self->fs.spifFs.cfg.phys_erase_block = cfg->format->spifFs.eraseBlockSize;
    self->fs.spifFs.cfg.log_block_size = blockSz;
    self->fs.spifFs.cfg.log_page_size = pageSz;

    Debug_LOG_INFO("Using SPIFFS ("
                   "phys_addr = %u, "
                   "phys_size = %u, "
                   "phys_erase_block = %u, "
                   "log_block_size = %u, "
                   "log_page_size = %u)",
                   self->fs.spifFs.cfg.phys_addr,
                   self->fs.spifFs.cfg.phys_size,
                   self->fs.spifFs.cfg.phys_erase_block,
                   self->fs.spifFs.cfg.log_block_size,
                   self->fs.spifFs.cfg.log_page_size);

    self->fs.spifFs.cfg.hal_read_f = storage_read;
    self->fs.spifFs.cfg.hal_write_f = storage_write;
    self->fs.spifFs.cfg.hal_erase_f = storage_erase;

    // These size calculations are taken from SPIFFS test code
    self->fs.spifFs.cacheSize = (cfg->format->spifFs.cachePages * (sizeof(
                                                                       spiffs_cache_page) +
                                                                   pageSz)) +
                                sizeof(spiffs_cache);

    self->fs.spifFs.cacheBuf = malloc(self->fs.spifFs.cacheSize);
    if (self->fs.spifFs.cacheBuf == NULL)
    {
        return OS_ERROR_INSUFFICIENT_SPACE;
    }

    self->fs.spifFs.workBuf = malloc(pageSz * 2);
    if (self->fs.spifFs.workBuf == NULL)
    {
        err = OS_ERROR_INSUFFICIENT_SPACE;
        goto err0;
    }

    self->fs.spifFs.fs.user_data = (void *)self;

    return OS_SUCCESS;

err0:
    free(self->fs.spifFs.cacheBuf);
    return err;
}

OS_Error_t
SpifFs_free(
    OS_FileSystem_Handle_t self)
{
    free(self->fs.spifFs.cacheBuf);
    free(self->fs.spifFs.workBuf);

    return OS_SUCCESS;
}

OS_Error_t
SpifFs_format(
    OS_FileSystem_Handle_t self)
{
    spiffs *fs = &self->fs.spifFs.fs;
    spiffs_config *cfg = &self->fs.spifFs.cfg;
    int rc;

    // SPIFFS_format needs to be called with an initalized spiffs structure,
    // the initialization of which happens in SPIFFS_mount.
    rc = SPIFFS_mount(fs, cfg, self->fs.spifFs.workBuf,
                      self->fs.spifFs.fds,
                      sizeof(self->fs.spifFs.fds),
                      self->fs.spifFs.cacheBuf,
                      self->fs.spifFs.cacheSize, NULL);

    if (rc == SPIFFS_OK)
    {
        SPIFFS_unmount(fs);
    }

    if ((rc = SPIFFS_format(fs)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_format() failed with %d", rc);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
SpifFs_mount(
    OS_FileSystem_Handle_t self)
{
    spiffs *fs = &self->fs.spifFs.fs;
    spiffs_config *cfg = &self->fs.spifFs.cfg;
    int rc;

    if ((rc = SPIFFS_mount(fs, cfg, self->fs.spifFs.workBuf,
                           self->fs.spifFs.fds,
                           sizeof(self->fs.spifFs.fds),
                           self->fs.spifFs.cacheBuf,
                           self->fs.spifFs.cacheSize, NULL)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_mount() failed with %d", rc);
        // If we have an ioError, return that; otherwise check if spiffs
        // complained about it not being a FS and then return NOT_FOUND;
        // otherwise return GENERIC.
        return (self->ioError != OS_SUCCESS)
                   ? self->ioError
               : (rc == SPIFFS_ERR_NOT_A_FS)
                   ? OS_ERROR_NOT_FOUND
                   : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
SpifFs_unmount(
    OS_FileSystem_Handle_t self)
{
    spiffs *fs = &self->fs.spifFs.fs;

    // SPIFFS_unmount does not return an error code.
    SPIFFS_unmount(fs);

    return OS_SUCCESS;
}

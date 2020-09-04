/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"
#include "OS_FileSystem_int.h"

#include "lib/LittleFs.h"
#include "lib/LittleFsFile.h"
#include "lib/FatFs.h"
#include "lib/FatFsFile.h"
#include "lib/SpifFs.h"
#include "lib/SpifFsFile.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// LittleFs callbacks
static const OS_FileSystem_FsOps_t littleFs_ops =
{
    .init       = LittleFs_init,
    .free       = LittleFs_free,
    .format     = LittleFs_format,
    .mount      = LittleFs_mount,
    .unmount    = LittleFs_unmount,
};
static const OS_FileSystem_FileOps_t littleFsFile_ops =
{
    .open       = LittleFsFile_open,
    .close      = LittleFsFile_close,
    .read       = LittleFsFile_read,
    .write      = LittleFsFile_write,
    .delete     = LittleFsFile_delete,
    .getSize    = LittleFsFile_getSize,
};

// FatFs callbacks
static const OS_FileSystem_FsOps_t fatFs_ops =
{
    .init       = FatFs_init,
    .free       = FatFs_free,
    .format     = FatFs_format,
    .mount      = FatFs_mount,
    .unmount    = FatFs_unmount,
};
static const OS_FileSystem_FileOps_t fatFsFile_ops =
{
    .open       = FatFsFile_open,
    .close      = FatFsFile_close,
    .read       = FatFsFile_read,
    .write      = FatFsFile_write,
    .delete     = FatFsFile_delete,
    .getSize    = FatFsFile_getSize,
};

// SpifFs callbacks
static const OS_FileSystem_FsOps_t spifFs_ops =
{
    .init       = SpifFs_init,
    .free       = SpifFs_free,
    .format     = SpifFs_format,
    .mount      = SpifFs_mount,
    .unmount    = SpifFs_unmount,
};
static const OS_FileSystem_FileOps_t spifFsFile_ops =
{
    .open       = SpifFsFile_open,
    .close      = SpifFsFile_close,
    .read       = SpifFsFile_read,
    .write      = SpifFsFile_write,
    .delete     = SpifFsFile_delete,
    .getSize    = SpifFsFile_getSize,
};

// Private Functions -----------------------------------------------------------
static inline bool
isInitParametersOk(
    OS_FileSystem_Handle_t*       self,
    const OS_FileSystem_Config_t* cfg)
{
    if (NULL == self || NULL == cfg)
    {
        return false;
    }
    if (OS_Dataport_isUnset(cfg->storage.dataport))
    {
        return false;
    }
    if (NULL == cfg->storage.erase ||
        NULL == cfg->storage.read ||
        NULL == cfg->storage.write ||
        NULL == cfg->storage.getState ||
        NULL == cfg->storage.getSize)
    {
        return false;
    }
    return true;
}


// Public Functions ------------------------------------------------------------

OS_Error_t
OS_FileSystem_init(
    OS_FileSystem_Handle_t*       self,
    const OS_FileSystem_Config_t* cfg)
{
    OS_Error_t err;
    OS_FileSystem_Handle_t fs;
    off_t sz;

    if (!isInitParametersOk(self, cfg))
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    // Get the size of the underlying storage for later
    if ((err = cfg->storage.getSize(&sz)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("getSize() failed with %d", err);
        return err;
    }

    if ((fs = calloc(1, sizeof(OS_FileSystem_t))) == NULL)
    {
        return OS_ERROR_INSUFFICIENT_SPACE;
    }

    fs->cfg = *cfg;

    // Check if a user passed a size; if it is zero, we just max out the
    // underlying storage. If it is non-zero, we need to check if it would fit.
    if (OS_FileSystem_USE_STORAGE_MAX == fs->cfg.size)
    {
        Debug_LOG_INFO(
            "Maximizing file system according to size reported by the storage "
            "layer (%" PRIiMAX " bytes)",
            sz);

        fs->cfg.size = sz;
    }
    else if (fs->cfg.size > sz)
    {
        Debug_LOG_ERROR(
            "Configured fileysten size (%" PRIiMAX " bytes) exceeds the size of "
            "the underlying storage (%" PRIiMAX " bytes)",
            fs->cfg.size, sz);

        err = OS_ERROR_INSUFFICIENT_SPACE;
        goto err0;
    }

    switch (cfg->type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        fs->fsOps   = &littleFs_ops;
        fs->fileOps = &littleFsFile_ops;
        break;
    case OS_FileSystem_Type_FATFS:
        fs->fsOps   = &fatFs_ops;
        fs->fileOps = &fatFsFile_ops;
        break;
    case OS_FileSystem_Type_SPIFFS:
        fs->fsOps   = &spifFs_ops;
        fs->fileOps = &spifFsFile_ops;
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        goto err0;
    }

    *self = fs;

    return fs->fsOps->init(fs);

err0:
    free(fs);
    return err;
}

OS_Error_t
OS_FileSystem_free(
    OS_FileSystem_Handle_t self)
{
    OS_Error_t err;

    if (NULL == self)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    err = self->fsOps->free(self);
    free(self);

    return err;
}

OS_Error_t
OS_FileSystem_format(
    OS_FileSystem_Handle_t self)
{
    return (NULL == self) ?
           OS_ERROR_INVALID_PARAMETER :
           self->fsOps->format(self);
}

OS_Error_t
OS_FileSystem_mount(
    OS_FileSystem_Handle_t self)
{
    return (NULL == self) ?
           OS_ERROR_INVALID_PARAMETER :
           self->fsOps->mount(self);
}

OS_Error_t
OS_FileSystem_unmount(
    OS_FileSystem_Handle_t self)
{
    return (NULL == self) ?
           OS_ERROR_INVALID_PARAMETER :
           self->fsOps->unmount(self);
}
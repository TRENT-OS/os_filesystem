/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

// For definition of OS_FileSystem, has to be included after the global header
#include "include/OS_FileSystem.h"

#include "include/lib/LittleFs.h"
#include "include/lib/FatFs.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Private Functions -----------------------------------------------------------

// Public Functions ------------------------------------------------------------

OS_Error_t
OS_FileSystem_init(
    OS_FileSystem_Handle_t*       self,
    const OS_FileSystem_Config_t* cfg)
{
    OS_Error_t err;
    OS_FileSystem_Handle_t fs;
    size_t sz;

    if (NULL == self || NULL == cfg)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (OS_Dataport_isUnset(cfg->storage.dataport))
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (NULL == cfg->storage.erase ||
        NULL == cfg->storage.read ||
        NULL == cfg->storage.write ||
        NULL == cfg->storage.getState ||
        NULL == cfg->storage.getSize)
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

    // Check if user passed as size; if it zero, we just max out the underlying
    // storage. If it is non-zero, we need to check if it would fit.
    if (OS_FileSystem_STORAGE_MAX == fs->cfg.size)
    {
        Debug_LOG_INFO("Maximizing file system according to size reported "
                       "by the storage layer (%zu bytes)", sz);
        fs->cfg.size = sz;
    }
    else if (fs->cfg.size > sz)
    {
        Debug_LOG_ERROR("Configured fileysten size (%zu bytes) exceeds "
                        "the size of the underlying storage (%zu bytes)",
                        fs->cfg.size, sz);
        err = OS_ERROR_INSUFFICIENT_SPACE;
        goto err0;
    }

    switch (cfg->type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        if ((err = LittleFs_init(fs)) != OS_SUCCESS)
        {
            goto err0;
        }
        break;
    case OS_FileSystem_Type_FATFS:
        if ((err = FatFs_init(fs)) != OS_SUCCESS)
        {
            goto err0;
        }
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        goto err0;
    }

    *self = fs;

    return OS_SUCCESS;

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

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        err = LittleFs_free(self);
        break;
    case OS_FileSystem_Type_FATFS:
        err = FatFs_free(self);
        break;
    default:
        err = OS_ERROR_GENERIC;;
        break;
    }

    free(self);

    return err;
}

OS_Error_t
OS_FileSystem_format(
    OS_FileSystem_Handle_t self)
{
    if (NULL == self)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        return LittleFs_format(self);
    case OS_FileSystem_Type_FATFS:
        return FatFs_format(self);
    default:
        break;
    }

    return OS_ERROR_GENERIC;
}

OS_Error_t
OS_FileSystem_mount(
    OS_FileSystem_Handle_t self)
{
    if (NULL == self)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        return LittleFs_mount(self);
    case OS_FileSystem_Type_FATFS:
        return FatFs_mount(self);
    default:
        break;
    }

    return OS_ERROR_GENERIC;
}

OS_Error_t
OS_FileSystem_unmount(
    OS_FileSystem_Handle_t self)
{
    if (NULL == self)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        return LittleFs_unmount(self);
    case OS_FileSystem_Type_FATFS:
        return FatFs_unmount(self);
    default:
        break;
    }

    return OS_ERROR_GENERIC;
}

OS_Error_t
OS_FileSystem_wipe(
    OS_FileSystem_Handle_t self)
{
    if (NULL == self)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        return LittleFs_wipe(self);
    case OS_FileSystem_Type_FATFS:
        return FatFs_wipe(self);
    default:
        break;
    }

    return OS_ERROR_GENERIC;
}
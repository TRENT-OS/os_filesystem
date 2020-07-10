/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

// For definition of OS_FileSystem, has to be included after the global header
#include "include/OS_FileSystem.h"

#include "include/lib/LittleFsFile.h"
#include "include/lib/FatFsFile.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Private Functions -----------------------------------------------------------

static OS_FileSystemFile_Handle_t
fileHandle_findFree(
    OS_FileSystem_Handle_t self)
{
    size_t i;

    for (i = 0; i < MAX_FILE_HANDLES; i++)
    {
        if (!self->inUse[i])
        {
            return i;
        }
    }

    return -1;
}

static void
fileHandle_take(
    OS_FileSystem_Handle_t           self,
    const OS_FileSystemFile_Handle_t hFile)
{
    self->inUse[hFile] = true;
}

static void
fileHandle_release(
    OS_FileSystem_Handle_t           self,
    const OS_FileSystemFile_Handle_t hFile)
{
    self->inUse[hFile] = false;
}

static bool
fileHandle_inUse(
    OS_FileSystem_Handle_t           self,
    const OS_FileSystemFile_Handle_t hFile)
{
    return self->inUse[hFile];
}

static bool
fileHandle_isValid(
    OS_FileSystem_Handle_t           self,
    const OS_FileSystemFile_Handle_t hFile)
{
    return hFile >= 0 && hFile < MAX_FILE_HANDLES;
}

// Public Functions ------------------------------------------------------------

OS_Error_t
OS_FileSystemFile_open(
    OS_FileSystem_Handle_t          self,
    OS_FileSystemFile_Handle_t*     hFile,
    const char*                     name,
    const OS_FileSystem_OpenMode_t  mode,
    const OS_FileSystem_OpenFlags_t flags)
{
    OS_Error_t err;

    if (NULL == self || NULL == hFile || NULL == name)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (!(mode == OS_FileSystem_OpenMode_RDONLY ||
          mode == OS_FileSystem_OpenMode_WRONLY ||
          mode == OS_FileSystem_OpenMode_RDWR))
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    if ((*hFile = fileHandle_findFree(self)) < 0)
    {
        Debug_LOG_ERROR("All file handles are in use");
        return OS_ERROR_NOT_FOUND;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        err = LittleFsFile_open(self, *hFile, name, mode, flags);
        break;
    case OS_FileSystem_Type_FATFS:
        err = FatFsFile_open(self, *hFile, name, mode, flags);
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        break;
    }

    // Mark file ID as "in use"
    if (err == OS_SUCCESS)
    {
        fileHandle_take(self, *hFile);
    }

    return err;
}

OS_Error_t
OS_FileSystemFile_close(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile)
{
    OS_Error_t err;

    if (NULL == self)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (!fileHandle_isValid(self, hFile) || !fileHandle_inUse(self, hFile))
    {
        return OS_ERROR_INVALID_HANDLE;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        err = LittleFsFile_close(self, hFile);
        break;
    case OS_FileSystem_Type_FATFS:
        err = FatFsFile_close(self, hFile);
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        break;
    }

    // Release file ID
    if (err == OS_SUCCESS)
    {
        fileHandle_release(self, hFile);
    }

    return err;
}

OS_Error_t
OS_FileSystemFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    void*                      buffer)
{
    OS_Error_t err;

    if (NULL == self || NULL == buffer)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (!fileHandle_isValid(self, hFile) || !fileHandle_inUse(self, hFile))
    {
        return OS_ERROR_INVALID_HANDLE;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        err = LittleFsFile_read(self, hFile, offset, len, buffer);
        break;
    case OS_FileSystem_Type_FATFS:
        err = FatFsFile_read(self, hFile, offset, len, buffer);
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        break;
    }

    return err;
}

OS_Error_t
OS_FileSystemFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    const void*                buffer)
{
    OS_Error_t err;

    if (NULL == self || NULL == buffer)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (!fileHandle_isValid(self, hFile) || !fileHandle_inUse(self, hFile))
    {
        return OS_ERROR_INVALID_HANDLE;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        err = LittleFsFile_write(self, hFile, offset, len, buffer);
        break;
    case OS_FileSystem_Type_FATFS:
        err = FatFsFile_write(self, hFile, offset, len, buffer);
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        break;
    }

    return err;
}

OS_Error_t
OS_FileSystemFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name)
{
    OS_Error_t err;

    if (NULL == self || NULL == name)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        err = LittleFsFile_delete(self, name);
        break;
    case OS_FileSystem_Type_FATFS:
        err = FatFsFile_delete(self, name);
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        break;
    }

    return err;
}

OS_Error_t
OS_FileSystemFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    size_t*                sz)
{
    OS_Error_t err;

    if (NULL == self || NULL == name || NULL == sz)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    switch (self->cfg.type)
    {
    case OS_FileSystem_Type_LITTLEFS:
        err = LittleFsFile_getSize(self, name, sz);
        break;
    case OS_FileSystem_Type_FATFS:
        err = FatFsFile_getSize(self, name, sz);
        break;
    default:
        err = OS_ERROR_INVALID_PARAMETER;
        break;
    }

    return err;
}
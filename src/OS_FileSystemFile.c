/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Private Functions -----------------------------------------------------------

static OS_FileSystemFile_Handle_t
fileHandle_findFree(
    OS_FileSystem_Handle_t self)
{
    UsageBitField_t u = self->usageBitField;

    for (size_t i = 0; i < MAX_FILE_HANDLES; i++)
    {
        if ((u & 1ULL) == 0)
        {
            // The file handles are array indizes and so we can simply return
            // the loop-index of the first unused file handle.
            return i;
        }
        u >>= 1;
    }

    return MAX_FILE_HANDLES;
}

static void
fileHandle_take(
    OS_FileSystem_Handle_t           self,
    const OS_FileSystemFile_Handle_t hFile)
{
    self->usageBitField |= (1ULL << hFile);
}

static void
fileHandle_release(
    OS_FileSystem_Handle_t           self,
    const OS_FileSystemFile_Handle_t hFile)
{
    self->usageBitField &= ~(1ULL << hFile);
}

static bool
fileHandle_inUse(
    OS_FileSystem_Handle_t           self,
    const OS_FileSystemFile_Handle_t hFile)
{
    return (self->usageBitField & (1ULL << hFile));
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

    if ((*hFile = fileHandle_findFree(self)) >= MAX_FILE_HANDLES)
    {
        Debug_LOG_ERROR("All file handles are in use");
        return OS_ERROR_OUT_OF_BOUNDS;
    }

    if ((err = self->fileOps->open(self, *hFile, name, mode, flags)) == OS_SUCCESS)
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

    if ((err = self->fileOps->close(self, hFile)) == OS_SUCCESS)
    {
        fileHandle_release(self, hFile);
    }

    return err;
}

OS_Error_t
OS_FileSystemFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    void*                      buffer)
{
    if (NULL == self || NULL == buffer)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (!fileHandle_isValid(self, hFile) || !fileHandle_inUse(self, hFile))
    {
        return OS_ERROR_INVALID_HANDLE;
    }

    return self->fileOps->read(self, hFile, offset, len, buffer);
}

OS_Error_t
OS_FileSystemFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    const void*                buffer)
{
    if (NULL == self || NULL == buffer)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (!fileHandle_isValid(self, hFile) || !fileHandle_inUse(self, hFile))
    {
        return OS_ERROR_INVALID_HANDLE;
    }

    return self->fileOps->write(self, hFile, offset, len, buffer);
}

OS_Error_t
OS_FileSystemFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name)
{
    if (NULL == self || NULL == name)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    return self->fileOps->delete (self, name);
}

OS_Error_t
OS_FileSystemFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    off_t*                 sz)
{
    if (NULL == self || NULL == name || NULL == sz)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    return self->fileOps->getSize(self, name, sz);
}
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

#include <inttypes.h>

// Private Functions -----------------------------------------------------------

// Public Functions ------------------------------------------------------------

OS_Error_t
SpifFsFile_open(
    OS_FileSystem_Handle_t          self,
    OS_FileSystemFile_Handle_t      hFile,
    const char*                     name,
    const OS_FileSystem_OpenMode_t  mode,
    const OS_FileSystem_OpenFlags_t flags)
{
    spiffs* fs = &self->fs.spifFs.fs;
    spiffs_file* file = &self->fs.spifFs.fh[hFile];
    uint32_t oflags;

    switch (mode)
    {
    case OS_FileSystem_OpenMode_RDONLY:
        oflags = SPIFFS_O_RDONLY;
        break;
    case OS_FileSystem_OpenMode_WRONLY:
        oflags = SPIFFS_O_WRONLY;
        break;
    case OS_FileSystem_OpenMode_RDWR:
        oflags = SPIFFS_O_RDWR;
        break;
    default:
        return OS_ERROR_INVALID_PARAMETER;
    }

    if (flags & OS_FileSystem_OpenFlags_CREATE)
    {
        oflags |= SPIFFS_O_CREAT;
    }
    if (flags & OS_FileSystem_OpenFlags_EXCLUSIVE)
    {
        oflags |= SPIFFS_O_EXCL;
    }
    if (flags & OS_FileSystem_OpenFlags_TRUNCATE)
    {
        oflags |= SPIFFS_O_TRUNC;
    }

    if ((*file = SPIFFS_open(fs, name, oflags, 0)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_open() failed with %d", *file);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
SpifFsFile_close(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile)
{
    spiffs* fs = &self->fs.spifFs.fs;
    spiffs_file* file = &self->fs.spifFs.fh[hFile];
    int rc;

    if ((rc = SPIFFS_close(fs, *file)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_close() failed with %d", rc);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
SpifFsFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    void*                      buffer)
{
    spiffs* fs = &self->fs.spifFs.fs;
    spiffs_file* file = &self->fs.spifFs.fh[hFile];
    ssize_t sz, pos;

    if ((pos = SPIFFS_lseek(fs, *file, offset, SPIFFS_SEEK_SET)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_lseek() failed with %zd", pos);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_ABORTED;
    }
    if (pos != offset)
    {
        Debug_LOG_ERROR(
            "SPIFFS_lseek() jumped to offset %zd instead of offset %" PRIiMAX,
            pos, offset);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_ABORTED;
    }
    if ((sz = SPIFFS_read(fs, *file, buffer, len)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_read() failed with %zd", sz);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }
    if (sz != len)
    {
        Debug_LOG_ERROR("SPIFFS_read() read %zd bytes instead of %zu bytes",
                        sz, len);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
SpifFsFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    const void*                buffer)
{
    spiffs* fs = &self->fs.spifFs.fs;
    spiffs_file* file = &self->fs.spifFs.fh[hFile];
    ssize_t sz, pos;

    if ((pos = SPIFFS_lseek(fs, *file, offset, SPIFFS_SEEK_SET)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_lseek() failed with %zd", pos);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_ABORTED;
    }
    if (pos != offset)
    {
        Debug_LOG_ERROR(
            "SPIFFS_lseek() jumped to offset %zd instead of offset %" PRIiMAX,
            pos, offset);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_ABORTED;
    }

    if ((sz = SPIFFS_write(fs, *file, (void*) buffer, len)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_write() failed with %zd", sz);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }
    if (sz != len)
    {
        Debug_LOG_ERROR("SPIFFS_write() wrote %zd bytes instead of %zu bytes",
                        sz, len);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
SpifFsFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name)
{
    spiffs* fs = &self->fs.spifFs.fs;
    int rc;

    if ((rc = SPIFFS_remove(fs, name)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_remove() failed with %d", rc);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
SpifFsFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    off_t*                 sz)
{
    spiffs* fs = &self->fs.spifFs.fs;
    spiffs_stat stat;
    int rc;

    if ((rc = SPIFFS_stat(fs, name, &stat)) < 0)
    {
        Debug_LOG_ERROR("SPIFFS_stat() failed with %d", rc);
        return (self->ioError != OS_SUCCESS) ? self->ioError : OS_ERROR_GENERIC;
    }

    *sz = stat.size;

    return OS_SUCCESS;
}
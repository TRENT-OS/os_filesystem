/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"
#include "OS_FileSystem_int.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

#include "lfs.h"

#include <stddef.h>
#include <stdint.h>

// Private Functions -----------------------------------------------------------

OS_Error_t
LittleFsFile_open(
    OS_FileSystem_Handle_t          self,
    OS_FileSystemFile_Handle_t      hFile,
    const char*                     name,
    const OS_FileSystem_OpenMode_t  mode,
    const OS_FileSystem_OpenFlags_t flags)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    lfs_file_t* fh = &self->fs.littleFs.fh[hFile];
    uint32_t oflags;
    int rc;

    switch (mode)
    {
    case OS_FileSystem_OpenMode_RDONLY:
        oflags = LFS_O_RDONLY;
        break;
    case OS_FileSystem_OpenMode_WRONLY:
        oflags = LFS_O_WRONLY;
        break;
    case OS_FileSystem_OpenMode_RDWR:
        oflags = LFS_O_RDWR;
        break;
    default:
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (flags & OS_FileSystem_OpenFlags_CREATE)
    {
        oflags |= LFS_O_CREAT;
    }
    if (flags & OS_FileSystem_OpenFlags_EXCLUSIVE)
    {
        oflags |= LFS_O_EXCL;
    }
    if (flags & OS_FileSystem_OpenFlags_TRUNCATE)
    {
        oflags |= LFS_O_TRUNC;
    }

    if ((rc = lfs_file_open(fs, fh, name, oflags)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_open() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
LittleFsFile_close(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    lfs_file_t* fh = &self->fs.littleFs.fh[hFile];
    int rc;

    if ((rc = lfs_file_close(fs, fh)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_close() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
LittleFsFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    void*                      buffer)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    lfs_file_t* fh = &self->fs.littleFs.fh[hFile];
    lfs_ssize_t sz;
    lfs_soff_t off;

    if ((off = lfs_file_seek(fs, fh, offset, LFS_SEEK_SET)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_seek() failed with %i", off);
        return OS_ERROR_ABORTED;
    }
    if (off != offset)
    {
        Debug_LOG_ERROR("lfs_file_seek() jumped to offset %i instead of offset %zu",
                        off, offset);
    }

    if ((sz = lfs_file_read(fs, fh, buffer, len)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_read() failed with %i", sz);
        return OS_ERROR_ABORTED;
    }
    if (sz != len)
    {
        Debug_LOG_ERROR("lfs_file_read() read %i bytes instead of %zu bytes",
                        sz, len);
    }

    return OS_SUCCESS;
}

OS_Error_t
LittleFsFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    const void*                buffer)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    lfs_file_t* fh = &self->fs.littleFs.fh[hFile];
    lfs_ssize_t sz;
    lfs_soff_t off;

    if ((off = lfs_file_seek(fs, fh, offset, LFS_SEEK_SET)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_seek() failed with %i", off);
        return OS_ERROR_ABORTED;
    }
    if (off != offset)
    {
        Debug_LOG_ERROR("lfs_file_seek() jumped to offset %i instead of offset %zu",
                        off, offset);
    }

    if ((sz = lfs_file_write(fs, fh, buffer, len)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_write() failed with %i", sz);
        return OS_ERROR_ABORTED;
    }
    if (sz != len)
    {
        Debug_LOG_ERROR("lfs_file_write() wrote %i bytes instead of %zu bytes",
                        sz, len);
    }

    return OS_SUCCESS;
}

OS_Error_t
LittleFsFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    int rc;

    if ((rc = lfs_remove(fs, name)) < 0)
    {
        Debug_LOG_ERROR("lfs_remove() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
LittleFsFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    size_t*                sz)
{
    lfs_t* fs = &self->fs.littleFs.fs;
    lfs_file_t fh;
    int rc;

    if ((rc = lfs_file_open(fs, &fh, name, LFS_O_RDONLY)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_open() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }
    *sz = lfs_file_size(fs, &fh);
    if ((rc = lfs_file_close(fs, &fh)) < 0)
    {
        Debug_LOG_ERROR("lfs_file_close() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}
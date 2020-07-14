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

// Private Functions -----------------------------------------------------------

// Public Functions ------------------------------------------------------------

OS_Error_t
FatFsFile_open(
    OS_FileSystem_Handle_t          self,
    OS_FileSystemFile_Handle_t      hFile,
    const char*                     name,
    const OS_FileSystem_OpenMode_t  mode,
    const OS_FileSystem_OpenFlags_t flags)
{
    FCTX* fctx = &self->fs.fatFs.fctx;
    FIL* fh = &self->fs.fatFs.fh[hFile];
    BYTE oflags;
    FRESULT rc;

    switch (mode)
    {
    case OS_FileSystem_OpenMode_RDONLY:
        oflags = FA_READ;
        break;
    case OS_FileSystem_OpenMode_WRONLY:
        oflags = FA_WRITE;
        break;
    case OS_FileSystem_OpenMode_RDWR:
        oflags = FA_WRITE | FA_READ;
        break;
    default:
        return OS_ERROR_INVALID_PARAMETER;
    }
    if (flags & OS_FileSystem_OpenFlags_CREATE)
    {
        oflags |= FA_CREATE_ALWAYS;
    }
    if (flags & OS_FileSystem_OpenFlags_APPEND)
    {
        oflags |= FA_OPEN_APPEND;
    }
    if (flags & OS_FileSystem_OpenFlags_EXCLUSIVE ||
        flags & OS_FileSystem_OpenFlags_TRUNCATE)
    {
        return OS_ERROR_NOT_SUPPORTED;
    }

    if ((rc = f_open(fctx, fh, name, oflags)) != FR_OK)
    {
        Debug_LOG_ERROR("f_open() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
FatFsFile_close(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile)
{
    FCTX* fctx = &self->fs.fatFs.fctx;
    FIL* fh = &self->fs.fatFs.fh[hFile];
    FRESULT rc;

    if ((rc = f_close(fctx, fh)) != FR_OK)
    {
        Debug_LOG_ERROR("f_close() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
FatFsFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    void*                      buffer)
{
    FCTX* fctx = &self->fs.fatFs.fctx;
    FIL* fh = &self->fs.fatFs.fh[hFile];
    UINT read;
    FRESULT rc;

    if ((rc = f_lseek(fctx, fh, offset)) != FR_OK)
    {
        Debug_LOG_ERROR("f_lseek() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }
    if ((rc = f_read(fctx, fh, buffer, len, &read)) != FR_OK)
    {
        Debug_LOG_ERROR("f_read() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
FatFsFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    const void*                buffer)
{
    FCTX* fctx = &self->fs.fatFs.fctx;
    FIL* fh = &self->fs.fatFs.fh[hFile];
    UINT written;
    FRESULT rc;

    if ((rc = f_lseek(fctx, fh, offset)) != FR_OK)
    {
        Debug_LOG_ERROR("f_lseek() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }
    if ((rc = f_write(fctx, fh, buffer, len, &written)) != FR_OK)
    {
        Debug_LOG_ERROR("f_write() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
FatFsFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name)
{
    FCTX* fctx = &self->fs.fatFs.fctx;
    FRESULT rc;

    if ((rc = f_unlink(fctx, name)) != FR_OK)
    {
        Debug_LOG_ERROR("f_unlink() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

OS_Error_t
FatFsFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    size_t*                sz)
{
    FCTX* fctx = &self->fs.fatFs.fctx;
    FILINFO fno;
    FRESULT rc;

    if ((rc = f_stat(fctx, name, &fno)) != FR_OK)
    {
        Debug_LOG_ERROR("f_stat() failed with %i", rc);
        return OS_ERROR_ABORTED;
    }

    *sz = fno.fsize;

    return OS_SUCCESS;
}
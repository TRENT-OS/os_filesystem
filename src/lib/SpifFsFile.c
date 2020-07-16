/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"
#include "OS_FileSystem_int.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

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
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFsFile_close(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFsFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    void*                      buffer)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFsFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const size_t               offset,
    const size_t               len,
    const void*                buffer)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFsFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFsFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    size_t*                sz)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

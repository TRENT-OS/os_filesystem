/*
 * Copyright (C) 2020, HENSOLDT Cyber GmbH
 */

#pragma once

#include "OS_FileSystem.h"

OS_Error_t
SpifFsFile_open(
    OS_FileSystem_Handle_t          self,
    OS_FileSystemFile_Handle_t      hFile,
    const char*                     name,
    const OS_FileSystem_OpenMode_t  mode,
    const OS_FileSystem_OpenFlags_t flags);

OS_Error_t
SpifFsFile_close(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile);

OS_Error_t
SpifFsFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    void*                      buffer);

OS_Error_t
SpifFsFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    const void*                buffer);

OS_Error_t
SpifFsFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name);

OS_Error_t
SpifFsFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    off_t*                 sz);

/*
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "OS_FileSystem.h"

OS_Error_t
LittleFsFile_open(
    OS_FileSystem_Handle_t          self,
    OS_FileSystemFile_Handle_t      hFile,
    const char*                     name,
    const OS_FileSystem_OpenMode_t  mode,
    const OS_FileSystem_OpenFlags_t flags);

OS_Error_t
LittleFsFile_close(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile);

OS_Error_t
LittleFsFile_read(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    void*                      buffer);

OS_Error_t
LittleFsFile_write(
    OS_FileSystem_Handle_t     self,
    OS_FileSystemFile_Handle_t hFile,
    const off_t                offset,
    const size_t               len,
    const void*                buffer);

OS_Error_t
LittleFsFile_delete(
    OS_FileSystem_Handle_t self,
    const char*            name);

OS_Error_t
LittleFsFile_getSize(
    OS_FileSystem_Handle_t self,
    const char*            name,
    off_t*                 sz);

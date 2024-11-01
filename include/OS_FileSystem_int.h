/*
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "OS_FileSystem.h"

// For LittleFS
#include "lfs.h"

// For FatFs
#include "ff.h"
#include "diskio.h"

// For SpifFs
#include "spiffs.h"
#include "spiffs_nucleus.h"

typedef struct
{
    OS_Error_t (*init) (OS_FileSystem_Handle_t  self);
    OS_Error_t (*free) (OS_FileSystem_Handle_t self);
    OS_Error_t (*format) (OS_FileSystem_Handle_t self);
    OS_Error_t (*mount) (OS_FileSystem_Handle_t self);
    OS_Error_t (*unmount) (OS_FileSystem_Handle_t self);
    OS_Error_t (*wipe) (OS_FileSystem_Handle_t self);
} OS_FileSystem_FsOps_t;

typedef struct
{
    OS_Error_t (*open) (OS_FileSystem_Handle_t          self,
                        OS_FileSystemFile_Handle_t      hFile,
                        const char*                     name,
                        const OS_FileSystem_OpenMode_t  mode,
                        const OS_FileSystem_OpenFlags_t flags);
    OS_Error_t (*close)(OS_FileSystem_Handle_t     self,
                        OS_FileSystemFile_Handle_t hFile);
    OS_Error_t (*read) (OS_FileSystem_Handle_t     self,
                        OS_FileSystemFile_Handle_t hFile,
                        const off_t                offset,
                        const size_t               len,
                        void*                      buffer);
    OS_Error_t (*write)(OS_FileSystem_Handle_t     self,
                        OS_FileSystemFile_Handle_t hFile,
                        const off_t                offset,
                        const size_t               len,
                        const void*                buffer);
    OS_Error_t (*delete)(OS_FileSystem_Handle_t self,
                         const char*            name);
    OS_Error_t (*getSize)(OS_FileSystem_Handle_t self,
                          const char*            name,
                          off_t*                 sz);
} OS_FileSystem_FileOps_t;

/*
 * Type of the usage bit-field.
 * Each open file handle is represented by a bit in the usage bit-field.
 */
typedef uint64_t UsageBitField_t;

/*
 * Maximum number of file handles.
 * The possible number of file handles is limited by the bit-width of the data
 * type used for the usage bit-field.
 */
#define MAX_FILE_HANDLES    (sizeof(UsageBitField_t) * 8)

// Hidden definition of struct
struct OS_FileSystem
{
    const OS_FileSystem_FsOps_t* fsOps;
    const OS_FileSystem_FileOps_t* fileOps;
    OS_FileSystem_Config_t cfg;
    OS_Error_t ioError;
    union
    {
        struct
        {
            lfs_t fs;
            struct lfs_config cfg;
            lfs_file_t fh[MAX_FILE_HANDLES];
        } littleFs;
        struct
        {
            DIO dio;
            FCTX fctx;
            FATFS fs;
            FIL fh[MAX_FILE_HANDLES];
            uint8_t buffer[FF_MAX_SS];
        } fatFs;
        struct
        {
            spiffs fs;
            spiffs_config cfg;
            spiffs_file fh[MAX_FILE_HANDLES];
            uint8_t fds[MAX_FILE_HANDLES * sizeof(spiffs_fd)];
            uint8_t* workBuf;
            uint8_t* cacheBuf;
            size_t cacheSize;
        } spifFs;
    } fs;
    UsageBitField_t usageBitField;
};

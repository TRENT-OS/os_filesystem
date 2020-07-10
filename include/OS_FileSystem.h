/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#pragma once

/*
 * Include this file only after including OS_FileSystem from the API headers;
 * and only use this internally to resolve anonymous structs..
 */

// For LittleFS
#include "lfs.h"

// For FatFs
#include "ff.h"
#include "diskio.h"

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
                        const size_t               offset,
                        const size_t               len,
                        void*                      buffer);
    OS_Error_t (*write)(OS_FileSystem_Handle_t     self,
                        OS_FileSystemFile_Handle_t hFile,
                        const size_t               offset,
                        const size_t               len,
                        const void*                buffer);
    OS_Error_t (*delete)(OS_FileSystem_Handle_t self,
                         const char*            name);
    OS_Error_t (*getSize)(OS_FileSystem_Handle_t self,
                          const char*            name,
                          size_t*                sz);
} OS_FileSystem_FileOps_t;

// Amount of file handles we can have open at a time
#define MAX_FILE_HANDLES    32

// Hidden definition of struct
struct OS_FileSystem
{
    const OS_FileSystem_FsOps_t* fsOps;
    const OS_FileSystem_FileOps_t* fileOps;
    OS_FileSystem_Config_t cfg;
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
    } fs;
    // Array for keeping track which file handles are in use
    bool inUse[MAX_FILE_HANDLES];
};
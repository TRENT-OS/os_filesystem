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

// Amount of file handles we can have open at a time
#define MAX_FILE_HANDLES    32

// Hidden definition of struct
struct OS_FileSystem
{
    OS_FileSystem_Config_t cfg;
    union {
        struct {
            lfs_t fs;
            struct lfs_config cfg;
            lfs_file_t fh[MAX_FILE_HANDLES];
        } littleFs;
    } fs;
    // Array for keeping track which file handles are in use
    bool inUse[MAX_FILE_HANDLES];
};
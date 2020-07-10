/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#pragma once

#include "OS_FileSystem.h"

OS_Error_t
LittleFs_init(
    OS_FileSystem_Handle_t self);

OS_Error_t
LittleFs_free(
    OS_FileSystem_Handle_t self);

OS_Error_t
LittleFs_format(
    OS_FileSystem_Handle_t self);

OS_Error_t
LittleFs_mount(
    OS_FileSystem_Handle_t self);

OS_Error_t
LittleFs_unmount(
    OS_FileSystem_Handle_t self);

OS_Error_t
LittleFs_wipe(
    OS_FileSystem_Handle_t self);
/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#pragma once

#include "OS_FileSystem.h"

OS_Error_t
SpifFs_init(
    OS_FileSystem_Handle_t self);

OS_Error_t
SpifFs_free(
    OS_FileSystem_Handle_t self);

OS_Error_t
SpifFs_format(
    OS_FileSystem_Handle_t self);

OS_Error_t
SpifFs_mount(
    OS_FileSystem_Handle_t self);

OS_Error_t
SpifFs_unmount(
    OS_FileSystem_Handle_t self);
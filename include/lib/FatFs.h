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
FatFs_init(
    OS_FileSystem_Handle_t self);

OS_Error_t
FatFs_free(
    OS_FileSystem_Handle_t self);

OS_Error_t
FatFs_format(
    OS_FileSystem_Handle_t self);

OS_Error_t
FatFs_mount(
    OS_FileSystem_Handle_t self);

OS_Error_t
FatFs_unmount(
    OS_FileSystem_Handle_t self);
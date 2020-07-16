/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"
#include "OS_FileSystem_int.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

#include <string.h>

// Private Functions -----------------------------------------------------------

// Public Functions ------------------------------------------------------------

OS_Error_t
SpifFs_init(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFs_free(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFs_format(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFs_mount(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
SpifFs_unmount(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}
/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

// For definition of OS_FileSystem, has to be included after the global header
#include "include/OS_FileSystem.h"

#if defined(OS_FILESYSTEM_REMOVE_DEBUG_LOGGING)
#undef Debug_Config_PRINT_TO_LOG_SERVER
#endif
#include "LibDebug/Debug.h"

// Private Functions -----------------------------------------------------------

// Public Functions ------------------------------------------------------------

OS_Error_t
LittleFs_init(
    OS_FileSystem_Handle_t  self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
LittleFs_free(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
LittleFs_format(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
LittleFs_mount(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
LittleFs_unmount(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

OS_Error_t
LittleFs_wipe(
    OS_FileSystem_Handle_t self)
{
    return OS_ERROR_NOT_IMPLEMENTED;
}

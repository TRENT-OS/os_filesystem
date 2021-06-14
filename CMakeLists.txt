#
# OS Filesystem library
#
# Copyright (C) 2019-2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.13.0)

# enable new behavior introduced in CMake 3.13, where relative paths will be
# converted into absolute paths in target_sources(INTERFACE ....)
cmake_policy(SET CMP0076 NEW)

#------------------------------------------------------------------------------

project(os_filesystem C)

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME}
    INTERFACE
        src/OS_FileSystem.c
        src/OS_FileSystemFile.c
        src/lib/LittleFs.c
        src/lib/LittleFsFile.c
        src/lib/SpifFs.c
        src/lib/SpifFsFile.c
        src/lib/FatFs.c
        src/lib/FatFsFile.c
        3rdParty/littlefs/src/lfs.c
        3rdParty/littlefs/src/lfs_util.c
        3rdParty/fatfs/src/ff.c
        3rdParty/fatfs/src/ffsystem.c
        3rdParty/fatfs/src/ffunicode.c
        3rdParty/spiffs/src/spiffs_cache.c
        3rdParty/spiffs/src/spiffs_gc.c
        3rdParty/spiffs/src/spiffs_nucleus.c
        3rdParty/spiffs/src/spiffs_check.c
        3rdParty/spiffs/src/spiffs_hydrogen.c
)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        include
        3rdParty/littlefs/include
        3rdParty/fatfs/include
        3rdParty/spiffs/include
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        lib_debug
        os_core_api
)

target_compile_options(${PROJECT_NAME}
    INTERFACE
       -Wall
       -Werror
)

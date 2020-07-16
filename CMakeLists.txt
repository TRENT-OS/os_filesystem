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
)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        include
        3rdParty/littlefs/include
        3rdParty/fatfs/include
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        os_core_api
)

target_compile_options(${PROJECT_NAME}
    INTERFACE
       -Wall
       -Werror
)


#-------------------------------------------------------------------------------
# if invoked from the OS sandbox, provide a documentation build target
if (COMMAND os_create_doxygen_target)

    get_target_property(
        OS_CORE_API_INCLUDES
        os_core_api
        INTERFACE_INCLUDE_DIRECTORIES)

    find_file(
        OS_FILESYSTEM_API_HEADER
        OS_FileSystem.h
        PATHS ${OS_CORE_API_INCLUDES}
        CMAKE_FIND_ROOT_PATH_BOTH)

    os_create_doxygen_target( os_filesystem_doc
        # pre-step: link the API file OS_FileSystem.h from os_core_api in
        #           order to produce a complete doxygen documentation
        "ln -sf ${OS_FILESYSTEM_API_HEADER} ${CMAKE_CURRENT_SOURCE_DIR}/include"
        # post-step: remove the link to OS_FileSystem.h
        "rm -f ${CMAKE_CURRENT_SOURCE_DIR}/include/OS_FileSystem.h"
    )

endif()

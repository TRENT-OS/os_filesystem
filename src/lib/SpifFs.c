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


#include "spiffs.h"
#include "spiffs_config.h"


/* Defines -------------------------------------------------------------------*/

#define PHYSICAL_ERASE_BLOCK      0x1000
#define LOG_PAGE_SIZE             0x100
#define LOG_BLOCK_SIZE            PHYSICAL_ERASE_BLOCK

#if LOG_PAGE_SIZE != 256
//
// struct OS_FileSystem.spifFs.spiffs_work_buf has to be resized accordingly
//
#error LOG_PAGE_SIZE expected to be 256
#endif


/* Private variables ---------------------------------------------------------*/

static const OS_FileSystem_Format_t SpifFs_defaultConfig =
{
  // the page size has to be smaller than the block size
  .spifFs = {
    .physEraseBlock = 0x1000,
    .logBlockSize   = 0x1000,
    .logPageSize    =  0x100,
  }
};


// Private Functions -----------------------------------------------------------


static int32_t spiffsRead(struct spiffs_t *fs, uint32_t addr, uint32_t size,
                          uint8_t* dst)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t)     fs->user_data;
    OS_Error_t err;
    size_t read;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        return OS_ERROR_BUFFER_TOO_SMALL;
    }

    if ((err = self->cfg.storage.read(addr, size, &read)) != OS_SUCCESS)
    {
        return err;
    }

    if (read != size)
    {
        return OS_ERROR_ABORTED;
    }

    memcpy(dst, OS_Dataport_getBuf(self->cfg.storage.dataport), read);

    return OS_SUCCESS;
}

static int32_t spiffsWrite(struct spiffs_t *fs, uint32_t addr, uint32_t size,
                           uint8_t* src)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t)     fs->user_data;
    OS_Error_t err;
    size_t written;

    if (size > OS_Dataport_getSize(self->cfg.storage.dataport))
    {
        return OS_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(OS_Dataport_getBuf(self->cfg.storage.dataport), src, size);

    if ((err = self->cfg.storage.write(addr, size, &written)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("write() failed with %d", err);
        return err;
    }

    if (written != size)
    {
        Debug_LOG_ERROR("write() requested to write %d bytes but got %zu bytes",
                        size, written);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}

static int32_t spiffsErase(struct spiffs_t *fs, uint32_t addr, uint32_t size)
{
    OS_FileSystem_Handle_t self = (OS_FileSystem_Handle_t) fs->user_data;
    OS_Error_t err;
    size_t erased;

    if ((err = self->cfg.storage.erase(addr, size, &erased)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("erase() failed with %d", err);
        return err;
    }

    if (erased != size)
    {
        Debug_LOG_ERROR("erase() requested to erase %zu bytes but erased %zu bytes",
                        size, erased);
        return OS_ERROR_ABORTED;
    }

    return OS_SUCCESS;
}


// Public Functions ------------------------------------------------------------

OS_Error_t
SpifFs_init(
    OS_FileSystem_Handle_t self)
{

  OS_FileSystem_Config_t* cfg = &self->cfg;

    if  (NULL == cfg->format)
    {
        cfg->format = &SpifFs_defaultConfig;
    }

    self->fs.spifFs.cfg.phys_size        = cfg->size;
    self->fs.spifFs.cfg.phys_addr        = 0;
    self->fs.spifFs.cfg.phys_erase_block = cfg->format->spifFs.physEraseBlock;
    self->fs.spifFs.cfg.log_block_size   = cfg->format->spifFs.logBlockSize;
    self->fs.spifFs.cfg.log_page_size    = cfg->format->spifFs.logPageSize;
    self->fs.spifFs.cfg.hal_read_f       = spiffsRead;
    self->fs.spifFs.cfg.hal_write_f      = spiffsWrite;
    self->fs.spifFs.cfg.hal_erase_f      = spiffsErase;

    self->fs.spifFs.fs.user_data = (void*) self;

    return OS_SUCCESS;
}

OS_Error_t
SpifFs_free(
    OS_FileSystem_Handle_t self)
{
  (void) self;
  return OS_SUCCESS;
}

OS_Error_t
SpifFs_format(
    OS_FileSystem_Handle_t self)
{
  spiffs *fs         = &self->fs.spifFs.fs;
  spiffs_config *cfg = &self->fs.spifFs.cfg;
  int rc;

  // SPIFFS_format needs to be called with an initalized spiffs structure,
  // the initialization of which happens in SPIFFS_mount.

  rc = SPIFFS_mount(fs, cfg, self->fs.spifFs.spiffs_work_buf,
		    self->fs.spifFs.spiffs_fds,
		    sizeof(self->fs.spifFs.spiffs_fds),
		    self->fs.spifFs.spiffs_cache_buf,
		    sizeof(self->fs.spifFs.spiffs_cache_buf), 0);

  if (rc == SPIFFS_OK)
  {
      SPIFFS_unmount (fs);
  }

  if ((rc = SPIFFS_format(fs)) < 0)
  {
      Debug_LOG_ERROR("SPIFFS_format() failed with %d", rc);
  }

  return (rc < 0) ? OS_ERROR_GENERIC : OS_SUCCESS;
}

OS_Error_t
SpifFs_mount(
    OS_FileSystem_Handle_t self)
{
  spiffs *fs         = &self->fs.spifFs.fs;
  spiffs_config *cfg = &self->fs.spifFs.cfg;
  int rc;

  rc = SPIFFS_mount(fs, cfg, self->fs.spifFs.spiffs_work_buf,
		    self->fs.spifFs.spiffs_fds,
		    sizeof(self->fs.spifFs.spiffs_fds),
		    self->fs.spifFs.spiffs_cache_buf,
		    sizeof(self->fs.spifFs.spiffs_cache_buf), 0);

  return (rc == SPIFFS_OK) ? OS_SUCCESS: OS_ERROR_GENERIC;
}

OS_Error_t
SpifFs_unmount(
    OS_FileSystem_Handle_t self)
{
  spiffs *fs         = &self->fs.spifFs.fs;

  // SPIFFS_unmount does not return an error code.
  SPIFFS_unmount (fs);

  return OS_SUCCESS;
}


/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "xbmc/libXBMC_addon.h"
#include "xbmc/threads/mutex.h"
#include <map>
#include <sstream>

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

extern "C" {

#include "xbmc/xbmc_vfs_dll.h"
#include "xbmc/IFileTypes.h"

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct AFPContext
{
  std::string name;
};

void* Open(const char* url, const char* hostname,
           const char* filename2, unsigned int port,
           const char* options, const char* username,
           const char* password)
{
}

bool Close(void* context)
{
}

int64_t GetLength(void* context)
{
}

//*********************************************************************************************
int64_t GetPosition(void* context)
{
}


int64_t Seek(void* context, int64_t iFilePosition, int iWhence)
{
}

bool Exists(const char* url, const char* hostname,
            const char* filename, unsigned int port,
            const char* options, const char* username,
            const char* password)
{
}

int Stat(const char* url, const char* hostname,
         const char* filename2, unsigned int port,
         const char* options, const char* username,
         const char* password, struct __stat64* buffer)
{
}

int IoControl(void* context, XFILE::EIoControl request, void* param)
{
}

void ClearOutIdle()
{
}

void DisconnectAll()
{
}

bool DirectoryExists(const char* url, const char* hostname,
                     const char* filename, unsigned int port,
                     const char* options, const char* username,
                     const char* password)
{
}

void* GetDirectory(const char* url, const char* hostname,
                   const char* filename, unsigned int port,
                   const char* options, const char* username,
                   const char* password, VFSDirEntry** items,
                   int* num_items)
{
}

void FreeDirectory(void* items)
{
}

bool CreateDirectory(const char* url, const char* hostname,
                     const char* filename, unsigned int port,
                     const char* options, const char* username,
                     const char* password)
{
}

bool RemoveDirectory(const char* url, const char* hostname,
                     const char* filename, unsigned int port,
                     const char* options, const char* username,
                     const char* password)
{
}

int Truncate(void* context, int64_t size)
{
}

int Write(void* context, const void* lpBuf, int64_t uiBufSize)
{
}

bool Delete(const char* url, const char* hostname,
            const char* filename2, unsigned int port,
            const char* options, const char* username,
            const char* password)
{
}

bool Rename(const char* url, const char* hostname,
            const char* filename, unsigned int port,
            const char* options, const char* username,
            const char* password,
            const char* url2, const char* hostname2,
            const char* filename2, unsigned int port2,
            const char* options2, const char* username2,
            const char* password2)
{
}

void* OpenForWrite(const char* url, const char* hostname,
                   const char* filename2, unsigned int port,
                   const char* options, const char* username,
                   const char* password, bool bOverWrite)
{ 
}

void* ContainsFiles(const char* url, const char* hostname,
                    const char* filename2, unsigned int port,
                    const char* options, const char* username,
                    const char* password,
                    VFSDirEntry** items, int* num_items)
{
  return NULL;
}

}

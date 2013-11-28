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
  int64_t size;
  int64_t pos;
  struct afp_file_info* pFp;
  struct afp_volume* pAfpVol;

  AFPContext()
  {
    size = pos = 0;
    pFp = NULL;
    pAfpVol = NULL;
  }
};

static bool IsValidFile(const std::string& strFileName)
{
  if (strFileName.find('/') == std::string::npos || /* doesn't have sharename */
      strFileName.substr(strFileName.size()-2) == "/." || /* not current folder */
      strFileName.substr(strFileName.size()-3) == "/..")  /* not parent folder */
    return false;
  return true;
}

void* Open(const char* url, const char* hostname,
           const char* filename2, unsigned int port,
           const char* options, const char* username,
           const char* password)
{
  // we can't open files like afp://file.f or afp://server/file.f
  // if a file matches the if below return false, it can't exist on a afp share.
  if (!IsValidFile(filename))
  {
    XBMC->Log(ADDON::LOG_INFO, "FileAfp: Bad URL : '%s'", filename);
    return NULL;
  }

  PLATFORM::CLockObject lock(CAfpConnection::Get());
  if (CAFPConnection::Get().Connect(url) != CAFPConnection::AfpOk ||
      !CAfpConnection::Get().GetVolume())
    return NULL;

  AFPContext* result = new AFPContext;
  ctx->pAfpVol = CAfpConnection::Get().GetVolume();

  std::string strPath = CAfpConnection::Get().GetPath(url);

  if (afp_wrap_open(ctx->pAfpVol, strPath.c_str(), O_RDONLY, &result->pFp))
  {
    char* encoded = XBMC->URLEncode(strPath.c_str());
    std::string strEncode(encoded);
    XBMC->FreeString(encoded);
    if (afp_wrap_open(ctx->pAfpVol, strEncode.c_str(), O_RDONLY, &result->pFp))
    {
      // write error to logfile
      delete result;
      XBMC->Log(ADDON::LOG_INFO, "CAFPFile::Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strPath.c_str(), errno, strerror(errno));
      return NULL;
    }
  }
  
  XBMC->Log(ADDON::LOG_DEBUG,"CAFPFile::Open - opened %s, fd=%d",filename, result->pFp ? result->pFp->fileid:-1);
  
#ifdef TARGET_POSIX
  struct __stat64 tmpBuffer;
#else
  struct stat tmpBuffer;
#endif  
  if(Stat(url, hostname, filename, options, username, password, &tmpBuffer))
  {
    delete result;
    return NULL;
  }

  result->size = tmpBuffer.st_size;
  result->pos = 0;

  // We've successfully opened the file!
  return result;
}

bool Close(void* context)
{
  PLATFORM::CLockObject lock(CAfpConnection::Get());
  if (ctx->pFp && ctx->pAfpVol)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "CAFPFile::Close closing fd %d", ctx->pFp->fileid);
#ifdef USE_CVS_AFPFS
    char *name = ctx->pFp->basename;
#else
    char *name = ctx->pFp->name;
    if (strlen(name) == 0)
      name = ctx->pFp->basename;
#endif
    afp_wrap_close(ctx->pAfpVol, name, ctx->pFp);
    free(ctx->pFp);
    delete ctx;
  }
}

int64_t GetLength(void* context)
{
  AFPContext* ctx = (AFPContext*)context;

  return ctx->size;
}

//*********************************************************************************************
int64_t GetPosition(void* context)
{
  AFPContext* ctx = (AFPContext*)context;

  return ctx->pos;
}


int64_t Seek(void* context, int64_t iFilePosition, int iWhence)
{
  AFPContext* ctx = (AFPContext*)context;

  off_t newOffset = ctx->pos;

  switch(iWhence)
  {
    case SEEK_SET:
      newOffset = iFilePosition;
      break;
    case SEEK_END:
      newOffset = ctx->pos+iFilePosition;
      break;
    case SEEK_CUR:
      newOffset += iFilePosition;
      break;
  }

  if ( newOffset < 0 || newOffset > ctx->size)
  {
    XBMC->Log(ADDON::LOG_ERROR, "%s - Error( %"PRId64")", __FUNCTION__, newOffset);
    return -1;
  }

  ctx->pos = newOffset;
  return (int64_t)ctx->pos;
}

bool Exists(const char* url, const char* hostname,
            const char* filename, unsigned int port,
            const char* options, const char* username,
            const char* password)
{
  return Stat(url, hostname, filename, port, options, username, password, NULL) == 0;
}

int Stat(const char* url, const char* hostname,
         const char* filename, unsigned int port,
         const char* options, const char* username,
         const char* password, struct __stat64* buffer)
{
  PLATFORM::CLockObject lock(CAFPConnection::Get());
  if (CAFPConnection::Get().Connect(url) != CAFPConnection::AfpOk ||
      !CAFPConnection::Get().GetVolume())
    return -1;

  std::string strPath = CAfpConnection::Get().GetPath(url);

  struct stat tmpBuffer = {0};
  int iResult = afp_wrap_getattr(CAfpConnection::Get().GetVolume(), strPath.c_str(), &tmpBuffer);

  if (buffer)
  {
    memset(buffer, 0, sizeof(struct __stat64));
    buffer->st_dev   = tmpBuffer.st_dev;
    buffer->st_ino   = tmpBuffer.st_ino;
    buffer->st_mode  = tmpBuffer.st_mode;
    buffer->st_nlink = tmpBuffer.st_nlink;
    buffer->st_uid   = tmpBuffer.st_uid;
    buffer->st_gid   = tmpBuffer.st_gid;
    buffer->st_rdev  = tmpBuffer.st_rdev;
    buffer->st_size  = tmpBuffer.st_size;
    buffer->st_atime = tmpBuffer.st_atime;
    buffer->st_mtime = tmpBuffer.st_mtime;
    buffer->st_ctime = tmpBuffer.st_ctime;
  }

  return iResult;
}

int IoControl(void* context, XFILE::EIoControl request, void* param)
{
  if(request == XFILE::IOCTRL_SEEK_POSSIBLE)
    return 1;

  return -1;
}

void ClearOutIdle()
{
  CAFPConnection::Get().CheckIfIdle();
}

void DisconnectAll()
{
  CAFPConnection::Get().Deinit();
}

bool DirectoryExists(const char* url, const char* hostname,
                     const char* filename, unsigned int port,
                     const char* options, const char* username,
                     const char* password)
{
  PLATFORM::CLockObject lock(CAfpConnection::Get());

  if (CAfpConnection::Get().Connect(url) != CAFPConnection::AfpOk || 
      !CAFPConnection::Get().GetVolume())
    return false;

  std::string strFileName(CAfpConnection::Get().GetPath(url));

  struct stat info;
  if (afp_wrap_getattr(CAfpConnection::Get().GetVolume(), strFileName.c_str(), &info) != 0)
    return false;

  return (info.st_mode & S_IFDIR) ? true : false;
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
  PLATFORM::CLockObject lock(CAfpConnection::Get());

  if (CAfpConnection::Get().Connect(url) != CAFPConnection::AfpOk ||
      !CAfpConnection::Get().GetVolume())
    return false;

  std::string strFilename = CAfpConnection::Get().GetPath(url);

  int result = afp_wrap_mkdir(CAfpConnection::Get().GetVolume(), strFilename.c_str(), 0);

  if (result != 0)
    XBMC->Log(ADDON::LOG_ERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0 || EEXIST == result);
}

bool RemoveDirectory(const char* url, const char* hostname,
                     const char* filename, unsigned int port,
                     const char* options, const char* username,
                     const char* password)
{
  PLATFORM::CLockObject lock(CAfpConnection::Get());

  if (CAfpConnection::Get().Connect(url) != CAFPConnection::AfpOk ||
      !CAFPConnection::Get().GetVolume())
    return false;

  std::string strFileName = CAfpConnection::Get().GetPath(url);

  int result = afp_wrap_rmdir(CAfpConnection::Get().GetVolume(), strFileName.c_str());

  if (result != 0 && errno != ENOENT)
  {
    XBMC->Log(ADDON::LOG_ERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
    return false;
  }

  return true;
}

int Truncate(void* context, int64_t size)
{
  return -1;
}

int Write(void* context, const void* lpBuf, int64_t uiBufSize)
{
  PLATFORM::CLockObject lock(CAfpConnection::Get();

  int numberOfBytesWritten = 0;
  uid_t uid;
  gid_t gid;

  // FIXME need a better way to get server's uid/gid
  uid = getuid();
  gid = getgid();
#ifdef USE_CVS_AFPFS
  char *name = ctx->pFp->basename;
#else
  char *name = ctx->pFp->name;
  if (strlen(name) == 0)
    name = ictx->pFp->basename;
#endif
  numberOfBytesWritten = afp_wrap_write(ctx->pAfpVol, name, (const char *)lpBuf,
                                        (size_t)uiBufSize, ctx->pos, ctx->pFp, uid, gid);

  return numberOfBytesWritten;
}

unsigned int Read(void* context, void* lpBuf, int64_t uiBufSize)
{
  AFPContext* ctx = (AFPContext*)context;
  PLATFORM::CLockObject lock(CAfpConnection::Get());
  if (ctx->pFp == NULL || !ctx->pAfpVol)
    return 0;

  if (uiBufSize > AFP_MAX_READ_SIZE)
    uiBufSize = AFP_MAX_READ_SIZE;

#ifdef USE_CVS_AFPFS
  char *name = ctx->pFp->basename;
#else
  char *name = ctx->pFp->name;
  if (strlen(name) == 0)
    name = ctx->pFp->basename;

#endif
  int eof = 0;
  int bytesRead = afp_wrap_read(ctx->pAfpVol, name, (char *)lpBuf,(size_t)uiBufSize,
                                ctx->pos, ctx->pFp, &eof);
  if (bytesRead > 0)
    ctx->pos += bytesRead;

  if (bytesRead < 0)
  {
    XBMC->Log(ADDON::LOG_ERROR, "%s - Error( %d, %d, %s )", __FUNCTION__, bytesRead, errno, strerror(errno));
    return 0;
  }

  return (unsigned int)bytesRead;
}

bool Delete(const char* url, const char* hostname,
            const char* filename2, unsigned int port,
            const char* options, const char* username,
            const char* password)
{
  PLATFORM::CLockObject lock(CAfpConnection::Get());
  if (CAfpConnection::Get().Connect(url) != CAFPConnection::AfpOk ||
      !CAfpConnection::Get().GetVolume())
    return false;

  std::string strPath = CAfpConnection::Get().GetPath(url);

  int result = afp_wrap_unlink(CAfpConnection::Get().GetVolume(), strPath.c_str());

  if (result != 0)
    XBMC->Log(ADDON::LOG_ERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0);
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
  PLATFORM::CLockObject lock(CAfpConnection::Get());
  if (CAfpConnection::Get().Connect(url) != CAFPConnection::AfpOk ||
      !CAfpConnection::Get().GetVolume())
    return false;

  std::string strFile = CAfpConnection::Get().GetPath(url);
  std::string strFileNew = CAfpConnection.GetPath(url2);

  int result = afp_wrap_rename(CAfpConnection::Get().GetVolume(),
                               strFile.c_str(), strFileNew.c_str());

  if (result != 0)
    XBMC->Log(ADDON::LOG_ERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

void* OpenForWrite(const char* url, const char* hostname,
                   const char* filename2, unsigned int port,
                   const char* options, const char* username,
                   const char* password, bool bOverWrite)
{ 
  int ret = 0;
  m_fileSize = 0;
  m_fileOffset = 0;

  PLATFORM::CLockObject lock(CAfpConnection::Get());
  if (CAFPConnection::Get().Connect(url) != CAFPConnection::AfpOk ||
      !CAFPConnection::Get().GetVolume())
    return false;

  // we can't open files like afp://file.f or afp://server/file.f
  // if a file matches the if below return false, it can't exist on a afp share.
  if (!IsValidFile(filename))
    return false;

  AFPContext* result = new AFPContext;
  result->pAfpVol = CAFPConnection::Get().GetVolume();

  std::string strPath = gAfpConnection.GetPath(url);

  if (bOverWrite)
  {
    XBMC->Log(ADDON::LOG_INFO, "FileAFP::OpenForWrite() called with overwriting enabled! - %s", strPath.c_str());
    ret = afp_wrap_creat(result->pAfpVol, strPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

  ret = afp_wrap_open(result->pAfpVol, strPath.c_str(), O_RDWR, &result->pFp);

  if (ret || result->pFp == NULL)
  {
    // write error to logfile
    XBMC->Log(ADDON::LOG_ERROR, "CAFPFile::Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strPath.c_str(), errno, strerror(errno));
    delete result;
    return NULL;
  }

  // We've successfully opened the file!
#ifdef TARGET_POSIX
  struct __stat64 tmpBuffer;
#else
  struct stat tmpBuffer;
#endif  
  if(Stat(url, filename, hostname, port, options, username, password, &tmpBuffer))
  {
    delete result;
    return NULL;
  }

  result->size = tmpBuffer.st_size;
  result->pos = 0;

  return result;
}

void* ContainsFiles(const char* url, const char* hostname,
                    const char* filename2, unsigned int port,
                    const char* options, const char* username,
                    const char* password,
                    VFSDirEntry** items, int* num_items)
{
  return NULL;
}

int GetStartTime(void* ctx)
{
  return 0;
}

int GetTotalTime(void* ctx)
{
  return 0;
}

bool NextChannel(void* context, bool preview)
{
  return false;
}

bool PrevChannel(void* context, bool preview)
{
  return false;
}

bool SelectChannel(void* context, unsigned int uiChannel)
{
  return false;
}

bool UpdateItem(void* context)
{
  return false;
}

}

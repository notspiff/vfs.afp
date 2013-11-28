/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include <string>
#include <xbmc/threads/mutex.h>

extern "C" {
#include <afpfs-ng/libafpclient.h>
#include <afpfs-ng/map_def.h>
#include <afpfs-ng/midlevel.h>
}

#ifdef USE_CVS_AFPFS
#define afp_wrap_open                 afp_ml_open
#define afp_wrap_close                afp_ml_close
#define afp_wrap_read                 afp_ml_read
#define afp_wrap_write                afp_ml_write
#define afp_wrap_getattr              afp_ml_getattr
#define afp_wrap_server_full_connect  afp_server_full_connect
#define afp_wrap_unlink               afp_ml_unlink
#define afp_wrap_rename               afp_ml_rename
#define afp_wrap_creat                afp_ml_creat
#define afp_wrap_readdir              afp_ml_readdir
#define afp_wrap_readlink             afp_ml_readlink
#define afp_wrap_mkdir                afp_ml_mkdir
#define afp_wrap_rmdir                afp_ml_rmdir
#else
#define afp_wrap_open                 ml_open
#define afp_wrap_close                ml_close
#define afp_wrap_read                 ml_read
#define afp_wrap_write                ml_write
#define afp_wrap_getattr              ml_getattr
#define afp_wrap_server_full_connect  afp_server_full_connect
#define afp_wrap_unlink               ml_unlink
#define afp_wrap_rename               ml_rename
#define afp_wrap_creat                ml_creat
#define afp_wrap_readdir              ml_readdir
#define afp_wrap_readlink             ml_readlink
#define afp_wrap_mkdir                ml_mkdir
#define afp_wrap_rmdir                ml_rmdir
#endif



class CAFPConnection : public PLATFORM::CMutex
{
public:
  enum eAfpConnectError
  {
    AfpOk     = 0,
    AfpFailed = 1,
    AfpAuth   = 2,
  };
  typedef enum eAfpConnectError afpConnnectError;

  static CAFPConnection& Get();
  virtual ~CAFPConnection();

  afpConnnectError      Connect(const std::string& url);
  void                  Disconnect(void);
  struct afp_server     *GetServer()    {return m_pAfpServer;}
  struct afp_volume     *GetVolume()    {return m_pAfpVol;};
  struct afp_url        *GetUrl()       {return m_pAfpUrl;};
  std::string           GetPath(const std::string& url);
  
  const char            *GetConnectedIp() const { if(m_pAfpUrl) return m_pAfpUrl->servername;else return "";}
  
  //special stat which uses its own context
  //needed for getting intervolume symlinks to work
  //it uses the same global server connection
  //but its own volume
  int                   stat(const std::string& url, struct stat *statbuff);
  
  void AddActiveConnection();
  void AddIdleConnection();
  void CheckIfIdle();  
  void Deinit();  

private:
  CAFPConnection();
  bool                  initLib(void);
  bool                  connectVolume(const char *volumename, struct afp_volume *&pVolume);
  void                  disconnectVolume(void);
  std::string           getAuthenticatedPath(const std::string& url);

  int                   m_OpenConnections;
  int                   m_IdleTimeout;
  struct afp_server     *m_pAfpServer;
  struct afp_volume     *m_pAfpVol;
  struct afp_url        *m_pAfpUrl;
  struct libafpclient   *m_pAfpClient;
  bool                   m_inited;
};


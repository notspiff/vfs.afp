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

struct afp_server;
struct afp_volume;
struct afp_url;
struct libafpclient;

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

  static AFPConnection& Get();
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


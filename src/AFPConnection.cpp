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

#include "AFPConnection.h"
#include <xbmc/libXBMC_addon.h>

extern ADDON::CHelper_libXBMC_addon* XBMC;

void AFPConnectionLog(void *priv, enum loglevels loglevel, int logtype, const char *message)
{
  if (!message) return;
  std::string msg = "LIBAFPCLIENT: " + std::string(message);

  switch(logtype)
  {
    case LOG_WARNING:
      XBMC->Log(ADDON::LOG_INFO, "%s", msg.c_str());
      break;
    case LOG_ERR:
      XBMC->Log(ADDON::LOG_ERROR, "%s", msg.c_str());
      break;
    default:
      XBMC->Log(ADDON::LOG_DEBUG, "%s", msg.c_str());
      break;
  }
}

CAFPConnection::CAFPConnection()
 : m_OpenConnections(0)
 , m_IdleTimeout(0)
 , m_pAfpServer(NULL)
 , m_pAfpVol(NULL)
 , m_pAfpUrl((struct afp_url*)malloc(sizeof(struct afp_url)))
 , m_pAfpClient((struct libafpclient*)malloc(sizeof(struct libafpclient)))
 , m_inited(false)
{
}

CAFPConnection::~CAFPConnection()
{
  Disconnect();
  free(m_pAfpClient);
  free(m_pAfpUrl);
}

bool CAFPConnection::initLib()
{
  if (!m_inited)
  {
    m_pAfpClient->unmount_volume = NULL;
    m_pAfpClient->log_for_client = AFPConnectionLog;
    m_pAfpClient->forced_ending_hook = NULL;
    m_pAfpClient->scan_extra_fds = NULL;
    m_pAfpClient->loop_started = NULL;

    m_pLibAfp->libafpclient_register(m_pAfpClient);
    m_pLibAfp->init_uams();
    m_pLibAfp->afp_main_quick_startup(NULL);
    XBMC->Log(ADDON::LOG_DEBUG, "AFP: Supported UAMs: %s", get_uam_names_list());
  }
  m_inited = true;
  return m_inited;
}

//only unmount here - afpclient lib is not
//stoppable (no afp_main_quick_shutdown as counter part
//for afp_main_quick_startup)
void CAFPConnection::Deinit()
{
  if(m_pAfpVol)
  {
    disconnectVolume();
    Disconnect();
    m_pAfpUrl->servername[0] = '\0';
  }        
}

void CAFPConnection::Disconnect()
{
  PLATFORM::CLockObject lock(*this);
  m_pAfpServer = NULL;
}

void CAFPConnection::disconnectVolume()
{
  if (m_pAfpVol)
  {
    // afp_unmount_volume(m_pAfpVol);
    afp_unmount_all_volumes(m_pAfpServer);
    m_pAfpVol = NULL;
  }
}

// taken from cmdline tool
bool CAFPConnection::connectVolume(const char *volumename, struct afp_volume *&pVolume)
{
  bool ret = false;
  if (strlen(volumename) != 0)
  {
    // Ah, we're not connected to a volume
    unsigned int len = 0;
    char mesg[1024];

    if ((pVolume = find_volume_by_name(m_pAfpServer, volumename)) == NULL)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "AFP: Could not find a volume called %s\n", volumename);
    }
    else
    {
      pVolume->mapping = AFP_MAPPING_LOGINIDS;
      pVolume->extra_flags |= VOLUME_EXTRA_FLAGS_NO_LOCKING;

      if (afp_connect_volume(pVolume, m_pAfpServer, mesg, &len, 1024 ))
      {
        XBMC->Log(ADDON::LOG_DEBUG, "AFP: Could not access volume %s (error: %s)\n", pVolume->volume_name, mesg);
        pVolume = NULL;
      }
      else
      {
        XBMC->Log(ADDON::LOG_DEBUG, "AFP: Connected to volume %s\n", pVolume->volume_name_printable);
        ret = true;
      }
    }
  }

  return ret;
}

std::string CAFPConnection::getAuthenticatedPath(const std::string& url)
{
/*  CURL authURL(url);
  CStdString ret;
  CPasswordManager::GetInstance().AuthenticateURL(authURL);
  ret = authURL.Get();
  return ret;*/
  return "";
}

CAFPConnection::afpConnnectError CAFPConnection::Connect(const std::string& url)
{
  PLATFORM::CLockObject lock(*this);
  struct afp_connection_request *conn_req = NULL;
  struct afp_url tmpurl;
//  CURL nonConstUrl(getAuthenticatedPath(url)); // we need a editable copy of the url
  bool serverChanged=false;

  if (!initLib())
    return AfpFailed;

  afp_default_url(&tmpurl);
/*
  // if hostname has changed - assume server changed
  if (!nonConstUrl.GetHostName().Equals(m_pAfpUrl->servername, false)|| (m_pAfpServer && m_pAfpServer->connect_state == 0))
  {
    serverChanged = true;
    Disconnect();
  }

  // if volume changed - also assume server changed (afpclient can't reuse old servobject it seems)
  if (!nonConstUrl.GetShareName().Equals(m_pAfpUrl->volumename, false))
  {
   // no reusing of old server object possible with libafpclient it seems...
    serverChanged = true;
    Disconnect();
  }

  // first, try to parse the URL
  if (m_pLibAfp->afp_parse_url(&tmpurl, nonConstUrl.Get().c_str(), 0) != 0)
  {
    // Okay, this isn't a real URL
    CLog::Log(LOGDEBUG, "AFP: Could not parse url: %s!\n", nonConstUrl.Get().c_str());
    return AfpFailed;
  }
  else // parsed sucessfull
  {
    // this is our current url object whe are connected to (at least we try)
    *m_pAfpUrl = tmpurl;
  }

  // if no username and password is set - use no user authent uam
  if (strlen(m_pAfpUrl->password) == 0 && strlen(m_pAfpUrl->username) == 0)
  {
    // try anonymous
    strncpy(m_pAfpUrl->uamname, "No User Authent", sizeof(m_pAfpUrl->uamname));
    CLog::Log(LOGDEBUG, "AFP: Using anonymous authentication.");
  }
  else if ((nonConstUrl.GetPassWord().empty() || nonConstUrl.GetUserName().empty()) && serverChanged)
  {
    // this is our current url object whe are connected to (at least we try)
    return AfpAuth;
  }

  // we got a password in the url
  if (!nonConstUrl.GetPassWord().empty())
  {
    // copy password because afp_parse_url just puts garbage into the password field :(
    strncpy(m_pAfpUrl->password, nonConstUrl.GetPassWord().c_str(), 127);
  }

  // whe are not connected or we want to connect to another server
  if (!m_pAfpServer || serverChanged)
  {
    // code from cmdline tool
    conn_req = (struct afp_connection_request*)malloc(sizeof(struct afp_connection_request));
    memset(conn_req, 0, sizeof(struct afp_connection_request));

    conn_req->url = *m_pAfpUrl;
    conn_req->url.requested_version = 31;

    if (strlen(m_pAfpUrl->uamname)>0)
    {
      if ((conn_req->uam_mask = m_pLibAfp->find_uam_by_name(m_pAfpUrl->uamname)) == 0)
      {
        CLog::Log(LOGDEBUG, "AFP:I don't know about UAM %s\n", m_pAfpUrl->uamname);
        m_pAfpUrl->volumename[0] = '\0';
        m_pAfpUrl->servername[0] = '\0';
        free(conn_req);
        return AfpFailed;
      }
    }
    else
    {
      conn_req->uam_mask = m_pLibAfp->default_uams_mask();
    }

    // try to connect
#ifdef USE_CVS_AFPFS
    if ((m_pAfpServer = m_pLibAfp->afp_wrap_server_full_connect(NULL, conn_req, NULL)) == NULL)
#else
    if ((m_pAfpServer = m_pLibAfp->afp_wrap_server_full_connect(NULL, conn_req)) == NULL)
#endif
    {
      m_pAfpUrl->volumename[0] = '\0';
      m_pAfpUrl->servername[0] = '\0';
      free(conn_req);
      CLog::Log(LOGERROR, "AFP: Error connecting to %s", url.Get().c_str());
      return AfpFailed;
    }
    // success!
    CLog::Log(LOGDEBUG, "AFP: Connected to server %s using UAM \"%s\"\n",
      m_pAfpServer->server_name, m_pLibAfp->uam_bitmap_to_string(m_pAfpServer->using_uam));
    // we don't need it after here ...
    free(conn_req);
  }

  // if server changed reconnect volume
  if (serverChanged)
  {
    connectVolume(m_pAfpUrl->volumename, m_pAfpVol); // connect new volume
  }*/
  return AfpOk;
}

int CAFPConnection::stat(const std::string& url, struct stat *statbuff)
{
  PLATFORM::CLockObject lock(*this);
  std::string strPath = gAfpConnection.GetPath(url);
  struct afp_volume *pTmpVol = NULL;
  struct afp_url tmpurl;
  int iResult = -1;
//  CURL nonConstUrl(getAuthenticatedPath(url)); // we need a editable copy of the url

  if (!initLib() || !m_pAfpServer)
    return -1;

  afp_default_url(&tmpurl);

  // first, try to parse the URL
  if (afp_parse_url(&tmpurl, strPath.c_str(), 0) != 0)
  {
    // Okay, this isn't a real URL
    XBMC->Log(ADDON::LOG_DEBUG, "AFP: Could not parse url: %s!\n", strPath.c_str());
    return -1;
  }

  // if no username and password is set - use no user authent uam
  if (strlen(tmpurl.password) == 0 && strlen(tmpurl.username) == 0)
  {
    // try anonymous
    strncpy(tmpurl.uamname, "No User Authent", sizeof(tmpurl.uamname));
    XBMC->Log(ADDON::LOG_DEBUG, "AFP: Using anonymous authentication.");
  }
  /*else if ((nonConstUrl.GetPassWord().empty() || nonConstUrl.GetUserName().empty()))
  {
    // this is our current url object whe are connected to (at least we try)
    return -1;
  }
*/
  // we got a password in the url
/*  if (!nonConstUrl.GetPassWord().empty())
  {
    // copy password because afp_parse_url just puts garbage into the password field :(
    strncpy(tmpurl.password, nonConstUrl.GetPassWord().c_str(), 127);
  }*/

  // connect new volume
  if(connectVolume(tmpurl.volumename, pTmpVol) && pTmpVol)
  {
    iResult = afp_wrap_getattr(pTmpVol, strPath.c_str(), statbuff);
    //unmount single volume crashs
    //we will get rid of the mounted volume
    //once the context is changed in connect function
    //ppppooooorrrr!!
    //m_pLibAfp->afp_unmount_volume(pTmpVol);
  }
  return iResult;
}


/* This is called from CApplication::ProcessSlow() and is used to tell if afp have been idle for too long */
void CAFPConnection::CheckIfIdle()
{
  /* We check if there are open connections. This is done without a lock to not halt the mainthread. It should be thread safe as
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if wich will lead to another check, wich is locked.  */
  if (m_OpenConnections == 0 && m_pAfpVol != NULL)
  { /* I've set the the maxiumum IDLE time to be 1 min and 30 sec. */
    PLATFORM::CLockObject lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */)
    {
      if (m_IdleTimeout > 0)
      {
        m_IdleTimeout--;
      }
      else
      {
        XBMC->Log(ADDON::LOG_NOTICE, "AFP is idle. Closing the remaining connections.");
        Deinit();
      }
    }
  }
}

/* The following two function is used to keep track on how many Opened files/directories there are.
needed for unloading the dylib*/
void CAFPConnection::AddActiveConnection()
{
  PLATFORM::CLockObject lock(*this);
  m_OpenConnections++;
}

void CAFPConnection::AddIdleConnection()
{
  PLATFORM::CLockObject lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any wierd behaviours if a user
   leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;
}

std:.string CAFPConnection::GetPath(const std::string& url)
{
  struct afp_url tmpurl;
  std::string ret;

  afp_default_url(&tmpurl);

  // First, try to parse the URL
  if (afp_parse_url(&tmpurl, url.c_str(), 0) != 0 )
  {
    // Okay, this isn't a real URL
    XBMC->Log(ADDON::LOG_DEBUG, "AFP: Could not parse url.\n");
  }
  else
  {
    ret = tmpurl.path;
  }
  return ret;
}

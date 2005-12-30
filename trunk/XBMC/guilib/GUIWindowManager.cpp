#include "include.h"
#include "GUIWindowManager.h"
#include "GUIAudioManager.h"
#include "GUIDialog.h"
#include "../xbmc/settings.h"
//GeminiServer 
#include "../xbmc/GUIPassword.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIWindowManager m_gWindowManager;
CGUIWindowManager::CGUIWindowManager(void)
{
  InitializeCriticalSection(&m_critSection);

  m_pCallback = NULL;
  m_iActiveWindow = -1;
  m_bShowOverlay = true;
}

CGUIWindowManager::~CGUIWindowManager(void)
{
  DeleteCriticalSection(&m_critSection);
}

void CGUIWindowManager::Initialize()
{
  m_iActiveWindow = -1;
  g_graphicsContext.setMessageSender(this);
  LoadNotOnDemandWindows();
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message)
{
  bool handled = false;
//  CLog::DebugLog("SendMessage: mess=%d send=%d control=%d param1=%d", message.GetMessage(), message.GetSenderId(), message.GetControlId(), message.GetParam1());
  // Send the message to all none window targets
  for (int i = 0; i < (int) m_vecMsgTargets.size(); i++)
  {
    IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];

    if (pMsgTarget)
    {
      if (pMsgTarget->OnMessage( message )) handled = true;
    }
  }

  //  Send the message to all active modeless windows ..
  for (int i=0; i < (int) m_vecModelessWindows.size(); i++)
  {
    CGUIWindow* pWindow = m_vecModelessWindows[i];

    if (pWindow)
    {
      if (pWindow->OnMessage( message )) handled = true;
    }
  }

  //  A GUI_MSG_NOTIFY_ALL is send to any active modal dialog
  //  and all windows whether they are active or not
  if (message.GetMessage()==GUI_MSG_NOTIFY_ALL)
  {
    int topWindow = m_vecModalWindows.size();
    while (topWindow)
      m_vecModalWindows[--topWindow]->OnMessage(message);

    topWindow = m_vecWindows.size();
    while (topWindow)
      m_vecWindows[--topWindow]->OnMessage(message);

    handled = true;
  }
  // Have we routed windows...
  else if (m_vecModalWindows.size() > 0)
  {
    // ...send the message to the top most.
    int topWindow = m_vecModalWindows.size();
    bool modalHandled = false;
    while (topWindow && !modalHandled)
    {
      if (m_vecModalWindows[--topWindow]->OnMessage(message))
        modalHandled = true;
    }
    if (modalHandled) handled = true;

    if (m_iActiveWindow < 0)
    {
      return false;
    }

    CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];

    // Also send the message to the parent of the routed window, if its the target
    if ( message.GetSenderId() == pWindow->GetID() ||
         message.GetControlId() == pWindow->GetID() ||
         message.GetSenderId() == 0 )
    {
      if (pWindow->OnMessage(message)) handled = true;
    }
  }
  else
  {
    // ..no, only call message function of the active window
    if (m_iActiveWindow < 0)
    {
      return false;
    }

    CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
    if (pWindow->OnMessage(message)) handled = true;
  }
  return handled;
}

void CGUIWindowManager::Add(CGUIWindow* pWindow)
{
  m_vecWindows.push_back(pWindow);
}

void CGUIWindowManager::AddCustomWindow(CGUIWindow* pWindow)
{
  Add(pWindow);
  m_vecCustomWindows.push_back(pWindow);
}

void CGUIWindowManager::AddModeless(CGUIWindow* pWindow)
{
  // only add the window if it's not already added
  for (unsigned int i = 0; i < m_vecModelessWindows.size(); i++)
    if (m_vecModelessWindows[i] == pWindow) return;
  m_vecModelessWindows.push_back(pWindow);
}

void CGUIWindowManager::Remove(DWORD dwID)
{
  vector<CGUIWindow*>::iterator it = m_vecWindows.begin();
  while (it != m_vecWindows.end())
  {
    CGUIWindow* pWindow = *it;
    if (pWindow->GetID() == dwID)
    {
      m_vecWindows.erase(it);
      it = m_vecWindows.end();
    }
    else it++;
  }
}

// removes and deletes the window.  Should only be called
// from the class that created the window using new.
void CGUIWindowManager::Delete(DWORD dwID)
{
  CGUIWindow *pWindow = GetWindow(dwID);
  if (pWindow)
  {
    Remove(dwID);
    delete pWindow;
  }
}

void CGUIWindowManager::RemoveModeless(DWORD dwID)
{
  vector<CGUIWindow*>::iterator it = m_vecModelessWindows.begin();
  while (it != m_vecModelessWindows.end())
  {
    CGUIWindow* pWindow = *it;
    if (pWindow->GetID() == dwID)
    {
      m_vecModelessWindows.erase(it);
      it = m_vecModelessWindows.end();
    }
    else it++;
  }
}

void CGUIWindowManager::PreviousWindow()
{
  // deactivate any window
  CLog::DebugLog("CGUIWindowManager::PreviousWindow: Deactivate");

  int iPrevActiveWindow = m_iActiveWindow;
  int iPrevActiveWindowID = 0;

  if (m_iActiveWindow >= 0 && m_iActiveWindow < (int)m_vecWindows.size())
  {
    CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
    iPrevActiveWindowID = pWindow->GetPreviousWindowID();
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, iPrevActiveWindowID);
    pWindow->OnMessage(msg);
    m_iActiveWindow = WINDOW_INVALID;
  }

  CLog::DebugLog("CGUIWindowManager::PreviousWindow: Activate new");
  // activate the new window
  for (int i = 0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow = m_vecWindows[i];

    if (pWindow->HasID(iPrevActiveWindowID))
    {
      CLog::DebugLog("CGUIWindowManager::PreviousWindow: Activating");
      m_iActiveWindow = i;
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, iPrevActiveWindowID);
      pWindow->OnMessage(msg);
      return ;
    }
  }

  CLog::DebugLog("CGUIWindowManager::PreviousWindow: No previous");
  // previous window doesnt exists. (maybe .xml file is invalid or doesnt exists)
  // so we go back to the previous window
  m_iActiveWindow = 0;
  CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID);
  pWindow->OnMessage(msg);
}

void CGUIWindowManager::RefreshWindow()
{
  // deactivate the current window
  if (m_iActiveWindow >= 0)
  {
    CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
    pWindow->OnMessage(msg);
  }

  // reactivate the current window
  CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID);
  pWindow->OnMessage(msg);
}

void CGUIWindowManager::ChangeActiveWindow(int newWindow)
{
  if (m_iActiveWindow >= 0)
  {
    DWORD previousID = m_vecWindows[m_iActiveWindow]->GetPreviousWindowID();
    ActivateWindow(newWindow);
    m_vecWindows[m_iActiveWindow]->SetPreviousWindowID(previousID);
  }
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const CStdString& strPath)
{
  // translate virtual windows
  // virtual music window which returns the last open music window (aka the music start window)
  if (iWindowID == WINDOW_MUSIC)
  {
    iWindowID = g_stSettings.m_iMyMusicStartWindow;
    // ensure the music virtual window only returns music files and music library windows
    if (iWindowID != WINDOW_MUSIC_FILES && iWindowID != WINDOW_MUSIC_NAV)
      iWindowID = WINDOW_MUSIC_FILES;
  }
  // virtual video window which returns the last open video window (aka the video start window)
  if (iWindowID == WINDOW_VIDEOS)
  {
    if (g_stSettings.m_iVideoStartWindow > 0)
    {
      iWindowID = g_stSettings.m_iVideoStartWindow;
    }
  }

  // first check existence of the window we wish to activate.
  CGUIWindow *pNewWindow = GetWindow(iWindowID);
  if (!pNewWindow)
  { // nothing to see here - move along
    CLog::Log(LOGERROR, "Unable to locate window with id %d.  Check skin files", iWindowID - WINDOW_HOME);
    return ;
  }

   // GeminiServer HomeMenuLock with MasterCode!
  if(!g_passwordManager.CheckMenuLock(iWindowID))
  {
    CLog::Log(LOGERROR, "MasterCode is Wrong: Window with id %d will not be loaded! Enter a correct MasterCode!", iWindowID);
    iWindowID = WINDOW_HOME;
  }

  // deactivate any window
  int iPrevActiveWindow = m_iActiveWindow;
  if (m_iActiveWindow >= 0)
  {
    CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
    //  Play the window specific deinit sound
    g_audioManager.PlayWindowSound(pWindow->GetID(), SOUND_DEINIT);
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, iWindowID);
    pWindow->OnMessage(msg);
    m_iActiveWindow = -1;
  }

  // activate the new window
  for (int i = 0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow = m_vecWindows[i];

    if (pWindow->HasID(iWindowID))
    {
      //  Play the window specific init sound
      g_audioManager.PlayWindowSound(pWindow->GetID(), SOUND_INIT);

      CLog::Log(LOGINFO, "Activating Window ID: %i", iWindowID);
      if (!strPath.IsEmpty()) CLog::Log(LOGINFO, "  with path parameter: %s", strPath.c_str());

      m_iActiveWindow = i;

      // Check to see that this window is not our previous window
      if (iPrevActiveWindow == -1 || m_vecWindows[iPrevActiveWindow]->GetPreviousWindowID() == iWindowID)
      {
        // we are going to the last window - don't update it's previous window id
        CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, iWindowID);

        // append the destination path
        if (!strPath.IsEmpty()) msg.SetStringParam(strPath);
        pWindow->OnMessage(msg);
      }
      else
      {
        if (m_vecWindows[iPrevActiveWindow] == pWindow)
        {
          // we are going to the same window - leave previous window ID as is
          CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, pWindow->GetPreviousWindowID(), iWindowID);

          // do we need to append a destination path if the window is the same?
          if (!strPath.IsEmpty()) msg.SetStringParam(strPath);
          pWindow->OnMessage(msg);
        }
        else
        {
          // we are going to a new window - put our current window into it's previous window ID
          CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, m_vecWindows[iPrevActiveWindow]->GetID(), iWindowID);

          // append the destination path
          if (!strPath.IsEmpty()) msg.SetStringParam(strPath);
          pWindow->OnMessage(msg);
        }
      }
      return ;
    }
  }
  // we should never, ever get here.
  CLog::Log(LOGERROR, "ActivateWindow() failed trying to activate window %d", iWindowID - WINDOW_HOME);
}

bool CGUIWindowManager::OnAction(const CAction &action)
{
  // Have we have routed windows...
  if (m_vecModalWindows.size() > 0)
  {
    // ...send the action to the top most.
    return m_vecModalWindows[m_vecModalWindows.size() - 1]->OnAction(action);
  }
  else if (m_iActiveWindow >= 0)
  {
    CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];

    if (pWindow)
    {
      return pWindow->OnAction(action);
    }
  }
  return false;
}

void CGUIWindowManager::Render()
{
  if (m_iActiveWindow >= 0)
  {
    CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
    pWindow->Render();
  }

//  RenderDialogs();
}

bool RenderOrderSortFunction(CGUIDialog *first, CGUIDialog *second)
{
  return first->GetRenderOrder() < second->GetRenderOrder();
}

void CGUIWindowManager::RenderDialogs()
{
  // find the window with the lowest render order
  vector<CGUIDialog *> renderList;
  for (unsigned int i = 0; i < m_vecModalWindows.size(); i++)
    renderList.push_back((CGUIDialog *)m_vecModalWindows[i]);
  for (unsigned int i = 0; i < m_vecModelessWindows.size(); i++)
    renderList.push_back((CGUIDialog *)m_vecModelessWindows[i]);
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  // iterate through and render if they're running
  for (unsigned int i = 0; i < renderList.size(); i++)
  {
    CGUIDialog *pDialog = renderList[i];
    if (pDialog->IsRunning())
      pDialog->Render();
  }
}

CGUIWindow* CGUIWindowManager::GetWindow(DWORD dwID)
{
  if (dwID == WINDOW_INVALID)
  {
    return NULL;
  }

  for (int i = 0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow = m_vecWindows[i];
    if (pWindow)
    {
      if (pWindow->HasID(dwID))
      {
        return pWindow;
      }
    }
  }

  return NULL;
}

// Shows and hides modeless dialogs as necessary.
void CGUIWindowManager::UpdateModelessVisibility()
{
  for (int i = 0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow = m_vecWindows[i];
    if (pWindow && pWindow->IsDialog() && pWindow->GetVisibleCondition())
    {
      if (g_infoManager.GetBool(pWindow->GetVisibleCondition(), m_iActiveWindow))
        ((CGUIDialog *)pWindow)->Show(GetActiveWindow());
      else
        ((CGUIDialog *)pWindow)->Close();
    }
  }
}

void CGUIWindowManager::Process()
{
  if (m_pCallback)
  {
    m_pCallback->Process();
    m_pCallback->FrameMove();
    m_pCallback->Render();

  }
}

void CGUIWindowManager::SetCallback(IWindowManagerCallback& callback)
{
  m_pCallback = &callback;
}

void CGUIWindowManager::DeInitialize()
{
  for (int i = 0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow = m_vecWindows[i];

    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
    pWindow->OnMessage(msg);
    pWindow->FreeResources(true);
  }
  UnloadNotOnDemandWindows();

  m_vecMsgTargets.erase( m_vecMsgTargets.begin(), m_vecMsgTargets.end() );

  // destroy our custom windows...
  for (int i = 0; i < (int)m_vecCustomWindows.size(); i++)
  {
    CGUIWindow *pWindow = m_vecCustomWindows[i];
    Remove(pWindow->GetID());
    delete pWindow;
  }
  m_vecCustomWindows.erase( m_vecCustomWindows.begin(), m_vecCustomWindows.end() );

}

/// \brief Route to a window
/// \param pWindow Window to route to
void CGUIWindowManager::RouteToWindow(CGUIWindow* pWindow)
{
  // Just to be sure: Unroute this window,
  // #we may have routed to it before
  UnRoute(pWindow->GetID());

  m_vecModalWindows.push_back(pWindow);

}

/// \brief Unroute window
/// \param dwID ID of the window routed
void CGUIWindowManager::UnRoute(DWORD dwID)
{
  vector<CGUIWindow*>::iterator it = m_vecModalWindows.begin();
  while (it != m_vecModalWindows.end())
  {
    CGUIWindow* pWindow = *it;
    if (pWindow->GetID() == dwID)
    {
      m_vecModalWindows.erase(it);
      it = m_vecModalWindows.end();
    }
    else it++;
  }
}

bool CGUIWindowManager::IsRouted(bool includeFadeOuts /*= false */) const
{
  if (includeFadeOuts)
    return m_vecModalWindows.size() > 0;

  bool hasActiveDialog = false;
  for (unsigned int i = 0; i < m_vecModalWindows.size(); i++)
  {
    if (m_vecModalWindows[i]->GetEffectState() != EFFECT_OUT)
    {
      hasActiveDialog = true;
      break;
    }
  }
  return hasActiveDialog;
}

bool CGUIWindowManager::IsModelessAvailable() const
{
  if (m_vecModelessWindows.size()>0)
    return true;

  return false;
}

/// \brief Get the ID of the top most routed window
/// \return dwID ID of the window or WINDOW_INVALID if no routed window available
int CGUIWindowManager::GetTopMostRoutedWindowID() const
{
  if (m_vecModalWindows.size() <= 0)
    return WINDOW_INVALID;

  return m_vecModalWindows[m_vecModalWindows.size() - 1]->GetID();
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message)
{
  ::EnterCriticalSection(&m_critSection );

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( msg );

  ::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::DispatchThreadMessages()
{
  ::EnterCriticalSection(&m_critSection );
  while ( m_vecThreadMessages.size() > 0 )
  {
    vector<CGUIMessage*>::iterator it = m_vecThreadMessages.begin();
    CGUIMessage* pMsg = *it;
    // first remove the message from the queue,
    // else the message could be processed more then once
    it = m_vecThreadMessages.erase(it);
    
    //Leave critical section here since this can cause some thread to come back here into dispatch
    ::LeaveCriticalSection(&m_critSection );
    SendMessage( *pMsg );
    delete pMsg;
    ::EnterCriticalSection(&m_critSection );
  }

  ::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::AddMsgTarget( IMsgTargetCallback* pMsgTarget )
{
  m_vecMsgTargets.push_back( pMsgTarget );
}

int CGUIWindowManager::GetActiveWindow() const
{
  if (m_iActiveWindow < 0)
  {
    return 0;
  }

  CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];

  return pWindow->GetID();
}

bool CGUIWindowManager::IsWindowActive(DWORD dwID) const
{
  if (GetActiveWindow() == dwID) return true;
  // run through the modal + modeless windows
  for (unsigned int i = 0; i < m_vecModalWindows.size(); i++)
  {
    CGUIWindow *pWindow = m_vecModalWindows[i];
    if (dwID == pWindow->GetID() && pWindow->GetEffectState() != EFFECT_OUT)
      return true;
  }
  for (unsigned int i = 0; i < m_vecModelessWindows.size(); i++)
  {
    CGUIWindow *pWindow = m_vecModelessWindows[i];
    if (dwID == pWindow->GetID() && pWindow->GetEffectState() != EFFECT_OUT)
      return true;
  }
  return false; // window isn't active
}

void CGUIWindowManager::LoadNotOnDemandWindows()
{
  for (int i = 0; i < (int)m_vecWindows.size(); i++)
  {
    if (!m_vecWindows[i]->GetLoadOnDemand())
    {
      m_vecWindows[i]->FreeResources(true);
      m_vecWindows[i]->Initialize();
    }
  }
}

void CGUIWindowManager::UnloadNotOnDemandWindows()
{
  for (int i = 0; i < (int)m_vecWindows.size(); i++)
  {
    if (!m_vecWindows[i]->GetLoadOnDemand())
    {
      m_vecWindows[i]->FreeResources(true);
    }
  }
}

bool CGUIWindowManager::IsOverlayAllowed() const
{
  return m_bShowOverlay;
}

void CGUIWindowManager::ShowOverlay(bool bOnOff)
{
  m_bShowOverlay = bOnOff;
}


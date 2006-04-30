#include "stdafx.h"
#include "GUIDialogProgress.h"
#include "GUIProgressControl.h"
#include "application.h"


#define CONTROL_PROGRESS_BAR 20

CGUIDialogProgress::CGUIDialogProgress(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_PROGRESS, "DialogProgress.xml")
{
  m_bCanceled = false;
  m_iCurrent=0;
  m_iMax=0;
}

CGUIDialogProgress::~CGUIDialogProgress(void)
{

}

void CGUIDialogProgress::StartModal(DWORD dwParentId)
{
  CLog::DebugLog("DialogProgress::StartModal called %s", m_bRunning ? "(already running)!" : "");
  m_bCanceled = false;
  m_dwParentWindowID = dwParentId;
  m_pParentWindow = m_gWindowManager.GetWindow( m_dwParentWindowID);
  if (!m_pParentWindow)
  {
    m_dwParentWindowID = 0;
    return ;
  }

  // set running before it's routed, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_bRunning = true;
  m_dialogClosing = false;
  m_gWindowManager.RouteToWindow(this);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);
  ShowProgressBar(false);

  while (m_bRunning && IsAnimating(ANIM_TYPE_WINDOW_OPEN))
    Progress();
}

void CGUIDialogProgress::Progress()
{
  if (m_bRunning)
  {
    m_gWindowManager.Process();
  }
}

void CGUIDialogProgress::ProgressKeys()
{
  if (m_bRunning)
  {
    g_application.FrameMove();
  }
}

bool CGUIDialogProgress::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {

  case GUI_MSG_CLICKED:
    {
      int iAction = message.GetParam1();
      if (1 || ACTION_SELECT_ITEM == iAction)
      {
        int iControl = message.GetSenderId();
        if (iControl == 10)
        {
          string strHeading = m_strHeading;
          strHeading.append(" : ");
          strHeading.append(g_localizeStrings.Get(16024));
          CGUIDialogBoxBase::SetHeading(strHeading);
          m_bCanceled = true;
          return true;
        }
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogProgress::OnAction(const CAction &action)
{
  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
    m_bCanceled = true;
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogProgress::SetPercentage(int iPercentage)
{
  if (iPercentage < 0) iPercentage = 0;
  if (iPercentage > 100) iPercentage = 100;

  g_graphicsContext.Lock();
  CGUIProgressControl* pControl = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS_BAR);
  if (pControl) pControl->SetPercentage((float)iPercentage);
  g_graphicsContext.Unlock();
}

void CGUIDialogProgress::SetProgressMax(int iMax)
{
  m_iMax=iMax;
  m_iCurrent=0;
}

void CGUIDialogProgress::SetProgressAdvance(int nSteps/*=1*/)
{
  m_iCurrent+=nSteps;

  if (m_iCurrent>m_iMax)
    m_iCurrent=0;

  SetPercentage((m_iCurrent*100)/m_iMax);
}

bool CGUIDialogProgress::Abort()
{
  return m_bRunning ? m_bCanceled : false;
}

void CGUIDialogProgress::ShowProgressBar(bool bOnOff)
{
  if (bOnOff)
  {
    SET_CONTROL_VISIBLE(CONTROL_PROGRESS_BAR);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_PROGRESS_BAR);
  }
}

void CGUIDialogProgress::SetHeading(const string& strLine)
{
  m_strHeading = strLine;
  CGUIDialogBoxBase::SetHeading(m_strHeading);
}

void CGUIDialogProgress::SetHeading(int iString)
{
  m_strHeading = g_localizeStrings.Get(iString);
  CGUIDialogBoxBase::SetHeading(m_strHeading);
}

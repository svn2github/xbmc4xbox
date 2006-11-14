
#include "../stdafx.h"
#include "dlgcache.h"

extern "C" void mplayer_exit_player(void);

CDlgCache::CDlgCache(DWORD dwDelay)
{
  m_pDlg = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  /* if progress dialog is already running, take it over */
  if( m_pDlg->IsRunning() )
    dwDelay = 0;

  m_pDlg = NULL;

  m_strLinePrev = "";

  if(dwDelay == 0)
    OpenDialog();    
  else
    m_dwTimeStamp = GetTickCount() + dwDelay;

  Create(true);
}

void CDlgCache::Close(bool bForceClose)
{
  if (m_pDlg) m_pDlg->Close(bForceClose);

  //Set stop, this will kill this object, when thread stops  
  CThread::m_bStop = true;
}

CDlgCache::~CDlgCache()
{
  CThread::m_bAutoDelete = false;
  CThread::m_bStop = true;
  CThread::m_eventStop.WaitMSec(1000);
}

void CDlgCache::OpenDialog()
{
  m_pDlg = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if(m_pDlg == NULL) return;

  /* if any other modal dialog is open, don't open this one */
  if (m_gWindowManager.IsRouted(true) && !m_pDlg->IsRunning() )
  {
    m_pDlg = NULL;
    return;
  }
  
  m_pDlg->SetHeading(438);
  m_pDlg->SetLine(0, m_strLinePrev);
  m_pDlg->SetLine(1, "");
  m_pDlg->SetLine(2, "");
  m_pDlg->StartModal();
  m_strLinePrev = "";

  bSentCancel = false;
}

void CDlgCache::Update()
{
  if( g_graphicsContext.IsFullScreenVideo() )
    return;

  if (m_pDlg)
  {
    m_pDlg->Progress();
    if( !bSentCancel && m_pDlg->IsCanceled())
    {
      try 
      {
        mplayer_exit_player(); 
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "CDlgCache::Update - Exception thrown in mplayer_exit_player()");
      }
    }
  }
  else if( GetTickCount() > m_dwTimeStamp )
    OpenDialog();
}

void CDlgCache::SetMessage(const CStdString& strMessage)
{
  if (m_pDlg)
  {    
    m_pDlg->SetLine(0, m_strLinePrev);
    m_pDlg->SetLine(1, strMessage);
  }
  m_strLinePrev = strMessage;
}

bool CDlgCache::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  CSingleLock lock(g_graphicsContext);
  ShowProgressBar(true);
  SetPercentage(ipercent);
  if( IsCanceled() ) 
  {
    return false;
  }
  else
    return true;
}

void CDlgCache::Process()
{
  while( true )
  {
    
    { //Section to make the lock go out of scope before sleep
      CSingleLock lock(g_graphicsContext);
      if( CThread::m_bStop ) break;

      try 
      {        
        Update();
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "Exception in CDlgCache::Process()");
      }
    }

    Sleep(10);
  }
};

void CDlgCache::ShowProgressBar(bool bOnOff) 
{ 
  if(m_pDlg) 
    m_pDlg->ShowProgressBar(bOnOff); 
}
void CDlgCache::SetPercentage(int iPercentage) 
{ 
  if(m_pDlg) 
    m_pDlg->SetPercentage(iPercentage); 
}
bool CDlgCache::IsCanceled() const
{ 
  if(m_pDlg) 
    return m_pDlg->IsCanceled();
  else 
    return false; 
}
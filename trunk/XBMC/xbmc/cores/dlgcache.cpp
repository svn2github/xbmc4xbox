
#include "../stdafx.h"
#include "dlgcache.h"

extern "C" void mplayer_exit_player(void);

CDlgCache::CDlgCache(DWORD dwDelay)
{
  m_pDlg = NULL;
  m_strLinePrev = "";
  if(dwDelay == 0) 
    OpenDialog();
  else
    m_dwTimeStamp = GetTickCount() + dwDelay;

  Create(true);
}

void CDlgCache::Close()
{
  if (m_pDlg) m_pDlg->Close();

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
  if (m_pDlg)
  {
    m_pDlg->SetHeading(438);
    m_pDlg->SetLine(0, m_strLinePrev);
    m_pDlg->SetLine(1, "");
    m_pDlg->SetLine(2, "");
    m_pDlg->StartModal( m_gWindowManager.GetActiveWindow());
    m_strLinePrev = "";
  }
  bSentCancel = false;
}

void CDlgCache::Update()
{
  if (m_pDlg)
  {
    m_pDlg->Progress();
    if( !bSentCancel && m_pDlg->IsCanceled())
    {
      mplayer_exit_player();      
    }
  }
  else if( g_graphicsContext.IsFullScreenVideo() )
  {
    //Could be used to display some progress while in fullscreen
    return;
  }
  else if( GetTickCount() > m_dwTimeStamp )
  {
    OpenDialog();
    m_pDlg->Progress();
  }
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

void CDlgCache::Process()
{
  while(!CThread::m_bStop )
  {
    try 
    {
      CGraphicContext::CLock lock(g_graphicsContext);
      Update();
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Exception in CDlgCache::Process()");
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
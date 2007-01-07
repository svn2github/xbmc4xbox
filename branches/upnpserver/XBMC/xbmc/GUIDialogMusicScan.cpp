#include "stdafx.h"
#include "GUIDialogMusicScan.h"
#include "GUIProgressControl.h"
#include "application.h"
#include "util.h"


#define CONTROL_LABELSTATUS     401
#define CONTROL_LABELDIRECTORY  402
#define CONTROL_PROGRESS        403

CGUIDialogMusicScan::CGUIDialogMusicScan(void)
: CGUIDialog(WINDOW_DIALOG_MUSIC_SCAN, "DialogMusicScan.xml")
{
  m_musicInfoScanner.SetObserver(this);
}

CGUIDialogMusicScan::~CGUIDialogMusicScan(void)
{
}

bool CGUIDialogMusicScan::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      m_strCurrentDir.Empty();

      m_fPercentDone=-1.0F;

      UpdateState();
      return true;
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogMusicScan::Render()
{
  if (m_bRunning)
    UpdateState();

  CGUIDialog::Render();
}

void CGUIDialogMusicScan::OnDirectoryChanged(const CStdString& strDirectory)
{
  CSingleLock lock (m_critical);

  m_strCurrentDir = strDirectory;
}

void CGUIDialogMusicScan::OnStateChanged(SCAN_STATE state)
{
  CSingleLock lock (m_critical);

  m_ScanState = state;
}

void CGUIDialogMusicScan::OnSetProgress(int currentItem, int itemCount)
{
  CSingleLock lock (m_critical);

  m_fPercentDone=(float)((currentItem*100)/itemCount);
  if (m_fPercentDone>100.0F) m_fPercentDone=100.0F;
}

void CGUIDialogMusicScan::StartScanning(const CStdString& strDirectory, bool bUpdateAll)
{
  m_ScanState = PREPARING;

  Show();

  // save settings
  g_application.SaveMusicScanSettings();

  m_musicInfoScanner.Start(strDirectory, bUpdateAll);
}

void CGUIDialogMusicScan::StopScanning()
{
  if (m_musicInfoScanner.IsScanning())
    m_musicInfoScanner.Stop();
}

bool CGUIDialogMusicScan::IsScanning()
{
  return m_musicInfoScanner.IsScanning();
}

void CGUIDialogMusicScan::OnDirectoryScanned(const CStdString& strDirectory)
{
  CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
  msg.SetStringParam(strDirectory);
  m_gWindowManager.SendThreadMessage(msg);
}

void CGUIDialogMusicScan::OnFinished()
{
  // clear cache
  CUtil::DeleteDatabaseDirectoryCache();

  // send message
  CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
  m_gWindowManager.SendThreadMessage(msg);

  // be sure to restore the settings
  CLog::Log(LOGINFO,"Music scan was stopped or finished ... restoring FindRemoteThumbs");
  g_application.RestoreMusicScanSettings();

  Close();
}

void CGUIDialogMusicScan::UpdateState()
{
  CSingleLock lock (m_critical);

  SET_CONTROL_LABEL(CONTROL_LABELSTATUS, GetStateString());

  if (m_ScanState == READING_MUSIC_INFO)
  {
    CURL url(m_strCurrentDir);
    CStdString strStrippedPath;
    url.GetURLWithoutUserDetails(strStrippedPath);

    SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, strStrippedPath);

    if (m_fPercentDone>-1.0F)
    {
      SET_CONTROL_VISIBLE(CONTROL_PROGRESS);
      CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
      if (pProgressCtrl) pProgressCtrl->SetPercentage(m_fPercentDone);
    }
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, "");
    SET_CONTROL_HIDDEN(CONTROL_PROGRESS);
  }
}

int CGUIDialogMusicScan::GetStateString()
{
  if (m_ScanState == PREPARING)
    return 314;
  else if (m_ScanState == REMOVING_OLD)
    return 701;
  else if (m_ScanState == CLEANING_UP_DATABASE)
    return 700;
  else if (m_ScanState == READING_MUSIC_INFO)
    return 505;
  else if (m_ScanState == COMPRESSING_DATABASE)
    return 331;
  else if (m_ScanState == WRITING_CHANGES)
    return 328;

  return -1;
}

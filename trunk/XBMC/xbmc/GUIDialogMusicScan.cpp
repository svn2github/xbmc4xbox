#include "stdafx.h"
#include "GUIDialogMusicScan.h"
#include "guiWindowManager.h"
#include "settings.h"
#include "application.h"
#include "localizeStrings.h"

#define CONTROL_LABELSTATUS			401
#define CONTROL_LABELDIRECTORY	402

CGUIDialogMusicScan::CGUIDialogMusicScan(void) : CGUIDialog(0)
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
			// resources are allocated in g_application

			m_ScanState=PREPARING;

			m_strCurrentDir.Empty();

			UpdateState();
			return true;
		}
		break;

 		case GUI_MSG_WINDOW_DEINIT:
		{
			//don't deinit, g_application handles it

			return true;
		}
		break;
	}

	return CGUIDialog::OnMessage(message);
}

void CGUIDialogMusicScan::OnDirectoryChanged(const CStdString& strDirectory)
{
	CSingleLock lock(m_critical);

	m_strCurrentDir=strDirectory;
}

void CGUIDialogMusicScan::OnStateChanged(SCAN_STATE state)
{
	CSingleLock lock(m_critical);

	m_ScanState=state;
}

void CGUIDialogMusicScan::StartScanning(const CStdString& strDirectory, bool bUpdateAll)
{
	Show(m_gWindowManager.GetActiveWindow());

	m_musicInfoScanner.Start(strDirectory, bUpdateAll);
}

void CGUIDialogMusicScan::StopScanning()
{
	if (m_musicInfoScanner.IsScanning())
		m_musicInfoScanner.Stop();

	Close();
}

void CGUIDialogMusicScan::OnDirectoryScanned(const CStdString& strDirectory)
{
	CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
	msg.SetStringParam(strDirectory);
	m_gWindowManager.SendThreadMessage(msg);
}

void CGUIDialogMusicScan::OnFinished()
{
	CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
	m_gWindowManager.SendThreadMessage(msg);

	Close();
}

void CGUIDialogMusicScan::UpdateState()
{
	if (m_bRunning)
	{
		CSingleLock lock(m_critical);

		SET_CONTROL_LABEL(CONTROL_LABELSTATUS, GetStateString());

		if (m_ScanState==READING_MUSIC_INFO)
		{
			CURL url(m_strCurrentDir);
			CStdString strStrippedPath;
			url.GetURLWithoutUserDetails(strStrippedPath);

			SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, strStrippedPath);
		}
		else
		{
			SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, "");
		}
	}
}

int CGUIDialogMusicScan::GetStateString()
{
	if (m_ScanState==PREPARING)
		return 314;
	else if (m_ScanState==REMOVING_OLD)
		return 701;
	else if (m_ScanState==CLEANING_UP_DATABASE)
		return 700;
	else if (m_ScanState==READING_MUSIC_INFO)
		return 505;
	else if (m_ScanState==COMPRESSING_DATABASE)
		return 331;
	else if (m_ScanState==WRITING_CHANGES)
		return 328;

	return -1;
}

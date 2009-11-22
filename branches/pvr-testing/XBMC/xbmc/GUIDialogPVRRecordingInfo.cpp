/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


/* Standart includes */
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUISettings.h"
#include "Util.h"

/* self include */
#include "GUIDialogTVRecordingInfo.h"

/* TV control */
#include "PVRManager.h"

/* TV information tags */
#include "utils/PVREpg.h"
#include "utils/PVRChannels.h"
#include "utils/PVRRecordings.h"
#include "utils/PVRTimers.h"

/* Dialog windows includes */
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"

using namespace std;

#define CONTROL_REC_TITLE               20 // from db
#define CONTROL_REC_SUBTITLE            21
#define CONTROL_REC_STARTTIME           23
#define CONTROL_REC_DATE                24
#define CONTROL_REC_DURATION            25
#define CONTROL_REC_GENRE               26
#define CONTROL_REC_CHANNEL             27

#define CONTROL_TEXTAREA                22

#define CONTROL_BTN_OK                  10


CGUIDialogTVRecordingInfo::CGUIDialogTVRecordingInfo(void)
    : CGUIDialog(WINDOW_DIALOG_TV_RECORDING_INFO, "DialogRecordingInfo.xml")
    , m_recordItem(new CFileItem)
{
}

CGUIDialogTVRecordingInfo::~CGUIDialogTVRecordingInfo(void)
{
}

bool CGUIDialogTVRecordingInfo::OnMessage(CGUIMessage& message)
{

  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
  {
  }
  break;
  case GUI_MSG_WINDOW_INIT:
  {
    CGUIDialog::OnMessage(message);
    Update();
    return true;
  }
  break;
  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();
    {
      if (iControl == CONTROL_BTN_OK)
      {
        Close();
        return true;
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTVRecordingInfo::SetRecording(const CFileItem *item)
{
  *m_recordItem = *item;
}

void CGUIDialogTVRecordingInfo::Update()
{
  CStdString strTemp;
  strTemp = m_recordItem->GetPVRRecordingInfoTag()->m_strTitle; strTemp.Trim();
  SetLabel(CONTROL_REC_TITLE, strTemp);

  strTemp = m_recordItem->GetPVRRecordingInfoTag()->RecordingTime().GetAsLocalizedDate(true); strTemp.Trim();
  SetLabel(CONTROL_REC_DATE, strTemp);

  strTemp = m_recordItem->GetPVRRecordingInfoTag()->RecordingTime().GetAsLocalizedTime("", false); strTemp.Trim();
  SetLabel(CONTROL_REC_STARTTIME, strTemp);

  strTemp.Format("%i", m_recordItem->GetPVRRecordingInfoTag()->GetDuration()/60); strTemp.Trim();
  SetLabel(CONTROL_REC_DURATION, strTemp);

  strTemp = m_recordItem->GetPVRRecordingInfoTag()->m_strGenre; strTemp.Trim();
  SetLabel(CONTROL_REC_GENRE, strTemp);

  strTemp = m_recordItem->GetPVRRecordingInfoTag()->ChannelName(); strTemp.Trim();
  SetLabel(CONTROL_REC_CHANNEL, strTemp);

  // programme subtitle
  strTemp = m_recordItem->GetPVRRecordingInfoTag()->m_strPlotOutline; strTemp.Trim();

  if (strTemp.IsEmpty())
  {
    SET_CONTROL_HIDDEN(CONTROL_REC_SUBTITLE);
  }
  else
  {
    SetLabel(CONTROL_REC_SUBTITLE, strTemp);
    SET_CONTROL_VISIBLE(CONTROL_REC_SUBTITLE);
  }

  // programme description
  strTemp = m_recordItem->GetPVRRecordingInfoTag()->m_strPlot; strTemp.Trim();

  SetLabel(CONTROL_TEXTAREA, strTemp);
}

void CGUIDialogTVRecordingInfo::SetLabel(int iControl, const CStdString &strLabel)
{
  if (strLabel.IsEmpty())
  {
    SET_CONTROL_LABEL(iControl, 416); /// disable instead? // "Not available"
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

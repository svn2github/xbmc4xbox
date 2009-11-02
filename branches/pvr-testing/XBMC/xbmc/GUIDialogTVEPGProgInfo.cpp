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
#include "GUIDialogOK.h"
#include "Util.h"

/* self include */
#include "GUIDialogTVEPGProgInfo.h"

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

#define CONTROL_PROG_TITLE              20 // from db
#define CONTROL_PROG_SUBTITLE           21
#define CONTROL_PROG_STARTTIME          23
#define CONTROL_PROG_DATE               24
#define CONTROL_PROG_DURATION           25
#define CONTROL_PROG_GENRE              26
#define CONTROL_PROG_CHANNEL            27
#define CONTROL_PROG_CHANNELCALLSIGN    28
#define CONTROL_PROG_CHANNELNUM         29

#define CONTROL_PROG_RECSTATUS          30 // from pvrmanager
#define CONTROL_PROG_JOBSTATUS          31

#define CONTROL_TEXTAREA                22

#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7


CGUIDialogTVEPGProgInfo::CGUIDialogTVEPGProgInfo(void)
    : CGUIDialog(WINDOW_DIALOG_TV_GUIDE_INFO, "DialogEPGProgInfo.xml")
    , m_progItem(new CFileItem)
{
}

CGUIDialogTVEPGProgInfo::~CGUIDialogTVEPGProgInfo(void)
{
}

bool CGUIDialogTVEPGProgInfo::OnMessage(CGUIMessage& message)
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
    m_viewDescription = true;
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
      else if (iControl == CONTROL_BTN_RECORD)
      {
        int iChannel = m_progItem->GetTVEPGInfoTag()->Channel();

        if (iChannel != -1)
        {
          if (m_progItem->GetTVEPGInfoTag()->m_isRecording == false)
          {
            // prompt user for confirmation of channel record
            CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

            if (pDialog)
            {
              pDialog->SetHeading(264);
              pDialog->SetLine(0, "");
              pDialog->SetLine(1, m_progItem->GetTVEPGInfoTag()->Title());
              pDialog->SetLine(2, "");
              pDialog->DoModal();

              if (pDialog->IsConfirmed())
              {
                cPVRTimerInfoTag newtimer(*m_progItem.get());
                CFileItem *item = new CFileItem(newtimer);

                if (cPVRTimers::AddTimer(*item))
                {
                  m_progItem->GetTVEPGInfoTag()->m_isRecording = true;
                }
              }
            }
          }
          else
          {
            CGUIDialogOK::ShowAndGetInput(18100,18107,0,0);
          }
        }

        Close();

        return true;
      }
      else if (iControl == CONTROL_BTN_SWITCH)
      {
        Close();
        {
          CFileItemList channelslist;
          int ret_channels;

          if (!m_progItem->GetTVEPGInfoTag()->m_isRadio)
            ret_channels = PVRChannelsTV.GetChannels(&channelslist, -1);
          else
            ret_channels = PVRChannelsRadio.GetChannels(&channelslist, -1);

          if (ret_channels > 0)
          {
            if (!g_application.PlayFile(*channelslist[m_progItem->GetTVEPGInfoTag()->Channel()-1]))
            {
              CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
              return false;
            }
            return true;
          }
        }
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTVEPGProgInfo::SetProgInfo(const CFileItem *item)
{
  *m_progItem = *item;
}

void CGUIDialogTVEPGProgInfo::Update()
{
  CStdString strTemp;

  strTemp = m_progItem->GetTVEPGInfoTag()->Title(); strTemp.Trim();
  SetLabel(CONTROL_PROG_TITLE, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->Start().GetAsLocalizedDate(true); strTemp.Trim();
  SetLabel(CONTROL_PROG_DATE, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->Start().GetAsLocalizedTime("", false); strTemp.Trim();
  SetLabel(CONTROL_PROG_STARTTIME, strTemp);

  strTemp.Format("%i", m_progItem->GetTVEPGInfoTag()->DurationSeconds()/60);
  strTemp.Trim();
  SetLabel(CONTROL_PROG_DURATION, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->Genre(); strTemp.Trim();
  SetLabel(CONTROL_PROG_GENRE, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->ChannelName();
  strTemp.Trim();
  SetLabel(CONTROL_PROG_CHANNEL, strTemp);

  strTemp.Format("%u", m_progItem->GetTVEPGInfoTag()->Channel()); // int value
  SetLabel(CONTROL_PROG_CHANNELNUM, strTemp);

  // programme subtitle
  strTemp = m_progItem->GetTVEPGInfoTag()->PlotOutline(); strTemp.Trim();

  if (strTemp.IsEmpty())
  {
    SET_CONTROL_HIDDEN(CONTROL_PROG_SUBTITLE);
  }
  else
  {
    SetLabel(CONTROL_PROG_SUBTITLE, strTemp);
    SET_CONTROL_VISIBLE(CONTROL_PROG_SUBTITLE);
  }

  // programme description
  strTemp = m_progItem->GetTVEPGInfoTag()->Plot(); strTemp.Trim();

  SetLabel(CONTROL_TEXTAREA, strTemp);

  SET_CONTROL_VISIBLE(CONTROL_BTN_RECORD);
}

void CGUIDialogTVEPGProgInfo::SetLabel(int iControl, const CStdString &strLabel)
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

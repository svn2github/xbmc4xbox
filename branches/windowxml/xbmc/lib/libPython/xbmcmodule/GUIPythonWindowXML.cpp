#include "../../../stdafx.h"
#include "GUIPythonWindowXML.h"
#include "guiwindow.h"
#include "pyutil.h"
#include "..\..\..\application.h"
#include "..\..\..\GUIMediaWindow.h"
#include "window.h"
#include "control.h"
#include "action.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_BTNTYPE         5
#define CONTROL_LIST            50
#define CONTROL_LABELFILES      12

#define CONTROL_VIEW_START      50
#define CONTROL_VIEW_END        59


using namespace PYXBMC;

CGUIPythonWindowXML::CGUIPythonWindowXML(DWORD dwId, CStdString strXML)
: CGUIWindow(dwId,strXML)
{
	pCallbackWindow = NULL;
	m_actionEvent = CreateEvent(NULL, true, false, NULL);
  m_loadOnDemand = false;
  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
  m_coordsRes  = PAL_4x3;
  //m_needsScaling = false;
}

CGUIPythonWindowXML::~CGUIPythonWindowXML(void)
{
  	CloseHandle(m_actionEvent);
}
void CGUIPythonWindowXML::Update()
{
}
bool CGUIPythonWindowXML::OnAction(const CAction &action)
{
  // do the base class window first, and the call to python after this
	bool ret = CGUIWindow::OnAction(action);
	if(pCallbackWindow)
	{
		PyXBMCAction* inf = new PyXBMCAction;
		inf->pObject = Action_FromAction(action);
		inf->pCallbackWindow = pCallbackWindow;

		// aquire lock?
		Py_AddPendingCall(Py_XBMC_Event_OnAction, inf);
		PulseActionEvent();
	}
  return ret;
}

void CGUIPythonWindowXML::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  for (int i = CONTROL_VIEW_START; i <= CONTROL_VIEW_END; i++)
    m_viewControl.AddView(GetControl(i));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
  UpdateButtons();
}

bool CGUIPythonWindowXML::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
			CGUIWindow::OnMessage(message);
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      PyXBMCAction* inf = new PyXBMCAction;
		  inf->pObject = NULL;
      // create a new call and set it in the python queue
		  inf->pCallbackWindow = pCallbackWindow;
      Py_AddPendingCall(Py_XBMC_Event_OnInit, inf);
			return true;
    }
		break;
  case GUI_MSG_ITEM_SELECT:
    {
      bool hs = m_viewControl.HasControl(message.GetControlId());
      int GCC = m_viewControl.GetCurrentControl();
      int gsi = message.GetSenderId();
      int gci = message.GetControlId();

    }
    break;
  case GUI_MSG_SETFOCUS:
    {

      // This fixes the SetFocus(50-59) it will automaticall set focus to the selected type not indivual control
      bool hs = m_viewControl.HasControl(message.GetControlId());
      int GCC = m_viewControl.GetCurrentControl();
      int gci = message.GetControlId();
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
      if (CGUIWindow::OnMessage(message))
      {
        // check if our focused control is one of our category buttons
        int iControl=message.GetControlId();
			  if(pCallbackWindow)
			  {
				  PyXBMCAction* inf = new PyXBMCAction;
				  inf->pObject = NULL;
          // create a new call and set it in the python queue
				  inf->pCallbackWindow = pCallbackWindow;
          inf->controlId = iControl;
				  // aquire lock?
				  Py_AddPendingCall(Py_XBMC_Event_OnFocus, inf);
				  PulseActionEvent();
        }
      }
    }
		case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      // Handle Sort/View internally. Scripters shouldn't use ID 2, 3 or 4

      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        if (m_guiState.get())
          m_guiState->SaveViewAsControl(m_viewControl.GetNextViewMode());
        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        CLog::Log(LOGINFO,"WindowXML: Internal asc/dsc button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortOrder();
        UpdateFileList();*/
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        CLog::Log(LOGINFO,"WindowXML: Internal sort button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortMethod();
        UpdateFileList();*/
        return true;
      }

			if(pCallbackWindow && iControl && iControl != this->GetID()) // pCallbackWindow &&  != this->GetID()) 
			{
				PyXBMCAction* inf = new PyXBMCAction;
				inf->pObject = NULL;
        // create a new call and set it in the python queue
				inf->pCallbackWindow = pCallbackWindow;
        inf->controlId = iControl;
				// aquire lock?
        Py_AddPendingCall(Py_XBMC_Event_OnClick, inf);
				PulseActionEvent();
      }
        // Currently we assume that your not using addContorl etc so the vector list of controls has nothing so nothing to check for anyway
        /*
				// find python control object with same iControl
				std::vector<Control*>::iterator it = ((Window*)pCallbackWindow)->vecControls.begin();
				while (it != ((Window*)pCallbackWindow)->vecControls.end())
				{
					Control* pControl = *it;
					if (pControl->iControlId == iControl)
					{
						inf->pObject = (PyObject*)pControl;
						break;
					}
					++it;
				}
				// did we find our control?
				if (inf->pObject)
				{
					// currently we only accept messages from a button or controllist with a select action
          if ((ControlList_CheckExact(inf->pObject) && (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK))||
					    ControlButton_CheckExact(inf->pObject) ||
              ControlCheckMark_CheckExact(inf->pObject))
					{
						// create a new call and set it in the python queue
						inf->pCallbackWindow = pCallbackWindow;

						// aquire lock?
						Py_AddPendingCall(Py_XBMC_Event_OnClick, inf);
						PulseActionEvent();
					}
				}*/
		}
		break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindowXML::AddItem(CFileItem * fileItem, bool bRefreshList)
{
  m_vecItems.Add(fileItem);
  if (bRefreshList)
  {
    m_viewControl.SetItems(m_vecItems);
    UpdateButtons();
  }
}

void CGUIPythonWindowXML::RefreshList()
{
  m_viewControl.SetItems(m_vecItems);
  UpdateButtons();
}

void CGUIPythonWindowXML::ClearList()
{
  m_viewControl.Clear();
  m_vecItems.Clear();
  m_viewControl.SetItems(m_vecItems);
  UpdateButtons();
}

void CGUIPythonWindowXML::WaitForActionEvent(DWORD timeout)
{
	WaitForSingleObject(m_actionEvent, timeout);
	ResetEvent(m_actionEvent);
}

void CGUIPythonWindowXML::PulseActionEvent()
{
	SetEvent(m_actionEvent);
}

void CGUIPythonWindowXML::Activate(DWORD dwParentId)
{
  // Currently not used
	m_dwParentWindowID = dwParentId;
	m_pParentWindow = m_gWindowManager.GetWindow(m_dwParentWindowID);
	if (!m_pParentWindow)
	{
		m_dwParentWindowID=0;
		return;
	}

	m_gWindowManager.RouteToWindow(this);

  // active this dialog...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
  OnMessage(msg);
  m_bRunning = true;
}

int Py_XBMC_Event_OnClick(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;
		PyObject_CallMethod(action->pCallbackWindow, "onClick", "i", action->controlId);
		delete action;
	}
	return 0;
}

int Py_XBMC_Event_OnFocus(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;
		PyObject_CallMethod(action->pCallbackWindow, "onFocus", "i", action->controlId);
		delete action;
	}
	return 0;
}

int Py_XBMC_Event_OnInit(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;
		PyObject_CallMethod(action->pCallbackWindow, "onInit",""); //, "O", &self);
		delete action;
	}
	return 0;
}

/// Functions Below here are speceifc for the 'MediaWindow' Like stuff (such as Sort and View)

// \brief Prepares and adds the fileitems list/thumb panel
void CGUIPythonWindowXML::OnSort()
{
  FormatItemLabels();
  SortItems(m_vecItems);
}

// \brief Formats item labels based on the formatting provided by guiViewState
void CGUIPythonWindowXML::FormatItemLabels()
{
  if (!m_guiState.get())
    return;

  CGUIViewState::LABEL_MASKS labelMasks;
  m_guiState->GetSortMethodLabelMasks(labelMasks);
}
// \brief Sorts Fileitems based on the sort method and sort oder provided by guiViewState
void CGUIPythonWindowXML::SortItems(CFileItemList &items)
{
  auto_ptr<CGUIViewState> guiState(CGUIViewState::GetViewState(GetID(), items));

  if (guiState.get())
  {
    items.Sort(guiState->GetSortMethod(), guiState->GetDisplaySortOrder());

    // Should these items be saved to the hdd
    if (items.GetCacheToDisc())
      items.Save();
  }
}
// \brief Synchonize the fileitems with the playlistplayer
// It recreated the playlist of the playlistplayer based
// on the fileitems of the window
void CGUIPythonWindowXML::UpdateFileList()
{
  int nItem = m_viewControl.GetSelectedItem();
  CFileItem* pItem = m_vecItems[nItem];
  const CStdString& strSelected = pItem->m_strPath;

  OnSort();
  UpdateButtons();

  m_viewControl.SetItems(m_vecItems);
  m_viewControl.SetSelectedItem(strSelected);
}
// \brief Updates the states (enable, disable, visible...)
// of the controls defined by this window
// Override this function in a derived class to add new controls
void CGUIPythonWindowXML::UpdateButtons()
{
  if (m_guiState.get())
  {
    // Update sorting controls
    if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTASC);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTASC);
      if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_ASC)
      {
        CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
      else
      {
        CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
    }

    // Update list/thumb control
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl());

    // Update sort by button
    if (m_guiState->GetSortMethod()==SORT_METHOD_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTBY);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTBY);
    }
    CStdString sortLabel;
    sortLabel.Format(g_localizeStrings.Get(550).c_str(), g_localizeStrings.Get(m_guiState->GetSortMethodLabel()).c_str());
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, sortLabel);
  }

  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->IsParentFolder()) iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);
}

/*void CGUIPythonWindowXML::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}*/

void CGUIPythonWindowXML::OnInitWindow()
{
  // Update list/thumb control
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  //Update();
  m_viewControl.SetFocused();
  SET_CONTROL_VISIBLE(CONTROL_LIST);
  CGUIWindow::OnInitWindow();
}

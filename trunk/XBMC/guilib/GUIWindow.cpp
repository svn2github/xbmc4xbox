#include "guiwindow.h"
#include "texturemanager.h"
#include "tinyxml/tinyxml.h"
#include "../xbmc/utils/log.h"
#include "GUIControlFactory.h"
#include "guibuttoncontrol.h"
#include "guiRadiobuttoncontrol.h"
#include "guiSpinControl.h"
#include "guiListControl.h"
#include "guiImage.h"
#include "GUILabelControl.h"
#include "GUIFadeLabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIMButtonControl.h"
#include "GUIToggleButtonControl.h" 
#include "GUITextBox.h" 
#include "guiVideoControl.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"

#include<string>
using namespace std;

CStdString CGUIWindow::CacheFilename = "";
CGUIWindow::VECREFERENCECONTOLS CGUIWindow::ControlsCache;

CGUIWindow::CGUIWindow(DWORD dwID)
{
  m_dwWindowId=dwID;
  m_dwPreviousWindowId=WINDOW_HOME;
  m_dwDefaultFocusControlID=0;
}

CGUIWindow::~CGUIWindow(void)
{
}

void CGUIWindow::FlushReferenceCache()
{
	CacheFilename.clear();
	ControlsCache.clear();
}

bool CGUIWindow::LoadReference(const CStdString& strFileName, VECREFERENCECONTOLS& controls)
{
	// load references.xml
	controls.clear();
	TiXmlDocument xmlDoc;
	int iPos = strFileName.ReverseFind('\\');
	CStdString strReferenceFile=strFileName.Left(iPos);
	strReferenceFile += "\\references.xml";

	// this takes ages and happens about 20 times per skin load.
	// caching the data speeds up skin loading by a factor of 2. :)
	if (CacheFilename == strReferenceFile)
	{
		for (IVECREFERENCECONTOLS it = ControlsCache.begin(); it != ControlsCache.end(); ++it)
		{
			stReferenceControl stControl;
			strcpy(stControl.m_szType,it->m_szType);
			if (!strcmp(it->m_szType,"label"))
			{
				stControl.m_pControl = new CGUILabelControl(*((CGUILabelControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"videowindow"))
			{
				stControl.m_pControl = new CGUIVideoControl(*((CGUIVideoControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"fadelabel"))
			{
				stControl.m_pControl = new CGUIFadeLabelControl(*((CGUIFadeLabelControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"button"))
			{
				stControl.m_pControl = new CGUIButtonControl(*((CGUIButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"togglebutton"))
			{
				stControl.m_pControl = new CGUIToggleButtonControl(*((CGUIToggleButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"buttonM"))
			{
				stControl.m_pControl = new CGUIMButtonControl(*((CGUIMButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"checkmark"))
			{
				stControl.m_pControl = new CGUICheckMarkControl(*((CGUICheckMarkControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"radiobutton"))
			{
				stControl.m_pControl = new CGUIRadioButtonControl(*((CGUIRadioButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"spincontrol"))
			{
				stControl.m_pControl = new CGUISpinControl(*((CGUISpinControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"slider"))
			{
				stControl.m_pControl= new CGUISliderControl(*((CGUISliderControl*)it->m_pControl));
			} 
			else if (!strcmp(it->m_szType,"progress"))
			{
				stControl.m_pControl= new CGUIProgressControl(*((CGUIProgressControl*)it->m_pControl));
			} 
			else if (!strcmp(it->m_szType,"image"))
			{
				stControl.m_pControl = new CGUIImage(*((CGUIImage*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"listcontrol"))
			{
				stControl.m_pControl = new CGUIListControl(*((CGUIListControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"textbox"))
			{
				stControl.m_pControl = new CGUITextBox(*((CGUITextBox*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"thumbnailpanel"))
			{
				stControl.m_pControl = new CGUIThumbnailPanel(*((CGUIThumbnailPanel*)it->m_pControl));
			}
			controls.push_back(stControl);
		}
		return true;
	}

	if ( !xmlDoc.LoadFile(strReferenceFile.c_str()) )
	{
    CLog::Log("unable to load:%s", strReferenceFile.c_str());
		return false;
	}

	TiXmlElement* pRootElement =xmlDoc.RootElement();
	CStdString strValue=pRootElement->Value();
	if (strValue!=CStdString("controls")) 
  {
    CLog::Log("references.xml doesnt contain <controls>");
    return false;
  }

	CGUIControlFactory factory;
	string strType;
	const TiXmlNode *pControl = pRootElement->FirstChild();
	while (pControl)
	{
		TiXmlNode* pNode=pControl->FirstChild("type");
		if (pNode)
		{
			strType = pNode->FirstChild()->Value();
			CGUIControl* pGUIControl = factory.Create(m_dwWindowId,pControl,NULL,true);
			if (pGUIControl)
			{	
				struct stReferenceControl stControl;
				strcpy(stControl.m_szType,strType.c_str());
				stControl.m_pControl=pGUIControl;
				controls.push_back(stControl);
			}
		}
		pControl=pControl->NextSibling();
	}

	CacheFilename = strReferenceFile;
	ControlsCache.clear();
	for (IVECREFERENCECONTOLS it = controls.begin(); it != controls.end(); ++it)
	{
		stReferenceControl stControl;
		strcpy(stControl.m_szType,it->m_szType);
		if (!strcmp(it->m_szType,"label"))
		{
			stControl.m_pControl = new CGUILabelControl(*((CGUILabelControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"videowindow"))
		{
			stControl.m_pControl = new CGUIVideoControl(*((CGUIVideoControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"fadelabel"))
		{
			stControl.m_pControl = new CGUIFadeLabelControl(*((CGUIFadeLabelControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"button"))
		{
			stControl.m_pControl = new CGUIButtonControl(*((CGUIButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"togglebutton"))
		{
			stControl.m_pControl = new CGUIToggleButtonControl(*((CGUIToggleButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"buttonM"))
		{
			stControl.m_pControl = new CGUIMButtonControl(*((CGUIMButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"checkmark"))
		{
			stControl.m_pControl = new CGUICheckMarkControl(*((CGUICheckMarkControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"radiobutton"))
		{
			stControl.m_pControl = new CGUIRadioButtonControl(*((CGUIRadioButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"spincontrol"))
		{
			stControl.m_pControl = new CGUISpinControl(*((CGUISpinControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"slider"))
		{
			stControl.m_pControl= new CGUISliderControl(*((CGUISliderControl*)it->m_pControl));
		} 
		else if (!strcmp(it->m_szType,"progress"))
		{
			stControl.m_pControl= new CGUIProgressControl(*((CGUIProgressControl*)it->m_pControl));
		} 
		else if (!strcmp(it->m_szType,"image"))
		{
			stControl.m_pControl = new CGUIImage(*((CGUIImage*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"listcontrol"))
		{
			stControl.m_pControl = new CGUIListControl(*((CGUIListControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"textbox"))
		{
			stControl.m_pControl = new CGUITextBox(*((CGUITextBox*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"thumbnailpanel"))
		{
			stControl.m_pControl = new CGUIThumbnailPanel(*((CGUIThumbnailPanel*)it->m_pControl));
		}
		ControlsCache.push_back(stControl);
	}

	return true;
}

bool CGUIWindow::Load(const CStdString& strFileName)
{
	TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile(strFileName.c_str()) )
  {
    CLog::Log("unable to load:%s", strFileName.c_str());
		m_dwWindowId=9999;
    return false;
  }
  TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
  if (strValue!=CStdString("window")) 
  {
    CLog::Log("file :%s doesnt contain <window>", strFileName.c_str());
    return false;
  }
  
  m_dwDefaultFocusControlID=0;
	
	VECREFERENCECONTOLS  referencecontrols;
	IVECREFERENCECONTOLS it;
	LoadReference(strFileName, referencecontrols);
 
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue=pChild->Value();
    if (strValue=="id")
    {
      m_dwWindowId=atoi(pChild->FirstChild()->Value());
    }
    if (strValue=="defaultcontrol")
    {
      m_dwDefaultFocusControlID=atoi(pChild->FirstChild()->Value());
    }
    if (strValue=="controls")
    {
			
       const TiXmlNode *pControl = pChild->FirstChild();
       while (pControl)
       {
				 // get control type
				 TiXmlNode* pNode=pControl->FirstChild("type");
				 if (pNode)
				 {
					string strType = pNode->FirstChild()->Value();
					 
					// get reference control
					CGUIControl* pGUIReferenceControl=NULL;
					for (int i=0; i < (int)referencecontrols.size(); ++i)
					{
						struct stReferenceControl stControl=referencecontrols[i];
						if (strType==stControl.m_szType)
						{
							pGUIReferenceControl=stControl.m_pControl;
						}
					}
					CGUIControlFactory factory;
					CGUIControl* pGUIControl = factory.Create(m_dwWindowId,pControl,pGUIReferenceControl,false);
					if (pGUIControl)
					{
						Add(pGUIControl);
					}
				}
         pControl=pControl->NextSibling();
       }
    }

    pChild=pChild->NextSibling();
  }

	
	for (int i=0; i < (int)referencecontrols.size();++i)
	{
		struct stReferenceControl stControl=referencecontrols[i];
		delete stControl.m_pControl;
	}
  return true;
}

void CGUIWindow::Render()
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->Render();
  }
}

void CGUIWindow::OnAction(const CAction &action)
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    if (pControl->HasFocus() )
    {
      pControl->OnAction(action);
      return;
    }
  }

  // no control has focus?
  // set focus to the default control then

  CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(),m_dwDefaultFocusControlID);
  OnMessage(msg);
}


DWORD CGUIWindow::GetID(void) const
{
  return m_dwWindowId;
}

void CGUIWindow::SetID(DWORD dwID)
{
	m_dwWindowId = dwID;
}

DWORD CGUIWindow::GetPreviousWindowID(void) const
{
	return m_dwPreviousWindowId;
}

bool CGUIWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
      {
        AllocResources();
		if (message.GetParam1()!=WINDOW_INVALID)
		{
			m_dwPreviousWindowId = message.GetParam1();
		}
        CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwDefaultFocusControlID);
        g_graphicsContext.SendMessage(msg);
      }
      return true;
    break;

    
    case GUI_MSG_WINDOW_DEINIT:
      FreeResources();
      return true;
    break;

    case GUI_MSG_SETFOCUS:
    {
      if ( message.GetControlId() )
      {
        ivecControls i;
        for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl= *i;
          if (pControl->HasFocus() ) 
          {
            CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS,GetID(),pControl->GetID(),message.GetControlId());
            pControl->OnMessage(msgLostFocus);
          }
        }
        for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl= *i;

          if (pControl)
          {
            if ( message.GetControlId() == pControl->GetID() )
            {
              pControl->OnMessage(message);
            }
          }
        }
      }
      return true;
    }
    break;
  }    

  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    if (pControl)
    {
      if ( message.GetControlId() == pControl->GetID() )
      {
        return pControl->OnMessage(message);
      }
    }
  }
  return false;
}

void CGUIWindow::AllocResources()
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->AllocResources();
  }
  //g_TextureManager.Dump();
}

void CGUIWindow::FreeResources()
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->FreeResources();
  }
  //g_TextureManager.Dump();
}

void CGUIWindow::Add(CGUIControl* pControl)
{
  m_vecControls.push_back(pControl);
}

void CGUIWindow::Remove(DWORD dwId)
{
	ivecControls i = m_vecControls.begin();
  while(i != m_vecControls.end())
  {
    CGUIControl* pControl= *i;
		if (pControl->GetID() == dwId)
    {
			m_vecControls.erase(i);
			return;
    }
		++i;
  }
}

int CGUIWindow::GetFocusControl()
{
  for (int i=0; i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
    if (pControl->HasFocus()) return i;
  }
  return -1;
}
void CGUIWindow::SelectNextControl()
{
  int i=GetFocusControl()+1;
  while (1)
  {
    if ( i < 0 || i >= (int)m_vecControls.size() )
    {
      i=0;
    }
    if (m_vecControls[i]->CanFocus()) break;
    else i++;
  }
  CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS,GetID(),m_vecControls[i]->GetID() );
  g_graphicsContext.SendMessage(msgSetFocus);
}


void CGUIWindow::ClearAll()
{
  for (int i=0; i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
    delete pControl;
  }
	m_vecControls.erase(m_vecControls.begin(),m_vecControls.end());
}

const CGUIControl* 	CGUIWindow::GetControl(int iControl) const
{
	for (int i=0;i < (int)m_vecControls.size(); ++i)
  {
    const CGUIControl* pControl= m_vecControls[i];
		if (pControl->GetID() == iControl) return pControl;
  }
	return NULL;
}
int	CGUIWindow::GetFocusedControl() const
{
	for (int i=0;i < (int)m_vecControls.size(); ++i)
  {
    const CGUIControl* pControl= m_vecControls[i];
		if (pControl->HasFocus() ) return pControl->GetID();
  }
	return -1;
}

void CGUIWindow::ResetAllControls()
{
	for (int i=0;i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
		pControl->SetWidth( pControl->GetWidth() );
	}
}
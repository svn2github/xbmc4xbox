#pragma once
#include "xbapplicationex.h"
#include "GUIWindowManager.h"
#include "guiwindow.h"
#include "GUIMessage.h"
#include "GUIButtonControl.h"
#include "GUIImage.h"
#include "GUIFontManager.h"
#include "key.h"
#include "GUIWindowHome.h"
#include "GUIWindowPrograms.h"
#include "GUIWindowPictures.h"
#include "GUIWindowMyFiles.h"
#include "GUIWindowSettings.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogProgress.h"
#include "LocalizeStrings.h"
#include "keyboard/virtualkeyboard.h"

class CApplication :
  public CXBApplicationEx
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void FrameMove();
  virtual void Render();


  CGUIWindowHome        m_guiHome;
  CGUIWindowPrograms    m_guiPrograms;
	CGUIWindowPictures		m_guiPictures;
	CGUIDialogYesNo				m_guiDialogYesNo;
	CGUIDialogProgress		m_guiDialogProgress;
	CGUIWindowMyFiles			m_guiMyFiles;
	CGUIWindowSettings		m_guiSettings;
  CXBVirtualKeyboard    m_keyboard;
};

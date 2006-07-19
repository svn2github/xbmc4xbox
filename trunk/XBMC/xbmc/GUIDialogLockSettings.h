#pragma once

#include "GUIDialogSettings.h"

class CGUIDialogLockSettings : public CGUIDialogSettings
{
public:
  CGUIDialogLockSettings(void);
  virtual ~CGUIDialogLockSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  static bool ShowAndGetLock(int& iLockMode, CStdString& strPassword, int iHeader=20091);
  static bool ShowAndGetLock(int& iLockMode, CStdString& strPassword, bool& bLockMusic, bool& bLockVideo, bool& bLockPictures, bool& bLockPrograms, bool& bLockFiles, bool& bLockSettings, int iButtonLabel=20091,bool bDetails=true);
protected:
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void OnSettingChanged(unsigned int setting);

  int m_iLock;
  CStdString m_strLock;
  bool m_bChanged;
  bool m_bDetails;
  int m_iButtonLabel;
  bool m_bLockMusic; 
  bool m_bLockVideo;
  bool m_bLockPictures;
  bool m_bLockPrograms;
  bool m_bLockFiles;
  bool m_bLockSettings;
};
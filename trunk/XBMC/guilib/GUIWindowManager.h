/*!
\file GUIWindowManager.h
\brief 
*/

#ifndef GUILIB_CGUIWindowManager_H
#define GUILIB_CGUIWindowManager_H

#pragma once

#include "GUIWindow.h"
#include "IMsgSenderCallback.h"
#include "IWindowManagerCallback.h"
#include "IMsgTargetCallback.h"

/*!
 \ingroup winman
 \brief 
 */
class CGUIWindowManager: public IMsgSenderCallback
{
public:
  CGUIWindowManager(void);
  virtual ~CGUIWindowManager(void);
  virtual bool SendMessage(CGUIMessage& message);
  void Initialize();
  void Add(CGUIWindow* pWindow);
  void AddCustomWindow(CGUIWindow* pWindow);
  void AddModeless(CGUIWindow* pWindow);
  void Remove(DWORD dwID);
  void Delete(DWORD dwID);
  void RemoveModeless(DWORD dwID);
  void ActivateWindow(int iWindowID, const CStdString& strPath = "", bool swappingWindows = false);
  void ChangeActiveWindow(int iNewID, const CStdString& strPath = "");
  void PreviousWindow();
  void RefreshWindow();
  void LoadNotOnDemandWindows();
  void UnloadNotOnDemandWindows();

  // OnAction() runs through our active dialogs and windows and sends the message
  // off to the callbacks (application, python, playlist player) and to the
  // currently focused window(s).  Returns true only if the message is handled.
  bool OnAction(const CAction &action);

  void Render();
  void RenderDialogs();
  CGUIWindow* GetWindow(DWORD dwID);
  void Process(bool renderOnly = false);
  void SetCallback(IWindowManagerCallback& callback);
  void DeInitialize();
  void RouteToWindow(CGUIWindow* pWindow);
  void UnRoute(DWORD dwID);
  int GetTopMostRoutedWindowID() const;
  void SendThreadMessage(CGUIMessage& message);
  void DispatchThreadMessages();
  void AddMsgTarget( IMsgTargetCallback* pMsgTarget );
  int GetActiveWindow() const;
  bool IsRouted(bool includeFadeOuts = false) const;
  bool IsModelessAvailable() const;
  void UpdateModelessVisibility();
  bool IsWindowActive(DWORD dwID, bool ignoreClosing = true) const;
  bool IsWindowVisible(DWORD id) const;
  bool IsOverlayAllowed() const;
  void ShowOverlay(bool bOnOff);
  void GetActiveModelessWindows(vector<DWORD> ids);

private:
  void AddToWindowHistory(DWORD newWindowID);
  map<DWORD, CGUIWindow *> m_mapWindows;
  vector <CGUIWindow*> m_vecModelessWindows;
  vector <CGUIWindow*> m_vecModalWindows;
  vector <CGUIWindow*> m_vecCustomWindows;

  stack<DWORD> m_windowHistory;

  IWindowManagerCallback* m_pCallback;
  vector <CGUIMessage*> m_vecThreadMessages;
  CRITICAL_SECTION m_critSection;
  vector <IMsgTargetCallback*> m_vecMsgTargets;

  bool m_bShowOverlay;
};

/*!
 \ingroup winman
 \brief 
 */
extern CGUIWindowManager m_gWindowManager;
#endif
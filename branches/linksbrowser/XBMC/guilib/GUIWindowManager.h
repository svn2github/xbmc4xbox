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

class CGUIDialog;

#define WINDOW_ID_MASK 0xffff

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
  void AddUniqueInstance(CGUIWindow *window);
  void AddCustomWindow(CGUIWindow* pWindow);
  void Remove(DWORD dwID);
  void Delete(DWORD dwID);
  void ActivateWindow(int iWindowID, const CStdString& strPath = "", bool swappingWindows = false);
  void ChangeActiveWindow(int iNewID, const CStdString& strPath = "");
  void PreviousWindow();
  void RefreshWindow();
  void LoadNotOnDemandWindows();
  void UnloadNotOnDemandWindows();

  void CloseDialogs(bool forceClose = false);

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

  void RouteToWindow(CGUIWindow* dialog);
  void AddModeless(CGUIWindow* dialog);
  void RemoveDialog(DWORD dwID);
  int GetTopMostDialogID() const;

  void SendThreadMessage(CGUIMessage& message);
  void DispatchThreadMessages();
  void AddMsgTarget( IMsgTargetCallback* pMsgTarget );
  int GetActiveWindow() const;
  bool HasModalDialog() const;
  bool HasDialogOnScreen() const;
  void UpdateModelessVisibility();
  bool IsWindowActive(DWORD dwID, bool ignoreClosing = true) const;
  bool IsWindowVisible(DWORD id) const;
  bool IsWindowTopMost(DWORD id) const;
  bool IsOverlayAllowed() const;
  void ShowOverlay(CGUIWindow::OVERLAY_STATE state);
  void GetActiveModelessWindows(vector<DWORD> &ids);
#ifdef _DEBUG
  void DumpTextureUse();
#endif
  bool m_bPointerNav; // Pointer navigation mode, on if TRUE

private:
  void HideOverlay(CGUIWindow::OVERLAY_STATE state);
  void AddToWindowHistory(DWORD newWindowID);
  void ClearWindowHistory();
  map<DWORD, CGUIWindow *> m_mapWindows;
  vector <CGUIWindow*> m_vecCustomWindows;
  vector <CGUIWindow*> m_activeDialogs;
  typedef vector<CGUIWindow*>::iterator iDialog;
  typedef vector<CGUIWindow*>::const_iterator ciDialog;
  typedef vector<CGUIWindow*>::reverse_iterator rDialog;
  typedef vector<CGUIWindow*>::const_reverse_iterator crDialog;

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
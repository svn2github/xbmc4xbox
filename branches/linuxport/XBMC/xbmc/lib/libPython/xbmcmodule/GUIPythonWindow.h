#pragma once
#include "GUIWindow.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#endif

class PyXBMCAction
{
public:
  DWORD dwParam;
  PyObject* pCallbackWindow;
  PyObject* pObject;
  int controlId; // for XML window
#ifdef _LINUX
  int type; // 0=Action, 1=Control;
#endif
};

#ifdef _LINUX
void Py_InitCriticalSection();
void Py_AddPendingActionCall(PyXBMCAction* inf);
void Py_MakePendingActionCalls();
#endif
int Py_XBMC_Event_OnAction(void* arg);
int Py_XBMC_Event_OnControl(void* arg);

class CGUIPythonWindow : public CGUIWindow
{
public:
  CGUIPythonWindow(DWORD dwId);
  virtual ~CGUIPythonWindow(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual bool    OnAction(const CAction &action);
  void             SetCallbackWindow(PyObject *object);
  void             WaitForActionEvent(DWORD timeout);
  void             PulseActionEvent();
protected:
  PyObject*        pCallbackWindow;
  HANDLE           m_actionEvent;
};

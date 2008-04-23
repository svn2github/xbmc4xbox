#pragma once

#ifndef _LINUX
#include "lib/libPython/python/Python.h"
#else
#include <python2.4/Python.h>
#endif
#include "cores/IPlayer.h"


int Py_XBMC_Event_OnPlayBackStarted(void* arg);
int Py_XBMC_Event_OnPlayBackEnded(void* arg);
int Py_XBMC_Event_OnPlayBackStopped(void* arg);

class CPythonPlayer : public IPlayerCallback
{
public:
  CPythonPlayer();
  virtual ~CPythonPlayer(void);
  void    SetCallback(PyObject *object);
  void    OnPlayBackStarted();
  void    OnPlayBackEnded();
  void    OnPlayBackStopped();
  void    OnQueueNextItem() {}; // unimplemented

protected:
  PyObject*   pCallback;
};

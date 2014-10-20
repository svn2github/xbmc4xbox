#ifndef XBPYTHREAD_H_
#define XBPYTHREAD_H_

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "python/Include/Python.h"
#include "utils/Thread.h"
#include "utils/StringUtils.h"

class XBPython;

class XBPyThread : public CThread
{
public:
  XBPyThread(XBPython *pExecuter, int id);
  virtual ~XBPyThread();
  int evalFile(const CStdString &src);
  int evalString(const CStdString &src);
  int setArgv(const std::vector<CStdString> &argv);
  bool isStopping();
  void stop();

protected:
  XBPython      *m_pExecuter;
  PyThreadState *m_threadState;

  char m_type;
  char *m_source;
  char **m_argv;
  unsigned int  m_argc;
  bool m_stopping;
  int  m_id;

  virtual void OnStartup();
  virtual void Process();
  virtual void OnExit();
  virtual void OnException();
};

#endif // XBPYTHREAD_H_

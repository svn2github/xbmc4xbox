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

// python.h should always be included first before any other includes
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "Python/Include/Python.h"
#endif
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"

#include "XBPython.h"
#include "XBPythonDll.h"
#include "Settings.h"
#include "Profile.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/log.h"

XBPython g_pythonParser;

#ifndef _LINUX
#define PYTHON_DLL "special://xbmc/system/python/python24.dll"
#else
#if defined(__APPLE__)
#if defined(__POWERPC__)
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc-osx.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-x86-osx.so"
#endif
#elif defined(__x86_64__)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-x86_64-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-x86_64-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-x86_64-linux.so"
#endif
#elif defined(__powerpc__)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-powerpc-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-powerpc-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc-linux.so"
#endif
#else /* !__x86_64__ && !__powerpc__ */
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-i486-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-i486-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-i486-linux.so"
#endif
#endif /* __x86_64__ */
#endif /* _LINUX */

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);

extern "C" {
  void InitXBMCModule(void);
  void InitXBMCTypes(void);
  void DeinitXBMCModule(void);
  void InitPluginModule(void);
  void InitPluginTypes(void);
  void DeinitPluginModule(void);
  void InitGUIModule(void);
  void InitGUITypes(void);
  void DeinitGUIModule(void);
}

XBPython::XBPython()
{
  m_bInitialized = false;
  bStartup = false;
  bLogin = false;
  nextid = 0;
  mainThreadState = NULL;
  InitializeCriticalSection(&m_critSection);
  m_hEvent = CreateEvent(NULL, false, false, (char*)"pythonEvent");
  m_globalEvent = CreateEvent(NULL, false, false, (char*)"pythonGlobalEvent");
  dThreadId = GetCurrentThreadId();
  vecPlayerCallbackList.clear();
  m_iDllScriptCounter = 0;
}

XBPython::~XBPython()
{
  CloseHandle(m_globalEvent);
}

bool XBPython::SendMessage(CGUIMessage& message)
{
  return (evalFile(message.GetStringParam().c_str()) != -1);
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackEnded();
      it++;
    }
  }
}

// message all registered callbacks that we started playing
void XBPython::OnPlayBackStarted()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStarted();
      it++;
    }
  }
}

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
      it++;
    }
  }
}

void XBPython::RegisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
  while (it != vecPlayerCallbackList.end())
  {
    if (*it == pCallback)
    {
      it = vecPlayerCallbackList.erase(it);
    }
    else it++;
  }
}

/**
* Check for file and print an error if needed
*/
bool XBPython::FileExist(const char* strFile)
{
  if (!strFile) return false;

  if (!XFILE::CFile::Exists(strFile))
  {
    CLog::Log(LOGERROR, "Python: Cannot find '%s'", strFile);
    return false;
  }
  return true;
}

void XBPython::RegisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib) 
    return;

  CLog::Log(LOGDEBUG,"%s, adding %s (%p)", __FUNCTION__, pLib->GetName(), (void*)pLib);
  EnterCriticalSection(&m_critSection);
  m_extensions.push_back(pLib);
  LeaveCriticalSection(&m_critSection);
}

void XBPython::UnregisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib) 
    return;

  CLog::Log(LOGDEBUG,"%s, removing %s (0x%p)", __FUNCTION__, pLib->GetName(), (void *)pLib);
  EnterCriticalSection(&m_critSection);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
    if (*iter == pLib)
    {
      m_extensions.erase(iter);
      break;
    }
    iter++;
  }
  LeaveCriticalSection(&m_critSection);
}

void XBPython::UnloadExtensionLibs()
{
  CLog::Log(LOGDEBUG,"%s, clearing python extension libraries", __FUNCTION__);
  EnterCriticalSection(&m_critSection);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
      DllLoaderContainer::ReleaseModule(*iter);
      iter++;
  }

  m_extensions.clear();
  LeaveCriticalSection(&m_critSection);
}

void XBPython::InitializeInterpreter()
{
  InitXBMCModule(); // init xbmc modules
  InitPluginModule(); // init plugin modules
  InitGUIModule(); // init xbmcgui modules

  // redirecting default output to debug console
  if (PyRun_SimpleString(""
        "import xbmc\n"
        "class xbmcout:\n"
        "	def write(self, data):\n"
        "		xbmc.output(data)\n"
        "	def close(self):\n"
        "		xbmc.output('.')\n"
        "	def flush(self):\n"
        "		xbmc.output('.')\n"
        "\n"
        "import sys\n"
        "sys.stdout = xbmcout()\n"
        "sys.stderr = xbmcout()\n"
        "print '-->Python Interpreter Initialized<--'\n"
        "") == -1)
  {
    CLog::Log(LOGFATAL, "Python Initialize Error");
  }
}

void XBPython::DeInitializeInterpreter()
{
  DeinitXBMCModule(); 
  DeinitPluginModule(); 
  DeinitGUIModule(); 
}

/**
* Should be called before executing a script
*/
void XBPython::Initialize()
{
  CLog::Log(LOGINFO, "initializing python engine. ");
  EnterCriticalSection(&m_critSection);
  m_iDllScriptCounter++;
  if (!m_bInitialized)
  {
    if (dThreadId == GetCurrentThreadId())
    {
      m_pDll = DllLoaderContainer::LoadModule(PYTHON_DLL, NULL, true);

      if (!m_pDll || !python_load_dll(*m_pDll))
      {
        CLog::Log(LOGFATAL, "Python: error loading python24.dll");
        Finalize();
        LeaveCriticalSection(&m_critSection);
        return;
      }

      // first we check if all necessary files are installed
#ifndef _LINUX      
      if (!FileExist("special://xbmc/system/python/python24.zlib") ||
        !FileExist("special://xbmc/system/python/DLLs/_socket.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/_ssl.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/bz2.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/pyexpat.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/select.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/unicodedata.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/zlib.pyd"))
      {
        CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
        Finalize();
        LeaveCriticalSection(&m_critSection);
        return;
      }
#endif        

#if (!defined USE_EXTERNAL_PYTHON)
#ifdef _LINUX
      // Required for python to find optimized code (pyo) files
      setenv("PYTHONOPTIMIZE", "1", 1);
      setenv("PYTHONHOME", _P("special://xbmc/system/python").c_str(), 1);
#ifdef __APPLE__
      // OSX uses contents from extracted zip, 3X to 4X times faster during Py_Initialize
      setenv("PYTHONPATH", _P("special://xbmc/system/python/Lib").c_str(), 1);
#else
      setenv("PYTHONPATH", _P("special://xbmc/system/python/python24.zip").c_str(), 1);
#endif /* __APPLE__ */
      //setenv("PYTHONDEBUG", "1", 1);
      //setenv("PYTHONINSPECT", "1", 1);
      //setenv("PYTHONVERBOSE", "1", 1);
      setenv("PYTHONCASEOK", "1", 1);
      CLog::Log(LOGDEBUG, "Python wrapper library linked with internal Python library");
#endif /* _LINUX */
#else
      /* PYTHONOPTIMIZE is set off intentionally when using external Python.
         Reason for this is because we cannot be sure what version of Python
         was used to compile the various Python object files (i.e. .pyo,
         .pyc, etc.). */
      //setenv("PYTHONOPTIMIZE", "1", 1);
      //setenv("PYTHONDEBUG", "1", 1);
      //setenv("PYTHONINSPECT", "1", 1);
      //setenv("PYTHONVERBOSE", "1", 1);
      setenv("PYTHONCASEOK", "1", 1); //This line should really be removed
      CLog::Log(LOGDEBUG, "Python wrapper library linked with system Python library");
#endif /* USE_EXTERNAL_PYTHON */

      Py_Initialize();
      PyEval_InitThreads();

      char* python_argv[1] = { (char*)"" } ;
      PySys_SetArgv(1, python_argv);

      InitXBMCTypes();
      InitGUITypes();
      InitPluginTypes();

      mainThreadState = PyThreadState_Get();

      // release the lock
      PyEval_ReleaseLock();

      m_bInitialized = true;
      PulseEvent(m_hEvent);
    }
    else
    {
      // only the main thread should initialize python.
      m_iDllScriptCounter--;

      LeaveCriticalSection(&m_critSection);
      WaitForSingleObject(m_hEvent, INFINITE);
      EnterCriticalSection(&m_critSection);
    }
  }
  LeaveCriticalSection(&m_critSection);
}

/**
* Should be called when a script is finished
*/
void XBPython::Finalize()
{
  // for linux - we never release the library. its loaded and stays in memory.
  EnterCriticalSection(&m_critSection);
  m_iDllScriptCounter--;
  if (m_iDllScriptCounter == 0 && m_bInitialized)
  {
    CLog::Log(LOGINFO, "Python, unloading python24.dll because no scripts are running anymore");
    PyEval_AcquireLock();
    PyThreadState_Swap(mainThreadState);
    Py_Finalize();

    UnloadExtensionLibs();

    // first free all dlls loaded by python, after that python24.dll (this is done by UnloadPythonDlls
    DllLoaderContainer::UnloadPythonDlls();
#ifdef _LINUX
    // we can't release it on windows, as this is done in UnloadPythonDlls() for win32 (see above).
    // The implementation for linux and os x needs looking at - UnloadPythonDlls() currently only searches for "python24.dll"
    DllLoaderContainer::ReleaseModule(m_pDll);
#endif
    m_hModule = NULL;
    mainThreadState = NULL;
    m_bInitialized = false;
  }
  LeaveCriticalSection(&m_critSection);
}

void XBPython::FreeResources()
{
  EnterCriticalSection(&m_critSection);
  if (m_bInitialized)
  {
    // cleanup threads that are still running
    PyList::iterator it = vecPyList.begin();
    while (it != vecPyList.end())
    { 
      LeaveCriticalSection(&m_critSection); //unlock here because the python thread might lock when it exits
      delete it->pyThread;
      EnterCriticalSection(&m_critSection);
      it = vecPyList.erase(it);
      Finalize();
    }
  }
  LeaveCriticalSection(&m_critSection);

  if (m_hEvent)
    CloseHandle(m_hEvent);

  DeleteCriticalSection(&m_critSection);
}

void XBPython::Process()
{
  if (bStartup)
  {
    bStartup = false;
    if (evalFile("special://home/scripts/autoexec.py") < 0)
      evalFile("special://xbmc/scripts/autoexec.py");
  }

  if (bLogin)
  {
    bLogin = false;
    evalFile("special://profile/scripts/autoexec.py");
  }

  EnterCriticalSection(&m_critSection);
  if (m_bInitialized)
  {
    PyList::iterator it = vecPyList.begin();
    while (it != vecPyList.end())
    {
      //delete scripts which are done
      if (it->bDone)
      {
        delete it->pyThread;
        it = vecPyList.erase(it);
        Finalize();
      }
      else ++it;
    }
  }
  LeaveCriticalSection(&m_critSection );
}

int XBPython::evalFile(const char *src) { return evalFile(src, 0, NULL); }
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const char *src, const unsigned int argc, const char ** argv)
{
  // return if file doesn't exist
  if (!XFILE::CFile::Exists(src))
  {
    CLog::Log(LOGERROR, "Python script \"%s\" does not exist", src);
    return -1;
  }

  // check if locked
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return -1;

  Initialize();

  if (!m_bInitialized) return -1;

  nextid++;
  XBPyThread *pyThread = new XBPyThread(this, nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalFile(src);
  PyElem inf;
  inf.id = nextid;
  inf.bDone = false;
  inf.strFile = src;
  inf.pyThread = pyThread;

  EnterCriticalSection(&m_critSection);
  vecPyList.push_back(inf);
  LeaveCriticalSection(&m_critSection);

  return nextid;
}

void XBPython::setDone(int id)
{
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == id)
    {
      if (it->pyThread->isStopping())
        CLog::Log(LOGINFO, "Python script interrupted by user");
      else
        CLog::Log(LOGINFO, "Python script stopped");
      it->bDone = true;
    }
    ++it;
  }
  LeaveCriticalSection(&m_critSection);
}

void XBPython::stopScript(int id)
{
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == id) {
      CLog::Log(LOGINFO, "Stopping script with id: %i", id);
      it->pyThread->stop();
      LeaveCriticalSection(&m_critSection);
      return;
    }
    ++it;
  }
  LeaveCriticalSection(&m_critSection );
}

PyThreadState *XBPython::getMainThreadState()
{
  return mainThreadState;
}

int XBPython::ScriptsSize()
{
  int iSize = 0;
  
  EnterCriticalSection(&m_critSection);
  iSize = vecPyList.size();
  LeaveCriticalSection(&m_critSection);

  return iSize;
}

const char* XBPython::getFileName(int scriptId)
{
  const char* cFileName = NULL;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId) cFileName = it->strFile.c_str();
    ++it;
  }
  LeaveCriticalSection(&m_critSection);

  return cFileName;
}

int XBPython::getScriptId(const char* strFile)
{
  int iId = -1;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (!stricmp(it->strFile.c_str(), strFile)) iId = it->id;
    ++it;
  }
  LeaveCriticalSection(&m_critSection);
  
  return iId;
}

bool XBPython::isRunning(int scriptId)
{
  bool bRunning = false;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId)	bRunning = true;
    ++it;
  }
  LeaveCriticalSection(&m_critSection);
  
  return bRunning;
}

bool XBPython::isStopping(int scriptId)
{
  bool bStopping = false;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId) bStopping = it->pyThread->isStopping();
    ++it;
  }
  LeaveCriticalSection(&m_critSection);
  
  return bStopping;
}

int XBPython::GetPythonScriptId(int scriptPosition)
{
  int iId = -1;

  EnterCriticalSection(&m_critSection);
  iId = (int)vecPyList[scriptPosition].id;
  LeaveCriticalSection(&m_critSection);

  return iId;
}

void XBPython::PulseGlobalEvent()
{
  SetEvent(m_globalEvent);
}

void XBPython::WaitForEvent(HANDLE hEvent, DWORD timeout)
{
  // wait for either this event our our global event
  HANDLE handles[2] = { hEvent, m_globalEvent };
  WaitForMultipleObjects(2, handles, FALSE, timeout);
  ResetEvent(m_globalEvent);
}

// execute script, returns -1 if script doesn't exist
int XBPython::evalString(const char *src, const unsigned int argc, const char ** argv)
{
  CLog::Log(LOGDEBUG, "XBPython::evalString (python)");
  
  Initialize();

  if (!m_bInitialized) 
  {
    CLog::Log(LOGERROR, "XBPython::evalString, python not initialized (python)");
    return -1;
  }

  // Previous implementation would create a new thread for every script
  nextid++;
  XBPyThread *pyThread = new XBPyThread(this, nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalString(src);
  
  PyElem inf;
  inf.id = nextid;
  inf.bDone = false;
  inf.strFile = "<string>";
  inf.pyThread = pyThread;

  EnterCriticalSection(&m_critSection );
  vecPyList.push_back(inf);
  LeaveCriticalSection(&m_critSection );

  return nextid;
}


#include "../../stdafx.h"
#include "DllLoaderContainer.h"

#include "dll_tracker.h" // for python unload hack

#define ENV_PATH "Q:\\system\\;Q:\\system\\players\\mplayer\\;Q:\\system\\players\\dvdplayer\\;Q:\\system\\players\\paplayer\\;Q:\\system\\python\\"

//Define this to get loggin on all calls to load/unload of dlls
//#define LOGALL

using namespace XFILE;

DllLoaderContainer g_dlls;

void export_reg();
void export_ole32();
void export_oleaut32();
void export_xbp();
void export_winmm();
void export_user32();
void export_msdmo();
void export_xbmc_vobsub();
void export_kernel32();
void export_wsock32();
void export_ws2_32();
void export_xbox_dx8();
void export_xbox___dx8();
void export_version();
void export_comdlg32();
void export_gdi32();
void export_ddraw();
void export_comctl32();
void export_msvcrt();
void export_msvcr71();
#ifdef WITH_LINKS_BROWSER
void export_xbox_other();
void export_winsockx();
#endif
void export_pncrt();
void export_iconvx();

DllLoaderContainer::DllLoaderContainer() :
    kernel32("kernel32.dll", false, true),
    msvcr71("msvcr71.dll", false, true),
    msvcrt("msvcrt.dll", false, true),
    wsock32("wsock32.dll", false, true),
    ws2_32("ws2_32.dll", false, true),
    user32("user32.dll", false, true),
    ddraw("ddraw.dll", false, true),
    wininet("wininet.dll", false, true),
    advapi32("advapi32.dll", false, true),
    ole32("ole32.dll", false, true),
    oleaut32("oleaut32.dll", false, true),
    xbp("xbp.dll", false, true),
    winmm("winmm.dll", false, true),
    msdmo("msdmo.dll", false, true),
    xbmc_vobsub("xbmc_vobsub.dll", false, true),
    xbox_dx8("xbox_dx8.dll", false, true),
    xbox___dx8("xbox-dx8.dll", false, true),
    version("version.dll", false, true),
    comdlg32("comdlg32.dll", false, true),
    gdi32("gdi32.dll", false, true),
    comctl32("comctl32.dll", false, true),
#ifdef WITH_LINKS_BROWSER
    xbox_other("xbox_other.dll", false, true),
    winsockx("winsockx.dll", false, true),
#endif
    pncrt("pncrt.dll", false, true),
    iconvx("iconv.dll", false, true)
{
  m_iNrOfDlls = 0;
  m_bTrack = true;
  
  RegisterDll(&kernel32); export_kernel32();
  RegisterDll(&msvcr71); export_msvcr71();
  RegisterDll(&msvcrt); export_msvcrt();
  RegisterDll(&wsock32); export_wsock32();
  RegisterDll(&ws2_32); export_ws2_32();
  RegisterDll(&user32); export_user32();
  RegisterDll(&ddraw); export_ddraw();
  RegisterDll(&wininet); // nothing is exported in this dll, is this one really needed?
  RegisterDll(&advapi32); export_reg();
  RegisterDll(&ole32); export_ole32();
  RegisterDll(&oleaut32); export_oleaut32();
  RegisterDll(&xbp); export_xbp();
  RegisterDll(&winmm); export_winmm();
  RegisterDll(&msdmo); export_msdmo();
  RegisterDll(&xbmc_vobsub); export_xbmc_vobsub();
  RegisterDll(&xbox_dx8); export_xbox_dx8();
  RegisterDll(&xbox___dx8); export_xbox___dx8();
  RegisterDll(&version); export_version();
  RegisterDll(&comdlg32); export_comdlg32();
  RegisterDll(&gdi32); export_gdi32();
  RegisterDll(&comctl32); export_comctl32();
#ifdef WITH_LINKS_BROWSER
  RegisterDll(&xbox_other); export_xbox_other();
  RegisterDll(&winsockx); export_winsockx();
#endif
  RegisterDll(&pncrt); export_pncrt();
  RegisterDll(&iconvx); export_iconvx();
}
  
void DllLoaderContainer::Clear()
{
}

HMODULE DllLoaderContainer::GetModuleAddress(const char* sName)
{
  return (HMODULE)GetModule(sName);
}

DllLoader* DllLoaderContainer::GetModule(const char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (stricmp(m_dlls[i]->GetName(), sName) == 0) return m_dlls[i];
    if (!m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetFileName(), sName) == 0) return m_dlls[i];
  }

  return NULL;
}

DllLoader* DllLoaderContainer::GetModule(HMODULE hModule)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (m_dlls[i]->hModule == hModule) return m_dlls[i];    
  }
  return NULL;
}

DllLoader* DllLoaderContainer::LoadModule(const char* sName, const char* sCurrentDir/*=NULL*/, bool bLoadSymbols/*=false*/)
{
  DllLoader* pDll=NULL;

  if (IsSystemDll(sName))
  {
    pDll = g_dlls.GetModule(sName);
  }
  else if (sCurrentDir)
  {
    CStdString strPath=sCurrentDir;
    strPath+=sName;
    pDll = g_dlls.GetModule(strPath.c_str());
  }
  
  if (!pDll)
  {
    pDll = g_dlls.GetModule(sName);
  }

  if (!pDll)
  {
    pDll=g_dlls.FindModule(sName, sCurrentDir, bLoadSymbols);
  }
  else if (!pDll->IsSystemDll())
  {
    pDll->IncrRef();

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Already loaded Dll %s at 0x%x", pDll->GetFileName(), pDll);
#endif

  }

  return pDll;
}

DllLoader* DllLoaderContainer::FindModule(const char* sName, const char* sCurrentDir, bool bLoadSymbols)
{
  if (strlen(sName) > 1 && sName[1] == ':')
  { //  Has a path, just try to load
    return LoadDll(sName, bLoadSymbols);
  }
  else if (sCurrentDir)
  { // in the path of the parent dll?
    CStdString strPath=sCurrentDir;
    strPath+=sName;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

  //  in environment variable?
  CStdStringArray vecEnv;
  StringUtils::SplitString(ENV_PATH, ";", vecEnv);

  for (int i=0; i<(int)vecEnv.size(); ++i)
  {
    CStdString strPath=vecEnv[i];

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Searching for the dll %s in directory %s", sName, strPath.c_str());
#endif

    strPath+=sName;

    // Have we already loaded this dll
    DllLoader* pDll = g_dlls.GetModule(strPath.c_str());
    if (pDll)
      return pDll;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

  CLog::Log(LOGDEBUG, "Dll %s was not found in path", sName);

  return NULL;
}

void DllLoaderContainer::ReleaseModule(DllLoader*& pDll)
{
  if (pDll->IsSystemDll())
  {
    CLog::Log(LOGFATAL, "%s is a system dll and should never be released", pDll->GetName());
    return;
  }

  int iRefCount=pDll->DecrRef();
  if (iRefCount==0)
  {

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Releasing Dll %s", pDll->GetFileName());
#endif

    if (!pDll->HasSymbols())
    {
      pDll->Unload();
      delete pDll;
      pDll=NULL;
    }
    else
      CLog::Log(LOGINFO, "%s has symbols loaded and can never be unloaded", pDll->GetName());
  }
#ifdef LOGALL
  else
  {
    CLog::Log(LOGDEBUG, "Dll %s is still referenced with a count of %d", pDll->GetFileName(), iRefCount);
  }
#endif
}

DllLoader* DllLoaderContainer::LoadDll(const char* sName, bool bLoadSymbols)
{

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "Loading dll %s", sName);
#endif

  DllLoader* pLoader = new DllLoader(sName, m_bTrack, false, bLoadSymbols);
  if (!pLoader)
  {
    CLog::Log(LOGERROR, "Unable to create dll %s", sName);
    return NULL;
  }

  if (!pLoader->Load())
  {
    delete pLoader;
    return NULL;
  }

  return pLoader;
}

bool DllLoaderContainer::IsSystemDll(const char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetName(), sName) == 0) return true;
  }

  return false;
}

int DllLoaderContainer::GetNrOfModules()
{
  return m_iNrOfDlls;
}

DllLoader* DllLoaderContainer::GetModule(int iPos)
{
  if (iPos < m_iNrOfDlls) return m_dlls[iPos];
  return NULL;
}

void DllLoaderContainer::RegisterDll(DllLoader* pDll)
{
  for (int i = 0; i < 64; i++)
  {
    if (m_dlls[i] == NULL)
    {
      m_dlls[i] = pDll;
      m_iNrOfDlls++;
      break;
    }
  }
}

void DllLoaderContainer::UnRegisterDll(DllLoader* pDll)
{
  if (pDll)
  {
    if (pDll->IsSystemDll())
    {
      CLog::Log(LOGFATAL, "%s is a system dll and should never be removed", pDll->GetName());
    }
    else
    {
      // remove from the list
      bool bRemoved = false;
      for (int i = 0; i < m_iNrOfDlls && m_dlls[i]; i++)
      {
        if (m_dlls[i] == pDll) bRemoved = true;
        if (bRemoved && i + 1 < m_iNrOfDlls)
        {
          m_dlls[i] = m_dlls[i + 1];
        }
      }
      if (bRemoved)
      {
        m_iNrOfDlls--;
        m_dlls[m_iNrOfDlls] = NULL;
      }
    }
  }
}

void DllLoaderContainer::UnloadPythonDlls()
{
  // unload all dlls that python24.dll could have loaded
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    char* name = m_dlls[i]->GetName();
    if (strstr(name, ".pyd") != NULL)
    {
      DllLoader* pDll = m_dlls[i];
      ReleaseModule(pDll);
      i = 0;
    }
  }

  // last dll to unload, python24.dll
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    char* name = m_dlls[i]->GetName();
    if (strstr(name, "python24.dll") != NULL)
    {
      DllLoader* pDll = m_dlls[i];
      pDll->IncrRef();
      while (pDll->DecrRef() > 1) pDll->DecrRef();
      
      // since we freed all python extension dlls first, we have to remove any associations with them first
      DllTrackInfo* info = tracker_get_dlltrackinfo_byobject(pDll);
      if (info != NULL)
      {
        info->dllList.clear();
      }
      
      ReleaseModule(pDll);
      break;
    }
  }

  
}
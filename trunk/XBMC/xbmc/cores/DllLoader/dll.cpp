
#include "stdafx.h"
#include "dll.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "dll_tracker.h"
#include "dll_util.h"

#define DEFAULT_DLLPATH "Q:\\system\\players\\mplayer\\codecs\\"
#define HIGH_WORD(a) ((WORD)(((DWORD)(a) >> 16) & MAXWORD))
#define LOW_WORD(a) ((WORD)(((DWORD)(a)) & MAXWORD))

//#define API_DEBUG

char* getpath(char *buf, const char *full)
{
  char* pos;
  if (pos = strrchr(full, '\\'))
  {
    strncpy(buf, full, pos - full + 1 );
    buf[pos - full + 1] = 0;
    return buf;
  }
  else 
  {
    buf[0] = 0;
    return buf;
  }
}

extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR lib_file, LPCSTR sourcedll)
{    
  char libname[MAX_PATH + 1] = {};
  char libpath[MAX_PATH + 1] = {};
  DllLoader* dll = NULL; 

  /* extract name */  
  char* p = strrchr(lib_file, '\\');
  if (p) 
    strcpy(libname, p+1);
  else 
    strcpy(libname, lib_file);  

  if( libname[0] == '\0' )
    return NULL;

  /* extract path */
  getpath(libpath, lib_file);
  
  CLog::Log(LOGDEBUG, "LoadLibraryA('%s')", libname);
  if (sourcedll)
  {
    /* also check for invalid paths wich begin with a \ */
    if( libpath[0] == '\0' || libpath[0] == '\\' )
    {
      /* use calling dll's path as base address for this call */
      getpath(libpath, sourcedll);

      /* mplayer has all it's dlls in a codecs subdirectory */
      if (strstr(sourcedll, "mplayer.dll"))
        strcat(libpath, "codecs\\");
    }
  }

  /* if we still don't have a path, use default path */
  if( libpath[0] == '\0' )
    strcpy(libpath, DEFAULT_DLLPATH);
  
  /* msdn docs state */
  /* "If no file name extension is specified in the lpFileName parameter, the default library extension .dll is appended.  */
  /* However, the file name string can include a trailing point character (.) to indicate that the module name has no extension." */
  if( strrchr(libname, '.') == NULL )
    strcat(libname, ".dll");
  else if( libname[strlen(libname)-1] == '.' )
    libname[strlen(libname)-1] = '\0';

  dll = g_dlls.LoadModule(libname, libpath);
    
  if (dll)
  {
    CLog::Log(LOGDEBUG, "LoadLibrary('%s') returning: 0x%x", libname, dll);
    return (HMODULE)dll->hModule;
  }

  CLog::Log(LOGERROR, "LoadLibrary('%s') failed", libname);
  return NULL;
}

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file)
{
  return dllLoadLibraryExtended(file, NULL);
}

#define DONT_RESOLVE_DLL_REFERENCES   0x00000001
#define LOAD_LIBRARY_AS_DATAFILE      0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH 0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL  0x00000010

extern "C" HMODULE __stdcall dllLoadLibraryExExtended(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags, LPCSTR sourcedll)
{
  char strFlags[512];
  strFlags[0] = '\0';

  if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) strcat(strFlags, "\n - DONT_RESOLVE_DLL_REFERENCES");
  if (dwFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL) strcat(strFlags, "\n - LOAD_IGNORE_CODE_AUTHZ_LEVEL");
  if (dwFlags & LOAD_LIBRARY_AS_DATAFILE) strcat(strFlags, "\n - LOAD_LIBRARY_AS_DATAFILE");
  if (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH) strcat(strFlags, "\n - LOAD_WITH_ALTERED_SEARCH_PATH");

  CLog::Log(LOGDEBUG, "LoadLibraryExA called with flags: %s", strFlags);
  
  return dllLoadLibraryExtended(lpLibFileName, sourcedll);
}

extern "C" HMODULE __stdcall dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  return dllLoadLibraryExExtended(lpLibFileName, hFile, dwFlags, NULL);
}

extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule)
{
  DllLoader* dllhandle = g_dlls.GetModule(hLibModule);
  
  // to make sure systems dlls are never deleted
  if (dllhandle->IsSystemDll()) return 1;
  
  CLog::Log(LOGDEBUG, "FreeLibrary(%s) -> 0x%x", dllhandle->GetName(), dllhandle);

  g_dlls.ReleaseModule(dllhandle);

  return 1;
}

extern "C" FARPROC __stdcall dllGetProcAddress(HMODULE hModule, LPCSTR function)
{
  unsigned loc = (unsigned)_ReturnAddress();
  
  void* address = NULL;
  DllLoader* dll = g_dlls.GetModule(hModule);

  if( !dll )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Invalid hModule specified");
    return NULL;
  }

  /* how can somebody get the stupid idea to create such a stupid function */
  /* where you never know if the given pointer is a pointer or a value */
  if( HIGH_WORD(function) == 0 && LOW_WORD(function) < 1000)
  {
    if( dll->ResolveExport(LOW_WORD(function), &address) )
    {
      CLog::Log(LOGDEBUG, __FUNCTION__"(0x%x(%s), %d) => 0x%x", hModule, dll->GetName(), LOW_WORD(function), address);
    }
    else if( dll->IsSystemDll() )
    {
      char ordinal[5];
      sprintf(ordinal, "%d", LOW_WORD(function));
      address = (void*)create_dummy_function(dll->GetName(), ordinal);

      /* add to tracklist if we are tracking this source dll */
      DllTrackInfo* track = tracker_get_dlltrackinfo(loc);
      if( track )
        tracker_dll_data_track(track->pDll, (unsigned long)address);

      CLog::Log(LOGDEBUG, __FUNCTION__" - created dummy function %s!%s", dll->GetName(), ordinal);
    }
    else
    {
      address = NULL;
      CLog::Log(LOGDEBUG, __FUNCTION__"(0x%x(%s), '%s') => 0x%x", hModule, dll->GetName(), function, address);
    }
  }
  else
  {
    if( dll->ResolveExport(function, &address) )
    {
      CLog::Log(LOGDEBUG, __FUNCTION__"(0x%x(%s), '%s') => 0x%x", hModule, dll->GetName(), function, address);
    }
    else
    {
      DllTrackInfo* track = tracker_get_dlltrackinfo(loc);
      /* some dll's require us to always return a function or it will fail, other's  */
      /* decide functionallity depending on if the functions exist and may fail      */
      if( dll->IsSystemDll() && track 
       && stricmp(track->pDll->GetName(), "CoreAVCDecoder.ax") == 0 )
      {
        address = (void*)create_dummy_function(dll->GetName(), function);
        tracker_dll_data_track(track->pDll, (unsigned long)address);
        CLog::Log(LOGDEBUG, __FUNCTION__" - created dummy function %s!%s", dll->GetName(), function);
      }
      else
      {      
        address = NULL;
        CLog::Log(LOGDEBUG, __FUNCTION__"(0x%x(%s), '%s') => 0x%x", hModule, dll->GetName(), function, address);
      }
    }
  }
  
  return (FARPROC)address;
}

extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName)
{
  /*
  If the file name extension is omitted, the default library extension .dll is appended.
  The file name string can include a trailing point character (.) to indicate that the module name has no extension.
  The string does not have to specify a path. When specifying a path, be sure to use backslashes (\), not forward slashes (/).
  The name is compared (case independently)
  If this parameter is NULL, GetModuleHandle returns a handle to the file used to create the calling process (.exe file).
  */

  if( lpModuleName == NULL ) 
    return NULL;

  char* strModuleName = new char[strlen(lpModuleName) + 5];
  strcpy(strModuleName, lpModuleName);

  if (strrchr(strModuleName, '.') == 0) strcat(strModuleName, ".dll");

  //CLog::Log(LOGDEBUG, "GetModuleHandleA(%s) .. looking up", lpModuleName);

  DllLoader *p = g_dlls.GetModule(strModuleName);
  delete []strModuleName;

  if (p)
  {
    //CLog::Log(LOGDEBUG, "GetModuleHandleA('%s') => 0x%x", lpModuleName, h);
    return (HMODULE)p->hModule;
  }
   
  CLog::Log(LOGDEBUG, "GetModuleHandleA('%s') failed", lpModuleName);
  return NULL;
}

extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
  if (NULL == hModule)
  {
    strncpy(lpFilename, "xbmc.xbe", nSize);
    CLog::Log(LOGDEBUG, "GetModuleFileNameA(0x%x, 0x%x, %d) => '%s'\n",
              hModule, lpFilename, nSize, lpFilename);
    return 8;
  }
  
  DllLoader* dll = g_dlls.GetModule(hModule);
  if( !dll )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Invalid hModule specified");
    return 0;
  }

  char* sName = dll->GetFileName();
  if (sName)
  {
    strncpy(lpFilename, sName, nSize);
    return strlen(lpFilename);
  }
  
  return 0;
}
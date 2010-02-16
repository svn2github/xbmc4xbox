#ifndef __XBMC_SCR_H__
#define __XBMC_SCR_H__

#include "xbmc_addon_dll.h"  
#include "xbmc_scr_types.h"

extern "C"
{

  // Functions that your visualisation must implement
  void Start();
  void Render();
  void Stop();
  void GetInfo(SCR_INFO* pInfo);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct ScreenSaver* pScr)
  {
    pScr->Start = Start;
    pScr->Render = Render;
    pScr->Stop = Stop;
    pScr->GetInfo = GetInfo;
  };
};

#endif

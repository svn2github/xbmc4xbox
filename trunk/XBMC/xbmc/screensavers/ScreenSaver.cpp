#include "../stdafx.h" 
// Screensaver.cpp: implementation of the CScreenSaver class.
//
//////////////////////////////////////////////////////////////////////

#include "../application.h"
#include "ScreenSaver.h" 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenSaver::CScreenSaver(struct ScreenSaver* pScr, DllLoader* pLoader, const CStdString& strScreenSaverName)
    : m_pLoader(pLoader)
    , m_pScr(pScr)
    , m_strScreenSaverName(strScreenSaverName)
{}

CScreenSaver::~CScreenSaver()
{
  g_application.m_CdgParser.FreeGraphics();
}

void CScreenSaver::Create()
{
  // pass it the screen width,height
  // and the name of the screensaver
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight = g_graphicsContext.GetHeight();

  char szTmp[129];
  sprintf(szTmp, "scr create:%ix%i %s\n", iWidth, iHeight, m_strScreenSaverName.c_str());
  OutputDebugString(szTmp);

  m_pScr->Create(g_graphicsContext.Get3DDevice(), iWidth, iHeight, m_strScreenSaverName.c_str());
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  m_pScr->Start();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  m_pScr->Render();
}

void CScreenSaver::Stop()
{
  // ask screensaver to cleanup
  m_pScr->Stop();
}


void CScreenSaver::GetInfo(SCR_INFO *info)
{
  // get info from screensaver
  m_pScr->GetInfo(info);
}

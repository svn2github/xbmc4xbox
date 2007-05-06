#pragma once

#include "GUIWindow.h"
#ifdef HAS_SCREENSAVER
#include "screensavers/ScreenSaver.h"
#endif

#include "Utils/CriticalSection.h"

#define SCREENSAVER_FADE   1
#define SCREENSAVER_BLACK  2
#define SCREENSAVER_XBS    3

class CGUIWindowScreensaver : public CGUIWindow
{
public:
  CGUIWindowScreensaver(void);
  virtual ~CGUIWindowScreensaver(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouse(float x, float y);
  virtual void Render();

private:
#ifdef HAS_SCREENSAVER
  CScreenSaver* m_pScreenSaver;
#endif
  bool m_bInitialized;
  CCriticalSection m_critSection;
};

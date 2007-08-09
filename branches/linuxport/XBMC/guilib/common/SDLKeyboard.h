#ifndef SDL_KEYBOARD_H
#define SDL_KEYBOARD_H

#include "../system.h"

#ifdef HAS_SDL

class CKeyboard
{
public:
  CKeyboard();

  void Initialize(HWND hwnd);
  void Reset();
  void Update(SDL_Event& event);
  bool GetShift() { return m_bShift;};
  bool GetCtrl() { return m_bCtrl;};
  bool GetAlt() { return m_bAlt;};
  char GetAscii() { return m_cAscii;};
  BYTE GetKey() { return m_VKey;};

private:
  bool m_bShift;
  bool m_bCtrl;
  bool m_bAlt;
  char m_cAscii;
  BYTE m_VKey;
#ifdef HAS_SDL_JOYSTICK
  SDL_Joystick* m_pJoy;
#endif
};

extern CKeyboard g_Keyboard;

#endif

#endif

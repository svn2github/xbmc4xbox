#ifndef SDL_JOYSTICK_H
#define SDL_JOYSTICK_H

#include "../system.h"

#ifdef HAS_SDL_JOYSTICK

#define MAX_AXES 64

#define JACTIVE_BUTTON 0x00000001
#define JACTIVE_AXIS   0x00000002
#define JACTIVE_NONE   0x00000000

// Class to manage all connected joysticks

class CJoystick
{
public:
  CJoystick();

  void Initialize(HWND hwnd);
  void Reset(bool axis=false);
  void CalibrateAxis(SDL_Joystick *joy);
  void ResetAxis(int axisId) { m_Amount[axisId] = 0; }
  void Update();
  void Update(SDL_Event& event);
  float GetAmount(int axis)
  {
    if (axis>=0 && axis<MAX_AXES) return (float)m_Amount[axis]/32768.0f; return 0.0f;
  }
  float GetAmount()
  {
    return (float)m_Amount[m_AxisId]/32768.0f;
  }
  bool GetButton (int& id, bool consider_repeat=true);
  bool GetAxis (int &id) { if (!IsAxisActive()) return false; id=m_AxisId; return true; }
  string GetJoystick() { return (m_JoyId>-1)?m_JoystickNames[m_JoyId]:""; }
  int GetAxisWithMaxAmount();
  void SetSafeRange(int val) { m_SafeRange=val; }

private:
  void SetAxisActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_AXIS):(m_ActiveFlags&(~JACTIVE_AXIS)); }
  void SetButtonActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_BUTTON):(m_ActiveFlags&(~JACTIVE_BUTTON)); }
  bool IsButtonActive() { return (bool)(m_ActiveFlags&JACTIVE_BUTTON); }
  bool IsAxisActive() { return (bool)(m_ActiveFlags&JACTIVE_AXIS); }

  int m_Amount[MAX_AXES];
  int m_DefaultAmount[MAX_AXES];
  int m_AxisId;
  int m_ButtonId;
  int m_JoyId;
  int m_NumAxes;
  int m_SafeRange;
  Uint32 m_pressTicks;
  WORD m_ActiveFlags;
  vector<SDL_Joystick*> m_Joysticks;
  vector<string> m_JoystickNames;
};

extern CJoystick g_Joystick;

#endif

#endif

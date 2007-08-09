#include "../include.h"
#include "../Key.h"
#include "SDLKeyboard.h"

#ifdef HAS_SDL

#define FRAME_DELAY 5

CKeyboard g_Keyboard; // global

CKeyboard::CKeyboard()
{
  Reset();
#ifdef HAS_SDL_JOYSTICK
  m_pJoy = NULL;
#endif
}

void CKeyboard::Initialize(HWND hWnd)
{
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#ifdef HAS_SDL_JOYSTICK
  if (SDL_NumJoysticks()>0)
  {
    SDL_JoystickEventState(SDL_ENABLE);
    m_pJoy = SDL_JoystickOpen(0);
    if (m_pJoy)
    {
      CLog::Log(LOGNOTICE, "Enabled Joystick: %s", SDL_JoystickName(0));
    }
  }
#endif
}

void CKeyboard::Reset()
{
  m_bShift = false;
  m_bCtrl = false;
  m_bAlt = false;
  m_cAscii = '\0';
  m_VKey = 0;
}

void CKeyboard::Update(SDL_Event& m_keyEvent)
{  
  if (m_keyEvent.type == SDL_KEYDOWN)
  {
    m_cAscii = 0;
    m_VKey = 0;

    m_bCtrl = (m_keyEvent.key.keysym.mod & KMOD_CTRL) != 0;
    m_bShift = (m_keyEvent.key.keysym.mod & KMOD_SHIFT) != 0;
    m_bAlt = (m_keyEvent.key.keysym.mod & KMOD_ALT) != 0;

    if ((m_keyEvent.key.keysym.unicode >= 'A' && m_keyEvent.key.keysym.unicode <= 'Z') ||
        (m_keyEvent.key.keysym.unicode >= 'a' && m_keyEvent.key.keysym.unicode <= 'z'))
    {
      m_cAscii = (char)m_keyEvent.key.keysym.unicode;
      m_VKey = toupper(m_cAscii);
    }
    else if (m_keyEvent.key.keysym.unicode >= '0' && m_keyEvent.key.keysym.unicode <= '9')
    {
      m_cAscii = (char)m_keyEvent.key.keysym.unicode;
      m_VKey = 0x60 + m_cAscii - '0'; // xbox keyboard routine appears to return 0x60->69 (unverified). Ideally this "fixing"
                                      // should be done in xbox routine, not in the sdl/directinput routines.
                                      // we should just be using the unicode/ascii value in all routines (perhaps with some
                                      // headroom for modifier keys?)
    }
    else
    {
      // see comment above about the weird use of m_VKey here...
      if (m_keyEvent.key.keysym.unicode == ')') { m_VKey = 0x60; m_cAscii = ')'; }
      else if (m_keyEvent.key.keysym.unicode == '!') { m_VKey = 0x61; m_cAscii = '!'; }
      else if (m_keyEvent.key.keysym.unicode == '@') { m_VKey = 0x62; m_cAscii = '@'; }
      else if (m_keyEvent.key.keysym.unicode == '#') { m_VKey = 0x63; m_cAscii = '#'; }
      else if (m_keyEvent.key.keysym.unicode == '$') { m_VKey = 0x64; m_cAscii = '$'; }
      else if (m_keyEvent.key.keysym.unicode == '%') { m_VKey = 0x65; m_cAscii = '%'; }
      else if (m_keyEvent.key.keysym.unicode == '^') { m_VKey = 0x66; m_cAscii = '^'; }
      else if (m_keyEvent.key.keysym.unicode == '&') { m_VKey = 0x67; m_cAscii = '&'; }
      else if (m_keyEvent.key.keysym.unicode == '*') { m_VKey = 0x68; m_cAscii = '*'; }
      else if (m_keyEvent.key.keysym.unicode == '(') { m_VKey = 0x69; m_cAscii = '('; }
      else if (m_keyEvent.key.keysym.unicode == ':') { m_VKey = 0xba; m_cAscii = ':'; }
      else if (m_keyEvent.key.keysym.unicode == ';') { m_VKey = 0xba; m_cAscii = ';'; }
      else if (m_keyEvent.key.keysym.unicode == '=') { m_VKey = 0xbb; m_cAscii = '='; }
      else if (m_keyEvent.key.keysym.unicode == '+') { m_VKey = 0xbb; m_cAscii = '+'; }
      else if (m_keyEvent.key.keysym.unicode == '<') { m_VKey = 0xbc; m_cAscii = '<'; }
      else if (m_keyEvent.key.keysym.unicode == ',') { m_VKey = 0xbc; m_cAscii = ','; }
      else if (m_keyEvent.key.keysym.unicode == '-') { m_VKey = 0xbd; m_cAscii = '-'; }
      else if (m_keyEvent.key.keysym.unicode == '_') { m_VKey = 0xbd; m_cAscii = '_'; }
      else if (m_keyEvent.key.keysym.unicode == '>') { m_VKey = 0xbe; m_cAscii = '>'; }
      else if (m_keyEvent.key.keysym.unicode == '.') { m_VKey = 0xbe; m_cAscii = '.'; }
      else if (m_keyEvent.key.keysym.unicode == '?') { m_VKey = 0xbf; m_cAscii = '?'; }
      else if (m_keyEvent.key.keysym.unicode == '/') { m_VKey = 0xbf; m_cAscii = '/'; }
      else if (m_keyEvent.key.keysym.unicode == '~') { m_VKey = 0xc0; m_cAscii = '~'; }
      else if (m_keyEvent.key.keysym.unicode == '`') { m_VKey = 0xc0; m_cAscii = '`'; }
      else if (m_keyEvent.key.keysym.unicode == '{') { m_VKey = 0xeb; m_cAscii = '{'; }
      else if (m_keyEvent.key.keysym.unicode == '[') { m_VKey = 0xeb; m_cAscii = '['; }
      else if (m_keyEvent.key.keysym.unicode == '|') { m_VKey = 0xec; m_cAscii = '|'; }
      else if (m_keyEvent.key.keysym.unicode == '\\') { m_VKey = 0xec; m_cAscii = '\\'; }
      else if (m_keyEvent.key.keysym.unicode == '}') { m_VKey = 0xed; m_cAscii = '}'; }
      else if (m_keyEvent.key.keysym.unicode == ']') { m_VKey = 0xed; m_cAscii = ']'; }
      else if (m_keyEvent.key.keysym.unicode == '"') { m_VKey = 0xee; m_cAscii = '"'; }
      else if (m_keyEvent.key.keysym.unicode == '\'') { m_VKey = 0xee; m_cAscii = '\''; }
      else if (m_keyEvent.key.keysym.sym == SDLK_BACKSPACE) m_VKey = 0x08;
      else if (m_keyEvent.key.keysym.sym == SDLK_TAB) m_VKey = 0x09;
      else if (m_keyEvent.key.keysym.sym == SDLK_RETURN) m_VKey = 0x0d;
      else if (m_keyEvent.key.keysym.sym == SDLK_ESCAPE) m_VKey = 0x1b;
      else if (m_keyEvent.key.keysym.sym == SDLK_SPACE) m_VKey = 0x20;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP0) m_VKey = 0x60;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP1) m_VKey = 0x61;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP2) m_VKey = 0x62;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP3) m_VKey = 0x63;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP4) m_VKey = 0x64;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP5) m_VKey = 0x65;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP6) m_VKey = 0x66;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP7) m_VKey = 0x67;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP8) m_VKey = 0x68;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP9) m_VKey = 0x69;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_ENTER) m_VKey = 0x6C;
      else if (m_keyEvent.key.keysym.sym == SDLK_UP) m_VKey = 0x26;
      else if (m_keyEvent.key.keysym.sym == SDLK_DOWN) m_VKey = 0x28;
      else if (m_keyEvent.key.keysym.sym == SDLK_LEFT) m_VKey = 0x25;
      else if (m_keyEvent.key.keysym.sym == SDLK_RIGHT) m_VKey = 0x27;
      else if (m_keyEvent.key.keysym.sym == SDLK_INSERT) m_VKey = 0x2D;
      else if (m_keyEvent.key.keysym.sym == SDLK_DELETE) m_VKey = 0x2E;
      else if (m_keyEvent.key.keysym.sym == SDLK_HOME) m_VKey = 0x24;
      else if (m_keyEvent.key.keysym.sym == SDLK_END) m_VKey = 0x23;
      else if (m_keyEvent.key.keysym.sym == SDLK_F1) m_VKey = 0x70;
      else if (m_keyEvent.key.keysym.sym == SDLK_F2) m_VKey = 0x71;
      else if (m_keyEvent.key.keysym.sym == SDLK_F3) m_VKey = 0x72;
      else if (m_keyEvent.key.keysym.sym == SDLK_F4) m_VKey = 0x73;
      else if (m_keyEvent.key.keysym.sym == SDLK_F5) m_VKey = 0x74;
      else if (m_keyEvent.key.keysym.sym == SDLK_F6) m_VKey = 0x75;
      else if (m_keyEvent.key.keysym.sym == SDLK_F7) m_VKey = 0x76;
      else if (m_keyEvent.key.keysym.sym == SDLK_F8) m_VKey = 0x77;
      else if (m_keyEvent.key.keysym.sym == SDLK_F9) m_VKey = 0x78;
      else if (m_keyEvent.key.keysym.sym == SDLK_F10) m_VKey = 0x79;
      else if (m_keyEvent.key.keysym.sym == SDLK_F11) m_VKey = 0x7a;
      else if (m_keyEvent.key.keysym.sym == SDLK_F12) m_VKey = 0x7b;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_PERIOD) m_VKey = 0x6e;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_MULTIPLY) m_VKey = 0x6a;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_MINUS) m_VKey = 0x6d;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_PLUS) m_VKey = 0x6b;
      else if (m_keyEvent.key.keysym.sym == SDLK_KP_DIVIDE) m_VKey = 0x6f;
      else if (m_keyEvent.key.keysym.sym == SDLK_PAGEUP) m_VKey = 0x21;
      else if (m_keyEvent.key.keysym.sym == SDLK_PAGEDOWN) m_VKey = 0x22;
      else if ((m_keyEvent.key.keysym.sym == SDLK_PRINT) || (m_keyEvent.key.keysym.scancode==111) ) m_VKey = 0x2a;
      else if (m_keyEvent.key.keysym.sym == SDLK_LSUPER) m_VKey = 0x5b;
      else if (m_keyEvent.key.keysym.sym == SDLK_RSUPER) m_VKey = 0x5c;
      else if (m_keyEvent.key.keysym.scancode==117) m_VKey = 0x5d; // right click
      else if (m_keyEvent.key.keysym.mod & KMOD_LSHIFT) m_VKey = 0xa0;
      else if (m_keyEvent.key.keysym.mod & KMOD_RSHIFT) m_VKey = 0xa1;
      else if (m_keyEvent.key.keysym.mod & KMOD_LALT) m_VKey = 0xa4;
      else if (m_keyEvent.key.keysym.mod & KMOD_RALT) m_VKey = 0xa5;
      else if (m_keyEvent.key.keysym.mod & KMOD_LCTRL) m_VKey = 0xa2;
      else if (m_keyEvent.key.keysym.mod & KMOD_RCTRL) m_VKey = 0xa3;		
      else if (m_keyEvent.key.keysym.unicode > 32 && m_keyEvent.key.keysym.unicode < 128)
        m_cAscii = (char)(m_keyEvent.key.keysym.unicode & 0xff);      // try and catch everything
    }
  }
#ifdef HAS_SDL_JOYSTICK
/*
  Initial support for the
  PS3 Controller using the SDL Joystick API

  /\ - 12
  O  - 13
  X  - 14
  [] - 15
  
  top - 4
  right - 5
  bottom - 6
  left - 7
  
  select - 0
  start - 3

  left analog click - 1
  right analog click - 2

  left shoulder - 10
  right shoulder - 11

  left analog shoulder - 8
  right analog shoulder - 9

  Controller reports 28 axes, one for each pressure sensitive button with values ranging 
  from [-32k, 32k].

--

  TODO: 1. Move this code to a Gamepad class so that custom gamepads can be configured using Keymap.xml
        2. Map each button and axis of the joystick to the Xbox gamepad instead of vkeys.
	3. Analog controls.
*/

  else if (m_keyEvent.type == SDL_JOYBUTTONDOWN)
  {
    CLog::Log(LOGNOTICE, "Joystick button pressed: %d\n", m_keyEvent.jbutton.button);
    if (m_keyEvent.jbutton.button==0) ;
    else if (m_keyEvent.jbutton.button==1) m_VKey = 0x2a;
    else if (m_keyEvent.jbutton.button==2) { m_cAscii = (char)'s'; m_VKey = toupper(m_cAscii); }
    else if (m_keyEvent.jbutton.button==3) { m_cAscii = (char)'m'; m_VKey = toupper(m_cAscii); }
    else if (m_keyEvent.jbutton.button==4) m_VKey = 0x26;
    else if (m_keyEvent.jbutton.button==5) m_VKey = 0x27;
    else if (m_keyEvent.jbutton.button==6) m_VKey = 0x28;
    else if (m_keyEvent.jbutton.button==7) m_VKey = 0x25;
    else if (m_keyEvent.jbutton.button==8) ;
    else if (m_keyEvent.jbutton.button==9) ;
    else if (m_keyEvent.jbutton.button==10) m_VKey = 0x1b;
    else if (m_keyEvent.jbutton.button==11) ;
    else if (m_keyEvent.jbutton.button==12) { m_cAscii = (char)'q'; m_VKey = toupper(m_cAscii); }
    else if (m_keyEvent.jbutton.button==13) m_VKey = 0x08;
    else if (m_keyEvent.jbutton.button==14) m_VKey = 0x0d;
    else if (m_keyEvent.jbutton.button==15) m_VKey = 0x09;
  }
#endif
  else
  {
    Reset();
  }
}

#endif

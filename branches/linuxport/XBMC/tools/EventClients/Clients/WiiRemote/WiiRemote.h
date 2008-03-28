/***************************************************************************
 *   Copyright (C) 2007 by Tobias Arrskog,,,   *
 *   topfs@tobias   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef WII_REMOTE_H
#define WII_REMOTE_H

/* Toggle one bit */
#define ToggleBit(bf,b) (bf) = ((bf) & b) ? ((bf) & ~(b)) : ((bf) | (b))

//Settings
#define WIIREMOTE_IR_DEADZONE 0.2f                    // Deadzone around the edges of the IRsource range
#define WIIREMOTE_BUTTON_REPEAT_TIME 30               // How long between buttonpresses in repeat mode
#define WIIREMOTE_BUTTON_DELAY_TIME 500
#define CWIID_OLD                                     // Uncomment if the system is running cwiid that is older than 6.0 (The one from ubuntu gutsy repository is < 6.0)

//CWIID
#include <cwiid.h>
//Bluetooth specific
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
// UDP Client
#include "../../lib/c++/xbmcclient.h"

/*#include <stdio.h>*/
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

class CWiiRemote
{
public:
  CWiiRemote(char *btaddr = NULL);
  ~CWiiRemote();
	
  void Initialize(CAddress Addr, int Socket);
  void Disconnect();	
  bool GetConnected();

  bool EnableWiiRemote();
  bool DisableWiiRemote();	

  void Update();

  // Mouse functions
  bool HaveIRSources();
  bool isActive();
  void EnableMouseEmulation();
  void DisableMouseEmulation();

  bool Connect();  

  void SetBluetoothAddress(const char * btaddr);
private:
#ifdef CWIID_OLD
  bool CheckConnection();
  int  m_LastMsgTime;
#endif  
	
  int  m_lastKeyPressed;
  int  m_LastKey;
  bool m_buttonRepeat;

  int  m_lastKeyPressedNunchuck;
  int  m_LastKeyNunchuck;
  bool m_buttonRepeatNunchuck;

  void SetRptMode();
  void SetLedState();

  void SetupWiiRemote();
		
  bool m_connected;

  bool m_DisconnectWhenPossible;
  bool m_connectThreadRunning;
			
  //CWIID Specific
  cwiid_wiimote_t *m_wiiremoteHandle;
  unsigned char    m_ledState;
  unsigned char    m_rptMode;
  bdaddr_t         m_btaddr;

  static void MessageCallback(cwiid_wiimote_t *wiiremote, int mesgCount, union cwiid_mesg mesg[], struct timespec *timestamp);
#ifdef CWIID_OLD
  static void MessageCallback(cwiid_wiimote_t *wiiremote, int mesgCount, union cwiid_mesg mesg[]);
#endif

#ifndef _DEBUG
/* This takes the errors generated at pre-connect and silence them as they are mostly not needed */
  static void ErrorCallback(struct wiimote *wiiremote, const char *str, va_list ap);
#endif

	int   m_Socket;
  CAddress m_MyAddr;
	
  // Mouse	
  bool	m_haveIRSources;
  bool  m_isActive;
  bool  m_useIRMouse;
  int   m_lastActiveTime;
	
/* The protected functions is for the static callbacks */	
  protected:
  //Connection
  void DisconnectNow(bool startConnectThread);
	
  //Mouse
  void CalculateMousePointer(int x1, int y1, int x2, int y2);
//  void SetIR(bool set);
 	
  //Button
  void ProcessKey(int Key);
 	
  //Nunchuck
  void ProcessNunchuck(struct cwiid_nunchuk_mesg &Nunchuck);
#ifdef CWIID_OLD
  //Disconnect check
  void CheckIn();
#endif
};

#endif // WII_REMOTE_H

extern CWiiRemote g_WiiRemote;



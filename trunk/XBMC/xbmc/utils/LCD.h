#pragma once
#include "stdstring.h"
#include "thread.h"
#include "../utils/event.h"
using namespace std;
#define MAX_ROWS 20

class CLCD : public CThread
{
public:
  CLCD();
  virtual ~CLCD(void);
  static void Initialize();
  static void Stop();
  static void SetLine(int iLine, const CStdString& strLine);
  static void SetBackLight(int iLight);
protected:
	virtual void		Process();
  void    DisplayInit();
  void    DisplaySetBacklight(unsigned char level) ;
  void    DisplayProgressBar(unsigned char percent, unsigned char charcnt);
  void    DisplayClearChars(unsigned char startpos , unsigned char line, unsigned char lenght) ;
  void    DisplayWriteString(char *pointer) ;
  void    DisplayWriteFixtext(const char *textstring);
  void    DisplaySetPos(unsigned char pos, unsigned char line) ;
  void    DisplayBuildCustomChars() ;
  void    DisplayOut(unsigned char data, unsigned char command) ;
  void    wait_us(unsigned int value) ;
  unsigned int m_iColumns;				// display columns for each line
  unsigned int m_iRows;				// total number of rows
  unsigned int m_iRow1adr ;
  unsigned int m_iRow2adr ;
  unsigned int m_iRow3adr ;
  unsigned int m_iRow4adr ;
  unsigned int m_iActualpos;				// actual cursor possition
  int          m_iBackLight;
  bool         m_bUpdate[MAX_ROWS];
  CStdString   m_strLine[MAX_ROWS];
  int          m_iPos[MAX_ROWS];
  DWORD        m_dwSleep[MAX_ROWS];
  CEvent       m_event;

};
extern CLCD g_lcd;
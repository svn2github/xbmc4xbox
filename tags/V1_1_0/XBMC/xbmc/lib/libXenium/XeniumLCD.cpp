
#include "../stdafx.h"
#include "xeniumlcd.h"
#include "../../settings.h"
#include "../../utils/log.h"
#include <xtl.h>
#include "conio.h"

#define SCROLL_SPEED_IN_MSEC 250


//*************************************************************************************************************
CXeniumLCD::CXeniumLCD()
{
  m_iActualpos=0;
  m_iRows    = 4;
  m_iColumns = 20;				// display rows each line
  m_iRow1adr = 0x00;
  m_iRow2adr = 0x20;
  m_iRow3adr = 0x40;
  m_iRow4adr = 0x60;

  m_iRow1adr = 0x00;
  m_iRow2adr = 0x40;
  m_iRow3adr = 0x14;
  m_iRow4adr = 0x54;
  m_iBackLight=32;
}

//*************************************************************************************************************
CXeniumLCD::~CXeniumLCD()
{
}

//*************************************************************************************************************
void CXeniumLCD::Initialize()
{
  StopThread();
  if (!g_stSettings.m_bLCDUsed) 
  {
    CLog::Log(LOGINFO, "lcd not used");
    return;
  }
  Create();
  
}
void CXeniumLCD::SetBackLight(int iLight)
{
  m_iBackLight=iLight;
}
void CXeniumLCD::SetContrast(int iContrast)
{
}

//*************************************************************************************************************
void CXeniumLCD::Stop()
{
  if (!g_stSettings.m_bLCDUsed) return;
  StopThread();
}

//*************************************************************************************************************
void CXeniumLCD::SetLine(int iLine, const CStdString& strLine)
{
  if (!g_stSettings.m_bLCDUsed) return;
  if (iLine < 0 || iLine >= (int)m_iRows) return;
  
  CStdString strLineLong=strLine;
  strLineLong.Trim();
	StringToLCDCharSet(strLineLong);

  while (strLineLong.size() < m_iColumns) strLineLong+=" ";
  if (strLineLong != m_strLine[iLine])
  {
//    CLog::Log(LOGINFO, "set line:%i [%s]", iLine,strLineLong.c_str());
    m_bUpdate[iLine]=true;
    m_strLine[iLine]=strLineLong;
    m_event.Set();
  }
}


//************************************************************************************************************************
// wait_us: delay routine
// Input: (wait in ~us)
//************************************************************************************************************************
void CXeniumLCD::wait_us(unsigned int value) 
{
}


//************************************************************************************************************************
// DisplayOut: writes command or datas to display
// Input: (Value to write, token as CMD = Command / DAT = DATAs / INI for switching to 4 bit mode)
//************************************************************************************************************************
void CXeniumLCD::DisplayOut(unsigned char data, unsigned char command) 
{

 }

//************************************************************************************************************************
//DisplayBuildCustomChars: load customized characters to character ram of display, resets cursor to pos 0
//************************************************************************************************************************
void CXeniumLCD::DisplayBuildCustomChars() 
{

}


//************************************************************************************************************************
// DisplaySetPos: sets cursor position
// Input: (row position, line number from 0 to 3)
//************************************************************************************************************************
void CXeniumLCD::DisplaySetPos(unsigned char pos, unsigned char line) 
{
  m_xenium.SetCursorPosition(  pos, line);
}


//************************************************************************************************************************
// DisplayWriteFixText: write a fixed text to actual cursor position
// Input: ("fixed text like")
//************************************************************************************************************************
void CXeniumLCD::DisplayWriteFixtext(const char *textstring)
{ 
  m_xenium.OutputString(textstring,m_iColumns);
} 


//************************************************************************************************************************
// DisplayWriteString: write a string to acutal cursor position 
// Input: (pointer to a 0x00 terminated string)
//************************************************************************************************************************

void CXeniumLCD::DisplayWriteString(char *pointer) 
{
  m_xenium.OutputString(pointer,m_iColumns);
}		


//************************************************************************************************************************
// DisplayClearChars:  clears a number of chars in a line and resets cursor position to it's startposition
// Input: (Startposition of clear in row, row number, number of chars to clear)
//************************************************************************************************************************
void CXeniumLCD::DisplayClearChars(unsigned char startpos , unsigned char line, unsigned char lenght) 
{
}


//************************************************************************************************************************
// DisplayProgressBar: shows a grafic bar staring at actual cursor position
// Input: (percent of bar to display, lenght of whole bar in chars when 100 %)
//************************************************************************************************************************
void CXeniumLCD::DisplayProgressBar(unsigned char percent, unsigned char charcnt) 
{

}
//************************************************************************************************************************
//Set brightness level 
//************************************************************************************************************************
void CXeniumLCD::DisplaySetBacklight(unsigned char level) 
{
  if (level<0) level=0;
  if (level>100) level=100;
  m_xenium.SetBacklight(level/4);
}
//************************************************************************************************************************
void CXeniumLCD::DisplayInit()
{
  m_xenium.ShowDisplay();
  m_xenium.HideCursor();
  m_xenium.ScrollOff();
  m_xenium.WrapOff();
  m_xenium.SetContrast(12);
}

//************************************************************************************************************************
void CXeniumLCD::Process()
{
  int iOldLight=-1;  

  m_iColumns = g_stSettings.m_iLCDColumns;
  m_iRows    = g_stSettings.m_iLCDRows;
  m_iRow1adr = g_stSettings.m_iLCDAdress[0];
  m_iRow2adr = g_stSettings.m_iLCDAdress[1];
  m_iRow3adr = g_stSettings.m_iLCDAdress[2];
  m_iRow4adr = g_stSettings.m_iLCDAdress[3];
  m_iBackLight= g_stSettings.m_iLCDBackLight;
  if (m_iRows >= MAX_ROWS) m_iRows=MAX_ROWS-1;

  DisplayInit();
  while (!m_bStop)
  {
    Sleep(SCROLL_SPEED_IN_MSEC);  
    if (m_iBackLight != iOldLight)
    {
      // backlight setting changed
      iOldLight=m_iBackLight;
      DisplaySetBacklight(m_iBackLight);
    }
	  DisplayBuildCustomChars();
	  for (int iLine=0; iLine < (int)m_iRows; ++iLine)
    {
	    if (m_bUpdate[iLine])
      {
        CStdString strTmp=m_strLine[iLine];
        if (strTmp.size() > m_iColumns)
        {
          strTmp=m_strLine[iLine].Left(m_iColumns);
        }
        m_iPos[iLine]=0;
        DisplaySetPos(0,iLine);
        DisplayWriteFixtext(strTmp.c_str());
        m_bUpdate[iLine]=false;
        m_dwSleep[iLine]=GetTickCount();
      }
      else if ( (GetTickCount()-m_dwSleep[iLine]) > 1000)
      {
        int iSize=m_strLine[iLine].size();
        if (iSize > (int)m_iColumns)
        {
          //scroll line
          CStdString strRow=m_strLine[iLine]+"   -   ";
          int iSize=strRow.size();
          m_iPos[iLine]++;
          if (m_iPos[iLine]>=iSize) m_iPos[iLine]=0;
          int iPos=m_iPos[iLine];
          CStdString strLine="";
          for (int iCol=0; iCol < (int)m_iColumns;++iCol)
          {
            strLine +=strRow.GetAt(iPos);
            iPos++;
            if (iPos >= iSize) iPos=0;
          }
          DisplaySetPos(0,iLine);
          DisplayWriteFixtext(strLine.c_str());
        }
      }
    }
  }
  for (int i=0; i < (int)m_iRows; i++)
  {
	  DisplayClearChars(0,i,m_iColumns);
  } 
  m_xenium.HideDisplay();
}
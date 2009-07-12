#ifndef CEDL_H
#define CEDL_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdString.h"
#include <vector>
#include "linux/PlatformDefs.h"

class CEdl
{
public:
  CEdl();
  virtual ~CEdl(void);

  typedef enum
  {
    CUT = 0,
    MUTE = 1,
    SCENE = 2
  } Action;

  struct Cut
  {
    __int64 start;
    __int64 end;
    Action action;
  };

  bool ReadFiles(const CStdString& strMovie);
  void Clear();

  bool HasCut();
  bool HasSceneMarker();
  char GetEdlStatus();
  __int64 GetTotalCutTime();
  __int64 RemoveCutTime(__int64 iTime);
  __int64 RestoreCutTime(__int64 iTime);

  bool InCut(__int64 iAbsSeek, Cut *pCurCut = NULL);

  bool GetNextSceneMarker(bool bPlus, const __int64 clock, __int64 *iScenemarker);

protected:
private:
  __int64 m_iTotalCutTime; // msec
  std::vector<Cut> m_vecCuts;
  std::vector<__int64> m_vecSceneMarkers;

  bool ReadEdl(const CStdString& strMovie);
  bool ReadComskip(const CStdString& strMovie);
  bool ReadVideoReDo(const CStdString& strMovie);
  bool ReadBeyondTV(const CStdString& strMovie);

  bool AddCut(const Cut& NewCut);
  bool AddSceneMarker(const __int64 sceneMarker);

  bool WriteMPlayerEdl();

  static CStdString MillisecondsToTimeString(const int iMilliseconds);
};

#endif // CEDL_H

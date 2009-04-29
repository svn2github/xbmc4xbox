#pragma once
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
#include "Database.h"
#include "ProgramInfoTag.h"
#include <set>

typedef std::vector<CStdString> VECPROGRAMPATHS;
struct SScraperInfo;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

#define PROGRAM_DATABASE_NAME "MyPrograms6.db"

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);
  bool AddTrainer(int iTitleId, const CStdString& strText);
  bool RemoveTrainer(const CStdString& strText);
  bool GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers);
  bool GetAllTrainers(std::vector<CStdString>& vecTrainers);
  bool SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  bool GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  void SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive);
  CStdString GetActiveTrainer(unsigned int iTitleId);
  bool HasTrainer(const CStdString& strTrainerPath);
  bool ItemHasTrainer(unsigned int iTitleId);

  int GetRegion(const CStdString& strFilenameAndPath);
  bool SetRegion(const CStdString& strFilenameAndPath, int iRegion=-1);

  DWORD GetTitleId(const CStdString& strFilenameAndPath);
  bool SetTitleId(const CStdString& strFilenameAndPath, DWORD dwTitleId);
  bool IncTimesPlayed(const CStdString& strFileName1);
  bool SetDescription(const CStdString& strFileName1, const CStdString& strDescription);
  bool GetXBEPathByTitleId(const DWORD titleId, CStdString& strPathAndFilename);

  DWORD GetProgramInfo(CFileItem *item);
  bool AddProgramInfo(CFileItem *item, unsigned int titleID);

  long AddPath(const CStdString& strPath);
  long GetPathId(const CStdString& strPath);
  void SetScraperForPath(const CStdString& filePath, const SScraperInfo& info);
  bool GetScraperForPath(const CStdString& strPath, SScraperInfo& info);

  DWORD AddTitle(CProgramInfoTag program);
  CProgramInfoTag GetTitle(DWORD titleId);
  void RemoveTitle(DWORD titleId);

protected:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

  FILETIME TimeStampToLocalTime( unsigned __int64 timeStamp );
};

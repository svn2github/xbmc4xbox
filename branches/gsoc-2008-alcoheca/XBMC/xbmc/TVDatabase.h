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
#include "DateTime.h"
#include "settings/VideoSettings.h"
#include "utils/EPGInfoTag.h"
#include "FileItem.h"

class CGUIDialogProgress;

typedef std::vector<VECFILEITEMS> EPGGrid;
typedef std::vector<VECFILEITEMS>::const_iterator iEPGRow;

typedef VECFILEITEMS::const_iterator iEPGItem;

class CTVDatabase : public CDatabase
{
public:
  CTVDatabase(void);
  virtual ~CTVDatabase(void);

  virtual bool CommitTransaction();

  // epg
  bool FillEPG(const CStdString &source, const CStdString &bouquet, const CStdString &channame, const CStdString &callsign, const int &channum, const CStdString &progTitle, 
               const CStdString &progSubtitle, const CStdString &progDescription, const CStdString &episode, const CStdString &series, 
               const CDateTime &progStartTime, const CDateTime &progEndTime, const CStdString &category);

  void GetChannels(bool freeToAirOnly, VECFILEITEMS* channels);

  bool GetProgrammesByChannelName(const CStdString &channel, VECFILEITEMS &shows, const CDateTime &start, const CDateTime &end);
  bool GetProgrammesByEpisodeID(const CStdString& episodeID, CFileItemList* items, bool noHistory /* == true */);
  void GetProgrammesByName(const CStdString& progName, CFileItemList& items, bool noHistory /* == true */);
  bool GetProgrammesBySubtitle(const CStdString& subtitle, CFileItemList* items, bool noHistory /* == true */);

  // per-channel video settings
  bool GetChannelSettings(const CStdString &channel, CVideoSettings &settings);
  bool SetChannelSettings(const CStdString &channel, const CVideoSettings &settings);
  // per-programme video settings
  bool GetProgrammeSettings(const CStdString &programme, CVideoSettings &settings);
  void SetProgrammeSettings(const CStdString &programme, const CVideoSettings &settings);
  
  CDateTime GetDataEnd();

  void EraseChannelSettings();

protected:
  CEPGInfoTag GetUniqueBroadcast(std::auto_ptr<dbiplus::Dataset> &pDS);
  void FillProperties(CFileItem* programme);

  long AddSource(const CStdString &source);
  long AddBouquet(const long &sourceId, const CStdString &bouquet);
  long AddChannel(const long &idSource, const long &idBouquet, const CStdString &Callsign, const CStdString &Name, const int &Number);
  long AddProgramme(const CStdString &Title, const long &categoryId);
  long AddCategory(const CStdString &category);

  long GetSourceId(const CStdString &source);
  long GetBouquetId(const CStdString &bouquet);
  long GetChannelId(const CStdString &channel);
  long GetCategoryId(const CStdString &category);
  long GetProgrammeId(const CStdString &programme);

  CStdString m_progInfoSelectStatement;

private:
  virtual bool CreateTables();
};

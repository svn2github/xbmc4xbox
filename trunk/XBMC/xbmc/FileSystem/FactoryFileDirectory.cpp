
#include "../stdafx.h"
#include "../util.h"
#include "factoryfiledirectory.h"
#include "oggfiledirectory.h"
#include "rardirectory.h"
#include "zipdirectory.h"
#include "nsffiledirectory.h"
#include "sidfiledirectory.h"
#include "SmartPlaylistDirectory.h"
#include "PlaylistDirectory.h"
#include "../SmartPlaylist.h"

CFactoryFileDirectory::CFactoryFileDirectory(void)
{}

CFactoryFileDirectory::~CFactoryFileDirectory(void)
{}

// return NULL + set pItem->m_bIsFolder to remove it completely from list.
IFileDirectory* CFactoryFileDirectory::Create(const CStdString& strPath, CFileItem* pItem, const CStdString& strMask)
{
  CStdString strExtension=CUtil::GetExtension(strPath);
  if (strExtension.size() == 0) return NULL;
  strExtension.MakeLower();

  if (strExtension.Equals(".ogg") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new COGGFileDirectory;
    //  Has the ogg file more then one bitstream?
    if (pDir->ContainsFiles(strPath))
    {
      return pDir; // treat as directory
    }

    delete pDir;
    return NULL;
  }
  if (strExtension.Equals(".nsf") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new CNSFFileDirectory;
    //  Has the nsf file more then one track?
    if (pDir->ContainsFiles(strPath))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
  if (strExtension.Equals(".sid") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new CSIDFileDirectory;
    //  Has the nsf file more then one track?
    if (pDir->ContainsFiles(strPath))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
  if (strExtension.Equals(".zip"))
  {
    CStdString strUrl; strUrl.Format("zip://Z:\\,2,,%s,\\",strPath.c_str());
    CFileItemList item;
    CGUIViewState* guiState = CGUIViewState::GetViewState(0,item);
    if (guiState)
    {
      bool bState = guiState->UnrollArchives();
      delete guiState;
      guiState = NULL;
      if (!bState)
      {
        pItem->m_strPath = strUrl;
        return new CZipDirectory;
      }
    }

    if (g_ZipManager.HasMultipleEntries(strPath))
    {
      pItem->m_strPath = strUrl;
      return new CZipDirectory;
    }

    item.Clear();
    CDirectory dir; dir.GetDirectory(strUrl,item,strMask);
    if (item.Size())
      *pItem = *item[0];
    else
      pItem->m_bIsFolder = true;

    return NULL;
  }
  if (strExtension.Equals(".rar") || strExtension.Equals(".001"))
  {
    CStdString strUrl; strUrl.Format("rar://Z:\\,2,,%s,\\",strPath.c_str());
    std::vector<CStdString> tokens;
    CUtil::Tokenize(strPath,tokens,".");
    if (tokens.size() > 2)
    {
      CStdString token = tokens[tokens.size()-2];
      if (token.Left(4).CompareNoCase("part") == 0) // only list '.part01.rar'
        if (atoi(token.substr(4).c_str()) > 1)
        {
          pItem->m_bIsFolder = true;
          return NULL;
        }
    }

    CFileItemList item;
    CGUIViewState* guiState = CGUIViewState::GetViewState(0,item);
    if (guiState)
    {
      bool bState = guiState->UnrollArchives();
      delete guiState;
      guiState = NULL;
      if (!bState)
      {
        pItem->m_strPath = strUrl;
        return new CRarDirectory;
      }
    }

    if (g_RarManager.HasMultipleEntries(strPath))
    {
      pItem->m_strPath = strUrl;
      return new CRarDirectory;
    }

    CDirectory dir; dir.GetDirectory(strUrl,item,strMask);
    if (item.Size())
      *pItem = *item[0];
    else
      pItem->m_bIsFolder = true;

    return NULL;
  }
  if (strExtension.Equals(".xsp"))
  { // XBMC Smart playlist - just XML renamed to XSP
    // read the name of the playlist in
    CSmartPlaylist playlist;
    if (playlist.OpenAndReadName(strPath))
    {
      pItem->SetLabel(playlist.GetName());
      pItem->SetLabelPreformated(true);
    }
    IFileDirectory* pDir=new CSmartPlaylistDirectory;
    return pDir; // treat as directory
  }
  if (g_advancedSettings.m_playlistAsFolders)
  if (strExtension == ".m3u" || strExtension == ".b4s" || strExtension == ".pls" ||
      strExtension == ".wpl")
  { // Playlist file
    // currently we only return the directory if it contains
    // more than one file.  Reason is that .pls and .m3u may be used
    // for links to http streams etc. 
    IFileDirectory *pDir = new CPlaylistDirectory;
    CFileItemList items;
    if (pDir->GetDirectory(strPath, items))
    {
      if (items.Size() > 1)
        return pDir;
    }
    return NULL;
  }
  return NULL;
}
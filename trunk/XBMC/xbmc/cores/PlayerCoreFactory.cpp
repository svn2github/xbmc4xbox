
#include "../stdafx.h"
#include "PlayerCoreFactory.h"
#include "mplayer.h"
#include "modplayer.h"
#include "SidPlayer.h"
#include "dvdplayer\DVDPlayer.h"
#include "paplayer\paplayer.h"
#include "..\GUIDialogContextMenu.h"


CPlayerCoreFactory::CPlayerCoreFactory()
{}
CPlayerCoreFactory::~CPlayerCoreFactory()
{}

IPlayer* CPlayerCoreFactory::CreatePlayer(const CStdString& strCore, IPlayerCallback& callback) const
{ 
  return CreatePlayer( GetPlayerCore(strCore), callback ); 
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const EPLAYERCORES eCore, IPlayerCallback& callback)
{
  switch( eCore )
  {
    case EPC_DVDPLAYER: return new CDVDPlayer(callback);
    case EPC_MPLAYER: return new CMPlayer(callback);
    case EPC_MODPLAYER: return new ModPlayer(callback);
    case EPC_SIDPLAYER: return new SidPlayer(callback);
    case EPC_PAPLAYER: return new PAPlayer(callback); // added by dataratt
  }
  return NULL;
}

EPLAYERCORES CPlayerCoreFactory::GetPlayerCore(const CStdString& strCore)
{
  CStdString strCoreLower = strCore;
  strCoreLower.ToLower();

  if (strCoreLower == "dvdplayer") return EPC_DVDPLAYER;
  if (strCoreLower == "mplayer") return EPC_MPLAYER;
  if (strCoreLower == "mod") return EPC_MODPLAYER;
  if (strCoreLower == "sid") return EPC_SIDPLAYER;
  if (strCoreLower == "paplayer" ) return EPC_PAPLAYER;
  return EPC_NONE;
}

CStdString CPlayerCoreFactory::GetPlayerName(const EPLAYERCORES eCore)
{
  switch( eCore )
  {
    case EPC_DVDPLAYER: return "DVDPlayer";
    case EPC_MPLAYER: return "MPlayer";
    case EPC_MODPLAYER: return "MODPlayer";
    case EPC_SIDPLAYER: return "SIDPlayer";
    case EPC_PAPLAYER: return "PAPlayer";
  }
  return "";
}

void CPlayerCoreFactory::GetPlayers( VECPLAYERCORES &vecCores )
{
  vecCores.push_back(EPC_MPLAYER);
  vecCores.push_back(EPC_DVDPLAYER);
  vecCores.push_back(EPC_MODPLAYER);
  vecCores.push_back(EPC_SIDPLAYER);
  vecCores.push_back(EPC_PAPLAYER);
}

void CPlayerCoreFactory::GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores)
{
  CURL url(item.m_strPath);
  bool bMPlayer(false), bDVDPlayer(false);

  bool bAllowDVD = strcmp(g_stSettings.m_szExternalDVDPlayer, "dvdplayerbeta") == 0;

  //Only mplayer can handle internetstreams for now
  if ( item.IsInternetStream() )
  {
    vecCores.push_back(EPC_MPLAYER);
    bMPlayer = true;
  }

  if( bAllowDVD && (item.IsDVD() || item.IsDVDFile() || item.IsDVDImage()) )
  {
    vecCores.push_back(EPC_DVDPLAYER);
    bDVDPlayer = true;
  }
  
  if( ModPlayer::IsSupportedFormat(url.GetFileType()) )
  {
    vecCores.push_back(EPC_MODPLAYER);
  }
  
  if( url.GetFileType() == "sid" )
  {
    vecCores.push_back(EPC_SIDPLAYER);
  }

  if( PAPlayer::HandlesType(url.GetFileType()) )
  {
    vecCores.push_back(EPC_PAPLAYER);
  }

  if( item.IsVideo() || item.IsAudio() )
  {
    if( !bMPlayer )
      vecCores.push_back(EPC_MPLAYER);

    if( bAllowDVD && !bDVDPlayer )
      vecCores.push_back(EPC_DVDPLAYER);
  }

}

EPLAYERCORES CPlayerCoreFactory::GetDefaultPlayer( const CFileItem& item )
{
  VECPLAYERCORES vecCores;
  GetPlayers(item, vecCores);

  //If we have any players return the first one
  if( vecCores.size() > 0 ) return vecCores.at(0);
  
  return EPC_NONE;
}

EPLAYERCORES CPlayerCoreFactory::SelectPlayerDialog(VECPLAYERCORES &vecCores, int iPosX, int iPosY)
{

  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);

  // Reset menu
  pMenu->Initialize();

  // Add all possible players
  auto_aptr<int> btn_Cores(NULL);
  if( vecCores.size() > 0 )
  {    
    btn_Cores = new int[ vecCores.size() ];
    btn_Cores[0] = 0;

    //Add all players
    for( unsigned int i = 0; i < vecCores.size(); i++ )
    {
      CStdStringW strCaption;
      strCaption += CPlayerCoreFactory::GetPlayerName(vecCores[i]);
      btn_Cores[i] = pMenu->AddButton(strCaption);
    }
  }

  // Display menu
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(m_gWindowManager.GetActiveWindow());

  //Check what player we selected
  int btnid = pMenu->GetButton();
  for( unsigned int i = 0; i < vecCores.size(); i++ )
  {
    if( btnid == btn_Cores[i] )
    {
      return vecCores[i];
    }
  }

  return EPC_NONE;
}

EPLAYERCORES CPlayerCoreFactory::SelectPlayerDialog(int iPosX, int iPosY)
{
  VECPLAYERCORES vecCores;
  GetPlayers(vecCores);
  return SelectPlayerDialog(vecCores, iPosX, iPosY);
}

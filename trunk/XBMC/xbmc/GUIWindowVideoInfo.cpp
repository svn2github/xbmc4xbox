
#include "stdafx.h"
#include "GUIWindow.h"
#include "GUIWindowVideoInfo.h"
#include "util.h"
#include "picture.h"
#include "VideoDatabase.h"
#include "GUIImage.h"
#include "StringUtils.h"
#include "GUIListControl.h"
#include "GUIWindowVideoBase.h"
#include "GUIWindowVideoFiles.h"
#include "GUIDialogFileBrowser.h"

#define CONTROL_TITLE    20
#define CONTROL_DIRECTOR   21
#define CONTROL_CREDITS    22
#define CONTROL_GENRE    23
#define CONTROL_YEAR    24
#define CONTROL_TAGLINE    25
#define CONTROL_PLOTOUTLINE   26
#define CONTROL_RATING    27
#define CONTROL_VOTES     28
#define CONTROL_CAST     29
#define CONTROL_RATING_AND_VOTES  30
#define CONTROL_RUNTIME    31

#define CONTROL_IMAGE    3
#define CONTROL_TEXTAREA    4

#define CONTROL_BTN_TRACKS   5
#define CONTROL_BTN_REFRESH   6
#define CONTROL_BTN_PLAY      8
#define CONTROL_BTN_RESUME    9
#define CONTROL_BTN_GET_THUMB 10

#define CONTROL_LIST            50
#define CONTROL_DISC             7

CGUIWindowVideoInfo::CGUIWindowVideoInfo(void)
    : CGUIDialog(WINDOW_VIDEO_INFO, "DialogVideoInfo.xml")
{}

CGUIWindowVideoInfo::~CGUIWindowVideoInfo(void)
{}

bool CGUIWindowVideoInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_pMovie = NULL;
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_bRefresh = false;
      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_DISC, 0, 0, NULL);
      g_graphicsContext.SendMessage(msg);
      for (int i = 0; i < 1000; ++i)
      {
        CStdString strItem;
        strItem.Format("DVD#%03i", i);
        CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_DISC, 0, 0);
        msg2.SetLabel(strItem);
        g_graphicsContext.SendMessage(msg2);
      }

      SET_CONTROL_HIDDEN(CONTROL_DISC);
      CONTROL_DISABLE(CONTROL_DISC);
      int iItem = 0;
      CFileItem movie(m_pMovie->m_strPath, false);
      if ( movie.IsOnDVD() )
      {
        SET_CONTROL_VISIBLE(CONTROL_DISC);
        CONTROL_ENABLE(CONTROL_DISC);
        char szNumber[1024];
        int iPos = 0;
        bool bNumber = false;
        for (int i = 0; i < (int)m_pMovie->m_strDVDLabel.size();++i)
        {
          char kar = m_pMovie->m_strDVDLabel.GetAt(i);
          if (kar >= '0' && kar <= '9' )
          {
            szNumber[iPos] = kar;
            iPos++;
            szNumber[iPos] = 0;
            bNumber = true;
          }
          else
          {
            if (bNumber) break;
          }
        }
        int iDVD = 0;
        if (strlen(szNumber))
        {
          int x = 0;
          while (szNumber[x] == '0' && x < (int)strlen(szNumber) ) x++;
          if (x < (int)strlen(szNumber))
          {
            sscanf(&szNumber[x], "%i", &iDVD);
            if (iDVD < 0 && iDVD >= 1000)
              iDVD = -1;
          }
        }
        if (iDVD <= 0) iDVD = 0;
        iItem = iDVD;

        CGUIMessage msgSet(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC, iItem, 0, NULL);
        g_graphicsContext.SendMessage(msgSet);
      }
      Refresh();
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_TRACKS)
      {
        m_bViewReview = !m_bViewReview;
        Update();
      }
      else if (iControl == CONTROL_BTN_PLAY)
      {
        Play();
      }
      else if (iControl == CONTROL_BTN_RESUME)
      {
        Play(true);
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetThumb();
      }
      else if (iControl == CONTROL_DISC)
      {
        int iItem = 0;
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl, 0, 0, NULL);
        g_graphicsContext.SendMessage(msg);
        CStdString strItem = msg.GetLabel();
        if (strItem != "HD" && strItem != "share")
        {
          long lMovieId;
          sscanf(m_pMovie->m_strSearchString.c_str(), "%i", &lMovieId);
          if (lMovieId > 0)
          {
            CStdString label;
            m_database.GetDVDLabel(lMovieId, label);
            int iPos = label.Find("DVD#");
            if (iPos >= 0)
            {
              label.Delete(iPos, label.GetLength());
            }
            label = label.TrimRight(" ");
            label += " ";
            label += strItem;
            m_database.SetDVDLabel( lMovieId, label);
          }
        }
      }
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl, 0, 0, NULL);
          g_graphicsContext.SendMessage(msg);
          int iItem = msg.GetParam1();
          CStdString strItem = m_vecStrCast[iItem];
          int iPos = strItem.Find(" as ");
          if (iPos > 0)
            OnSearch(strItem.Left(iPos));
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowVideoInfo::SetMovie(CIMDBMovie& album, const CFileItem *item)
{
  m_pMovie = &album;
  m_movieItem = *item;
}

void CGUIWindowVideoInfo::Update()
{
  if (!m_pMovie) return ;
  CStdString strTmp;
  strTmp = m_pMovie->m_strTitle; strTmp.Trim();
  SetLabel(CONTROL_TITLE, strTmp.c_str() );

  strTmp = m_pMovie->m_strDirector; strTmp.Trim();
  SetLabel(CONTROL_DIRECTOR, strTmp.c_str() );

  strTmp = m_pMovie->m_strWritingCredits; strTmp.Trim();
  SetLabel(CONTROL_CREDITS, strTmp.c_str() );

  strTmp = m_pMovie->m_strGenre; strTmp.Trim();
  SetLabel(CONTROL_GENRE, strTmp.c_str() );

  {
    CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_TAGLINE);
    OnMessage(msg1);
  }
  {
    strTmp = m_pMovie->m_strTagLine; strTmp.Trim();
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_TAGLINE);
    msg1.SetLabel( strTmp );
    OnMessage(msg1);
  }
  {
    CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PLOTOUTLINE);
    OnMessage(msg1);
  }
  {
    strTmp = m_pMovie->m_strPlotOutline; strTmp.Trim();
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_PLOTOUTLINE);
    msg1.SetLabel( strTmp );
    OnMessage(msg1);
  }
  CStdString strYear;
  strYear.Format("%i", m_pMovie->m_iYear);
  SetLabel(CONTROL_YEAR, strYear );

  CStdString strRating;
  strRating.Format("%03.1f", m_pMovie->m_fRating);
  SetLabel(CONTROL_RATING, strRating );

  strTmp = m_pMovie->m_strVotes; strTmp.Trim();
  SetLabel(CONTROL_VOTES, strTmp.c_str() );

  CStdString strRating_And_Votes;
  if (strRating.Equals("0.0")) {strRating_And_Votes = m_pMovie->m_strVotes;}
  else
    // if rating is 0 there are no votes so display not available message already set in Votes string
  {
    strRating_And_Votes.Format("%s (%s votes)", strRating, strTmp);
    SetLabel(CONTROL_RATING_AND_VOTES, strRating_And_Votes);
  }

  strTmp = m_pMovie->m_strRuntime; strTmp.Trim();
  SetLabel(CONTROL_RUNTIME, strTmp.c_str() );

  // setup plot text area
  strTmp = m_pMovie->m_strPlot; strTmp.Trim();
  SET_CONTROL_LABEL(CONTROL_TEXTAREA, strTmp.c_str() );

  // setup cast list
  strTmp = m_pMovie->m_strCast; strTmp.Trim();
  m_vecStrCast.clear();
  vector<CStdString> vecCast;
  int iNumItems = StringUtils::SplitString(strTmp, "\n", vecCast);
  for (int i = 0; i < (int)vecCast.size(); i++)
  {
    int iPos = vecCast[i].Find(" as ");
    if (iPos > 0)
      m_vecStrCast.push_back(vecCast[i]);
  }
  AddItemsToList(m_vecStrCast);

  if (m_bViewReview)
  {
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 206);
    
    SET_CONTROL_HIDDEN(CONTROL_LIST);
    SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 207);

    SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
    SET_CONTROL_VISIBLE(CONTROL_LIST);
  }

  // Check for resumability
  CGUIWindowVideoFiles *window = (CGUIWindowVideoFiles *)m_gWindowManager.GetWindow(WINDOW_VIDEOS);
  CFileItem movie(m_pMovie->m_strPath, false);
  if (window && window->GetResumeItemOffset(&movie))
  {
    CONTROL_ENABLE(CONTROL_BTN_RESUME);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTN_RESUME);
  }
  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_movieItem.GetThumbnailImage());
  }
}

void CGUIWindowVideoInfo::SetLabel(int iControl, const CStdString& strLabel)
{
  if (strLabel.size() == 0) return ;

  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iControl);
  msg.SetLabel(strLabel);
  OnMessage(msg);

}

void CGUIWindowVideoInfo::Render()
{
  CGUIDialog::Render();
}


void CGUIWindowVideoInfo::Refresh()
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("Network.EnableInternet"))
  {
    Update();
    return ;
  }

  try
  {
    OutputDebugString("Refresh\n");

    CStdString strImage = m_pMovie->m_strPictureURL;

    CStdString thumbImage = m_movieItem.GetThumbnailImage();
    if (!m_movieItem.HasThumbnail())
      CUtil::GetCachedThumbnail(m_movieItem.m_strPath, thumbImage);
    if (!CFile::Exists(thumbImage) && strImage.size() > 0)
    {
      DownloadThumbnail(thumbImage);
    }

    if (!CFile::Exists(thumbImage) )
    {
      thumbImage = "";
    }

    m_movieItem.SetThumbnailImage(thumbImage);

    //OutputDebugString("update\n");
    Update();
    //OutputDebugString("updated\n");
  }
  catch (...)
  {}
}
bool CGUIWindowVideoInfo::NeedRefresh() const
{
  return m_bRefresh;
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowVideoInfo::OnSearch(CStdString& strSearch)
{
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
    m_dlgProgress->SetLine(0, strSearch);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal(GetID());
    m_dlgProgress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItem* pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal(GetID());

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
    {
      if (m_dlgProgress) m_dlgProgress->Close();
      return ;
    }

    CFileItem* pSelItem = new CFileItem(*items[iItem]);

    OnSearchItemFound(pSelItem);

    delete pSelItem;
    if (m_dlgProgress) m_dlgProgress->Close();
  }
  else
  {
    if (m_dlgProgress) m_dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
}

/// \brief Make the actual search for the OnSearch function.
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowVideoInfo::DoSearch(CStdString& strSearch, CFileItemList& items)
{
  VECMOVIES movies;
  m_database.GetMoviesByActor(strSearch, movies);
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CStdString strItem = movies[i].m_strTitle;
    CStdString strYear;
    strYear.Format(" (%i)", movies[i].m_iYear);
    strItem += strYear;
    CFileItem *pItem = new CFileItem(strItem);
    pItem->m_strPath = movies[i].m_strSearchString;
    items.Add(pItem);
  }
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowVideoInfo::OnSearchItemFound(const CFileItem* pItem)
{
  long lMovieId = atol(pItem->m_strPath);
  CIMDBMovie movieDetails;
  m_database.GetMovieInfo(pItem->m_strPath, movieDetails, lMovieId);
  SetMovie(movieDetails, pItem);
  Refresh();
}

void CGUIWindowVideoInfo::AddItemsToList(const vector<CStdString> &vecStr)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  for (int i = 0; i < (int)vecStr.size(); i++)
  {
    CGUIListItem* pItem = new CGUIListItem(vecStr[i]);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, (void*)pItem);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIWindowVideoInfo::Play(bool resume)
{
  CFileItem movie(m_pMovie->m_strPath, false);
  CGUIWindowVideoFiles* pWindow = (CGUIWindowVideoFiles*)m_gWindowManager.GetWindow(WINDOW_VIDEOS);
  if (pWindow)
  {
    // close our dialog
    Close(true);
    if (resume)
      movie.m_lStartOffset = pWindow->GetResumeItemOffset(&movie);
    pWindow->PlayMovie(&movie);
  }
}

void CGUIWindowVideoInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  // disable button with id 10 as we don't have support for it yet!
//  CONTROL_DISABLE(10);
}

bool CGUIWindowVideoInfo::DownloadThumbnail(const CStdString &thumb)
{
  // TODO: This routine should be generalised to allow more than one
  // thumb to be downloaded (possibly from amazon.com or other sources)
  // and then a thumb chooser should be presented, with the current thumb
  // and the downloaded thumbs available (possibly also with a generic
  // file browse option?)
  if (m_pMovie->m_strPictureURL.IsEmpty())
    return false;
  CHTTP http;
  CStdString strExtension;
  CUtil::GetExtension(m_pMovie->m_strPictureURL, strExtension);
  CStdString strTemp = "Z:\\temp";
  strTemp += strExtension;
  ::DeleteFile(strTemp.c_str());
  http.Download(m_pMovie->m_strPictureURL, strTemp);

  try
  {
    CPicture picture;
    picture.DoCreateThumbnail(strTemp, thumb);
  }
  catch (...)
  {
    OutputDebugString("...\n");
    ::DeleteFile(thumb.c_str());
  }
  ::DeleteFile(strTemp.c_str());
  return true;
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  IMDb thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)
void CGUIWindowVideoInfo::OnGetThumb()
{
  CFileItemList items;

  // Grab the thumbnail from the web
  CStdString thumbFromWeb;
  CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "imdbthumb.jpg", thumbFromWeb);
  if (DownloadThumbnail(thumbFromWeb))
  {
    CFileItem *item = new CFileItem("thumb://IMDb", false);
    item->SetThumbnailImage(thumbFromWeb);
    item->SetLabel("IMDb Thumb");
    items.Add(item);
  }

  if (CFile::Exists(m_movieItem.GetThumbnailImage()))
  {
    CFileItem *item = new CFileItem("thumb://Current", false);
    item->SetThumbnailImage(m_movieItem.GetThumbnailImage());
    item->SetLabel("Current Thumb");
    items.Add(item);
  }

  CStdString localThumb, cachedLocalThumb;
  CUtil::GetUserThumbnail(m_movieItem.m_strPath, localThumb);
  if (CFile::Exists(localThumb))
  {
    CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    CPicture pic;
    pic.DoCreateThumbnail(localThumb, cachedLocalThumb);
    CFileItem *item = new CFileItem("thumb://Local", false);
    item->SetThumbnailImage(cachedLocalThumb);
    item->SetLabel("Local Thumb");
    items.Add(item);
  }
  else
  { // no local thumb exists, so we are just using the IMDb thumb or cached thumb
    // which is probably the IMDb thumb.  These could be wrong, so allow the user
    // to delete the incorrect thumb
    if (0 == items.Size())
    { // no cached thumb or no imdb thumb available
      // TODO: tell user and return
      return;
    }
    CFileItem *item = new CFileItem("thumb://None", false);
    item->SetThumbnailImage("defaultVideoBig.png");
    item->SetLabel("No Thumb");
    items.Add(item);
  }

  CStdString result;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, "Choose Thumbnail", result))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CStdString cachedThumb;
  CUtil::GetCachedThumbnail(m_movieItem.m_strPath, cachedThumb);

  if (result == "thumb://None")
  { // delete any cached thumb
    CFile::Delete(cachedThumb);
    cachedThumb.Empty();
  }
  else if (result == "thumb://IMDb")
    CFile::Cache(thumbFromWeb, cachedThumb);
  else // if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);

  m_movieItem.SetThumbnailImage(cachedThumb);

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS, 0, NULL);
  g_graphicsContext.SendMessage(msg);
  // Update our screen
  Update();
}
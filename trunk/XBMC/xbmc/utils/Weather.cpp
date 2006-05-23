
#include "../stdafx.h"
#include "Weather.h"
#include "../FileSystem/ZipManager.h"
#include "../FileSystem/RarManager.h"
#include "HTTP.h"
#include "xmlutils.h"
#include "../temperature.h"
#include "../systemtime.h"


#define CONTROL_BTNREFRESH  2
#define CONTROL_SELECTLOCATION 3
#define CONTROL_LABELLOCATION 10
#define CONTROL_LABELUPDATED 11
#define CONTROL_IMAGELOGO  101

#define CONTROL_IMAGENOWICON 21
#define CONTROL_LABELNOWCOND 22
#define CONTROL_LABELNOWTEMP 23
#define CONTROL_LABELNOWFEEL 24
#define CONTROL_LABELNOWUVID 25
#define CONTROL_LABELNOWWIND 26
#define CONTROL_LABELNOWDEWP 27
#define CONTROL_LABELNOWHUMI 28

#define CONTROL_STATICTEMP  223
#define CONTROL_STATICFEEL  224
#define CONTROL_STATICUVID  225
#define CONTROL_STATICWIND  226
#define CONTROL_STATICDEWP  227
#define CONTROL_STATICHUMI  228

#define CONTROL_LABELD0DAY  31
#define CONTROL_LABELD0HI  32
#define CONTROL_LABELD0LOW  33
#define CONTROL_LABELD0GEN  34
#define CONTROL_IMAGED0IMG  35

#define PARTNER_ID    "1004124588"   //weather.com partner id
#define PARTNER_KEY    "079f24145f208494"  //weather.com partner key

#define MAX_LOCATION   3
#define LOCALIZED_TOKEN_FIRSTID   370
#define LOCALIZED_TOKEN_LASTID   395
#define LOCALIZED_TOKEN_FIRSTID2 1396
#define LOCALIZED_TOKEN_LASTID2   1410 
/*
FIXME'S
>strings are not centered
>weather.com dev account is mine not a general xbmc one
*/

/*const CStdString strBasePath = "Q:\\weather\\";
const bool bUseZip = false;
const bool bUseRar = false;
const CStdString strZipFile = "";
const CStdString strRarFile = "";*/

// FOR ZIP
/*const CStdString strBasePath = "Z:\\weather\\";
const bool bUseZip = true;
const bool bUseRar = false;
const CStdString strZipFile = "Q:\\weather\\weather.zip";
const CStdString strRarFile = "Q:\\weather\\weather.rar";*/

// OR THESE FOR RAR
const CStdString strBasePath = "Z:\\weather\\";
const bool bUseZip = false;
const bool bUseRar = true;
const CStdString strZipFile = "";
const CStdString strRarFile = "Q:\\media\\weather.rar";

CWeather g_weatherManager;

void CBackgroundWeatherLoader::GetInformation()
{
  if (!g_guiSettings.GetBool("Network.EnableInternet"))
    return;
  CWeather *callback = (CWeather *)m_callback;
  // Download our weather
  CLog::Log(LOGINFO, "WEATHER: Downloading weather");
  CHTTP httpUtil;
  CStdString strURL;

  CStdString strSetting;
  strSetting.Format("Weather.AreaCode%i", callback->GetArea() + 1);
  strURL.Format("http://xoap.weather.com/weather/local/%s?cc=*&unit=m&dayf=4&prod=xoap&par=%s&key=%s",
                g_guiSettings.GetString(strSetting), PARTNER_ID, PARTNER_KEY);
  CStdString strWeatherFile = "Z:\\curWeather.xml";
  if (httpUtil.Download(strURL, strWeatherFile))
  {
    CLog::Log(LOGINFO, "WEATHER: Weather download successful");
    if (!callback->m_bImagesOkay)
    {
      CDirectory::Create(strBasePath);
      if (bUseZip)
        g_ZipManager.ExtractArchive(strZipFile,strBasePath);
      else if (bUseRar)
        g_RarManager.ExtractArchive(strRarFile,strBasePath);
      callback->m_bImagesOkay = true;
    }
    callback->LoadWeather(strWeatherFile);
  }
  else
    CLog::Log(LOGERROR, "WEATHER: Weather download failed!");
}

CWeather::CWeather(void) : CInfoLoader("weather")
{
  m_bImagesOkay = false;
  for (int i = 0; i < MAX_LOCATION; i++)
  {
    strcpy(m_szLocation[i], "");
  }
  // empty all our strings etc.
  strcpy(m_szLastUpdateTime, "");
  //strcpy(m_szCurrentIcon, "Q:\\weather\\128x128\\na.png");
  /*strcpy(m_szCurrentIcon,strBasePath.c_str());
  strcat(m_szCurrentIcon,"128x128\\na.png");*/
  strcpy(m_szCurrentIcon,"");
  strcpy(m_szCurrentConditions, "");
  strcpy(m_szCurrentTemperature, "");
  strcpy(m_szCurrentFeelsLike, "");

  strcpy(m_szCurrentWind, "");
  strcpy(m_szCurrentHumidity, "");
  strcpy(m_szCurrentUVIndex, "");
  strcpy(m_szCurrentDewPoint, "");

  //loop here as well
  for (int i = 0; i < NUM_DAYS; i++)
  {
    //strcpy(m_dfForcast[i].m_szIcon, "Q:\\weather\\64x64\\na.png");
    /*strcpy(m_dfForcast[i].m_szIcon,strBasePath.c_str());
    strcat(m_dfForcast[i].m_szIcon,"64x64\\na.png");*/
    strcat(m_dfForcast[i].m_szIcon,"");
    strcpy(m_dfForcast[i].m_szOverview, "");
    strcpy(m_dfForcast[i].m_szDay, "");
    strcpy(m_dfForcast[i].m_szHigh, "");
    strcpy(m_dfForcast[i].m_szLow, "");
  }
  for (int i = 0; i < MAX_LOCATION; i++)
  {
    strcpy(m_szLocation[i], "");
  }
  m_iCurWeather = 0;
  srand(timeGetTime());
}

CWeather::~CWeather(void)
{
}

void CWeather::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue)
{
  strcpy(szValue, "");
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    CStdString strValue = pChild->FirstChild()->Value();
    if (strValue.size() )
    {
      if (strValue != "-")
        strcpy(szValue, strValue.c_str());
    }
  }
  if (strlen(szValue) == 0)
  {
    strcpy(szValue, strDefaultValue.c_str());
  }
}

void CWeather::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue)
{
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    iValue = atoi( pChild->FirstChild()->Value() );
  }
}

void CWeather::LocalizeOverviewToken(char *szToken, bool bAppendSpace)
{
  CStdString strLocStr = "";
  CStdString token = szToken;
  if (!token.IsEmpty())
  {
    ilocalizedTokens i;
    i = m_localizedTokens.find(token);
    if (i != m_localizedTokens.end())
    {
      strLocStr = g_localizeStrings.Get(i->second);
    }
  }
  if (strLocStr == "")
    strLocStr = szToken; //if not found, let fallback
  if (bAppendSpace)
    strLocStr += " ";     //append space if applicable
  strcpy(szToken, strLocStr.c_str());
}

void CWeather::LocalizeOverview(char *szStr)
{
  char loc[256];
  char szToken[256];
  int intOffset = 0;
  char *pnt = NULL;
  memset(loc, '\0', sizeof(loc));

  while ((pnt = strstr(szStr + intOffset, " ")) != NULL)
  {
    //get the length of this token (everything before pnt)
    int iTokenLen = (int)(strlen(szStr) - strlen(pnt) - intOffset);
    strncpy(szToken, szStr + intOffset, iTokenLen); //extract the token
    szToken[iTokenLen] = '\0';      //stick an end on it
    LocalizeOverviewToken(szToken);     //localize
    strcpy(loc + strlen(loc), szToken);    //add it to the end of loc
    intOffset += iTokenLen + 1;      //update offset for next strstr search
  }
  strncpy(szToken, szStr + intOffset, strlen(szStr) - intOffset); //last word, copy the rest of the string
  szToken[strlen(szStr) - intOffset] = '\0';     //stick an end on it
  LocalizeOverviewToken(szToken);        //localize
  strcpy(loc + strlen(loc), szToken);       //add it to the end of loc
  strcpy(szStr, loc);           //copy loc over the original input string
}

// input param must be kmh
int CWeather::ConvertSpeed(int curSpeed)
{
  switch (g_langInfo.GetSpeedUnit())
  {
  case CLangInfo::SPEED_UNIT_KMH:
    break;
  case CLangInfo::SPEED_UNIT_MPH:
    curSpeed=(int)(curSpeed / (8.0 / 5.0));
    break;
  case CLangInfo::SPEED_UNIT_MPS:
    curSpeed=(int)(curSpeed * (1000.0 / 3600.0) + 0.5);
    break;
  default:
    assert(false);
  }

  return curSpeed;
}

bool CWeather::LoadWeather(const CStdString &strWeatherFile)
{
  int iTmpInt;
  char iTmpStr[256];
  SYSTEMTIME time;

  GetLocalTime(&time); //used when deciding what weather to grab for today

  // Load in our tokens if necessary
  if (!m_localizedTokens.size())
    LoadLocalizedToken();

  // load the xml file
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strWeatherFile))
  {
    CLog::Log(LOGERROR, "WEATHER: Unable to get data - invalid XML");
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR, "WEATHER: Unable to get data - invalid XML");
    return false;
  }

  //if root element is 'error' display the error message
  if (strcmp(pRootElement->Value(), "error") == 0)
  {
    char szCheckError[256];
    GetString(pRootElement, "err", szCheckError, "Unknown Error"); //grab the error string
    CLog::Log(LOGERROR, "WEATHER: Unable to get data: %s", szCheckError);
    return false;
  }

  // location
  TiXmlElement *pElement = pRootElement->FirstChildElement("loc");
  if (pElement)
  {
    GetString(pElement, "dnam", m_szLocation[m_iCurWeather], "");
  }

  //current weather
  pElement = pRootElement->FirstChildElement("cc");
  if (pElement)
  {
    // Use the local date/time the file is parsed...
    CSystemTime time;
    time.Now();
    CStdString strDateTime=time.GetAsLocalizedDate();
    strDateTime+=" ";
    strDateTime+=time.GetAsLocalizedTime();
    strcpy(m_szLastUpdateTime, strDateTime.c_str());

    // ...and not the date/time from weather.com
    //GetString(pElement, "lsup", m_szLastUpdateTime, "");

    GetString(pElement, "icon", iTmpStr, ""); //string cause i've seen it return N/A
    if (strcmp(iTmpStr, "N/A") == 0)
    {
      sprintf(m_szCurrentIcon, "%s128x128\\na.png",strBasePath.c_str());
    }
    else
      sprintf(m_szCurrentIcon, "%s128x128\\%s.png", strBasePath.c_str(),iTmpStr);

    GetString(pElement, "t", m_szCurrentConditions, "");   //current condition
    LocalizeOverview(m_szCurrentConditions);

    GetInteger(pElement, "tmp", iTmpInt);    //current temp
    CTemperature temp=CTemperature::CreateFromCelsius(iTmpInt);
    sprintf(m_szCurrentTemperature, "%s", temp.ToString().c_str());
    GetInteger(pElement, "flik", iTmpInt);    //current 'Feels Like'
    CTemperature tempFlik=CTemperature::CreateFromCelsius(iTmpInt);
    sprintf(m_szCurrentFeelsLike, "%s", tempFlik.ToString().c_str());

    TiXmlElement *pNestElement = pElement->FirstChildElement("wind"); //current wind
    if (pNestElement)
    {
      GetInteger(pNestElement, "s", iTmpInt);   //current wind strength
      iTmpInt = ConvertSpeed(iTmpInt);    //convert speed if needed
      GetString(pNestElement, "t", iTmpStr, "N");  //current wind direction

      //From <dir eg NW> at <speed> km/h   g_localizeStrings.Get(407)
      //This is a bit untidy, but i'm fed up with localization and string formats :)
      CStdString szWindFrom = g_localizeStrings.Get(407);
      CStdString szWindAt = g_localizeStrings.Get(408);
      CStdString szCalm = g_localizeStrings.Get(1410);

      // get speed unit
      char szUnitSpeed[5];
      strcpy(szUnitSpeed, g_langInfo.GetSpeedUnitString().c_str());

      if (strcmp(iTmpStr,"CALM") == 0)
        sprintf(m_szCurrentWind, "%s", szCalm.c_str());
      else
        sprintf(m_szCurrentWind, "%s %s %s %i %s",
              szWindFrom.GetBuffer(szWindFrom.GetLength()), iTmpStr,
              szWindAt.GetBuffer(szWindAt.GetLength()), iTmpInt, szUnitSpeed);
    }

    GetInteger(pElement, "hmid", iTmpInt);    //current humidity
    sprintf(m_szCurrentHumidity, "%i%%", iTmpInt);

    pNestElement = pElement->FirstChildElement("uv"); //current UV index
    if (pNestElement)
    {
      GetInteger(pNestElement, "i", iTmpInt);
      GetString(pNestElement, "t", iTmpStr, "");
      LocalizeOverviewToken(iTmpStr, false);
      sprintf(m_szCurrentUVIndex, "%i %s", iTmpInt, iTmpStr);
    }

    GetInteger(pElement, "dewp", iTmpInt);    //current dew point
    CTemperature dewPoint=CTemperature::CreateFromCelsius(iTmpInt);
    sprintf(m_szCurrentDewPoint, "%s", dewPoint.ToString().c_str());
  }
  //future forcast
  pElement = pRootElement->FirstChildElement("dayf");
  if (pElement)
  {
    TiXmlElement *pOneDayElement = pElement->FirstChildElement("day");;
    for (int i = 0; i < NUM_DAYS; i++)
    {
      if (pOneDayElement)
      {
        strcpy(m_dfForcast[i].m_szDay, pOneDayElement->Attribute("t"));
        LocalizeDay(m_dfForcast[i].m_szDay);

        GetString(pOneDayElement, "hi", iTmpStr, ""); //string cause i've seen it return N/A
        if (strcmp(iTmpStr, "N/A") == 0)
          strcpy(m_dfForcast[i].m_szHigh, "");
        else
        {
          CTemperature temp=CTemperature::CreateFromCelsius(atoi(iTmpStr));
          sprintf(m_dfForcast[i].m_szHigh, "%s", temp.ToString().c_str());
        }

        GetString(pOneDayElement, "low", iTmpStr, "");
        if (strcmp(iTmpStr, "N/A") == 0)
          strcpy(m_dfForcast[i].m_szHigh, "");
        else
        {
          CTemperature temp=CTemperature::CreateFromCelsius(atoi(iTmpStr));
          sprintf(m_dfForcast[i].m_szLow, "%s", temp.ToString().c_str());
        }

        TiXmlElement *pDayTimeElement = pOneDayElement->FirstChildElement("part"); //grab the first day/night part (should be day)
        if (i == 0 && (time.wHour < 7 || time.wHour >= 19)) //weather.com works on a 7am to 7pm basis so grab night if its late in the day
          pDayTimeElement = pDayTimeElement->NextSiblingElement("part");

        if (pDayTimeElement)
        {
          GetString(pDayTimeElement, "icon", iTmpStr, ""); //string cause i've seen it return N/A
          if (strcmp(iTmpStr, "N/A") == 0)
            sprintf(m_dfForcast[i].m_szIcon, "%s64x64\\na.png",strBasePath.c_str());
          else
            sprintf(m_dfForcast[i].m_szIcon, "%s64x64\\%s.png", strBasePath.c_str(),iTmpStr);

          GetString(pDayTimeElement, "t", m_dfForcast[i].m_szOverview, "");
          LocalizeOverview(m_dfForcast[i].m_szOverview);
        }
      }
      pOneDayElement = pOneDayElement->NextSiblingElement("day");
    }
  }
  return true;
}

//convert weather.com day strings into localized string id's
void CWeather::LocalizeDay(char *szDay)
{
  CStdString strLocDay;

  if (strcmp(szDay, "Monday") == 0)   //monday is localized string 11
    strLocDay = g_localizeStrings.Get(11);
  else if (strcmp(szDay, "Tuesday") == 0)
    strLocDay = g_localizeStrings.Get(12);
  else if (strcmp(szDay, "Wednesday") == 0)
    strLocDay = g_localizeStrings.Get(13);
  else if (strcmp(szDay, "Thursday") == 0)
    strLocDay = g_localizeStrings.Get(14);
  else if (strcmp(szDay, "Friday") == 0)
    strLocDay = g_localizeStrings.Get(15);
  else if (strcmp(szDay, "Saturday") == 0)
    strLocDay = g_localizeStrings.Get(16);
  else if (strcmp(szDay, "Sunday") == 0)
    strLocDay = g_localizeStrings.Get(17);
  else
    strLocDay = "";

  strcpy(szDay, strLocDay.GetBuffer(strLocDay.GetLength()));
}


void CWeather::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile("Q:\\language\\english\\strings.xml") )
  {
    return ;
  }

  CStdString strEncoding;
  XMLUtils::GetEncoding(&xmlDoc, strEncoding);

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("strings")) return ;
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue = pChild->Value();
    if (strValue == "string")
    {
      const TiXmlNode *pChildID = pChild->FirstChild("id");
      const TiXmlNode *pChildText = pChild->FirstChild("value");
      if (pChildID && pChildID->FirstChild() && pChildText && pChildText->FirstChild())
      {
        DWORD dwID = atoi(pChildID->FirstChild()->Value());
        if ( (LOCALIZED_TOKEN_FIRSTID <= dwID && dwID <= LOCALIZED_TOKEN_LASTID) ||
            (LOCALIZED_TOKEN_FIRSTID2 <= dwID && dwID <= LOCALIZED_TOKEN_LASTID2) )
        {
          CStdString utf8Label;
          if (strEncoding.IsEmpty()) // Is language file utf8?
            utf8Label=pChildText->FirstChild()->Value();
          else
            g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8Label);

          if (!utf8Label.IsEmpty())
            m_localizedTokens.insert(std::pair<string, DWORD>(utf8Label, dwID));
        }
      }
    }
    pChild = pChild->NextSibling();
  }
}

bool CWeather::GetSearchResults(const CStdString &strSearch, CStdString &strResult)
{
  // Check to see if the user entered a weather.com code
  if (strSearch.size() == 8)
  {
    strResult = "";
    int i = 0;
    for (i = 0; i < 4; ++i)
    {
      strResult += toupper(strSearch[i]);
      if (!isalpha(strSearch[i]))
        break;
    }
    if (i == 4)
    {
      for ( ; i < 8; ++i)
      {
        strResult += strSearch[i];
        if (!isdigit(strSearch[i]))
          break;
      }
      if (i == 8)
      {
        return true; // match
      }
    }
    // no match, wipe string
    strResult = "";
  }

  CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIDialogProgress *pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  //do the download
  CHTTP httpUtil;
  CStdString strURL;
  //  CStdString strResultsFile = "Z:\\searchresults.xml";
  CStdString strXML;

  if (pDlgProgress)
  {
    pDlgProgress->SetHeading(410);       //"Accessing Weather.com"
    pDlgProgress->SetLine(0, 194);       //"Searching"
    pDlgProgress->SetLine(1, strSearch);
    pDlgProgress->SetLine(2, "");
    pDlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
    pDlgProgress->Progress();
  }

  strURL.Format("http://xoap.weather.com/search/search?where=%s", strSearch);

  if (!httpUtil.Get(strURL, strXML))
  {
    if (pDlgProgress) pDlgProgress->Close();
    return false;
  }

  //some select dialog init stuff
  if (!pDlgSelect)
  {
    if (pDlgProgress) pDlgProgress->Close();
    return false;
  }

  pDlgSelect->SetHeading(396); //"Select Location"
  pDlgSelect->Reset();

  ///////////////////////////////
  // load the xml file
  ///////////////////////////////
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(strXML.c_str());
  if (xmlDoc.Error())
    return false;

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  TiXmlElement *pElement = pRootElement->FirstChildElement("loc");
  CStdString strItemTmp;
  while (pElement)
  {
    strItemTmp.Format("%s - %s", pElement->Attribute("id"), pElement->FirstChild()->Value());
    pDlgSelect->Add(strItemTmp);
    pElement = pElement->NextSiblingElement("loc");
  }

  if (pDlgProgress) pDlgProgress->Close();

  pDlgSelect->EnableButton(TRUE);
  pDlgSelect->SetButtonLabel(222); //'Cancel' button returns to weather settings
  pDlgSelect->DoModal(m_gWindowManager.GetActiveWindow());

  if (pDlgSelect->GetSelectedLabel() < 0)
  {
    if (pDlgSelect->IsButtonPressed())
    {
      pDlgSelect->Close(); //close the select dialog and return to weather settings
      return true;
    }
  }

  //copy the selected code into the settings
  if (pDlgSelect->GetSelectedLabel() >= 0)
  {
    CStdString areacode;
    areacode = pDlgSelect->GetSelectedLabelText();
    strResult = areacode.substr(0, areacode.Find("-") - 1);
  }

  if (pDlgProgress) pDlgProgress->Close();

  return true;
}

const char *CWeather::BusyInfo(DWORD dwInfo)
{
  if (dwInfo == WEATHER_IMAGE_CURRENT_ICON)
  {
    sprintf(m_szNAIcon,"%s128x128\\na.png",strBasePath.c_str());
    return m_szNAIcon;
  }
  return CInfoLoader::BusyInfo(dwInfo);
}

const char *CWeather::TranslateInfo(DWORD dwInfo)
{
  if (dwInfo == WEATHER_LABEL_CURRENT_COND) return m_szCurrentConditions;
  else if (dwInfo == WEATHER_IMAGE_CURRENT_ICON) return m_szCurrentIcon;
  else if (dwInfo == WEATHER_LABEL_CURRENT_TEMP) return m_szCurrentTemperature;
  else if (dwInfo == WEATHER_LABEL_CURRENT_FEEL) return m_szCurrentFeelsLike;
  else if (dwInfo == WEATHER_LABEL_CURRENT_UVID) return m_szCurrentUVIndex;
  else if (dwInfo == WEATHER_LABEL_CURRENT_WIND) return m_szCurrentWind;
  else if (dwInfo == WEATHER_LABEL_CURRENT_DEWP) return m_szCurrentDewPoint;
  else if (dwInfo == WEATHER_LABEL_CURRENT_HUMI) return m_szCurrentHumidity;
  else if (dwInfo == WEATHER_LABEL_LOCATION) return m_szLocation[m_iCurWeather];
  return "";
}

DWORD CWeather::TimeToNextRefreshInMs()
{
  return g_guiSettings.GetInt("Weather.RefreshTime") * 60000;
}
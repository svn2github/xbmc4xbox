// Shortcut.cpp: implementation of the CShortcut class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Shortcut.h"
#include "util.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShortcut::CShortcut()
{
}

CShortcut::~CShortcut()
{
}

bool CShortcut::Create(const CStdString& szPath)
{
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( szPath.c_str() ) )
    return FALSE;

  bool bPath = false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "shortcut")
    return false;
  const TiXmlNode *pChild = pRootElement->FirstChild();

  m_strCustomGame.Empty();
  while (pChild > 0)
  {
    CStdString strValue = pChild->Value();
    if (strValue == "path")
    {
      m_strPath = pChild->FirstChild()->Value();
      bPath = true;
    }

    if (strValue == "video")
    {
      m_strVideo = pChild->FirstChild()->Value();
    }

    if (strValue == "parameters")
    {
      m_strParameters = pChild->FirstChild()->Value();
    }
    if (strValue == "custom")
    {
      const TiXmlNode* pCustomElement = pChild->FirstChildElement();
      while (pCustomElement > 0)
      {
        CStdString strCustomValue = pCustomElement->Value();
        if (strCustomValue == "game")
          m_strCustomGame = pCustomElement->FirstChild()->Value();

        pCustomElement = pCustomElement->NextSibling();
      }
    }

    pChild = pChild->NextSibling();

  }

  return bPath ? true : false;
}

bool CShortcut::Save(const CStdString& strFileName)
{
  if (g_stSettings.m_szShortcutDirectory[0] == 0) return false;

  // Make shortcut filename fatx compatible
  CStdString strShort = strFileName;
  CUtil::ShortenFileName(strShort);
  CUtil::RemoveIllegalChars(strShort);

  CStdString strTotalPath;
  strTotalPath.Format("%s\\%s.cut", g_stSettings.m_szShortcutDirectory, strShort.c_str());

  // Remove old file
  ::DeleteFile(strTotalPath.c_str());

  // Create shortcut document:
  // <shortcut>
  //   <path>F:\App\default.xbe</path>
  // </shortcut>
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("shortcut");
  TiXmlNode *pRootNode = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRootNode) return false;

  TiXmlElement newElement("path");
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (!pNewNode) return false;

  TiXmlText value(m_strPath);
  pNewNode->InsertEndChild(value);

  return xmlDoc.SaveFile(strTotalPath);
}

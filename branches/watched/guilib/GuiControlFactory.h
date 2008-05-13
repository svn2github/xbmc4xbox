/*!
\file GuiControlFactory.h
\brief 
*/

#ifndef GUI_CONTROL_FACTORY_H
#define GIU_CONTROL_FACTORY_H

#pragma once

#include "GUIControl.h"
#include "guiImage.h"

class CImage; // forward
class TiXmlNode;

/*!
 \ingroup controls
 \brief 
 */
class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  static CStdString GetType(const TiXmlElement *pControlNode);
  CGUIControl* Create(DWORD dwParentId, const FRECT &rect, TiXmlElement* pControlNode);
  void ScaleElement(TiXmlElement *element, RESOLUTION fileRes, RESOLUTION destRes);
  static bool GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value);
  static bool GetAspectRatio(const TiXmlNode* pRootNode, const char* strTag, CGUIImage::CAspectRatio &aspectRatio);
  static bool GetTexture(const TiXmlNode* pRootNode, const char* strTag, CImage &image);
  static bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
  static bool GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
  static bool GetAnimations(const TiXmlNode *control, const FRECT &rect, std::vector<CAnimation> &animation);
  static void GetInfoLabel(const TiXmlNode *pControlNode, const CStdString &labelTag, CGUIInfoLabel &infoLabel);
  static void GetInfoLabels(const TiXmlNode *pControlNode, const CStdString &labelTag, std::vector<CGUIInfoLabel> &infoLabels);
  static bool GetColor(const TiXmlNode* pRootNode, const char* strTag, DWORD &value);
  static bool GetInfoColor(const TiXmlNode* pRootNode, const char* strTag, CGUIInfoColor &value);
  static CStdString FilterLabel(const CStdString &label);
  static bool GetConditionalVisibility(const TiXmlNode* control, int &condition);
  static bool GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, std::vector<CStdString>& vecStringValue);
  static void GetRectFromString(const CStdString &string, FRECT &rect);
private:
  bool GetNavigation(const TiXmlElement *node, const char *tag, DWORD &direction, std::vector<CStdString> &actions);
  bool GetCondition(const TiXmlNode *control, const char *tag, int &condition);
  static bool GetConditionalVisibility(const TiXmlNode* control, int &condition, bool &allowHiddenFocus);
  bool GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath);
  bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strString);
  bool GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& iMinValue, float& iMaxValue, float& iIntervalValue);
  bool GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue);
  bool GetHitRect(const TiXmlNode* pRootNode, CRect &rect);
};
#endif

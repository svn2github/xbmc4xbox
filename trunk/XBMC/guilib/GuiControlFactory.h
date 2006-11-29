/*!
\file GuiControlFactory.h
\brief 
*/

#ifndef CGUIControlFactory_H
#define CGUIControlFactory_H

#pragma once

#include "GUIControl.h"
#include "GUIImage.h" // for aspect ratio

class CImage; // forward

/*!
 \ingroup controls
 \brief 
 */
class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  CStdString GetType(const TiXmlElement *pControlNode);
  CGUIControl* Create(DWORD dwParentId, CGUIControl *group, TiXmlElement* pControlNode);
  bool GetConditionalVisibility(const TiXmlNode* control, int &condition);
  void ScaleElement(TiXmlElement *element, RESOLUTION fileRes, RESOLUTION destRes);
  bool GetAspectRatio(const TiXmlNode* pRootNode, const char* strTag, CGUIImage::GUIIMAGE_ASPECT_RATIO &aspectRatio);
  bool GetTexture(const TiXmlNode* pRootNode, const char* strTag, CImage &image);
  bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
  bool GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
private:
  bool GetConditionalVisibility(const TiXmlNode* control, int &condition, bool &allowHiddenFocus);
  bool GetAnimations(const TiXmlNode *control, vector<CAnimation> &animation);
  bool GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, vector<CStdString>& vecStringValue);
  bool GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath);
  bool GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& iMinValue, float& iMaxValue, float& iIntervalValue);
  bool GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue);
};
#endif

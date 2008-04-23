#include "include.h"
#include "GuiControlFactory.h"
#include "LocalizeStrings.h"
#include "GUIButtonControl.h"
#include "GUIRadioButtonControl.h"
#include "GUISpinControl.h"
#include "GUIRSSControl.h"
#include "GUIConsoleControl.h"
#include "GUIListControlEx.h"
#include "guiImage.h"
#include "GUIBorderedImage.h"
#include "GUILabelControl.h"
#include "GUIEditControl.h"
#include "GUIFadeLabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIToggleButtonControl.h"
#include "GUITextBox.h"
#include "GUIVideoControl.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUISelectButtonControl.h"
#include "GUIMoverControl.h"
#include "GUIResizeControl.h"
#include "GUIButtonScroller.h"
#include "GUISpinControlEx.h"
#include "GUIVisualisationControl.h"
#include "GUISettingsSliderControl.h"
#include "GUIMultiImage.h"
#include "GUIControlGroup.h"
#include "GUIControlGroupList.h"
#include "GUIScrollBarControl.h"
#include "GUIListContainer.h"
#include "GUIFixedListContainer.h"
#include "GUIWrappingListContainer.h"
#include "GUIPanelContainer.h"
#include "GUILargeImage.h"
#include "GUIMultiSelectText.h"
#include "utils/GUIInfoManager.h"
#include "utils/CharsetConverter.h"
#include "ButtonTranslator.h"
#include "XMLUtils.h"
#include "GUIFontManager.h"
#include "GUIColorManager.h"
#include "SkinInfo.h"
#include "Settings.h"

using namespace std;

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "SkinInfo.h"
#endif

CGUIControlFactory::CGUIControlFactory(void)
{}

CGUIControlFactory::~CGUIControlFactory(void)
{}

bool CGUIControlFactory::GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild()) return false;
  iMinValue = atoi(pNode->FirstChild()->Value());
  const char* maxValue = strchr(pNode->FirstChild()->Value(), ',');
  if (maxValue)
  {
    maxValue++;
    iMaxValue = atoi(maxValue);

    const char* intervalValue = strchr(maxValue, ',');
    if (intervalValue)
    {
      intervalValue++;
      iIntervalValue = atoi(intervalValue);
    }
  }

  return true;
}

bool CGUIControlFactory::GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& fMinValue, float& fMaxValue, float& fIntervalValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild()) return false;
  fMinValue = (float)atof(pNode->FirstChild()->Value());
  const char* maxValue = strchr(pNode->FirstChild()->Value(), ',');
  if (maxValue)
  {
    maxValue++;
    fMaxValue = (float)atof(maxValue);

    const char* intervalValue = strchr(maxValue, ',');
    if (intervalValue)
    {
      intervalValue++;
      fIntervalValue = (float)atof(intervalValue);
    }
  }

  return true;
}

bool CGUIControlFactory::GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  return g_SkinInfo.ResolveConstant(pNode->FirstChild()->Value(), value);
}

bool CGUIControlFactory::GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, vector<CStdString>& vecStringValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  vecStringValue.clear();
  bool bFound = false;
  while (pNode)
  {
    const TiXmlNode *pChild = pNode->FirstChild();
    if (pChild != NULL)
    {
      vecStringValue.push_back(pChild->Value());
      bFound = true;
    }
    pNode = pNode->NextSibling(strTag);
  }
  return bFound;
}

bool CGUIControlFactory::GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  strStringPath = pNode->FirstChild() ? pNode->FirstChild()->Value() : "";
  strStringPath.Replace('/', '\\');
  return true;
}

bool CGUIControlFactory::GetAspectRatio(const TiXmlNode* pRootNode, const char* strTag, CGUIImage::CAspectRatio &aspect)
{
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  bool keepAR;
  // backward compatibility
  if (XMLUtils::GetBoolean(pRootNode, "keepaspectratio", keepAR))
  {
    aspect.ratio = CGUIImage::CAspectRatio::AR_KEEP;
    return true;
  }
#endif
  CStdString ratio;
  const TiXmlElement *node = pRootNode->FirstChildElement(strTag);
  if (!node || !node->FirstChild())
    return false;

  ratio = node->FirstChild()->Value();
  if (ratio.CompareNoCase("keep") == 0) aspect.ratio = CGUIImage::CAspectRatio::AR_KEEP;
  else if (ratio.CompareNoCase("scale") == 0) aspect.ratio = CGUIImage::CAspectRatio::AR_SCALE;
  else if (ratio.CompareNoCase("center") == 0) aspect.ratio = CGUIImage::CAspectRatio::AR_CENTER;
  else if (ratio.CompareNoCase("stretch") == 0) aspect.ratio = CGUIImage::CAspectRatio::AR_STRETCH;

  const char *attribute = node->Attribute("align");
  if (attribute)
  {
    CStdString align(attribute);
    if (align.CompareNoCase("center") == 0) aspect.align = ASPECT_ALIGN_CENTER | (aspect.align & ASPECT_ALIGNY_MASK);
    else if (align.CompareNoCase("right") == 0) aspect.align = ASPECT_ALIGN_RIGHT | (aspect.align & ASPECT_ALIGNY_MASK);
    else if (align.CompareNoCase("left") == 0) aspect.align = ASPECT_ALIGN_LEFT | (aspect.align & ASPECT_ALIGNY_MASK);
  }
  attribute = node->Attribute("aligny");
  if (attribute)
  {
    CStdString align(attribute);
    if (align.CompareNoCase("center") == 0) aspect.align = ASPECT_ALIGNY_CENTER | (aspect.align & ASPECT_ALIGN_MASK);
    else if (align.CompareNoCase("bottom") == 0) aspect.align = ASPECT_ALIGNY_BOTTOM | (aspect.align & ASPECT_ALIGN_MASK);
    else if (align.CompareNoCase("top") == 0) aspect.align = ASPECT_ALIGNY_TOP | (aspect.align & ASPECT_ALIGN_MASK);
  }
  attribute = node->Attribute("scalediffuse");
  if (attribute)
  {
    CStdString scale(attribute);
    if (scale.CompareNoCase("true") == 0 || scale.CompareNoCase("yes") == 0)
      aspect.scaleDiffuse = true;
    else
      aspect.scaleDiffuse = false;
  }
  return true;
}

bool CGUIControlFactory::GetTexture(const TiXmlNode* pRootNode, const char* strTag, CImage &image)
{
  const TiXmlElement* pNode = pRootNode->FirstChildElement(strTag);
  if (!pNode) return false;
  const char *border = pNode->Attribute("border");
  if (border)
    GetRectFromString(border, image.border);
  image.orientation = 0;
  const char *flipX = pNode->Attribute("flipx");
  if (flipX && strcmpi(flipX, "true") == 0) image.orientation = 1;
  const char *flipY = pNode->Attribute("flipy");
  if (flipY && strcmpi(flipY, "true") == 0) image.orientation = 3 - image.orientation;  // either 3 or 2
  image.diffuse = pNode->Attribute("diffuse");
  CStdString fallback = pNode->Attribute("fallback");
  CStdString file = pNode->FirstChild() ? pNode->FirstChild()->Value() : "";
  image.diffuse.Replace("/", "\\");
  file.Replace("/", "\\");
  fallback.Replace("/", "\\");
  image.file.SetLabel(file, fallback);
  return true;
}

void CGUIControlFactory::GetRectFromString(const CStdString &string, FRECT &rect)
{
  // format is rect="left,right,top,bottom"
  CStdStringArray strRect;
  StringUtils::SplitString(string, ",", strRect);
  if (strRect.size() == 1)
  {
    g_SkinInfo.ResolveConstant(strRect[0], rect.left);
    rect.top = rect.left;
    rect.right = rect.left;
    rect.bottom = rect.left;
  }
  else if (strRect.size() == 4)
  {
    g_SkinInfo.ResolveConstant(strRect[0], rect.left);
    g_SkinInfo.ResolveConstant(strRect[1], rect.top);
    g_SkinInfo.ResolveConstant(strRect[2], rect.right);
    g_SkinInfo.ResolveConstant(strRect[3], rect.bottom);
  }
}

bool CGUIControlFactory::GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild()) return false;

  CStdString strAlign = pNode->FirstChild()->Value();
  if (strAlign == "right") dwAlignment = XBFONT_RIGHT;
  else if (strAlign == "center") dwAlignment = XBFONT_CENTER_X;
  else if (strAlign == "justify") dwAlignment = XBFONT_JUSTIFIED;
  else dwAlignment = XBFONT_LEFT;
  return true;
}

bool CGUIControlFactory::GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild())
  {
    return false;
  }

  CStdString strAlign = pNode->FirstChild()->Value();

  dwAlignment = 0;
  if (strAlign == "center")
  {
    dwAlignment = XBFONT_CENTER_Y;
  }

  return true;
}

bool CGUIControlFactory::GetConditionalVisibility(const TiXmlNode* control, int &condition, bool &allowHiddenFocus)
{
  const TiXmlElement* node = control->FirstChildElement("visible");
  if (!node) return false;
  allowHiddenFocus = false;
  vector<CStdString> conditions;
  while (node)
  {
    const char *hidden = node->Attribute("allowhiddenfocus");
    if (hidden && strcmpi(hidden, "true") == 0)
      allowHiddenFocus = true;
    // add to our condition string
    if (!node->NoChildren())
      conditions.push_back(node->FirstChild()->Value());
    node = node->NextSiblingElement("visible");
  }
  if (!conditions.size())
    return false;
  if (conditions.size() == 1)
    condition = g_infoManager.TranslateString(conditions[0]);
  else
  { // multiple conditions should be anded together
    CStdString conditionString = "[";
    for (unsigned int i = 0; i < conditions.size() - 1; i++)
      conditionString += conditions[i] + "] + [";
    conditionString += conditions[conditions.size() - 1] + "]";
    condition = g_infoManager.TranslateString(conditionString);
  }
  return (condition != 0);
}

bool CGUIControlFactory::GetCondition(const TiXmlNode *control, const char *tag, int &condition)
{
  CStdString condString;
  if (XMLUtils::GetString(control, tag, condString))
  {
    condition = g_infoManager.TranslateString(condString);
    return true;
  }
  return false;
}

bool CGUIControlFactory::GetConditionalVisibility(const TiXmlNode *control, int &condition)
{
  bool allowHiddenFocus;
  return GetConditionalVisibility(control, condition, allowHiddenFocus);
}

bool CGUIControlFactory::GetAnimations(const TiXmlNode *control, const FRECT &rect, vector<CAnimation> &animations)
{
  const TiXmlElement* node = control->FirstChildElement("animation");
  bool ret = false;
  if (node)
    animations.clear();
  while (node)
  {
    ret = true;
    if (node->FirstChild())
    {
      CAnimation anim;
      anim.Create(node, rect);
      animations.push_back(anim);
      if (strcmpi(node->FirstChild()->Value(), "VisibleChange") == 0)
      { // add the hidden one as well
        TiXmlElement hidden(*node);
        hidden.FirstChild()->SetValue("hidden");
        const char *start = hidden.Attribute("start");
        const char *end = hidden.Attribute("end");
        if (start && end)
        {
          CStdString temp = end;
          hidden.SetAttribute("end", start);
          hidden.SetAttribute("start", temp.c_str());
        }
        else if (start)
          hidden.SetAttribute("end", start);
        else if (end)
          hidden.SetAttribute("start", end);
        CAnimation anim2;
        anim2.Create(&hidden, rect);
        animations.push_back(anim2);
      }
    }
    node = node->NextSiblingElement("animation");
  }
  return ret;
}

bool CGUIControlFactory::GetHitRect(const TiXmlNode *control, CRect &rect)
{
  const TiXmlElement* node = control->FirstChildElement("hitrect");
  if (node)
  {
    if (node->Attribute("x")) g_SkinInfo.ResolveConstant(node->Attribute("x"), rect.x1);
    if (node->Attribute("y")) g_SkinInfo.ResolveConstant(node->Attribute("y"), rect.y1);
    if (node->Attribute("w")) 
    {
      g_SkinInfo.ResolveConstant(node->Attribute("w"), rect.x2);
      rect.x2 += rect.x1;
    }
    if (node->Attribute("h"))
    {
      g_SkinInfo.ResolveConstant(node->Attribute("h"), rect.y2);
      rect.y2 += rect.y1;
    }
    return true;
  }
  return false;
}

bool CGUIControlFactory::GetColor(const TiXmlNode *control, const char *strTag, DWORD &value)
{
  const TiXmlElement* node = control->FirstChildElement(strTag);
  if (node && node->FirstChild())
  {
    value = g_colorManager.GetColor(node->FirstChild()->Value());
    return true;
  }
  return false;
}

bool CGUIControlFactory::GetInfoColor(const TiXmlNode *control, const char *strTag, CGUIInfoColor &value)
{
  const TiXmlElement* node = control->FirstChildElement(strTag);
  if (node && node->FirstChild())
  {
    value.Parse(node->FirstChild()->Value());
    return true;
  }
  return false;
}

bool CGUIControlFactory::GetNavigation(const TiXmlElement *node, const char *tag, DWORD &direction, vector<CStdString> &actions)
{
  if (!GetMultipleString(node, tag, actions))
    return false; // no tag specified
  if (actions.size() == 1 && StringUtils::IsNaturalNumber(actions[0]))
  { // single numeric tag specified
    direction = atol(actions[0].c_str());
    actions.clear();
  }
  else
    direction = 0;
  return true;
}

void CGUIControlFactory::GetInfoLabel(const TiXmlNode *pControlNode, const CStdString &labelTag, CGUIInfoLabel &infoLabel)
{
  vector<CGUIInfoLabel> labels;
  GetInfoLabels(pControlNode, labelTag, labels);
  if (labels.size())
    infoLabel = labels[0];
}

void CGUIControlFactory::GetInfoLabels(const TiXmlNode *pControlNode, const CStdString &labelTag, vector<CGUIInfoLabel> &infoLabels)
{
  // we can have the following infolabels:
  // 1.  <number>1234</number> -> direct number
  // 2.  <label>number</label> -> lookup in localizestrings
  // 3.  <label fallback="blah">$LOCALIZE(blah) $INFO(blah)</label> -> infolabel with given fallback
  // 4.  <info>ListItem.Album</info> (uses <label> as fallback)
  int labelNumber = 0;
  if (XMLUtils::GetInt(pControlNode, "number", labelNumber))
  {
    CStdString label;
    label.Format("%i", labelNumber);
    infoLabels.push_back(CGUIInfoLabel(label));
    return; // done
  }
  const TiXmlElement *labelNode = pControlNode->FirstChildElement(labelTag);
  while (labelNode)
  {
    if (labelNode->FirstChild())
    {
      CStdString label = labelNode->FirstChild()->Value();
      CStdString fallback = labelNode->Attribute("fallback");
      if (label.size() && label[0] != '-')
      {
        if (StringUtils::IsNaturalNumber(label))
          label = g_localizeStrings.Get(atoi(label));
        else // we assume the skin xml's aren't encoded as UTF-8
          g_charsetConverter.stringCharsetToUtf8(label);
        if (StringUtils::IsNaturalNumber(fallback))
          fallback = g_localizeStrings.Get(atoi(fallback));
        else
          g_charsetConverter.stringCharsetToUtf8(fallback);
        infoLabels.push_back(CGUIInfoLabel(label, fallback));
      }
    }
    labelNode = labelNode->NextSiblingElement(labelTag);
  }
  const TiXmlNode *infoNode = pControlNode->FirstChild("info");
  if (infoNode)
  { // <info> nodes override <label>'s (backward compatibility)
    CStdString fallback;
    if (infoLabels.size())
      fallback = infoLabels[0].GetLabel(0);
    infoLabels.clear();
    while (infoNode)
    {
      if (infoNode->FirstChild())
      {
        CStdString info;
        info.Format("$INFO[%s]", infoNode->FirstChild()->Value());
        infoLabels.push_back(CGUIInfoLabel(info, fallback));
      }
      infoNode = infoNode->NextSibling("info");
    }
  }
}

// Convert a string to a GUI label, by translating/parsing the label for localisable strings
CStdString CGUIControlFactory::FilterLabel(const CStdString &label)
{
  CStdString viewLabel = label;
  if (StringUtils::IsNaturalNumber(viewLabel))
    viewLabel = g_localizeStrings.Get(atoi(label));
  else
  { // TODO: UTF-8: What if the xml is encoded as UTF-8 already?
    g_charsetConverter.stringCharsetToUtf8(viewLabel);
  }
  // translate the label
  CGUIInfoLabel info(viewLabel, "");
  return info.GetLabel(0);
}

bool CGUIControlFactory::GetString(const TiXmlNode* pRootNode, const char *strTag, CStdString &text)
{
  if (!XMLUtils::GetString(pRootNode, strTag, text))
    return false;
  if (text == "-")
    text.Empty();
  if (StringUtils::IsNaturalNumber(text))
    text = g_localizeStrings.Get(atoi(text.c_str()));
  else // TODO: UTF-8: What if the xml is encoded as UTF-8 already?
    g_charsetConverter.stringCharsetToUtf8(text);
  return true;
}

CStdString CGUIControlFactory::GetType(const TiXmlElement *pControlNode)
{
  CStdString type;
  const char *szType = pControlNode->Attribute("type");
  if (szType)
    type = szType;
  else  // backward compatibility - not desired
    XMLUtils::GetString(pControlNode, "type", type);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  // check if we are a <controlgroup>
  if (strcmpi(pControlNode->Value(), "controlgroup") == 0)
    type = "group";
#endif
  return type;
}

CGUIControl* CGUIControlFactory::Create(DWORD dwParentId, const FRECT &rect, TiXmlElement* pControlNode)
{
  // resolve any <include> tag's in this control
  g_SkinInfo.ResolveIncludes(pControlNode);

  // get the control type
  CStdString strType = GetType(pControlNode);

  // resolve again with strType set so that <default> tags are added
  g_SkinInfo.ResolveIncludes(pControlNode, strType);

  DWORD id = 0;
  float posX = 0, posY = 0;
  float width = 0, height = 0;

  DWORD left = 0, right = 0, up = 0, down = 0;
  vector<CStdString> leftActions, rightActions, upActions, downActions;

  DWORD pageControl = 0;
  CGUIInfoColor colorDiffuse(0xFFFFFFFF);
  DWORD defaultControl = 0;
  CStdString strTmp;
  int singleInfo = 0;
  CStdString strLabel;
  int iUrlSet=0;
  int iToggleSelect;

  float spinWidth = 16;
  float spinHeight = 16;
  float spinPosX, spinPosY;
  float checkWidth, checkHeight;
  CStdString strSubType;
  int iType = SPIN_CONTROL_TYPE_TEXT;
  int iMin = 0;
  int iMax = 100;
  int iInterval = 1;
  float fMin = 0.0f;
  float fMax = 1.0f;
  float fInterval = 0.1f;
  bool bReverse = true;
  bool bReveal = false;
  CImage textureBackground, textureLeft, textureRight, textureMid, textureOverlay;
  float rMin = 0.0f;
  float rMax = 100.0f;
  CImage textureNib, textureNibFocus, textureBar, textureBarFocus;
  CImage textureLeftFocus, textureRightFocus;
  CImage textureUp, textureDown;
  CImage textureUpFocus, textureDownFocus;
  CImage texture;
  CImage borderTexture;
  CImage textureCheckMark, textureCheckMarkNF;
  CImage textureFocus, textureNoFocus;
  CImage textureAltFocus, textureAltNoFocus;
  CImage textureRadioFocus, textureRadioNoFocus;
  CImage imageNoFocus, imageFocus;
  DWORD dwColorKey = 0;
  CStdString strSuffix = "";
  FRECT borderSize = { 0, 0, 0, 0};

  float controlOffsetX = 0;
  float controlOffsetY = 0;

  float itemWidth = 16, itemHeight = 16;
  float sliderWidth = 150, sliderHeight = 16;
  float textureWidthBig = 128;
  float textureHeightBig = 128;
  float textureHeight = 30;
  float textureWidth = 80;
  float itemWidthBig = 150;
  float itemHeightBig = 150;
  DWORD dwDisposition = 0;

  float spaceBetweenItems = 2;
  bool bHasPath = false;
  vector<CStdString> clickActions;
  vector<CStdString> altclickActions;
  vector<CStdString> focusActions;
  CStdString strTitle = "";
  CStdString strRSSTags = "";

  DWORD dwThumbAlign = 0;

  float thumbXPos = 4;
  float thumbYPos = 10;
  float thumbWidth = 64;
  float thumbHeight = 64;

  float thumbXPosBig = 14;
  float thumbYPosBig = 14;
  float thumbWidthBig = 100;
  float thumbHeightBig = 100;
  DWORD dwBuddyControlID = 0;
  int iNumSlots = 7;
  float buttonGap = 5;
  int iDefaultSlot = 2;
  int iMovementRange = 2;
  bool bHorizontal = false;
  int iAlpha = 0;
  bool bWrapAround = true;
  bool bSmoothScrolling = true;
  CGUIImage::CAspectRatio aspect;
  if (strType == "thumbnailpanel")  // default for thumbpanel is keep
    aspect.ratio = CGUIImage::CAspectRatio::AR_KEEP;

  int iVisibleCondition = 0;
  bool allowHiddenFocus = false;
  int enableCondition = 0;

  vector<CAnimation> animations;

  bool bScrollLabel = false;
  bool bPulse = true;
  DWORD timePerImage = 0;
  DWORD fadeTime = 0;
  DWORD timeToPauseAtEnd = 0;
  bool randomized = false;
  bool loop = true;
  bool wrapMultiLine = false;
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  bool thumbPanelHideLabels = false;
#endif
  ORIENTATION orientation = VERTICAL;
  bool showOnePage = true;
  bool scrollOut = true;

  CLabelInfo labelInfo;
  CLabelInfo labelInfo2;
  CLabelInfo spinInfo;

  CGUIInfoColor textColor3;

  float radioWidth = 0;
  float radioHeight = 0;
  float radioPosX = 0;
  float radioPosY = 0;

  CStdString altLabel;
  CStdString strLabel2;

  int focusPosition = 0;
  int scrollTime = 200;
  bool useControlCoords = false;

  CRect hitRect;
  CPoint camera;
  bool   hasCamera = false;

  /////////////////////////////////////////////////////////////////////////////
  // Read control properties from XML
  //

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  // check if we are a <controlgroup>
  if (strcmpi(pControlNode->Value(), "controlgroup") == 0)
  {
    if (pControlNode->Attribute("id", (int*) &id))
      id += 9000;       // offset at 9000 for old controlgroups
                        // NOTE: An old control group with no id means that it can't be focused
                        //       Which isn't too good :(
                        //       We check for this in OnWindowLoaded()
  }
  else
#endif
  if (!pControlNode->Attribute("id", (int*) &id))
    XMLUtils::GetInt(pControlNode, "id", (int&) id);       // backward compatibility - not desired
  // TODO: Perhaps we should check here whether id is valid for focusable controls
  // such as buttons etc.  For labels/fadelabels/images it does not matter

  GetFloat(pControlNode, "posx", posX);
  GetFloat(pControlNode, "posy", posY);
  // Convert these from relative coords
  CStdString pos;
  XMLUtils::GetString(pControlNode, "posx", pos);
  if (pos.Right(1) == "r")
    posX = (rect.right - rect.left) - posX;
  XMLUtils::GetString(pControlNode, "posy", pos);
  if (pos.Right(1) == "r")
    posY = (rect.bottom - rect.top) - posY;

  GetFloat(pControlNode, "width", width);
  GetFloat(pControlNode, "height", height);

  // adjust width and height accordingly for groups.  Groups should
  // take the width/height of the parent (adjusted for positioning)
  // if none is defined.
  if (strType == "group" || strType == "grouplist")
  {
    if (!width)
      width = max(rect.right - posX, 0.0f);
    if (!height)
      height = max(rect.bottom - posY, 0.0f);
  }

  hitRect.SetRect(posX, posY, posX + width, posY + height);
  GetHitRect(pControlNode, hitRect);

  GetFloat(pControlNode, "controloffsetx", controlOffsetX);
  GetFloat(pControlNode, "controloffsety", controlOffsetY);

  if (!GetNavigation(pControlNode, "onup", up, upActions)) up = id - 1;
  if (!GetNavigation(pControlNode, "ondown", down, downActions)) down = id + 1;
  if (!GetNavigation(pControlNode, "onleft", left, leftActions)) left = id;
  if (!GetNavigation(pControlNode, "onright", right, rightActions)) right = id;

  XMLUtils::GetDWORD(pControlNode, "defaultcontrol", defaultControl);
  XMLUtils::GetDWORD(pControlNode, "pagecontrol", pageControl);

  GetInfoColor(pControlNode, "colordiffuse", colorDiffuse);

  GetConditionalVisibility(pControlNode, iVisibleCondition, allowHiddenFocus);
  GetCondition(pControlNode, "enable", enableCondition);

  // note: animrect here uses .right and .bottom as width and height respectively (nonstandard)
  FRECT animRect = { posX, posY, width, height };
  GetAnimations(pControlNode, animRect, animations);

  GetInfoColor(pControlNode, "textcolor", labelInfo.textColor);
  GetInfoColor(pControlNode, "focusedcolor", labelInfo.focusedColor);
  GetInfoColor(pControlNode, "disabledcolor", labelInfo.disabledColor);
  GetInfoColor(pControlNode, "shadowcolor", labelInfo.shadowColor);
  GetInfoColor(pControlNode, "selectedcolor", labelInfo.selectedColor);
  GetFloat(pControlNode, "textoffsetx", labelInfo.offsetX);
  GetFloat(pControlNode, "textoffsety", labelInfo.offsetY);
  GetFloat(pControlNode, "textxoff", labelInfo.offsetX);
  GetFloat(pControlNode, "textyoff", labelInfo.offsetY);
  GetFloat(pControlNode, "textxoff2", labelInfo2.offsetX);
  GetFloat(pControlNode, "textyoff2", labelInfo2.offsetY);
  int angle = 0;  // use the negative angle to compensate for our vertically flipped cartesian plane
  if (XMLUtils::GetInt(pControlNode, "angle", angle)) labelInfo.angle = (float)-angle;
  CStdString strFont;
  if (XMLUtils::GetString(pControlNode, "font", strFont))
    labelInfo.font = g_fontManager.GetFont(strFont);
  GetAlignment(pControlNode, "align", labelInfo.align);
  DWORD alignY = 0;
  if (GetAlignmentY(pControlNode, "aligny", alignY))
    labelInfo.align |= alignY;
  if (GetFloat(pControlNode, "textwidth", labelInfo.width))
    labelInfo.align |= XBFONT_TRUNCATED;
  labelInfo2.selectedColor = labelInfo.selectedColor;
  GetInfoColor(pControlNode, "selectedcolor2", labelInfo2.selectedColor);
  GetInfoColor(pControlNode, "textcolor2", labelInfo2.textColor);
  GetInfoColor(pControlNode, "focusedcolor2", labelInfo2.focusedColor);
  labelInfo2.font = labelInfo.font;
  if (XMLUtils::GetString(pControlNode, "font2", strFont))
    labelInfo2.font = g_fontManager.GetFont(strFont);

  GetMultipleString(pControlNode, "onclick", clickActions);
  GetMultipleString(pControlNode, "onfocus", focusActions);
  GetMultipleString(pControlNode, "altclick", altclickActions);

  CStdString infoString;
  if (XMLUtils::GetString(pControlNode, "info", infoString))
    singleInfo = g_infoManager.TranslateString(infoString);

  GetTexture(pControlNode, "texturefocus", textureFocus);
  GetTexture(pControlNode, "texturenofocus", textureNoFocus);
  GetTexture(pControlNode, "alttexturefocus", textureAltFocus);
  GetTexture(pControlNode, "alttexturenofocus", textureAltNoFocus);
  CStdString strToggleSelect;
  XMLUtils::GetString(pControlNode, "usealttexture", strToggleSelect);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 2.1 && strToggleSelect.IsEmpty() && strType == "togglebutton")
  { // swap them over
    CImage temp = textureFocus;
    textureFocus = textureAltFocus;
    textureAltFocus = temp;
    temp = textureNoFocus;
    textureNoFocus = textureAltNoFocus;
    textureAltNoFocus = temp;
  }
#endif
  XMLUtils::GetString(pControlNode, "selected", strToggleSelect);
  iToggleSelect = g_infoManager.TranslateString(strToggleSelect);

  XMLUtils::GetBoolean(pControlNode, "haspath", bHasPath);

  GetTexture(pControlNode, "textureup", textureUp);
  GetTexture(pControlNode, "texturedown", textureDown);
  GetTexture(pControlNode, "textureupfocus", textureUpFocus);
  GetTexture(pControlNode, "texturedownfocus", textureDownFocus);

  GetTexture(pControlNode, "textureleft", textureLeft);
  GetTexture(pControlNode, "textureright", textureRight);
  GetTexture(pControlNode, "textureleftfocus", textureLeftFocus);
  GetTexture(pControlNode, "texturerightfocus", textureRightFocus);

  GetInfoColor(pControlNode, "spincolor", spinInfo.textColor);
  if (XMLUtils::GetString(pControlNode, "spinfont", strFont))
    spinInfo.font = g_fontManager.GetFont(strFont);
  if (!spinInfo.font) spinInfo.font = labelInfo.font;

  GetFloat(pControlNode, "spinwidth", spinWidth);
  GetFloat(pControlNode, "spinheight", spinHeight);
  GetFloat(pControlNode, "spinposx", spinPosX);
  GetFloat(pControlNode, "spinposy", spinPosY);

  GetFloat(pControlNode, "markwidth", checkWidth);
  GetFloat(pControlNode, "markheight", checkHeight);
  GetFloat(pControlNode, "sliderwidth", sliderWidth);
  GetFloat(pControlNode, "sliderheight", sliderHeight);
  GetTexture(pControlNode, "texturecheckmark", textureCheckMark);
  GetTexture(pControlNode, "texturecheckmarknofocus", textureCheckMarkNF);
  GetTexture(pControlNode, "textureradiofocus", textureRadioFocus);
  GetTexture(pControlNode, "textureradionofocus", textureRadioNoFocus);

  GetTexture(pControlNode, "texturesliderbackground", textureBackground);
  GetTexture(pControlNode, "texturesliderbar", textureBar);
  GetTexture(pControlNode, "texturesliderbarfocus", textureBarFocus);
  GetTexture(pControlNode, "textureslidernib", textureNib);
  GetTexture(pControlNode, "textureslidernibfocus", textureNibFocus);
  XMLUtils::GetDWORD(pControlNode, "disposition", dwDisposition);

  XMLUtils::GetString(pControlNode, "title", strTitle);
  XMLUtils::GetString(pControlNode, "tagset", strRSSTags);
  GetInfoColor(pControlNode, "headlinecolor", labelInfo2.textColor);
  GetInfoColor(pControlNode, "titlecolor", textColor3);

  if (XMLUtils::GetString(pControlNode, "subtype", strSubType))
  {
    strSubType.ToLower();

    if ( strSubType == "int")
      iType = SPIN_CONTROL_TYPE_INT;
    else if ( strSubType == "page")
      iType = SPIN_CONTROL_TYPE_PAGE;
    else if ( strSubType == "float")
      iType = SPIN_CONTROL_TYPE_FLOAT;
    else
      iType = SPIN_CONTROL_TYPE_TEXT;
  }

  if (!GetIntRange(pControlNode, "range", iMin, iMax, iInterval))
  {
    GetFloatRange(pControlNode, "range", fMin, fMax, fInterval);
  }

  XMLUtils::GetBoolean(pControlNode, "reverse", bReverse);
  XMLUtils::GetBoolean(pControlNode, "reveal", bReveal);

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CStdString hideLabels;
  if (XMLUtils::GetString(pControlNode, "hidelabels", hideLabels))
  {
    if (hideLabels.Equals("all"))
      thumbPanelHideLabels = true;
    else
      thumbPanelHideLabels = false;
  }
#endif

  GetTexture(pControlNode, "texturebg", textureBackground);
  GetTexture(pControlNode, "lefttexture", textureLeft);
  GetTexture(pControlNode, "midtexture", textureMid);
  GetTexture(pControlNode, "righttexture", textureRight);
  GetTexture(pControlNode, "overlaytexture", textureOverlay);

  // the <texture> tag can be overridden by the <info> tag
  GetTexture(pControlNode, "texture", texture);
  GetInfoLabel(pControlNode, "texture", texture.file);

  GetTexture(pControlNode, "bordertexture", borderTexture);
  GetFloat(pControlNode, "rangemin", rMin);
  GetFloat(pControlNode, "rangemax", rMax);
  GetColor(pControlNode, "colorkey", dwColorKey);

  XMLUtils::GetString(pControlNode, "suffix", strSuffix);

  GetFloat(pControlNode, "itemwidth", itemWidth);
  GetFloat(pControlNode, "itemheight", itemHeight);
  GetFloat(pControlNode, "spacebetweenitems", spaceBetweenItems);

  GetTexture(pControlNode, "imagefolder", imageNoFocus);
  GetTexture(pControlNode, "imagefolderfocus", imageFocus);
  GetFloat(pControlNode, "texturewidth", textureWidth);
  GetFloat(pControlNode, "textureheight", textureHeight);

  GetFloat(pControlNode, "thumbwidth", thumbWidth);
  GetFloat(pControlNode, "thumbheight", thumbHeight);
  GetFloat(pControlNode, "thumbposx", thumbXPos);
  GetFloat(pControlNode, "thumbposy", thumbYPos);

  GetAlignment(pControlNode, "thumbalign", dwThumbAlign);

  GetFloat(pControlNode, "thumbwidthbig", thumbWidthBig);
  GetFloat(pControlNode, "thumbheightbig", thumbHeightBig);
  GetFloat(pControlNode, "thumbposxbig", thumbXPosBig);
  GetFloat(pControlNode, "thumbposybig", thumbYPosBig);

  GetFloat(pControlNode, "texturewidthbig", textureWidthBig);
  GetFloat(pControlNode, "textureheightbig", textureHeightBig);
  GetFloat(pControlNode, "itemwidthbig", itemWidthBig);
  GetFloat(pControlNode, "itemheightbig", itemHeightBig);

  // fade label can have a whole bunch, but most just have one
  vector<CGUIInfoLabel> infoLabels;
  GetInfoLabels(pControlNode, "label", infoLabels);
  
  GetString(pControlNode, "label", strLabel);
  GetString(pControlNode, "altlabel", altLabel);
  GetString(pControlNode, "label2", strLabel2);

  XMLUtils::GetBoolean(pControlNode, "wrapmultiline", wrapMultiLine);
  XMLUtils::GetInt(pControlNode,"urlset",iUrlSet);

  // stuff for button scroller
  if ( XMLUtils::GetString(pControlNode, "orientation", strTmp) )
  {
    if (strTmp.ToLower() == "horizontal")
    {
      bHorizontal = true;
      orientation = HORIZONTAL;
    }
  }
  GetFloat(pControlNode, "buttongap", buttonGap);
  GetFloat(pControlNode, "itemgap", buttonGap);
  XMLUtils::GetInt(pControlNode, "numbuttons", iNumSlots);
  XMLUtils::GetInt(pControlNode, "movement", iMovementRange);
  XMLUtils::GetInt(pControlNode, "defaultbutton", iDefaultSlot);
  XMLUtils::GetInt(pControlNode, "alpha", iAlpha);
  XMLUtils::GetBoolean(pControlNode, "wraparound", bWrapAround);
  XMLUtils::GetBoolean(pControlNode, "smoothscrolling", bSmoothScrolling);
  GetAspectRatio(pControlNode, "aspectratio", aspect);
  XMLUtils::GetBoolean(pControlNode, "scroll", bScrollLabel);
  XMLUtils::GetBoolean(pControlNode,"pulseonselect", bPulse);

  CGUIInfoLabel texturePath;
  GetInfoLabel(pControlNode,"imagepath", texturePath);
  XMLUtils::GetDWORD(pControlNode,"timeperimage", timePerImage);
  XMLUtils::GetDWORD(pControlNode,"fadetime", fadeTime);
  XMLUtils::GetDWORD(pControlNode,"pauseatend", timeToPauseAtEnd);
  XMLUtils::GetBoolean(pControlNode, "randomize", randomized);
  XMLUtils::GetBoolean(pControlNode, "loop", loop);
  XMLUtils::GetBoolean(pControlNode, "scrollout", scrollOut);

  GetFloat(pControlNode, "radiowidth", radioWidth);
  GetFloat(pControlNode, "radioheight", radioHeight);
  GetFloat(pControlNode, "radioposx", radioPosX);
  GetFloat(pControlNode, "radioposy", radioPosY);
  GetFloat(pControlNode, "spinposx", radioPosX);
  CStdString borderStr;
  if (XMLUtils::GetString(pControlNode, "bordersize", borderStr))
    GetRectFromString(borderStr, borderSize);

  XMLUtils::GetBoolean(pControlNode, "showonepage", showOnePage);
  XMLUtils::GetInt(pControlNode, "focusposition", focusPosition);
  XMLUtils::GetInt(pControlNode, "scrolltime", scrollTime);

  XMLUtils::GetBoolean(pControlNode, "usecontrolcoords", useControlCoords);

  // view type
  VIEW_TYPE viewType = VIEW_TYPE_NONE;
  CStdString viewLabel;
  if (strType == "panel")
  {
    viewType = VIEW_TYPE_ICON;
    viewLabel = g_localizeStrings.Get(536);
  }
  else if (strType == "list")
  {
    viewType = VIEW_TYPE_LIST;
    viewLabel = g_localizeStrings.Get(535);
  }
  else if (strType == "wraplist")
  {
    viewType = VIEW_TYPE_WRAP;
    viewLabel = g_localizeStrings.Get(541);
  }
  TiXmlElement *itemElement = pControlNode->FirstChildElement("viewtype");
  if (itemElement && itemElement->FirstChild())
  {
    CStdString type = itemElement->FirstChild()->Value();
    if (type == "list")
      viewType = VIEW_TYPE_LIST;
    else if (type == "icon")
      viewType = VIEW_TYPE_ICON;
    else if (type == "biglist")
      viewType = VIEW_TYPE_BIG_LIST;
    else if (type == "bigicon")
      viewType = VIEW_TYPE_BIG_ICON;
    else if (type == "wide")
      viewType = VIEW_TYPE_WIDE;
    else if (type == "bigwide")
      viewType = VIEW_TYPE_BIG_WIDE;
    else if (type == "wrap")
      viewType = VIEW_TYPE_WRAP;
    else if (type == "bigwrap")
      viewType = VIEW_TYPE_BIG_WRAP;
    const char *label = itemElement->Attribute("label");
    if (label)
      viewLabel = FilterLabel(label);
  }

  TiXmlElement *cam = pControlNode->FirstChildElement("camera");
  if (cam)
  {
    hasCamera = true;
    g_SkinInfo.ResolveConstant(cam->Attribute("x"), camera.x);
    g_SkinInfo.ResolveConstant(cam->Attribute("y"), camera.y);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Instantiate a new control using the properties gathered above
  //

  CGUIControl *control = NULL;
  if (strType == "group")
  {
    control = new CGUIControlGroup(
      dwParentId, id, posX, posY, width, height);
    ((CGUIControlGroup *)control)->SetDefaultControl(defaultControl);
  }
  else if (strType == "grouplist")
  {
    control = new CGUIControlGroupList(
      dwParentId, id, posX, posY, width, height, buttonGap, pageControl, orientation, useControlCoords);
  }
  else if (strType == "label")
  {
    control = new CGUILabelControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, wrapMultiLine, bHasPath);
    if (infoLabels.size())
      ((CGUILabelControl *)control)->SetInfo(infoLabels[0]);
    ((CGUILabelControl *)control)->SetWidthControl(bScrollLabel);
  }
  else if (strType == "edit")
  {
    control = new CGUIEditControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, strLabel);
  }
  else if (strType == "videowindow")
  {
    control = new CGUIVideoControl(
      dwParentId, id, posX, posY, width, height);
  }
  else if (strType == "fadelabel")
  {
    control = new CGUIFadeLabelControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, scrollOut, timeToPauseAtEnd);

    ((CGUIFadeLabelControl *)control)->SetInfo(infoLabels);
  }
  else if (strType == "rss")
  {
    control = new CGUIRSSControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, textColor3, labelInfo2.textColor, strRSSTags);

    std::map<int, std::pair<std::vector<int>,std::vector<string> > >::iterator iter=g_settings.m_mapRssUrls.find(iUrlSet);
    if (iter != g_settings.m_mapRssUrls.end())
    {
      ((CGUIRSSControl *)control)->SetUrls(iter->second.second);
      ((CGUIRSSControl *)control)->SetIntervals(iter->second.first);
    }
    else
      CLog::Log(LOGERROR,"invalid rss url set referenced in skin");
  }
  else if (strType == "console")
  {
    control = new CGUIConsoleControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, labelInfo.textColor, labelInfo2.textColor, textColor3, labelInfo.selectedColor);
  }
  else if (strType == "button")
  {
    control = new CGUIButtonControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus,
      labelInfo);

    ((CGUIButtonControl *)control)->SetLabel(strLabel);
    ((CGUIButtonControl *)control)->SetLabel2(strLabel2);
    ((CGUIButtonControl *)control)->SetClickActions(clickActions);
    ((CGUIButtonControl *)control)->SetFocusActions(focusActions);
  }
  else if (strType == "togglebutton")
  {
    control = new CGUIToggleButtonControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus,
      textureAltFocus, textureAltNoFocus, labelInfo);

    ((CGUIToggleButtonControl *)control)->SetLabel(strLabel);
    ((CGUIToggleButtonControl *)control)->SetAltLabel(altLabel);
    ((CGUIToggleButtonControl *)control)->SetClickActions(clickActions);
    ((CGUIToggleButtonControl *)control)->SetAltClickActions(altclickActions);
    ((CGUIToggleButtonControl *)control)->SetFocusActions(focusActions);
    ((CGUIToggleButtonControl *)control)->SetToggleSelect(iToggleSelect);
  }
  else if (strType == "checkmark")
  {
    control = new CGUICheckMarkControl(
      dwParentId, id, posX, posY, width, height,
      textureCheckMark, textureCheckMarkNF,
      checkWidth, checkHeight, labelInfo);

    ((CGUICheckMarkControl *)control)->SetLabel(strLabel);
  }
  else if (strType == "radiobutton")
  {
    control = new CGUIRadioButtonControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus,
      labelInfo,
      textureRadioFocus, textureRadioNoFocus);

    ((CGUIRadioButtonControl *)control)->SetLabel(strLabel);
    ((CGUIRadioButtonControl *)control)->SetRadioDimensions(radioPosX, radioPosY, radioWidth, radioHeight);
    ((CGUIRadioButtonControl *)control)->SetToggleSelect(iToggleSelect);
    ((CGUIRadioButtonControl *)control)->SetClickActions(clickActions);
    ((CGUIRadioButtonControl *)control)->SetFocusActions(focusActions);
  }
  else if (strType == "multiselect")
  {
    CGUIInfoLabel label;
    if (infoLabels.size())
      label = infoLabels[0];
    control = new CGUIMultiSelectTextControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus, labelInfo, label);
  }
  else if (strType == "spincontrol")
  {
    control = new CGUISpinControl(
      dwParentId, id, posX, posY, width, height,
      textureUp, textureDown, textureUpFocus, textureDownFocus,
      labelInfo, iType);

    ((CGUISpinControl *)control)->SetReverse(bReverse);

    if (iType == SPIN_CONTROL_TYPE_INT)
    {
      ((CGUISpinControl *)control)->SetRange(iMin, iMax);
    }
    else if (iType == SPIN_CONTROL_TYPE_PAGE)
    {
      ((CGUISpinControl *)control)->SetRange(iMin, iMax);
      ((CGUISpinControl *)control)->SetShowRange(true);
      ((CGUISpinControl *)control)->SetReverse(false);
      ((CGUISpinControl *)control)->SetShowOnePage(showOnePage);
    }
    else if (iType == SPIN_CONTROL_TYPE_FLOAT)
    {
      ((CGUISpinControl *)control)->SetFloatRange(fMin, fMax);
      ((CGUISpinControl *)control)->SetFloatInterval(fInterval);
    }
  }
  else if (strType == "slider")
  {
    control = new CGUISliderControl(
      dwParentId, id, posX, posY, width, height,
      textureBar, textureNib, textureNibFocus, SPIN_CONTROL_TYPE_TEXT);

    ((CGUISliderControl *)control)->SetInfo(singleInfo);
    ((CGUISliderControl *)control)->SetControlOffsetX(controlOffsetX);
    ((CGUISliderControl *)control)->SetControlOffsetY(controlOffsetY);
  }
  else if (strType == "sliderex")
  {
    labelInfo.align |= XBFONT_CENTER_Y;    // always center text vertically
    control = new CGUISettingsSliderControl(
      dwParentId, id, posX, posY, width, height, sliderWidth, sliderHeight, textureFocus, textureNoFocus,
      textureBar, textureNib, textureNibFocus, labelInfo, SPIN_CONTROL_TYPE_TEXT);

    ((CGUISettingsSliderControl *)control)->SetText(strLabel);
    ((CGUISettingsSliderControl *)control)->SetInfo(singleInfo);
  }
  else if (strType == "scrollbar")
  {
    control = new CGUIScrollBar(
      dwParentId, id, posX, posY, width, height,
      textureBackground, textureBar, textureBarFocus, textureNib, textureNibFocus, orientation, showOnePage);
  }
  else if (strType == "progress")
  {
    control = new CGUIProgressControl(
      dwParentId, id, posX, posY, width, height,
      textureBackground, textureLeft, textureMid, textureRight, 
      textureOverlay, rMin, rMax, bReveal);
    ((CGUIProgressControl *)control)->SetInfo(singleInfo);
  }
  else if (strType == "image")
  {
    if (borderTexture.file.IsEmpty())
      control = new CGUIImage(
        dwParentId, id, posX, posY, width, height, texture, dwColorKey);
    else
      control = new CGUIBorderedImage(
        dwParentId, id, posX, posY, width, height, texture, borderTexture, borderSize, dwColorKey);
    ((CGUIImage *)control)->SetAspectRatio(aspect);
  }
  else if (strType == "largeimage")
  {
    control = new CGUILargeImage(
      dwParentId, id, posX, posY, width, height, texture);
    ((CGUILargeImage *)control)->SetAspectRatio(aspect);
  }
  else if (strType == "multiimage")
  {
    control = new CGUIMultiImage(
      dwParentId, id, posX, posY, width, height, texturePath, timePerImage, fadeTime, randomized, loop, timeToPauseAtEnd);
    ((CGUIMultiImage *)control)->SetAspectRatio(aspect.ratio);
  }
  else if (strType == "list")
  {
    control = new CGUIListContainer(dwParentId, id, posX, posY, width, height, orientation, scrollTime);
    ((CGUIListContainer *)control)->LoadLayout(pControlNode);
    ((CGUIListContainer *)control)->LoadContent(pControlNode);
    ((CGUIListContainer *)control)->SetType(viewType, viewLabel);
    ((CGUIListContainer *)control)->SetPageControl(pageControl);
  }
  else if (strType == "wraplist")
  {
    control = new CGUIWrappingListContainer(dwParentId, id, posX, posY, width, height, orientation, scrollTime, focusPosition);
    ((CGUIWrappingListContainer *)control)->LoadLayout(pControlNode);
    ((CGUIWrappingListContainer *)control)->LoadContent(pControlNode);
    ((CGUIWrappingListContainer *)control)->SetType(viewType, viewLabel);
    ((CGUIWrappingListContainer *)control)->SetPageControl(pageControl);
  }
  else if (strType == "fixedlist")
  {
    control = new CGUIFixedListContainer(dwParentId, id, posX, posY, width, height, orientation, scrollTime, focusPosition);
    ((CGUIFixedListContainer *)control)->LoadLayout(pControlNode);
    ((CGUIFixedListContainer *)control)->LoadContent(pControlNode);
    ((CGUIFixedListContainer *)control)->SetType(viewType, viewLabel);
    ((CGUIFixedListContainer *)control)->SetPageControl(pageControl);
  }
  else if (strType == "panel")
  {
    control = new CGUIPanelContainer(dwParentId, id, posX, posY, width, height, orientation, scrollTime);
    ((CGUIPanelContainer *)control)->LoadLayout(pControlNode);
    ((CGUIPanelContainer *)control)->LoadContent(pControlNode);
    ((CGUIPanelContainer *)control)->SetType(viewType, viewLabel);
    ((CGUIPanelContainer *)control)->SetPageControl(pageControl);
  }
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  else if (strType == "listcontrol")
  {
    // create the spin control
    CGUISpinControl *pSpin = new CGUISpinControl(dwParentId, id + 5000, posX + spinPosX, posY + spinPosY, spinWidth, spinHeight,
      textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_PAGE);
    // spincontrol should be visible when our list is
    CStdString spinVis;
    spinVis.Format("control.isvisible(%i)", id);
    pSpin->SetVisibleCondition(g_infoManager.TranslateString(spinVis), false);
    pSpin->SetAnimations(animations);
    pSpin->SetNavigation(id, down, id, right);
    pSpin->SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);

    labelInfo2.align |= XBFONT_RIGHT;
    if (labelInfo.align & XBFONT_CENTER_Y)
      labelInfo2.align |= XBFONT_CENTER_Y;
    CGUIListContainer* pControl = new CGUIListContainer(dwParentId, id, posX, posY, width, height - spinHeight - 5,
      labelInfo, labelInfo2, textureNoFocus, textureFocus, textureHeight, itemWidth, itemHeight, spaceBetweenItems, pSpin);

    if (id == 53) // big list
      pControl->SetType(VIEW_TYPE_BIG_LIST, g_localizeStrings.Get(537)); // Big List
    else
      pControl->SetType(VIEW_TYPE_LIST, g_localizeStrings.Get(535)); // List

    pControl->SetPageControl(id + 5000);
    pControl->SetNavigation(up, down, left, id + 5000);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    return pControl;
  }
#endif
  else if (strType == "listcontrolex")
  {
    control = new CGUIListControlEx(
      dwParentId, id, posX, posY, width, height,
      spinWidth, spinHeight,
      textureUp, textureDown,
      textureUpFocus, textureDownFocus,
      spinInfo, spinPosX, spinPosY,
      labelInfo, labelInfo2,
      textureNoFocus, textureFocus);

    ((CGUIListControlEx*)control)->SetScrollySuffix(strSuffix);
    ((CGUIListControlEx*)control)->SetImageDimensions(itemWidth, itemHeight);
    ((CGUIListControlEx*)control)->SetItemHeight(textureHeight);
    ((CGUIListControlEx*)control)->SetSpaceBetweenItems(spaceBetweenItems);
  }
  else if (strType == "textbox")
  {
    control = new CGUITextBox(
      dwParentId, id, posX, posY, width, height,
      spinWidth, spinHeight,
      textureUp, textureDown,
      textureUpFocus, textureDownFocus,
      spinInfo, spinPosX, spinPosY,
      labelInfo, scrollTime);

    ((CGUITextBox *)control)->SetPageControl(pageControl);
    if (infoLabels.size())
      ((CGUITextBox *)control)->SetInfo(infoLabels[0]);
    ((CGUITextBox *)control)->SetAutoScrolling(pControlNode);
  }
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  else if (strType == "thumbnailpanel")
  {
    // create the spin control
    CGUISpinControl *pSpin = NULL;
    if (!pageControl)
    {
      pSpin = new CGUISpinControl(dwParentId, id + 5000, posX + spinPosX, posY + spinPosY, spinWidth, spinHeight,
        textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_PAGE);
      // spincontrol should be visible when our list is
      CStdString spinVis;
      spinVis.Format("control.isvisible(%i) | control.isvisible(%i)", id, id + 2);
      pSpin->SetVisibleCondition(g_infoManager.TranslateString(spinVis), false);
      pSpin->SetAnimations(animations);
      pSpin->SetNavigation(id, down, id, right);
      pSpin->SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
    }
    labelInfo.align |= XBFONT_CENTER_X;

    // large panel
    CGUIPanelContainer* pPanel = new CGUIPanelContainer(
      dwParentId, id + 2, posX, posY, width, height,
      imageNoFocus, imageFocus,
      itemWidthBig, itemHeightBig,
      textureWidthBig, textureHeightBig, 
      thumbXPosBig, thumbYPosBig, thumbWidthBig, thumbHeightBig, dwThumbAlign, aspect,
      labelInfo, thumbPanelHideLabels, NULL, NULL);

    pPanel->SetType(VIEW_TYPE_BIG_ICON, g_localizeStrings.Get(538)); // Big Icons
    pPanel->SetPageControl(pageControl ? pageControl : id + 5000);
    pPanel->SetNavigation(up == id ? id + 2 : up, down == id ? id + 2 : down, left == id ? id + 2 : left, pageControl ? pageControl : id + 5000);

    pPanel->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pPanel->SetAnimations(animations);

    // small panel
    CGUIPanelContainer* pControl = new CGUIPanelContainer(
      dwParentId, id, posX, posY, width, height,
      imageNoFocus, imageFocus,
      itemWidth, itemHeight,
      textureWidth, textureHeight, 
      thumbXPos, thumbYPos, thumbWidth, thumbHeight, dwThumbAlign, aspect,
      labelInfo, thumbPanelHideLabels, pSpin, pPanel);

    pControl->SetType(VIEW_TYPE_ICON, g_localizeStrings.Get(536)); // Icons
    pControl->SetPageControl(pageControl ? pageControl : id + 5000);
    pControl->SetNavigation(up, down, left, pageControl ? pageControl : id + 5000);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    return pControl;
  }
#endif
  else if (strType == "selectbutton")
  {
    control = new CGUISelectButtonControl(
      dwParentId, id, posX, posY,
      width, height, textureFocus, textureNoFocus,
      labelInfo,
      textureBackground, textureLeft, textureLeftFocus, textureRight, textureRightFocus);

    ((CGUISelectButtonControl *)control)->SetLabel(strLabel);
  }
  else if (strType == "mover")
  {
    control = new CGUIMoverControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus);
  }
  else if (strType == "resize")
  {
    control = new CGUIResizeControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus);
  }
  else if (strType == "buttonscroller")
  {
    control = new CGUIButtonScroller(
      dwParentId, id, posX, posY, width, height, buttonGap, iNumSlots, iDefaultSlot,
      iMovementRange, bHorizontal, iAlpha, bWrapAround, bSmoothScrolling,
      textureFocus, textureNoFocus, labelInfo);
    ((CGUIButtonScroller *)control)->LoadButtons(pControlNode);
  }
  else if (strType == "spincontrolex")
  {
    control = new CGUISpinControlEx(
      dwParentId, id, posX, posY, width, height, spinWidth, spinHeight,
      labelInfo, textureFocus, textureNoFocus, textureUp, textureDown, textureUpFocus, textureDownFocus,
      labelInfo, iType);

    ((CGUISpinControlEx *)control)->SetSpinPosition(radioPosX);
    ((CGUISpinControlEx *)control)->SetText(strLabel);
    ((CGUISpinControlEx *)control)->SetReverse(bReverse);
  }
  else if (strType == "visualisation")
  {
    control = new CGUIVisualisationControl(dwParentId, id, posX, posY, width, height);
  }

  // things that apply to all controls
  if (control)
  {
    control->SetHitRect(hitRect);
    control->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    control->SetEnableCondition(enableCondition);
    control->SetAnimations(animations);
    control->SetColorDiffuse(colorDiffuse);
    control->SetNavigation(up, down, left, right);
    control->SetNavigationActions(upActions, downActions, leftActions, rightActions);
    control->SetPulseOnSelect(bPulse);
    if (hasCamera)
      control->SetCamera(camera);
  }
  return control;
}

void CGUIControlFactory::ScaleElement(TiXmlElement *element, RESOLUTION fileRes, RESOLUTION destRes)
{
  if (element->FirstChild())
  {
    const char *value = element->FirstChild()->Value();
    if (value)
    {
      float v = (float)atof(value);
      CStdString name = element->Value();
      if (name == "posx" ||
          name == "width" ||
          name == "gfxthumbwidth" ||
          name == "gfxthumbspacex" ||
          name == "controloffsetx" ||
          name == "textoffsetx" ||
          name == "textxoff" ||
          name == "textxoff2" ||
          name == "textwidth" ||
          name == "spinwidth" ||
          name == "spinposx" ||
          name == "markwidth" ||
          name == "sliderwidth" ||
          name == "itemwidth" ||
          name == "texturewidth" ||
          name == "thumbwidth" ||
          name == "thumbposx" ||
          name == "thumbwidthbig" ||
          name == "thumbposxbig" ||
          name == "texturewidthbig" ||
          name == "itemwidthbig" ||
          name == "radiowidth" ||
          name == "radioposx")
      {
        // scale
        v *= (float)g_settings.m_ResInfo[destRes].iWidth / g_settings.m_ResInfo[fileRes].iWidth;
        CStdString floatValue;
        floatValue.Format("%f", v);
        element->FirstChild()->SetValue(floatValue);
      }
      else if (name == "posy" ||
          name == "height" ||
          name == "textspacey" ||
          name == "gfxthumbheight" ||
          name == "gfxthumbspacey" ||
          name == "controloffsety" ||
          name == "textoffsety" ||
          name == "textyoff" ||
          name == "textyoff2" ||
          name == "spinheight" ||
          name == "spinposy" ||
          name == "markheight" ||
          name == "sliderheight" ||
          name == "spacebetweenitems" ||
          name == "textureheight" ||
          name == "thumbheight" ||
          name == "thumbposy" ||
          name == "thumbheightbig" ||
          name == "thumbposybig" ||
          name == "textureheightbig" ||
          name == "itemheightbig" ||
          name == "buttongap" ||  // should really depend on orientation
          name == "radioheight" ||
          name == "radioposy")
      {
        // scale
        v *= (float)g_settings.m_ResInfo[destRes].iHeight / g_settings.m_ResInfo[fileRes].iHeight;
        CStdString floatValue;
        floatValue.Format("%f", v);
        element->FirstChild()->SetValue(floatValue);
      }
    }
  }
}

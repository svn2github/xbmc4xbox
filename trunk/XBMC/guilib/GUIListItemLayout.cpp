#include "include.h"
#include "GUIListItemLayout.h"
#include "GUIListItem.h"
#include "GuiControlFactory.h"
#include "GUIFontManager.h"
#include "XMLUtils.h"
#include "SkinInfo.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/FileItem.h"

CGUIListItemLayout::CListBase::CListBase(float posX, float posY, float width, float height, int visibleCondition)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_visible = true;
  m_visibleCondition = visibleCondition;
}

CGUIListItemLayout::CListBase::~CListBase()
{
}

CGUIListItemLayout::CListLabel::CListLabel(float posX, float posY, float width, float height, int visibleCondition, const CLabelInfo &label, int info, const CStdString &content)
: CGUIListItemLayout::CListBase(posX, posY, width, height, visibleCondition)
{
  m_label = label;
  m_info = info;
  m_type = LIST_LABEL;
  g_infoManager.ParseLabel(content, m_multiInfo);
}

CGUIListItemLayout::CListLabel::~CListLabel()
{
}

CGUIListItemLayout::CListTexture::CListTexture(float posX, float posY, float width, float height, int visibleCondition, const CImage &image, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio, DWORD aspectAlign, D3DCOLOR colorDiffuse, const vector<CAnimation> &animations)
: CGUIListItemLayout::CListBase(posX, posY, width, height, visibleCondition),
  m_image(0, 0, posX, posY, width, height, image)
{
  m_type = LIST_TEXTURE;
  m_image.SetAspectRatio(aspectRatio, aspectAlign);
  m_image.SetAnimations(animations);
  m_image.SetColorDiffuse(colorDiffuse);
}

CGUIListItemLayout::CListTexture::~CListTexture()
{
  m_image.FreeResources();
}

CGUIListItemLayout::CListImage::CListImage(float posX, float posY, float width, float height, int visibleCondition, const CImage &image, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio, DWORD aspectAlign, D3DCOLOR colorDiffuse, const vector<CAnimation> &animations, int info)
: CGUIListItemLayout::CListTexture(posX, posY, width, height, visibleCondition, image, aspectRatio, aspectAlign, colorDiffuse, animations)
{
  m_info = info;
  m_type = LIST_IMAGE;
}

CGUIListItemLayout::CListImage::~CListImage()
{
}

CGUIListItemLayout::CGUIListItemLayout()
{
  m_width = 0;
  m_height = 0;
  m_focused = false;
  m_invalidated = true;
  m_isPlaying = false;
}

CGUIListItemLayout::CGUIListItemLayout(const CGUIListItemLayout &from)
{
  m_width = from.m_width;
  m_height = from.m_height;
  m_focused = from.m_focused;
  // copy across our controls
  for (ciControls it = from.m_controls.begin(); it != from.m_controls.end(); ++it)
  {
    CListBase *item = *it;
    if (item->m_type == CListBase::LIST_LABEL)
      m_controls.push_back(new CListLabel(*(CListLabel *)item));
    else if (item->m_type ==  CListBase::LIST_IMAGE)
      m_controls.push_back(new CListImage(*(CListImage *)item));
    else if (item->m_type ==  CListBase::LIST_TEXTURE)
      m_controls.push_back(new CListTexture(*(CListTexture *)item));
  }
  m_invalidated = true;
  m_isPlaying = false;
}

CGUIListItemLayout::~CGUIListItemLayout()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    delete *it;
}

float CGUIListItemLayout::Size(ORIENTATION orientation)
{
  return (orientation == HORIZONTAL) ? m_width : m_height;
}

void CGUIListItemLayout::Render(CGUIListItem *item, DWORD parentID, DWORD time)
{
  if (m_invalidated)
  {
    // could use a dynamic cast here if RTTI was enabled.  As it's not,
    // let's use a static cast with a virtual base function
    CFileItem *fileItem = item->IsFileItem() ? (CFileItem *)item : new CFileItem(*item);

    // check for boolean conditions
    m_isPlaying = g_infoManager.GetItemBool(fileItem, LISTITEM_ISPLAYING, parentID);
    for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
      UpdateItem(*it, fileItem, parentID);
    // now we have to check our overlapping label pairs
    for (unsigned int i = 0; i < m_controls.size(); i++)
    {
      if (m_controls[i]->m_type == CListBase::LIST_LABEL && m_controls[i]->m_visible)
      {
        CListLabel *label1 = (CListLabel *)m_controls[i];
        for (unsigned int j = i + 1; j < m_controls.size(); j++)
        {
          if (m_controls[j]->m_type == CListBase::LIST_LABEL && m_controls[j]->m_visible)
          { // ok, now check if they overlap
            CListLabel *label2 = (CListLabel *)m_controls[j];
            if ((label1->m_renderY <= label2->m_renderY + label2->m_renderH*0.5f && label2->m_renderY + label2->m_renderH*0.5f <= label1->m_renderY + label1->m_renderH) ||
                (label2->m_renderY <= label1->m_renderY + label1->m_renderH*0.5f && label1->m_renderY + label1->m_renderH*0.5f <= label2->m_renderY + label2->m_renderH))
            { // overlap vertically - check horizontal
              CListLabel *left = label1->m_renderX < label2->m_renderX ? label1 : label2;
              CListLabel *right = label1->m_renderX < label2->m_renderX ? label2 : label1;
              if ((left->m_label.align & 3) == 0 && right->m_label.align & XBFONT_RIGHT)
              { // left is aligned left, right is aligned right, and they overlap vertically
                if (left->m_renderX + left->m_renderW + 10 > right->m_renderX && left->m_renderX + left->m_renderW <= right->m_renderX + right->m_renderW)
                { // overlap, so chop accordingly
                  float chopPoint = (left->m_posX + left->m_width + right->m_posX - right->m_width) * 0.5f;
// [1       [2...[2  1].|..........1]         2]
// [1       [2.....[2   |      1]..1]         2]
// [1       [2..........|.[2   1]..1]         2]
                  if (right->m_renderX > chopPoint)
                    chopPoint = right->m_renderX - 5;
                  else if (left->m_renderX + left->m_renderW < chopPoint)
                    chopPoint = left->m_renderX + left->m_renderW + 5;
                  left->m_renderW = chopPoint - 5 - left->m_renderX;
                  right->m_renderW -= (chopPoint + 5 - right->m_renderX);
                  right->m_renderX = chopPoint + 5;
                }
              }
            }
          }
        }
      }
    }
    m_invalidated = false;
    // delete our temporary fileitem
    if (!item->IsFileItem())
      delete fileItem;
  }

  // and render
  for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
  {
    CListBase *layoutItem = *it;
    if (layoutItem->m_visible)
    {
      if (layoutItem->m_type == CListBase::LIST_LABEL)
      {
        RenderLabel((CListLabel *)layoutItem, item->IsSelected() || m_isPlaying, m_focused);
      }
      else
      {
        ((CListTexture *)layoutItem)->m_image.UpdateVisibility();
        ((CListTexture *)layoutItem)->m_image.DoRender(time);
      }
    }
  }
}

void CGUIListItemLayout::UpdateItem(CGUIListItemLayout::CListBase *control, CFileItem *item, DWORD parentID)
{
  // check boolean conditions
  if (control->m_visibleCondition)
    control->m_visible = g_infoManager.GetItemBool(item, control->m_visibleCondition, parentID);
  if (control->m_type == CListBase::LIST_IMAGE && item)
  {
    CListImage *image = (CListImage *)control;
    image->m_image.SetFileName(g_infoManager.GetItemImage(item, image->m_info));
  }
  else if (control->m_type == CListBase::LIST_LABEL)
  {
    CListLabel *label = (CListLabel *)control;
    CStdStringW oldText = label->m_text;
    if (label->m_info)
      g_charsetConverter.utf8ToUTF16(g_infoManager.GetItemLabel(item, label->m_info), label->m_text);
    else
      g_charsetConverter.utf8ToUTF16(g_infoManager.GetItemMultiLabel(item, label->m_multiInfo), label->m_text);
    if (oldText != label->m_text)
    { // changed label - reset scrolling
      label->m_scrollInfo.Reset();
    }
    if (label->m_label.font)
    {
      label->m_label.font->GetTextExtent(label->m_text, &label->m_textW, &label->m_renderH);
      label->m_renderW = min(label->m_textW, label->m_width);
      if (label->m_label.align & XBFONT_CENTER_Y)
        label->m_renderY = label->m_posY + (label->m_height - label->m_renderH) * 0.5f;
      else
        label->m_renderY = label->m_posY;
      if (label->m_label.align & XBFONT_RIGHT)
        label->m_renderX = label->m_posX - label->m_renderW;
      else if (label->m_label.align & XBFONT_CENTER_X)
        label->m_renderX = label->m_posX - label->m_renderW * 0.5f;
      else
        label->m_renderX = label->m_posX;
    }
  }
}

void CGUIListItemLayout::RenderLabel(CListLabel *label, bool selected, bool scroll)
{
  if (label->m_label.font && !label->m_text.IsEmpty())
  {
    DWORD color = selected ? label->m_label.selectedColor : label->m_label.textColor;
    if (scroll && label->m_renderW < label->m_textW)
      label->m_label.font->DrawScrollingText(label->m_renderX, label->m_renderY, &color, 1,
                                  label->m_label.shadowColor, label->m_text, label->m_renderW, label->m_scrollInfo);
    else
      label->m_label.font->DrawTextWidth(label->m_renderX, label->m_renderY, label->m_label.angle, color,
                                  label->m_label.shadowColor, label->m_text, label->m_renderW);
  }
}

void CGUIListItemLayout::ResetScrolling()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
  {
    CListBase *layoutItem = (*it);
    if (layoutItem->m_type == CListBase::LIST_LABEL)
      ((CListLabel *)layoutItem)->m_scrollInfo.Reset();
  }
}

void CGUIListItemLayout::QueueAnimation(ANIMATION_TYPE animType)
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
  {
    CListBase *layoutItem = (*it);
    if (layoutItem->m_type == CListBase::LIST_IMAGE ||
        layoutItem->m_type == CListBase::LIST_TEXTURE)
      ((CListTexture *)layoutItem)->m_image.QueueAnimation(animType);
  }
}

CGUIListItemLayout::CListBase *CGUIListItemLayout::CreateItem(TiXmlElement *child)
{
  // resolve any <include> tag's in this control
  g_SkinInfo.ResolveIncludes(child);

  // grab the type...
  CStdString type = CGUIControlFactory::GetType(child);

  // resolve again with strType set so that <default> tags are added
  g_SkinInfo.ResolveIncludes(child, type);

  float posX = 0;
  float posY = 0;
  float width = 10;
  float height = 10;
  CStdString infoString;
  CImage image;
  CLabelInfo label;
  CGUIControlFactory::GetFloat(child, "posx", posX);
  CGUIControlFactory::GetFloat(child, "posy", posY);
  CGUIControlFactory::GetFloat(child, "width", width);
  CGUIControlFactory::GetFloat(child, "height", height);
  XMLUtils::GetString(child, "info", infoString);
  CGUIControlFactory::GetColor(child, "textcolor", label.textColor);
  CGUIControlFactory::GetColor(child, "selectedcolor", label.selectedColor);
  CGUIControlFactory::GetColor(child, "shadowcolor", label.shadowColor);
  CStdString fontName;
  XMLUtils::GetString(child, "font", fontName);
  label.font = g_fontManager.GetFont(fontName);
  int info = g_infoManager.TranslateString(infoString);
  if (info && (info < LISTITEM_START || info > LISTITEM_END))
  {
    CLog::Log(LOGERROR, __FUNCTION__" Invalid item info %s", infoString.c_str());
    return NULL;
  }
  CGUIControlFactory::GetTexture(child, "texture", image);
  CGUIControlFactory::GetAlignment(child, "align", label.align);
  FRECT rect = { posX, posY, width, height };
  vector<CAnimation> animations;
  CGUIControlFactory::GetAnimations(child, rect, animations);
  D3DCOLOR colorDiffuse(0xffffffff);
  CGUIControlFactory::GetColor(child, "colordiffuse", colorDiffuse);
  DWORD alignY = 0;
  if (CGUIControlFactory::GetAlignmentY(child, "aligny", alignY))
    label.align |= alignY;
  CStdString content;
  XMLUtils::GetString(child, "label", content);
  CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio = CGUIImage::ASPECT_RATIO_KEEP;
  DWORD aspectAlign = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
  CGUIControlFactory::GetAspectRatio(child, "aspectratio", aspectRatio, aspectAlign);
  int visibleCondition = 0;
  CGUIControlFactory::GetConditionalVisibility(child, visibleCondition);
  if (type == "label")
  { // info label
    return new CListLabel(posX, posY, width, height, visibleCondition, label, info, content);
  }
  else if (type == "image")
  {
    if (info)
    { // info image
      return new CListImage(posX, posY, width, height, visibleCondition, image, aspectRatio, aspectAlign, colorDiffuse, animations, info);
    }
    else
    { // texture
      return new CListTexture(posX, posY, width, height, visibleCondition, image, CGUIImage::ASPECT_RATIO_STRETCH, aspectAlign, colorDiffuse, animations);
    }
  }
  return NULL;
}

void CGUIListItemLayout::LoadLayout(TiXmlElement *layout, bool focused)
{
  m_focused = focused;
  g_SkinInfo.ResolveConstant(layout->Attribute("width"), m_width);
  g_SkinInfo.ResolveConstant(layout->Attribute("height"), m_height);
  TiXmlElement *child = layout->FirstChildElement("control");
  while (child)
  {
    CListBase *item = CreateItem(child);
    if (item)
      m_controls.push_back(item);
    child = child->NextSiblingElement("control");
  }
}

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
void CGUIListItemLayout::CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CImage &texture, const CImage &textureFocus, float texHeight, float iconWidth, float iconHeight, int nofocusCondition, int focusCondition)
{
  m_width = width;
  m_height = height;
  m_focused = focused;
  vector<CAnimation> blankAnims;
  CListTexture *tex = new CListTexture(0, 0, width, texHeight, nofocusCondition, texture, CGUIImage::ASPECT_RATIO_STRETCH, 0, 0xffffffff, blankAnims);
  m_controls.push_back(tex);
  if (focused)
  {
    CListTexture *tex = new CListTexture(0, 0, width, texHeight, focusCondition, textureFocus, CGUIImage::ASPECT_RATIO_STRETCH, 0, 0xffffffff, blankAnims);
    m_controls.push_back(tex);
  }
  CListImage *image = new CListImage(8, 0, iconWidth, texHeight, 0, CImage(""), CGUIImage::ASPECT_RATIO_KEEP, 0, 0xffffffff, blankAnims, LISTITEM_ICON);
  m_controls.push_back(image);
  float x = iconWidth + labelInfo.offsetX + 10;
  CListLabel *label = new CListLabel(x, labelInfo.offsetY, width - x - 18, height, 0, labelInfo, LISTITEM_LABEL, "");
  m_controls.push_back(label);
  x = labelInfo2.offsetX ? labelInfo2.offsetX : m_width - 16;
  label = new CListLabel(x, labelInfo2.offsetY, x - iconWidth - 20, height, 0, labelInfo2, LISTITEM_LABEL2, "");
  m_controls.push_back(label);
}

void CGUIListItemLayout::CreateThumbnailPanelLayouts(float width, float height, bool focused, const CImage &image, float texWidth, float texHeight, float thumbPosX, float thumbPosY, float thumbWidth, float thumbHeight, DWORD thumbAlign, CGUIImage::GUIIMAGE_ASPECT_RATIO thumbAspect, const CLabelInfo &labelInfo, bool hideLabels)
{
  m_width = width;
  m_height = height;
  m_focused = focused;
  float centeredPosX = (m_width - texWidth)*0.5f;
  // background texture
  vector<CAnimation> blankAnims;
  CListTexture *tex = new CListTexture(centeredPosX, 0, texWidth, texHeight, 0, image, CGUIImage::ASPECT_RATIO_STRETCH, 0, 0xffffffff, blankAnims);
  m_controls.push_back(tex);
  // thumbnail
  float xOff = 0;
  float yOff = 0;
  if (thumbAlign != 0)
  {
    xOff += (texWidth - thumbWidth) * 0.5f;
    yOff += (texHeight - thumbHeight) * 0.5f;
    //if thumbPosX or thumbPosX != 0 the thumb will be bumped off-center
  }
  CListImage *thumb = new CListImage(thumbPosX + centeredPosX + xOff, thumbPosY + yOff, thumbWidth, thumbHeight, 0, CImage(""), thumbAspect, 0, 0xffffffff, blankAnims, LISTITEM_ICON);
  m_controls.push_back(thumb);
  // overlay
  CListImage *overlay = new CListImage(thumbPosX + centeredPosX + xOff + thumbWidth - 32, thumbPosY + yOff + thumbHeight - 32, 32, 32, 0, CImage(""), thumbAspect, 0, 0xffffffff, blankAnims, LISTITEM_OVERLAY);
  m_controls.push_back(overlay);
  // label
  if (hideLabels) return;
  CListLabel *label = new CListLabel(width*0.5f, texHeight, width, height, 0, labelInfo, LISTITEM_LABEL, "");
  m_controls.push_back(label);
}
//#endif

#ifdef _DEBUG
void CGUIListItemLayout::DumpTextureUse()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
  {
    CListBase *layoutItem = (*it);
    if (layoutItem->m_type == CListBase::LIST_IMAGE || layoutItem->m_type == CListBase::LIST_TEXTURE)
      ((CListTexture *)layoutItem)->m_image.DumpTextureUse();
  }
}
#endif
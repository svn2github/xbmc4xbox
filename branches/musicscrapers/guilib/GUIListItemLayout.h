#pragma once

#include "GUIBorderedImage.h"
#include "GUIListLabel.h"
#include "GUIMultiSelectText.h"

class CGUIListItem;
class CFileItem;

class CGUIListItemLayout
{
  class CListBase
  {
  public:
    CListBase();
    virtual ~CListBase();

    enum LIST_TYPE { LIST_LABEL, LIST_SELECT_LABEL, LIST_IMAGE, LIST_TEXTURE };
    LIST_TYPE m_type;
  };

  class CListLabel : public CListBase
  {
  public:
    CListLabel(float posX, float posY, float width, float height, int visibleCondition, const CLabelInfo &label, bool alwyasScroll, const CGUIInfoLabel &content, const std::vector<CAnimation> &animations);
    virtual ~CListLabel();

    CGUIListLabel m_label;
    CGUIInfoLabel m_info;
  };

  class CListSelectLabel : public CListBase
  {
  public:
    CListSelectLabel(float posX, float posY, float width, float height, int visibleCondition, const CImage &imageFocus, const CImage &imageNoFocus, const CLabelInfo &label, const CGUIInfoLabel &content, const std::vector<CAnimation> &animations);
    virtual ~CListSelectLabel();

    CGUIMultiSelectTextControl m_label;
    CGUIInfoLabel m_info;
  };

  class CListTexture : public CListBase
  {
  public:
    CListTexture(float posX, float posY, float width, float height, int visibleCondition, const CImage &image, const CImage &borderImage, const FRECT &borderSize, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio, DWORD aspectAlign, D3DCOLOR colorDiffuse, const std::vector<CAnimation> &animations);
    virtual ~CListTexture();
    CGUIBorderedImage m_image;
  };

  class CListImage: public CListTexture
  {
  public:
    CListImage(float posX, float posY, float width, float height, int visibleCondition, const CImage &image, const CImage &borderImage, const FRECT &borderSize, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio, DWORD aspectAlign, D3DCOLOR colorDiffuse, const CGUIInfoLabel &content, const std::vector<CAnimation> &animations);
    virtual ~CListImage();
    CGUIInfoLabel m_info;
  };

public:
  CGUIListItemLayout();
  CGUIListItemLayout(const CGUIListItemLayout &from);
  virtual ~CGUIListItemLayout();
  void LoadLayout(TiXmlElement *layout, bool focused);
  void Render(CGUIListItem *item, DWORD parentID, DWORD time = 0);
  float Size(ORIENTATION orientation) const;
  unsigned int GetFocus() const;
  void SetFocus(unsigned int focus);
  void ResetScrolling();

  void SetInvalid() { m_invalidated = true; };

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  void CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CImage &texture, const CImage &textureFocus, float texHeight, float iconWidth, float iconHeight, int nofocusCondition, int focusCondition);
  void CreateThumbnailPanelLayouts(float width, float height, bool focused, const CImage &image, float texWidth, float texHeight, float thumbPosX, float thumbPosY, float thumbWidth, float thumbHeight, DWORD thumbAlign, CGUIImage::GUIIMAGE_ASPECT_RATIO thumbAspect, const CLabelInfo &labelInfo, bool hideLabel);
//#endif

  void SelectItemFromPoint(const CPoint &point);
  bool MoveLeft();
  bool MoveRight();

  int GetCondition() const { return m_condition; };
#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  CListBase *CreateItem(TiXmlElement *child);
  void UpdateItem(CListBase *control, CFileItem *item, DWORD parentID);
  void RenderLabel(CListLabel *label, bool selected, bool scroll);
  void Update(CFileItem *item, DWORD parentID);

  std::vector<CListBase*> m_controls;
  typedef std::vector<CListBase*>::iterator iControls;
  typedef std::vector<CListBase*>::const_iterator ciControls;
  float m_width;
  float m_height;
  bool m_focused;
  bool m_invalidated;

  int m_condition;
  bool m_isPlaying;
};


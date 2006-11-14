/*!
\file GUIControlGroupList.h
\brief 
*/

#pragma once

#include "GUIControlGroup.h"

/*!
 \ingroup controls
 \brief list of controls that is scrollable
 */
class CGUIControlGroupList : public CGUIControlGroup
{
public:
  CGUIControlGroupList(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float itemGap, DWORD pageControl);
  virtual ~CGUIControlGroupList(void);
  virtual void Render();
  virtual bool OnMessage(CGUIMessage& message);

protected:
  void ValidateOffset();
  float m_itemGap;
  DWORD m_pageControl;

  float m_offset; // measurement in pixels of our origin
  float m_totalSize;
};


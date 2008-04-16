/*!
\file GUIFixedListContainer.h
\brief 
*/

#pragma once

#include "GUIBaseContainer.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIFixedListContainer : public CGUIBaseContainer
{
public:
  CGUIFixedListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int fixedPosition);
  virtual ~CGUIFixedListContainer(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void ValidateOffset();
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void SelectItem(int item);
  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;
  virtual int GetCurrentPage() const;
};


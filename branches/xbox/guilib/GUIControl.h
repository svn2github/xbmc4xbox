/*!
\file GUIControl.h
\brief 
*/

#ifndef GUILIB_GUICONTROL_H
#define GUILIB_GUICONTROL_H
#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GraphicContext.h" // needed by any rendering operation (all controls)
#include "Key.h"            // needed by practically all controls (CAction + defines)
#include "GUIMessage.h"     // needed by practically all controls
#include "GUIFont.h"        // needed for the CAngle member (CLabelInfo) among other stuff
#include "VisibleEffect.h"  // needed for the CAnimation members
#include "GUIInfoColor.h"   // needed for CGuiInfoColor to handle infolabel'ed colors

class CGUIListItem; // forward

enum ORIENTATION { HORIZONTAL = 0, VERTICAL };

class CLabelInfo
{
public:
  CLabelInfo()
  {
    font = NULL;
    align = XBFONT_LEFT;
    offsetX = offsetY = 0;
    width = 0;
    angle = 0;
  };
  void UpdateColors()
  {
    textColor.Update();
    shadowColor.Update();
    selectedColor.Update();
    disabledColor.Update();
    focusedColor.Update();
  };

  CGUIInfoColor textColor;
  CGUIInfoColor shadowColor;
  CGUIInfoColor selectedColor;
  CGUIInfoColor disabledColor;
  CGUIInfoColor focusedColor;
  uint32_t align;
  float offsetX;
  float offsetY;
  float width;
  float angle;
  CGUIFont *font;
};

class CControlState
{
public:
  CControlState(int id, int data)
  {
    m_id = id;
    m_data = data;
  }
  int m_id;
  int m_data;
};

/*!
 \ingroup controls
 \brief Base class for controls
 */
class CGUIControl
{
public:
  CGUIControl();
  CGUIControl(int parentID, int controlID, float posX, float posY, float width, float height);
  virtual ~CGUIControl(void);
  virtual CGUIControl *Clone() const=0;

  virtual void DoRender(DWORD currentTime);
  virtual void Render();
  bool HasRendered() const { return m_hasRendered; };

  // OnAction() is called by our window when we are the focused control.
  // We should process any control-specific actions in the derived classes,
  // and return true if we have taken care of the action.  Returning false
  // indicates that the message may be handed down to the window or application
  // levels.  This base class implementation handles basic movement, and should
  // be called from the derived classes when the action has not been handled.
  // Return true to indicate that the action has been dealt with.
  virtual bool OnAction(const CAction &action);

  // Common actions to make the code easier to read (no ugly switch statements in derived controls)
  virtual void OnUp();
  virtual void OnDown();
  virtual void OnLeft();
  virtual void OnRight();
  virtual void OnFocus() {};
  virtual void OnUnFocus() {};

  /// \brief Called when the mouse is over the control.  Default implementation selects the control.
  virtual bool OnMouseOver(const CPoint &point);
  /// \brief Called when the mouse is dragging over the control.  Default implementation does nothing.
  virtual bool OnMouseDrag(const CPoint &offset, const CPoint &point) { return false; };
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual bool OnMouseClick(int button, const CPoint &point) { return false; };
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual bool OnMouseDoubleClick(int button, const CPoint &point) { return false; };
  /// \brief Called when the mouse wheel has moved whilst over the control.  Default implementation does nothing
  virtual bool OnMouseWheel(char wheel, const CPoint &point) { return false; };
  /// \brief Used to test whether the pointer location (fPosX, fPosY) is inside the control.  For mouse events.
  virtual bool HitTest(const CPoint &point) const;
  /// \brief Focus a control from a screen location.  Returns the coordinates of the screen location relative to the control and a pointer to the control.
  virtual bool CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const;
  /// \brief Unfocus a control if it's not in a screen location.
  virtual void UnfocusFromPoint(const CPoint &point);

  virtual bool OnMessage(CGUIMessage& message);
  virtual int GetID(void) const;
  void SetID(int id) { m_controlID = id; };
  virtual bool HasID(int id) const;
  virtual bool HasVisibleID(int id) const;
  int GetParentID() const;
  virtual bool HasFocus() const;
  virtual void PreAllocResources() {}
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return false; };
  virtual bool CanFocus() const;
  virtual bool IsVisible() const;
  bool IsVisibleFromSkin() const { return m_visibleFromSkinCondition; };
  virtual bool IsDisabled() const;
  virtual void SetPosition(float posX, float posY);
  virtual void SetHitRect(const CRect &rect);
  virtual void SetCamera(const CPoint &camera);
  void SetColorDiffuse(const CGUIInfoColor &color);
  CPoint GetRenderPosition() const;
  virtual float GetXPosition() const;
  virtual float GetYPosition() const;
  virtual float GetWidth() const;
  virtual float GetHeight() const;
  virtual void SetNavigation(int up, int down, int left, int right);
  virtual void SetNavigationActions(const std::vector<CStdString> &up, const std::vector<CStdString> &down,
                                    const std::vector<CStdString> &left, const std::vector<CStdString> &right);
  void ExecuteActions(const std::vector<CStdString> &actions);
  int GetControlIdUp() const { return m_controlUp;};
  int GetControlIdDown() const { return m_controlDown;};
  int GetControlIdLeft() const { return m_controlLeft;};
  int GetControlIdRight() const { return m_controlRight;};
  virtual int GetNextControl(int direction) const;
  virtual void SetFocus(bool focus);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetVisible(bool bVisible);
  void SetVisibleCondition(int visible, const CGUIInfoBool &allowHiddenFocus);
  int GetVisibleCondition() const { return m_visibleCondition; };
  void SetEnableCondition(int condition);
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void SetInitialVisibility();
  virtual void SetEnabled(bool bEnable);
  virtual void SetInvalid() { m_bInvalidated = true; };
  virtual void SetPulseOnSelect(bool pulse) { m_pulseOnSelect = pulse; };
  virtual CStdString GetDescription() const { return ""; };

  void SetAnimations(const std::vector<CAnimation> &animations);
  const std::vector<CAnimation> &GetAnimations() const { return m_animations; };

  virtual void QueueAnimation(ANIMATION_TYPE anim);
  virtual bool IsAnimating(ANIMATION_TYPE anim);
  virtual bool HasAnimation(ANIMATION_TYPE anim);
  CAnimation *GetAnimation(ANIMATION_TYPE type, bool checkConditions = true);
  virtual void ResetAnimation(ANIMATION_TYPE type);
  virtual void ResetAnimations();

  // push information updates
  virtual void UpdateInfo(const CGUIListItem *item = NULL) {};
  virtual void SetPushUpdates(bool pushUpdates) { m_pushedUpdates = pushUpdates; };

  virtual bool IsGroup() const { return false; };
  virtual bool IsContainer() const { return false; };
  virtual bool GetCondition(int condition, int data) const { return false; };

  void SetParentControl(CGUIControl *control) { m_parentControl = control; };
  virtual void SaveStates(std::vector<CControlState> &states);

  enum GUICONTROLTYPES {
    GUICONTROL_UNKNOWN,
    GUICONTROL_BUTTON,
    GUICONTROL_CHECKMARK,
    GUICONTROL_FADELABEL,
    GUICONTROL_IMAGE,
    GUICONTROL_BORDEREDIMAGE,
    GUICONTROL_LARGE_IMAGE,
    GUICONTROL_LABEL,
    GUICONTROL_LIST,
    GUICONTROL_LISTGROUP,
    GUICONTROL_LISTEX,
    GUICONTROL_PROGRESS,
    GUICONTROL_RADIO,
    GUICONTROL_RSS,
    GUICONTROL_SELECTBUTTON,
    GUICONTROL_SLIDER,
    GUICONTROL_SETTINGS_SLIDER,
    GUICONTROL_SPINBUTTON,
    GUICONTROL_SPIN,
    GUICONTROL_SPINEX,
    GUICONTROL_TEXTBOX,
    GUICONTROL_THUMBNAIL,
    GUICONTROL_TOGGLEBUTTON,
    GUICONTROL_VIDEO,
    GUICONTROL_MOVER,
    GUICONTROL_RESIZE,
    GUICONTROL_BUTTONBAR,
    GUICONTROL_CONSOLE,
    GUICONTROL_EDIT,
    GUICONTROL_VISUALISATION,
    GUICONTROL_MULTI_IMAGE,
    GUICONTROL_GROUP,
    GUICONTROL_GROUPLIST,
    GUICONTROL_SCROLLBAR,
    GUICONTROL_LISTLABEL,
    GUICONTROL_MULTISELECT,
    GUICONTAINER_LIST,
    GUICONTAINER_WRAPLIST,
    GUICONTAINER_FIXEDLIST,
    GUICONTAINER_PANEL
  };
  GUICONTROLTYPES GetControlType() const { return ControlType; }

  enum GUIVISIBLE { HIDDEN = 0, DELAYED, VISIBLE };

#ifdef _DEBUG
  virtual void DumpTextureUse() {};
#endif
protected:
  virtual void UpdateColors();
  virtual void Animate(DWORD currentTime);
  virtual bool CheckAnimation(ANIMATION_TYPE animType);
  void UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState);
  bool SendWindowMessage(CGUIMessage &message);

  // navigation
  int m_controlLeft;
  int m_controlRight;
  int m_controlUp;
  int m_controlDown;
  std::vector<CStdString> m_leftActions;
  std::vector<CStdString> m_rightActions;
  std::vector<CStdString> m_upActions;
  std::vector<CStdString> m_downActions;

  float m_posX;
  float m_posY;
  float m_height;
  float m_width;
  CRect m_hitRect;
  CGUIInfoColor m_diffuseColor;
  int m_controlID;
  int m_parentID;
  bool m_bHasFocus;
  bool m_bInvalidated;
  bool m_bAllocated;
  bool m_pulseOnSelect;
  GUICONTROLTYPES ControlType;

  CGUIControl *m_parentControl;   // our parent control if we're part of a group

  // visibility condition/state
  int m_visibleCondition;
  GUIVISIBLE m_visible;
  bool m_visibleFromSkinCondition;
  bool m_forceHidden;       // set from the code when a hidden operation is given - overrides m_visible
  CGUIInfoBool m_allowHiddenFocus;
  bool m_hasRendered;
  // enable/disable state
  int m_enableCondition;
  bool m_enabled;

  bool m_pushedUpdates;

  // animation effects
  std::vector<CAnimation> m_animations;
  CPoint m_camera;
  bool m_hasCamera;
  TransformMatrix m_transform;
};

#endif

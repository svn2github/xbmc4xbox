#include "include.h"
#include "GUIControl.h"

#include "../xbmc/utils/GUIInfoManager.h"
#include "LocalizeStrings.h"
#include "../xbmc/Util.h"
#include "GUIWindowManager.h"

CGUIControl::CGUIControl()
{
  m_hasRendered = false;
  m_bHasFocus = false;
  m_dwControlID = 0;
  m_dwParentID = 0;
  m_visible = true;
  m_visibleFromSkinCondition = true;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_bDisabled = false;
  m_colDiffuse = 0xFFFFFFFF;
  m_posX = 0;
  m_posY = 0;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_parentControl = NULL;
}

CGUIControl::CGUIControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height)
{
  m_colDiffuse = 0xFFFFFFFF;
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_bHasFocus = false;
  m_dwControlID = dwControlId;
  m_dwParentID = dwParentID;
  m_visible = true;
  m_visibleFromSkinCondition = true;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_bDisabled = false;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_hasRendered = false;
  m_parentControl = NULL;
}


CGUIControl::~CGUIControl(void)
{

}

void CGUIControl::AllocResources()
{
  m_hasRendered = false;
  m_bInvalidated = true;
  m_bAllocated=true;
}

void CGUIControl::FreeResources()
{
  if (m_bAllocated)
  {
    // Reset our animation states
    for (unsigned int i = 0; i < m_animations.size(); i++)
      m_animations[i].ResetAnimation();
    m_bAllocated=false;
  }
  m_hasRendered = false;
}

bool CGUIControl::IsAllocated() const
{
  return m_bAllocated;
}

void CGUIControl::DynamicResourceAlloc(bool bOnOff)
{

}

void CGUIControl::Render()
{
  m_bInvalidated = false;
  m_hasRendered = true;
}

bool CGUIControl::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_MOVE_DOWN:
    if (!HasFocus()) return false;
    OnDown();
    return true;
    break;

  case ACTION_MOVE_UP:
    if (!HasFocus()) return false;
    OnUp();
    return true;
    break;

  case ACTION_MOVE_LEFT:
    if (!HasFocus()) return false;
    OnLeft();
    return true;
    break;

  case ACTION_MOVE_RIGHT:
    if (!HasFocus()) return false;
    OnRight();
    return true;
    break;
  }
  return false;
}

// Movement controls (derived classes can override)
void CGUIControl::OnUp()
{
  if (HasFocus() && m_dwControlID != m_dwControlUp)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_UP);
    SendWindowMessage(msg);
  }
}

void CGUIControl::OnDown()
{
  if (HasFocus() && m_dwControlID != m_dwControlDown)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_DOWN);
    SendWindowMessage(msg);
  }
}

void CGUIControl::OnLeft()
{
  if (HasFocus() && m_dwControlID != m_dwControlLeft)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_LEFT);
    SendWindowMessage(msg);
  }
}

void CGUIControl::OnRight()
{
  if (HasFocus() && m_dwControlID != m_dwControlRight)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_RIGHT);
    SendWindowMessage(msg);
  }
}

void CGUIControl::SendWindowMessage(CGUIMessage &message)
{
  CGUIWindow *pWindow = m_gWindowManager.GetWindow(GetParentID());
  if (pWindow)
    pWindow->OnMessage(message);
  else
    g_graphicsContext.SendMessage(message);
}

DWORD CGUIControl::GetID(void) const
{
  return m_dwControlID;
}


DWORD CGUIControl::GetParentID(void) const
{
  return m_dwParentID;
}

bool CGUIControl::HasFocus(void) const
{
  return m_bHasFocus;
}

void CGUIControl::SetFocus(bool bOnOff)
{
  if (m_bHasFocus && !bOnOff)
    QueueAnimation(ANIM_TYPE_UNFOCUS);
  else if (!m_bHasFocus && bOnOff)
    QueueAnimation(ANIM_TYPE_FOCUS);
  m_bHasFocus = bOnOff;
}

bool CGUIControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    switch (message.GetMessage() )
    {
    case GUI_MSG_SETFOCUS:
      // if control is disabled then move 2 the next control
      if ( !CanFocus() )
      {
        CLog::Log(LOGERROR, "Control %d in window %d has been asked to focus, but it can't", GetID(), GetParentID());
        return false;
      }
      SetFocus(true);
      {
        // inform our parent window that this has happened
        CGUIMessage message(GUI_MSG_FOCUSED, GetParentID(), GetID());
        if (m_parentControl)
          m_parentControl->OnMessage(message);
        else
        SendWindowMessage(message);
      }
      return true;
      break;

    case GUI_MSG_LOSTFOCUS:
      {
        SetFocus(false);
        // and tell our parent so it can unfocus
        if (m_parentControl)
          m_parentControl->OnMessage(message);
        return true;
      }
      break;

    case GUI_MSG_VISIBLE:
      m_visible = m_visibleCondition ? g_infoManager.GetBool(m_visibleCondition, m_dwParentID) : true;
      m_forceHidden = false;
      return true;
      break;

    case GUI_MSG_HIDDEN:
      m_forceHidden = true;
      // reset any visible animations that are in process
      if (IsAnimating(ANIM_TYPE_VISIBLE))
      {
//        CLog::DebugLog("Resetting visible animation on control %i (we are %s)", m_dwControlID, m_visible ? "visible" : "hidden");
        CAnimation *visibleAnim = GetAnimation(ANIM_TYPE_VISIBLE);
        if (visibleAnim) visibleAnim->ResetAnimation();
      }
      return true;
      break;

    case GUI_MSG_ENABLED:
      m_bDisabled = false;
      return true;
      break;

    case GUI_MSG_DISABLED:
      m_bDisabled = true;
      return true;
      break;
    }
  }
  return false;
}

bool CGUIControl::CanFocus() const
{
  if (!IsVisible() && !m_allowHiddenFocus) return false;
  if (IsDisabled()) return false;
  return true;
}

bool CGUIControl::IsVisible() const
{
  if (m_forceHidden) return false;
  return m_visible;
}

bool CGUIControl::IsDisabled() const
{
  return m_bDisabled;
}

void CGUIControl::SetEnabled(bool bEnable)
{
  m_bDisabled = !bEnable;
}

void CGUIControl::SetPosition(float posX, float posY)
{
  if ((m_posX != posX) || (m_posY != posY))
  {
    m_posX = posX;
    m_posY = posY;
    Update();
  }
}

void CGUIControl::SetColourDiffuse(D3DCOLOR colour)
{
  if (colour != m_colDiffuse)
  {
    m_colDiffuse = colour;
    Update();
  }
}

float CGUIControl::GetXPosition() const
{
  return m_posX;
}

float CGUIControl::GetYPosition() const
{
  return m_posY;
}

float CGUIControl::GetWidth() const
{
  return m_width;
}

float CGUIControl::GetHeight() const
{
  return m_height;
}

void CGUIControl::SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight)
{
  m_dwControlUp = dwUp;
  m_dwControlDown = dwDown;
  m_dwControlLeft = dwLeft;
  m_dwControlRight = dwRight;
}

void CGUIControl::SetWidth(float width)
{
  if (m_width != width)
  {
    m_width = width;
    Update();
  }
}

void CGUIControl::SetHeight(float height)
{
  if (m_height != height)
  {
    m_height = height;
    Update();
  }
}

void CGUIControl::SetVisible(bool bVisible)
{
  // just force to hidden if necessary
  m_forceHidden = !bVisible;
/*
  if (m_visibleCondition)
    bVisible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
  if (m_bVisible != bVisible)
  {
    m_visible = bVisible;
    m_visibleFromSkinCondition = bVisible;
    m_bInvalidated = true;
  }*/
}

bool CGUIControl::HitTest(float posX, float posY) const
{
  if( posX >= g_graphicsContext.ScaleFinalXCoord(m_posX, m_posY)
   && posX <= g_graphicsContext.ScaleFinalXCoord(m_posX + m_width, m_posY)
   && posY >= g_graphicsContext.ScaleFinalYCoord(m_posX, m_posY)
   && posY <= g_graphicsContext.ScaleFinalYCoord(m_posX, m_posY + m_height) )
    return true;
  return false;
}

// override this function to implement custom mouse behaviour
bool CGUIControl::OnMouseOver()
{
  if (g_Mouse.GetState() != MOUSE_STATE_DRAG)
    g_Mouse.SetState(MOUSE_STATE_FOCUS);
  if (!CanFocus()) return false;
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), GetID());
  SendWindowMessage(msg);
  return true;
}

void CGUIControl::UpdateVisibility()
{
  bool bWasVisible = m_visibleFromSkinCondition;
  m_visibleFromSkinCondition = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
  if (!bWasVisible && m_visibleFromSkinCondition)
  { // automatic change of visibility - queue the in effect
//    CLog::DebugLog("Visibility changed to visible for control id %i", m_dwControlID);
    QueueAnimation(ANIM_TYPE_VISIBLE);
  }
  else if (bWasVisible && !m_visibleFromSkinCondition)
  { // automatic change of visibility - do the out effect
//    CLog::DebugLog("Visibility changed to hidden for control id %i", m_dwControlID);
    QueueAnimation(ANIM_TYPE_HIDDEN);
  }
}

void CGUIControl::SetInitialVisibility()
{
  if (m_visibleCondition)
  {
    m_visibleFromSkinCondition = m_visible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
  //  CLog::DebugLog("Set initial visibility for control %i: %s", m_dwControlID, m_visible ? "visible" : "hidden");
    // no need to enquire every frame if we are always visible or always hidden
    if (m_visibleCondition == SYSTEM_ALWAYS_TRUE || m_visibleCondition == SYSTEM_ALWAYS_FALSE)
      m_visibleCondition = 0;
  }
}

void CGUIControl::UpdateEffectState(DWORD currentTime)
{
  if (m_visibleCondition)
    UpdateVisibility();
  Animate(currentTime);
}

void CGUIControl::SetVisibleCondition(int visible, bool allowHiddenFocus)
{
  m_visibleCondition = visible;
  m_allowHiddenFocus = allowHiddenFocus;
}

void CGUIControl::SetAnimations(const vector<CAnimation> &animations)
{
  m_animations = animations;
}

void CGUIControl::QueueAnimation(ANIMATION_TYPE animType)
{
  // rule out the animations we shouldn't perform
  if (!IsVisible() || !HasRendered()) 
  { // hidden or never rendered - don't allow exit or entry animations for this control
    if (animType == ANIM_TYPE_WINDOW_CLOSE && !IsAnimating(ANIM_TYPE_WINDOW_OPEN))
      return;
  }
  if (!IsVisible())
  { // hidden - only allow hidden anims if we're animating a visible anim
    if (animType == ANIM_TYPE_HIDDEN && !IsAnimating(ANIM_TYPE_VISIBLE))
      return;
    if (animType == ANIM_TYPE_WINDOW_OPEN)
      return;
  }
  CAnimation *reverseAnim = GetAnimation((ANIMATION_TYPE)-animType, false);
  CAnimation *forwardAnim = GetAnimation(animType);
  // we first check whether the reverse animation is in progress (and reverse it)
  // then we check for the normal animation, and queue it
  if (reverseAnim && reverseAnim->IsReversible() && (reverseAnim->currentState == ANIM_STATE_IN_PROCESS || reverseAnim->currentState == ANIM_STATE_DELAYED))
  {
    reverseAnim->queuedProcess = ANIM_PROCESS_REVERSE;
    if (forwardAnim) forwardAnim->ResetAnimation();
  }
  else if (forwardAnim)
  {
    forwardAnim->queuedProcess = ANIM_PROCESS_NORMAL;
    if (reverseAnim) reverseAnim->ResetAnimation();
  }
  else
  { // hidden and visible animations delay the change of state.  If there is no animations
    // to perform, then we should just change the state straightaway
    if (reverseAnim) reverseAnim->ResetAnimation();
    UpdateStates(animType, ANIM_PROCESS_NORMAL, ANIM_STATE_APPLIED);
  }
}

CAnimation *CGUIControl::GetAnimation(ANIMATION_TYPE type, bool checkConditions /* = true */)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    if (m_animations[i].type == type)
    {
      if (!checkConditions || !m_animations[i].condition || g_infoManager.GetBool(m_animations[i].condition))
        return &m_animations[i];
    }
  }
  return NULL;
}

void CGUIControl::UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState)
{
  bool visible = m_visible;
  // Make sure control is hidden or visible at the appropriate times
  // while processing a visible or hidden animation it needs to be visible,
  // but when finished a hidden operation it needs to be hidden
  if (type == ANIM_TYPE_VISIBLE)
  {
    if (currentProcess == ANIM_PROCESS_REVERSE)
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_visible = false;
    }
    else if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_visible = false;
      else
        m_visible = m_visibleFromSkinCondition;
    }
  }
  else if (type == ANIM_TYPE_HIDDEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)  // a hide animation
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_visible = false; // finished
      else
        m_visible = true; // have to be visible until we are finished
    }
    else if (currentProcess == ANIM_PROCESS_REVERSE)  // a visible animation
    { // no delay involved here - just make sure it's visible
      m_visible = m_visibleFromSkinCondition;
    }
  }
  else if (type == ANIM_TYPE_WINDOW_OPEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_visible = false; // delayed
      else
        m_visible = m_visibleFromSkinCondition;
    }
  }
  else if (type == ANIM_TYPE_FOCUS)
  {
    // call the focus function if we have finished a focus animation
    // (buttons can "click" on focus)
    if (currentProcess == ANIM_PROCESS_NORMAL && currentState == ANIM_STATE_APPLIED)
      OnFocus();
  }
//  if (visible != m_visible)
//    CLog::DebugLog("UpdateControlState of control id %i - now %s (type=%d, process=%d, state=%d)", m_dwControlID, m_visible ? "visible" : "hidden", type, currentProcess, currentState);
}

void CGUIControl::Animate(DWORD currentTime)
{
  TransformMatrix transform;
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasRendered());
    // Update the control states (such as visibility)
    UpdateStates(anim.type, anim.currentProcess, anim.currentState);
    // and render the animation effect
    anim.RenderAnimation(transform);
/*
    // debug stuff
    if (anim.currentProcess != ANIM_PROCESS_NONE)
    {
      if (anim.effect == EFFECT_TYPE_SLIDE)
      {
        if (IsVisible())
          CLog::DebugLog("Animating control %d with a %s slide effect %s. Amount is %2.1f, visible=%s", m_dwControlID, anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
      else if (anim.effect == EFFECT_TYPE_FADE)
      {
        if (IsVisible())
          CLog::DebugLog("Animating control %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", m_dwControlID, anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
    }*/
  }
  g_graphicsContext.SetControlTransform(transform);
}

bool CGUIControl::IsAnimating(ANIMATION_TYPE animType)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.type == animType)
    {
      if (anim.queuedProcess == ANIM_PROCESS_NORMAL)
        return true;
      if (anim.currentProcess == ANIM_PROCESS_NORMAL)
        return true;
    }
    else if (anim.type == -animType)
    {
      if (anim.queuedProcess == ANIM_PROCESS_REVERSE)
        return true;
      if (anim.currentProcess == ANIM_PROCESS_REVERSE)
        return true;
    }
  }
  return false;
}

DWORD CGUIControl::GetNextControl(int direction) const
{
  switch (direction)
  {
  case ACTION_MOVE_UP:
    return m_dwControlUp;
  case ACTION_MOVE_DOWN:
    return m_dwControlDown;
  case ACTION_MOVE_LEFT:
    return m_dwControlLeft;
  case ACTION_MOVE_RIGHT:
    return m_dwControlRight;
  default:
    return -1;
  }
}

bool CGUIControl::CanFocusFromPoint(float posX, float posY, CGUIControl **control) const
{
  if (CanFocus() && HitTest(posX, posY))
  {
    *control = (CGUIControl *)this;
    return true;
  }
  *control = NULL;
  return false;
}

bool CGUIControl::HasID(DWORD dwID) const
{
  return GetID() == dwID;
}

bool CGUIControl::HasVisibleID(DWORD dwID) const
{
  return GetID() == dwID && IsVisible();
}

void CGUIControl::SaveStates(vector<CControlState> &states)
{
  // empty for now - do nothing with the majority of controls
}

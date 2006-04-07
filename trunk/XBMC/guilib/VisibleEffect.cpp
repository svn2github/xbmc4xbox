#include "include.h"
#include "VisibleEffect.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "SkinInfo.h" // for the effect time adjustments

CAnimation::CAnimation()
{
  Reset();
}

void CAnimation::Reset()
{
  type = ANIM_TYPE_NONE;
  effect = EFFECT_TYPE_NONE;
  currentState = ANIM_STATE_NONE;
  currentProcess = queuedProcess = ANIM_PROCESS_NONE;
  amount = 0;
  delay = start = length = 0;
  startX = startY = endX = endY = 0;
  startAlpha = 0;
  endAlpha = 100;
  acceleration = 0;
  condition = 0;
  reversible = true;
}

void CAnimation::Create(const TiXmlElement *node, RESOLUTION &res)
{
  if (!node || !node->FirstChild())
    return;
  const char *animType = node->FirstChild()->Value();
  if (strcmpi(animType, "visible") == 0)
    type = ANIM_TYPE_VISIBLE;
  else if (strcmpi(animType, "hidden") == 0)
    type = ANIM_TYPE_HIDDEN;
  else if (strcmpi(animType, "visiblechange") == 0)
    type = ANIM_TYPE_VISIBLE;
  else if (strcmpi(animType, "focus") == 0)
    type = ANIM_TYPE_FOCUS;
  else if (strcmpi(animType, "unfocus") == 0)
    type = ANIM_TYPE_UNFOCUS;
  else if (strcmpi(animType, "windowopen") == 0)
    type = ANIM_TYPE_WINDOW_OPEN;
  else if (strcmpi(animType, "windowclose") == 0)
    type = ANIM_TYPE_WINDOW_CLOSE;
  if (type == ANIM_TYPE_NONE)
  {
    CLog::Log(LOGERROR, "Control has invalid animation type");
    return;
  }
  const char *conditionString = node->Attribute("condition");
  if (conditionString)
    condition = g_infoManager.TranslateString(conditionString);
  const char *effectType = node->Attribute("effect");
  if (!effectType)
    return;
  // effect type
  if (strcmpi(effectType, "fade") == 0)
    effect = EFFECT_TYPE_FADE;
  else if (strcmpi(effectType, "slide") == 0)
    effect = EFFECT_TYPE_SLIDE;
  else if (strcmpi(effectType, "rotate") == 0)
    effect = EFFECT_TYPE_ROTATE;
  else if (strcmpi(effectType, "zoom") == 0)
    effect = EFFECT_TYPE_ZOOM;
  // time and delay
  node->Attribute("time", (int *)&length);
  node->Attribute("delay", (int *)&delay);
  length = (unsigned int)(length * g_SkinInfo.GetEffectsSlowdown());
  delay = (unsigned int)(delay * g_SkinInfo.GetEffectsSlowdown());
  // reversible (defaults to true)
  const char *reverse = node->Attribute("reversible");
  if (reverse && strcmpi(reverse, "false") == 0)
    reversible = false;
  // acceleration of effect
  double accel;
  if (node->Attribute("acceleration", &accel)) acceleration = (float)accel;
  // slide parameters
  if (effect == EFFECT_TYPE_SLIDE)
  {
    const char *startPos = node->Attribute("start");
    if (startPos)
    {
      startX = atoi(startPos);
      const char *comma = strstr(startPos, ",");
      if (comma)
        startY = atoi(comma + 1);
    }
    const char *endPos = node->Attribute("end");
    if (endPos)
    {
      endX = atoi(endPos);
      const char *comma = strstr(endPos, ",");
      if (comma)
        endY = atoi(comma + 1);
    }
    // scale our parameters
    g_graphicsContext.ScaleXCoord(startX, res);
    g_graphicsContext.ScaleYCoord(startY, res);
    g_graphicsContext.ScaleXCoord(endX, res);
    g_graphicsContext.ScaleYCoord(endY, res);
  }
  else if (effect == EFFECT_TYPE_FADE)
  {  // alpha parameters
    if (type < 0)
    { // out effect defaults
      startAlpha = 100;
      endAlpha = 0;
    }
    else
    { // in effect defaults
      startAlpha = 0;
      endAlpha = 100;
    }
    if (node->Attribute("start")) node->Attribute("start", &startAlpha);
    if (node->Attribute("end")) node->Attribute("end", &endAlpha);
    if (startAlpha > 100) startAlpha = 100;
    if (endAlpha > 100) endAlpha = 100;
    if (startAlpha < 0) startAlpha = 0;
    if (endAlpha < 0) endAlpha = 0;
  }
  else if (effect == EFFECT_TYPE_ROTATE)
  {
    if (node->Attribute("start")) node->Attribute("start", &startX);
    if (node->Attribute("end")) node->Attribute("end", &endX);

    // convert to a negative to account for our reversed vertical axis
    startX *= -1;
    endX *= -1;

    const char *centerPos = node->Attribute("center");
    if (centerPos)
    {
      startY = atoi(centerPos);
      const char *comma = strstr(centerPos, ",");
      if (comma)
        endY = atoi(comma + 1);
    }
  }
  else // if (effect == EFFECT_TYPE_ZOOM)
  {
    // effect defaults
    startX = 100;
    endX = 100;

    if (node->Attribute("start")) node->Attribute("start", &startX);
    if (node->Attribute("end")) node->Attribute("end", &endX);

    const char *centerPos = node->Attribute("center");
    if (centerPos)
    {
      startY = atoi(centerPos);
      const char *comma = strstr(centerPos, ",");
      if (comma)
        endY = atoi(comma + 1);
    }
  }
}

// creates the reverse animation
void CAnimation::CreateReverse(const CAnimation &anim)
{
  acceleration = -anim.acceleration;
  startX = anim.endX;
  startY = anim.endY;
  endX = anim.startX;
  endY = anim.startY;
  endAlpha = anim.startAlpha;
  startAlpha = anim.endAlpha;
  type = (ANIMATION_TYPE)-anim.type;
  effect = anim.effect;
  length = anim.length;
  reversible = anim.reversible;
}

void CAnimation::Animate(unsigned int time, bool hasRendered)
{
  // First start any queued animations
  if (queuedProcess == ANIM_PROCESS_NORMAL)
  {
    if (currentProcess == ANIM_PROCESS_REVERSE)
      start = time - (int)(length * amount);  // reverse direction of effect
    else
      start = time;
    currentProcess = ANIM_PROCESS_NORMAL;
  }
  else if (queuedProcess == ANIM_PROCESS_REVERSE)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)
      start = time - (int)(length * (1 - amount)); // turn around direction of effect
    else
      start = time;
    currentProcess = ANIM_PROCESS_REVERSE;
  }
  // reset the queued state once we've rendered
  // Note that if we are delayed, then the resource may not have been allocated as yet
  // as it hasn't been rendered (is still invisible).  Ideally, the resource should
  // be allocated based on a visible state, rather than a bool on/off, then only rendered
  // if it's in the appropriate state (ie allow visible = NO, DELAYED, VISIBLE, and allocate
  // if it's not NO, render if it's VISIBLE)  The alternative, is to just always render
  // the control while it's in the DELAYED state (comes down to the definition of the states)
  if (hasRendered || queuedProcess == ANIM_PROCESS_REVERSE
      || (currentState == ANIM_STATE_DELAYED && type > 0))
    queuedProcess = ANIM_PROCESS_NONE;
  // Update our animation process
  if (currentProcess == ANIM_PROCESS_NORMAL)
  {
    if (time - start < delay)
    {
      amount = 0.0f;
      currentState = ANIM_STATE_DELAYED;
    }
    else if (time - start < length + delay)
    {
      amount = (float)(time - start - delay) / length;
      currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      amount = 1.0f;
      currentState = ANIM_STATE_APPLIED;
    }
  }
  else if (currentProcess == ANIM_PROCESS_REVERSE)
  {
    if (time - start < length)
    {
      amount = 1.0f - (float)(time - start) / length;
      currentState = ANIM_STATE_IN_PROCESS;
    }
    else
    {
      amount = 0.0f;
      currentState = ANIM_STATE_APPLIED;
    }
  }
}

#define DEGREE_TO_RADIAN 0.01745329f

TransformMatrix CAnimation::RenderAnimation()
{
  // If we have finished an animation, reset the animation state
  // We do this here (rather than in Animate()) as we need the
  // currentProcess information in the UpdateStates() function of the
  // window and control classes.
  if (currentState == ANIM_STATE_APPLIED)
    currentProcess = ANIM_PROCESS_NONE;
  // Now do the real animation
  if (currentProcess != ANIM_PROCESS_NONE || currentState == ANIM_STATE_APPLIED)
  {
    float offset = amount * (acceleration * amount + 1.0f - acceleration);
    if (effect == EFFECT_TYPE_FADE)
      return TransformMatrix::CreateFader(((float)(endAlpha - startAlpha) * amount + startAlpha) * 0.01f);
    else if (effect == EFFECT_TYPE_SLIDE)
    {
      return TransformMatrix::CreateTranslation((endX - startX)*offset + startX, (endY - startY)*offset + startY);
    }
    else if (effect == EFFECT_TYPE_ROTATE)
    {
      TransformMatrix translation1 = TransformMatrix::CreateTranslation((float)-startY, (float)-endY);
      TransformMatrix rotation = TransformMatrix::CreateRotation(((endX - startX)*offset + startX) * DEGREE_TO_RADIAN);
      TransformMatrix translation2 = TransformMatrix::CreateTranslation((float)startY, (float)endY);
      return translation2 * rotation * translation1;
    }
    else if (effect == EFFECT_TYPE_ZOOM)
    {
      // could be extended to different X and Y scalings reasonably easily
      float scaleX = ((endX - startX)*offset + startX) * 0.01f;
      TransformMatrix translation1 = TransformMatrix::CreateTranslation((float)-startY, (float)-endY);
      TransformMatrix scaler = TransformMatrix::CreateScaler(scaleX, scaleX);
      TransformMatrix translation2 = TransformMatrix::CreateTranslation((float)startY, (float)endY);
      return translation2 * scaler * translation1;
    }
  }
  return TransformMatrix();
}

void CAnimation::ResetAnimation()
{
  currentProcess = ANIM_PROCESS_NONE;
  queuedProcess = ANIM_PROCESS_NONE;
  currentState = ANIM_STATE_NONE;
}
#ifndef __TWEEN_H__
#define __TWEEN_H__

///////////////////////////////////////////////////////////////////////
// Tween.h
// A couple of tweening classes implemented in C++.
// ref: http://www.robertpenner.com/easing/
//
// Author: d4rk <d4rk@xboxmediacenter.com>
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// Current list of classes:
//
// LinearTweener
// QuadTweener
// CubicTweener
// SineTweener
// CircleTweener
// BackTweener
// BounceTweener
// ElasticTweener
//
///////////////////////////////////////////////////////////////////////

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum TweenerType
{
  EASE_IN,
  EASE_OUT,
  EASE_INOUT
};


class Tweener
{
public:
  Tweener(TweenerType tweenerType = EASE_OUT) { m_tweenerType = tweenerType; _ref=1; _scale=1.0; }
  virtual ~Tweener() {};
  
  void SetEasing(TweenerType type) { m_tweenerType = type; }
  void SetScale(float scale) { _scale = scale; }
  virtual float Tween(float time, float start, float change, float duration)=0;
  void Free() { _ref--; if (_ref==0) delete this; }
  void IncRef() { _ref++; }
  
protected:
  int _ref;
  float _scale;
  TweenerType m_tweenerType;
};


class LinearTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    return change * time / duration + start;
  }
};


class QuadTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
	return _scale * change * time * time + start;
	break;

      case EASE_OUT:
	return _scale * (-change) * (time) * (time - 2) + start;
	break;

      case EASE_INOUT:
	if (time/2 < 1)
	  return _scale * (change/2) * time * time + start;
	time--;
	return _scale * (-change/2) * (time * (time - 2) - 1) + start;
	break;
      }
    return _scale * change * (time /= duration) * time + start;
  }
};


class CubicTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
	return _scale * change * time * time * time + start;
	break;

      case EASE_OUT:
	time--;
	return _scale * change * (time * time * time + 1) + start;
	break;

      case EASE_INOUT:
	if (time/2 < 1)
	  return _scale * (change/2) * time * time * time + start;
	time-=2;
	return _scale * (change/2) * (time * time * time + 2) + start;
	break;
      }
    return _scale * change * time * time + start;
  }
};

class CircleTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
	return _scale * (-change) * ((double)sqrt(1 - time * time) - 1) + start;
	break;

      case EASE_OUT:
	time--;
	return _scale * change * (double)sqrt(1 - time * time) + start;
	break;

      case EASE_INOUT:
	if ((time / 2) < 1)
	  return _scale * (-change/2) * ((double)sqrt(1 - time * time) - 1) + start;
	time-=2;
	return _scale * change/2 * ((double)sqrt(1 - time * time) + 1) + start;
	break;
      }
    return _scale * change * (double)sqrt(1 - time * time) + start;
  }
};

class BackTweener : public Tweener
{
public:
  BackTweener(float s=1.70158) { _s=s; }

  virtual float Tween(float time, float start, float change, float duration)
  {
    float s = _s;
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
	return _scale * change * time * time * ((s + 1) * time - s) + start;
	break;

      case EASE_OUT:
	time--;
	return _scale * change * ((time-1) * time * ((s + 1) * time + s) + 1) + start;
	break;

      case EASE_INOUT:
	s*=(1.525);
	if ((time / 2) < 1)
	{
	  return _scale *  (change/2) * (time * time * ((s + 1) * time - s)) + start;
	}
	time-=2;
	return _scale * (change/2) * (time * time * ((s + 1) * time + s) + 2) + start;
	break;
      }
    return _scale * change * ((time-1) * time * ((s + 1) * time + s) + 1) + start;
  }

private:
  float _s;
  
};


class SineTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    time /= duration;
    switch (m_tweenerType)
      {
      case EASE_IN:
	return _scale * change * (1 - cos(time * M_PI / 2.0)) + start;
	break;

      case EASE_OUT:
	return _scale * change * sin(time * M_PI / 2.0) + start;
	break;

      case EASE_INOUT:
	return _scale * change/2 * (1 - cos(M_PI * time)) + start;
	break;
      }
    return _scale * (change/2) * (1 - cos(M_PI * time)) + start;
  }
};


class BounceTweener : public Tweener
{
public:
  virtual float Tween(float time, float start, float change, float duration)
  {
    switch (m_tweenerType)
      {
      case EASE_IN:
	return _scale * (change - easeOut(duration - time, 0, change, duration)) + start;
	break;

      case EASE_OUT:
	return _scale * easeOut(time, start, change, duration);
	break;

      case EASE_INOUT:
	if (time < duration/2) 
	  return _scale * (change - easeOut (duration - (time * 2), 0, change, duration) + start) * .5 + start;
	else 
	  return _scale * (easeOut (time * 2 - duration, 0, change, duration) * .5 + change * .5) + start;
	break;
      }

    return _scale * easeOut(time, start, change, duration);
  }

protected:
  float easeOut(float time, float start, float change, float duration)
  {
    time /= duration;
    if (time < (1/2.75)) {
      return  _scale * change * (7.5625 * time * time) + start;
    } else if (time < (2/2.75)) {
      time -= (1.5/2.75);
      return _scale * change * (7.5625 * time * time + .75) + start;
    } else if (time < (2.5/2.75)) {
      time -= (2.25/2.75);
      return _scale * change * (7.5625 * time * time + .9375) + start;
    } else {
      time -= (2.625/2.75);
      return _scale * change * (7.5625 * time * time + .984375) + start;
    }
  }
};


class ElasticTweener : public Tweener
{
public:
  ElasticTweener(float a=0.0, float p=0.0) { _a=a; _p=p; }

  virtual float Tween(float time, float start, float change, float duration)
  {
    switch (m_tweenerType)
      {
      case EASE_IN:
	return easeIn(time, start, change, duration);
	break;

      case EASE_OUT:
	return easeOut(time, start, change, duration);	
	break;

      case EASE_INOUT:
	return easeInOut(time, start, change, duration);	
	break;
      }
    return easeOut(time, start, change, duration);
  }

protected:
  float _a;
  float _p;

  float easeIn(float time, float start, float change, float duration)
  {    
    float s=0;
    float a=_a;
    float p=_p;

    if (time==0) 
      return start;  
    time /= duration;
    if (time==1) 
	return start + _scale *change;  
    if (!p) 
      p=duration*.3;
    if (!a || a < (double)fabs(change)) 
    { 
      a = change; 
      s = p / 4.0; 
    }
    else
    {
      s = p / (2 * M_PI) * asin (change / a);
    }
    time--;
    return _scale * (-(a * (double)pow(2, 10*time) * sin((time * duration - s) * (2 * M_PI) / p ))) + start;
  }

  float easeOut(float time, float start, float change, float duration)
  {
    float s=0;
    float a=_a;
    float p=_p;

    if (time==0) 
      return start;  
    time /= duration;
    if (time==1) 
	return start + _scale * change;  
    if (!p) 
      p=duration*.3;
    if (!a || a < (double)fabs(change)) 
    { 
      a = change; 
      s = p / 4.0; 
    }
    else
    {
      s = p / (2 * M_PI) * asin (change / a);
    }
    return _scale * (-(a * (double)pow(2, -10*time) * sin((time * duration - s) * (2 * M_PI) / p )) + change) + start;
  }

  float easeInOut(float time, float start, float change, float duration)
  {
    float s=0;
    float a=_a;
    float p=_p;

    if (time==0) 
      return start;  
    time /= duration;
    if (time/2==2) 
	return start + change;  
    if (!p) 
      p=duration*.3*1.5;
    if (!a || a < (double)fabs(change)) 
    { 
      a = change; 
      s = p / 4.0; 
    }
    else
    {
      s = p / (2 * M_PI) * asin (change / a);
    }

    time--;
    if (time < 1) 
    {
      return _scale * (-.5 * (a * (double)pow(2, 10 * (time)) * sin((time * duration - s) * (2 * M_PI) / p ))) + start;
    }
    return _scale * (a * (double)pow(2, -10 * (time)) * sin((time * duration-s) * (2 * M_PI) / p ) * .5 + change) + start;
  }
};



#endif // __TWEEN_H__

#pragma once
#include "stdstring.h"
#include "thread.h"
#include "../utils/event.h"
using namespace std;
#define MAX_ROWS 20

class ILCD : public CThread
{
public:
  ILCD();
  virtual ~ILCD(void);
  virtual void Initialize()=0;
  virtual void Stop()=0;
  virtual void SetLine(int iLine, const CStdString& strLine)=0;
  virtual void SetBackLight(int iLight)=0;
protected:
	virtual void		Process()=0;
};
extern ILCD* g_lcd;
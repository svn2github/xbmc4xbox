#pragma once
#include "guidialog.h"

class CGUIDialogMuteBug : public CGUIDialog
{
public:
  CGUIDialogMuteBug(void);
  virtual ~CGUIDialogMuteBug(void);
  virtual bool OnMessage(CGUIMessage& message);
protected:
};

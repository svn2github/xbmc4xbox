#pragma once
#include "GUIDialogBoxBase.h"
#include "IProgressCallback.h"

class CGUIDialogProgress :
      public CGUIDialogBoxBase, public IProgressCallback
{
public:
  CGUIDialogProgress(void);
  virtual ~CGUIDialogProgress(void);

  void StartModal(DWORD dwParentId);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void Progress();
  void ProgressKeys();
  bool IsCanceled() const { return m_bCanceled; }
  void SetPercentage(int iPercentage);
  void ShowProgressBar(bool bOnOff);
  void SetHeading(const string& strLine);
  void SetHeading(int iString);             // for convenience to lookup in strings.xml

  // Implements IProgressCallback
  virtual void SetProgressMax(int iMax);
  virtual void SetProgressAdvance(int nSteps=1);
  virtual bool Abort();

protected:
  bool m_bCanceled;
  string m_strHeading;

  int  m_iCurrent;
  int  m_iMax;
};

#pragma once
#include "GUIDialog.h"
#include "VideoInfoScanner.h"
#include "utils/CriticalSection.h"

class CGUIDialogVideoScan: public CGUIDialog, public IVideoInfoScannerObserver
{
public:
  CGUIDialogVideoScan(void);
  virtual ~CGUIDialogVideoScan(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  void StartScanning(const CStdString& strDirectory, const SScraperInfo& info, const SScanSettings& settings, bool bUpdateAll);
  bool IsScanning();
  void StopScanning();

  void UpdateState();
protected:
  int GetStateString();
  virtual void OnDirectoryChanged(const CStdString& strDirectory);
  virtual void OnDirectoryScanned(const CStdString& strDirectory);
  virtual void OnFinished();
  virtual void OnStateChanged(SCAN_STATE state);
  virtual void OnSetProgress(int currentItem, int itemCount);
  virtual void OnSetCurrentProgress(int currentItem, int itemCount);

  CVideoInfoScanner m_videoInfoScanner;
  SCAN_STATE m_ScanState;
  CStdString m_strCurrentDir;

  CCriticalSection m_critical;

  float m_fPercentDone;
  float m_fCurrentPercentDone;
  int m_currentItem;
  int m_itemCount;
};

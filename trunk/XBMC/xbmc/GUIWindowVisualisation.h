#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "visualizations/Visualisation.h"
#include "cores/IAudioCallback.h"
#include "utils/CriticalSection.h"

#define AUDIO_BUFFER_SIZE 1024	// MUST BE A POWER OF 2!!!
#define MAX_AUDIO_BUFFERS 32

#include <list>
using namespace std;
class CAudioBuffer
{
public:
	CAudioBuffer(int iSize);
	virtual ~CAudioBuffer();
	const short* Get() const;
	void Set(const short* psBuffer, int iSize);
private:
	CAudioBuffer();
	short* m_pBuffer;
	int    m_iLen;
};

class CGUIWindowVisualisation :
  public CGUIWindow, public IAudioCallback
{
public:
  CGUIWindowVisualisation(void);
  virtual ~CGUIWindowVisualisation(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
	virtual void		Render();
	virtual void		OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
	virtual void		OnAudioData(const unsigned char* pAudioData, int iAudioDataLength);

private:
	void						CreateBuffers();
	void						ClearBuffers();
	CVisualisation* m_pVisualisation;

	int								m_iChannels;
	int								m_iSamplesPerSec;
	int								m_iBitsPerSample;
	list<CAudioBuffer*>	m_vecBuffers;
	int								m_iNumBuffers;								// Number of Audio buffers
	bool							m_bWantsFreq;
	float							m_fFreq[2*AUDIO_BUFFER_SIZE];									// Frequency data
	bool							m_bCalculate_Freq;							// True if the vis wants freq data
	bool							m_bInitialized;
	CCriticalSection	m_critSection;
};

/*!
	\file GUIEditControl.h
	\brief 
	*/

#ifndef GUILIB_GUIEditControl_H
#define GUILIB_GUIEditControl_H

#pragma once
#include "gui3D.h"
#include "guiControl.h"
#include "guiLabelControl.h"
#include "guiMessage.h"
#include "guiFont.h"
#include "StdString.h"
using namespace std;

/*!
	\ingroup controls
	\brief 
	*/

class IEditControlObserver
{
	public:
		virtual void OnEditTextComplete(CStdString& strLineOfText)=0;
};

class CGUIEditControl : public CGUILabelControl
{
public:
	CGUIEditControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY,
	  DWORD dwWidth, DWORD dwHeight, const CStdString& strFont, const wstring& strLabel,
	  DWORD dwTextColor, DWORD dwDisabledColor );

	virtual ~CGUIEditControl(void);

	virtual void SetObserver(IEditControlObserver* aObserver);
	virtual void OnKeyPress(WORD wKeyId);

protected:
	IEditControlObserver* m_pObserver;
};
#endif

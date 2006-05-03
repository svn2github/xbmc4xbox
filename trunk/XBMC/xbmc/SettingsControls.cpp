#include "stdafx.h"
#include "SettingsControls.h"
#include "GUIDialogNumeric.h"
#include "Util.h"

CBaseSettingControl::CBaseSettingControl(DWORD dwID, CSetting *pSetting)
{
  m_dwID = dwID;
  m_pSetting = pSetting;
}

CRadioButtonSettingControl::CRadioButtonSettingControl(CGUIRadioButtonControl *pRadioButton, DWORD dwID, CSetting *pSetting)
    : CBaseSettingControl(dwID, pSetting)
{
  m_pRadioButton = pRadioButton;
  m_pRadioButton->SetID(dwID);
  Update();
}

CRadioButtonSettingControl::~CRadioButtonSettingControl()
{}

void CRadioButtonSettingControl::OnClick()
{
  ((CSettingBool *)m_pSetting)->SetData(!((CSettingBool *)m_pSetting)->GetData());
}

void CRadioButtonSettingControl::Update()
{
  m_pRadioButton->SetSelected(((CSettingBool *)m_pSetting)->GetData());
}

CSpinExSettingControl::CSpinExSettingControl(CGUISpinControlEx *pSpin, DWORD dwID, CSetting *pSetting)
    : CBaseSettingControl(dwID, pSetting)
{
  m_pSpin = pSpin;
  m_pSpin->SetID(dwID);
  if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT)
  {
    CSettingFloat *pSettingFloat = (CSettingFloat *)pSetting;
    m_pSpin->SetType(SPIN_CONTROL_TYPE_FLOAT);
    m_pSpin->SetFloatRange(pSettingFloat->m_fMin, pSettingFloat->m_fMax);
    m_pSpin->SetFloatInterval(pSettingFloat->m_fStep);
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSetting;
    m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);
    m_pSpin->Clear();
    CStdString strLabel;
    int i = pSettingInt->m_iMin;
    if (pSettingInt->m_iLabelMin>-1)
    {
      strLabel=g_localizeStrings.Get(pSettingInt->m_iLabelMin);
      m_pSpin->AddLabel(strLabel, pSettingInt->m_iMin);
      i += pSettingInt->m_iStep;
    }
    for (; i <= pSettingInt->m_iMax; i += pSettingInt->m_iStep)
    {
      if (pSettingInt->m_iFormat > -1)
      {
        CStdString strFormat = g_localizeStrings.Get(pSettingInt->m_iFormat);
        strLabel.Format(strFormat, i);
      }
      else
        strLabel.Format(pSettingInt->m_strFormat, i);
      m_pSpin->AddLabel(strLabel, i);
    }
  }
  else // if (pSetting->GetControlType() == SPIN_CONTROL_TEXT)
  {
    m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);
    m_pSpin->Clear();
  }
  Update();
}

CSpinExSettingControl::~CSpinExSettingControl()
{}

void CSpinExSettingControl::OnClick()
{
  if (m_pSetting->GetControlType() == SPIN_CONTROL_FLOAT)
    ((CSettingFloat *)m_pSetting)->SetData(m_pSpin->GetFloatValue());
  else
  {
    if (m_pSetting->GetType() != SETTINGS_TYPE_STRING)
    {
      CSettingInt *pSettingInt = (CSettingInt *)m_pSetting;
      pSettingInt->SetData(m_pSpin->GetValue());
    }
  }
}

void CSpinExSettingControl::Update()
{
  if (m_pSetting->GetControlType() == SPIN_CONTROL_FLOAT)
  {
    CSettingFloat *pSettingFloat = (CSettingFloat *)m_pSetting;
    m_pSpin->SetFloatValue(pSettingFloat->GetData());
  }
  else if (m_pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || m_pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    CSettingInt *pSettingInt = (CSettingInt *)m_pSetting;
    m_pSpin->SetValue(pSettingInt->GetData());
  }
}

CButtonSettingControl::CButtonSettingControl(CGUIButtonControl *pButton, DWORD dwID, CSetting *pSetting)
    : CBaseSettingControl(dwID, pSetting)
{
  m_pButton = pButton;
  m_pButton->SetID(dwID);
  Update();
}

CButtonSettingControl::~CButtonSettingControl()
{}

void CButtonSettingControl::OnClick()
{
  // grab the onscreen keyboard
  CStdString keyboardInput(((CSettingString *)m_pSetting)->GetData());
  CStdString heading;
  if (((CSettingString *)m_pSetting)->m_iHeadingString > 0)
    heading = g_localizeStrings.Get(((CSettingString *)m_pSetting)->m_iHeadingString);
  if (m_pSetting->GetControlType() == BUTTON_CONTROL_INPUT)
  {
    if (!CGUIDialogKeyboard::ShowAndGetInput(keyboardInput, heading, ((CSettingString *)m_pSetting)->m_bAllowEmpty))
      return ;
  }
  if (m_pSetting->GetControlType() == BUTTON_CONTROL_HIDDEN_INPUT)
  {
    if (!CGUIDialogKeyboard::ShowAndGetNewPassword(keyboardInput, heading, ((CSettingString *)m_pSetting)->m_bAllowEmpty))
      return ;
  }
  if (m_pSetting->GetControlType() == BUTTON_CONTROL_IP_INPUT)
  {
    if (!CGUIDialogNumeric::ShowAndGetIPAddress(keyboardInput, g_localizeStrings.Get(14068)))
      return;
  }
  ((CSettingString *)m_pSetting)->SetData(keyboardInput);
  Update();
}

void CButtonSettingControl::Update()
{
  if (m_pSetting->GetControlType() == BUTTON_CONTROL_STANDARD)
    return ;
  CStdString strText = ((CSettingString *)m_pSetting)->GetData();
  if (m_pSetting->GetControlType() == BUTTON_CONTROL_HIDDEN_INPUT)
  {
    int iNumChars = strText.size();
    strText.Empty();
    for (int i = 0; i < iNumChars; i++)
      strText += '*';
  }
  if (m_pSetting->GetControlType() == BUTTON_CONTROL_PATH_INPUT)
  {
    CStdString shortPath;
    if (CUtil::MakeShortenPath(strText, shortPath, 30 ))
      strText = shortPath;
  }
  m_pButton->SetLabel2(strText);
}


bool CButtonSettingControl::IsValidIPAddress(const CStdString &strIP)
{
  const char* s = strIP.c_str();
  bool legalFormat = true;
  bool numSet = false;
  int num = 0;
  int dots = 0;

  while (*s != '\0')
  {
    if (*s == '.')
    {
      ++dots;

      // There must be a number before a .
      if (!numSet)
      {
        legalFormat = false;
        break;
      }

      if (num > 255)
      {
        legalFormat = false;
        break;
      }

      num = 0;
      numSet = false;
    }
    else if (*s >= '0' && *s <= '9')
    {
      num = (num * 10) + (*s - '0');
      numSet = true;
    }
    else
    {
      legalFormat = false;
      break;
    }

    ++s;
  }

  if (legalFormat)
  {
    if (!numSet)
    {
      legalFormat = false;
    }

    if (num > 255 || dots != 3)
    {
      legalFormat = false;
    }
  }

  if (!legalFormat)
    CGUIDialogOK::ShowAndGetInput(257, 724, 725, 0);

  return legalFormat;
}

CSeparatorSettingControl::CSeparatorSettingControl(CGUIImage *pImage, DWORD dwID, CSetting *pSetting)
    : CBaseSettingControl(dwID, pSetting)
{
  m_pImage = pImage;
  m_pImage->SetID(dwID);
}

CSeparatorSettingControl::~CSeparatorSettingControl()
{}
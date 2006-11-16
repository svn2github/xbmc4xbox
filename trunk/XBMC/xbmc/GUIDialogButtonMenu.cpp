#include "stdafx.h"
#include "GUIDialogButtonMenu.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"


#define CONTROL_BUTTON_LABEL  3100

CGUIDialogButtonMenu::CGUIDialogButtonMenu(void)
    : CGUIDialog(WINDOW_DIALOG_BUTTON_MENU, "DialogButtonMenu.xml")
{
}

CGUIDialogButtonMenu::~CGUIDialogButtonMenu(void)
{}

bool CGUIDialogButtonMenu::OnMessage(CGUIMessage &message)
{
  bool bRet = CGUIDialog::OnMessage(message);
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    // someone has been clicked - deinit...
    Close();
    return true;
  }
  return bRet;
}

void CGUIDialogButtonMenu::Render()
{
  // get the label control
  CGUILabelControl *pLabel = (CGUILabelControl *)GetControl(CONTROL_BUTTON_LABEL);
  if (pLabel)
  {
    // get the active window, and put it's label into the label control
    const CGUIControl *pControl = GetFocusedControl();
    if (pControl && (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON || pControl->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON))
    {
      CGUIButtonControl *pButton = (CGUIButtonControl *)pControl;
      pLabel->SetLabel(pButton->GetLabel());
    }
  }
  CGUIDialog::Render();
}

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUICheckMarkControl.h"
#include "utils/CharsetConverter.h"
#include "GUIFontManager.h"
#include "Key.h"

using namespace std;

CGUICheckMarkControl::CGUICheckMarkControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureCheckMark, const CTextureInfo& textureCheckMarkNF, float checkWidth, float checkHeight, const CLabelInfo &labelInfo)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_imgCheckMark(posX, posY, checkWidth, checkHeight, textureCheckMark)
    , m_imgCheckMarkNoFocus(posX, posY, checkWidth, checkHeight, textureCheckMarkNF)
    , m_textLayout(labelInfo.font, false)
{
  m_strLabel = "";
  m_label = labelInfo;
  m_bSelected = false;
  ControlType = GUICONTROL_CHECKMARK;
}

CGUICheckMarkControl::~CGUICheckMarkControl(void)
{}

void CGUICheckMarkControl::Render()
{
  m_textLayout.Update(m_strLabel);

  float fTextHeight, fTextWidth;
  m_textLayout.GetTextExtent(fTextWidth, fTextHeight);
  m_width = fTextWidth + 5 + m_imgCheckMark.GetWidth();
  m_height = m_imgCheckMark.GetHeight();

  float textPosX = m_posX;
  float textPosY = m_posY + m_height * 0.5f;
  float checkMarkPosX = m_posX;

  if (m_label.align & (XBFONT_RIGHT | XBFONT_CENTER_X))
    textPosX += m_imgCheckMark.GetWidth() + 5;
  else
    checkMarkPosX += fTextWidth + 5;

  if (IsDisabled() )
    m_textLayout.Render(textPosX, textPosY, 0, m_label.disabledColor, m_label.shadowColor, XBFONT_CENTER_Y, 0, true);
  else if (HasFocus() && m_label.focusedColor)
    m_textLayout.Render(textPosX, textPosY, 0, m_label.focusedColor, m_label.shadowColor, XBFONT_CENTER_Y, 0);
  else
    m_textLayout.Render(textPosX, textPosY, 0, m_label.textColor, m_label.shadowColor, XBFONT_CENTER_Y, 0);

  if (m_bSelected)
  {
    m_imgCheckMark.SetPosition(checkMarkPosX, m_posY);
    m_imgCheckMark.Render();
  }
  else
  {
    m_imgCheckMarkNoFocus.SetPosition(checkMarkPosX, m_posY);
    m_imgCheckMarkNoFocus.Render();
  }
  CGUIControl::Render();
}

bool CGUICheckMarkControl::OnAction(const CAction &action)
{
  if (action.actionId == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
    CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.actionId);
    SendWindowMessage(msg);
    return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUICheckMarkControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_strLabel = message.GetLabel();
      return true;
    }
  }
  if (CGUIControl::OnMessage(message)) return true;
  return false;
}

void CGUICheckMarkControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_imgCheckMark.AllocResources();
  m_imgCheckMarkNoFocus.AllocResources();
}

void CGUICheckMarkControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgCheckMark.FreeResources();
  m_imgCheckMarkNoFocus.FreeResources();
}

void CGUICheckMarkControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgCheckMark.DynamicResourceAlloc(bOnOff);
  m_imgCheckMarkNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUICheckMarkControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_imgCheckMark.SetInvalid();
  m_imgCheckMarkNoFocus.SetInvalid();
}

void CGUICheckMarkControl::SetSelected(bool bOnOff)
{
  m_bSelected = bOnOff;
}

bool CGUICheckMarkControl::GetSelected() const
{
  return m_bSelected;
}

bool CGUICheckMarkControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
  CAction action;
    action.actionId = ACTION_SELECT_ITEM;
  OnAction(action);
  return true;
}
  return false;
}

void CGUICheckMarkControl::SetLabel(const string &label)
{
  m_strLabel = label;
}

void CGUICheckMarkControl::PythonSetLabel(const CStdString &strFont, const string &strText, color_t textColor)
{
  m_label.font = g_fontManager.GetFont(strFont);
  m_label.textColor = textColor;
  m_label.focusedColor = textColor;
  m_strLabel = strText;
}

void CGUICheckMarkControl::PythonSetDisabledColor(color_t disabledColor)
{
  m_label.disabledColor = disabledColor;
}

void CGUICheckMarkControl::UpdateColors()
{
  m_label.UpdateColors();
  CGUIControl::UpdateColors();
  m_imgCheckMark.SetDiffuseColor(m_diffuseColor);
  m_imgCheckMarkNoFocus.SetDiffuseColor(m_diffuseColor);
}

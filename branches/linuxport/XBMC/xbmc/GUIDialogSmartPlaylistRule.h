#pragma once
#include "GUIDialog.h"
#include "SmartPlaylist.h"

class CGUIDialogSmartPlaylistRule :
      public CGUIDialog
{
public:
  CGUIDialogSmartPlaylistRule(void);
  virtual ~CGUIDialogSmartPlaylistRule(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnInitWindow();

  static bool EditRule(CSmartPlaylistRule &rule);

protected:
  void OnValue();
  void OnField();
  void OnOperator();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  void AddOperatorLabel(CSmartPlaylistRule::SEARCH_OPERATOR op);

  enum FIELD { TEXT_FIELD, NUMERIC_FIELD, DATE_FIELD, PLAYLIST_FIELD };
  FIELD GetFieldType(CSmartPlaylistRule::DATABASE_FIELD field);

  CSmartPlaylistRule m_rule;
  bool m_cancelled;
};

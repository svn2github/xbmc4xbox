#pragma once
#include "guiImage.h"
#include "guiwindow.h"
#include "guiList.h"
#include "TinyXML/TinyXML.h"
#include "StdString.h"

#include "utils/UdpClient.h"
#include "guiDialogProgress.h"
#include "utils/KaiClient.h"

#include "BuddyItem.h"
#include "ArenaItem.h"

using namespace std;
#include <map>

// TODO: OnEnterArenaFailed, may incorrectly prompt user for password when not needed.

class CGUIWindowBuddies : 	public CGUIWindow, public IBuddyObserver
{
public:
	CGUIWindowBuddies(void);
	virtual ~CGUIWindowBuddies(void);

	virtual void			OnInitialise(CKaiClient* pClient);
	virtual void			OnContactOffline(CStdString& aFriend);
	virtual void			OnContactOnline(CStdString& aFriend);
	virtual void			OnContactPing(CStdString& aFriend, CStdString& aVector, DWORD aPing, int aStatus, CStdString& aBearerCapability);
	virtual void			OnContactRemove(CStdString& aFriend);
	virtual void			OnContactSpeexStatus(CStdString& aFriend, bool bSpeexEnabled);
	virtual void			OnContactSpeexRing(CStdString& aFriend);
	virtual void			OnContactSpeex(CStdString& aFriend);
	virtual void			OnContactInvite(CStdString& aFriend, CStdString& aVector, CStdString& aTime, CStdString& aMessage);
	virtual void			OnEnterArena(CStdString& aVector, BOOL bCanHost);
	virtual void			OnEnterArenaFailed(CStdString& aVector, CStdString& aReason);
	virtual void			OnNewArena( CStdString& aVector, CStdString& aDescription,
										int nPlayers, int nPlayerLimit, int nPassword, bool bPersonal );
	virtual void			OnUpdateArena( CStdString& aVector, int nPlayers );
	virtual void			OnUpdateOpponent(CStdString& aOpponent, CStdString& aAge,
										CStdString& aBandwidth,	CStdString& aLocation, CStdString& aBio);
	virtual void			OnUpdateOpponent(CStdString& aOpponent, CStdString& aAvatarURL);
	virtual void			OnOpponentEnter(CStdString& aOpponent);
	virtual void			OnOpponentPing(CStdString& aOpponent, DWORD aPing, int aStatus, CStdString& aBearerCapability);
	virtual void			OnOpponentLeave(CStdString& aOpponent);
	virtual void			OnSupportedTitle(DWORD aTitleId, CStdString& aVector);

	virtual void			OnInitWindow();
	virtual void			OnAction(const CAction &action);
	virtual void			Render();

protected:
	enum State {Uninitialized,Buddies,Games,Arenas};

	void OnClickModeButton	(CGUIMessage& aMessage);
	void OnClickAddButton	(CGUIMessage& aMessage);
	void OnClickRemoveButton(CGUIMessage& aMessage);
	void OnClickSpeexButton (CGUIMessage& aMessage);
	void OnClickInviteButton(CGUIMessage& aMessage);
	void OnClickJoinButton	(CGUIMessage& aMessage);
	void OnClickPlayButton	(CGUIMessage& aMessage);
	void OnClickHostButton	(CGUIMessage& aMessage);
	void OnClickListItem	(CGUIMessage& aMessage);
	void OnSelectListItem	(CGUIMessage& aMessage);

	void	QueryInstalledGames();
	void	UpdatePanel();
	void	UpdateFriends();
	void	UpdateArena();

	void	UpdateGamesPlayerCount();

	void	ChangeState();
	void	ChangeState(State aNewState);

	void	Enter(CArenaItem& aArena);
	void	Play(CStdString& aVector);

	static bool	SortFriends(CGUIItem* pStart, CGUIItem* pEnd);
	static bool	SortArenaItems(CGUIItem* pStart, CGUIItem* pEnd);

	CBuddyItem* GetBuddySelection();
	CArenaItem* GetArenaSelection();
	CGUIImage*	GetCurrentAvatar();

	struct Invitation
	{
		CStdString vector;
		CStdString time;
		CStdString message;
	};

	CGUISortedList			m_friends;
	CGUISortedList			m_arena;
	CGUISortedList			m_games;
	State window_state;

	CGUIImage*	m_pOpponentImage;
	CGUIImage*  m_pCurrentAvatar;

	CKaiClient* m_pKaiClient;

	DWORD		m_dwGamesUpdateTimer;

	typedef map<CStdString,Invitation> INVITETABLE;
	INVITETABLE m_invitations;
};

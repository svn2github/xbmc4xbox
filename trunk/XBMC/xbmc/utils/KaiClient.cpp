#include "stdafx.h"
#include "KaiClient.h"
#include "Log.h"
#include "../Settings.h"
#include "../Application.h"
#include "../Crc32.h"
#include "dsstdfx.h"

CKaiClient* CKaiClient::client = NULL;

//#define SPEEX_LOOPBACK 1
#define	KAI_CONTACT_SETTLING_PERIOD		5000L

#ifdef SPEEX_LOOPBACK
DWORD g_speexLoopbackPlayerId = 0xDEADBEEF;
#endif

void CKaiClient::QueryVector(DWORD aTitleId)
{
	if (client_state==State::Authenticated)
	{
		CStdString strResolveMessage;
		CStdString strQuestion;
		strQuestion.Format("%x",aTitleId);
		strResolveMessage.Format("KAI_CLIENT_APP_SPECIFIC;XBE_0x%s;",strQuestion.ToUpper().c_str());
		Send(server_addr, strResolveMessage);
	}
}

void CKaiClient::QueryVectorPlayerCount(CStdString& aVector)
{
	if (client_state==State::Authenticated)
	{
		CStdString strQueryMessage;
		strQueryMessage.Format("KAI_CLIENT_SPECIFIC_COUNT;%s;",aVector.c_str());
		Send(server_addr, strQueryMessage);
//		OutputDebugString(strQueryMessage.c_str());
//		OutputDebugString("\r\n");
	}
}

CKaiClient::CKaiClient(void) : CUdpClient()
{
	CLog::Log(LOGINFO, "KAICLIENT: Instantiating...");
	observer = NULL;
	m_bHosting	= FALSE;
	m_bContactsSettling = TRUE;
	m_nFriendsOnline = 0;
	m_pDSound	= NULL;

	// outbound packet queue, collects compressed audio until sufficient to send
	CStdString strEgress = "egress";
	m_pEgress  = new CMediaPacketQueue(strEgress);

	Create();

	CLog::Log(LOGINFO, "KAICLIENT: Ready.");
}

void CKaiClient::VoiceChatStart()
{
	if(!m_pDSound)
	{
		CLog::Log(LOGINFO, "KAICLIENT: Initializing DirectSound...");
		if (FAILED(DirectSoundCreate( NULL, &m_pDSound, NULL )))
		{
			CLog::Log(LOGERROR, "KAICLIENT: Failed to initialize DirectSound.");
		}

		DSEFFECTIMAGELOC dsImageLoc;
		dsImageLoc.dwI3DL2ReverbIndex = GraphI3DL2_I3DL2Reverb;
		dsImageLoc.dwCrosstalkIndex = GraphXTalk_XTalk;

		CLog::Log(LOGINFO, "KAICLIENT: Loading sound effects image...");
		LPDSEFFECTIMAGEDESC pdsImageDesc;
		if (FAILED(XAudioDownloadEffectsImage( "dsstdfx", &dsImageLoc, XAUDIO_DOWNLOADFX_XBESECTION, &pdsImageDesc )))
		{
			CLog::Log(LOGERROR, "KAICLIENT: Failed to load image.");
		}

		// Configure the voice manager.
		VOICE_MANAGER_CONFIG VoiceConfig;
		VoiceConfig.dwVoicePacketTime       = 20;      // 20ms per microphone callback
		VoiceConfig.dwMaxRemotePlayers      = 12;      // 12 remote players
		VoiceConfig.dwFirstSRCEffectIndex   = GraphVoice_Voice_0;
		VoiceConfig.pEffectImageDesc        = pdsImageDesc;
		VoiceConfig.pDSound                 = m_pDSound;
		VoiceConfig.dwMaxStoredPackets      = MAX_VOICE_PER_SEND;

		VoiceConfig.pCallbackContext        = this;
		VoiceConfig.pfnCommunicatorCallback = CommunicatorCallback;
		VoiceConfig.pfnVoiceDataCallback    = VoiceDataCallback;

		CLog::Log(LOGNOTICE, "KAICLIENT: Initializing voice manager...");
		if (FAILED(g_VoiceManager.Initialize( &VoiceConfig )))
		{
			CLog::Log(LOGERROR, "KAICLIENT: Failed to initialize voice manager.");
		}

		CLog::Log(LOGINFO, "KAICLIENT: Voice chat enabled.");

		g_VoiceManager.EnterChatSession();
		m_VoiceTimer.StartZero();

		#ifdef SPEEX_LOOPBACK
		g_VoiceManager.AddChatter(g_speexLoopbackPlayerId);
		#endif
	}
}

void CKaiClient::VoiceChatStop()
{
	if(m_pDSound)
	{
		CLog::Log(LOGINFO, "KAICLIENT: Releasing DirectSound...");
		g_VoiceManager.LeaveChatSession();
// TODO: we really ought to shutdown, but somethings up - I can never seem to reinit properly.
//		g_VoiceManager.Shutdown();
		m_pDSound->Release();
		m_pDSound=NULL;
		m_pEgress->Flush();
		CLog::Log(LOGINFO, "KAICLIENT: Voice chat disabled.");
	}
}

CKaiClient::~CKaiClient(void)
{
	client = NULL;
}

CKaiClient* CKaiClient::GetInstance()
{
	if (client==NULL)
	{
		client = new CKaiClient();
	}

	return client;
}

void CKaiClient::SetObserver(IBuddyObserver* aObserver)
{
	if (observer != aObserver)
	{
		// Enable call back methods
		observer = aObserver;
		m_dwTimer = timeGetTime();

		aObserver->OnInitialise(this);

		// Discover location of KAI engine
		Discover();
	}
}
void CKaiClient::RemoveObserver()
{
	if (observer)
	{
		observer = NULL;
		Detach();
	}
}

void CKaiClient::EnterVector(CStdString& aVector, CStdString& aPassword)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strVectorMessage;
		strVectorMessage.Format("KAI_CLIENT_VECTOR;%s;%s;",aVector,aPassword);

		Send(server_addr, strVectorMessage);
	}
}

void CKaiClient::AddContact(CStdString& aContact)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strVectorMessage;
		strVectorMessage.Format("KAI_CLIENT_ADD_CONTACT;%s;",aContact);
		Send(server_addr, strVectorMessage);
	}
}

void CKaiClient::RemoveContact(CStdString& aContact)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strVectorMessage;
		strVectorMessage.Format("KAI_CLIENT_REMOVE_CONTACT;%s;",aContact);
		Send(server_addr, strVectorMessage);
	}
}

void CKaiClient::Invite(CStdString& aPlayer, CStdString& aVector, CStdString& aMessage)
{
	if (client_state==State::Authenticated)
	{
		CStdString strInvitationMessage;
		strInvitationMessage.Format("KAI_CLIENT_INVITE;%s;%s;%s;",aPlayer,aVector,aMessage);

		//OutputDebugString(strInvitationMessage.c_str());
		//OutputDebugString("\r\n");

		Send(server_addr, strInvitationMessage);
	}
}

void CKaiClient::Host(CStdString& aPassword, int aPlayerLimit, CStdString& aDescription)
{
	if (client_state==State::Authenticated)
	{
		CStdString strHostMessage;
		strHostMessage.Format("KAI_CLIENT_CREATE_VECTOR;%d;%s;%s;",aPlayerLimit,aDescription,aPassword);
		Send(server_addr, strHostMessage);
	}
}
void CKaiClient::SetHostingStatus(BOOL bIsHosting)
{
	if (client_state==State::Authenticated)
	{
		CStdString strStatusMessage;
		strStatusMessage.Format("KAI_CLIENT_ARENA_STATUS;%d;%d;",bIsHosting ? 1:0, 1);
		Send(server_addr, strStatusMessage);
	}
}

void CKaiClient::GetSubVectors(CStdString& aVector)
{
	if (client_state==State::Authenticated)
	{
		CStdString strGetSubArenasMessage;
		strGetSubArenasMessage.Format("KAI_CLIENT_GET_VECTORS;%s;",aVector);
		Send(server_addr, strGetSubArenasMessage);
	}
}

void CKaiClient::ExitVector()
{
	if (client_state==State::Authenticated)
	{
		INT vectorDelimiter = client_vector.ReverseFind('/');
		if (vectorDelimiter>0)
		{
			CStdString previousVector = client_vector.Mid(0,vectorDelimiter);
			CStdString strPassword = "";
			EnterVector(previousVector,strPassword);
		}
	}
}

CStdString CKaiClient::GetCurrentVector()
{
	return client_vector;
}


void CKaiClient::Discover()
{	
	client_state = State::Discovering;
	CStdString strInitiateDiscoveryMessage = "KAI_CLIENT_DISCOVER;";
	Broadcast(KAI_SYSTEM_PORT, strInitiateDiscoveryMessage);
}

void CKaiClient::Detach()
{	
	if (client_state==State::Authenticated)
	{
		CStdString strDisconnectionMessage = "KAI_CLIENT_DETACH;";
		client_state = State::Disconnected;
		Send(server_addr, strDisconnectionMessage);
		Broadcast(KAI_SYSTEM_PORT, strDisconnectionMessage);
	}
}


void CKaiClient::Reattach()
{	
	m_bHosting	= FALSE;
	m_bContactsSettling = TRUE;
	m_nFriendsOnline = 0;

	Discover();
}
void CKaiClient::Attach(SOCKADDR_IN& aAddress)
{	
	client_state = State::Attaching;
	CStdString strAttachMessage = "KAI_CLIENT_ATTACH;";
	server_addr = aAddress;
	Send(server_addr, strAttachMessage);
}

void CKaiClient::TakeOver()
{	
	client_state = State::Attaching;
	CStdString strTakeOverMessage = "KAI_CLIENT_TAKEOVER;";
	Send(server_addr, strTakeOverMessage);
}

void CKaiClient::Query()
{	
	client_state = State::Querying;
	CStdString strQueryMessage = "KAI_CLIENT_GETSTATE;";
	Send(server_addr, strQueryMessage);
}

void CKaiClient::Login(LPCSTR aUsername, LPCSTR aPassword)
{	
	client_state = State::LoggingIn;
	CStdString strLoginMessage;
	strLoginMessage.Format("KAI_CLIENT_LOGIN;%s;%s;",aUsername,aPassword);
	Send(server_addr, strLoginMessage);
}
bool CKaiClient::IsEngineConnected()
{
	return client_state == State::Authenticated;
}

void CKaiClient::QueryUserProfile(CStdString& aPlayerName)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strQueryMessage;
		strQueryMessage.Format("KAI_CLIENT_GET_PROFILE;%s;", aPlayerName.c_str());
		Send(server_addr, strQueryMessage);
	}
}

void CKaiClient::QueryAvatar(CStdString& aPlayerName)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strQueryMessage;
		strQueryMessage.Format("KAI_CLIENT_AVATAR;%s;", aPlayerName.c_str());
		Send(server_addr, strQueryMessage);
	}
}

void CKaiClient::SetBearerCaps(BOOL bIsHeadsetPresent)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strStatusMessage;
		strStatusMessage.Format("KAI_CLIENT_CAPS;01%s;", bIsHeadsetPresent ? "2":"" );
		Send(server_addr, strStatusMessage);
	}
}

void CKaiClient::EnableContactVoice(CStdString& aContactName, BOOL bEnable)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strVoiceMessage;
		strVoiceMessage.Format("KAI_CLIENT_SPEEX_%s;%s;", bEnable ? "ON":"OFF",
			aContactName.c_str());
		Send(server_addr, strVoiceMessage);
	}
}

void CKaiClient::QueueContactVoice(CStdString& aContactName, DWORD aPlayerId, LPBYTE pMessage, DWORD dwMessageLength)
{
	if (client_state==State::Authenticated)
	{
		CStdString header;
		header.Format("KAI_CLIENT_SPEEX;%s;",aContactName.c_str());

		int frames = 0;
		for(DWORD messageOffset = header.length(); messageOffset < dwMessageLength; messageOffset += COMPRESSED_FRAME_SIZE)
		{
			frames++;

			// we need to persist this buffer before we can have it queued
			g_VoiceManager.ReceivePacket(
				aPlayerId,
				(LPVOID) &pMessage[messageOffset],
				COMPRESSED_FRAME_SIZE ); 
		}

		//char szDebug[128];
		//sprintf(szDebug,"RX KAI SPEEX %d frames, total: %u bytes.\r\n",frames,dwMessageLength);
		//OutputDebugString(szDebug);
	}
}


void CKaiClient::SendVoiceDataToEngine()
{
	if (client_state==State::Authenticated)
	{
		if (!m_pEgress->IsEmpty())
		{
			CHAR* buffer= new CHAR[700];

			#ifndef SPEEX_LOOPBACK
			sprintf(buffer,"KAI_CLIENT_SPEEX;");
			#else
			sprintf(buffer,"KAI_CLIENT_SPEEX;SpeexLoopback;");
			#endif

			int nPackets = m_pEgress->Size();

			//char szDebug[128];
			//sprintf(szDebug,"Sending %d packets\r\n", nPackets);
			//OutputDebugString(szDebug);

			int messageOffset = strlen(buffer);
			for(int i=0; i<nPackets; i++, messageOffset+= COMPRESSED_FRAME_SIZE)
			{
				XMEDIAPACKET outboundPacket;
				m_pEgress->Read(outboundPacket);
				
				memcpy((LPVOID)&buffer[messageOffset],outboundPacket.pvBuffer,COMPRESSED_FRAME_SIZE);

				*outboundPacket.pdwStatus = XMEDIAPACKET_STATUS_SUCCESS;
			}

			#ifndef SPEEX_LOOPBACK
			Send(server_addr, (LPBYTE)buffer, messageOffset);
			#else
			CStdString strName = "SpeexLoopback";
			QueueContactVoice(strName,g_speexLoopbackPlayerId,(LPBYTE)buffer,messageOffset);
			#endif
		}
	}

	// reset buffer and stopwatch
	m_VoiceTimer.StartZero();
}




void CKaiClient::OnMessage(SOCKADDR_IN& aRemoteAddress, CStdString& aMessage, LPBYTE pMessage, DWORD dwMessageLength)
{
//	CLog::Log(aMessage);	
//	OutputDebugString("KAI: ");
//	OutputDebugString(aMessage.c_str());
//	OutputDebugString("\r\n");

	CHAR* szMessage = strtok( (char*)((LPCSTR)aMessage), ";"); 

	// now depending on state...
	switch (client_state)
	{
		case State::Discovering:
			if (strcmp(szMessage,"KAI_CLIENT_ENGINE_HERE")==0)
			{
				Attach(aRemoteAddress);
			}
			break;

		case State::Attaching:
			if (strcmp(szMessage,"KAI_CLIENT_ATTACH")==0)
			{
				Query();
			}
			else if (strcmp(szMessage,"KAI_CLIENT_ENGINE_IN_USE")==0)
			{
				TakeOver();
			}
			break;

		case State::Querying:
			if (strcmp(szMessage,"KAI_CLIENT_LOGGED_IN")==0)
			{
				client_state = State::Authenticated;
				m_dwTimer = timeGetTime();
			}
			else if (strcmp(szMessage,"KAI_CLIENT_NOT_LOGGED_IN")==0)
			{
				// has XBMC user set up KAI username and password?
				if (g_stSettings.szOnlineUsername[0]==0)
				{
					CStdString strDefaultUsername = strtok(NULL, ";");  
					CStdString strDefaultPassword = strtok(NULL, ";");  
					
					// has KAI engine provided us with a default username and password?
					if (strDefaultUsername.length()>0 && strDefaultPassword.length()>0)
					{
						strcpy(g_stSettings.szOnlineUsername,strDefaultUsername.c_str());
						strcpy(g_stSettings.szOnlinePassword,strDefaultPassword.c_str());

						// auto login with KAI credentials
						Login(g_stSettings.szOnlineUsername, g_stSettings.szOnlinePassword);
					}
				}
				else
				{
					// auto login with XBMC credentials
					Login(g_stSettings.szOnlineUsername, g_stSettings.szOnlinePassword);
				}
			}
			break;

		case State::LoggingIn:
			if (strcmp(szMessage,"KAI_CLIENT_USER_DATA")==0)
			{
				client_state = State::Authenticated;

				CStdString strCaseCorrectedName = strtok(NULL, ";");
				if (strCaseCorrectedName.length()>0)
				{
					sprintf(g_stSettings.szOnlineUsername,strCaseCorrectedName.c_str());
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_AUTHENTICATION_FAILED")==0)
			{
				if (observer!=NULL)
				{
					CStdString strUsername = g_stSettings.szOnlineUsername;
					observer->OnAuthenticationFailed(strUsername);
				}
			}
			break;

		case State::Authenticated:

			if ((strcmp(szMessage,"KAI_CLIENT_ADD_CONTACT")==0) || 
				(strcmp(szMessage,"KAI_CLIENT_CONTACT_OFFLINE")==0))

			{
				m_nFriendsOnline--;
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  

					observer->OnContactOffline(	strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_CONTACT_ONLINE")==0)
			{
				m_nFriendsOnline++;
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
				  
					observer->OnContactOnline( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SPEEX_ON")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  			  
					observer->OnContactSpeexStatus(strContactName,true);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SPEEX_OFF")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  			  
					observer->OnContactSpeexStatus(strContactName,false);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SPEEX_START")==0)
			{
				VoiceChatStart();
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SPEEX")==0)
			{
				CStdString strContactName = strtok(NULL, ";");

				if (m_pDSound)
				{
					DWORD playerId = Crc32FromString(strContactName);
					BOOL bRegistered = g_VoiceManager.IsPlayerRegistered( playerId );
					BOOL bAvailable = TRUE;

					if (!bRegistered)
					{
						//CStdString strDebug;
						//strDebug.Format("Adding chatter %s\r\n",strContactName.c_str());
						//OutputDebugString(strDebug.c_str());
						bAvailable = SUCCEEDED(g_VoiceManager.AddChatter(playerId));
					}
					if (bAvailable)
					{
						QueueContactVoice(strContactName,playerId,pMessage,dwMessageLength);
						if (observer!=NULL)
						{
							observer->OnContactSpeex(strContactName);
						}
					}
					else
					{
						CLog::Log(LOGERROR,"Failed to queue contact voice.");
					}
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SPEEX_STOP")==0)
			{
				VoiceChatStop();
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SPEEX_RING")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  				  
					observer->OnContactSpeexRing( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_CONTACT_PING")==0)
			{
				if (observer!=NULL)
				{
					CHAR* szContact = strtok(NULL, ";");
					CStdString strContactName = szContact;
					CStdString strVector = "Idle";

					if ( szContact[strlen(szContact)+1] != ';' )
					{
						strVector = strtok(NULL, ";");  
					}

					CStdString strPing = strtok(NULL, ";");
					CStdString strCaps = strtok(NULL, ";");

					observer->OnContactPing(strContactName, strVector, 
						strtoul(strPing.c_str(),NULL,10), 0, strCaps);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_REMOVE_CONTACT")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
				  
					observer->OnContactRemove( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_INVITE")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
					CStdString strVector = strtok(NULL, ";");  
					CStdString strTime = strtok(NULL, ";");  
					CStdString strMessage = strtok(NULL, ";"); 

					observer->OnContactInvite( strContactName, strVector, strTime, strMessage );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_VECTOR")==0)
			{
				CStdString strVector = strtok(NULL, ";"); 

				bool bInPrivateRoom = strVector.Find(g_stSettings.szOnlineUsername)>0;

				// check to see if we've left our private room and we're still hosting			
				if (m_bHosting)
				{
					if (!bInPrivateRoom)
					{
						// set our status to looking to join games as opposed to hosting them
						SetHostingStatus(FALSE);
					}
				}
				else
				{
					if (bInPrivateRoom)
					{
						// set our status to looking to host games as opposed to joining them
						SetHostingStatus(TRUE);
					}
				}

				CStdString strCanCreate = strtok(NULL, ";");  
				if (observer!=NULL)
				{
					client_vector = strVector;
					observer->OnEnterArena(strVector,atoi(strCanCreate.c_str()));
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SUB_VECTOR")==0)
			{
				CStdString strVector		= strtok(NULL, ";");
				CStdString strPlayers		= strtok(NULL, ";");
				CStdString strSubVectors	= strtok(NULL, ";");
				CStdString strIsPrivate		= strtok(NULL, ";");
				CStdString strPlayerLimit	= strtok(NULL, ";");
				CStdString strDescription	= "Public Arena";

				if (observer!=NULL)
				{
					observer->OnNewArena(	strVector,
											strDescription,
											atoi(strPlayers.c_str()),
											atoi(strPlayerLimit.c_str()),
											atoi(strIsPrivate.c_str()),
											false );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_ARENA_STATUS")==0)
			{
				CStdString strMode			= strtok(NULL, ";");
				m_bHosting = atoi(strMode.c_str()) >0;
			}

			else if (strcmp(szMessage,"KAI_CLIENT_USER_SUB_VECTOR")==0)
			{
				CStdString strVector		= strtok(NULL, ";");
				CStdString strPlayers		= strtok(NULL, ";");
				CStdString strSubVectors	= strtok(NULL, ";");
				CStdString strIsPrivate		= strtok(NULL, ";");
				CStdString strPlayerLimit	= strtok(NULL, ";");
				CStdString strDescription	= strtok(NULL, ";");

				if (observer!=NULL)
				{
					observer->OnNewArena(	strVector,
											strDescription,
											atoi(strPlayers.c_str()),
											atoi(strPlayerLimit.c_str()),
											atoi(strIsPrivate.c_str()),
											true );
				}
			}

			else if (strcmp(szMessage,"KAI_CLIENT_SUB_VECTOR_UPDATE")==0)
			{
				CStdString strVector		= strtok(NULL, ";");
				CStdString strPlayers		= strtok(NULL, ";");

				if (observer!=NULL)
				{
					observer->OnUpdateArena( strVector,
											 atoi(strPlayers.c_str()) );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_VECTOR_DISALLOWED")==0)
			{
				CStdString strVector		= strtok(NULL, ";");
				CStdString strReason		= strtok(NULL, ";");

				if (observer!=NULL)
				{
					observer->OnEnterArenaFailed( strVector, strReason );
				}
			}

			else if (strcmp(szMessage,"KAI_CLIENT_JOINS_VECTOR")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
			  
					observer->OnOpponentEnter( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_ARENA_PING")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");
					CStdString strPing = strtok(NULL, ";");
					CStdString strStatus = strtok(NULL, ";"); // 0 - idle, 1 - join, 2- host, 3-dedicated host
					CStdString strPlayers = strtok(NULL, ";");
					CStdString strCaps = strtok(NULL, ";");
					observer->OnOpponentPing(strContactName,
						strtoul(strPing.c_str(),NULL,10), atoi(strStatus.c_str()), strCaps);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_USER_PROFILE")==0)
			{
				if (observer!=NULL)
				{
					CStdString strOpponent  = strtok(NULL, ";");
					CStdString strAge		= strtok(NULL, ";");
					CStdString strBandwidth	= strtok(NULL, ";");
					CStdString strLocation	= strtok(NULL, ";");
					CStdString strXBOX		= strtok(NULL, ";");
					CStdString strGCN		= strtok(NULL, ";");
					CStdString strPS2		= strtok(NULL, ";");
					CStdString strBio		= strtok(NULL, ";");

					observer->OnUpdateOpponent(strOpponent, strAge, strBandwidth, strLocation, strBio );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_AVATAR")==0)
			{
				if (observer!=NULL)
				{
					CStdString strOpponent  = strtok(NULL, ";");
					CStdString strUrl		= strtok(NULL, ";");

					if (strUrl.length()>0)
					{
						observer->OnUpdateOpponent(strOpponent, strUrl);
					}
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_LEAVES_VECTOR")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
			  
					observer->OnOpponentLeave( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_APP_SPECIFIC")==0)
			{
				if (observer!=NULL)
				{
					CStdString strQuestion = strtok(NULL, ";"); 
					CStdString strAnswer = strtok(NULL, ";");  

					if (strQuestion.Find("XBE_")==0)
					{
						CStdString strTitleId = strQuestion.Mid(4);
						DWORD dwTitleId = strtoul(strTitleId.c_str(),NULL,16);
						observer->OnSupportedTitle(dwTitleId, strAnswer );
						QueryVectorPlayerCount(strAnswer);
					}
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SPECIFIC_COUNT")==0)
			{
				if (observer!=NULL)
				{
					CStdString strVector		= strtok(NULL, ";");
					CStdString strPlayers		= strtok(NULL, ";");
					observer->OnUpdateArena(strVector, atoi(strPlayers));
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_DETACH")==0)
			{
				client_state = State::Disconnected;
				if (observer!=NULL)
				{
					observer->OnEngineDetached();
				}
			}

			break;
	}
}

void CKaiClient::DoWork()
{
	// generate our own notifications
	if (observer!=NULL && client_state == State::Authenticated)
	{
		if (m_bContactsSettling && (timeGetTime() - m_dwTimer > KAI_CONTACT_SETTLING_PERIOD) )
		{
			m_bContactsSettling = FALSE;
			observer->OnContactsOnline(m_nFriendsOnline);
		}
	}

	if (m_pDSound)
	{
		g_VoiceManager.ProcessVoice();

		// Make sure we send voice data at an appropriate rate
		if( m_VoiceTimer.GetElapsedSeconds() > VOICE_SEND_INTERVAL )
		{
			SendVoiceDataToEngine();
		}
	}
}
// Called whenever voice data is produced by the voice system
void CKaiClient::VoiceDataCallback( DWORD dwPort, DWORD dwSize, VOID* pvData, VOID* pContext )
{
    CKaiClient* pThis = (CKaiClient*)pContext;
    pThis->OnVoiceData( dwPort, dwSize, pvData );
}

void CKaiClient::OnVoiceData( DWORD dwControllerPort, DWORD dwSize, VOID* pvData )
{
	if (m_pDSound)
	{
		m_pEgress->Write((LPBYTE)pvData);

		// We've set up our voice timer such that it SHOULD cause us to send out 
		// our buffered voice data before the buffer fills up.  However, things
		// like framerate glitches, etc., could cause us to fill up before we 
		// notice the timer has fired.
		if( m_pEgress->Size() == MAX_VOICE_PER_SEND )
		{
			SendVoiceDataToEngine();
		}
	}
}



// Called whenever a voice communicator event occurs e.g. insertions/removals
void CKaiClient::CommunicatorCallback( DWORD dwPort, VOICE_COMMUNICATOR_EVENT event, VOID* pContext )
{
    CKaiClient* pThis = (CKaiClient*)pContext;
    pThis->OnCommunicatorEvent( dwPort, event );
}

void CKaiClient::OnCommunicatorEvent( DWORD dwControllerPort, VOICE_COMMUNICATOR_EVENT event )
{
    switch( event )
    {
    case VOICE_COMMUNICATOR_INSERTED:
		CLog::Log(LOGINFO,"Voice communicator inserted.");
        g_VoiceManager.SetLoopback( dwControllerPort, FALSE );
		SetBearerCaps(TRUE);
        break;

    case VOICE_COMMUNICATOR_REMOVED:
		CLog::Log(LOGINFO,"Voice communicator removed.");
		SetBearerCaps(FALSE);
        break;
    }
}

DWORD CKaiClient::Crc32FromString(CStdString& aString)
{
	Crc32 crc;
	crc.Compute(aString.c_str(),aString.length());
	return (DWORD)crc;
}

#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::Chat(char *pBuf)
{
	int index = 0, send_index = 0;
	BYTE type;
	CUser* pUser = NULL;
	char chatstr[1024], send_buff[1024], finalstr[1024]; 
	memset( finalstr, NULL, 1024 );
	std::string buff;

	if (isMuted())
		return;	

	type = GetByte(pBuf, index);
	if (!GetKOString(pBuf, chatstr, index, 512))
		return;

	SetByte(send_buff, WIZ_CHAT, send_index);

	if( isGM() && type == GENERAL_CHAT)
		type = 0x14;

	SetByte(send_buff, type, send_index);
	SetByte(send_buff, getNation(), send_index);
	SetShort(send_buff, GetSocketID(), send_index);
	SetKOString(send_buff, m_pUserData->m_id, send_index, 1);

	if (type != PUBLIC_CHAT && type != ANNOUNCEMENT_CHAT)
		SetKOString(send_buff, chatstr, send_index);

	if (type == PUBLIC_CHAT) {
		if (!isGM()) 
			return;
		::_LoadStringFromResource(IDP_ANNOUNCEMENT, buff);
		sprintf( finalstr, buff.c_str(), chatstr );
		SetKOString(send_buff, chatstr, send_index);
	}

	if (type == ANNOUNCEMENT_CHAT) {
		if (!isGM())
			return;
		::_LoadStringFromResource(IDP_ANNOUNCEMENT, buff);
		sprintf( finalstr, buff.c_str(), chatstr );
		SetKOString(send_buff, chatstr, send_index);
	}

	switch (type) 
	{
	case GENERAL_CHAT:
		m_pMain->Send_NearRegion(send_buff, send_index, GetMap(), m_RegionX, m_RegionZ, m_pUserData->m_curx, m_pUserData->m_curz);
		break;

	case PRIVATE_CHAT:
		if (m_sPrivateChatUser == GetSocketID()) 
			break;

		pUser = m_pMain->GetUserPtr(m_sPrivateChatUser);
		if (pUser == NULL || pUser->GetState() != STATE_GAMESTART) 
			break;

		pUser->Send(send_buff, send_index);
		// Send(send_buff, send_index);
		break;

	case PARTY_CHAT:
		if (isInParty())
			m_pMain->Send_PartyMember(m_sPartyIndex, send_buff, send_index );
		break;

	case SHOUT_CHAT:
		if (m_pUserData->m_sMp < (m_iMaxMp / 5))
			break;

		MSpChange(-(m_iMaxMp / 5));
		m_pMain->Send_Region(send_buff, send_index, GetMap(), m_RegionX, m_RegionZ);
		break;

	case KNIGHTS_CHAT:
		if (isInClan())
			m_pMain->Send_KnightsMember(m_pUserData->m_bKnights, send_buff, send_index, m_pUserData->m_bZone);
		break;
	case PUBLIC_CHAT:
		if (isGM())
			m_pMain->Send_All( send_buff, send_index );
		break;
	case ANNOUNCEMENT_CHAT:
		if (isGM())
			m_pMain->Send_All( send_buff, send_index );
		break;
	case COMMAND_CHAT:
		if( m_pUserData->m_bFame == COMMAND_CAPTAIN )
			m_pMain->Send_CommandChat( send_buff, send_index, m_pUserData->m_bNation, this );
		break;
	case MERCHANT_CHAT:
		m_pMain->Send_Region( send_buff, send_index, GetMap(), m_RegionX, m_RegionZ );
	break;
	//case WAR_SYSTEM_CHAT:
	//	m_pMain->Send_All( send_buff, send_index );
	//	break;
	}
}

void CUser::ChatTargetSelect(char *pBuf)
{
	int index = 0, send_index = 0, i = 0;
	CUser* pUser = NULL;
	char chatid[MAX_ID_SIZE+1];
	char send_buff[128];

	if (!GetKOString(pBuf, chatid, index, MAX_ID_SIZE))
		return;

	m_pMain->GetUserPtr(chatid, TYPE_CHARACTER);
	for (i = 0; i < MAX_USER; i++)
	{
		pUser = m_pMain->GetUnsafeUserPtr(i); 
		if (pUser && pUser->GetState() == STATE_GAMESTART)
		{
			if (_strnicmp(chatid, pUser->m_pUserData->m_id, MAX_ID_SIZE) == 0)
			{
				m_sPrivateChatUser = i;
				break;
			}
		}
	}

	SetByte( send_buff, WIZ_CHAT_TARGET, send_index );
	if (i == MAX_USER)
		SetKOString(send_buff, "", send_index);
	else
		SetKOString(send_buff, chatid, send_index);

	Send(send_buff, send_index);
}

// Knights.cpp: implementation of the CKnights class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Knights.h"
#include "User.h"
#include "GameDefine.h"
#include "EbenezerDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKnights::CKnights()
{
	InitializeValue();
}

void CKnights::InitializeValue()
{
	m_sIndex = 0;
	m_byFlag = 0;			// 1 : Clan, 2 : ����
	m_byNation = 0;		// nation
	m_byGrade = 5;			// clan ��� (1 ~ 5���)
	m_byRanking = 0;		// clan ��� (1 ~ 5��)
	m_sMembers = 1;
	memset(m_strName, 0x00, sizeof(m_strName));
	memset(m_strChief, 0x00, sizeof(m_strChief));
	memset(m_strViceChief_1, 0x00, sizeof(m_strViceChief_1));
	memset(m_strViceChief_2, 0x00, sizeof(m_strViceChief_2));
	memset(m_strViceChief_3, 0x00, sizeof(m_strViceChief_3));
	memset( m_Image, 0x00, MAX_KNIGHTS_MARK );
	m_nMoney = 0;
	m_sDomination = 0;
	m_nPoints = 0;
	m_sCape = -1;
	m_sAlliance = 0;
}

void CKnights::OnLogin(CUser *pUser)
{
	// TO-DO: Implement login notice here

	// Set the active session for this user
	foreach_array (i, m_arKnightsUser)
	{
		if (!m_arKnightsUser[i].byUsed
			|| _strcmpi(m_arKnightsUser[i].strUserName, pUser->m_pUserData->m_id))
			continue;

		m_arKnightsUser[i].pSession = pUser;
		break;
	}
}

void CKnights::OnLogout(CUser *pUser)
{
	// TO-DO: Implement logout notice here

	// Unset the active session for this user
	foreach_array (i, m_arKnightsUser)
	{
		if (!m_arKnightsUser[i].byUsed
			|| _strcmpi(m_arKnightsUser[i].strUserName, pUser->m_pUserData->m_id))
			continue;

		m_arKnightsUser[i].pSession = NULL;
		break;
	}
}

bool CKnights::AddUser(const char *strUserID)
{
	for (int i = 0; i < MAX_CLAN_USERS; i++)
	{
		if (m_arKnightsUser[i].byUsed == 0)
		{
			m_arKnightsUser[i].byUsed = 1;
			strcpy(m_arKnightsUser[i].strUserName, strUserID);
			m_arKnightsUser[i].pSession = g_pMain->GetUserPtr(strUserID, TYPE_CHARACTER);
			return true;
		}
	}

	return false;
}

bool CKnights::AddUser(CUser *pUser)
{
	if (pUser == NULL
		|| !AddUser(pUser->m_pUserData->m_id))
		return false;

	pUser->m_pUserData->m_bKnights = m_sIndex;
	pUser->m_pUserData->m_bFame = TRAINEE;
	return true;
}

bool CKnights::RemoveUser(const char *strUserID)
{
	for (int i = 0; i < MAX_CLAN_USERS; i++)
	{
		if (m_arKnightsUser[i].byUsed == 0)
			continue;

		if (!_strcmpi(m_arKnightsUser[i].strUserName, strUserID))
		{
			m_arKnightsUser[i].byUsed = 0;
			strcpy(m_arKnightsUser[i].strUserName, "");
			m_arKnightsUser[i].pSession = NULL;
			return true;
		}
	}

	return false;
}

bool CKnights::RemoveUser(CUser *pUser)
{
	if (pUser == NULL)
		return false;

	bool result = RemoveUser(pUser->m_pUserData->m_id);
	pUser->m_pUserData->m_bKnights = 0;
	pUser->m_pUserData->m_bFame = 0;
	if (!pUser->isClanLeader())
		pUser->SendClanUserStatusUpdate();
	return result;
}

void CKnights::Disband(CUser *pLeader /*= NULL*/)
{
	CString clanNotice = g_pMain->GetServerResource(m_byFlag == CLAN_TYPE ? IDS_CLAN_DESTROY : IDS_KNIGHTS_DESTROY);
	SendChat(clanNotice, m_strName);

	foreach_array (i, m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &m_arKnightsUser[i];
		if (p->byUsed && p->pSession != NULL)
			RemoveUser(p->pSession);
	}
	g_pMain->m_KnightsArray.DeleteData(m_sIndex);

	if (pLeader == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_DESTROY));
	result << uint8(1);
	pLeader->Send(&result);
}

void CKnights::ConstructChatPacket(Packet & data, const char * format, ...)
{
	char buffer[128];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, 128, format, ap);
	va_end(ap);

	data.Initialize(WIZ_CHAT);
	data  << uint8(KNIGHTS_CHAT)	// clan chat opcode
		  << uint8(1)				// nation
		  << int16(-1)				// session ID
		  << uint8(0)				// character name length
		  << buffer;				// chat message
}

void CKnights::SendChat(const char * format, ...)
{
	Packet data;
	char buffer[128];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, 128, format, ap);
	va_end(ap);

	ConstructChatPacket(data, "%s", buffer); // hmm, this could be done better.
	Send(&data);
}

void CKnights::Send(Packet *pkt)
{
	foreach_array (i, m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &m_arKnightsUser[i];
		if (p->byUsed && p->pSession != NULL)
			p->pSession->Send(pkt);
	}
}

CKnights::~CKnights()
{
}

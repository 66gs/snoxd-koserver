#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::FriendProcess(char *pBuf)
{
	int index = 0;
	BYTE subcommand = GetByte( pBuf, index );

	switch (subcommand)
	{
		case FRIEND_REQUEST:
			FriendRequest(pBuf+index);
			break;
		case FRIEND_ACCEPT:
			FriendAccept(pBuf+index);
			break;
		case FRIEND_REPORT:
			FriendReport(pBuf+index);
			break;
		case FRIEND_CANCEL:
			FriendCancel(pBuf+index);
			break;
	}
}

void CUser::FriendRequest(char *pBuf)
{
	int index = 0, destid = -1, send_index = 0;

	CUser* pUser = NULL;
	char buff[4];

	destid = GetShort(pBuf, index);
	pUser = m_pMain->GetUserPtr(destid);
	if( pUser == NULL
		|| pUser->m_sFriendUser != -1
		|| pUser->getNation() != getNation())
		goto fail_return;

	m_sFriendUser = destid;
	pUser->m_sFriendUser = m_Sid;

	SetByte( buff, WIZ_FRIEND_PROCESS, send_index );
	SetByte( buff, FRIEND_REQUEST, send_index );
	SetShort( buff, m_Sid, send_index );
	pUser->Send( buff, send_index );	
	return;

fail_return:
	SetByte( buff, WIZ_FRIEND_PROCESS, send_index );
	SetByte( buff, FRIEND_CANCEL, send_index );
	Send( buff, send_index );
}

void CUser::FriendAccept(char *pBuf)
{
	int index = 0, destid = -1, send_index = 0;
	CUser* pUser = NULL;
	char buff[256];	memset( buff, 0x00, 256 );
	BYTE result = GetByte( pBuf, index );

	pUser = m_pMain->GetUserPtr(m_sFriendUser);

	if (pUser == NULL) 
	{
		m_sFriendUser = -1;
		return;
	}

	m_sFriendUser = -1;
	pUser->m_sFriendUser = -1;

	SetByte( buff, WIZ_FRIEND_PROCESS, send_index );
	SetByte( buff, FRIEND_ACCEPT, send_index );
	SetByte( buff, result, send_index );
	pUser->Send( buff, send_index );
}

void CUser::FriendReport(char *pBuf)
{
	int index = 0; short usercount = 0, idlen = 0;		// Basic Initializations.
	int send_index = 0;
	char send_buff[640];
	memset( send_buff, NULL, 640);
	char userid[MAX_ID_SIZE+1];
	memset( userid, NULL, MAX_ID_SIZE+1 );
	CUser* pUser = NULL;

	return;

	usercount = GetShort( pBuf, index );	// Get usercount packet.
	if( usercount >= 30 || usercount < 0) return;
	
	SetByte( send_buff, WIZ_FRIEND_PROCESS, send_index );
	SetShort( send_buff, usercount, send_index);

	for (int k = 0 ; k < usercount ; k++) {
		idlen = GetShort( pBuf, index );
		if( idlen > MAX_ID_SIZE ) {
			SetShort(send_buff, strlen(userid), send_index);
			SetString( send_buff, userid, strlen(userid), send_index );
			SetShort(send_buff, -1, send_index);
			SetByte( send_buff, 0, send_index);
			continue;
		}
		GetString( userid, pBuf, idlen, index );

		pUser = m_pMain->GetUserPtr(userid, TYPE_CHARACTER);

		SetKOString(send_buff, userid, send_index);

		if (!pUser) { // No such user
			SetShort(send_buff, -1, send_index);
			SetByte( send_buff, 0, send_index);
		}
		else {
			SetShort(send_buff, pUser->GetSocketID(), send_index);
			if (pUser->m_sPartyIndex >=0) {
				SetByte( send_buff, 3, send_index);
			}
			else {
				SetByte( send_buff, 1, send_index);
			}
		}
	}

	Send( send_buff, send_index );
}

void CUser::FriendCancel(char *pBuf)
{
	// TO-DO
}
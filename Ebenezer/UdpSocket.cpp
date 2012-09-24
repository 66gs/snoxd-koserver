// UdpSocket.cpp: implementation of the CUdpSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "define.h"
#include "UdpSocket.h"
#include "EbenezerDlg.h"
#include "AiPacket.h"
#include "Knights.h"
#include "User.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

DWORD WINAPI RecvUDPThread( LPVOID lp )
{
	CUdpSocket *pUdp = (CUdpSocket*)lp;
	int ret = 0, addrlen = sizeof(pUdp->m_ReplyAddress);

	while(1) {
		ret = recvfrom( pUdp->m_hUDPSocket, pUdp->m_pRecvBuf, 1024, 0, (LPSOCKADDR)&pUdp->m_ReplyAddress, &addrlen );

		if(ret == SOCKET_ERROR) {
			int err = WSAGetLastError(); 
			getpeername(pUdp->m_hUDPSocket, (SOCKADDR*)&pUdp->m_ReplyAddress, &addrlen);
			TRACE("recvfrom() error : %d IP : %s\n", err, inet_ntoa(pUdp->m_ReplyAddress.sin_addr));

			// ������ ��ƾ...

			Sleep(10);
			continue;
		}

		if( ret ) {
			if( pUdp->PacketProcess(ret) == false ) {
				// broken packet...
			}
		}

		Sleep(10);
	}

	return 1;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUdpSocket::CUdpSocket(CEbenezerDlg* pMain)
{
	m_hUDPSocket = INVALID_SOCKET;
	memset( m_pRecvBuf, 0x00, 8192 );
	m_pMain = pMain;
}

CUdpSocket::~CUdpSocket()
{

}

bool CUdpSocket::CreateSocket()
{
	m_hUDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_hUDPSocket == INVALID_SOCKET) {
	    TRACE("udp socket create fail...\n");
        return false;
    }

	int sock_buf_size = 32768;
    setsockopt(m_hUDPSocket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
    setsockopt(m_hUDPSocket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));

	int optlen = sizeof(sock_buf_size);
    int r = getsockopt(m_hUDPSocket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, &optlen);
	if( r == SOCKET_ERROR ) {
		TRACE("buffer size set fail...\n");
		return false;
	}

    memset((char*)&m_SocketAddress, 0x00, sizeof(m_SocketAddress));
    m_SocketAddress.sin_family = AF_INET;
    m_SocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_SocketAddress.sin_port = htons(_UDP_PORT);

    if( bind(m_hUDPSocket, (LPSOCKADDR)&m_SocketAddress, sizeof(m_SocketAddress)) == SOCKET_ERROR ) {
	    TRACE("UDP bind() Error...\n");
		closesocket(m_hUDPSocket);
        return false;
    }

	DWORD id;
	m_hUdpThread = ::CreateThread( NULL, 0, RecvUDPThread, (LPVOID)this, 0, &id);
	::SetThreadPriority(m_hUdpThread,THREAD_PRIORITY_ABOVE_NORMAL);

	TRACE("UDP Socket Create Success...\n");
	return true;
}

int CUdpSocket::SendUDPPacket(char* strAddress, char* pBuf, int len)
{
	int s_size = 0, index = 0;
	
	BYTE pTBuf[2048];

	if( len > 2048 || len <= 0 )
		return 0;

	pTBuf[index++] = (BYTE)PACKET_START1;
	pTBuf[index++] = (BYTE)PACKET_START2;
	memcpy( pTBuf+index, &len, 2 );
	index += 2;
	memcpy( pTBuf+index, pBuf, len );
	index += len;
	pTBuf[index++] = (BYTE)PACKET_END1;
	pTBuf[index++] = (BYTE)PACKET_END2;

    m_SocketAddress.sin_addr.s_addr = inet_addr(strAddress);

	s_size = sendto(m_hUDPSocket, (char*)pTBuf, index, 0, (LPSOCKADDR)&m_SocketAddress, sizeof(m_SocketAddress));

	return s_size;
}

int CUdpSocket::SendUDPPacket(char* strAddress, Packet *pkt)
{
	uint16 len = (uint16)pkt->size() + 1;
	if (len == 0)
		return 0;

	ByteBuffer buff(len + 6);
	buff	<< uint8(PACKET_START1) << uint8(PACKET_START2)
			<< len << pkt->GetOpcode() << *pkt
			<< uint8(PACKET_END1) << uint8(PACKET_END2);

    m_SocketAddress.sin_addr.s_addr = inet_addr(strAddress);
	return sendto(m_hUDPSocket, (const char *)buff.contents(), buff.size(), 0, (LPSOCKADDR)&m_SocketAddress, sizeof(m_SocketAddress));
}

bool CUdpSocket::PacketProcess(int len)
{
	BYTE		*pTmp;
	bool		foundCore;
	MYSHORT		slen;
	int length;

	if( len <= 0 ) return false;

	pTmp = new BYTE[len+1];

	memcpy( pTmp, m_pRecvBuf, len );

	foundCore = FALSE;

	int	sPos=0, ePos = 0;

	for (int i = 0; i < len && !foundCore; i++)
	{
		if (i+2 >= len) break;

		if (pTmp[i] == PACKET_START1 && pTmp[i+1] == PACKET_START2)
		{
			sPos = i+2;

			slen.b[0] = pTmp[sPos];
			slen.b[1] = pTmp[sPos + 1];

			length = slen.w;

			if( length <= 0 ) goto cancelRoutine;
			if( length > len ) goto cancelRoutine;

			ePos = sPos+length + 2;

			if( (ePos + 2) > len ) goto cancelRoutine;

			if (pTmp[ePos] == PACKET_END1 && pTmp[ePos+1] == PACKET_END2)
			{
				Parsing( (char*)(pTmp+sPos+2), length );
				foundCore = TRUE;
				break;
			}
			else 
				goto cancelRoutine;
		}
	}

	delete[] pTmp;

	return foundCore;

cancelRoutine:	
	delete[] pTmp;
	return foundCore;
}

void CUdpSocket::Parsing(char *pBuf, int len)
{
	BYTE command;
	int index = 0;

	command = GetByte( pBuf, index );

	switch( command ) {
	case STS_CHAT:
		ServerChat( pBuf+index );
		break;
	case UDP_BATTLE_EVENT_PACKET:
		RecvBattleEvent( pBuf+index );
		break;
	case UDP_KNIGHTS_PROCESS:
		ReceiveKnightsProcess( pBuf+index );
		break;
	case UDP_BATTLEZONE_CURRENT_USERS:
		RecvBattleZoneCurrentUsers( pBuf+index );
		break;
	}
}

void CUdpSocket::ServerChat(char *pBuf)
{
	int index = 0;
	char chatstr[512];

	if (!GetKOString(pBuf, chatstr, index, sizeof(chatstr)))
		return;

	DEBUG_LOG(chatstr);
}

void CUdpSocket::RecvBattleEvent(char *pBuf)
{
	int index = 0, send_index = 0, udp_index = 0;
	int nType = 0, nResult = 0, nLen = 0, nKillKarus = 0, nElmoKill = 0;
	char strMaxUserName[MAX_ID_SIZE+1], strKnightsName[MAX_ID_SIZE+1];
	char finalstr[1024], send_buff[1024];

	std::string buff;

	nType = GetByte( pBuf, index );
	nResult = GetByte(pBuf, index);

	if( nType == BATTLE_EVENT_OPEN )	{
	}
	else if( nType == BATTLE_MAP_EVENT_RESULT )	{
		if( m_pMain->m_byBattleOpen == NO_BATTLE )	{
			TRACE("#### UDP RecvBattleEvent Fail : battleopen = %d, type = %d\n", m_pMain->m_byBattleOpen, nType);
			return;
		}
		if( nResult == KARUS )	{
			//TRACE("--> UDP RecvBattleEvent : ī�罺 ������ �Ѿ �� �־�\n");
			m_pMain->m_byKarusOpenFlag = 1;		// ī�罺 ������ �Ѿ �� �־�
		}
		else if( nResult == ELMORAD )	{
			//TRACE("--> UDP  RecvBattleEvent : ���� ������ �Ѿ �� �־�\n");
			m_pMain->m_byElmoradOpenFlag = 1;	// ���� ������ �Ѿ �� �־�
		}
	}
	else if( nType == BATTLE_EVENT_RESULT )	{
		if( m_pMain->m_byBattleOpen == NO_BATTLE )	{
			TRACE("####  UDP  RecvBattleEvent Fail : battleopen = %d, type=%d\n", m_pMain->m_byBattleOpen, nType);
			return;
		}
		if( nResult == KARUS )	{
			//TRACE("-->  UDP RecvBattleEvent : ī�罺�� �¸��Ͽ����ϴ�.\n");
		}
		else if( nResult == ELMORAD )	{
			//TRACE("-->  UDP RecvBattleEvent : �����尡 �¸��Ͽ����ϴ�.\n");
		}

		m_pMain->m_bVictory = nResult;
		m_pMain->m_byOldVictory = nResult;
		m_pMain->m_byKarusOpenFlag = 0;		// ī�罺 ������ �Ѿ �� ������
		m_pMain->m_byElmoradOpenFlag = 0;	// ���� ������ �Ѿ �� ������
		m_pMain->m_byBanishFlag = 1;
	}
	else if( nType == BATTLE_EVENT_MAX_USER )	{
		nLen = GetByte(pBuf, index);
		if (!GetKOString(pBuf, strKnightsName, index, MAX_ID_SIZE)
			|| GetKOString(pBuf, strMaxUserName, index, MAX_ID_SIZE))
			return;

		int nResourceID = 0;
		switch (nResult)
		{
		case 1: // captain
			nResourceID = IDS_KILL_CAPTAIN;
			break;
		case 2: // keeper

		case 7: // warders?
		case 8:
			nResourceID = IDS_KILL_GATEKEEPER;
			break;

		case 3: // Karus sentry
			nResourceID = IDS_KILL_KARUS_GUARD1;
			break;
		case 4: // Karus sentry
			nResourceID = IDS_KILL_KARUS_GUARD2;
			break;
		case 5: // El Morad sentry
			nResourceID = IDS_KILL_ELMO_GUARD1;
			break;
		case 6: // El Morad sentry
			nResourceID = IDS_KILL_ELMO_GUARD2;
			break;
		}

		if (nResourceID == 0)
		{
			TRACE("RecvBattleEvent: could not establish resource for result %d", nResult);
			return;
		}

		_snprintf(finalstr, sizeof(finalstr), m_pMain->GetServerResource(nResourceID), strKnightsName, strMaxUserName);

		SetByte( send_buff, WIZ_CHAT, send_index );
		SetByte( send_buff, WAR_SYSTEM_CHAT, send_index );
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, -1, send_index );
		SetKOString( send_buff, finalstr, send_index );
		m_pMain->Send_All( send_buff, send_index );

		send_index = 0;
		SetByte( send_buff, WIZ_CHAT, send_index );
		SetByte( send_buff, PUBLIC_CHAT, send_index );
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, -1, send_index );
		SetKOString( send_buff, finalstr, send_index );
		m_pMain->Send_All( send_buff, send_index );
	}
	else if( nType == BATTLE_EVENT_KILL_USER )	{
		if( nResult == 1 )	{
			nKillKarus = GetShort( pBuf, index );
			nElmoKill = GetShort( pBuf, index );
			m_pMain->m_sKarusDead = m_pMain->m_sKarusDead + nKillKarus;
			m_pMain->m_sElmoradDead = m_pMain->m_sElmoradDead + nElmoKill;

			//TRACE("-->  UDP RecvBattleEvent type = 1 : ���� ���� ���μ� : karus=%d->%d, elmo=%d->%d\n", nKillKarus, m_pMain->m_sKarusDead, nElmoKill, m_pMain->m_sElmoradDead);

			SetByte( send_buff, UDP_BATTLE_EVENT_PACKET, send_index );
			SetByte( send_buff, BATTLE_EVENT_KILL_USER, send_index );
			SetByte( send_buff, 2, send_index );						// karus�� ���� ����
			SetShort( send_buff, m_pMain->m_sKarusDead, send_index );
			SetShort( send_buff, m_pMain->m_sElmoradDead, send_index );
			m_pMain->Send_UDP_All( send_buff, send_index );
		}
		else if( nResult == 2 )	{
			nKillKarus = GetShort( pBuf, index );
			nElmoKill = GetShort( pBuf, index );

			//TRACE("-->  UDP RecvBattleEvent type = 2 : ���� ���� ���μ� : karus=%d->%d, elmo=%d->%d\n", m_pMain->m_sKarusDead, nKillKarus, m_pMain->m_sElmoradDead, nElmoKill);

			m_pMain->m_sKarusDead = nKillKarus;
			m_pMain->m_sElmoradDead = nElmoKill;
		}
	}

}

void CUdpSocket::ReceiveKnightsProcess( char* pBuf )
{
	int index = 0, command = 0, pktsize = 0, count = 0;

	command = GetByte( pBuf, index );
	//TRACE("UDP - ReceiveKnightsProcess - command=%d\n", command);

	switch(command) {
	case KNIGHTS_CREATE:
		RecvCreateKnights( pBuf+index );
		break;
	case KNIGHTS_JOIN:
	case KNIGHTS_WITHDRAW:
		RecvJoinKnights( pBuf+index, command );
		break;
	case KNIGHTS_REMOVE:
	case KNIGHTS_ADMIT:
	case KNIGHTS_REJECT:
	case KNIGHTS_CHIEF:
	case KNIGHTS_VICECHIEF:
	case KNIGHTS_OFFICER:
	case KNIGHTS_PUNISH:
		RecvModifyFame( pBuf+index, command );
		break;
	case KNIGHTS_DESTROY:
		RecvDestroyKnights( pBuf+index );
		break;
	}
}

void CUdpSocket::RecvCreateKnights( char* pBuf )
{
	int index = 0, send_index = 0, knightsindex = 0, nation = 0, community = 0;
	char knightsname[MAX_ID_SIZE+1], chiefname[MAX_ID_SIZE+1];
	CKnights* pKnights = NULL;

	community = GetByte( pBuf, index );
	knightsindex = GetShort( pBuf, index );
	nation = GetByte( pBuf, index );
	if (!GetKOString(pBuf, knightsname, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, chiefname, index, MAX_ID_SIZE))
		return;

	pKnights = new CKnights();

	pKnights->m_sIndex = knightsindex;
	pKnights->m_byFlag = community;
	pKnights->m_byNation = nation;
	pKnights->m_strName = knightsname;
	pKnights->m_strChief = chiefname;

	m_pMain->m_KnightsArray.PutData( pKnights->m_sIndex, pKnights );

	// Ŭ�������� �߰�
	m_pMain->m_KnightsManager.AddKnightsUser( knightsindex, chiefname );

	//TRACE("UDP - RecvCreateKnights - knname=%s, name=%s, index=%d\n", knightsname, chiefname, knightsindex);
}

void CUdpSocket::RecvJoinKnights( char* pBuf, BYTE command )
{
	int send_index = 0, knightsindex = 0, index = 0;
	char charid[MAX_ID_SIZE+1], send_buff[128], finalstr[128];
	CKnights*	pKnights = NULL;

	knightsindex = GetShort( pBuf, index );
	if (!GetKOString(pBuf, charid, index, MAX_ID_SIZE))
		return;

	pKnights = m_pMain->m_KnightsArray.GetData( knightsindex );

	if( command == KNIGHTS_JOIN ) {
		sprintf( finalstr, "#### %s���� �����ϼ̽��ϴ�. ####", charid );
		// Ŭ�������� �߰�
		m_pMain->m_KnightsManager.AddKnightsUser( knightsindex, charid );
		TRACE("UDP - RecvJoinKnights - ����, name=%s, index=%d\n", charid, knightsindex);
	}
	else {		// Ż��..
		// Ŭ�������� �߰�
		m_pMain->m_KnightsManager.RemoveKnightsUser( knightsindex, charid );
		sprintf( finalstr, "#### %s���� Ż���ϼ̽��ϴ�. ####", charid );
		TRACE("UDP - RecvJoinKnights - Ż��, name=%s, index=%d\n", charid, knightsindex );
	}

	//TRACE("UDP - RecvJoinKnights - command=%d, name=%s, index=%d\n", command, charid, knightsindex);

	send_index = 0;
	SetByte( send_buff, WIZ_CHAT, send_index );
	SetByte( send_buff, KNIGHTS_CHAT, send_index );
	SetByte( send_buff, 1, send_index );
	SetShort( send_buff, -1, send_index );
	SetKOString( send_buff, finalstr, send_index );
	m_pMain->Send_KnightsMember( knightsindex, send_buff, send_index );
}

void CUdpSocket::RecvModifyFame( char* pBuf, BYTE command )
{
	int index = 0, send_index = 0, knightsindex = 0, vicechief = 0;
	char send_buff[128], finalstr[128], userid[MAX_ID_SIZE+1];
	CUser* pTUser = NULL;
	CKnights*	pKnights = NULL;

	knightsindex = GetShort( pBuf, index );
	if (!GetKOString(pBuf, userid, index, MAX_ID_SIZE))
		return;

	pTUser = m_pMain->GetUserPtr(userid, TYPE_CHARACTER);
	pKnights = m_pMain->m_KnightsArray.GetData( knightsindex );

	switch( command ) {
	case KNIGHTS_REMOVE:
		if( pTUser ) {
			pTUser->m_pUserData->m_bKnights = 0;
			pTUser->m_pUserData->m_bFame = 0;
			sprintf( finalstr, "#### %s���� �߹�Ǽ̽��ϴ�. ####", pTUser->m_pUserData->m_id );
			m_pMain->m_KnightsManager.RemoveKnightsUser( knightsindex, pTUser->m_pUserData->m_id );
		}
		else	{
			m_pMain->m_KnightsManager.RemoveKnightsUser( knightsindex, userid );
		}
		break;
	case KNIGHTS_ADMIT:
		if( pTUser )
			pTUser->m_pUserData->m_bFame = KNIGHT;
		break;
	case KNIGHTS_REJECT:
		if( pTUser ) {
			pTUser->m_pUserData->m_bKnights = 0;
			pTUser->m_pUserData->m_bFame = 0;
			m_pMain->m_KnightsManager.RemoveKnightsUser( knightsindex, pTUser->m_pUserData->m_id );
		}
		break;
	case KNIGHTS_CHIEF+0x10:
		if( pTUser )	{
			pTUser->m_pUserData->m_bFame = CHIEF;
			m_pMain->m_KnightsManager.ModifyKnightsUser( knightsindex, pTUser->m_pUserData->m_id );
			sprintf( finalstr, "#### %s���� �������� �Ӹ�Ǽ̽��ϴ�. ####", pTUser->m_pUserData->m_id );
		}
		break;
	case KNIGHTS_VICECHIEF+0x10:
		if( pTUser )	{
			pTUser->m_pUserData->m_bFame = VICECHIEF;
			m_pMain->m_KnightsManager.ModifyKnightsUser( knightsindex, pTUser->m_pUserData->m_id );
			sprintf( finalstr, "#### %s���� �δ������� �Ӹ�Ǽ̽��ϴ�. ####", pTUser->m_pUserData->m_id );
		}
		break;
	case KNIGHTS_OFFICER+0x10:
		if( pTUser )
			pTUser->m_pUserData->m_bFame = OFFICER;
		break;
	case KNIGHTS_PUNISH+0x10:
		if( pTUser )
			pTUser->m_pUserData->m_bFame = PUNISH;
		break;
	}

	if( pTUser ) {
		//TRACE("UDP - RecvModifyFame - command=%d, nid=%d, name=%s, index=%d, fame=%d\n", command, pTUser->GetSocketID(), pTUser->m_pUserData->m_id, knightsindex, pTUser->m_pUserData->m_bFame);
		send_index = 0;
		SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
		SetByte( send_buff, KNIGHTS_MODIFY_FAME, send_index );
		SetByte( send_buff, 0x01, send_index );
		if( command == KNIGHTS_REMOVE )	{
			SetShort( send_buff, pTUser->GetSocketID(), send_index );
			SetShort( send_buff, pTUser->m_pUserData->m_bKnights, send_index );
			SetByte( send_buff, pTUser->m_pUserData->m_bFame, send_index );
			m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ, NULL, false );
		}
		else	{
			SetShort( send_buff, pTUser->GetSocketID(), send_index );
			SetShort( send_buff, pTUser->m_pUserData->m_bKnights, send_index );
			SetByte( send_buff, pTUser->m_pUserData->m_bFame, send_index );
			pTUser->Send( send_buff, send_index );
		}

		if( command == KNIGHTS_REMOVE )	{
			send_index = 0;
			SetByte( send_buff, WIZ_CHAT, send_index );
			SetByte( send_buff, KNIGHTS_CHAT, send_index );
			SetByte( send_buff, 1, send_index );
			SetShort( send_buff, -1, send_index );
			SetKOString( send_buff, finalstr, send_index );
			pTUser->Send( send_buff, send_index );
		}
	}

	send_index = 0;
	SetByte( send_buff, WIZ_CHAT, send_index );
	SetByte( send_buff, KNIGHTS_CHAT, send_index );
	SetByte( send_buff, 1, send_index );
	SetShort( send_buff, -1, send_index );
	SetKOString( send_buff, finalstr, send_index );
	m_pMain->Send_KnightsMember( knightsindex, send_buff, send_index );
}

void CUdpSocket::RecvDestroyKnights( char* pBuf )
{
	int send_index = 0, knightsindex = 0, index = 0, flag = 0;
	char send_buff[128], finalstr[128]; 
	CKnights*	pKnights = NULL;
	CUser* pTUser = NULL;

	knightsindex = GetShort( pBuf, index );

	pKnights = m_pMain->m_KnightsArray.GetData( knightsindex );
	if( !pKnights )		{
		TRACE("UDP - ### RecvDestoryKnights  Fail == index = %d ###\n", knightsindex);
		return;
	}

	flag = pKnights->m_byFlag;

	// Ŭ���̳� ������ �ı��� �޽����� ������ ���� ����Ÿ�� �ʱ�ȭ
	if( flag == CLAN_TYPE)
		sprintf( finalstr, "#### %s Ŭ���� ��ü�Ǿ����ϴ� ####", pKnights->m_strName );
	else if( flag == KNIGHTS_TYPE )
		sprintf( finalstr, "#### %s ������ ��ü�Ǿ����ϴ� ####", pKnights->m_strName );

	send_index = 0;
	SetByte( send_buff, WIZ_CHAT, send_index );
	SetByte( send_buff, KNIGHTS_CHAT, send_index );
	SetByte( send_buff, 1, send_index );
	SetShort( send_buff, -1, send_index );
	SetKOString( send_buff, finalstr, send_index );
	m_pMain->Send_KnightsMember( knightsindex, send_buff, send_index );

	for (int i = 0; i < MAX_USER; i++)
	{
		pTUser = m_pMain->GetUnsafeUserPtr(i);
		if (pTUser == NULL || pTUser->m_pUserData->m_bKnights != knightsindex)
			continue;

		pTUser->m_pUserData->m_bKnights = 0;
		pTUser->m_pUserData->m_bFame = 0;

		m_pMain->m_KnightsManager.RemoveKnightsUser( knightsindex, pTUser->m_pUserData->m_id );

		send_index = 0;
		SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
		SetByte( send_buff, KNIGHTS_MODIFY_FAME, send_index );
		SetByte( send_buff, 0x01, send_index );
		SetShort( send_buff, pTUser->GetSocketID(), send_index );
		SetShort( send_buff, pTUser->m_pUserData->m_bKnights, send_index );
		SetByte( send_buff, pTUser->m_pUserData->m_bFame, send_index );
		m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ, NULL, false );
	}
	
	m_pMain->m_KnightsArray.DeleteData( knightsindex );
	//TRACE("UDP - RecvDestoryKnights - index=%d\n", knightsindex);
}
	
void CUdpSocket::RecvBattleZoneCurrentUsers( char* pBuf )
{
	int nKarusMan = 0, nElmoradMan = 0, index = 0;

	nKarusMan = GetShort( pBuf, index );
	nElmoradMan = GetShort( pBuf, index );

	m_pMain->m_sKarusCount = nKarusMan;
	m_pMain->m_sElmoradCount = nElmoradMan;
	//TRACE("UDP - RecvBattleZoneCurrentUsers - karus=%d, elmorad=%d\n", nKarusMan, nElmoradMan);
}

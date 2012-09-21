// User.cpp: implementation of the CUser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "versionmanager.h"
#include "versionmanagerdlg.h"
#include "User.h"

#pragma warning(disable : 4786)		// Visual C++ Only
#include <set>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUser::CUser()
{

}

CUser::~CUser()
{

}

void CUser::Initialize()
{
	m_pMain = (CVersionManagerDlg*)AfxGetApp()->GetMainWnd();

	CIOCPSocket2::Initialize();
}

void CUser::CloseProcess()
{

	CIOCPSocket2::CloseProcess();
}

void CUser::Parsing(int len, char *pData)
{
	int index = 0, send_index = 0;
	char buff[4096]; 
	BYTE command = GetByte(pData, index);

	switch (command) 
	{
	case LS_VERSION_REQ:
		SetByte(buff, LS_VERSION_REQ, send_index);
		SetShort(buff, m_pMain->m_nLastVersion, send_index);
		Send(buff, send_index);
		break;

	case LS_SERVERLIST:
		m_pMain->m_DBProcess.LoadUserCountList();

		SetByte( buff, LS_SERVERLIST, send_index );

#if __VERSION >= 1500
		SetShort(buff, GetShort(pData, index), send_index); // echo
#endif

		SetByte( buff, m_pMain->m_nServerCount, send_index );
		foreach (itr, m_pMain->m_ServerList) 
		{		
			_SERVER_INFO *pServer = *itr;

			SetKOString(buff, pServer->strServerIP, send_index);
#if __VERSION >= 1890
			SetKOString(buff, pServer->strLanIP, send_index);
#endif
			SetKOString(buff, pServer->strServerName, send_index);

			if (pServer->sUserCount <= pServer->sPlayerCap)
				SetShort( buff, pServer->sUserCount, send_index);
			else
				SetShort(buff, -1, send_index);
#if __VERSION >= 1453
			SetShort(buff, pServer->sServerID, send_index);
			SetShort(buff, pServer->sGroupID, send_index);
			SetShort(buff, pServer->sPlayerCap, send_index);
			SetShort(buff, pServer->sFreePlayerCap, send_index);

#if __VERSION < 1600
			SetByte(buff, 1, send_index); // unknown, 1 in 15XX samples, 0 in 18XX+
#else
			SetByte(buff, 0, send_index); 
#endif

			// we read all this stuff from ini, TO-DO: make this more versatile.
			SetKOString(buff, pServer->strKarusKingName, send_index);
			SetKOString(buff, pServer->strKarusNotice, send_index);
			SetKOString(buff, pServer->strElMoradKingName, send_index);
			SetKOString(buff, pServer->strElMoradNotice, send_index);
#endif
		}
		Send(buff, send_index);
		break;

	case LS_DOWNLOADINFO_REQ:
		SendDownloadInfo(GetShort(pData, index));
		break;

	case LS_LOGIN_REQ:
		LogInReq( pData+index );
		break;

	case LS_NEWS:
		SetByte(buff, LS_NEWS, send_index);

		if (m_pMain->m_news.Size)
		{
			SetKOString(buff, "Login Notice", send_index);
			SetShort(buff, m_pMain->m_news.Size, send_index);
			SetString(buff, (char *)m_pMain->m_news.Content, m_pMain->m_news.Size, send_index);
		}
		else
		{
			// dummy news, will skip past it
			SetKOString(buff, "Login Notice", send_index);
			SetKOString(buff, "<empty>", send_index);
		}

		Send(buff, send_index);
		break;

	case LS_CRYPTION: // send a key of 0, so it won't encrypt anything
		SetByte(buff, LS_CRYPTION, send_index);
		SetInt64(buff, 0, send_index);
		Send(buff, send_index);
		break;

	case LS_UNKF7:
		SetByte(buff, LS_UNKF7, send_index);
		SetShort(buff, 0, send_index);
		Send(buff, send_index);
		break;
	}
}
void CUser::LogInReq(char *pBuf)
{
	int index = 0, send_index = 0, result = 0, serverno = 0;
	BOOL bCurrentuser = FALSE;
	char send_buff[256], serverip[20] = "", accountid[MAX_ID_SIZE+1], pwd[MAX_PW_SIZE+1];

	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, pwd, index, MAX_PW_SIZE))
		goto fail_return;

	result = m_pMain->m_DBProcess.AccountLogin( accountid, pwd );
	SetByte( send_buff, LS_LOGIN_REQ, send_index );
	if( result == 1 ) { // success 
/*		bCurrentuser = m_pMain->m_DBProcess.IsCurrentUser( accountid, serverip, serverno );
		if( bCurrentuser ) {
			result = 0x05;		// Kick out
			SetByte( send_buff, result, send_index );
			SetKOString(send_buff, serverip, send_index);
			SetShort( send_buff, serverno, send_index );
		}
		else
		{*/
			SetByte( send_buff, result, send_index );
			SetByte(send_buff, -1, send_index); // prem type
			SetByte(send_buff, -1, send_index); // prem days/hours (what are we even with 19XX)

			SetKOString(send_buff, accountid, send_index); // it uses this for the game server now.
//		}
	}
	else 
	{
		SetByte( send_buff, result, send_index );
	}
	Send( send_buff, send_index );

	return;
fail_return:
	SetByte( send_buff, LS_LOGIN_REQ, send_index );
	SetByte( send_buff, 0x02, send_index );				// id, pwd �̻�...
	Send( send_buff, send_index );
}

void CUser::SendDownloadInfo(int version)
{
	int send_index = 0, filecount = 0;
	set <string> downloadset;
	char buff[2048];

	foreach (itr, m_pMain->m_VersionList) 
	{
		_VERSION_INFO *pInfo = itr->second;
		if( pInfo->sVersion > version )
			downloadset.insert(pInfo->strFileName);
	}

	SetByte( buff, LS_DOWNLOADINFO_REQ, send_index );
	SetKOString(buff, m_pMain->m_strFtpUrl, send_index);
	SetKOString(buff, m_pMain->m_strFilePath, send_index);
	SetShort( buff, downloadset.size(), send_index );
	
	foreach (itr, downloadset)
		SetKOString( buff, (char*)(*itr).c_str(), send_index );
	Send( buff, send_index );
}

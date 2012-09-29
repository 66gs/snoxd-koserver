// DBProcess.cpp: implementation of the CDBProcess class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "define.h"
#include "DBProcess.h"
#include "VersionManagerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDBProcess::CDBProcess() : m_pMain(NULL)
{
}

bool CDBProcess::Connect(TCHAR *szDSN, TCHAR *szUser, TCHAR *szPass)
{
	if (m_pMain == NULL)
		m_pMain = (CVersionManagerDlg*)AfxGetApp()->GetMainWnd();

	if (!m_dbConnection.Connect(szDSN, szUser, szPass))
	{
		m_pMain->ReportSQLError(m_dbConnection.GetError());
		return false;
	}

	return true;
}

bool CDBProcess::LoadVersionList()
{
	bool result = false;
	auto_ptr<OdbcCommand> dbCommand(m_dbConnection.CreateCommand());

	if (dbCommand.get() == NULL)
		return false;

	if (!dbCommand->Execute(_T("SELECT sVersion, sHistoryVersion, strFileName FROM VERSION")))
	{
		m_pMain->ReportSQLError(m_dbConnection.GetError());
		return false;
	}

	if (dbCommand->hasData())
	{
		m_pMain->m_sLastVersion = 0;
		do
		{
			_VERSION_INFO *pVersion = new _VERSION_INFO;

			dbCommand->FetchUInt16(1, pVersion->sVersion);
			dbCommand->FetchUInt16(2, pVersion->sHistoryVersion);
			dbCommand->FetchString(3, pVersion->strFileName);

			m_pMain->m_VersionList.insert(make_pair(pVersion->strFileName, pVersion));

			if (m_pMain->m_sLastVersion < pVersion->sVersion)
				m_pMain->m_sLastVersion = pVersion->sVersion;

		} while (dbCommand->MoveNext());
	}

	return true;
}

bool CDBProcess::LoadUserCountList()
{
	auto_ptr<OdbcCommand> dbCommand(m_dbConnection.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (!dbCommand->Execute(_T("SELECT serverid, zone1_count, zone2_count, zone3_count FROM CONCURRENT")))
	{
		m_pMain->ReportSQLError(m_dbConnection.GetError());
		return false;
	}

	if (dbCommand->hasData())
	{
		do
		{
			uint16 zone_1, zone_2, zone_3; uint8 serverID;
			dbCommand->FetchByte(1, serverID);
			dbCommand->FetchUInt16(2, zone_1);
			dbCommand->FetchUInt16(3, zone_2);
			dbCommand->FetchUInt16(4, zone_3);

			if ((uint8)(serverID - 1) < m_pMain->m_ServerList.size())
				m_pMain->m_ServerList[serverID - 1]->sUserCount = zone_1 + zone_2 + zone_3;
		} while (dbCommand->MoveNext());
	}

	return true;
}

uint16 CDBProcess::AccountLogin(string & id, string & pwd)
{
	uint16 result = 2; // account not found
	auto_ptr<OdbcCommand> dbCommand(m_dbConnection.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)id.c_str(), id.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pwd.c_str(), pwd.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &result);

	if (!dbCommand->Prepare(_T("{CALL MAIN_LOGIN(?, ?, ?)}")))
		m_pMain->ReportSQLError(m_dbConnection.GetError());

	return result;
}
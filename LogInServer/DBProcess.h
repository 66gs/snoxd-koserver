#pragma once

#include "../shared/database/OdbcConnection.h"

class CVersionManagerDlg;
class CDBProcess  
{
public:
	CDBProcess();

	bool Connect(TCHAR *szDSN, TCHAR *szUser, TCHAR *szPass);

	bool LoadVersionList();
	bool LoadUserCountList();

	uint16 AccountLogin(string & id, string & pwd);

	OdbcConnection m_dbConnection;
	CVersionManagerDlg* m_pMain;
};
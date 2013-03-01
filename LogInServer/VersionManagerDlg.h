#pragma once

#include "DBProcess.h"
#include "VersionManager.h"
#include "../shared/STLMap.h"
#include "../shared/Ini.h"
#include "User.h"

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#include "../shared/KOSocketMgr.h"

/////////////////////////////////////////////////////////////////////////////
// CVersionManagerDlg dialog

typedef std::map <std::string, _VERSION_INFO*> VersionInfoList;
typedef std::vector <_SERVER_INFO*>	ServerInfoList;

class CVersionManagerDlg : public CDialog
{
	friend class CDBProcess;
public:
	CVersionManagerDlg(CWnd* pParent = NULL);	// standard constructor

	__forceinline short GetVersion() { return m_sLastVersion; };
	__forceinline char * GetFTPUrl() { return m_strFtpUrl; };
	__forceinline char * GetFTPPath() { return m_strFilePath; };

	__forceinline News * GetNews() { return &m_news; };

	__forceinline VersionInfoList* GetPatchList() { return &m_VersionList; };
	__forceinline ServerInfoList* GetServerList() { return &m_ServerList; };

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();

	afx_msg void OnBnClickedExit();

	static KOSocketMgr<LoginSession> s_socketMgr;

private:
	void GetInfoFromIni();
	void ReportSQLError(OdbcError *pError);

// Implementation
protected:
	HICON m_hIcon;

	// Dialog Data
	//{{AFX_DATA(CVersionManagerDlg)
	enum { IDD = IDD_VERSIONMANAGER_DIALOG };
	CListBox	m_OutputList;
	//}}AFX_DATA

	// Generated message map functions
	//{{AFX_MSG(CVersionManagerDlg)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	char	m_strFtpUrl[256], m_strFilePath[256];
	char	m_ODBCName[32], m_ODBCLogin[32], m_ODBCPwd[32];
	short	m_sLastVersion;

	VersionInfoList		m_VersionList;
	ServerInfoList		m_ServerList;

	CIni m_Ini;
	News m_news;

	RWLock m_serverListLock;
	RWLock m_patchListLock;

public:
	CDBProcess	m_DBProcess;
};

extern CVersionManagerDlg *g_pMain;

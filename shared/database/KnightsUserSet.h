#pragma once

#define T		_KNIGHTS_USER
#define MapType	KnightsUserArray

class CKnightsUserSet : public CMyRecordSet<T>
{
public:
	CKnightsUserSet(CKnightsManager *pManager, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_KnightsManager(pManager)
	{
		m_nFields = 2;
	}

	DECLARE_DYNAMIC(CKnightsUserSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[KNIGHTS_USER]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[sIDNum]"), m_sIDNum);
		RFX_Text(pFX, _T("[strUserID]"), m_strUserID);
	};

	virtual void HandleRead()
	{
		m_strUserID.TrimRight();
		m_KnightsManager->AddKnightsUser(m_sIDNum, C2A(m_strUserID));
	};

private:
	CKnightsManager * m_KnightsManager;
	int m_sIDNum;
	CString m_strUserID;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CKnightsUserSet, CRecordset)
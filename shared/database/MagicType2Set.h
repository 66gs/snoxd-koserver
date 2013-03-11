#pragma once

class CMagicType2Set : public OdbcRecordset
{
public:
	CMagicType2Set(OdbcConnection * dbConnection, Magictype2Array * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT iNum, HitType, HitRate, AddDamage, AddRange, NeedArrow FROM MAGIC_TYPE2"); }
	virtual void Fetch()
	{
		_MAGIC_TYPE2 *pData = new _MAGIC_TYPE2;

		_dbCommand->FetchUInt32(1, pData->iNum);
		_dbCommand->FetchByte(2, pData->bHitType);
		_dbCommand->FetchUInt16(3, pData->sHitRate);
		_dbCommand->FetchUInt16(4, pData->sAddDamage);
		_dbCommand->FetchByte(5, pData->bNeedArrow);

		if (!m_pMap->PutData(pData->iNum, pData))
			delete pData;
	}

	Magictype2Array *m_pMap;
};
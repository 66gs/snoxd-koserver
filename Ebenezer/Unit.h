#pragma once

/**
 * This class is a bridge between the CNpc & CUser classes
 * Currently it's excessively messier than it needs to be, 
 * because of Aujard and the whole _USER_DATA setup.
 *
 * This will be written out eventually, so we can do this properly.
 **/
struct _MAGIC_TABLE;
class Unit
{
public:
	Unit(bool bPlayer = false);

	__forceinline bool isPlayer() { return m_bPlayer; }
	__forceinline bool isNPC() { return !isPlayer(); }

	__forceinline C3DMap * GetMap() { return m_pMap; }

	virtual uint16 GetID() = 0;
	virtual uint8 GetZoneID() = 0;
	virtual float GetX() = 0;
	virtual float GetY() = 0;
	virtual float GetZ() = 0;

	__forceinline uint16 GetSPosX() { return uint16(GetX() * 10); };
	__forceinline uint16 GetSPosY() { return uint16(GetY() * 10); };
	__forceinline uint16 GetSPosZ() { return uint16(GetZ() * 10); };

	__forceinline uint16 GetRegionX() { return m_sRegionX; }
	__forceinline uint16 GetRegionZ() { return m_sRegionZ; }

	__forceinline uint16 GetNewRegionX() { return (uint16)(GetX()) / VIEW_DISTANCE; }
	__forceinline uint16 GetNewRegionZ() { return (uint16)(GetZ()) / VIEW_DISTANCE; }

	__forceinline CRegion * GetRegion() { return m_pRegion; }
	__forceinline void SetRegion(uint16 x = -1, uint16 z = -1) 
	{
		m_sRegionX = x; m_sRegionZ = z; 
		m_pRegion = m_pMap->GetRegion(x, z); // TO-DO: Clean this up
	}

	virtual const char * GetName() = 0; // this is especially fun...

	virtual uint8 GetNation() = 0;
	virtual uint8 GetLevel() = 0;

	virtual bool isDead() = 0;
	virtual bool isAlive() { return !isDead(); }

	virtual void GetInOut(Packet & result, uint8 bType) = 0;

	bool RegisterRegion();
	void RemoveRegion(int16 del_x, int16 del_z);
	void InsertRegion(int16 insert_x, int16 insert_z);

	short GetDamage(Unit *pTarget, _MAGIC_TABLE *pSkill);
	short GetMagicDamage(int damage, Unit *pTarget);
	short GetACDamage(int damage, Unit *pTarget);
	uint8 GetHitRate(float rate);

	void SendToRegion(Packet *result);

	void InitType3();
	void InitType4();

	void OnDeath(Unit *pKiller);
	void SendDeathAnimation();

// public for the moment
// protected:
	C3DMap * m_pMap;
	CRegion * m_pRegion;

	uint16 m_sRegionX, m_sRegionZ; // this is probably redundant

	bool m_bPlayer;

	short	m_sTotalHit;
	short	m_sTotalAc;
	float	m_sTotalHitrate;
	float	m_sTotalEvasionrate;

	short   m_sACAmount;
	BYTE    m_bAttackAmount;
	short	m_sMaxHPAmount;
	BYTE	m_bHitRateAmount;
	short	m_sAvoidRateAmount;

	BYTE	m_bAttackSpeedAmount;		// For Character stats in Type 4 Durational Spells.
	BYTE    m_bSpeedAmount;

	BYTE	m_bFireR;
	BYTE	m_bFireRAmount;
	BYTE	m_bColdR;
	BYTE	m_bColdRAmount;
	BYTE	m_bLightningR;
	BYTE	m_bLightningRAmount;
	BYTE	m_bMagicR;
	BYTE	m_bMagicRAmount;
	BYTE	m_bDiseaseR;
	BYTE	m_bDiseaseRAmount;
	BYTE	m_bPoisonR;
	BYTE	m_bPoisonRAmount;	

	BYTE    m_bMagicTypeLeftHand;			// The type of magic item in user's left hand  
	BYTE    m_bMagicTypeRightHand;			// The type of magic item in user's right hand
	short   m_sMagicAmountLeftHand;         // The amount of magic item in user's left hand
	short	m_sMagicAmountRightHand;        // The amount of magic item in user's left hand

	short   m_sDaggerR;						// Resistance to Dagger
	short   m_sSwordR;						// Resistance to Sword
	short	m_sAxeR;						// Resistance to Axe
	short	m_sMaceR;						// Resistance to Mace
	short	m_sSpearR;						// Resistance to Spear
	short	m_sBowR;						// Resistance to Bow		


	float	m_fHPLastTime[MAX_TYPE3_REPEAT];		// For Automatic HP recovery and Type 3 durational HP recovery.
	float	m_fHPStartTime[MAX_TYPE3_REPEAT];
	short	m_bHPAmount[MAX_TYPE3_REPEAT];
	BYTE	m_bHPDuration[MAX_TYPE3_REPEAT];
	BYTE	m_bHPInterval[MAX_TYPE3_REPEAT];
	short	m_sSourceID[MAX_TYPE3_REPEAT];
	BOOL	m_bType3Flag;

	short   m_sDuration[MAX_TYPE4_BUFF];
	float   m_fStartTime[MAX_TYPE4_BUFF];

	BYTE	m_bType4Buff[MAX_TYPE4_BUFF];
	BOOL	m_bType4Flag;
};

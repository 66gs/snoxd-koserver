#pragma once

#include "define.h"
#include "Unit.h"

class CEbenezerDlg;
class CNpc  : public Unit
{
public:
	virtual uint16 GetID() { return m_sNid; }
	virtual const char * GetName() { return m_strName; }

	uint16	m_sNid; // NPC ID
	uint16	m_sSid; // prototype ID

	short	m_sPid;				// MONSTER(NPC) Picture ID
	short	m_sSize;			// MONSTER(NPC) Size
	int		m_iWeapon_1;
	int		m_iWeapon_2;
	char	m_strName[MAX_NPC_SIZE+1];		// MONSTER(NPC) Name
	int		m_iMaxHP;			// �ִ� HP
	int		m_iHP;				// ���� HP
	BYTE	m_byState;			// ���� (NPC) ����
	BYTE	m_tNpcType;			// NPC Type
								// 0 : Normal Monster
								// 1 : NPC
								// 2 : �� �Ա�,�ⱸ NPC
								// 3 : ���
	int   m_iSellingGroup;		// ItemGroup
//	DWORD	m_dwStepDelay;		

	BYTE	m_NpcState;			// NPC�� ���� - ��Ҵ�, �׾���, ���ִ� ���...
	BYTE	m_byGateOpen;		// Gate Npc Status -> 1 : open 0 : close

	BYTE    m_byObjectType;     // ������ 0, objectŸ��(����, ����)�� 1
	int16	m_byDirection;

	uint8   m_byTrapNumber;

public:
	CNpc();


	virtual void Initialize();
	void MoveResult(float xpos, float ypos, float zpos, float speed);
	virtual void GetInOut(Packet & result, uint8 bType);
	void SendInOut(uint8 bType, float fx, float fz, float fy);
	void GetNpcInfo(Packet & pkt);

	void SendGateFlag(BYTE bFlag = -1, bool bSendAI = true);

	virtual void HpChange(int amount, Unit *pAttacker = NULL, bool bSendToAI = true); 
	virtual void MSpChange(int amount);

	virtual void OnDeath(Unit *pKiller);

	virtual bool isDead() { return m_NpcState == NPC_DEAD || m_iHP <= 0; };

	__forceinline bool isGate() { return GetType() == NPC_GATE || GetType() == NPC_PHOENIX_GATE || GetType() == NPC_SPECIAL_GATE || GetType() == NPC_VICTORY_GATE; };
	__forceinline bool isGateOpen() { return m_byGateOpen == TRUE; };
	__forceinline bool isGateClosed() { return !isGateOpen(); };

	__forceinline short GetEntryID() { return m_sSid; };
	__forceinline BYTE GetType() { return m_tNpcType; };
	__forceinline BYTE GetState() { return m_byState; };

	virtual ~CNpc();
};
// Npc.h: interface for the CNpc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NPC_H__1DE71CDD_4040_4479_828D_E8EE07BD7A67__INCLUDED_)
#define AFX_NPC_H__1DE71CDD_4040_4479_828D_E8EE07BD7A67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "define.h"

class CEbenezerDlg;
class C3DMap;

class CNpc  
{
public:
	CEbenezerDlg* m_pMain;

	short	m_sNid;				// NPC (��������)�Ϸù�ȣ
	short	m_sSid;				// NPC ���̺� ������ȣ
	BYTE	m_bCurZone;			// Current Zone;

	C3DMap * m_pMap;

	float	m_fCurX;			// Current X Pos;
	float	m_fCurY;			// Current Y Pos;
	float	m_fCurZ;			// Current Z Pos;
	short	m_sPid;				// MONSTER(NPC) Picture ID
	short	m_sSize;			// MONSTER(NPC) Size
	int		m_iWeapon_1;
	int		m_iWeapon_2;
	TCHAR	m_strName[MAX_NPC_SIZE+1];		// MONSTER(NPC) Name
	int		m_iMaxHP;			// �ִ� HP
	int		m_iHP;				// ���� HP
	BYTE	m_byState;			// ���� (NPC) ����
	BYTE	m_byGroup;			// �Ҽ� ����
	BYTE	m_byLevel;			// ����
	BYTE	m_tNpcType;			// NPC Type
								// 0 : Normal Monster
								// 1 : NPC
								// 2 : �� �Ա�,�ⱸ NPC
								// 3 : ���
	int   m_iSellingGroup;		// ItemGroup
//	DWORD	m_dwStepDelay;		

	short m_sRegion_X;			// region x position
	short m_sRegion_Z;			// region z position
	BYTE	m_NpcState;			// NPC�� ���� - ��Ҵ�, �׾���, ���ִ� ���...
	BYTE	m_byGateOpen;		// Gate Npc Status -> 1 : open 0 : close
	short   m_sHitRate;			// ���� ������
	BYTE    m_byObjectType;     // ������ 0, objectŸ��(����, ����)�� 1
	BYTE	m_byDirection;

	short   m_byEvent;		    // This is for the quest. 

public:
	CNpc();


	void Initialize();
	void MoveResult(float xpos, float ypos, float zpos, float speed);
	void NpcInOut(BYTE Type, float fx, float fz, float fy);
	void GetNpcInfo(char *buff, int & send_index);
	void RegisterRegion();
	void RemoveRegion(int del_x, int del_z);
	void InsertRegion(int del_x, int del_z);
	int GetRegionNpcList(int region_x, int region_z, char *buff, int &t_count);

	void SendGateFlag(BYTE bFlag = -1, bool bSendAI = true);

	__forceinline bool isDead() { return m_NpcState == NPC_DEAD || m_iHP <= 0; };
	__forceinline bool isAlive() { return !isDead(); };

	__forceinline bool isGate() { return GetType() == NPC_GATE || GetType() == NPC_PHOENIX_GATE || GetType() == NPC_SPECIAL_GATE; };
	__forceinline bool isGateOpen() { return m_byGateOpen == TRUE; };
	__forceinline bool isGateClosed() { return !isGateOpen(); };

	__forceinline short GetID() { return m_sNid; };
	__forceinline short GetEntryID() { return m_sSid; };
	__forceinline BYTE GetType() { return m_tNpcType; };
	__forceinline BYTE getNation() { return m_byGroup; }; // NOTE: Need some consistency with naming
	__forceinline BYTE getZoneID() { return m_bCurZone; };

	__forceinline BYTE GetState() { return m_byState; };
	__forceinline C3DMap * GetMap() { return m_pMap; };

	virtual ~CNpc();
};

#endif // !defined(AFX_NPC_H__1DE71CDD_4040_4479_828D_E8EE07BD7A67__INCLUDED_)

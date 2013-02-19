// MAP.h: interface for the MAP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAP_H__BCC14687_C38F_4597_8522_2170ED077037__INCLUDED_)
#define AFX_MAP_H__BCC14687_C38F_4597_8522_2170ED077037__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../N3Base/N3ShapeMgr.h"
#include "../shared/STLMap.h"
#include "RoomEvent.h"

struct _ZONE_INFO
{
	int m_nServerNo;
	int m_nZoneNumber;
	char m_MapName[_MAX_PATH];
	BYTE m_byRoomEvent;
};

typedef CSTLMap <_OBJECT_EVENT>		ObjectEventArray;
typedef CSTLMap <CRoomEvent>		RoomEventArray;

class CRegion;
class CNpc;
class CUser;
//class CRoomEvent;

class CMapInfo					// �� ��ǥ�� ����
{
public:
	short	m_sEvent;			// ����ǥ�� �̺�Ʈ ��ȣ

	CMapInfo();
	virtual ~CMapInfo();
};

class MAP  
{
public:
	CN3ShapeMgr m_N3ShapeMgr;
	CMapInfo**		m_pMap;					// Ÿ���� ����(1�� : 4����)
	CRegion**		m_ppRegion;				// 64������ Ÿ������..
	//CRoomEvent*		m_pRoomEvent;
	CSize			m_sizeMap;				// ���� ũ��
	CSize			m_sizeRegion;			// ���� resion size
	int m_nZoneNumber;						// zone number
	int	m_nServerNo;
	CString m_MapName;
	int			m_nMapSize;		// Grid Unit ex) 4m
	float		m_fUnitDist;	// i Grid Distance
	float**		m_fHeight;
//	short		m_arDungeonBossMonster[MAX_DUNGEON_BOSS_MONSTER];
	BYTE		m_byRoomType;		// ���� �ʱ�ȭ����( 0:�ڵ����� �ʱ�ȭ, 1:�����̺�Ʈ ����(Ư�������� �Ϸ�� �ʱ�ȭ)
	BYTE		m_byRoomEvent;		// event room(0:empty, 1:use)
	BYTE		m_byRoomStatus;		// room status(1:������, 2:���� �ʱ�ȭ��, 3:���ʱ�ȭ �Ϸ�)
	BYTE		m_byInitRoomCount;	// room �ʱ�ȭ �ð��� ����(���Ϳ� ����ȭ�� ���߱� ����)
	ObjectEventArray m_ObjectEventArray;
	RoomEventArray	 m_arRoomEventArray;
	short	m_sKarusRoom;			// karus�� ������
	short	m_sElmoradRoom;			// elmorad�� ������

public:
	MAP();
	virtual ~MAP();

	void Initialize(_ZONE_INFO *pZone);

	BOOL LoadMap( HANDLE hFile );
	void LoadTerrain( HANDLE hFile );
	void LoadMapTile( HANDLE hFile );
	void LoadObjectEvent(HANDLE hFile);
	BOOL LoadRoomEvent( int zone_number );
	BOOL ObjectIntersect(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float z );

	BOOL RegionNpcRemove( int rx, int rz, int nid );
	void RegionNpcAdd( int rx, int rz, int nid );
	BOOL RegionUserRemove( int rx, int rz, int uid );
	void RegionUserAdd( int rx, int rz, int uid );
	BOOL RegionItemRemove( int rx, int rz, int itemid, int count, int index );
	void RegionItemAdd( int rx, int rz, int itemid, int count, int index );
	int  GetRegionUserSize(int rx, int rz);
	int  GetRegionNpcSize(int rx, int rz);

	int GetXRegionMax() {return m_sizeRegion.cx-1;};
	int GetZRegionMax() {return m_sizeRegion.cy-1;};

	int IsRoomCheck(float fx, float fz);	// �������� ���, ������ ������ġ�� ������ ��� ��ġ�� �ִ����� �Ǵ�
	BOOL IsRoomStatusCheck();

	BOOL IsMovable(int dest_x, int dest_y);
	void InitializeRoom();

	CRoomEvent* SetRoomEvent( int number );

protected:
	void RemoveMapData();

};

#endif // !defined(AFX_MAP_H__BCC14687_C38F_4597_8522_2170ED077037__INCLUDED_)


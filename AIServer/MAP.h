#pragma once

#include "../N3Base/N3ShapeMgr.h"
#include "../shared/STLMap.h"
#include "Extern.h"
#include "RoomEvent.h"

typedef CSTLMap <_OBJECT_EVENT>		ObjectEventArray;
typedef CSTLMap <CRoomEvent>		RoomEventArray;

#include "../shared/SMDFile.h"

class CRegion;
class CNpc;
class CUser;

// temporary
struct CSize
{
	CSize() : cx(0), cy(0) {}
	CSize(int cx, int cy) : cx(cx), cy(cy) {}
	int cx, cy;
};

class MAP  
{
public:
	// Passthru methods
	__forceinline int GetXRegionMax() { return m_smdFile->GetXRegionMax(); }
	__forceinline int GetZRegionMax() { return m_smdFile->GetZRegionMax(); }
	__forceinline short * GetEventIDs() { return m_smdFile->GetEventIDs(); }
	__forceinline int GetEventID(float x, float z) { return m_smdFile->GetEventID(x, z); }

	CRegion**		m_ppRegion;				// 64������ Ÿ������..
	int m_nZoneNumber;						// zone number
	int	m_nServerNo;
	std::string m_MapName;
	int			m_nMapSize;		// Grid Unit ex) 4m
	float		m_fUnitDist;	// i Grid Distance
	float*		m_fHeight;
	BYTE		m_byRoomType;		// ���� �ʱ�ȭ����( 0:�ڵ����� �ʱ�ȭ, 1:�����̺�Ʈ ����(Ư�������� �Ϸ�� �ʱ�ȭ)
	BYTE		m_byRoomEvent;		// event room(0:empty, 1:use)
	BYTE		m_byRoomStatus;		// room status(1:������, 2:���� �ʱ�ȭ��, 3:���ʱ�ȭ �Ϸ�)
	BYTE		m_byInitRoomCount;	// room �ʱ�ȭ �ð��� ����(���Ϳ� ����ȭ�� ���߱� ����)
	ObjectEventArray m_ObjectEventArray;
	RoomEventArray	 m_arRoomEventArray;
	short	m_sKarusRoom;			// karus�� ������
	short	m_sElmoradRoom;			// elmorad�� ������
	CSize m_sizeRegion, m_sizeMap;
public:
	MAP();
	virtual ~MAP();

	bool Initialize(_ZONE_INFO *pZone);

	BOOL LoadRoomEvent();
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

	int IsRoomCheck(float fx, float fz);	// �������� ���, ������ ������ġ�� ������ ��� ��ġ�� �ִ����� �Ǵ�
	BOOL IsRoomStatusCheck();

	BOOL IsMovable(int dest_x, int dest_y);
	void InitializeRoom();

	CRoomEvent* SetRoomEvent( int number );

protected:
	void RemoveMapData();

	SMDFile * m_smdFile;
};
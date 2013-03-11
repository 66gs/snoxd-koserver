#pragma once

#include "..\N3Base\N3ShapeMgr.h"
#include "Region.h"
#include "GameEvent.h"
#include "../shared/STLMap.h"
#include <set>

typedef CSTLMap <CGameEvent>		EventArray;
typedef CSTLMap <_OBJECT_EVENT>		ObjectEventArray;
typedef CSTLMap <_REGENE_EVENT>		ObjectRegeneArray;
typedef	CSTLMap <_WARP_INFO>		WarpArray;

class CUser;
class CEbenezerDlg;

class SMDFile
{
public:
	SMDFile();

	static SMDFile *Load(std::string mapName);

	bool LoadMap(FILE *fp);
	void LoadTerrain(FILE *fp);
	void LoadObjectEvent(FILE *fp);
	void LoadMapTile(FILE *fp);
	void LoadRegeneEvent(FILE *fp);
	void LoadWarpList(FILE *fp);

	BOOL IsValidPosition(float x, float z, float y);
	BOOL CheckEvent( float x, float z, CUser* pUser = NULL );
	BOOL ObjectCollision(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float y, float z );

	int GetEventID(float x, float z);

	__forceinline int GetXRegionMax() { return m_nXRegion - 1; }
	__forceinline int GetZRegionMax() { return m_nZRegion - 1; }

	__forceinline void IncRef() { m_ref++; }
	__forceinline void DecRef() { if (--m_ref == 0) delete this; }

	__forceinline _OBJECT_EVENT * GetObjectEvent(int objectindex) { return m_ObjectEventArray.GetData(objectindex); }
	__forceinline _REGENE_EVENT * GetRegeneEvent(int objectindex) { return m_ObjectRegeneArray.GetData(objectindex); }
	__forceinline _WARP_INFO * GetWarp(int warpID) { return m_WarpArray.GetData(warpID); }

	void GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries);

	virtual ~SMDFile();

private:
	int m_ref;

	short*		m_ppnEvent;
	WarpArray	m_WarpArray;

	ObjectEventArray	m_ObjectEventArray;
	ObjectRegeneArray	m_ObjectRegeneArray;

	CN3ShapeMgr m_N3ShapeMgr;

	float*		m_fHeight;

	int			m_nXRegion, m_nZRegion;

	int			m_nMapSize;		// Grid Unit ex) 4m
	float		m_fUnitDist;	// i Grid Distance

	typedef std::map<std::string, SMDFile *> SMDMap;
	static SMDMap s_loadedMaps;

	friend class C3DMap;
};

class C3DMap
{
public:
	// Passthru methods
	__forceinline int GetXRegionMax() { return m_smdFile->GetXRegionMax(); }
	__forceinline int GetZRegionMax() { return m_smdFile->GetZRegionMax(); }

	__forceinline BOOL IsValidPosition(float x, float z, float y) { return m_smdFile->IsValidPosition(x, z, y); }
	
	__forceinline _OBJECT_EVENT * GetObjectEvent(int objectindex) { return m_smdFile->GetObjectEvent(objectindex); }
	__forceinline _REGENE_EVENT * GetRegeneEvent(int objectindex) { return m_smdFile->GetRegeneEvent(objectindex); }
	__forceinline _WARP_INFO * GetWarp(int warpID) { return m_smdFile->GetWarp(warpID); }
	__forceinline void GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries) { m_smdFile->GetWarpList(warpGroup, warpEntries); }
	
	__forceinline bool isAttackZone() { return m_isAttackZone; }

	C3DMap();
	bool Initialize(_ZONE_INFO *pZone);
	CRegion * GetRegion(uint16 regionX, uint16 regionZ);
	BOOL LoadEvent();
	BOOL CheckEvent( float x, float z, CUser* pUser = NULL );
	BOOL RegionItemRemove(uint16 rx, uint16 rz, int bundle_index, int itemid, int count);
	BOOL RegionItemAdd(uint16 rx, uint16 rz, _ZONE_ITEM * pItem);
	BOOL ObjectCollision(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float y, float z );
	virtual ~C3DMap();

	EventArray	m_EventArray;

	int	m_nServerNo, m_nZoneNumber;
	float m_fInitX, m_fInitZ, m_fInitY;
	BYTE	m_bType;		// Zone Type : 1 -> common zone,  2 -> battle zone, 3 -> 24 hour open battle zone
	short	m_sMaxUser;
	bool m_isAttackZone;

	CRegion**	m_ppRegion;

	DWORD m_wBundle;	// Zone Item Max Count

	SMDFile *m_smdFile;
};
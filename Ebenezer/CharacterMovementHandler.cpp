#include "StdAfx.h"

void CUser::MoveProcess(Packet & pkt)
{
	ASSERT(GetMap() != NULL);
	if (m_bWarp || isDead()) 
		return;
		
	uint16 will_x, will_z, will_y, speed=0;
	float real_x, real_z, real_y;
	uint8 echo;

	pkt >> will_x >> will_z >> will_y >> speed >> echo;
	real_x = will_x/10.0f; real_z = will_z/10.0f; real_y = will_y/10.0f;

	if (GetMap()->IsValidPosition(real_x, real_z, real_y) == FALSE) 
		return;

	// TO-DO: Ensure this is checked properly to prevent speedhacking
	m_pUserData->m_curx = real_x;
	m_pUserData->m_curz = real_z;
	m_pUserData->m_cury = real_y;

	if (RegisterRegion())
	{
		g_pMain->RegionNpcInfoForMe(this);
		g_pMain->RegionUserInOutForMe(this);
	}

	Packet result(WIZ_MOVE);
	result << GetSocketID() << will_x << will_z << will_y << speed << echo;
	SendToRegion(&result);

	GetMap()->CheckEvent(real_x, real_z, this);

	result.Initialize(AG_USER_MOVE);
	result << GetSocketID() << m_pUserData->m_curx << m_pUserData->m_curz << m_pUserData->m_cury << speed;
	g_pMain->Send_AIServer(&result);
}

void CUser::GetInOut(Packet & result, uint8 bType)
{
	result.Initialize(WIZ_USER_INOUT);
	result << uint16(bType) << GetID();
	if (bType != INOUT_OUT)
		GetUserInfo(result);
}

void CUser::UserInOut(uint8 bType)
{
	if (GetRegion() == NULL)
		return;

	Packet result;
	GetInOut(result, bType);

	if (bType == INOUT_OUT)
		GetRegion()->Remove(this);
	else
		GetRegion()->Add(this);

	SendToRegion(&result, this);

	if (bType == INOUT_OUT || !isBlinking())
	{
		result.Initialize(AG_USER_INOUT);
		result.SByte();
		result << bType << GetSocketID() << m_pUserData->m_id << m_pUserData->m_curx << m_pUserData->m_curz;
		g_pMain->Send_AIServer(&result);
	}
}

void CUser::GetUserInfo(Packet & pkt)
{
	CKnights *pKnights = NULL;
	pkt.SByte();

	pkt		<< m_pUserData->m_id
			<< uint16(GetNation()) << m_pUserData->m_bKnights << getFame();

	pKnights = g_pMain->GetClanPtr(m_pUserData->m_bKnights);
	if (pKnights == NULL)
	{
		pkt	<< uint32(0) << uint16(0) << uint8(0) << uint16(-1) << uint32(0) << uint8(0);
	}
	else
	{
		pkt	<< uint16(pKnights->m_sAlliance)
				<< pKnights->m_strName
				<< pKnights->m_byGrade << pKnights->m_byRanking
				<< uint16(pKnights->m_sMarkVersion) // symbol/mark version
				<< uint16(pKnights->m_sCape) // cape ID
				<< pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0) // this is stored in 4 bytes after all.
				// not sure what this is, but it (just?) enables the clan symbol on the cape 
				// value in dump was 9, but everything tested seems to behave as equally well...
				// we'll probably have to implement logic to respect requirements.
				<< uint8(1); 
	}

	pkt	<< GetLevel() << m_pUserData->m_bRace << m_pUserData->m_sClass
		<< GetSPosX() << GetSPosZ() << GetSPosY()
		<< m_pUserData->m_bFace << m_pUserData->m_nHair
		<< m_bResHpType << uint32(m_bAbnormalType)
		<< m_bNeedParty
		<< m_pUserData->m_bAuthority
		<< m_bPartyLeader // is party leader (bool)
		<< m_bIsInvisible // visibility state
		<< uint8(0) // team colour (i.e. in soccer, 0=none, 1=blue, 2=red)
		<< uint8(0) // unknown, doesn't seem to do anything noticeable for a regular player or GM (tested with 0, 1, 2, 255)
		<< m_sDirection // direction 
		<< uint8(0) // chicken flag
		<< m_pUserData->m_bRank // king cape (this used to just be rank, above!?)
		<< int8(-1) << int8(-1) // NP ranks (total, monthly)
		<< m_pUserData->m_sItemArray[BREAST].nNum << m_pUserData->m_sItemArray[BREAST].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[LEG].nNum << m_pUserData->m_sItemArray[LEG].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[HEAD].nNum << m_pUserData->m_sItemArray[HEAD].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[GLOVE].nNum << m_pUserData->m_sItemArray[GLOVE].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[FOOT].nNum << m_pUserData->m_sItemArray[FOOT].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[SHOULDER].nNum << m_pUserData->m_sItemArray[SHOULDER].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[RIGHTHAND].nNum << m_pUserData->m_sItemArray[RIGHTHAND].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[LEFTHAND].nNum << m_pUserData->m_sItemArray[LEFTHAND].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CWING].nNum << m_pUserData->m_sItemArray[CWING].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CTOP].nNum << m_pUserData->m_sItemArray[CTOP].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CHELMET].nNum << m_pUserData->m_sItemArray[CHELMET].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CRIGHT].nNum << m_pUserData->m_sItemArray[CRIGHT].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CLEFT].nNum << m_pUserData->m_sItemArray[CLEFT].sDuration << uint8(0)
		<< GetZoneID() << uint8(-1) << uint8(-1) << uint16(0) << uint16(0) << uint16(0);
}

void CUser::Rotate(Packet & pkt)
{
	if (isDead())
		return;

	Packet result(WIZ_ROTATE);
	pkt >> m_sDirection;
	result << GetSocketID() << m_sDirection;
	SendToRegion(&result, this);
}

void CUser::ZoneChange(int zone, float x, float z)
{
	m_bZoneChangeFlag = TRUE;

	C3DMap* pMap = NULL;
	_ZONE_SERVERINFO *pInfo = NULL;

	pMap = g_pMain->GetZoneByID(zone);
	if (!pMap) 
		return;

	if( pMap->m_bType == 2 ) {	// If Target zone is frontier zone.
		if( GetLevel() < 20 && g_pMain->m_byBattleOpen != SNOW_BATTLE)
			return;
	}

	if( g_pMain->m_byBattleOpen == NATION_BATTLE )	{		// Battle zone open
		if( m_pUserData->m_bZone == BATTLE_ZONE )	{
			if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone )	{	// ???? ?????? ???? ????..
				if( m_pUserData->m_bNation == KARUS && !g_pMain->m_byElmoradOpenFlag )	{
					TRACE("#### ZoneChange Fail ,,, id=%s, nation=%d, flag=%d\n", m_pUserData->m_id, m_pUserData->m_bNation, g_pMain->m_byElmoradOpenFlag);
					return;
				}
				else if( m_pUserData->m_bNation == ELMORAD && !g_pMain->m_byKarusOpenFlag )	{
					TRACE("#### ZoneChange Fail ,,, id=%s, nation=%d, flag=%d\n", m_pUserData->m_id, m_pUserData->m_bNation, g_pMain->m_byKarusOpenFlag);
					return;
				}
			}
		}
		else if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone ) {		// ???? ?????? ???? ????..
			return;
		}
//
		else if( pMap->m_bType == 2 && zone == ZONE_FRONTIER ) {	 // You can't go to frontier zone when Battlezone is open.
			int temp_index = 0;
			char temp_buff[3];

			SetByte( temp_buff, WIZ_WARP_LIST, temp_index );
			SetByte( temp_buff, 2, temp_index );
			SetByte( temp_buff,0, temp_index );
			Send(temp_buff, temp_index);
//
			return;
		}
//
	}
	else if( g_pMain->m_byBattleOpen == SNOW_BATTLE )	{					// Snow Battle zone open
		if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone ) {		// ???? ?????? ???? ????..
			return;
		}
		else if( pMap->m_bType == 2 && (zone == ZONE_FRONTIER || zone == ZONE_BATTLE ) ) {			// You can't go to frontier zone when Battlezone is open.
			return;
		}
	}
	else	{					// Battle zone close
		if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone && (zone < 10 || zone > 21))
			return;
	}

	m_bWarp = 0x01;

	UserInOut(INOUT_OUT);

	if( m_pUserData->m_bZone == ZONE_SNOW_BATTLE )	{
		//TRACE("ZoneChange - name=%s\n", m_pUserData->m_id);
		SetMaxHp( 1 );
	}

	m_pUserData->m_bZone = zone;
	m_pUserData->m_curx = x;
	m_pUserData->m_curz = z;

	m_pMap = pMap;

	if( m_pUserData->m_bZone == ZONE_SNOW_BATTLE )	{
		//TRACE("ZoneChange - name=%s\n", m_pUserData->m_id);
		SetMaxHp();
	}

	PartyRemove(GetSocketID());	// ??????? Z?????? �??

	//TRACE("ZoneChange ,,, id=%s, nation=%d, zone=%d, x=%.2f, z=%.2f\n", m_pUserData->m_id, m_pUserData->m_bNation, zone, x, z);
	
	if( g_pMain->m_nServerNo != pMap->m_nServerNo ) {
		pInfo = g_pMain->m_ServerArray.GetData( pMap->m_nServerNo );
		if( !pInfo ) 
			return;

		UserDataSaveToAgent();
		
		CTime t = CTime::GetCurrentTime();
		g_pMain->WriteLog("[ZoneChange : %d-%d-%d] - sid=%d, acname=%s, name=%s, zone=%d, x=%d, z=%d \r\n", t.GetHour(), t.GetMinute(), t.GetSecond(), GetSocketID(), m_strAccountID, m_pUserData->m_id, zone, (int)x, (int)z);

		m_pUserData->m_bLogout = 2;	// server change flag
		SendServerChange(pInfo->strServerIP, 2);
		return;
	}
	
	SetRegion(GetNewRegionX(), GetNewRegionZ());

	Packet result(WIZ_ZONE_CHANGE, uint8(3)); // magic numbers, sigh.
	result << uint16(GetZoneID()) << GetSPosX() << GetSPosZ() << GetSPosY() << g_pMain->m_byOldVictory;
	Send(&result);

	if (!m_bZoneChangeSameZone) {
		m_sWhoKilledMe = -1;
		m_iLostExp = 0;
		m_bRegeneType = 0;
		m_fLastRegeneTime = 0.0f;
		m_pUserData->m_sBind = -1;
		InitType3();
		InitType4();
	}	

	result.Initialize(AG_ZONE_CHANGE);
	result << GetSocketID() << GetZoneID();
	g_pMain->Send_AIServer(&result);

	m_bZoneChangeSameZone = FALSE;
	m_bZoneChangeFlag = FALSE;
}

void CUser::Warp(uint16 sPosX, uint16 sPosZ)
{
	ASSERT(GetMap() != NULL);
	if (m_bWarp)
		return;

	float real_x = sPosX / 10.0f, real_z = sPosZ / 10.0f;
	if (!GetMap()->IsValidPosition(real_x, real_z, 0.0f)) 
	{
		TRACE("Invalid position %f,%f\n", real_x, real_z);
		return;
	}

	Packet result(WIZ_WARP);
	result << sPosX << sPosZ;
	Send(&result);

	UserInOut(INOUT_OUT);

	m_pUserData->m_curx = real_x;
	m_pUserData->m_curz = real_z;

	SetRegion(GetNewRegionX(), GetNewRegionZ());

	UserInOut(INOUT_WARP);
	g_pMain->UserInOutForMe(this);
	g_pMain->NpcInOutForMe(this);
	g_pMain->MerchantUserInOutForMe(this);

	ResetWindows();
}

void CUser::RecvWarp(Packet & pkt)
{
	uint16 warp_x, warp_z;
	pkt >> warp_x >> warp_z;
	Warp(warp_x, warp_z);	
}

void CUser::RecvZoneChange(Packet & pkt)
{
	if (isDead()) // we also need to make sure we're actually waiting on this request...
		return;

	uint8 opcode = pkt.read<uint8>();
	if (opcode == 1)
	{
		g_pMain->UserInOutForMe(this);
		g_pMain->NpcInOutForMe(this);
		g_pMain->MerchantUserInOutForMe(this);
		
		Packet result(WIZ_ZONE_CHANGE, uint8(2)); // finalise the zone change
		Send(&result);
	}
	else if (opcode == 2)
	{
		UserInOut(INOUT_RESPAWN);

		// TO-DO: Fix all this up (it's too messy/confusing)
		if (!m_bZoneChangeSameZone)
		{
			BlinkStart();
			// TO-DO: 'recast' buffs (otherwise they're lost to the client on zone change)
		}

		m_bZoneChangeFlag = 0;
		m_bWarp = 0;
	}
}


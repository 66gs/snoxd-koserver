// MagicProcess.cpp: implementation of the CMagicProcess class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MagicProcess.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "Npc.h"
#include "AiPacket.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MORAL_SELF				1		// �� �ڽ�..
#define MORAL_FRIEND_WITHME		2		// ���� ������ �츮��(����) �� �ϳ� ..
#define MORAL_FRIEND_EXCEPTME	3		// ���� �� �츮�� �� �ϳ� 
#define MORAL_PARTY				4		// ���� ������ �츮��Ƽ �� �ϳ�..
#define MORAL_NPC				5		// NPC�� �ϳ�.
#define MORAL_PARTY_ALL			6		// ���� ȣ���� ��Ƽ ����..
#define MORAL_ENEMY				7		// ������ ������ ���� ���� �ϳ�(NPC����)
#define MORAL_ALL				8		// �׻��� �����ϴ� ���� ���� �ϳ�.
#define MORAL_AREA_ENEMY		10		// ������ ���Ե� ��
#define MORAL_AREA_FRIEND		11		// ������ ���Ե� �츮��
#define MORAL_AREA_ALL			12		// ������ ���Ե� ����
#define MORAL_SELF_AREA			13		// ���� �߽����� �� ����
// �񷯸ӱ� Ŭ����ȯ
#define MORAL_CLAN				14		// Ŭ�� �ɹ� �� �Ѹ�...
#define MORAL_CLAN_ALL			15		// ���� ������ Ŭ�� �ɹ� ��...
//

#define MORAL_UNDEAD			16		// Undead Monster
#define MORAL_PET_WITHME		17      // My Pet
#define MORAL_PET_ENEMY			18		// Enemy's Pet
#define MORAL_ANIMAL1			19		// Animal #1
#define MORAL_ANIMAL2			20		// Animal #2
#define MORAL_ANIMAL3			21		// Animal #3
#define MORAL_ANGEL				22		// Angel
#define MORAL_DRAGON			23		// Dragon
#define MORAL_CORPSE_FRIEND     25		// A Corpse of the same race.
#define MORAL_CORPSE_ENEMY      26		// A Corpse of a different race.

#define WARP_RESURRECTION		1		// To the resurrection point.

#define REMOVE_TYPE3			1
#define REMOVE_TYPE4			2
#define RESURRECTION			3
#define	RESURRECTION_SELF		4
#define REMOVE_BLESS			5

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMagicProcess::CMagicProcess()
{
	m_pSrcUser = NULL;
	m_pMain = NULL;
	m_bMagicState = NONE; 
}

CMagicProcess::~CMagicProcess()
{
}

void CMagicProcess::MagicPacket(Packet & pkt)
{
	/* TO-DO: Update the main method with this... eventually. */
	char *tmp_buff = new char[pkt.size() + 1];
	tmp_buff[0] = pkt.GetOpcode();
	if (pkt.size() > 0)
		memcpy(tmp_buff + 1, pkt.contents(), pkt.size()); 
	MagicPacket(tmp_buff);
	delete [] tmp_buff;
}

void CMagicProcess::MagicPacket(char *pBuf)
{
	int index = 0, send_index = 0, magicid = 0, sid = -1, tid = -2, data1 = 0, data2 = 0, data3 = 0, data4 = 0, data5 = 0, data6 = 0, data7 = 0,data8 = 0, type3_attribute = 0,NpcMagic = 0;
	char send_buff[128];

	_MAGIC_TABLE* pTable = NULL;
	CNpc* pMon = NULL;

	BYTE command = GetByte( pBuf, index );		// Get the magic status.  
	magicid = GetDWORD( pBuf, index );          // Get ID of magic.

	if (command == MAGIC_CANCEL2 &&  m_pSrcUser) {
		Type3Cancel(magicid, m_pSrcUser->GetSocketID());	 // Type 3 cancel procedure.
		Type4Cancel(magicid, m_pSrcUser->GetSocketID());   // Type 4 cancel procedure.
		//Type6Cancel(magicid, m_pSrcUser);   // Scrolls etc.
		//Type9Cancel(magicid, m_pSrcUser);   // Stealth lupin etc.
		return;
	}

	sid = GetShort( pBuf, index );			    // Get ID of source.
	tid = GetShort( pBuf, index );              // Get ID of target.
	data1 = GetShort( pBuf, index );            // Additional Info :)
	data2 = GetShort( pBuf, index );		 
	data3 = GetShort( pBuf, index );
	data4 = GetShort( pBuf, index );
	data5 = GetShort( pBuf, index );
	data6 = GetShort( pBuf, index ); 
	data7 = GetShort( pBuf, index );
	data8 = GetShort( pBuf, index ); // new
	NpcMagic = GetByte( pBuf, index ); // new

	if (m_pSrcUser)
	{
		// We don't want the source ever being anyone other than us.
		// Ever. Even in the case of NPCs, it's BAD. BAD!
		// We're better than that -- we don't need to have the client tell NPCs what to do.
		if (sid != m_pSrcUser->GetSocketID()) 
			return;

		// If we're in a snow war, we're only ever allowed to use the snowball skill.
		if (m_pSrcUser->getZoneID() == ZONE_SNOW_BATTLE && m_pMain->m_byBattleOpen == SNOW_BATTLE 
			&& magicid != SNOW_EVENT_SKILL)
			return;
	}

	if (command == MAGIC_CANCEL) {
		Type3Cancel(magicid, sid);	 // Type 3 cancel procedure.
		Type4Cancel(magicid, sid);   // Type 4 cancel procedure.
		//Type6Cancel(magicid, m_pSrcUser);   // Scrolls etc.
		//Type9Cancel(magicid, m_pSrcUser);
		return;
	}

	_MAGIC_TABLE* pMagic = NULL ;
	pMagic = m_pMain->m_MagictableArray.GetData( magicid ) ;   // Get main magic table.
	if( !pMagic )
		return ;	

	if( pMagic->bMoral == 10 && tid != -1 )
		return;

	if ( sid >= NPC_BAND ) {
		pMon = m_pMain->m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) return;
	}
	else if( sid >=0 && sid < MAX_USER )	{
		CUser* pUser = m_pMain->GetUserPtr(sid);
		if (pUser == NULL || pUser->isDead())	
			return;
	}

	if (tid >= 0 && tid < MAX_USER) {	// Type 4 Repeat Check!!!
		if (pMagic->bType1 == 4) {
			if (pMagic->bMoral < 5) {
				CUser* pTUser = m_pMain->GetUserPtr(tid) ;
				if (pTUser == NULL) return;

				_MAGIC_TYPE4* pType4 = NULL;
				pType4 = m_pMain->m_Magictype4Array.GetData( magicid );     // Get magic skill table type 4.
				if ( !pType4 ) return ;

				if ( pTUser->m_bType4Buff[pType4->bBuffType - 1] > 0) {			
					SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
					SetByte( send_buff, MAGIC_FAIL, send_index );
					SetDWORD( send_buff, magicid, send_index );
					SetShort( send_buff, sid, send_index );
					SetShort( send_buff, tid, send_index );	
					SetShort( send_buff, 0, send_index );	
					SetShort( send_buff, 0, send_index );
					SetShort( send_buff, 0, send_index );
					SetShort( send_buff, -103, send_index );			
					SetShort( send_buff, 0, send_index );
					SetShort( send_buff, 0, send_index );

					if( m_bMagicState == CASTING ) {
						m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
					}
					else {			
						m_pSrcUser->Send( send_buff, send_index );				
					}
					m_bMagicState = NONE;
					return;     // Magic was a failure!						
				}
			}
		}
	}
	
	if (tid >= 0 && tid < MAX_USER) {		// Type 3 Repeat Check!!!
		if (pMagic->bType1 == 3) {
			if (pMagic->bMoral < 5) {
				CUser* pTUser = m_pMain->GetUserPtr(tid);
				if (pTUser == NULL) return;

				_MAGIC_TYPE3* pType3 = NULL;
				pType3 = m_pMain->m_Magictype3Array.GetData( magicid );     // Get magic skill table type 4.
				if ( !pType3 ) return ;

				if (pType3->sTimeDamage > 0) {
					for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {
						if (pTUser->m_bHPAmount[i] > 0) {
							SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
							SetByte( send_buff, MAGIC_FAIL, send_index );
							SetDWORD( send_buff, magicid, send_index );
							SetShort( send_buff, sid, send_index );
							SetShort( send_buff, tid, send_index );	
							SetShort( send_buff, 0, send_index );	
							SetShort( send_buff, 0, send_index );
							SetShort( send_buff, 0, send_index );
							SetShort( send_buff, -103, send_index );			
							SetShort( send_buff, 0, send_index );
							SetShort( send_buff, 0, send_index );

							if( m_bMagicState == CASTING ) {
								m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
							}
							else {			
								m_pSrcUser->Send( send_buff, send_index );				
							}
							m_bMagicState = NONE;
							return;     // Magic was a failure!	
						}
					}
				}				
			}
		}
	}

// �񷯸ӱ� Ŭ�� ��ȯ >.<
	if (sid >= 0 && sid < MAX_USER) {	// Make sure the source is a user!
		if (m_pSrcUser->m_pUserData->m_bZone == ZONE_BATTLE) {		// Make sure the zone is a battlezone!
			if (tid >= 0 && tid < MAX_USER) {		// Make sure the target is another player.
				if (pMagic->bType1 == 8) {		// Is it a warp spell?
					if (pMagic->bMoral < 5 || pMagic->bMoral == MORAL_CLAN) {		
						float currenttime;
						currenttime = TimeGet();

						CUser* pTUser = m_pMain->GetUserPtr(tid);
						if (pTUser == NULL) return;

						if (currenttime - pTUser->m_fLastRegeneTime < CLAN_SUMMON_TIME) {
							SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
							SetByte( send_buff, MAGIC_FAIL, send_index );
							SetDWORD( send_buff, magicid, send_index );
							SetShort( send_buff, sid, send_index );
							SetShort( send_buff, tid, send_index );	
							SetShort( send_buff, 0, send_index );	
							SetShort( send_buff, 0, send_index );
							SetShort( send_buff, 0, send_index );
							SetShort( send_buff, -103, send_index );			
							SetShort( send_buff, 0, send_index );
							SetShort( send_buff, 0, send_index ); 

							if( m_bMagicState == CASTING ) {
								m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
							}
							else {			
								m_pSrcUser->Send( send_buff, send_index );				
							}
							m_bMagicState = NONE;
							return;     // Magic was a failure!	
						}		
					}
				}
			}
		}
	}
//
	if( command == MAGIC_FAIL ) 	// Client indicates that magic failed. Just send back packet.
		goto return_echo;
	
	if ( command == MAGIC_FLYING ) {	// When the arrow starts flying....
	 	if (pMagic->bType1 == 2) {
			_MAGIC_TYPE2* pType = NULL;
			pType = m_pMain->m_Magictype2Array.GetData( magicid );     // Get magic skill table type 2.
			if( !pType ) return;

			if (sid >= 0 && sid < MAX_USER)	{	// If the PLAYER shoots an arrow.
				if (pMagic->bFlyingEffect > 0) {	// Only if Flying Effect is greater than 0.			
					int total_hit = m_pSrcUser->m_sTotalHit + m_pSrcUser->m_sItemHit ;
					int skill_mana = total_hit * pMagic->sMsp / 100 ;

					if( skill_mana > m_pSrcUser->m_pUserData->m_sMp ) {	// Reduce Magic Point!
						command = MAGIC_FAIL ;
						goto return_echo ; 
					}				
					m_pSrcUser->MSpChange( -(skill_mana) ) ;
				}

				if (!m_pSrcUser->RobItem(pMagic->iUseItem, pType->bNeedArrow)) { // Subtract Arrow!				
					command = MAGIC_FAIL ;
					goto return_echo ; 
				}
			}		
		}
		goto return_echo;
	}
	
	pTable = IsAvailable( magicid, tid, sid, command, data1, data2, data3 );     // If magic was successful...
	if( !pTable ) return;

	if( command == MAGIC_EFFECTING ) {
		int initial_result = 1;
		
		if (sid >= 0 && sid < MAX_USER) {
			if( tid >= NPC_BAND || (tid == -1 && (pMagic->bMoral == MORAL_AREA_ENEMY || pMagic->bMoral == MORAL_SELF_AREA))) {		// If the target is an NPC.
				int total_magic_damage = 0;

				SetByte( send_buff, AG_MAGIC_ATTACK_REQ, send_index );
				SetShort( send_buff, sid, send_index );
				SetByte( send_buff, command, send_index );
				SetShort( send_buff, tid, send_index );
				SetDWORD( send_buff, magicid, send_index );
				SetShort( send_buff, data1, send_index );
				SetShort( send_buff, data2, send_index );
				SetShort( send_buff, data3, send_index );
				SetShort( send_buff, data4, send_index );
				SetShort( send_buff, data5, send_index );
				SetShort( send_buff, data6, send_index );
				short total_cha = m_pSrcUser->m_pUserData->m_bCha + m_pSrcUser->m_sItemCham;
				SetShort( send_buff, total_cha, send_index);

				if( m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {	// Does the magic user have a staff?
					_ITEM_TABLE* pRightHand = NULL;
					pRightHand = m_pMain->m_ItemtableArray.GetData(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);

					if( pRightHand && m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum == 0 && pRightHand->m_bKind / 10 == WEAPON_STAFF) {					
//						total_magic_damage += pRightHand->m_sDamage ;
//						total_magic_damage += ((pRightHand->m_sDamage * 0.8f)+ (pRightHand->m_sDamage * m_pSrcUser->m_pUserData->m_bLevel) / 60);

						if (pMagic->bType1 == 3) {
//
							total_magic_damage += (int)((pRightHand->m_sDamage * 0.8f)+ (pRightHand->m_sDamage * m_pSrcUser->m_pUserData->m_bLevel) / 60);
//
							_MAGIC_TYPE3* pType3 = NULL;
							pType3 = m_pMain->m_Magictype3Array.GetData( magicid );     // Get magic skill table type 4.
							if ( !pType3 ) return ;
  
							if (m_pSrcUser->m_bMagicTypeRightHand == pType3->bAttribute ) {							
//								total_magic_damage += pRightHand->m_sDamage ;
								total_magic_damage += (int)((pRightHand->m_sDamage * 0.8f) + (pRightHand->m_sDamage * m_pSrcUser->m_pUserData->m_bLevel) / 30);
							}
//
							if (pType3->bAttribute == 4) {	// Remember what Sunglae told ya!
								total_magic_damage = 0;
							}
//
						}

						SetShort(send_buff, total_magic_damage, send_index);						
					}
					else {
						SetShort(send_buff, 0, send_index);
					}
				}
				else {	// If not, just use the normal formula :)
					SetShort(send_buff, 0, send_index);
				}				

				m_pMain->Send_AIServer(send_buff, send_index);		
			}
		}

		if (tid < -1 || tid >= MAX_USER) return;	// Make sure the target is another player and it exists.

		switch( pTable->bType1 ) {
			case 1:
				initial_result = ExecuteType1( pTable->iNum, sid, tid, data1, data2, data3 );
				break;
			case 2:
				initial_result = ExecuteType2( pTable->iNum, sid, tid, data1, data2, data3 );	
				break;
			case 3:
				ExecuteType3( pTable->iNum, sid, tid, data1, data2, data3 );
				break;
			case 4:
				ExecuteType4( pTable->iNum, sid, tid, data1, data2, data3, data4 );
				break;
			case 5:
				ExecuteType5( pTable->iNum, sid, tid, data1, data2, data3  );
				break;
			case 6:
				ExecuteType6( pTable->iNum );
				break;
			case 7:
				ExecuteType7( pTable->iNum );
				break;
			case 8:
				ExecuteType8( pTable->iNum, sid, tid, data1, data2, data3 );
				break;
			case 9:
				ExecuteType9( pTable->iNum );
				break;
			case 10:
				ExecuteType10( pTable->iNum );
				break;
		}

		if (initial_result != 0) {
			switch( pTable->bType2 ) {
				case 1:
					ExecuteType1( pTable->iNum, sid, tid, data1, data2, data3 );
					break;
				case 2:
					ExecuteType2( pTable->iNum, sid, tid, data1, data2, data3 );	
					break;
				case 3:
					ExecuteType3( pTable->iNum, sid, tid, data1, data2, data3 );
					break;
				case 4:
					ExecuteType4( pTable->iNum, sid, tid, data1, data2, data3, data4 );
					break;
				case 5:
					ExecuteType5( pTable->iNum, sid, tid, data1, data2, data3 );
					break;
				case 6:
					ExecuteType6( pTable->iNum );
					break;
				case 7:
					ExecuteType7( pTable->iNum );
					break;
				case 8:
					ExecuteType8( pTable->iNum, sid, tid, data1, data2, data3 );
					break;
				case 9:
					ExecuteType9( pTable->iNum );
					break;
				case 10:
					ExecuteType10( pTable->iNum );
					break;	
			}
		}
	}
	else if( command == MAGIC_CASTING ) {
		goto return_echo;		// ���� �����ٸ� �־���.....
	}

	return;

return_echo:
	SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
	SetByte( send_buff, command, send_index );
	SetDWORD( send_buff, magicid, send_index );
	SetShort( send_buff, sid, send_index );
	SetShort( send_buff, tid, send_index );
	SetShort( send_buff, data1, send_index );	
	SetShort( send_buff, data2, send_index );	
	SetShort( send_buff, data3, send_index );	
	SetShort( send_buff, data4, send_index );	
	SetShort( send_buff, data5, send_index );	
	SetShort( send_buff, data6, send_index );	

	if (sid >= 0 && sid < MAX_USER) {
		m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
	}
	else if (sid >= NPC_BAND) { 
		m_pMain->Send_Region( send_buff, send_index, pMon->GetMap(), pMon->m_sRegion_X, pMon->m_sRegion_Z );
	}		
}

_MAGIC_TABLE* CMagicProcess::IsAvailable(int magicid, int tid, int sid, BYTE type, int data1, int data2, int data3)
{
	CUser* pUser = NULL;	// When the target is a player....
	CUser* pParty = NULL;   // When the target is a party....
	CNpc* pNpc = NULL;		// When the monster is the target....
	CNpc* pMon = NULL;		// When the monster is the source....
	BOOL bFlag = FALSE;		// Identifies source : TRUE means source is NPC.
	_MAGIC_TYPE5* pType = NULL;		// Only for type 5 magic!
	
	int modulator = 0, Class = 0, send_index = 0, moral = 0 ;	// Variable Initialization.
	char send_buff[128] ;	
	int skill_mod = 0 ;

	_MAGIC_TABLE* pTable = NULL;
	pTable = m_pMain->m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pTable ) goto fail_return;   
	
	if ( sid >= 0 && sid < MAX_USER) {		// Check source validity when the source is a player.
		if (!m_pSrcUser) goto fail_return;
		if (m_pSrcUser->m_bAbnormalType == ABNORMAL_BLINKING) goto fail_return; 
	}
	else if( sid >= NPC_BAND) {	   //  Check source validity when the source is a NPC.
		bFlag = TRUE;
		pMon = m_pMain->m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) goto fail_return;
	}
	else goto fail_return;

	if( tid >= 0 && tid < MAX_USER ) {		// Target existence check routine for player.          
		pUser = m_pMain->GetUserPtr(tid);

		if (pTable->bType1 != 5) {		// If not a Warp/Resurrection spell...
			if (pUser == NULL || pUser->isDead() || pUser->m_bAbnormalType == ABNORMAL_BLINKING) {
				goto fail_return;
			}
		}
		else if (pTable->bType1 == 5) {
			pType = m_pMain->m_Magictype5Array.GetData(magicid);    
			if (!pType) goto fail_return;
			if (!pUser) goto fail_return;	

			if (pUser->m_bAbnormalType == ABNORMAL_BLINKING) goto fail_return;
			if (pUser->isDead() && pType->sNeedStone == 0 && pType->bExpRecover == 0) goto fail_return;					
		}

		moral = pUser->m_pUserData->m_bNation;
	}
	else if ( tid >= NPC_BAND ) {     // Target existence check routine for NPC.          	
		if( m_pMain->m_bPointCheckFlag == FALSE)	goto fail_return;	// ������ �����ϸ� �ȵ�
		pNpc = m_pMain->m_arNpcArray.GetData(tid);
		if( !pNpc || pNpc->m_NpcState == NPC_DEAD ) goto fail_return;	//... Assuming NPCs can't be resurrected.

		moral = pNpc->m_byGroup;
	}
	else if ( tid == -1) {  // Party Moral check routine.
		if (pTable->bMoral == MORAL_AREA_ENEMY) {
			if(!bFlag)	{
				if (m_pSrcUser->m_pUserData->m_bNation == 1) {	// Switch morals.
					moral = 2 ;
				}
				else {
					moral = 1 ;
				}
			}
			else {
				moral = 1 ;
			}
		}
		else {
			if(!bFlag)	{
				moral = m_pSrcUser->m_pUserData->m_bNation;	
			}
			else {
				moral = 1;
			}

		}
	}
	else moral = m_pSrcUser->m_pUserData->m_bNation ;
	
	switch( pTable->bMoral ) {		// Compare morals between source and target character.
		case MORAL_SELF:   // #1         // ( to see if spell is cast on the right target or not )
			if(bFlag) {
				if( tid != pMon->m_sNid ) goto fail_return;
			}
			else {
				if( tid != m_pSrcUser->GetSocketID() ) goto fail_return;
			}
			break;
		case MORAL_FRIEND_WITHME:	// #2
			if(bFlag) {
				if(pMon->m_byGroup != moral ) goto fail_return;
			}
			else {
				if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
			}
			break;
		case MORAL_FRIEND_EXCEPTME:	   // #3
			if(bFlag) {
				if( pMon->m_byGroup != moral ) goto fail_return;
				if( tid == pMon->m_sNid ) goto fail_return;				
			}
			else {
				if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
				if( tid == m_pSrcUser->GetSocketID() ) goto fail_return;
			}
			break;
		case MORAL_PARTY:	 // #4
			if( !m_pSrcUser->isInParty() && sid != tid) goto fail_return;
			if (m_pSrcUser->m_pUserData->m_bNation != moral) goto fail_return;
			if( pUser )
				if( pUser->m_sPartyIndex != m_pSrcUser->m_sPartyIndex ) goto fail_return;
			break;
		case MORAL_NPC:		// #5
			if( !pNpc ) goto fail_return;
			if( pNpc->m_byGroup != moral ) goto fail_return;
			break;
		case MORAL_PARTY_ALL:     // #6
//			if ( !m_pSrcUser->isInParty() ) goto fail_return;		
//			if ( !m_pSrcUser->isInParty() && sid != tid) goto fail_return;					

			break;
		case MORAL_ENEMY:	// #7	
			if(bFlag) {
				if( pMon->m_byGroup == moral ) goto fail_return;		
			}
			else {
				if( m_pSrcUser->m_pUserData->m_bNation == moral ) goto fail_return;	
			}
			break;	
		case MORAL_ALL:	 // #8
			// N/A
			break;
		case MORAL_AREA_ENEMY:		// #10
			// N/A
			break;
		case MORAL_AREA_FRIEND:		// #11
			if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
			break;
		case MORAL_AREA_ALL:	// #12
			// N/A
			break;
		case MORAL_SELF_AREA:     // #13
			// Remeber, EVERYONE in the area is affected by this one. No moral check!!!
			break;
		case MORAL_CORPSE_FRIEND:		// #25
			if(bFlag) {
				if( pMon->m_byGroup != moral ) goto fail_return;
				if( tid == pMon->m_sNid ) goto fail_return;				
			}
			else {
				if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
				if( tid == m_pSrcUser->GetSocketID() ) goto fail_return;
				if(pUser->m_bResHpType != USER_DEAD) goto fail_return; 
			}
			break;
//
		case MORAL_CLAN:		// #14
			if( m_pSrcUser->m_pUserData->m_bKnights == -1 && sid != tid ) goto fail_return;
			if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
			if( pUser ) {
				if( pUser->m_pUserData->m_bKnights != m_pSrcUser->m_pUserData->m_bKnights ) goto fail_return;
			}
			break;
			
		case MORAL_CLAN_ALL:	// #15
			break;
//
	}

	if(!bFlag) {	// If the user cast the spell (and not the NPC).....

/*	���߿� �ݵ��� �� �κ� ��ĥ�� !!!
		if( type == MAGIC_CASTING ) {    		
			if( m_bMagicState == CASTING && pTable->bType1 != 2 ) goto fail_return;		
			if( pTable->bCastTime == 0 )  goto fail_return;
			m_bMagicState = CASTING;
		}
		else if ( type == MAGIC_EFFECTING && pTable->bType1 != 2 ) {
			if( m_bMagicState == NONE  && pTable->bCastTime != 0 ) goto fail_return;
		} 
*/		
		modulator = pTable->sSkill % 10;     // Hacking prevention!
		if( modulator != 0 ) {	
			Class = pTable->sSkill / 10;
			if( Class != m_pSrcUser->m_pUserData->m_sClass ) goto fail_return;
			if( pTable->sSkillLevel > m_pSrcUser->m_pUserData->m_bstrSkill[modulator] ) goto fail_return;
		}
		else if( pTable->sSkillLevel > m_pSrcUser->m_pUserData->m_bLevel ) goto fail_return;

		if (pTable->bType1 == 1) {	// Weapons verification in case of COMBO attack (another hacking prevention).
			if (pTable->sSkill == 1055 || pTable->sSkill == 2055) {		// Weapons verification in case of DUAL ATTACK (type 1)!		
				_ITEM_TABLE* pLeftHand = NULL;		// Get item info for left hand.
				pLeftHand = m_pMain->m_ItemtableArray.GetData(m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum);
				if (!pLeftHand) return NULL;

				_ITEM_TABLE* pRightHand = NULL;     // Get item info for right hand.
				pRightHand = m_pMain->m_ItemtableArray.GetData(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);
				if (!pRightHand) return NULL;
				
				int left_index = pLeftHand->m_bKind / 10 ;
				int right_index = pRightHand->m_bKind / 10 ;

				if ((left_index != WEAPON_SWORD && left_index != WEAPON_AXE && left_index != WEAPON_MACE) 
					&& (right_index != WEAPON_SWORD  && right_index != WEAPON_AXE && right_index != WEAPON_MACE)) 
					return NULL ;
			}
			else if (pTable->sSkill == 1056 || pTable->sSkill == 2056) {	// Weapons verification in case of DOUBLE ATTACK !
				_ITEM_TABLE* pRightHand = NULL;     // Get item info for right hand.
				pRightHand = m_pMain->m_ItemtableArray.GetData(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);
				if (!pRightHand) return NULL;

				int right_index = pRightHand->m_bKind / 10 ;

				if(	(right_index != WEAPON_SWORD && right_index != WEAPON_AXE && right_index != WEAPON_MACE
						&& right_index != WEAPON_SPEAR) || m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 )
					return NULL ;
			}
		}

		if( type == MAGIC_EFFECTING ) {    // MP/SP SUBTRACTION ROUTINE!!! ITEM AND HP TOO!!!	
			int total_hit = m_pSrcUser->m_sTotalHit ;
			int skill_mana = (pTable->sMsp * total_hit) / 100 ; 

			if ( pTable->bType1 == 2 && pTable->bFlyingEffect != 0) {		// Type 2 related...
				m_bMagicState = NONE;
				return pTable;		// Do not reduce MP/SP when flying effect is not 0.
			}

			if ( pTable->bType1 == 1 && data1 > 1) {
				m_bMagicState = NONE;
				return pTable;		// Do not reduce MP/SP when combo number is higher than 0.
			}
 
			if( pTable->bType1 == 1 || pTable->bType1 == 2 ) {
				if( skill_mana > m_pSrcUser->m_pUserData->m_sMp )
					goto fail_return;
			}
			else{
				if( pTable->sMsp > m_pSrcUser->m_pUserData->m_sMp )
					goto fail_return;
			}
/*
			if ( pTable->bType1 == 1 || pTable->bType1 == 2 ) {	// Actual deduction of Skill or Magic point.
				m_pSrcUser->MSpChange( -(skill_mana) ) ;
			}	
			else if (pTable->bType1 != 4 || (pTable->bType1 == 4 && tid == -1)) {
				m_pSrcUser->MSpChange( -(pTable->sMsp) );
			}

			if (pTable->sHP > 0 && pTable->sMsp == 0) {			// DEDUCTION OF HPs in Magic/Skill using HPs.
				if (pTable->sHP > m_pSrcUser->m_pUserData->m_sHp) goto fail_return;
				m_pSrcUser->HpChange(-pTable->sHP);
			}
*/
			if (pTable->bType1 == 3 || pTable->bType1 == 4) {   // If the PLAYER uses an item to cast a spell.
				if (sid >= 0 && sid < MAX_USER)	{	
					if (pTable->iUseItem != 0) {
//	�̰͵� ������ ��û�� ���� �ϴ� ���Դϴ� --;
						_ITEM_TABLE* pItem = NULL;				// This checks if such an item exists.
						pItem = m_pMain->m_ItemtableArray.GetData(pTable->iUseItem);
						if( !pItem ) return FALSE;
						
						if (pItem->m_bRace != 0) {
							if (m_pSrcUser->m_pUserData->m_bRace != pItem->m_bRace) {
								type = MAGIC_CASTING;
								goto fail_return;
							}
						}

						if (pItem->m_bClass != 0) {
							if (!(m_pSrcUser->JobGroupCheck(pItem->m_bClass))) {
								type = MAGIC_CASTING;
								goto fail_return;
							}
						}
						
						if (pItem->m_bReqLevel != 0) {
							if (m_pSrcUser->m_pUserData->m_bLevel < pItem->m_bReqLevel) {
								type = MAGIC_CASTING;
								goto fail_return;
							}
						}
//
						if (!m_pSrcUser->RobItem(pTable->iUseItem, 1)) {					
							type = MAGIC_CASTING;
							goto fail_return;
						}
					}
				}
			}
			
			if (pTable->bType1 == 5) {
				if (tid >= 0 && tid < MAX_USER)	{	
					if (pTable->iUseItem != 0) {					
						pType = m_pMain->m_Magictype5Array.GetData(magicid);    
						if (!pType) goto fail_return;

						CUser* pTUser = m_pMain->GetUserPtr(tid);    
						if (!pTUser) goto fail_return;
// �񷯸ӱ� ��Ȱ --;
						if (pType->bType == 3 && pTUser->m_pUserData->m_bLevel <= 5) {	
							type = MAGIC_CASTING;	// No resurrections for low level users...
							goto fail_return; 							
						}
//
						if (pType->bType == 3) {
							if (!pTUser->RobItem(pTable->iUseItem, pType->sNeedStone)) {
								type = MAGIC_CASTING;
								goto fail_return; 
							}
						}
						else if (!m_pSrcUser->RobItem(pTable->iUseItem, pType->sNeedStone)) {
							type = MAGIC_CASTING;
							goto fail_return; 
						}
					}
				}
			}
//
			if ( pTable->bType1 == 1 || pTable->bType1 == 2 ) {	// Actual deduction of Skill or Magic point.
				m_pSrcUser->MSpChange( -(skill_mana) ) ;
			}	
			else if (pTable->bType1 != 4 || (pTable->bType1 == 4 && tid == -1)) {
				m_pSrcUser->MSpChange( -(pTable->sMsp) );
			}

			if (pTable->sHP > 0 && pTable->sMsp == 0) {			// DEDUCTION OF HPs in Magic/Skill using HPs.
				if (pTable->sHP > m_pSrcUser->m_pUserData->m_sHp) goto fail_return;
				m_pSrcUser->HpChange(-pTable->sHP);
			}
//
			m_bMagicState = NONE;	
		}
	}

	return pTable;      // Magic was successful! 

fail_return:    // In case the magic failed, just send a packet.
	send_index = 0;
	SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
	SetByte( send_buff, MAGIC_FAIL, send_index );
	SetDWORD( send_buff, magicid, send_index );
	SetShort( send_buff, sid, send_index );
	SetShort( send_buff, tid, send_index );	
	SetShort( send_buff, 0, send_index );	
	SetShort( send_buff, 0, send_index );
	SetShort( send_buff, 0, send_index );
	if( type == MAGIC_CASTING )
		SetShort( send_buff, -100, send_index );
	else
		SetShort( send_buff, 0, send_index );	
	SetShort( send_buff, 0, send_index );
	SetShort( send_buff, 0, send_index );

	if( m_bMagicState == CASTING ) {
		if (!bFlag) {
			m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
		}
		else {
			m_pMain->Send_Region( send_buff, send_index, pMon->GetMap(), pMon->m_sRegion_X, pMon->m_sRegion_Z );
		}
	}
	else {
		if (!bFlag) {
			m_pSrcUser->Send( send_buff, send_index );
		}	
	}
	m_bMagicState = NONE;
	return NULL;     // Magic was a failure!
}

BYTE CMagicProcess::ExecuteType1(int magicid, int sid, int tid, int data1, int data2, int data3)   // Applied to an attack skill using a weapon.
{	
	int damage = 0, send_index = 0, result = 1;     // Variable initialization. result == 1 : success, 0 : fail
	char send_buff[128];

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = m_pMain->m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return 0; 

	_MAGIC_TYPE1* pType = NULL;
	pType = m_pMain->m_Magictype1Array.GetData( magicid );     // Get magic skill table type 1.
	if( !pType ) {
		result = 0;
		return result;
	}

	damage = m_pSrcUser->GetDamage(tid, magicid);  // Get damage points of enemy.

	CUser* pTUser = m_pMain->GetUserPtr(tid);     // Get target info.  
	if (pTUser == NULL || pTUser->isDead())
	{
		result = 0;
		goto packet_send;
	}

	pTUser->HpChange( -damage );     // Reduce target health point.

	if( pTUser->m_pUserData->m_sHp == 0) {    // Check if the target is dead.
		pTUser->m_bResHpType = USER_DEAD;     // Target status is officially dead now.

		// sungyong work : loyalty

/* �������� ���� �ӽ÷� ��
//		pTUser->ExpChange( -pTUser->m_iMaxExp/100 );     // Reduce target experience.
		if( !m_pSrcUser->isInParty() )     // Something regarding loyalty points.
			m_pSrcUser->LoyaltyChange( (pTUser->m_pUserData->m_bLevel * pTUser->m_pUserData->m_bLevel) );
		else
			m_pSrcUser->LoyaltyDivide( (pTUser->m_pUserData->m_bLevel * pTUser->m_pUserData->m_bLevel) );
*/
		if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
			m_pSrcUser->LoyaltyChange(tid);
		}
		else {
			m_pSrcUser->LoyaltyDivide(tid);
		}

		m_pSrcUser->GoldChange(tid, 0);

		// �������� �Ϻ��� ��ȣ �ڵ�!!!
		pTUser->InitType3();	// Init Type 3.....
		pTUser->InitType4();	// Init Type 4.....
//
		if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
			pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
			//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
		}
//
		pTUser->m_sWhoKilledMe = sid;		// Who the hell killed me?
	} 
	m_pSrcUser->SendTargetHP( 0, tid, -damage );     // Change the HP of the target.

packet_send:
	if (pMagic->bType2 == 0 || pMagic->bType2 == 1) {
		SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
		SetByte( send_buff, MAGIC_EFFECTING, send_index );
		SetDWORD( send_buff, magicid, send_index );
		SetShort( send_buff, sid, send_index );
		SetShort( send_buff, tid, send_index );
		SetShort( send_buff, data1, send_index );	
		SetShort( send_buff, data2, send_index );	
		SetShort( send_buff, data3, send_index );	
		if (damage == 0) {
			SetShort( send_buff, -104, send_index );
		}
		else {
			SetShort( send_buff, 0, send_index );
		}

		m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
	}

	return result ;
}

BYTE CMagicProcess::ExecuteType2(int magicid, int sid, int tid, int data1, int data2, int data3)
{		
	int damage = 0, send_index = 0, result = 1 ; // Variable initialization. result == 1 : success, 0 : fail	
	char send_buff[128];

	float total_range = 0.0f;	// These variables are used for range verification!
	int sx, sz, tx, tz;

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = m_pMain->m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return 0;

	_ITEM_TABLE* pTable = NULL;		// Get item info.
	if (m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum) {
		pTable = m_pMain->m_ItemtableArray.GetData(m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum);
	}
	else{
		pTable = m_pMain->m_ItemtableArray.GetData(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);
	}

	if (!pTable) return 0;

	_MAGIC_TYPE2* pType = NULL;
	pType = m_pMain->m_Magictype2Array.GetData( magicid );     // Get magic skill table type 2.
	if( !pType ) return 0;

	CUser* pTUser = m_pMain->GetUserPtr(tid);
	if (pTUser == NULL || pTUser->isDead())
	{
		result = 0;
		goto packet_send;
	}
	
	total_range = pow(((pType->sAddRange * pTable->m_sRange) / 100.0f), 2.0f) ;     // Range verification procedure.
	sx = (int)m_pSrcUser->m_pUserData->m_curx; tx = (int)pTUser->m_pUserData->m_curx;
	sz = (int)m_pSrcUser->m_pUserData->m_curz; tz = (int)pTUser->m_pUserData->m_curz;
	
	if ((pow((float)(sx - tx), 2.0f) + pow((float)(sz - tz), 2.0f)) > total_range) {	   // Target is out of range, exit.
		result = 0;
		goto packet_send;
	}
	
	damage = m_pSrcUser->GetDamage(tid, magicid);  // Get damage points of enemy.	

	pTUser->HpChange( -damage );     // Reduce target health point.

	if( pTUser->m_pUserData->m_sHp == 0){     // Check if the target is dead.    
		pTUser->m_bResHpType = USER_DEAD;     // Target status is officially dead now.

		// sungyong work : loyalty

/* �������� ���� �ӽ÷� ��
//		pTUser->ExpChange( -pTUser->m_iMaxExp/100 );     // Reduce target experience.
		if( !m_pSrcUser->isInParty() )     // Something regarding loyalty points.
			m_pSrcUser->LoyaltyChange( (pTUser->m_pUserData->m_bLevel * pTUser->m_pUserData->m_bLevel) );
		else
			m_pSrcUser->LoyaltyDivide( (pTUser->m_pUserData->m_bLevel * pTUser->m_pUserData->m_bLevel) );
*/

		if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
			m_pSrcUser->LoyaltyChange(tid);
		}
		else {
			m_pSrcUser->LoyaltyDivide(tid);
		}

		m_pSrcUser->GoldChange(tid, 0);

		// �������� �Ϻ��� ��ȣ �ڵ�!!!
		pTUser->InitType3();	// Init Type 3.....
		pTUser->InitType4();	// Init Type 4.....
//
		if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
			pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
			//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
		}
//
		pTUser->m_sWhoKilledMe = sid;		// Who the hell killed me?
	} 

	m_pSrcUser->SendTargetHP( 0, tid, -damage );     // Change the HP of the target.

packet_send:
	if ( pMagic->bType2 == 0 || pMagic->bType2 == 2) {
		SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
		SetByte( send_buff, MAGIC_EFFECTING, send_index );
		SetDWORD( send_buff, magicid, send_index ); 
		SetShort( send_buff, sid, send_index );
		SetShort( send_buff, tid, send_index );
		SetShort( send_buff, data1, send_index );	
		SetShort( send_buff, result, send_index );	
		SetShort( send_buff, data3, send_index );	

		if (damage == 0) {
			SetShort( send_buff, -104, send_index );
		}
		else {
			SetShort( send_buff, 0, send_index );
		}

		m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
	}
	return result;
}

void CMagicProcess::ExecuteType3(int magicid, int sid, int tid, int data1, int data2, int data3)  // Applied when a magical attack, healing, and mana restoration is done.
{	
	int damage = 0, duration_damage = 0, send_index = 0, result = 1;     // Variable initialization. result == 1 : success, 0 : fail
	char send_buff[128], strLogData[128];
	BOOL bFlag = FALSE; 
	CNpc* pMon = NULL;

	vector<int> casted_member;

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = m_pMain->m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return;

	_MAGIC_TYPE3* pType = NULL;
	pType = m_pMain->m_Magictype3Array.GetData(magicid);      // Get magic skill table type 3.
	if( !pType ) return;

	if (sid >= NPC_BAND) {
		bFlag = TRUE;
		pMon = m_pMain->m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) return;
	}

	if (tid == -1) {		// If the target was the source's party....
		for (int i = 0 ; i < MAX_USER ; i++) {		// Maximum number of users in the server....
			CUser* pTUser = m_pMain->GetUnsafeUserPtr(i);
			if (pTUser == NULL || pTUser->m_bResHpType == USER_DEAD || pTUser->m_bAbnormalType == ABNORMAL_BLINKING) continue;
			
			if (UserRegionCheck(sid, i, magicid, pType->bRadius, data1, data3))
				casted_member.push_back(i);
		}

		if (casted_member.size() == 0) {		// If none of the members are in the region, return.				
			SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
			SetByte( send_buff, MAGIC_FAIL, send_index );
			SetDWORD( send_buff, magicid, send_index );
			SetShort( send_buff, sid, send_index );
			SetShort( send_buff, tid, send_index );	
			SetShort( send_buff, 0, send_index );	
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );

			if (!bFlag) {
				m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
			}
			else {
				m_pMain->Send_Region( send_buff, send_index, pMon->GetMap(), pMon->m_sRegion_X, pMon->m_sRegion_Z );
			}
		
			return;			
		}
	}
	else {		// If the target was another single player.
		CUser* pTUser = m_pMain->GetUserPtr(tid);
		if (pTUser == NULL) return;		
		
		casted_member.push_back(tid);
	}
	
	foreach (itr, casted_member)
	{
		CUser* pTUser = m_pMain->GetUserPtr(*itr);     // Get target info.  
		if (pTUser == NULL || pTUser->m_bResHpType == USER_DEAD) continue;

		if ((pType->sFirstDamage < 0) && (pType->bDirectType == 1) && (magicid < 400000)) {		// If you are casting an attack spell.
			damage = GetMagicDamage(sid, *itr, pType->sFirstDamage, pType->bAttribute) ;	// Get Magical damage point.
		}
		else damage = pType->sFirstDamage ;

		// ���ο����������� ���ο����̶��� ������ ���� ������ �͸� �����ϵ���,,,
		if( m_pSrcUser )	{
			if( m_pSrcUser->m_pUserData->m_bZone == ZONE_SNOW_BATTLE && m_pMain->m_byBattleOpen == SNOW_BATTLE )	{
				damage = -10;		
			}
		}

		if (pType->bDuration == 0) {     // Non-Durational Spells.
			if (pType->bDirectType == 1)     // Health Point related !
			{			
				pTUser->HpChange( damage );     // Reduce target health point.
				
				if( pTUser->m_pUserData->m_sHp == 0) {     // Check if the target is dead.		
					pTUser->m_bResHpType = USER_DEAD;

					// sungyong work : loyalty

					if (bFlag) {	// Killed by monster/NPC.
//
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
							//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
						}
						else {
//
							pTUser->ExpChange( -pTUser->m_iMaxExp/20 ); 					
//
						}
//
					}

					if (!bFlag) {	// Killed by another player.
						// ���ο����������� ���ο����̶��� ������ ���� ������ �͸� �����ϵ���,,,
						if( m_pSrcUser->m_pUserData->m_bZone == ZONE_SNOW_BATTLE && m_pMain->m_byBattleOpen == SNOW_BATTLE )	{
							m_pSrcUser->GoldGain( SNOW_EVENT_MONEY );	// 10000���Ƹ� �ִ� �κ�,,,,,

							wsprintf( strLogData, "%s -> %s userdead", m_pSrcUser->m_pUserData->m_id, pTUser->m_pUserData->m_id);
							m_pMain->WriteEventLog( strLogData );

							if( m_pSrcUser->m_pUserData->m_bZone == ZONE_SNOW_BATTLE && m_pMain->m_byBattleOpen == SNOW_BATTLE )	{
								if (pTUser->m_pUserData->m_bNation == KARUS) {
									m_pMain->m_sKarusDead++;
									//TRACE("++ ExecuteType3 - ka=%d, el=%d\n", m_pMain->m_sKarusDead, m_pMain->m_sElmoradDead);
								}
								else if (pTUser->m_pUserData->m_bNation == ELMORAD) {
									m_pMain->m_sElmoradDead++;
									//TRACE("++ ExecuteType3 - ka=%d, el=%d\n", m_pMain->m_sKarusDead, m_pMain->m_sElmoradDead);
								}
							}

						}
						else	{
							if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
								m_pSrcUser->LoyaltyChange(*itr);
							}
							else {
								m_pSrcUser->LoyaltyDivide(*itr);
							}

							m_pSrcUser->GoldChange(*itr, 0);
						}
					}

					// �������� �Ϻ��� ��ȣ �ڵ�!!!
					pTUser->InitType3();	// Init Type 3.....
					pTUser->InitType4();	// Init Type 4.....

					if (sid >= 0 && sid < MAX_USER) {
//
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
							//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
						}
//
						pTUser->m_sWhoKilledMe = sid;	// Who the hell killed me?
					}
				}

				if (!bFlag) m_pSrcUser->SendTargetHP( 0, *itr, damage ) ;     // Change the HP of the target.			
			}
			else if ( pType->bDirectType == 2 || pType->bDirectType == 3 )    // Magic or Skill Point related !
				pTUser->MSpChange(damage);     // Change the SP or the MP of the target.		
			else if( pType->bDirectType == 4 )     // Armor Durability related.
				pTUser->ItemWoreOut( DEFENCE, -damage);     // Reduce Slot Item Durability
		}
		else if (pType->bDuration != 0) {    // Durational Spells! Remember, durational spells only involve HPs.
			if (damage != 0) {		// In case there was first damage......
				pTUser->HpChange( damage );			// Initial damage!!!
		
				if( pTUser->m_pUserData->m_sHp == 0) {     // Check if the target is dead.	
					pTUser->m_bResHpType = USER_DEAD;

					// sungyong work : loyalty

					if (bFlag) {	// Killed by monster/NPC.
//
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
							//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
						}
						else {
//
							pTUser->ExpChange( -pTUser->m_iMaxExp / 20 );
//
						}
//
					}
					
					if (!bFlag) {	// Killed by another player.
						if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
							m_pSrcUser->LoyaltyChange(*itr);
						}
						else {
							m_pSrcUser->LoyaltyDivide(*itr);
						}

						m_pSrcUser->GoldChange(*itr, 0);
					}

					// �������� �Ϻ��� ��ȣ �ڵ� !!!
					pTUser->InitType3();	// Init Type 3.....
					pTUser->InitType4();	// Init Type 4..... 

					if (sid >= 0 && sid < MAX_USER) {
//
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
							//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
						}
//
						pTUser->m_sWhoKilledMe = sid;	// Who the hell killed me?
					}
				}
				if (!bFlag) m_pSrcUser->SendTargetHP( 0, *itr, damage );     // Change the HP of the target. 
			}

			if (pTUser->m_bResHpType != USER_DEAD) {	// ���⵵ ��ȣ �ڵ� �߽�...
				if (pType->sTimeDamage < 0) {
					duration_damage = GetMagicDamage(sid, *itr, pType->sTimeDamage, pType->bAttribute) ;
				}
				else duration_damage = pType->sTimeDamage ;

				for (int k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {	// For continuous damages...
					if (pTUser->m_bHPInterval[k] == 5) {
						pTUser->m_fHPStartTime[k] = pTUser->m_fHPLastTime[k] = TimeGet();     // The durational magic routine.
						pTUser->m_bHPDuration[k] = pType->bDuration;
						pTUser->m_bHPInterval[k] = 2;		
						pTUser->m_bHPAmount[k] = duration_damage / ( pTUser->m_bHPDuration[k] / pTUser->m_bHPInterval[k] ) ;
						pTUser->m_sSourceID[k] = sid;
						break;
					}
				}

				pTUser->m_bType3Flag = TRUE;
			}
//
			if (pTUser->isInParty() && pType->sTimeDamage < 0)
				pTUser->SendPartyStatusUpdate(1, 1);
//
		} 
	
		if ( pMagic->bType2 == 0 || pMagic->bType2 == 3 ) {
			SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
			SetByte( send_buff, MAGIC_EFFECTING, send_index );
			SetDWORD( send_buff, magicid, send_index );
			SetShort( send_buff, sid, send_index );
			SetShort( send_buff, *itr, send_index );
			SetShort( send_buff, data1, send_index );	
			SetShort( send_buff, result, send_index );	
			SetShort( send_buff, data3, send_index );	
			if (!bFlag) {
				m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
			}
			else {
				m_pMain->Send_Region( send_buff, send_index, pMon->GetMap(), pMon->m_sRegion_X, pMon->m_sRegion_Z ) ;
			}	
		}

		// Heal magic
		if( pType->bDirectType == 1 && damage > 0)	{
			if (!bFlag) {
				if (sid != tid) {		
					send_index = 0;
					SetByte( send_buff, AG_HEAL_MAGIC, send_index );
					SetShort( send_buff, sid, send_index );
					m_pMain->Send_AIServer(send_buff, send_index);
				}
			}
		}

		result = 1 ;
		send_index = 0 ;
	}
	
	return;		
}

void CMagicProcess::ExecuteType4(int magicid, int sid, int tid, int data1, int data2, int data3, int data4 )
{
	int damage = 0, send_index = 0, result = 1;     // Variable initialization. result == 1 : success, 0 : fail
	char send_buff[128];

	vector<int> casted_member;

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = m_pMain->m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return;

	_MAGIC_TYPE4* pType = NULL;
	pType = m_pMain->m_Magictype4Array.GetData( magicid );     // Get magic skill table type 4.
	if ( !pType ) return;

	if (tid == -1) {		// If the target was the source's party......		
		for (int i = 0 ; i < MAX_USER ; i++) {		// Maximum number of members in a party...
			CUser* pTUser = NULL ;     // Pointer initialization!		
			pTUser = m_pMain->GetUnsafeUserPtr(i);     // Get target info.  
			if (pTUser == NULL || pTUser->isDead() 
				|| pTUser->m_bAbnormalType == ABNORMAL_BLINKING) continue;
			
			if (UserRegionCheck(sid, i, magicid, pType->bRadius, data1, data3))
				casted_member.push_back(i);
		}

		if (casted_member.empty()) {		// If none of the members are in the region, return.
			SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
			SetByte( send_buff, MAGIC_FAIL, send_index );
			SetDWORD( send_buff, magicid, send_index );
			SetShort( send_buff, sid, send_index );
			SetShort( send_buff, tid, send_index );	
			SetShort( send_buff, 0, send_index );	
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			if (sid >= 0 && sid < MAX_USER) {
				m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
			}
			return ;	
		}
	}
	else {		// If the target was another single player.
		CUser* pTUser = m_pMain->GetUserPtr(tid);     // Get target info.  
		if (pTUser == NULL) return;     // Check if target exists		
		
		casted_member.push_back(tid);
	}

	foreach (itr, casted_member)
	{
		CUser* pTUser = m_pMain->GetUserPtr(*itr) ;     // Get target info.  
		if (pTUser == NULL || pTUser->isDead()) continue;
//
		if (pTUser->m_bType4Buff[pType->bBuffType - 1] == 2 && tid == -1) {		// Is this buff-type already casted on the player?
			result = 0 ;				// If so, fail! 
			goto fail_return ;					
		}
//
		//if ( data4 == -1 ) //Need to create InsertSaved Magic before enabling this.
			//pTUser->InsertSavedMagic( magicid, pType->sDuration );

		switch (pType->bBuffType) {	// Depending on which buff-type it is.....
			case 1 :
				pTUser->m_sMaxHPAmount = pType->sMaxHP;		// Get the amount that will be added/multiplied.
				pTUser->m_sDuration1 = pType->sDuration;	// Get the duration time.
				pTUser->m_fStartTime1 = TimeGet();			// Get the time when Type 4 spell starts.	
				break;
			case 2 :
				pTUser->m_sACAmount = pType->sAC;
				pTUser->m_sDuration2 = pType->sDuration;
				pTUser->m_fStartTime2 = TimeGet();
				break;
//
			case 3 : 
				// These really shouldn't be hardcoded
				if (magicid == 490034)	// Bezoar!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_GIANT); 
				else if (magicid == 490035)	// Rice Cake!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_DWARF); 

				pTUser->m_sDuration3 = pType->sDuration;
				pTUser->m_fStartTime3 = TimeGet();
				break;
//
			case 4 :
				pTUser->m_bAttackAmount = pType->bAttack;
				pTUser->m_sDuration4 = pType->sDuration;
				pTUser->m_fStartTime4 = TimeGet();					
				break;

			case 5 :
				pTUser->m_bAttackSpeedAmount = pType->bAttackSpeed;
				pTUser->m_sDuration5 = pType->sDuration;
				pTUser->m_fStartTime5 = TimeGet();
				break;

			case 6 :
				pTUser->m_bSpeedAmount = pType->bSpeed;
				pTUser->m_sDuration6 = pType->sDuration;
				pTUser->m_fStartTime6 = TimeGet();
				break;

			case 7 :
				pTUser->m_bStrAmount = pType->bStr;
				pTUser->m_bStaAmount = pType->bSta;
				pTUser->m_bDexAmount = pType->bDex;
				pTUser->m_bIntelAmount = pType->bIntel;
				pTUser->m_bChaAmount = pType->bCha;	
				pTUser->m_sDuration7 = pType->sDuration;
				pTUser->m_fStartTime7 = TimeGet();
				break;

			case 8 :
				pTUser->m_bFireRAmount = pType->bFireR;
				pTUser->m_bColdRAmount = pType->bColdR;
				pTUser->m_bLightningRAmount = pType->bLightningR;
				pTUser->m_bMagicRAmount = pType->bMagicR;
				pTUser->m_bDiseaseRAmount = pType->bDiseaseR;
				pTUser->m_bPoisonRAmount = pType->bPoisonR;
				pTUser->m_sDuration8 = pType->sDuration;
				pTUser->m_fStartTime8 = TimeGet();
				break;

			case 9 :
				pTUser->m_bHitRateAmount = pType->bHitRate;
				pTUser->m_sAvoidRateAmount = pType->sAvoidRate;
				pTUser->m_sDuration9 = pType->sDuration;
				pTUser->m_fStartTime9 = TimeGet();
				break;	

			default :
				result = 0 ;
				goto fail_return;
		}

		if (tid != -1 && pMagic->bType1 == 4) {
			if (sid >= 0 && sid < MAX_USER) {
				m_pSrcUser->MSpChange( -(pMagic->sMsp) );
			}
		}

		if (sid >= 0 && sid < MAX_USER) {
			if (m_pSrcUser->m_pUserData->m_bNation == pTUser->m_pUserData->m_bNation) {
				pTUser->m_bType4Buff[pType->bBuffType - 1] = 2;
			}
			else {
				pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;
			}
		}
		else {
			pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;
		}

		pTUser->m_bType4Flag = TRUE;

		pTUser->SetSlotItemValue();				// Update character stats.
		pTUser->SetUserAbility();

		if (pTUser->isInParty() && pTUser->m_bType4Buff[pType->bBuffType - 1] == 1)
			pTUser->SendPartyStatusUpdate(2, 1);

		pTUser->Send2AI_UserUpdateInfo();	// AI Server�� �م� ����Ÿ ����....

		if ( pMagic->bType2 == 0 || pMagic->bType2 == 4 ) {
			SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
			SetByte( send_buff, MAGIC_EFFECTING, send_index );
			SetDWORD( send_buff, magicid, send_index );
			SetShort( send_buff, sid, send_index );
			SetShort( send_buff, *itr, send_index );
			SetShort( send_buff, data1, send_index );	
			SetShort( send_buff, result, send_index );	
			SetShort( send_buff, data3, send_index );	
			SetShort( send_buff, pType->sDuration, send_index );
			SetShort( send_buff, 0x00, send_index );
			SetShort( send_buff, pType->bSpeed, send_index );

			if (sid >=0 && sid < MAX_USER) {
				m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
			}
			else {
				m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );
			}
		}

		send_index = 0;
		result = 1;	
		continue; 

	fail_return:
		if ( pMagic->bType2 == 4 ) {
			SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
			SetByte( send_buff, MAGIC_EFFECTING, send_index );
			SetDWORD( send_buff, magicid, send_index );
			SetShort( send_buff, sid, send_index );
			SetShort( send_buff, *itr, send_index );
			SetShort( send_buff, data1, send_index );	
			SetShort( send_buff, result, send_index );	
			SetShort( send_buff, data3, send_index );

			if( data4 != 0 )
				SetShort( send_buff, data4, send_index );
			else
				SetShort( send_buff, pType->sDuration, send_index );

			SetShort( send_buff, 0x00, send_index );
			SetShort( send_buff, pType->bSpeed, send_index );

			if (sid >= 0 && sid < MAX_USER) {
				m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
			}
			else {
				m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );
			}
			send_index = 0 ;
		}
		
		if (sid >= 0 && sid < MAX_USER) {
			SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
			SetByte( send_buff, MAGIC_FAIL, send_index );
			SetDWORD( send_buff, magicid, send_index );
			SetShort( send_buff, sid, send_index );
			SetShort( send_buff, *itr, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			SetShort( send_buff, 0, send_index );
			m_pSrcUser->Send( send_buff, send_index );
		}

		send_index = 0 ;
		result = 1;	
		continue;
	}
}

void CMagicProcess::ExecuteType5(int magicid, int sid, int tid, int data1, int data2, int data3 )
{
	int damage = 0, send_index = 0, result = 1;     // Variable initialization. result == 1 : success, 0 : fail
	char send_buff[128]; 
	int i = 0, j = 0, k =0; int buff_test = 0; BOOL bType3Test = TRUE; BOOL bType4Test = TRUE; 	

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = m_pMain->m_MagictableArray.GetData(magicid);   // Get main magic table.
	if( !pMagic ) return;

	_MAGIC_TYPE5* pType = NULL;
	pType = m_pMain->m_Magictype5Array.GetData(magicid);     // Get magic skill table type 4.
	if ( !pType ) return;

	CUser* pTUser = m_pMain->GetUserPtr(tid);
	if (pTUser == NULL || (pTUser->m_bResHpType == USER_DEAD && pType->bType != RESURRECTION) ||
				   (pTUser->m_bResHpType != USER_DEAD && pType->bType == RESURRECTION)) return;

	switch(pType->bType) {
		case REMOVE_TYPE3:		// REMOVE TYPE 3!!!
			for (i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {
				if (pTUser->m_bHPAmount[i] < 0) {
					pTUser->m_fHPStartTime[i] = 0.0f;
					pTUser->m_fHPLastTime[i] = 0.0f;   
					pTUser->m_bHPAmount[i] = 0;
					pTUser->m_bHPDuration[i] = 0;				
					pTUser->m_bHPInterval[i] = 5;
					pTUser->m_sSourceID[i] = -1;

					send_index = 0;
					SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
					SetByte( send_buff, MAGIC_TYPE3_END, send_index );	
					SetByte( send_buff, 200, send_index );		// REMOVE ALL!!!
					pTUser->Send( send_buff, send_index ); 
				}
			}

			buff_test = 0;
			for (j = 0 ; j < MAX_TYPE3_REPEAT ; j++) {
				buff_test += pTUser->m_bHPDuration[j];
			}
			if (buff_test == 0) pTUser->m_bType3Flag = FALSE;	
//
			// Check for Type 3 Curses.
			for (k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {
				if (pTUser->m_bHPAmount[k] < 0) {
					bType3Test = FALSE;
					break;
				}
			}
  
			if (pTUser->isInParty() && bType3Test)
				pTUser->SendPartyStatusUpdate(1);
			break;

		case REMOVE_TYPE4:		// REMOVE TYPE 4!!!
			if (pTUser->m_bType4Buff[0] == 1) {
				pTUser->m_sDuration1 = 0;		
				pTUser->m_fStartTime1 = 0.0f;
				pTUser->m_sMaxHPAmount = 0;
				pTUser->m_bType4Buff[0] = 0;

				SendType4BuffRemove(tid, 1);
			}

			if (pTUser->m_bType4Buff[1] == 1) {
				pTUser->m_sDuration2 = 0;		
				pTUser->m_fStartTime2 = 0.0f;
				pTUser->m_sACAmount = 0;
				pTUser->m_bType4Buff[1] = 0;
				
				SendType4BuffRemove(tid, 2);
			}

			if (pTUser->m_bType4Buff[3] == 1) {
				pTUser->m_sDuration4 = 0;		
				pTUser->m_fStartTime4 = 0.0f;
				pTUser->m_bAttackAmount = 100;
				pTUser->m_bType4Buff[3] = 0;

				SendType4BuffRemove(tid, 4);
			}

			if (pTUser->m_bType4Buff[4] == 1) {
				pTUser->m_sDuration5 = 0;		
				pTUser->m_fStartTime5 = 0.0f;
				pTUser->m_bAttackSpeedAmount = 100;	
				pTUser->m_bType4Buff[4] = 0;

				SendType4BuffRemove(tid, 5);
			}

			if (pTUser->m_bType4Buff[5] == 1) {
				pTUser->m_sDuration6 = 0;		
				pTUser->m_fStartTime6 = 0.0f;
				pTUser->m_bSpeedAmount = 100;
				pTUser->m_bType4Buff[5] = 0;				

				SendType4BuffRemove(tid, 6);
			}

			if (pTUser->m_bType4Buff[6] == 1) {
				pTUser->m_sDuration7 = 0;		
				pTUser->m_fStartTime7 = 0.0f;
				pTUser->m_bStrAmount = 0;
				pTUser->m_bStaAmount = 0;
				pTUser->m_bDexAmount = 0;
				pTUser->m_bIntelAmount = 0;
				pTUser->m_bChaAmount = 0;
				pTUser->m_bType4Buff[6] = 0;			

				SendType4BuffRemove(tid, 7);
			}

			if (pTUser->m_bType4Buff[7] == 1) {
				pTUser->m_sDuration8 = 0;		
				pTUser->m_fStartTime8 = 0.0f;
				pTUser->m_bFireRAmount = 0;
				pTUser->m_bColdRAmount = 0;
				pTUser->m_bLightningRAmount = 0;
				pTUser->m_bMagicRAmount = 0;
				pTUser->m_bDiseaseRAmount = 0;
				pTUser->m_bPoisonRAmount = 0;
				pTUser->m_bType4Buff[7] = 0;			

				SendType4BuffRemove(tid, 8);
			}

			if (pTUser->m_bType4Buff[8] == 1) {
				pTUser->m_sDuration9 = 0;		
				pTUser->m_fStartTime9 = 0.0f;
				pTUser->m_bHitRateAmount = 100;
				pTUser->m_sAvoidRateAmount = 100;
				pTUser->m_bType4Buff[8] = 0;			

				SendType4BuffRemove(tid, 9);
			}

			pTUser->SetSlotItemValue();
			pTUser->SetUserAbility();
			pTUser->Send2AI_UserUpdateInfo();	// AI Server�� �م� ����Ÿ ����....		

			buff_test = 0;
			for (i = 0 ; i < MAX_TYPE4_BUFF ; i++) {
				buff_test += pTUser->m_bType4Buff[i];
			}
			if (buff_test == 0) pTUser->m_bType4Flag = FALSE;

			bType4Test = TRUE ;
			for (j = 0 ; j < MAX_TYPE4_BUFF ; j++) {
				if (pTUser->m_bType4Buff[j] == 1) {
					bType4Test = FALSE;
					break;
				}
			}

			if (pTUser->isInParty() && bType4Test)
				pTUser->SendPartyStatusUpdate(2, 0);
			break;
			
		case RESURRECTION:		// RESURRECT A DEAD PLAYER!!!
			pTUser->Regene(1, magicid);
			break;

		case REMOVE_BLESS:
			if (pTUser->m_bType4Buff[0] == 2) {
				pTUser->m_sDuration1 = 0;		
				pTUser->m_fStartTime1 = 0.0f;
				pTUser->m_sMaxHPAmount = 0;
				pTUser->m_bType4Buff[0] = 0;

				SendType4BuffRemove(tid, 1);

				pTUser->SetSlotItemValue();
				pTUser->SetUserAbility();
				pTUser->Send2AI_UserUpdateInfo();	// AI Server�� �م� ����Ÿ ����....		
			
				buff_test = 0;
				for (i = 0 ; i < MAX_TYPE4_BUFF ; i++) {
					buff_test += pTUser->m_bType4Buff[i];
				}
				if (buff_test == 0) pTUser->m_bType4Flag = FALSE;

				bType4Test = TRUE ;
				for (j = 0 ; j < MAX_TYPE4_BUFF ; j++) {
					if (pTUser->m_bType4Buff[j] == 1) {
						bType4Test = FALSE;
						break;
					}
				}

				if (pTUser->isInParty() && bType4Test) 
					pTUser->SendPartyStatusUpdate(2, 0);
			}

			break;
	}

	if ( pMagic->bType2 == 0 || pMagic->bType2 == 5) {		   // In case of success!!!
		send_index = 0;
		SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
		SetByte( send_buff, MAGIC_EFFECTING, send_index );
		SetDWORD( send_buff, magicid, send_index );
		SetShort( send_buff, sid, send_index );
		SetShort( send_buff, tid, send_index );
		SetShort( send_buff, data1, send_index );	
		SetShort( send_buff, result, send_index );	
		SetShort( send_buff, data3, send_index );

		if (sid >= 0 && sid < MAX_USER) {
			m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );
		}
		else {
			m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );
		}
	}
}

void CMagicProcess::ExecuteType6(int magicid)
{
	return;
}

void CMagicProcess::ExecuteType7(int magicid)
{
	return;
}

void CMagicProcess::ExecuteType8(int magicid, int sid, int tid, int data1, int data2, int data3 )	// Warp, resurrection, and summon spells.
{	
	int damage = 0, send_index = 0, result = 1;     // Variable initialization. result == 1 : success, 0 : fail
	char send_buff[128];

	vector<int> casted_member;
	
	_MAGIC_TYPE8* pType = NULL;
	pType = m_pMain->m_Magictype8Array.GetData( magicid );      // Get magic skill table type 8.
	if( !pType ) return;

	if (tid == -1) {		
		for (int i = 0 ; i < MAX_USER ; i++) {		
			CUser* pTUser = m_pMain->GetUnsafeUserPtr(i);
			if (pTUser == NULL) continue;     // Check if target exists
			
			if (UserRegionCheck(sid, i, magicid, pType->sRadius, data1, data3))
				casted_member.push_back(i);
		}
		if (casted_member.size() == 0) return;	// If none of the members are in the region, return.
	}
	else {		// If the target was another single player.
		CUser* pTUser = m_pMain->GetUserPtr(tid);     // Get target info.  
		if (pTUser == NULL) return;     // Check if target exists		
		
		casted_member.push_back(tid);
	}

	foreach (itr, casted_member)
	{
		_OBJECT_EVENT* pEvent = NULL;
		float x = 0.0f, z = 0.0f;
		x = (float)(myrand( 0, 400 )/100.0f);	z = (float)(myrand( 0, 400 )/100.0f);
		if( x < 2.5f )	x = 1.5f + x;
		if( z < 2.5f )	z = 1.5f + z;

		CUser* pTUser = m_pMain->GetUserPtr(*itr);
		if (pTUser == NULL) continue;

// �񷯸ӱ� �븸 ������ >.<
		_HOME_INFO* pHomeInfo = NULL;
		pHomeInfo = m_pMain->m_HomeArray.GetData(pTUser->m_pUserData->m_bNation);
		if (!pHomeInfo) return;
//
		if (pType->bWarpType != 11) 
		{   // Warp or summon related: targets CANNOT be dead.
			if (pTUser->isDead())
			{
				result = 0;
				goto packet_send;
			}
		}
		else 
		{   // Resurrection related: we're reviving DEAD targets.
			if (!pTUser->isDead()) 
			{ 
				result = 0 ;
				goto packet_send;
			}
		}

		if (pTUser->m_bWarp)
		{	// Is target already warping?			
			result = 0;
			goto packet_send;
		}

		switch(pType->bWarpType) {	
			case 1:		// Send source to resurrection point.
//				_OBJECT_EVENT* pEvent = NULL;

				SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
				SetByte( send_buff, MAGIC_EFFECTING, send_index );
				SetDWORD( send_buff, magicid, send_index );
				SetShort( send_buff, sid, send_index );
				SetShort( send_buff, *itr, send_index );
				SetShort( send_buff, data1, send_index );	
				SetShort( send_buff, result, send_index );	
				SetShort( send_buff, data3, send_index );	
				m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );

				send_index = 0 ;

				if (pTUser->GetMap() == NULL)
					continue;

				pEvent = pTUser->GetMap()->GetObjectEvent(pTUser->m_pUserData->m_sBind);

				if( pEvent ) {
					pTUser->Warp(uint16((pEvent->fPosX + x) * 10), uint16((pEvent->fPosZ + z) * 10));	
				}
				// TO-DO: Remove this hardcoded nonsense
				else if(pTUser->getNation() != pTUser->getZoneID() && pTUser->getZoneID() <= ELMORAD) {	 // User is in different zone.
					if(pTUser->getNation() == KARUS) {	// Land of Karus
						pTUser->Warp(uint16((852 + x) * 10), uint16((164 + z) * 10));
					}
					else {	// Land of Elmorad
						pTUser->Warp(uint16((177 + x) * 10), uint16((923 + z) * 10));
					}				
				}
				else if (pTUser->m_pUserData->m_bZone == ZONE_BATTLE) {
					pTUser->Warp(uint16((pHomeInfo->BattleZoneX + x) * 10), uint16((pHomeInfo->BattleZoneZ + z) * 10));	
				}
				else if (pTUser->m_pUserData->m_bZone == ZONE_FRONTIER) {
					pTUser->Warp(uint16((pHomeInfo->FreeZoneX + x) * 10), uint16((pHomeInfo->FreeZoneZ + z) * 10));
				}
				else {		
					pTUser->Warp(uint16((pTUser->GetMap()->m_fInitX + x) * 10), uint16((pTUser->GetMap()->m_fInitZ + z) * 10));	
				}
							
				send_index = 0 ;
				break;

			case 2:		// Send target to teleport point WITHIN the zone.
				// LATER!!!
				break;
			case 3:		// Send target to teleport point OUTSIDE the zone.
				// LATER!!!
				break;
			case 5:		// Send target to a hidden zone.
				// LATER!!!
				break;
			
			case 11:	// Resurrect a dead player.
				SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
				SetByte( send_buff, MAGIC_EFFECTING, send_index );
				SetDWORD( send_buff, magicid, send_index );
				SetShort( send_buff, sid, send_index );
				SetShort( send_buff, *itr, send_index );
				SetShort( send_buff, data1, send_index );	
				SetShort( send_buff, result, send_index );	
				SetShort( send_buff, data3, send_index );	
				m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );

				send_index = 0 ;

				pTUser->m_bResHpType = USER_STANDING;     // Target status is officially alive now.
				pTUser->HpChange( pTUser->m_iMaxHp);	 // Refill HP.	
				pTUser->ExpChange( pType->sExpRecover/100 );     // Increase target experience.
				
				SetByte( send_buff, AG_USER_REGENE, send_index ) ;		// Send a packet to AI server.
				SetShort( send_buff, *itr, send_index );
				SetShort( send_buff, pTUser->m_pUserData->m_bZone, send_index);
				m_pMain->Send_AIServer(send_buff, send_index);

				send_index = 0;			// Clear index and buffer!
				break;

			case 12:	// Summon a target within the zone.	
				if (m_pSrcUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bZone) {	  // Same zone? 
					result = 0 ;
					goto packet_send ;
				}

				SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
				SetByte( send_buff, MAGIC_EFFECTING, send_index );
				SetDWORD( send_buff, magicid, send_index );
				SetShort( send_buff, sid, send_index );
				SetShort( send_buff, *itr, send_index );
				SetShort( send_buff, data1, send_index );	
				SetShort( send_buff, result, send_index );	
				SetShort( send_buff, data3, send_index );	
				m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );
				send_index = 0; 
					
				pTUser->Warp(m_pSrcUser->GetSPosX(), m_pSrcUser->GetSPosZ());
				break;

			case 13:	// Summon a target outside the zone.			
				if (m_pSrcUser->getZoneID() == pTUser->getZoneID()) {	  // Different zone? 
					result = 0 ;
					goto packet_send ;
				}

				SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
				SetByte( send_buff, MAGIC_EFFECTING, send_index );
				SetDWORD( send_buff, magicid, send_index );
				SetShort( send_buff, sid, send_index );
				SetShort( send_buff, *itr, send_index );
				SetShort( send_buff, data1, send_index );	
				SetShort( send_buff, result, send_index );	
				SetShort( send_buff, data3, send_index );	
				m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );

				send_index = 0;

				pTUser->ZoneChange(m_pSrcUser->m_pUserData->m_bZone, m_pSrcUser->m_pUserData->m_curx, m_pSrcUser->m_pUserData->m_curz) ;
				//pTUser->UserInOut( USER_IN );
				pTUser->UserInOut( USER_REGENE );
				//TRACE(" Summon ,, name=%s, x=%.2f, z=%.2f\n", pTUser->m_pUserData->m_id, pTUser->m_pUserData->m_curx, pTUser->m_pUserData->m_curz);
				break;

			case 20:	// Randomly teleport the source (within 20 meters)		
				SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
				SetByte( send_buff, MAGIC_EFFECTING, send_index );
				SetDWORD( send_buff, magicid, send_index );
				SetShort( send_buff, sid, send_index );
				SetShort( send_buff, *itr, send_index );
				SetShort( send_buff, data1, send_index );	
				SetShort( send_buff, result, send_index );	
				SetShort( send_buff, data3, send_index );	
				m_pMain->Send_Region( send_buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );

				send_index = 0 ;

				float warp_x, warp_z;		// Variable Initialization.
				float temp_warp_x, temp_warp_z;

				warp_x = pTUser->m_pUserData->m_curx;	// Get current locations.
				warp_z = pTUser->m_pUserData->m_curz;

				temp_warp_x = (float)myrand(0, 20) ;	// Get random positions (within 20 meters)
				temp_warp_z = (float)myrand(0, 20) ;

				if (temp_warp_x > 10)	// Get new x-position.
					warp_x = warp_x + (temp_warp_x - 10 ) ;
				else
					warp_x = warp_x - temp_warp_x ;

				if (temp_warp_z > 10)	// Get new z-position.
					warp_z = warp_z + (temp_warp_z - 10 ) ;
				else
					warp_z = warp_z - temp_warp_z ;
				
				if (warp_x < 0.0f) warp_x = 0.0f ;		// Make sure all positions are within range.
				if (warp_x > 4096) warp_x = 4096 ;		// Change it if it isn't!!!
				if (warp_z < 0.0f) warp_z = 0.0f ;		// (Warp function does not check this!)
				if (warp_z > 4096) warp_z = 4096 ;

				SetShort(send_buff, (WORD)warp_x, send_index);	// Send packet with new positions to the Warp() function.
				SetShort(send_buff, (WORD)warp_z, send_index);
				pTUser->Warp(uint16(warp_x * 10), uint16(warp_z * 10));

				send_index = 0;			// Clear index and buffer!
				break;

			case 21:	// Summon a monster within a zone.
				// LATER!!! 
				break;

			default :
				result = 0 ;
				goto packet_send ;
				break;
		}
		
	packet_send:
		SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
		SetByte( send_buff, MAGIC_EFFECTING, send_index );
		SetDWORD( send_buff, magicid, send_index );
		SetShort( send_buff, sid, send_index );
		SetShort( send_buff, *itr, send_index );
		SetShort( send_buff, data1, send_index );	
		SetShort( send_buff, result, send_index );	
		SetShort( send_buff, data3, send_index );	
		m_pMain->Send_Region( send_buff, send_index, m_pSrcUser->GetMap(), m_pSrcUser->m_RegionX, m_pSrcUser->m_RegionZ );

		send_index = 0 ;
		result = 1;
	}
}

void CMagicProcess::ExecuteType9(int magicid)
{
	return;
}

void CMagicProcess::ExecuteType10(int magicid)
{
	return;
}

short CMagicProcess::GetMagicDamage(int sid, int tid, int total_hit, int attribute)
{	
	CNpc* pMon;

	short damage = 0, temp_hit = 0, righthand_damage = 0, attribute_damage = 0 ; 
	int random = 0, total_r = 0 ;
	BYTE result; 

	CUser* pTUser = m_pMain->GetUserPtr(tid);  
	if (pTUser == NULL || pTUser->isDead()) return 0;

	if (sid >= NPC_BAND) {	// If the source is a monster.
		pMon = m_pMain->m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) return 0;

		result = m_pSrcUser->GetHitRate( pMon->m_sHitRate / pTUser->m_sTotalEvasionrate ); 		
	}
	else {	// If the source is another player.
		total_hit = total_hit * m_pSrcUser->m_pUserData->m_bCha / 170;
		result = SUCCESS ;
	}
		
	if (result != FAIL) {		// In case of SUCCESS.... 
		switch (attribute) {
			case NONE_R :
				total_r = 0;		
				break;
			case FIRE_R	:
				total_r = pTUser->m_bFireR + pTUser->m_bFireRAmount ;
				break;
			case COLD_R :
				total_r = pTUser->m_bColdR + pTUser->m_bColdRAmount ;
				break;
			case LIGHTNING_R :
				total_r = pTUser->m_bLightningR + pTUser->m_bLightningRAmount ; 
				break;
			case MAGIC_R :
				total_r = pTUser->m_bMagicR + pTUser->m_bMagicRAmount ;
				break;
			case DISEASE_R :
				total_r = pTUser->m_bDiseaseR + pTUser->m_bDiseaseRAmount ;
				break;
			case POISON_R :			
				total_r = pTUser->m_bPoisonR + pTUser->m_bPoisonRAmount ;
				break;
			case LIGHT_R :
				// LATER !!!
				break;
			case DARKNESS_R	:
				// LATER !!!
				break;
		}
		
		if ( sid >= 0 && sid < MAX_USER) {
			if( m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {	// Does the magic user have a staff?
				_ITEM_TABLE* pRightHand = NULL;
				pRightHand = m_pMain->m_ItemtableArray.GetData(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);

				if( pRightHand && m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum == 0 && pRightHand->m_bKind / 10 == WEAPON_STAFF) {				
					righthand_damage = pRightHand->m_sDamage ;
					
					if (m_pSrcUser->m_bMagicTypeRightHand == attribute) {
						attribute_damage = pRightHand->m_sDamage ;	
					}
				}
				else {
					righthand_damage = 0 ;
				}
			}
		}

		damage = (short)(total_hit - ((0.7 * total_hit * total_r) / 200)) ;
		random = myrand (0, damage);
		damage = (short)((0.7 * (total_hit - ((0.9 * total_hit * total_r) / 200))) + 0.2 * random);
//	
		if (sid >= NPC_BAND) {
			damage -= (3 * righthand_damage) - (3 * attribute_damage);
		}
		else{
			if (attribute != 4) {	// Only if the staff has an attribute.
				damage -= (short)(((righthand_damage * 0.8f) + (righthand_damage * m_pSrcUser->m_pUserData->m_bLevel) / 60) - ((attribute_damage * 0.8f) + (attribute_damage * m_pSrcUser->m_pUserData->m_bLevel) / 30));
			}


		}
//
	}

	damage = damage / 3 ;	// ������ ��û 

	return damage ;		
}

BOOL CMagicProcess::UserRegionCheck(int sid, int tid, int magicid, int radius, short mousex, short mousez)
{
	CNpc* pMon = NULL;

	float currenttime = 0.0f;
	BOOL bFlag = FALSE;

	CUser* pTUser = m_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) return FALSE;
	
	if (sid >= NPC_BAND) {					
		pMon = m_pMain->m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) return FALSE;
		bFlag = TRUE;
	}

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = m_pMain->m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return FALSE;

	switch (pMagic->bMoral) {
		case MORAL_PARTY_ALL :		// Check that it's your party.
/*
			if( !pTUser->isInParty()) {
				if (sid == tid) {
					return TRUE; 
				}
				else {
					return FALSE; 
				}
			}			

			if ( pTUser->m_sPartyIndex == m_pSrcUser->m_sPartyIndex) 
				goto final_test;

			break;
*/

// �񷯸ӱ� ������ ��Ƽ ��ȯ >.<
			if( !pTUser->isInParty()) {
				if (sid == tid) {
					return TRUE; 
				}
				else {
					return FALSE; 
				}
			}			

			if ( pTUser->m_sPartyIndex == m_pSrcUser->m_sPartyIndex && pMagic->bType1 != 8 ) {
				goto final_test;
			}
			else if (pTUser->m_sPartyIndex == m_pSrcUser->m_sPartyIndex && pMagic->bType1 == 8) {
				currenttime = TimeGet();
				if (pTUser->m_pUserData->m_bZone == ZONE_BATTLE && (currenttime - pTUser->m_fLastRegeneTime < CLAN_SUMMON_TIME)) {
					return FALSE;
				}
				else {
					goto final_test;	
				}
			}

			break;
//
		case MORAL_SELF_AREA :
		case MORAL_AREA_ENEMY :
			if (!bFlag) {
				if (pTUser->m_pUserData->m_bNation != m_pSrcUser->m_pUserData->m_bNation)		// Check that it's your enemy.
					goto final_test;
			}
			else {
				if (pTUser->m_pUserData->m_bNation != pMon->m_byGroup)
					goto final_test;
			}
			break;

		case MORAL_AREA_FRIEND :				
			if (pTUser->m_pUserData->m_bNation == m_pSrcUser->m_pUserData->m_bNation) 		// Check that it's your ally.
				goto final_test;
			break;
// �񷯸ӱ� Ŭ�� ��ȯ!!!
		case MORAL_CLAN_ALL :
			if( pTUser->m_pUserData->m_bKnights == -1) {
				if (sid == tid) {
					return TRUE; 
				}
				else {
					return FALSE; 
				}
			}			
/*
			if ( pTUser->m_pUserData->m_bKnights == m_pSrcUser->m_pUserData->m_bKnights) 
				goto final_test;
*/
			if ( pTUser->m_pUserData->m_bKnights == m_pSrcUser->m_pUserData->m_bKnights && pMagic->bType1 != 8 ) {
				goto final_test;
			}
			else if (pTUser->m_pUserData->m_bKnights == m_pSrcUser->m_pUserData->m_bKnights && pMagic->bType1 == 8) {
				currenttime = TimeGet();
				if (pTUser->m_pUserData->m_bZone == ZONE_BATTLE && (currenttime - pTUser->m_fLastRegeneTime < CLAN_SUMMON_TIME)) {
					return FALSE;
				}
				else {
					goto final_test;	
				}
			}

			break;
//
	}

	return FALSE;	

final_test :
	if (!bFlag) {	// When players attack...
		if ( pTUser->m_pUserData->m_bZone == m_pSrcUser->m_pUserData->m_bZone ) {		// Zone Check!
			if ( (pTUser->m_RegionX == m_pSrcUser->m_RegionX) && (pTUser->m_RegionZ == m_pSrcUser->m_RegionZ) ) { // Region Check!
				if (radius !=0) { 	// Radius check! ( ...in case there is one :(  )
					float temp_x = pTUser->m_pUserData->m_curx - mousex;
					float temp_z = pTUser->m_pUserData->m_curz - mousez;
					float distance = pow(temp_x, 2.0f) + pow(temp_z, 2.0f);
					if ( distance > pow((float)radius, 2.0f) ) return FALSE ;
				}		
				return TRUE;	// Target is in the area.
			}
		}	
	}
	else {			// When monsters attack...
		if ( pTUser->getZoneID() == pMon->getZoneID() ) {		// Zone Check!
			if ( (pTUser->m_RegionX == pMon->m_sRegion_X) && (pTUser->m_RegionZ == pMon->m_sRegion_Z) ) { // Region Check!
				if (radius !=0) { 	// Radius check! ( ...in case there is one :(  )
					float temp_x = pTUser->m_pUserData->m_curx - pMon->m_fCurX;
					float temp_z = pTUser->m_pUserData->m_curz - pMon->m_fCurZ;
					float distance = pow(temp_x, 2.0f) + pow(temp_z, 2.0f);	
					if ( distance > pow((float)radius, 2.0f) ) return FALSE ;
				}		
				return TRUE;	// Target is in the area.
			}
		}	
	}

	return FALSE;
}

void CMagicProcess::Type4Cancel(int magicid, short tid)
{
	int send_index = 0;     
	char send_buff[128];

	CUser* pTUser = m_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) return;

	_MAGIC_TYPE4* pType = NULL;
	pType = m_pMain->m_Magictype4Array.GetData( magicid );     // Get magic skill table type 4.
	if ( !pType ) return;

	BOOL buff = FALSE;
	BYTE buff_type = pType->bBuffType; 

	switch (buff_type) {		
		case 1:
			if (pTUser->m_sMaxHPAmount > 0) {
				pTUser->m_sDuration1 = 0;		
				pTUser->m_fStartTime1 = 0.0f;
				pTUser->m_sMaxHPAmount = 0;		
				buff = TRUE;
			}
			break;

		case 2:
			if (pTUser->m_sACAmount > 0) {
				pTUser->m_sDuration2 = 0;		
				pTUser->m_fStartTime2 = 0.0f;
				pTUser->m_sACAmount = 0;
				buff = TRUE;
			}
			break;
// 
		case 3:
			pTUser->m_sDuration3 = 0;		
			pTUser->m_fStartTime3 = 0.0f;
			pTUser->StateChangeServerDirect(3, ABNORMAL_NORMAL);
			buff = TRUE;	
			break;
//  
		case 4:
			if (pTUser->m_bAttackAmount > 100) {
				pTUser->m_sDuration4 = 0;		
				pTUser->m_fStartTime4 = 0.0f;
				pTUser->m_bAttackAmount = 100;
				buff = TRUE;
			}
			break;

		case 5:
			if (pTUser->m_bAttackSpeedAmount > 100) {
				pTUser->m_sDuration5 = 0;		
				pTUser->m_fStartTime5 = 0.0f;
				pTUser->m_bAttackSpeedAmount = 100;	
				buff = TRUE;
			}
			break;	

		case 6:	
			if (pTUser->m_bSpeedAmount > 100) {
				pTUser->m_sDuration6 = 0;		
				pTUser->m_fStartTime6 = 0.0f;
				pTUser->m_bSpeedAmount = 100;
				buff = TRUE;
			}
			break;

		case 7:	
			if ((pTUser->m_bStrAmount + pTUser->m_bStaAmount + pTUser->m_bDexAmount +
				 pTUser->m_bIntelAmount + pTUser->m_bChaAmount) > 0) {
				pTUser->m_sDuration7 = 0;		
				pTUser->m_fStartTime7 = 0.0f;
				pTUser->m_bStrAmount = 0;
				pTUser->m_bStaAmount = 0;
				pTUser->m_bDexAmount = 0;
				pTUser->m_bIntelAmount = 0;
				pTUser->m_bChaAmount = 0;
				buff = TRUE;
			}
			break;

		case 8:	
			if ((pTUser->m_bFireRAmount + pTUser->m_bColdRAmount + pTUser->m_bLightningRAmount +
				pTUser->m_bMagicRAmount + pTUser->m_bDiseaseRAmount + pTUser->m_bPoisonRAmount) > 0) {
				pTUser->m_sDuration8 = 0;		
				pTUser->m_fStartTime8 = 0.0f;
				pTUser->m_bFireRAmount = 0;
				pTUser->m_bColdRAmount = 0;
				pTUser->m_bLightningRAmount = 0;
				pTUser->m_bMagicRAmount = 0;
				pTUser->m_bDiseaseRAmount = 0;
				pTUser->m_bPoisonRAmount = 0;
				buff = TRUE;
			}
			break;	

		case 9:	
			if ((pTUser->m_bHitRateAmount + pTUser->m_sAvoidRateAmount) > 200) {
				pTUser->m_sDuration9 = 0;		
				pTUser->m_fStartTime9 = 0.0f;
				pTUser->m_bHitRateAmount = 100;
				pTUser->m_sAvoidRateAmount = 100;
				buff = TRUE;
			}
			break;
	}
	
	if (buff) {
		pTUser->m_bType4Buff[buff_type - 1] = 0;
		pTUser->SetSlotItemValue();
		pTUser->SetUserAbility();
		pTUser->Send2AI_UserUpdateInfo();	// AI Server�� �م� ����Ÿ ����....		

		SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
		SetByte( send_buff, MAGIC_TYPE4_END, send_index );	
		SetByte( send_buff, buff_type, send_index ); 
		pTUser->Send( send_buff, send_index ); 		
	}

	int buff_test = 0;
	for (int i = 0 ; i < MAX_TYPE4_BUFF ; i++) {
		buff_test += pTUser->m_bType4Buff[i];
	}
	if (buff_test == 0) pTUser->m_bType4Flag = FALSE;	

	if (pTUser->isInParty() && !pTUser->m_bType4Flag)
		pTUser->SendPartyStatusUpdate(2);
}

void CMagicProcess::Type3Cancel(int magicid, short tid)
{
	int send_index = 0;     
	char send_buff[128];

	CUser* pTUser = m_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) return;

	_MAGIC_TYPE3* pType = NULL;
	pType = m_pMain->m_Magictype3Array.GetData( magicid );     // Get magic skill table type 3.
	if ( !pType ) return;

	for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {
		if (pTUser->m_bHPAmount[i] > 0) {
			pTUser->m_fHPStartTime[i] = 0.0f;
			pTUser->m_fHPLastTime[i] = 0.0f;   
			pTUser->m_bHPAmount[i] = 0;
			pTUser->m_bHPDuration[i] = 0;				
			pTUser->m_bHPInterval[i] = 5;
			pTUser->m_sSourceID[i] = -1;
			break;
		}
	}

	SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
	SetByte( send_buff, MAGIC_TYPE3_END, send_index );	
	SetByte( send_buff, 100, send_index );
	pTUser->Send( send_buff, send_index ); 

	int buff_test = 0;
	for (int j = 0 ; j < MAX_TYPE3_REPEAT ; j++) {
		buff_test += pTUser->m_bHPDuration[j];
	}
	if (buff_test == 0) pTUser->m_bType3Flag = FALSE;	

	if (pTUser->isInParty() && !pTUser->m_bType3Flag)
		pTUser->SendPartyStatusUpdate(1, 0);
}

void CMagicProcess::SendType4BuffRemove(short tid, BYTE buff)
{
	int send_index = 0; char send_buff[128];

	CUser* pTUser = m_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) return;

	SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
	SetByte( send_buff, MAGIC_TYPE4_END, send_index );	
	SetByte( send_buff, buff, send_index );		
	pTUser->Send( send_buff, send_index );
}

short CMagicProcess::GetWeatherDamage(short damage, short attribute)
{
	BOOL weather_buff = FALSE;

	switch (m_pMain->m_nWeather) {
		case WEATHER_FINE:
			if (attribute == ATTRIBUTE_FIRE) {
				weather_buff = TRUE;
			}
			break;

		case WEATHER_RAIN:
			if (attribute == ATTRIBUTE_LIGHTNING) {
				weather_buff = TRUE;
			}
			break;

		case WEATHER_SNOW:
			if (attribute == ATTRIBUTE_ICE) {
				weather_buff = TRUE;
			}
			break;
	}

	if (weather_buff) {
		damage = (damage * 110) / 100 ;
	}

	return damage;
}

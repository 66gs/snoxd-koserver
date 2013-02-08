#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"
#include "AIPacket.h"

void CUser::Attack(char *pBuf)
{
	int index = 0, send_index = 0;
	int sid = -1, tid = -1, damage = 0; short delaytime = 0; short distance = 0;	
	BYTE type, result;	
	char buff[256]; 
	
//	CUser* pUser = NULL;
	CUser* pTUser = NULL;
	CNpc* pNpc  = NULL;
	_ITEM_TABLE* pTable = NULL;

	type = GetByte( pBuf, index );
	result = GetByte( pBuf, index );
//	sid = GetShort( pBuf, index );
	tid = GetShort( pBuf, index );

	delaytime = GetShort( pBuf, index );
	distance = GetShort( pBuf,index );

//	delaytime = delaytime / 100.0f;
//	distance = distance / 10.0f;	// 'Coz the server multiplies it by 10 before they send it to you.

	if (m_bAbnormalType == ABNORMAL_BLINKING) return;

	if (isDead())
	{
		TRACE("### Attack Fail : name=%s(%d), m_bResHpType=%d, hp=%d###\n", m_pUserData->m_id, m_Sid, m_bResHpType, m_pUserData->m_sHp);
		return;
	}
	pTable = m_pMain->m_ItemtableArray.GetData(m_pUserData->m_sItemArray[RIGHTHAND].nNum);	// This checks if such an item exists.
	if(!pTable && m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0) return;
	
	if (pTable) {	// If you're holding a weapon, do a delay check.
//		TRACE("Delay time : %f  ,  Table Delay Time : %f \r\n", delaytime, pTable->m_sDelay / 100.0f);
//		if (delaytime + 0.01f < (pTable->m_sDelay / 100.0f)) {
		if (delaytime < pTable->m_sDelay) {
			return;	
		}
	}
	else {		// Empty handed.
//		if (delaytime + 0.01f < 1.0f) { 
		if (delaytime < 100) {
			return;			
		}
	}
//
	if(tid < NPC_BAND) {	// USER
		if( tid >= MAX_USER || tid < 0 ) return;
		pTUser = m_pMain->GetUserPtr(tid);
 
		if( !pTUser || pTUser->m_bResHpType == USER_DEAD || pTUser->m_bAbnormalType == ABNORMAL_BLINKING
		    || pTUser->m_pUserData->m_bNation == m_pUserData->m_bNation ) 
			result = 0x00;
		else {
			if (pTable) {	// Check if the user is holding a weapon!!! No null pointers allowed!!!
//				TRACE("Distance : %f  , Table Distance : %f  \r\n", distance, pTable->m_sRange / 10.0f);
//				if ( distance > (pTable->m_sRange / 10.0f)) {
				if ( distance > pTable->m_sRange ) {
					return;
				}
			}
//
			damage = GetDamage(tid, 0);

			if( m_pUserData->m_bZone == ZONE_SNOW_BATTLE && m_pMain->m_byBattleOpen == SNOW_BATTLE )	{
				damage = 0;		
			}

			if( damage <= 0 )
				result = 0x00;
			else {
				pTUser->HpChange( -damage, 0, true );
				ItemWoreOut( ATTACK, damage );
				pTUser->ItemWoreOut( DEFENCE, damage );
				//TRACE("%s - HP:%d, (damage-%d, t_hit-%d)\n", pTUser->m_pUserData->m_id, pTUser->m_pUserData->m_sHp, damage, m_sTotalHit);
				if( pTUser->m_pUserData->m_sHp == 0) {
					result = 0x02;
					pTUser->m_bResHpType = USER_DEAD;

					// sungyong work : loyalty					
					if( !isInParty() ) {    // Something regarding loyalty points.
						LoyaltyChange(tid);
					}
					else {
						LoyaltyDivide(tid);
					}

					GoldChange(tid, 0);

					pTUser->InitType3();	// Init Type 3.....
					pTUser->InitType4();	// Init Type 4.....

					send_index = 0;
					if( pTUser->m_pUserData->m_bFame == COMMAND_CAPTAIN )	{
						pTUser->m_pUserData->m_bFame = CHIEF;
						SetByte( buff, WIZ_AUTHORITY_CHANGE, send_index );
						SetByte( buff, COMMAND_AUTHORITY, send_index );
						SetShort( buff, pTUser->GetSocketID(), send_index );
						SetByte( buff, pTUser->m_pUserData->m_bFame, send_index );
						m_pMain->Send_Region( buff, send_index, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ );

						Send( buff, send_index );
						//TRACE("---> UserAttack Dead Captain Deprive - %s\n", pTUser->m_pUserData->m_id);
						if( pTUser->m_pUserData->m_bNation == KARUS )			m_pMain->Announcement( KARUS_CAPTAIN_DEPRIVE_NOTIFY, KARUS );
						else if( pTUser->m_pUserData->m_bNation == ELMORAD )	m_pMain->Announcement( ELMORAD_CAPTAIN_DEPRIVE_NOTIFY, ELMORAD );
					}

					pTUser->m_sWhoKilledMe = m_Sid;		// You killed me, you.....
//
					if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
						pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
					}
//
				}
				SendTargetHP( 0, tid, -damage );
			}
		}
	}
	else if(tid >= NPC_BAND) { // NPC
		if( m_pMain->m_bPointCheckFlag == FALSE)	return;	
		pNpc = m_pMain->m_arNpcArray.GetData(tid);		
		if( pNpc && pNpc->m_NpcState != NPC_DEAD && pNpc->m_iHP > 0 
			&& (pNpc->getNation() == 0 || pNpc->getNation() == getNation())) {	

			if (pTable) {	// Check if the user is holding a weapon!!! No null pointers allowed!!!
//				TRACE("Distance : %f  , Table Distance : %f  \r\n", distance, pTable->m_sRange / 10.0f);
//				if ( distance > (pTable->m_sRange / 10.0f)) {
				if ( distance > pTable->m_sRange ) {
					return;
				}
//				TRACE("Success!!! \r\n");
			}
//
			send_index = 0;
			SetByte( buff, AG_ATTACK_REQ, send_index );
			SetByte( buff, type, send_index );
			SetByte( buff, result, send_index ); 
//			SetShort( buff, sid, send_index );
			SetShort( buff, m_Sid, send_index );
			SetShort( buff, tid, send_index );
			SetShort( buff, m_sTotalHit * m_bAttackAmount / 100, send_index );  
			SetShort( buff, m_sTotalAc + m_sACAmount, send_index );   
			Setfloat( buff, m_sTotalHitrate, send_index );
			Setfloat( buff, m_sTotalEvasionrate, send_index ); 
			SetShort( buff, m_sItemAc, send_index);
			SetByte( buff, m_bMagicTypeLeftHand, send_index);
			SetByte( buff, m_bMagicTypeRightHand, send_index);
			SetShort( buff, m_sMagicAmountLeftHand, send_index);
			SetShort( buff, m_sMagicAmountRightHand, send_index);
			m_pMain->Send_AIServer(buff, send_index);	
			return;
		}
	}

	send_index = 0;
	SetByte( buff, WIZ_ATTACK, send_index );
	SetByte( buff, type, send_index );
	SetByte( buff, result, send_index );
//	SetShort( buff, sid, send_index );
	SetShort( buff, m_Sid, send_index );
	SetShort( buff, tid, send_index );
	m_pMain->Send_Region( buff, send_index, GetMap(), m_RegionX, m_RegionZ );

	if( tid < NPC_BAND )	{
		if( result == 0x02 )	{
			if( pTUser )	{
				pTUser->Send( buff, send_index );
				DEBUG_LOG("*** User Attack Dead, id=%s, result=%d, type=%d, HP=%d", pTUser->m_pUserData->m_id, result, pTUser->m_bResHpType, pTUser->m_pUserData->m_sHp);
			}
		}
	}
}

short CUser::GetDamage(short tid, int magicid)
{
	short damage = 0;
	int random = 0;
	short common_damage = 0, temp_hit = 0, temp_ac = 0, temp_hit_B = 0;
	BYTE result;

	_MAGIC_TABLE* pTable = NULL;
	_MAGIC_TYPE1* pType1 = NULL; 
	_MAGIC_TYPE2* pType2 = NULL;

	if( tid < 0 || tid >= MAX_USER) return -1;     // Check if target id is valid.

	CUser* pTUser = m_pMain->GetUserPtr(tid);
	if (!pTUser || pTUser->isDead()) 
		return -1;

	temp_ac = pTUser->m_sTotalAc + pTUser->m_sACAmount;    // g??   
	temp_hit_B = (int)( (m_sTotalHit* m_bAttackAmount * 200 / 100) / (temp_ac + 240) ) ;   // g??

	if (magicid > 0) {    // Skill/Arrow hit.    
		pTable = m_pMain->m_MagictableArray.GetData( magicid );     // Get main magic table.
		if( !pTable ) return -1; 
		
		if (pTable->bType1 == 1) {	// SKILL HIT!			                                
			pType1 = m_pMain->m_Magictype1Array.GetData( magicid );	    // Get magic skill table type 1.
			if( !pType1 ) return -1;     	                                

			if(pType1->bHitType) {    // Non-relative hit.
				random = myrand(0,100) ;       
				if (pType1->sHitRate <= random)
					result = FAIL;
				else 
					result = SUCCESS;
			}
			else {     // Relative hit.
				result = GetHitRate( (m_sTotalHitrate / pTUser->m_sTotalEvasionrate) * (pType1->sHitRate / 100.0f) );			
			}
			temp_hit = (short)(temp_hit_B * (pType1->sHit / 100.0f));
		}
		else if (pTable->bType1 == 2) {   // ARROW HIT!
			pType2 = m_pMain->m_Magictype2Array.GetData( magicid );	    // Get magic skill table type 1.
			if( !pType2 ) return -1; 
			
			if(pType2->bHitType == 1 || pType2->bHitType == 2 ) {    // Non-relative/Penetration hit.
				random = myrand(0,100) ; 
				
				if (pType2->sHitRate <= random)
					result = FAIL;
				else 
					result = SUCCESS;
			}
			else     // Relative hit/Arc hit.
				result = GetHitRate( (m_sTotalHitrate / pTUser->m_sTotalEvasionrate) * (pType2->sHitRate / 100.0f) );
			
			if(pType2->bHitType == 1 /* || pType2->bHitType == 2 */)  {
				temp_hit = (short)(m_sTotalHit * m_bAttackAmount * (pType2->sAddDamage / 100.0f) / 100);
			}
			else {
				temp_hit = (short)(temp_hit_B * (pType2->sAddDamage / 100.0f));
			}
		}
	}
	else {    // Normal Hit.     
		temp_hit = m_sTotalHit * m_bAttackAmount / 100 ;	// g??
		result = GetHitRate( m_sTotalHitrate / pTUser->m_sTotalEvasionrate ); 
	}
	
	switch(result) {						// 1. Magical item damage....
		case GREAT_SUCCESS:
		case SUCCESS:
		case NORMAL:
			if( magicid > 0 ) {	 // Skill Hit.
				damage = (short)temp_hit;
				random = myrand(0, damage);
				if (pTable->bType1 == 1) {
					damage = (short)((temp_hit + 0.3f * random) + 0.99f);
				}
				else {
					damage = (short)(((temp_hit * 0.6f) + 1.0f * random) + 0.99f);
				}
			}
			else {	// Normal Hit.	
				damage = (short)temp_hit_B;
				random = myrand(0, damage);
				damage = (short)((0.85f * temp_hit_B) + 0.3f * random);
			}		
			
			break;
		case FAIL:
			damage = 0;
			break;
	}	

	damage = GetMagicDamage(damage, tid);	// 2. Magical item damage....	
	damage = GetACDamage(damage, tid);		// 3. Additional AC calculation....	
//	damage = damage / 2;	// ?????? ??? ??�!!!!
	damage = damage / 3;	// ?????? ??? ??�!!!!  

	return damage;	  
}

short CUser::GetMagicDamage(int damage, short tid)
{
	short total_r = 0;
	short temp_damage = 0;

	CUser* pTUser = m_pMain->GetUserPtr(tid);
	if (!pTUser || pTUser->isDead())
		return damage;	

	// RIGHT HAND!!! by Yookozuna
	if (m_bMagicTypeRightHand > 4 && m_bMagicTypeRightHand < 8) {
		temp_damage = damage * m_sMagicAmountRightHand / 100;
	}

	switch (m_bMagicTypeRightHand) {	// RIGHT HAND!!!
		case ITEM_TYPE_FIRE :	// Fire Damage
			total_r = pTUser->m_bFireR + pTUser->m_bFireRAmount;
			break;
		case ITEM_TYPE_COLD :	// Ice Damage
			total_r = pTUser->m_bColdR + pTUser->m_bColdRAmount;
			break;
		case ITEM_TYPE_LIGHTNING :	// Lightning Damage
			total_r = pTUser->m_bLightningR + pTUser->m_bLightningRAmount;
			break;
		case ITEM_TYPE_POISON :	// Poison Damage
			total_r = pTUser->m_bPoisonR + pTUser->m_bPoisonRAmount;
			break;
		case ITEM_TYPE_HP_DRAIN :	// HP Drain		
			HpChange(temp_damage, 0);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTUser->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			MSpChange(temp_damage);
			break;
		case 0:
			break;
	}

	if (m_bMagicTypeRightHand > 0 && m_bMagicTypeRightHand < 5) {
		if (total_r > 200) total_r = 200;
		temp_damage = m_sMagicAmountRightHand - m_sMagicAmountRightHand * total_r / 200;
		damage = damage + temp_damage;
	}

	total_r = 0 ;		// Reset all temporary data.
	temp_damage = 0 ;

	// LEFT HAND!!! by Yookozuna
	if (m_bMagicTypeLeftHand > 4 && m_bMagicTypeLeftHand < 8) {
		temp_damage = damage * m_sMagicAmountLeftHand / 100;
	}

	switch (m_bMagicTypeLeftHand) {	// LEFT HAND!!!
		case ITEM_TYPE_FIRE :	// Fire Damage
			total_r = pTUser->m_bFireR + pTUser->m_bFireRAmount;
			break;
		case ITEM_TYPE_COLD :	// Ice Damage
			total_r = pTUser->m_bColdR + pTUser->m_bColdRAmount;
			break;
		case ITEM_TYPE_LIGHTNING :	// Lightning Damage
			total_r = pTUser->m_bLightningR + pTUser->m_bLightningRAmount;
			break;
		case ITEM_TYPE_POISON :	// Poison Damage
			total_r = pTUser->m_bPoisonR + pTUser->m_bPoisonRAmount;
			break;
		case ITEM_TYPE_HP_DRAIN :	// HP Drain		
			HpChange(temp_damage, 0);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTUser->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			MSpChange(temp_damage);
			break;
		case 0:
			break;
	}

	if (m_bMagicTypeLeftHand > 0 && m_bMagicTypeLeftHand < 5) {
		if (total_r > 200) total_r = 200;
		temp_damage = m_sMagicAmountLeftHand - m_sMagicAmountLeftHand * total_r / 200;
		damage = damage + temp_damage;
	}

	total_r = 0 ;		// Reset all temporary data.
	temp_damage = 0 ;

	// Mirror Attack Check routine.
	if (pTUser->m_bMagicTypeLeftHand == ITEM_TYPE_MIRROR_DAMAGE) {
		temp_damage = damage * pTUser->m_sMagicAmountLeftHand / 100;
		HpChange(-temp_damage);		// Reflective Hit.
	}

	return damage;
}

short CUser::GetACDamage(int damage, short tid)
{
	_ITEM_TABLE* pLeftHand = NULL;
	_ITEM_TABLE* pRightHand = NULL;

	CUser* pTUser = m_pMain->GetUserPtr(tid);
	if (pTUser == NULL || pTUser->isDead())
		return damage;	

	if( m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {
		pRightHand = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[RIGHTHAND].nNum );
		if( pRightHand ) {
			switch(pRightHand->m_bKind/10) {		// Weapon Type Right Hand....
				case WEAPON_DAGGER:		
					damage = damage - damage * pTUser->m_sDaggerR / 200 ;
					break;
				case WEAPON_SWORD:
					damage = damage - damage * pTUser->m_sSwordR / 200 ;				
					break;
				case WEAPON_AXE:
					damage = damage - damage * pTUser->m_sAxeR / 200 ;				
					break;
				case WEAPON_MACE:
					damage = damage - damage * pTUser->m_sMaceR / 200 ;				
					break;
				case WEAPON_SPEAR:
					damage = damage - damage * pTUser->m_sSpearR / 200 ;			
					break;
				case WEAPON_BOW:
					damage = damage - damage * pTUser->m_sBowR / 200 ;			
					break;
			}
		}
	}

	if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0) {
		pLeftHand = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[LEFTHAND].nNum );	
		if( pLeftHand ) {
			switch(pLeftHand->m_bKind/10) {			// Weapon Type Right Hand....
				case WEAPON_DAGGER:		
					damage = damage - damage * pTUser->m_sDaggerR / 200 ;
					break;
				case WEAPON_SWORD:
					damage = damage - damage * pTUser->m_sSwordR / 200 ;				
					break;
				case WEAPON_AXE:
					damage = damage - damage * pTUser->m_sAxeR / 200 ;				
					break;
				case WEAPON_MACE:
					damage = damage - damage * pTUser->m_sMaceR / 200 ;				
					break;
				case WEAPON_SPEAR:
					damage = damage - damage * pTUser->m_sSpearR / 200 ;			
					break;
				case WEAPON_BOW:
					damage = damage - damage * pTUser->m_sBowR / 200 ;			
					break;
			}
		}
	}

	return damage;
}

BYTE CUser::GetHitRate(float rate)
{
	BYTE result;
	int random = 0;
	random = myrand(1, 10000);

	if( rate >= 5.0 )
	{
		if( random >= 1 && random <= 3500)
			result = GREAT_SUCCESS;
		else if( random >= 3501 && random <= 7500)
			result = SUCCESS;
		else if( random >= 7501 && random <= 9800)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 5.0 && rate >= 3.0)
	{
		if( random >= 1 && random <= 2500)
			result = GREAT_SUCCESS;
		else if( random >= 2501 && random <= 6000)
			result = SUCCESS;
		else if( random >= 6001 && random <= 9600)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 3.0 && rate >= 2.0)
	{
		if( random >= 1 && random <= 2000)
			result = GREAT_SUCCESS;
		else if( random >= 2001 && random <= 5000)
			result = SUCCESS;
		else if( random >= 5001 && random <= 9400)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 2.0 && rate >= 1.25)
	{
		if( random >= 1 && random <= 1500)
			result = GREAT_SUCCESS;
		else if( random >= 1501 && random <= 4000)
			result = SUCCESS;
		else if( random >= 4001 && random <= 9200)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 1.25 && rate >= 0.8)
	{
		if( random >= 1 && random <= 1000)
			result = GREAT_SUCCESS;
		else if( random >= 1001 && random <= 3000)
			result = SUCCESS;
		else if( random >= 3001 && random <= 9000)
			result = NORMAL;
		else
			result = FAIL;
	}	
	else if ( rate < 0.8 && rate >= 0.5)
	{
		if( random >= 1 && random <= 800)
			result = GREAT_SUCCESS;
		else if( random >= 801 && random <= 2500)
			result = SUCCESS;
		else if( random >= 2501 && random <= 8000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 0.5 && rate >= 0.33)
	{
		if( random >= 1 && random <= 600)
			result = GREAT_SUCCESS;
		else if( random >= 601 && random <= 2000)
			result = SUCCESS;
		else if( random >= 2001 && random <= 7000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 0.33 && rate >= 0.2)
	{
		if( random >= 1 && random <= 400)
			result = GREAT_SUCCESS;
		else if( random >= 401 && random <= 1500)
			result = SUCCESS;
		else if( random >= 1501 && random <= 6000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else
	{
		if( random >= 1 && random <= 200)
			result = GREAT_SUCCESS;
		else if( random >= 201 && random <= 1000)
			result = SUCCESS;
		else if( random >= 1001 && random <= 5000)
			result = NORMAL;
		else
			result = FAIL;
	}
	
	return result;
}

void CUser::Regene(char *pBuf, int magicid)
{
	ASSERT(GetMap() != NULL);

	InitType3();
	InitType4();

	CUser* pUser = NULL;
	_OBJECT_EVENT* pEvent = NULL;
	_HOME_INFO* pHomeInfo = NULL;
	_MAGIC_TYPE5* pType = NULL;

	int index = 0; BYTE regene_type = 0;

	regene_type = GetByte(pBuf, index);

	if (regene_type != 1 && regene_type != 2) {
		regene_type = 1;
	}

	if (regene_type == 2) {
		magicid = 490041;	// The Stone of Ressurection magic ID

		if (!RobItem(379006000, 3 * getLevel())) {
			return;	// Subtract resurrection stones.
		}

		if (m_pUserData->m_bLevel <= 5) {
			return;	// 5 level minimum.
		}
	}

	pHomeInfo = m_pMain->m_HomeArray.GetData(m_pUserData->m_bNation);
	if (!pHomeInfo) return;

	UserInOut(USER_OUT);

	float x = 0.0f, z = 0.0f;
	x = (float)(myrand( 0, 400 )/100.0f);	z = (float)(myrand( 0, 400 )/100.0f);
	if( x < 2.5f )	x = 1.5f + x;
	if( z < 2.5f )	z = 1.5f + z;

	pEvent = GetMap()->GetObjectEvent(m_pUserData->m_sBind);	

	// TO-DO: Clean this entire thing up. Holy crap.
	if (magicid == 0) {
		if( pEvent && pEvent->byLife == 1 ) {		// Bind Point
			m_pUserData->m_curx = m_fWill_x = pEvent->fPosX + x;
			m_pUserData->m_curz = m_fWill_z = pEvent->fPosZ + z;
			m_pUserData->m_cury = 0;
		}
		else if( m_pUserData->m_bNation != m_pUserData->m_bZone) {	// Free Zone or Opposite Zone
			if(m_pUserData->m_bZone > 200) {		// Frontier Zone...
				x = (float)(pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX));
				z = (float)(pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ));
			}
//
			else if(m_pUserData->m_bZone > 100 && m_pUserData->m_bZone < 200) {		// Battle Zone...
/*
				m_bResHpType = USER_STANDING;
				HpChange( m_iMaxHp );
				KickOutZoneUser();	// Go back to your own zone!
				return;
*/
				x = (float)(pHomeInfo->BattleZoneX + myrand(0, pHomeInfo->BattleZoneLX));
				z = (float)(pHomeInfo->BattleZoneZ + myrand(0, pHomeInfo->BattleZoneLZ));
				if (m_pUserData->m_bZone == ZONE_SNOW_BATTLE) {
					x = (float)(pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX));
					z = (float)(pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ));					
				}
			}
			else if (m_pUserData->m_bZone > 10 && m_pUserData->m_bZone < 20) {
				x = (float)(527 + myrand(0, 10));
				z = (float)(543 + myrand(0, 10));
			}
			else if (m_pUserData->m_bZone < 3) {	// Specific Lands...
				if (m_pUserData->m_bNation == KARUS) {
					x = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
					z = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ));			
				}
				else if (m_pUserData->m_bNation == ELMORAD) {
					x = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
					z = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ));	
				}		
				else return;
			}

			m_pUserData->m_curx = x;
			m_pUserData->m_curz = z;
		}
		else {	
			if (m_pUserData->m_bNation == KARUS) {
				x = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
				z = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ));			
			}
			else if (m_pUserData->m_bNation == ELMORAD) {			
				x = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
				z = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ));
			}		
			else return;		

			m_pUserData->m_curx = x;
			m_pUserData->m_curz = z;
		}
	}

	Packet result(WIZ_REGENE);
	result << GetSPosX() << GetSPosZ() << GetSPosY();
	Send(&result);
	
	if (magicid > 0) {	// Clerical Resurrection.
		pType = m_pMain->m_Magictype5Array.GetData(magicid);     
		if ( !pType ) return;

		m_bResHpType = USER_STANDING;
		MSpChange(-m_iMaxMp);					// Empty out MP.

		if (m_sWhoKilledMe == -1 && regene_type == 1) {		
			ExpChange((m_iLostExp * pType->bExpRecover) / 100);		// Restore Target Experience.
		}

		m_bRegeneType = REGENE_MAGIC;
	}
	else {		// Normal Regene.
//
		m_bAbnormalType = ABNORMAL_BLINKING;
//
		m_bResHpType = USER_STANDING;	
//		HpChange( m_iMaxHp );
		m_bRegeneType = REGENE_NORMAL;
	}

	m_fLastRegeneTime = TimeGet();
	m_sWhoKilledMe = -1;
	m_iLostExp = 0;

	if (m_bAbnormalType != ABNORMAL_BLINKING) {
		result.Initialize(AG_USER_REGENE);
		result << uint16(GetSocketID()) << m_pUserData->m_sHp;
		m_pMain->Send_AIServer(&result);
	}

	m_RegionX = (int)(m_pUserData->m_curx / VIEW_DISTANCE);
	m_RegionZ = (int)(m_pUserData->m_curz / VIEW_DISTANCE);

	UserInOut(USER_REGENE);		

	m_pMain->RegionUserInOutForMe(this);
	m_pMain->RegionNpcInfoForMe(this);

	BlinkStart();

	if (isInParty())
	{
		// TO-DO: Wrap these up into Party-specific methods (nothing for that yet)
		if (!m_bType3Flag)
		{
			result.Initialize(WIZ_PARTY);
			result	<< uint8(PARTY_STATUSCHANGE)
					<< uint16(GetSocketID()) << uint8(1) << uint8(0);
			m_pMain->Send_PartyMember(m_sPartyIndex, &result);
		}

		if (!m_bType4Flag)
		{
			result.Initialize(WIZ_PARTY);
			result	<< uint8(PARTY_STATUSCHANGE)
					<< uint16(GetSocketID()) << uint8(2) << uint8(0);
			m_pMain->Send_PartyMember(m_sPartyIndex, &result);
		}
	}
}

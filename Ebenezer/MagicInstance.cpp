﻿#include "stdafx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "../shared/KOSocketMgr.h"
#include "User.h"
#include "MagicProcess.h"
#include "MagicInstance.h"

extern KOSocketMgr<CUser> g_socketMgr;

using std::string;
using std::vector;

void MagicInstance::Run()
{
	if (pSkill == NULL)
		pSkill = g_pMain.m_MagictableArray.GetData(nSkillID);

	if (pSkill == NULL
		|| pSkillCaster == NULL
		|| !UserCanCast())
	{
		SendSkillFailed();
		return;
	}

	// If the target is a mob/NPC *or* we're casting an AOE, tell the AI to handle it.
	if (sTargetID >= NPC_BAND
		|| (sTargetID == -1 && 
			(pSkill->bMoral == MORAL_AREA_ENEMY 
				|| pSkill->bMoral == MORAL_AREA_ALL 
				|| pSkill->bMoral == MORAL_SELF_AREA)))
	{
		SendSkillToAI();

		// If the target is specifically a mob, stop here. AI's got this one.
		// Otherwise, it's an AOE -- which means it might affect players too, so we need to handle it too.
		if (sTargetID >= NPC_BAND)
			return;
	}

	bool bInitialResult;
	switch (bOpcode)
	{
		case MAGIC_CASTING:
			SendSkill();
			break;

		case MAGIC_EFFECTING:
			// Hacky check for a transformation item (Disguise Totem, Disguise Scroll)
			// These apply when first type's set to 0, second type's set and obviously, there's a consumable item.
			// Need to find a better way of handling this.
			if (pSkill->bType[0] == 0 && pSkill->bType[1] != 0
				&& pSkill->iUseItem != 0
				&& (pSkillCaster->isPlayer() 
					&& TO_USER(pSkillCaster)->CheckExistItem(pSkill->iUseItem, 1)))
			{
				SendTransformationList();
				return;
			}

			bInitialResult = ExecuteSkill(pSkill->bType[0]);

			// NOTE: Some ROFD skills require a THIRD type.
			if (bInitialResult)
				ExecuteSkill(pSkill->bType[1]);
			break;
		case MAGIC_FLYING:
		case MAGIC_FAIL:
			SendSkill();
			break;
		case MAGIC_TYPE3_END: //This is also MAGIC_TYPE4_END
			break;
		case MAGIC_CANCEL:
		case MAGIC_CANCEL2:
			Type3Cancel();	//Damage over Time skills.
			Type4Cancel();	//Buffs
			Type6Cancel();	//Transformations
			Type9Cancel();	//Stealth, lupine etc.
			break;
		case MAGIC_TYPE4_EXTEND:
			Type4Extend();
			break;
	}
}

bool MagicInstance::UserCanCast()
{
	if (pSkill == NULL)
		return false;
	
	// We don't need to check anything as we're just canceling our buffs.
	if (bOpcode == MAGIC_CANCEL || bOpcode == MAGIC_CANCEL2) 
		return true;

	if (bIsRecastingSavedMagic)
		return true;

	if (pSkillCaster->isBlinded())
		return false;

	// Users who are blinking cannot use skills.
	// Additionally, unless it's resurrection-related, dead players cannot use skills.
	if (pSkillCaster->isBlinking()
		|| (pSkillCaster->isDead() && pSkill->bType[0] != 5)) 
		return false;

	// If we're using an AOE, and the target is specified... something's not right.
	if ((pSkill->bMoral >= MORAL_AREA_ENEMY
			&& pSkill->bMoral <= MORAL_SELF_AREA)
		&& sTargetID != -1)
		return false;

	// NPCs do not need further checks.
	// NOTE: The source check's implemented in the main packet wrapper.
	if (pSkillCaster->isNPC())
		return true;

	if (pSkill->sSkill != 0
		&& (TO_USER(pSkillCaster)->m_sClass != (pSkill->sSkill / 10)
			|| pSkillCaster->GetLevel() < pSkill->sSkillLevel))
		return false;

	if ((pSkillCaster->GetMana() - pSkill->sMsp) < 0)
		return false;

	// If we're in a snow war, we're only ever allowed to use the snowball skill.
	if (pSkillCaster->GetZoneID() == ZONE_SNOW_BATTLE && g_pMain.m_byBattleOpen == SNOW_BATTLE 
		&& nSkillID != SNOW_EVENT_SKILL)
		return false;

	if (pSkillTarget != NULL)
	{
		// Players require a little more rigorous checking
		if (pSkillTarget->isPlayer())
		{
			if (pSkillTarget != pSkillCaster)
			{
				if (pSkillTarget->GetZoneID() != pSkillCaster->GetZoneID()
					|| !TO_USER(pSkillTarget)->isAttackZone()
					// Will have to add support for the outside battlefield
					|| (TO_USER(pSkillTarget)->isAttackZone() 
						&& pSkillTarget->GetNation() == pSkillCaster->GetNation()))
					return false;
			}
		}
		// If the target's an NPC, we don't need to be as thorough.
		else 
		{
			if (pSkillTarget->GetZoneID() != pSkillCaster->GetZoneID()
				|| pSkillTarget->GetNation() == pSkillCaster->GetNation())
				return false;
		}
	}

	// Archer & transformation skills will handle item checking themselves
	if ((pSkill->bType[0] != 2 && pSkill->bType[0] != 6) 
		// The user does not meet the item's requirements or does not have any of said item.
		&& (pSkill->iUseItem != 0
			&& !TO_USER(pSkillCaster)->CanUseItem(pSkill->iUseItem, 1))) 
		return false;

	if ((bOpcode == MAGIC_EFFECTING || bOpcode == MAGIC_CASTING) 
		&& !IsAvailable())
		return false;

	if (!pSkillCaster->canInstantCast())
		pSkillCaster->m_bInstantCast = false;
	else
		pSkillCaster->m_bInstantCast = false;
	

	//Incase we made it to here, we can cast! Hurray!
	return true;
}

void MagicInstance::SendSkillToAI()
{
	if (pSkill == NULL)
		return;

	if (sTargetID >= NPC_BAND 
		|| (sTargetID == -1 && (pSkill->bMoral == MORAL_AREA_ENEMY || pSkill->bMoral == MORAL_SELF_AREA)))
	{		
		int total_magic_damage = 0;

		Packet result(AG_MAGIC_ATTACK_REQ); // this is the order it was in.
		result	<< sCasterID << bOpcode << sTargetID << nSkillID 
				<< sData1 << sData2 << sData3 
				<< sData4 << sData5 << sData6
				<< TO_USER(pSkillCaster)->getStatWithItemBonus(STAT_CHA);


		_ITEM_TABLE* pRightHand = TO_USER(pSkillCaster)->GetItemPrototype(RIGHTHAND);
		if (pRightHand != NULL && pRightHand->isStaff()
			&& TO_USER(pSkillCaster)->GetItemPrototype(LEFTHAND) == NULL)
		{
			if (pSkill->bType[0] == 3) {
				total_magic_damage += (int)((pRightHand->m_sDamage * 0.8f)+ (pRightHand->m_sDamage * pSkillCaster->GetLevel()) / 60);

				_MAGIC_TYPE3 *pType3 = g_pMain.m_Magictype3Array.GetData(nSkillID);
				if (pType3 == NULL)
					return;
				if (pSkillCaster->m_bMagicTypeRightHand == pType3->bAttribute )					
					total_magic_damage += (int)((pRightHand->m_sDamage * 0.8f) + (pRightHand->m_sDamage * pSkillCaster->GetLevel()) / 30);
				
				//TO-DO : Divide damage by 3 if duration = 0

				if (pType3->bAttribute == 4)
					total_magic_damage = 0;
			}
		}
		result << uint16(total_magic_damage);
		g_pMain.Send_AIServer(&result);		
	}
}

bool MagicInstance::ExecuteSkill(uint8 bType)
{
	switch (bType)
	{
		case 1: return ExecuteType1();
		case 2: return ExecuteType2();
		case 3: return ExecuteType3();
		case 4: return ExecuteType4();
		case 5: return ExecuteType5();
		case 6: return ExecuteType6();
		case 7: return ExecuteType7();
		case 8: return ExecuteType8();
		case 9: return ExecuteType9();
	}

	return false;
}

void MagicInstance::SendTransformationList()
{
	if (!pSkillCaster->isPlayer())
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TRANSFORM_LIST));
	result << nSkillID;
	pSkillCaster->m_nTransformationItem = pSkill->iUseItem;
	TO_USER(pSkillCaster)->Send(&result);
}

void MagicInstance::SendSkillFailed()
{
	SendSkill(sCasterID, sTargetID, MAGIC_FAIL, 
				nSkillID, sData1, sData2, sData3, sData4, 
				sData5, sData6, sData7, sData8);
}

void MagicInstance::SendSkill(int16 sSkillCaster /* = -1 */, int16 sSkillTarget /* = -1 */,	
								int8 opcode /* = -1 */, uint32 nSkillID /* = 0 */, 
								int16 sData1 /* = -999 */, int16 sData2 /* = -999 */, int16 sData3 /* = -999 */, int16 sData4 /* = -999 */, 
								int16 sData5 /* = -999 */, int16 sData6 /* = -999 */, int16 sData7 /* = -999 */, int16 sData8 /* = -999 */)
{
	Packet result(WIZ_MAGIC_PROCESS);
	int16 sid = 0, tid = 0;

	// Yes, these are all default value overrides.
	// This is completely horrible, but will suffice for now...

	if (opcode == -1)
		opcode = bOpcode;

	if (opcode == MAGIC_FAIL
		&& sSkillCaster >= NPC_BAND)
		return;

	if (sSkillCaster == -1)
		sSkillCaster = sCasterID;

	if (sSkillTarget == -1)
		sSkillTarget = sTargetID;

	if (nSkillID == 0)
		nSkillID = this->nSkillID;

	if (sData1 == -999)
		sData1 = this->sData1;
	if (sData2 == -999)
		sData2 = this->sData2;
	if (sData3 == -999)
		sData3 = this->sData3;
	if (sData4 == -999)
		sData4 = this->sData4;
	if (sData5 == -999)
		sData5 = this->sData5;
	if (sData6 == -999)
		sData6 = this->sData6;
	if (sData7 == -999)
		sData7 = this->sData7;
	if (sData8 == -999)
		sData8 = this->sData8;

	result	<< opcode << nSkillID << sSkillCaster << sSkillTarget 
			<< sData1 << sData2 << sData3 << sData4
			<< sData5 << sData6 << sData7 << sData8;

	if (pSkillCaster->isNPC())
	{
		CNpc *pNpc = g_pMain.m_arNpcArray.GetData(sSkillCaster);
		if (pNpc == NULL)
			return;

		pNpc->SendToRegion(&result);
	}
	else
	{
		CUser *pUser = NULL;
		if ((sSkillCaster < 0 || sSkillCaster >= MAX_USER)
			&& (sSkillTarget >= 0 && sSkillTarget < MAX_USER))
			pUser = g_pMain.GetUserPtr(sSkillTarget);
		else
			pUser = g_pMain.GetUserPtr(sSkillCaster);

		if (pUser == NULL)
			return;

		if (opcode == MAGIC_FAIL)
			TO_USER(pSkillCaster)->Send(&result);
		else
			pUser->SendToRegion(&result);
	}
}

bool MagicInstance::IsAvailable()
{
	CUser* pParty = NULL;   // When the target is a party....
	bool isNPC = (sCasterID >= NPC_BAND);		// Identifies source : true means source is NPC.

	if (pSkill == NULL)
		goto fail_return;

	int modulator = 0, Class = 0, moral = 0, skill_mod = 0 ;

	if (sTargetID >= 0 && sTargetID < MAX_USER) 
		moral = pSkillTarget->GetNation();
	else if (sTargetID >= NPC_BAND)     // Target existence check routine for NPC.          	
	{
		if (pSkillTarget == NULL || pSkillTarget->isDead())
			goto fail_return;	//... Assuming NPCs can't be resurrected.

		moral = pSkillTarget->GetNation();
	}
	else if (sTargetID == -1)  // AOE/Party Moral check routine.
	{
		if (isNPC)
		{
			moral = 1;
		}
		else
		{
			if (pSkill->bMoral == MORAL_AREA_ENEMY)
				moral = pSkillCaster->GetNation() == KARUS ? ELMORAD : KARUS;
			else 
				moral = pSkillCaster->GetNation();	
		}
	}
	else 
		moral = pSkillCaster->GetNation();

	switch( pSkill->bMoral ) {		// Compare morals between source and target character.
		case MORAL_SELF:   // #1         // ( to see if spell is cast on the right target or not )
			if (pSkillCaster != pSkillTarget)
				goto fail_return;
			break;
		case MORAL_FRIEND_WITHME:	// #2
			if (moral != 0 && 
				pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_FRIEND_EXCEPTME:	   // #3
			if (pSkillCaster->GetNation() != moral
				|| pSkillCaster == pSkillTarget)
				goto fail_return;
			break;
		case MORAL_PARTY:	 // #4
		{
			// NPCs can't *be* in parties.
			if (pSkillCaster->isNPC()
				|| (pSkillTarget != NULL && pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so...
			CUser *pCaster = TO_USER(pSkillCaster);

			// If the caster's not in a party, make sure the target's not someone other than themselves.
			if ((!pCaster->isInParty() && pSkillCaster != pSkillTarget)
				// Verify that the nation matches the intended moral
				|| pCaster->GetNation() != moral
				// and that if there is a target, they're in the same party.
				|| (pSkillTarget != NULL && 
					TO_USER(pSkillTarget)->m_sPartyIndex != pCaster->m_sPartyIndex))
				goto fail_return;
		} break;
		case MORAL_NPC:		// #5
			if (pSkillTarget == NULL
				|| !pSkillTarget->isNPC()
				|| pSkillTarget->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_PARTY_ALL:     // #6
//			if ( !m_pSrcUser->isInParty() ) goto fail_return;		
//			if ( !m_pSrcUser->isInParty() && sid != tid) goto fail_return;					

			break;
		case MORAL_ENEMY:	// #7
			if (pSkillCaster->GetNation() == moral)
				goto fail_return;
			break;	
		case MORAL_ALL:	 // #8
			// N/A
			break;
		case MORAL_AREA_ENEMY:		// #10
			// N/A
			break;
		case MORAL_AREA_FRIEND:		// #11
			if (pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_AREA_ALL:	// #12
			// N/A
			break;
		case MORAL_SELF_AREA:     // #13
			// Remeber, EVERYONE in the area is affected by this one. No moral check!!!
			break;
		case MORAL_CORPSE_FRIEND:		// #25
			if (pSkillCaster->GetNation() != moral
				// We need to revive *something*.
				|| pSkillTarget == NULL
				// We cannot revive ourselves.
				|| pSkillCaster == pSkillTarget
				// We can't revive living targets.
				|| pSkillTarget->isAlive())
				goto fail_return;
			break;
		case MORAL_CLAN:		// #14
		{
			// NPCs cannot be in clans.
			if (pSkillCaster->isNPC()
				|| (pSkillTarget != NULL && pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so....
			CUser * pCaster = TO_USER(pSkillCaster);

			// If the caster's not in a clan, make sure the target's not someone other than themselves.
			if ((!pCaster->isInClan() && pSkillCaster != pSkillTarget)
				// Verify the intended moral
				|| pCaster->GetNation() != moral
				// If we're targeting someone, that target must be in our clan.
				|| (pSkillTarget != NULL 
					&& TO_USER(pSkillTarget)->GetClanID() != pCaster->GetClanID()))
				goto fail_return;
		} break;

		case MORAL_CLAN_ALL:	// #15
			break;
//
	}

	// This only applies to users casting skills. NPCs are fine and dandy (we can trust THEM at least).
	if (pSkillCaster->isPlayer())
	{
		modulator = pSkill->sSkill % 10;     // Hacking prevention!
		if( modulator != 0 ) {	
			Class = pSkill->sSkill / 10;
			if( Class != TO_USER(pSkillCaster)->GetClass() ) goto fail_return;
			if( pSkill->sSkillLevel > TO_USER(pSkillCaster)->m_bstrSkill[modulator] ) goto fail_return;
		}
		else if (pSkill->sSkillLevel > pSkillCaster->GetLevel()) goto fail_return;

		if (pSkill->bType[0] == 1) {	// Weapons verification in case of COMBO attack (another hacking prevention).
			if (pSkill->sSkill == 1055 || pSkill->sSkill == 2055) {		// Weapons verification in case of dual wielding attacks !		
				_ITEM_TABLE *pLeftHand = TO_USER(pSkillCaster)->GetItemPrototype(LEFTHAND),
							*pRightHand = TO_USER(pSkillCaster)->GetItemPrototype(RIGHTHAND);

				if ((pLeftHand != NULL && !pLeftHand->isSword() && !pLeftHand->isAxe() && !pLeftHand->isMace())
					|| (pRightHand != NULL && !pRightHand->isSword() && !pRightHand->isAxe() && !pRightHand->isMace()))
					return false;
			}
			else if (pSkill->sSkill == 1056 || pSkill->sSkill == 2056) {	// Weapons verification in case of 2 handed attacks !
				_ITEM_TABLE	*pRightHand = TO_USER(pSkillCaster)->GetItemPrototype(RIGHTHAND);

				if (TO_USER(pSkillCaster)->GetItem(LEFTHAND)->nNum != 0
					|| (pRightHand != NULL 
						&& !pRightHand->isSword() && !pRightHand->isAxe() 
						&& !pRightHand->isMace() && !pRightHand->isSpear()))
					return false;
			}
		}

		if (bOpcode == MAGIC_EFFECTING) {    // MP/SP SUBTRACTION ROUTINE!!! ITEM AND HP TOO!!!	
			int total_hit = pSkillCaster->m_sTotalHit ;
			int skill_mana = (pSkill->sMsp * total_hit) / 100 ; 

			if (pSkill->bType[0] == 2 && pSkill->bFlyingEffect != 0) // Type 2 related...
				return true;		// Do not reduce MP/SP when flying effect is not 0.

			if (pSkill->bType[0] == 1 && sData1 > 1)
				return true;		// Do not reduce MP/SP when combo number is higher than 0.
 
			if (pSkill->bType[0] == 1 || pSkill->bType[0] == 2)
			{
				if (skill_mana > pSkillCaster->GetMana())
					goto fail_return;
			}
			else if (pSkill->sMsp > pSkillCaster->GetMana())
				goto fail_return;

			if (pSkill->bType[0] == 3 || pSkill->bType[0] == 4) {   // If the PLAYER uses an item to cast a spell.
				if (sCasterID >= 0 && sCasterID < MAX_USER)
				{	
					if (pSkill->iUseItem != 0) {
						_ITEM_TABLE* pItem = NULL;				// This checks if such an item exists.
						pItem = g_pMain.GetItemPtr(pSkill->iUseItem);
						if( !pItem ) return false;

						if ((pItem->m_bRace != 0 && TO_USER(pSkillCaster)->m_bRace != pItem->m_bRace)
							|| (pItem->m_bClass != 0 && !TO_USER(pSkillCaster)->JobGroupCheck(pItem->m_bClass))
							|| (pItem->m_bReqLevel != 0 && TO_USER(pSkillCaster)->GetLevel() < pItem->m_bReqLevel)
							|| (!TO_USER(pSkillCaster)->RobItem(pSkill->iUseItem, 1)))	
							return false;
					}
				}
			}
			if (pSkill->bType[0] == 1 || pSkill->bType[0] == 2)	// Actual deduction of Skill or Magic point.
				pSkillCaster->MSpChange(-(skill_mana));
			else if (pSkill->bType[0] != 4 || (pSkill->bType[0] == 4 && sTargetID == -1))
				pSkillCaster->MSpChange(-(pSkill->sMsp));

			if (pSkill->sHP > 0 && pSkill->sMsp == 0) {			// DEDUCTION OF HPs in Magic/Skill using HPs.
				if (pSkill->sHP > pSkillCaster->GetHealth()) goto fail_return;
				pSkillCaster->HpChange(-pSkill->sHP);
			}
		}
	}

	return true;      // Magic was successful! 

fail_return:    // In case of failure, send a packet(!)
	if (!isNPC)
		SendSkillFailed();

	return false;     // Magic was a failure!
}

bool MagicInstance::ExecuteType1()
{	
	if (pSkill == NULL)
		return false;

	int damage = 0;
	bool bResult = false;

	_MAGIC_TYPE1* pType = g_pMain.m_Magictype1Array.GetData(nSkillID);
	if (pType == NULL)
		return false;

	if (pSkillTarget != NULL && !pSkillTarget->isDead())
	{
		bResult = 1;
		damage = pSkillCaster->GetDamage(pSkillTarget, pSkill);
		pSkillTarget->HpChange(-damage, pSkillCaster);

		if(pSkillTarget->m_bReflectArmorType != 0)
			ReflectDamage(damage);

		// This is more than a little ugly.
		if (pSkillCaster->isPlayer())
			TO_USER(pSkillCaster)->SendTargetHP(0, sTargetID, -damage);
	}

	// If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
	if (pSkillCaster->isPlayer() && TO_USER(pSkillCaster)->m_bInvisibilityType != 0) 
		pSkillCaster->StateChangeServerDirect(7, INVIS_NONE);

	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	sData4 = (damage == 0 ? -104 : 0);
	SendSkill();	

	return bResult;
}

bool MagicInstance::ExecuteType2()
{
	if (pSkill == NULL)
		return false;

	int damage = 0;
	bool bResult = false;
	float total_range = 0.0f;	// These variables are used for range verification!
	int sx, sz, tx, tz;

	_MAGIC_TYPE2 *pType = g_pMain.m_Magictype2Array.GetData(nSkillID);
	if (pType == NULL
		// The user does not have enough arrows! We should point them in the right direction. ;)
		|| (pSkillCaster->isPlayer() 
			&& !TO_USER(pSkillCaster)->CheckExistItem(pSkill->iUseItem, pType->bNeedArrow)))
		return false;

	_ITEM_TABLE* pTable = NULL;
	if (pSkillCaster->isPlayer())
	{
		// Not wearing a left-handed bow
		pTable = TO_USER(pSkillCaster)->GetItemPrototype(LEFTHAND);
		if (pTable == NULL || !pTable->isBow())
		{
			pTable = TO_USER(pSkillCaster)->GetItemPrototype(RIGHTHAND);

			// Not wearing a right-handed (2h) bow either!
			if (pTable == NULL || !pTable->isBow())
				return false;
		}
	}
	else
	{
		// TO-DO: Verify this. It's more a placeholder than anything. 
		pTable = g_pMain.GetItemPtr(TO_NPC(pSkillCaster)->m_iWeapon_1);
		if (pTable == NULL)
			return false; 
	}

	// is this checked already?
	if (pSkillTarget == NULL || pSkillTarget->isDead())
		goto packet_send;
	
	total_range = pow(((pType->sAddRange * pTable->m_sRange) / 100.0f), 2.0f) ;     // Range verification procedure.
	sx = (int)pSkillCaster->GetX(); tx = (int)pSkillTarget->GetX();
	sz = (int)pSkillCaster->GetZ(); tz = (int)pSkillTarget->GetZ();
	
	if ((pow((float)(sx - tx), 2.0f) + pow((float)(sz - tz), 2.0f)) > total_range)	   // Target is out of range, exit.
		goto packet_send;
	
	damage = pSkillCaster->GetDamage(pSkillTarget, pSkill);  // Get damage points of enemy.	

	pSkillTarget->HpChange(-damage, pSkillCaster);     // Reduce target health point.

	if(pSkillTarget->m_bReflectArmorType != 0)
		ReflectDamage(damage);

	// This is more than a little ugly.
	if (pSkillCaster->isPlayer())
		TO_USER(pSkillCaster)->SendTargetHP(0, sTargetID, -damage);     // Change the HP of the target.

packet_send:
	// If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
	if (pSkillCaster->isPlayer() && TO_USER(pSkillCaster)->m_bInvisibilityType != 0) 
		pSkillCaster->StateChangeServerDirect(7, INVIS_NONE);
	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	sData4 = (damage == 0 ? -104 : 0);
	SendSkill();

	return bResult;
}

// Applied when a magical attack, healing, and mana restoration is done.
bool MagicInstance::ExecuteType3()
{	
	int damage = 0, duration_damage = 0;
	vector<Unit *> casted_member;

	_MAGIC_TYPE3* pType = g_pMain.m_Magictype3Array.GetData(nSkillID);
	if (pType == NULL) 
		return false;

	// If the target's a group of people...
	if (sTargetID == -1)
	{
		// TO-DO: Make this not completely and utterly suck (i.e. kill that loop!).
		SessionMap & sessMap = g_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& CMagicProcess::UserRegionCheck(pSkillCaster, pTUser, pSkill, pType->bRadius, sData1, sData3))
				casted_member.push_back(pTUser);
		}
		g_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{
			SendSkillFailed();
			return false;			
		}
	}
	else
	{	// If the target was a single unit.
		if (pSkillTarget == NULL 
			|| pSkillTarget->isDead() 
			|| (pSkillTarget->isPlayer() 
				&& TO_USER(pSkillTarget)->isBlinking())) 
			return false;
		
		casted_member.push_back(pSkillTarget);
	}
	
	foreach (itr, casted_member)
	{
		// assume player for now
		CUser* pTUser = TO_USER(*itr); // it's checked above, not much need to check it again
		if ((pType->sFirstDamage < 0) && (pType->bDirectType == 1) && (nSkillID < 400000))	// If you are casting an attack spell.
			damage = GetMagicDamage(pTUser, pType->sFirstDamage, pType->bAttribute) ;	// Get Magical damage point.
		else 
			damage = pType->sFirstDamage;

		if (pSkillCaster->isPlayer())
		{
			if( pSkillCaster->GetZoneID() == ZONE_SNOW_BATTLE && g_pMain.m_byBattleOpen == SNOW_BATTLE )
				damage = -10;		
		}

		if (pType->bDuration == 0) 
		{     // Non-Durational Spells.
			if (pType->bDirectType == 1)     // Health Point related !
			{			
				pTUser->HpChange(damage, pSkillCaster);     // Reduce target health point.
				if (pSkillCaster->isPlayer())
					TO_USER(pSkillCaster)->SendTargetHP( 0, (*itr)->GetID(), damage );     // Change the HP of the target.			
				if(pSkillTarget->m_bReflectArmorType != 0)
					ReflectDamage(damage);
			}
			else if ( pType->bDirectType == 2 || pType->bDirectType == 3 )    // Magic or Skill Point related !
				pTUser->MSpChange(damage);     // Change the SP or the MP of the target.		
			else if( pType->bDirectType == 4 )     // Armor Durability related.
				pTUser->ItemWoreOut( DEFENCE, -damage);     // Reduce Slot Item Durability
			else if( pType->bDirectType == 8 ) //
				continue;
				//Need to absorb HP from the target user to the source user
			else if( pType->bDirectType == 9 ) //Damage based on percentage of target's max HP
			{
				if (pType->sFirstDamage < 100)
					damage = (pType->sFirstDamage * pTUser->m_sHp) / -100;
				else
					damage = (pTUser->m_iMaxHp * (pType->sFirstDamage - 100)) / 100;

				pTUser->HpChange(damage, pSkillCaster);
				if (pSkillCaster->isPlayer())
					TO_USER(pSkillCaster)->SendTargetHP( 0, (*itr)->GetID(), damage );
			}
			if (sTargetID != -1)
				sData2 = 1;
		}
		else if (pType->bDuration != 0) {    // Durational Spells! Remember, durational spells only involve HPs.
			if (damage != 0) {		// In case there was first damage......
				pTUser->HpChange(damage, pSkillCaster);			// Initial damage!!!
				if (pSkillCaster->isPlayer())
					TO_USER(pSkillCaster)->SendTargetHP(0, pTUser->GetSocketID(), damage );     // Change the HP of the target. 
			}

			if (pTUser->m_bResHpType != USER_DEAD) {	// ���⵵ ��ȣ �ڵ� �߽�...
				if (pType->sTimeDamage < 0) {
					duration_damage = GetMagicDamage(pTUser, pType->sTimeDamage, pType->bAttribute) ;
				}
				else duration_damage = pType->sTimeDamage ;

				for (int k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {	// For continuous damages...
					if (pTUser->m_bHPInterval[k] == 5) {
						pTUser->m_tHPStartTime[k] = pTUser->m_tHPLastTime[k] = UNIXTIME;     // The durational magic routine.
						pTUser->m_bHPDuration[k] = pType->bDuration;
						pTUser->m_bHPInterval[k] = 2;		
						pTUser->m_bHPAmount[k] = duration_damage / ( pTUser->m_bHPDuration[k] / pTUser->m_bHPInterval[k] ) ;
						pTUser->m_sSourceID[k] = sCasterID;
						break;
					}
				}

				pTUser->m_bType3Flag = true;
			}

			if (pTUser->isInParty() && pType->sTimeDamage < 0)
				pTUser->SendPartyStatusUpdate(1, 1);

			pTUser->SendUserStatusUpdate(pType->bAttribute == POISON_R ? USER_STATUS_POISON : USER_STATUS_DOT, USER_STATUS_INFLICT);
		}

		if (pSkillCaster->isPlayer() //If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
			&& TO_USER(pSkillCaster)->m_bInvisibilityType != 0 
			&& damage < 0) //To allow for minor healing (as rogues)
			pSkillCaster->StateChangeServerDirect(7, INVIS_NONE);
	
		if ( pSkill->bType[1] == 0 || pSkill->bType[1] == 3 )
			SendSkill(sCasterID, pTUser->GetSocketID());

		// Tell the AI server we're healing someone (so that they can choose to pick on the healer!)
		if (pType->bDirectType == 1 && damage > 0
			&& sCasterID != sTargetID)
		{
			Packet result(AG_HEAL_MAGIC);
			result << sCasterID;
			g_pMain.Send_AIServer(&result);
		}
	}

	return true;
}

bool MagicInstance::ExecuteType4()
{
	int damage = 0;

	vector<CUser *> casted_member;
	if (pSkill == NULL)
		return false;

	_MAGIC_TYPE4* pType = g_pMain.m_Magictype4Array.GetData(nSkillID);
	if (pType == NULL)
		return false;

	if (!bIsRecastingSavedMagic && pSkillTarget->HasSavedMagic(nSkillID))
		return false;

	if (sTargetID == -1)
	{
		// TO-DO: Localise this. This is horribly unnecessary.
		SessionMap & sessMap = g_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& CMagicProcess::UserRegionCheck(pSkillCaster, pTUser, pSkill, pType->bRadius, sData1, sData3))
				casted_member.push_back(pTUser);
		}
		g_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{		
			if (sCasterID >= 0 && sCasterID < MAX_USER) 
				SendSkillFailed();

			return false;
		}
	}
	else 
	{
		// If the target was another single player.
		CUser* pTUser = g_pMain.GetUserPtr(sTargetID);
		if (pTUser == NULL 
			|| pTUser->isDead() || (pTUser->isBlinking() && !bIsRecastingSavedMagic)) 
			return false;

		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 1;
		CUser* pTUser = *itr;

		if (TO_USER(pSkillCaster)->m_bType4Buff[pType->bBuffType - 1] == 2 && sTargetID == -1) {		// Is this buff-type already casted on the player?
			bResult = 0;
			goto fail_return ;					
		}

		if ( nSkillID > 500000 && pTUser->isPlayer() )
			pTUser->InsertSavedMagic(nSkillID, pType->sDuration);

		switch (pType->bBuffType)
		{
			case BUFF_TYPE_HP_MP:
				if (pType->sMaxHP == 0 && pType->sMaxHPPct > 0)
					pTUser->m_sMaxHPAmount = (pType->sMaxHPPct - 100) * (pTUser->GetMaxHealth() - pTUser->m_sMaxHPAmount) / 100;
				else
					pTUser->m_sMaxHPAmount = pType->sMaxHP;

				if (pType->sMaxMP == 0 && pType->sMaxMPPct > 0)
					pTUser->m_sMaxMPAmount = (pType->sMaxMPPct - 100) * (pTUser->m_iMaxMp - pTUser->m_sMaxMPAmount) / 100;
				else
					pTUser->m_sMaxMPAmount = pType->sMaxMP;
				break;

			case BUFF_TYPE_AC:
				if (pType->sAC == 0 && pType->sACPct > 0)
					pTUser->m_sACAmount = pTUser->m_sTotalAc * (pType->sACPct - 100) / 100;
				else
					pTUser->m_sACAmount = pType->sAC;
				break;

			case BUFF_TYPE_SIZE:
				// These really shouldn't be hardcoded, but with mgame's implementation it doesn't seem we have a choice (as is).
				if (nSkillID == 490034)	// Bezoar!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_GIANT); 
				else if (nSkillID == 490035)	// Rice Cake!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_DWARF); 
				break;

			case BUFF_TYPE_DAMAGE:
				pTUser->m_bAttackAmount = pType->bAttack;
				break;

			case BUFF_TYPE_ATTACK_SPEED:
				pTUser->m_bAttackSpeedAmount = pType->bAttackSpeed;
				break;

			case BUFF_TYPE_SPEED:
				pTUser->m_bSpeedAmount = pType->bSpeed;
				break;

			case BUFF_TYPE_STATS:
				TO_USER(pTUser)->setStatBuff(STAT_STR, pType->bStr);
				TO_USER(pTUser)->setStatBuff(STAT_STA, pType->bSta);
				TO_USER(pTUser)->setStatBuff(STAT_DEX, pType->bDex);
				TO_USER(pTUser)->setStatBuff(STAT_INT, pType->bIntel);
				TO_USER(pSkillCaster)->setStatBuff(STAT_CHA, pType->bCha);	
				break;

			case BUFF_TYPE_RESISTANCES:
				pTUser->m_bFireRAmount = pType->bFireR;
				pTUser->m_bColdRAmount = pType->bColdR;
				pTUser->m_bLightningRAmount = pType->bLightningR;
				pTUser->m_bMagicRAmount = pType->bMagicR;
				pTUser->m_bDiseaseRAmount = pType->bDiseaseR;
				pTUser->m_bPoisonRAmount = pType->bPoisonR;
				break;

			case BUFF_TYPE_ACCURACY:
				pTUser->m_bHitRateAmount = pType->bHitRate;
				pTUser->m_sAvoidRateAmount = pType->sAvoidRate;
				break;	

			case BUFF_TYPE_MAGIC_POWER:
				pTUser->m_sMagicAttackAmount = (pType->bMagicAttack - 100) * TO_USER(pTUser)->getStat(STAT_CHA) / 100;
				break;

			case BUFF_TYPE_EXPERIENCE:
				pTUser->m_bExpGainAmount = pType->bExpPct;
				break;

			case BUFF_TYPE_WEIGHT:
				pTUser->m_bMaxWeightAmount = pType->bExpPct;
				break;

			case BUFF_TYPE_WEAPON_DAMAGE:
				// uses pType->Attack
				break;

			case BUFF_TYPE_WEAPON_AC:
				if (pType->sAC == 0 && pType->sACPct > 0)
					pTUser->m_sACAmount = pTUser->m_sTotalAc * (pType->sACPct - 100) / 100;
				else
					pTUser->m_sACAmount = pType->sAC;
				break;

			case BUFF_TYPE_LOYALTY:
				// uses pType->ExpPct
				break;

			case BUFF_TYPE_NOAH_BONUS:
				break;

			case BUFF_TYPE_PREMIUM_MERCHANT:
				TO_USER(pTUser)->m_bPremiumMerchant = true;
				break;

			case BUFF_TYPE_ATTACK_SPEED_ARMOR:
				pTUser->m_sACAmount -= pType->sAC;
				pTUser->m_bAttackAmount = pType->bAttack;
				break;

			case BUFF_TYPE_DAMAGE_DOUBLE:
				pTUser->m_bAttackAmount = pType->bAttack;
				break;

			case BUFF_TYPE_DISABLE_TARGETING:
				pTUser->m_bIsBlinded = true;
				break;

			case BUFF_TYPE_BLIND:
				pTUser->m_bIsBlinded = true;
				if (pTUser->isPlayer())
					TO_USER(pTUser)->SendUserStatusUpdate(USER_STATUS_BLIND, USER_STATUS_INFLICT);
				break;

			case BUFF_TYPE_FREEZE:
				//Proportianal to the target user's current HP.
				pTUser->m_bSpeedAmount = pType->bSpeed;
				break;

			case BUFF_TYPE_INSTANT_MAGIC:
				pTUser->m_bInstantCast = true;
				break;

			case BUFF_TYPE_DECREASE_RESIST:
				pTUser->m_bFireRAmount			= -(pType->bFireR / 100)	*	(pTUser->m_bFireR		- pTUser->m_bFireRAmount);
				pTUser->m_bColdRAmount			= -(pType->bColdR / 100)	*	(pTUser->m_bColdR		- pTUser->m_bColdRAmount);
				pTUser->m_bLightningRAmount		= -(pType->bLightningR / 100) * (pTUser->m_bLightningR - pTUser->m_bLightningRAmount);
				pTUser->m_bMagicRAmount			= -(pType->bMagicR / 100)	*	(pTUser->m_bMagicR		- pTUser->m_bMagicRAmount);
				pTUser->m_bDiseaseRAmount		= -(pType->bDiseaseR / 100) *	(pTUser->m_bDiseaseR	- pTUser->m_bDiseaseRAmount);
				pTUser->m_bPoisonRAmount		= -(pType->bPoisonR / 100)	*	(pTUser->m_bPoisonR	- pTUser->m_bPoisonRAmount);
				break;

			case BUFF_TYPE_MAGE_ARMOR:
				pTUser->m_bReflectArmorType =  pSkill->sSkill % 100;
				break;

			case BUFF_TYPE_PROHIBIT_INVIS:
				pTUser->m_bCanStealth = false;
				break;

			default:
				bResult = 0;
				goto fail_return;
		}


		pTUser->m_sDuration[pType->bBuffType - 1] = pType->sDuration;
		pTUser->m_tStartTime[pType->bBuffType - 1] = UNIXTIME;

		if (sTargetID != -1 && pSkill->bType[0] == 4)
		{
			if (sCasterID >= 0 && sCasterID < MAX_USER)
				pSkillCaster->MSpChange( -(pSkill->sMsp) );
		}

		if (sCasterID >= 0 && sCasterID < MAX_USER) 
			pTUser->m_bType4Buff[pType->bBuffType - 1] = (pSkillCaster->GetNation() == pTUser->GetNation() ? 2 : 1);
		else
			pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;

		pTUser->m_bType4Flag = true;

		pTUser->SetSlotItemValue();				// Update character stats.
		pTUser->SetUserAbility();

		if (pTUser->isInParty() && pTUser->m_bType4Buff[pType->bBuffType - 1] == 1)
			pTUser->SendPartyStatusUpdate(2, 1);

		pTUser->Send2AI_UserUpdateInfo();

	fail_return:
		if (pSkill->bType[1] == 0 || pSkill->bType[1] == 4)
		{
			CUser *pUser = (sCasterID >= 0 && sCasterID < MAX_USER ? TO_USER(pSkillCaster) : pTUser);

			sTargetID = (*itr)->GetSocketID();
			sData2 = bResult;
			if (!bIsRecastingSavedMagic)
				sData4 = (bResult == 1 || sData4 == 0 ? pType->sDuration : 0);
			sData6 = pType->bSpeed;
			SendSkill();

			if (pSkill->bMoral >= MORAL_ENEMY)
				pTUser->SendUserStatusUpdate(pType->bBuffType == BUFF_TYPE_SPEED ? USER_STATUS_SPEED : USER_STATUS_POISON, USER_STATUS_INFLICT);
		}
		
		if (bResult == 0
			&& sCasterID >= 0 && sCasterID < MAX_USER)
			SendSkill(sCasterID, (*itr)->GetSocketID(), MAGIC_FAIL);

		continue;
	}
	return true;
}

bool MagicInstance::ExecuteType5()
{
	// Disallow NPCs (for now?).
	if (pSkillCaster->isNPC()
		|| (pSkillTarget != NULL && pSkillTarget->isNPC()))
		return false;

	int damage = 0;
	int buff_test = 0; bool bType3Test = true, bType4Test = true; 	

	if (pSkill == NULL)
		return false;

	_MAGIC_TYPE5* pType = g_pMain.m_Magictype5Array.GetData(nSkillID);
	if (pType == NULL
		|| pSkillTarget == NULL 
		|| (pSkillTarget->isDead() && pType->bType != RESURRECTION) 
		|| (!pSkillTarget->isDead() && pType->bType == RESURRECTION)) 
		return false;

	switch (pType->bType) 
	{
		case REMOVE_TYPE3:		// REMOVE TYPE 3!!!
			for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
			{
				if (pSkillTarget->m_bHPAmount[i] < 0) {
					pSkillTarget->m_tHPStartTime[i] = 0;
					pSkillTarget->m_tHPLastTime[i] = 0;   
					pSkillTarget->m_bHPAmount[i] = 0;
					pSkillTarget->m_bHPDuration[i] = 0;				
					pSkillTarget->m_bHPInterval[i] = 5;
					pSkillTarget->m_sSourceID[i] = -1;

					if (pSkillTarget->isPlayer())
					{
						// TO-DO: Wrap this up (ugh, I feel so dirty)
						Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
						result << uint8(200); // removes all
						TO_USER(pSkillTarget)->Send(&result); 
					}
				}
			}

			buff_test = 0;
			for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
				buff_test += pSkillTarget->m_bHPDuration[j];
			if (buff_test == 0) pSkillTarget->m_bType3Flag = false;	

			// Check for Type 3 Curses.
			for (int k = 0; k < MAX_TYPE3_REPEAT; k++)
			{
				if (pSkillTarget->m_bHPAmount[k] < 0)
				{
					bType3Test = false;
					break;
				}
			}
  
			if (pSkillTarget->isPlayer()
				&& TO_USER(pSkillTarget)->isInParty() && bType3Test)
				TO_USER(pSkillTarget)->SendPartyStatusUpdate(1);
			break;

		case REMOVE_TYPE4:		// REMOVE TYPE 4!!!
			for (int i = 0; i < MAX_TYPE4_BUFF; i++)
			{
				if (pSkillTarget->m_bType4Buff[i] == 0)
					continue;

				uint8 buff_type = i + 1;

				switch (buff_type)
				{
				case 1: 
					pSkillTarget->m_sMaxHPAmount = 0;
					pSkillTarget->m_sMaxMPAmount = 0;
					break;

				case 2:
					pSkillTarget->m_sACAmount = 0;
					break;

				case 3:
					if (pSkillTarget->isPlayer())
						TO_USER(pSkillTarget)->StateChangeServerDirect(3, ABNORMAL_NORMAL);
					break;

				case 4:
					pSkillTarget->m_bAttackAmount = 100;
					break;

				case 5:
					pSkillTarget->m_bAttackSpeedAmount = 100;
					break;

				case 6:
					pSkillTarget->m_bSpeedAmount = 100;
					break;

				case 7:
					if (pSkillTarget->isPlayer())
						memset(TO_USER(pSkillTarget)->m_sStatItemBonuses, 0, sizeof(uint16) * STAT_COUNT);
					break;

				case 8:
					pSkillTarget->m_bFireRAmount = pSkillTarget->m_bColdRAmount = pSkillTarget->m_bLightningRAmount = 0;
					pSkillTarget->m_bMagicRAmount = pSkillTarget->m_bDiseaseRAmount = pSkillTarget->m_bPoisonRAmount = 0;
					break;

				case 9:
					pSkillTarget->m_bHitRateAmount = 100;
					pSkillTarget->m_sAvoidRateAmount = 100;
					break;
				}

				pSkillTarget->m_bType4Buff[i] = 0;
				CMagicProcess::SendType4BuffRemove(sTargetID, buff_type);
			}

			if (pSkillTarget->isPlayer())
			{
				TO_USER(pSkillTarget)->SetSlotItemValue();
				TO_USER(pSkillTarget)->SetUserAbility();
				TO_USER(pSkillTarget)->Send2AI_UserUpdateInfo();
			}

			buff_test = 0;
			for (int i = 0; i < MAX_TYPE4_BUFF; i++)
				buff_test += pSkillTarget->m_bType4Buff[i];
			if (buff_test == 0) pSkillTarget->m_bType4Flag = false;

			bType4Test = true ;
			for (int j = 0; j < MAX_TYPE4_BUFF; j++)
			{
				if (pSkillTarget->m_bType4Buff[j] == 1)
				{
					bType4Test = false;
					break;
				}
			}

			if (pSkillTarget->isPlayer() && TO_USER(pSkillTarget)->isInParty() && bType4Test)
				TO_USER(pSkillTarget)->SendPartyStatusUpdate(2, 0);
			break;
			
		case RESURRECTION:		// RESURRECT A DEAD PLAYER!!!
			if (pSkillTarget->isPlayer())
				TO_USER(pSkillTarget)->Regene(1, nSkillID);
			break;

		case REMOVE_BLESS:
			if (pSkillTarget->m_bType4Buff[0] == 2) {
				pSkillTarget->m_sDuration[0] = 0;		
				pSkillTarget->m_tStartTime[0] = 0;
				pSkillTarget->m_sMaxHPAmount = 0;
				pSkillTarget->m_sMaxMPAmount = 0;
				pSkillTarget->m_bType4Buff[0] = 0;

				CMagicProcess::SendType4BuffRemove(sTargetID, 1);

				if (pSkillTarget->isPlayer())
				{
					TO_USER(pSkillTarget)->SetSlotItemValue();
					TO_USER(pSkillTarget)->SetUserAbility();
					TO_USER(pSkillTarget)->Send2AI_UserUpdateInfo();
				}

				buff_test = 0;
				for (int i = 0; i < MAX_TYPE4_BUFF; i++)
					buff_test += pSkillTarget->m_bType4Buff[i];
				if (buff_test == 0) pSkillTarget->m_bType4Flag = false;

				bType4Test = true;
				for (int j = 0; j < MAX_TYPE4_BUFF; j++)
				{
					if (pSkillTarget->m_bType4Buff[j] == 1)
					{
						bType4Test = false;
						break;
					}
				}

				if (pSkillTarget->isPlayer() && TO_USER(pSkillTarget)->isInParty() && bType4Test) 
					TO_USER(pSkillTarget)->SendPartyStatusUpdate(2, 0);
			}

			break;
	}

	if (pSkill->bType[1] == 0 || pSkill->bType[1] == 5)
		SendSkill();

	return true;
}

bool MagicInstance::ExecuteType6()
{
	if (pSkill == NULL
		|| !pSkillCaster->isPlayer())
		return false;

	_MAGIC_TYPE6 * pType = g_pMain.m_Magictype6Array.GetData(nSkillID);
	uint32 iUseItem = 0;
	uint16 sDuration = 0;

	// We can ignore all these checks if we're just recasting on relog.
	if (!bIsRecastingSavedMagic)
	{
		if (pSkillTarget->HasSavedMagic(nSkillID))
			return false;

		if (pType == NULL
			|| TO_USER(pSkillCaster)->isAttackZone()
			|| pSkillCaster->isTransformed())
			return false;

		// Let's start by looking at the item that was used for the transformation.
		_ITEM_TABLE *pTable = g_pMain.GetItemPtr(pSkillCaster->m_nTransformationItem);

		// Also, for the sake of specific skills that bypass the list, let's lookup the 
		// item attached to the skill.
		_ITEM_TABLE *pTable2 = g_pMain.GetItemPtr(pSkill->iUseItem);

		// If neither of these items exist, we have a bit of a problem...
		if (pTable == NULL 
			&& pTable2 == NULL)
			return false;

		/*
			If it's a totem (which is apparently a ring), then we need to override it 
			with a gem (which are conveniently stored in the skill table!)

			The same is true for special items such as the Hera transformation scroll, 
			however we need to go by the item attached to the skill for this one as 
			these skills bypass the transformation list and thus do not set the flag.
		*/

		// Special items (e.g. Hera transformation scroll) use the scroll (tied to the skill)
		if ((pTable2 != NULL && pTable2->m_bKind == 255)
			// Totems (i.e. rings) take gems (tied to the skill)
			|| (pTable != NULL && pTable->m_bKind == 93)) 
			iUseItem = pSkill->iUseItem;
		// If we're using a normal transformation scroll, we can leave the item as it is.
		else 
			iUseItem = pSkillCaster->m_nTransformationItem;

		// Attempt to take the item (no further checks, so no harm in multipurposing)
		// If we add more checks, remember to change this check.
		if (!TO_USER(pSkillCaster)->RobItem(iUseItem, 1))
			return false;

		// User's casting a new skill. Use the full duration.
		sDuration = pType->sDuration;
	}
	else 
	{
		// Server's recasting the skill (kept on relog, zone change, etc.)
		int16 tmp = pSkillCaster->GetSavedMagicDuration(nSkillID);

		// Has it expired (or not been set?) -- just in case.
		if (tmp <= 0)
			return false;

		// it's positive, so no harm here in casting.
		sDuration = tmp;
	}

	// TO-DO : Save duration, and obviously end
	pSkillCaster->m_tTransformationStartTime = UNIXTIME;
	pSkillCaster->m_sTransformationDuration = sDuration;

	pSkillCaster->StateChangeServerDirect(3, nSkillID);
	pSkillCaster->m_bIsTransformed = true;

	// TO-DO : Give the users ALL TEH BONUSES!!

	SendSkill(sCasterID, sTargetID, bOpcode,
		nSkillID, sData1, 1, sData3, sDuration, 0, 0, 0, 0);

	pSkillCaster->InsertSavedMagic(nSkillID, sDuration);
	return true;
}


bool MagicInstance::ExecuteType7()
{
	return true;
}

// Warp, resurrection, and summon spells.
bool MagicInstance::ExecuteType8()
{	
	if (pSkill == NULL)
		return false;

	vector<CUser *> casted_member;
	_MAGIC_TYPE8* pType = g_pMain.m_Magictype8Array.GetData(nSkillID);
	if (pType == NULL)
		return false;

	if (sTargetID == -1)
	{
		// TO-DO: Localise this loop to make it not suck (the life out of the server).
		SessionMap & sessMap = g_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (CMagicProcess::UserRegionCheck(pSkillCaster, pTUser, pSkill, pType->sRadius, sData1, sData3))
				casted_member.push_back(pTUser);
		}
		g_socketMgr.ReleaseLock();

		if (casted_member.empty()) 
			return false;	
	}
	else 
	{	// If the target was another single player.
		CUser* pTUser = g_pMain.GetUserPtr(sTargetID);
		if (pTUser == NULL) 
			return false;
		
		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 0;
		_OBJECT_EVENT* pEvent = NULL;
		float x = 0.0f, z = 0.0f;
		x = (float)(myrand( 0, 400 )/100.0f);	z = (float)(myrand( 0, 400 )/100.0f);
		if( x < 2.5f )	x = 1.5f + x;
		if( z < 2.5f )	z = 1.5f + z;

		CUser* pTUser = *itr;
		_HOME_INFO* pHomeInfo = g_pMain.m_HomeArray.GetData(pTUser->GetNation());
		if (pHomeInfo == NULL)
			return false;

		if (pType->bWarpType != 11) 
		{   // Warp or summon related: targets CANNOT be dead.
			if (pTUser->isDead())
				goto packet_send;
		}
		// Resurrection related: we're reviving DEAD targets.
		else if (!pTUser->isDead()) 
			goto packet_send;

		// Is target already warping?			
		if (pTUser->m_bWarp)
			goto packet_send;

		switch(pType->bWarpType) {	
			case 1:		// Send source to resurrection point.
				// here we need to send to pTUser, but SendSkill() is dumb
				SendSkill(sCasterID, (*itr)->GetSocketID(), 
					bOpcode, nSkillID, sData1, 1, sData3);

				if (pTUser->GetMap() == NULL)
					continue;

				pEvent = pTUser->GetMap()->GetObjectEvent(pTUser->m_sBind);

				if( pEvent ) {
					pTUser->Warp(uint16((pEvent->fPosX + x) * 10), uint16((pEvent->fPosZ + z) * 10));	
				}
				// TO-DO: Remove this hardcoded nonsense
				else if(pTUser->GetNation() != pTUser->GetZoneID() && pTUser->GetZoneID() <= ELMORAD) 
				{	 // User is in different zone.
					if (pTUser->GetNation() == KARUS) // Land of Karus
						pTUser->Warp(uint16((852 + x) * 10), uint16((164 + z) * 10));
					else	// Land of Elmorad
						pTUser->Warp(uint16((177 + x) * 10), uint16((923 + z) * 10));
				}
				else if (pTUser->m_bZone == ZONE_BATTLE)
					pTUser->Warp(uint16((pHomeInfo->BattleZoneX + x) * 10), uint16((pHomeInfo->BattleZoneZ + z) * 10));	
				else if (pTUser->m_bZone == ZONE_RONARK_LAND)
					pTUser->Warp(uint16((pHomeInfo->FreeZoneX + x) * 10), uint16((pHomeInfo->FreeZoneZ + z) * 10));
				else
					pTUser->Warp(uint16((pTUser->GetMap()->m_fInitX + x) * 10), uint16((pTUser->GetMap()->m_fInitZ + z) * 10));	
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
			{
				// here we need to send to pTUser, but SendSkill() is dumb
				SendSkill(sCasterID, (*itr)->GetSocketID(), 
					bOpcode, nSkillID, sData1, 1, sData3);

				pTUser->m_bResHpType = USER_STANDING;     // Target status is officially alive now.
				pTUser->HpChange(pTUser->m_iMaxHp, pSkillCaster);	 // Refill HP.	
				pTUser->ExpChange( pType->sExpRecover/100 );     // Increase target experience.
				
				Packet result(AG_USER_REGENE);
				result << uint16((*itr)->GetSocketID()) << uint16(pTUser->GetZoneID());
				g_pMain.Send_AIServer(&result);
			} break;

			case 12:	// Summon a target within the zone.	
				if (pSkillCaster->GetZoneID() != pTUser->GetZoneID())   // Same zone? 
					goto packet_send;

				// here we need to send to pTUser, but SendSkill() is dumb
				SendSkill(sCasterID, (*itr)->GetSocketID(), 
					bOpcode, nSkillID, sData1, 1, sData3);
					
				pTUser->Warp(pSkillCaster->GetSPosX(), pSkillCaster->GetSPosZ());
				break;

			case 13:	// Summon a target outside the zone.			
				if (pSkillCaster->GetZoneID() == pTUser->GetZoneID())	  // Different zone? 
					goto packet_send;

				// here we need to send to pTUser, but SendSkill() is dumb
				SendSkill(sCasterID, (*itr)->GetSocketID(), 
					bOpcode, nSkillID, sData1, 1, sData3);

				pTUser->ZoneChange(pSkillCaster->GetZoneID(), pSkillCaster->GetX(), pSkillCaster->GetZ()) ;
				pTUser->UserInOut(INOUT_RESPAWN);
				//TRACE(" Summon ,, name=%s, x=%.2f, z=%.2f\n", pTUser->GetName(), pTUser->m_curx, pTUser->m_curz);
				break;

			case 20:	// Randomly teleport the source (within 20 meters)		
				// here we need to send to pTUser, but SendSkill() is dumb
				SendSkill(sCasterID, (*itr)->GetSocketID(), 
					bOpcode, nSkillID, sData1, 1, sData3);

				float warp_x, warp_z;		// Variable Initialization.
				float temp_warp_x, temp_warp_z;

				warp_x = pTUser->m_curx;	// Get current locations.
				warp_z = pTUser->m_curz;

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

				pTUser->Warp(uint16(warp_x * 10), uint16(warp_z * 10));
				break;

			case 21:	// Summon a monster within a zone.
				// LATER!!! 
				break;
			case 25:
				//This is used by Wild Advent (70 rogue skill) and Descent, teleport the user to the target user (Moral check to distinguish between the 2 skills)
				float dest_x, dest_z = 0.0f;
				if (pTUser->GetZoneID() != pSkillCaster->GetZoneID()) //If we're not even in the same zone, I can't teleport to you!
					return false;
				if (pSkill->bMoral < MORAL_ENEMY && pTUser->GetNation() != pSkillCaster->GetNation()) //I'm not the same nation as you are and thus can't t
					return false;
					
				dest_x = pTUser->m_curx;
				dest_z = pTUser->m_curz;

				if (pSkillCaster->isPlayer())
					TO_USER(pSkillCaster)->Warp(uint16(dest_x * 10), uint16(dest_z * 10));
				break;
		}

		bResult = 1;
		
packet_send:
		sData2 = bResult;
		// send to caster
		SendSkill(sCasterID, (*itr)->GetSocketID());
	}
	return true;
}

bool MagicInstance::ExecuteType9()
{
	if (pSkill == NULL
		|| !pSkillCaster->isPlayer())
		return false;

	_MAGIC_TYPE9* pType = g_pMain.m_Magictype9Array.GetData(nSkillID);
	if (pType == NULL)
		return false;

	sData2 = 1;
	
	if (pSkillTarget->HasSavedMagic(nSkillID))
	{
		sData2 = 0;
		SendSkillFailed();
		return false;
	}
	
	if (pType->bStateChange <= 2 && pSkillTarget->canStealth())
	{
		pSkillCaster->StateChangeServerDirect(7, pType->bStateChange); // Update the client to be invisible
		pSkillTarget->InsertSavedMagic(nSkillID, pType->sDuration);
	}
	else if (pType->bStateChange >= 3 && pType->bStateChange <= 4)
	{
		Packet stealth(WIZ_STEALTH);
		stealth << uint8(1) << uint16(pType->sRadius);
		if (TO_USER(pSkillCaster)->isInParty())
		{
			_PARTY_GROUP* pParty = g_pMain.m_PartyArray.GetData(TO_USER(pSkillCaster)->m_sPartyIndex);
			if (pParty == NULL)
				return false;

			for (int i = 0; i < MAX_PARTY_USERS; i++)
			{
				CUser *pUser = g_pMain.GetUserPtr(pParty->uid[i]);
				if (pUser == NULL)
					continue;
				//To-do : save the skill for all the party members
				pUser->Send(&stealth);
			}
		}
		else
		{
			TO_USER(pSkillCaster)->Send(&stealth);
		}

		//TO-DO : Save the skill in the saved skill list
	}

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_EFFECTING));
	result	<< nSkillID << sCasterID << sTargetID
			<< sData1 << sData2 << sData3 << sData4 << sData5 << sData6 << sData7 << sData8;
	if (TO_USER(pSkillCaster)->isInParty() && pType->bStateChange == 4)
		g_pMain.Send_PartyMember(TO_USER(pSkillCaster)->m_sPartyIndex, &result);
	else
		TO_USER(pSkillCaster)->Send(&result);

	return true;
}

short MagicInstance::GetMagicDamage(Unit *pTarget, int total_hit, int attribute)
{	
	short damage = 0, temp_hit = 0, righthand_damage = 0, attribute_damage = 0 ; 
	int random = 0, total_r = 0 ;
	BYTE result; 

	if (pTarget == NULL
		|| pSkillCaster == NULL
		|| pTarget->isDead()
		|| pSkillCaster->isDead())
		return 0;

	uint16 sMagicAmount = 0;
	if (pSkillCaster->isNPC())
	{
		result = pSkillCaster->GetHitRate(pTarget->m_sTotalHitrate / pSkillCaster->m_sTotalEvasionrate); 
	}
	else
	{
		CUser *pUser = TO_USER(pSkillCaster);
		uint8 bCha = pUser->getStat(STAT_CHA);
		if (bCha > 86)
			sMagicAmount = pUser->m_sMagicAttackAmount - (bCha - 86);

		total_hit *= pUser->getStat(STAT_CHA) / 186;
		result = SUCCESS;
	}
		
	if (result != FAIL) 
	{
		// In case of SUCCESS.... 
		switch (attribute)
		{
			case FIRE_R: 
				total_r = pTarget->m_bFireR + pTarget->m_bFireRAmount;
				break;
			case COLD_R :
				total_r = pTarget->m_bColdR + pTarget->m_bColdRAmount;
				break;
			case LIGHTNING_R :
				total_r = pTarget->m_bLightningR + pTarget->m_bLightningRAmount; 
				break;
			case MAGIC_R :
				total_r = pTarget->m_bMagicR + pTarget->m_bMagicRAmount;
				break;
			case DISEASE_R :
				total_r = pTarget->m_bDiseaseR + pTarget->m_bDiseaseRAmount;
				break;
			case POISON_R :			
				total_r = pTarget->m_bPoisonR + pTarget->m_bPoisonRAmount;
				break;
		}
		
		if (pSkillCaster->isPlayer()) 
		{
			CUser *pUser = TO_USER(pSkillCaster);
			_ITEM_TABLE *pRightHand = pUser->GetItemPrototype(RIGHTHAND);
			if (pRightHand != NULL && pRightHand->isStaff()
				&& pUser->GetItemPrototype(LEFTHAND) == NULL)
			{
				righthand_damage = pRightHand->m_sDamage;
					
				if (pSkillCaster->m_bMagicTypeRightHand == attribute)
					attribute_damage = pRightHand->m_sDamage;
			}
			else 
			{
				righthand_damage = 0;
			}
		}

		damage = (short)(230 * total_hit / (total_r + 250));
		random = myrand(0, damage);
		damage = (short)(random * 0.3f + (damage * 0.85f)) - sMagicAmount;

		if (pSkillCaster->isNPC())
			damage -= ((3 * righthand_damage) + (3 * attribute_damage));
		else if (attribute != 4)	// Only if the staff has an attribute.
			damage -= (short)(((righthand_damage * 0.8f) + (righthand_damage * pSkillCaster->GetLevel()) / 60) + ((attribute_damage * 0.8f) + (attribute_damage * pSkillCaster->GetLevel()) / 30));
	}

	// Apply boost for skills matching weather type.
	// This isn't actually used officially, but I think it's neat...
	GetWeatherDamage(damage, attribute);
	return damage / 3;		
}

short MagicInstance::GetWeatherDamage(short damage, int attribute)
{
	// Give a 10% damage output boost based on weather (and skill's elemental attribute)
	if ((g_pMain.m_nWeather == WEATHER_FINE && attribute == ATTRIBUTE_FIRE)
		|| (g_pMain.m_nWeather == WEATHER_RAIN && attribute == ATTRIBUTE_LIGHTNING)
		|| (g_pMain.m_nWeather == WEATHER_SNOW && attribute == ATTRIBUTE_ICE))
		damage = (damage * 110) / 100;

	return damage;
}

void MagicInstance::Type6Cancel()
{
	if (!pSkillCaster->isPlayer()
		|| !pSkillCaster->isTransformed())
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_CANCEL_TYPE6));
	uint32 nSkillID = TO_USER(pSkillCaster)->m_bAbnormalType;

	// TO-DO: Reset stat changes, recalculate stats.
	TO_USER(pSkillCaster)->m_bIsTransformed = false;
	TO_USER(pSkillCaster)->Send(&result);

	pSkillCaster->StateChangeServerDirect(3, ABNORMAL_NORMAL);
	TO_USER(pSkillCaster)->m_savedMagicMap.erase(nSkillID);
}

void MagicInstance::Type9Cancel()
{
	if (pSkill == NULL)
		return;

	_MAGIC_TYPE9 *pType = g_pMain.m_Magictype9Array.GetData(nSkillID);
	if (pType == NULL)
		return;

	uint8 bResponse = 0;
	
	if (pType->bStateChange <= 2 || pType->bStateChange >= 5 && pType->bStateChange < 7) //Stealths
	{
		TO_USER(pSkillCaster)->StateChangeServerDirect(7, INVIS_NONE);
		TO_USER(pSkillCaster)->m_savedMagicMap.erase(nSkillID);
		bResponse = 91;
	}
	else if (pType->bStateChange >= 3 && pType->bStateChange <= 4) //Lupine etc.
	{
		Packet stealth(WIZ_STEALTH);
		stealth << uint16(0) << uint8(0);
		TO_USER(pSkillCaster)->Send(&stealth);
		bResponse = 92;
	}
	else if (pType->bStateChange == 7) //Guardian pet related
	{
		Packet pet(WIZ_PET, uint8(1));
		pet << uint16(1) << uint16(6);
		TO_USER(pSkillCaster)->Send(&pet);
		bResponse = 93;
	}


	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
		result << bResponse;
	TO_USER(pSkillCaster)->Send(&result);
}

void MagicInstance::Type4Cancel()
{
	if (pSkill == NULL)
		return;

	if (pSkillTarget != pSkillCaster)
		return;

	_MAGIC_TYPE4* pType = g_pMain.m_Magictype4Array.GetData(nSkillID);
	if (pType == NULL)
		return;

	bool buff = false;
	switch (pType->bBuffType)
	{
		case BUFF_TYPE_HP_MP:
			if (pSkillCaster->m_sMaxHPAmount > 0
				|| pSkillCaster->m_sMaxMPAmount > 0)
			{
				pSkillCaster->m_sMaxHPAmount = 0;
				pSkillCaster->m_sMaxMPAmount = 0;
				buff = true;
			}
			break;

		case BUFF_TYPE_AC:
			if (pSkillCaster->m_sACAmount > 0) 
			{
				pSkillCaster->m_sACAmount = 0;
				buff = true;
			}
			break;

		case BUFF_TYPE_SIZE:
			if (pSkillCaster->isPlayer())
			{
				TO_USER(pSkillCaster)->StateChangeServerDirect(3, ABNORMAL_NORMAL);
				buff = true;
			}
			break;

		case BUFF_TYPE_DAMAGE:
			if (pSkillCaster->m_bAttackAmount > 100) 
			{
				pSkillCaster->m_bAttackAmount = 100;
				buff = true;
			}
			break;

		case BUFF_TYPE_ATTACK_SPEED:
			if (pSkillCaster->m_bAttackSpeedAmount > 100) 
			{
				pSkillCaster->m_bAttackSpeedAmount = 100;	
				buff = true;
			}
			break;	

		case BUFF_TYPE_SPEED:
			if (pSkillCaster->m_bSpeedAmount > 100)
			{
				pSkillCaster->m_bSpeedAmount = 100;
				buff = true;
			}
			break;

		case BUFF_TYPE_STATS:
			if (pSkillCaster->isPlayer()
				&& TO_USER(pSkillCaster)->getStatBuffTotal() > 0) {
				// TO-DO: Implement reset
				memset(TO_USER(pSkillCaster)->m_bStatBuffs, 0, sizeof(uint8) * STAT_COUNT);
				buff = true;
			}
			break;

		case BUFF_TYPE_RESISTANCES:
			if ((pSkillCaster->m_bFireRAmount + pSkillCaster->m_bColdRAmount + pSkillCaster->m_bLightningRAmount +
				pSkillCaster->m_bMagicRAmount + pSkillCaster->m_bDiseaseRAmount + pSkillCaster->m_bPoisonRAmount) > 0) {
				pSkillCaster->m_bFireRAmount = 0;
				pSkillCaster->m_bColdRAmount = 0;
				pSkillCaster->m_bLightningRAmount = 0;
				pSkillCaster->m_bMagicRAmount = 0;
				pSkillCaster->m_bDiseaseRAmount = 0;
				pSkillCaster->m_bPoisonRAmount = 0;
				buff = true;
			}
			break;	

		case BUFF_TYPE_ACCURACY:
			if ((pSkillCaster->m_bHitRateAmount + pSkillCaster->m_sAvoidRateAmount) > 200)
			{
				pSkillCaster->m_bHitRateAmount = 100;
				pSkillCaster->m_sAvoidRateAmount = 100;
				buff = true;
			}
			break;

		case BUFF_TYPE_MAGIC_POWER:
			if (pSkillCaster->m_sMagicAttackAmount > 0)
			{
				pSkillCaster->m_sMagicAttackAmount = 0;
				buff = true;
			}
			break;

		case BUFF_TYPE_EXPERIENCE:
			if (pSkillCaster->m_bExpGainAmount > 100)
			{
				pSkillCaster->m_bExpGainAmount = 100;
				buff = true;
			}
			break;

		case BUFF_TYPE_WEIGHT:
			if (pSkillCaster->m_bMaxWeightAmount > 100)
			{
				pSkillCaster->m_bMaxWeightAmount = 100;
				buff = true;
			}
			break;

		case BUFF_TYPE_WEAPON_DAMAGE:
			// TO-DO
			break;

		case BUFF_TYPE_WEAPON_AC:
			if (pSkillCaster->m_sACAmount > 0) 
			{
				pSkillCaster->m_sACAmount = 0;
				buff = true;
			}
			break;
		case BUFF_TYPE_LOYALTY:
			// TO-DO
			break;
		case BUFF_TYPE_PREMIUM_MERCHANT:
			if(TO_USER(pSkillCaster)->m_bPremiumMerchant)
				TO_USER(pSkillCaster)->m_bPremiumMerchant = false;
			break;

		case BUFF_TYPE_ATTACK_SPEED_ARMOR:
			pSkillCaster->m_sACAmount += pType->sAC;
			pSkillCaster->m_bAttackAmount = 100;
			break;

		case BUFF_TYPE_DAMAGE_DOUBLE:
			pSkillCaster->m_bAttackAmount = pType->bAttack;
			break;

		case BUFF_TYPE_INSTANT_MAGIC:
			if (pSkillCaster->m_bInstantCast)
				pSkillCaster->m_bInstantCast = false;
			break;

		case BUFF_TYPE_MAGE_ARMOR:
			if(pSkillTarget->m_bReflectArmorType > 0)
				pSkillTarget->m_bReflectArmorType = 0;
			break;
	}
	
	if (buff)
	{
		pSkillCaster->m_sDuration[pType->bBuffType - 1] = 0;
		pSkillCaster->m_tStartTime[pType->bBuffType - 1] = 0;
		pSkillCaster->m_bType4Buff[pType->bBuffType - 1] = 0;

		if (pSkillCaster->isPlayer())
		{
			TO_USER(pSkillCaster)->SetSlotItemValue();
			TO_USER(pSkillCaster)->SetUserAbility();
			TO_USER(pSkillCaster)->SendItemMove(2);
			TO_USER(pSkillCaster)->Send2AI_UserUpdateInfo();

			Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
			result << pType->bBuffType;
			TO_USER(pSkillCaster)->Send(&result);
		}
	}

	int buff_test = 0;
	for (int i = 0; i < MAX_TYPE4_BUFF; i++)
		buff_test += pSkillCaster->m_bType4Buff[i];
	if (buff_test == 0) pSkillCaster->m_bType4Flag = false;	

	if (pSkillCaster->isPlayer() && !pSkillCaster->m_bType4Flag
		&& TO_USER(pSkillCaster)->isInParty())
		TO_USER(pSkillCaster)->SendPartyStatusUpdate(2);

	if (TO_USER(pSkillCaster)->m_savedMagicMap.find(nSkillID) != TO_USER(pSkillCaster)->m_savedMagicMap.end())
		TO_USER(pSkillCaster)->m_savedMagicMap.erase(nSkillID);
}

void MagicInstance::Type3Cancel()
{
	if (pSkill == NULL
		|| pSkillCaster != pSkillTarget)
		return;

	// Should this take only the specified skill? I'm thinking so.
	_MAGIC_TYPE3* pType = g_pMain.m_Magictype3Array.GetData(nSkillID);
	if (pType == NULL)
		return;

	for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
	{
		if (pSkillCaster->m_bHPAmount[i] > 0)
		{
			pSkillCaster->m_tHPStartTime[i] = 0;
			pSkillCaster->m_tHPLastTime[i] = 0;   
			pSkillCaster->m_bHPAmount[i] = 0;
			pSkillCaster->m_bHPDuration[i] = 0;				
			pSkillCaster->m_bHPInterval[i] = 5;
			pSkillCaster->m_sSourceID[i] = -1;
			break;
		}
	}

	if (pSkillCaster->isPlayer())
	{
		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
		result << uint8(100);
		TO_USER(pSkillCaster)->Send(&result); 
	}

	int buff_test = 0;
	for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
		buff_test += pSkillCaster->m_bHPDuration[j];
	if (buff_test == 0) pSkillCaster->m_bType3Flag = false;	

	if (pSkillCaster->isPlayer() && !pSkillCaster->m_bType3Flag
		&& TO_USER(pSkillCaster)->isInParty())
		TO_USER(pSkillCaster)->SendPartyStatusUpdate(1, 0);
}

void CMagicProcess::SendType4BuffRemove(short tid, BYTE buff)
{
	CUser* pTUser = g_pMain.GetUserPtr(tid); 
	if (pTUser == NULL) 
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
	result << buff;
	pTUser->Send(&result);
}

void MagicInstance::Type4Extend()
{
	if (pSkill == NULL)
		return;

	_MAGIC_TYPE4 *pType = g_pMain.m_Magictype4Array.GetData(nSkillID);
	if (pType == NULL)
		return;

	CUser* pTUser = g_pMain.GetUserPtr(sTargetID);
	if (pTUser == NULL) 
		return;

	if (pSkillCaster->isPlayer() && pTUser->RobItem(800022000, 1))
	{
		pTUser->m_sDuration[pType->bBuffType -1] *= 2;
		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_EXTEND));
		result << uint32(nSkillID);
		pTUser->Send(&result);
	}	
}

void MagicInstance::ReflectDamage(int32 damage)
{
	if(damage < 0)
		damage *= -1;

	int16 total_resistance_caster = 0;
	int32 reflect_damage = 0;

	switch(pSkillTarget->m_bReflectArmorType)
	{
		case FIRE_DAMAGE:
			total_resistance_caster = pSkillCaster->m_bFireR + pSkillCaster->m_bFireRAmount;
			reflect_damage = ((230 * damage) / (total_resistance_caster + 250)) / 100 * 25;
			pSkillCaster->HpChange(-damage, pSkillTarget);
		break;
		
		case ICE_DAMAGE:
			total_resistance_caster = pSkillCaster->m_bColdR + pSkillCaster->m_bColdRAmount;
			reflect_damage = ((230 * damage) / (total_resistance_caster + 250)) / 100 * 25;
			pSkillCaster->HpChange(-damage, pSkillTarget);
		break;

		case LIGHTNING_DAMAGE:
			total_resistance_caster = pSkillCaster->m_bLightningR + pSkillCaster->m_bLightningRAmount;
			reflect_damage = ((230 * damage) / (total_resistance_caster + 250)) / 100 * 25;
			pSkillCaster->HpChange(-damage, pSkillTarget);
		break;
	}
}
#include "StdAfx.h"

void CUser::QuestDataRequest()
{
	// Sending this now is probably wrong, but it's cleaner than it was before.
	Packet result(WIZ_QUEST, uint8(1));
	result << uint16(m_questMap.size());
	foreach (itr, m_questMap)
		result	<< itr->first << itr->second;
	Send(&result);

	// NOTE: To activate Seed/Max (the kid that gets eaten by a worm), we need to send:
	// sub opcode  = 2
	// quest ID    = 500 (says something about beginner weapons), 
	// quest state = 1 (pending completion)
	// and additionally, a final byte to specify whether he's shown or not (1 - no, 2 - yes)
	// Before this is implemented I think it best to research this updated system further.

	// Unlock skill data (level 70 skill quest).
	// NOTE: This is just temporary until we can load quest data.
	// At which time, we'll just send a list of quest IDs & their states (as is done here, just.. hardcoded)
	{
		Packet result(WIZ_QUEST, uint8(1));
		uint16 Class = m_sClass % 100;
		if (Class == 1 || Class == 5 || Class == 6)
			result << uint16(3) << uint16(51) << uint8(2) << uint16(510) << uint8(2) << uint16(511) << uint8(2); // if 50+baseclass quest ID is completed
		else if (Class == 2 || Class == 7 || Class == 8)
			result << uint16(4) << uint16(52) << uint8(2) << uint16(512) << uint8(2) << uint16(513) << uint8(2) << uint16(514) << uint8(2);
		else if (Class == 3 || Class == 9 || Class == 10)
			result << uint16(4) << uint16(53) << uint8(2) << uint16(515) << uint8(2) << uint16(516) << uint8(2) << uint16(517) << uint8(2);
		else if (Class == 4 || Class == 11 || Class == 12)
			result << uint16(7) << uint16(54) << uint8(2) << uint16(518) << uint8(2) << uint16(519) << uint8(2) << uint16(520) << uint8(2) << uint16(521) << uint8(2) << uint16(522) << uint8(2) << uint16(523) << uint8(2);
		Send(&result);
	}
}

void CUser::QuestV2PacketProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	uint32 nQuestID = pkt.read<uint32>();

	_QUEST_HELPER * pQuestHelper = g_pMain.m_QuestHelperArray.GetData(nQuestID);
	if (pQuestHelper == NULL)
		return;

	// Is this quest for this player's nation? NOTE: 3 indicates both (why not 0, I don't know)
	if ((pQuestHelper->bNation != 3
		&& pQuestHelper->bNation != GetNation())
		// Is the player's level high enough to do this quest?
		|| (pQuestHelper->bLevel > GetLevel())
		// Are we the correct class? NOTE: 5 indicates any class.
		|| (pQuestHelper->bClass != 5 
			&& !JobGroupCheck(pQuestHelper->bClass))
		// Are we in the correct zone? NOTE: This isn't checked officially, may be for good reason.
		|| GetZoneID() != pQuestHelper->bZone)
		return;

	// If we're the same min. level as the quest requires, 
	// do we have the min. required XP? Seems kind of silly, but OK..
	if (pQuestHelper->bLevel == GetLevel()
		&& pQuestHelper->nExp > m_iExp)
		return;

	switch (opcode)
	{
	case 3:
	case 7:
		QuestV2ExecuteHelper(pQuestHelper);
		break;

	case 4:
		QuestV2CheckFulfill(pQuestHelper);
		break;

	case 5:
		SaveEvent(pQuestHelper->sEventDataIndex, 4);

		if (m_sEventDataIndex > 0 
			&& m_sEventDataIndex == pQuestHelper->sEventDataIndex)
		{
			QuestV2MonsterDataDeleteAll();
			QuestV2MonsterDataRequest();
		}

		// This really could be rewritten to make more sense.
		if (GetZoneID() - 101 <= 99)
			KickOutZoneUser(TRUE);
		break;

	case 6:
		if (!CheckExistEvent(pQuestHelper->sEventDataIndex, 2))
			SaveEvent(pQuestHelper->sEventDataIndex, 1);
		break;

	case 12:
		if (!CheckExistEvent(pQuestHelper->sEventDataIndex, 3))
			SaveEvent(pQuestHelper->sEventDataIndex, 1);
		break;
	}
}

void CUser::SaveEvent(uint16 sQuestID, uint8 bQuestState)
{
	_QUEST_MONSTER * pQuestMonster = g_pMain.m_QuestMonsterArray.GetData(sQuestID);

	if (pQuestMonster != NULL
		&& bQuestState == 1
		&& m_sEventDataIndex > 0)
		return;

	m_questMap[sQuestID] = bQuestState;

	// Don't need to handle special/kill quests any further
	if (sQuestID >= QUEST_KILL_GROUP1)
		return;

	Packet result(WIZ_QUEST, uint8(2));
	result << sQuestID << bQuestState;
	Send(&result);

	if (m_sEventDataIndex == sQuestID 
		&& bQuestState == 2)
	{
		QuestV2MonsterDataDeleteAll();
		QuestV2MonsterDataRequest();
	}

	if (bQuestState == 1
		&& pQuestMonster != NULL)
	{
		// TO-DO: Decipher this into more meaningful code. :p
		int16 v11 = ((int16)((uint32)(6711 * sQuestID) >> 16) >> 10) - (sQuestID >> 15);
		int16 v12 = ((int16)((uint32)(5243 * (int16)(sQuestID - 10000 * v11)) >> 16) >> 3)
			- ((int16)(sQuestID - 10000 * v11) >> 15);

        SaveEvent(32005, (uint16)((int16)((uint32)(6711 * sQuestID) >> 16) >> 10) 
			- (int16)(sQuestID >> 15));
		SaveEvent(32006, (uint8)v12);
		SaveEvent(32007, sQuestID
			- 100 * ((uint16)((int16)((uint32)(5243 * sQuestID) >> 16) >> 3) - (sQuestID >> 15)));
		m_sEventDataIndex = sQuestID;
		QuestV2MonsterDataRequest();
	}

}

void CUser::DeleteEvent(uint16 sQuestID)
{
	m_questMap.erase(sQuestID);
}

bool CUser::CheckExistEvent(uint16 sQuestID, uint8 bQuestState)
{
	// Attempt to find a quest with that ID in the map
	QuestMap::iterator itr = m_questMap.find(sQuestID);

	// If it doesn't exist, it doesn't exist. 
	// Unless of course, we wanted it to not exist, in which case we're right!
	// (this is pretty annoyingly dumb, but we'd have to change existing EVT logic to fix this)
	if (itr == m_questMap.end())
		return bQuestState == 0;

	return itr->second == bQuestState;
}

void CUser::QuestV2MonsterCountAdd(uint16 sNpcID)
{
	if (m_sEventDataIndex == 0)
		return;

	// it looks like they use an active quest ID which is kind of dumb
	// we'd rather search through the player's active quests for applicable mob counts to increment
	// but then, this system can't really handle that (static counts). More research is necessary.
	uint16 sQuestNum = 0; // placeholder so that we can implement logic mockup
	_QUEST_MONSTER *pQuestMonster = g_pMain.m_QuestMonsterArray.GetData(sQuestNum);
	if (pQuestMonster == NULL)
		return;

	// TO-DO: Implement obscure zone ID logic

	for (int group = 0; group < QUEST_MOB_GROUPS; group++)
	{
		for (int i = 0; i < QUEST_MOBS_PER_GROUP; i++)
		{
			if (pQuestMonster->sNum[group][i] != sNpcID)
				continue;

			if (m_bKillCounts[group] + 1 > pQuestMonster->sCount[group])
				return;

			m_bKillCounts[group]++;
			SaveEvent(QUEST_KILL_GROUP1 + group, m_bKillCounts[group]);
			Packet result(WIZ_QUEST, uint8(9));
			result << uint8(2) << uint8(group + 1) << m_bKillCounts[group];
			Send(&result);
			return;
		}
	}
}

uint8 CUser::QuestV2CheckMonsterCount(uint16 sQuestID)
{
	// Attempt to find a quest with that ID in the map
	QuestMap::iterator itr = m_questMap.find(sQuestID);

	// If it doesn't exist, it doesn't exist. 
	if (itr == m_questMap.end())
		return 0;

	return itr->second;
}

void CUser::QuestV2MonsterDataDeleteAll()
{
	memset(&m_bKillCounts, 0, sizeof(m_bKillCounts));
	m_sEventDataIndex = 0;

	for (int i = QUEST_KILL_GROUP1; i <= 32007; i++)
		DeleteEvent(i);
}

void CUser::QuestV2MonsterDataRequest()
{
	Packet result(WIZ_QUEST, uint8(9));

	// Still not sure, but it's generating an ID.
	m_sEventDataIndex = 
		10000	*	QuestV2CheckMonsterCount(32005) +
		100		*	QuestV2CheckMonsterCount(32006) +
					QuestV2CheckMonsterCount(32007);

	// Lookup the current kill counts for each mob group in the active quest
	m_bKillCounts[0] = QuestV2CheckMonsterCount(QUEST_KILL_GROUP1);
	m_bKillCounts[1] = QuestV2CheckMonsterCount(QUEST_KILL_GROUP2);
	m_bKillCounts[2] = QuestV2CheckMonsterCount(QUEST_KILL_GROUP3);
	m_bKillCounts[3] = QuestV2CheckMonsterCount(QUEST_KILL_GROUP4);

	result	<< uint8(1)
			<< m_sEventDataIndex
			<< m_bKillCounts[0] << m_bKillCounts[1]
			<< m_bKillCounts[2] << m_bKillCounts[3];

	Send(&result);
}

void CUser::QuestV2ExecuteHelper(_QUEST_HELPER * pQuestHelper)
{
	if (pQuestHelper == NULL
		|| !CheckExistEvent(pQuestHelper->sEventDataIndex, 2))
		return;

	QuestV2RunEvent(pQuestHelper, pQuestHelper->nEventTriggerIndex); // NOTE: Fulfill will use nEventCompleteIndex
}

void CUser::QuestV2CheckFulfill(_QUEST_HELPER * pQuestHelper)
{
	if (pQuestHelper == NULL
		|| !CheckExistEvent(pQuestHelper->sEventDataIndex, 1))
		return;

	QuestV2RunEvent(pQuestHelper, pQuestHelper->nEventCompleteIndex);
}

void CUser::QuestV2RunEvent(_QUEST_HELPER * pQuestHelper, uint32 nEventID)
{
	// TO-DO: Run helper's Lua script.
}

/* 
	These are called by quest scripts. 
*/

void CUser::QuestV2SaveEvent(uint16 sQuestID)
{
	_QUEST_HELPER * pQuestHelper = g_pMain.m_QuestHelperArray.GetData(sQuestID);
	if (pQuestHelper == NULL)
		return;

	SaveEvent(pQuestHelper->sEventDataIndex, pQuestHelper->bEventStatus);
}

void CUser::QuestV2SendNpcMsg(uint32 nQuestID, uint16 sNpcID)
{
	Packet result(WIZ_QUEST, uint8(7));
	result << nQuestID << sNpcID;
	Send(&result);
}

void CUser::QuestV2ShowGiveItem(uint32 nUnk1, uint16 sUnk1, 
								uint32 nUnk2, uint16 sUnk2,
								uint32 nUnk3, uint16 sUnk3,
								uint32 nUnk4, uint16 sUnk4,
								uint32 nUnk5 /*= 0*/, uint16 sUnk5 /*= 0*/)
{
	Packet result(WIZ_QUEST, uint8(10));
	result	<< nUnk1 << sUnk1
			<< nUnk2 << sUnk2
			<< nUnk3 << sUnk3
			<< nUnk4 << sUnk4
			<< nUnk5 << sUnk5;
	Send(&result);
}

void CUser::QuestV2ShowMap(uint32 nQuestHelperID)
{
	Packet result(WIZ_QUEST, uint8(11));
	result << nQuestHelperID;
	Send(&result);
}

uint8 CUser::CheckMonsterCount(uint8 bGroup)
{
	_QUEST_MONSTER * pQuestMonster = g_pMain.m_QuestMonsterArray.GetData(m_sEventDataIndex);
	if (pQuestMonster == NULL
		|| bGroup == 0
		|| bGroup > QUEST_MOB_GROUPS)
		return 0;

	return m_bKillCounts[bGroup];
}
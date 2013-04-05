#pragma once

#include "../shared/Mutex.h"
#include <set>
#include <hash_map>

typedef std::set<uint16> ClanIDSet;
typedef stdext::hash_map<std::string, std::string> KingCandidacyNoticeBoardMap; 

enum ElectionType
{
	ELECTION_TYPE_NO_TERM		= 0,
	ELECTION_TYPE_NOMINATION	= 1,
	ELECTION_TYPE_PRE_ELECTION	= 2, // not sure if this is accurately named
	ELECTION_TYPE_ELECTION		= 3,

	ELECTION_TYPE_TERM_STARTED	= 6, // these two probably aren't accurately named at all
	ELECTION_TYPE_TERM_ENDED	= 7,

};

enum ImpeachmentType
{
};

class CUser;
class CKingSystem
{
public:
	CKingSystem();

	// Handles timed events.
	void CheckKingTimer();

	// Checks to see if a special (coin/XP) event should end.
	void CheckSpecialEvent();

	void LoadRecommendList();

	// Generates a list of the top 10 clan leaders eligible to nominate a King.
	void KingNotifyMessage(uint32 nResourceID, int byNation, ChatType byChatType);

	// Wrapper to lookup the appropriate King system instance
	static void PacketProcess(CUser * pUser, Packet & pkt);

	// The real method to handle packets from the client.
	void KingPacketProcess(CUser * pUser, Packet & pkt);

	/* Election system */
	void ElectionSystem(CUser * pUser, Packet & pkt);
	void ElectionScheduleConfirmation(CUser * pUser, Packet & pkt);
	void CandidacyRecommend(CUser * pUser, Packet & pkt);
	void CandidacyNoticeBoard(CUser * pUser, Packet & pkt);
	void ElectionPoll(CUser * pUser, Packet & pkt);
	void CandidacyResign(CUser * pUser, Packet & pkt);

	/* Impeachment system */
	void ImpeachmentSystem(CUser * pUser, Packet & pkt);
	void ImpeachmentRequest(CUser * pUser, Packet & pkt);
	void ImpeachmentRequestElect(CUser * pUser, Packet & pkt);
	void ImpeachmentList(CUser * pUser, Packet & pkt);
	void ImpeachmentElect(CUser * pUser, Packet & pkt);
	void ImpeachmentRequestUiOpen(CUser * pUser, Packet & pkt);
	void ImpeachmentElectionUiOpen(CUser * pUser, Packet & pkt);

	void KingTaxSystem(CUser * pUser, Packet & pkt);
	void KingSpecialEvent(CUser * pUser, Packet & pkt);

	static void HandleDatabaseRequest(CUser * pUser, Packet & pkt);

	uint8	m_byNation;

	uint8	m_byType;
	uint16	m_sYear;
	uint8	m_byMonth;
	uint8	m_byDay;
	uint8	m_byHour;
	uint8	m_byMinute;

	uint8	m_byImType;
	uint16	m_sImYear;
	uint8	m_byImMonth;
	uint8	m_byImDay;
	uint8	m_byImHour;
	uint8	m_byImMinute;

	uint8	m_byNoahEvent;
	uint8	m_byNoahEvent_Day;
	uint8	m_byNoahEvent_Hour;
	uint8	m_byNoahEvent_Minute;
	uint16	m_sNoahEvent_Duration;

	uint8	m_byExpEvent;
	uint8	m_byExpEvent_Day;
	uint8	m_byExpEvent_Hour;
	uint8	m_byExpEvent_Minute;
	uint16	m_sExpEvent_Duration;

	uint32	m_nTribute;
	uint8	m_byTerritoryTariff;
	uint32	m_nTerritoryTax;
	uint32	m_nNationalTreasury;

	std::string m_strKingName;
	std::string m_strImRequestID;

	FastMutex m_lock;
	ClanIDSet m_top10ClanSet;
	KingCandidacyNoticeBoardMap m_noticeBoardMap;

	// TO-DO: Give this a more appropriate name.
	bool m_bSentFirstMessage;
};
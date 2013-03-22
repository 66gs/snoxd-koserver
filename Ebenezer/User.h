#pragma once

#include "MagicProcess.h"
#include "EVENT.h"
#include "EVENT_DATA.h"
#include "LOGIC_ELSE.h"
#include "EXEC.h"     

#include <list>
#include <vector>

#include "ChatHandler.h"

typedef	 std::list<_EXCHANGE_ITEM*>		ItemList;
typedef	 std::map<uint32, time_t>		SkillCooldownList;
typedef std::map<uint32, time_t>		UserSavedMagicMap;

#define BANISH_DELAY_TIME    30

enum GameState
{
	GAME_STATE_CONNECTED,
	GAME_STATE_INGAME
};

enum InvisibilityType
{
	INVIS_NONE				= 0,
	INVIS_DISPEL_ON_MOVE	= 1,
	INVIS_NORMAL			= 2
};

enum MerchantState
{
	MERCHANT_STATE_NONE		= -1,
	MERCHANT_STATE_SELLING	= 0,
	MERCHANT_STATE_BUYING	= 1
};

class CEbenezerDlg;
class CUser : public Unit, public KOSocket
{
public:
	virtual uint16 GetID() { return GetSocketID(); }

	const char * GetAccountName() { return m_strAccountID.c_str(); }
	virtual const char * GetName() { return m_strUserID.c_str(); }

	std::string	m_strAccountID, m_strUserID;

	uint8	m_bRace;
	uint16	m_sClass;

	uint32	m_nHair;

	uint8	m_bRank;
	uint8	m_bTitle;
	int64	m_iExp;	
	int32	m_iLoyalty, m_iLoyaltyMonthly;
	int32	m_iMannerPoint;
	uint8	m_bFace;
	uint8	m_bCity;
	int16	m_bKnights;	
	uint8	m_bFame;
	int16	m_sHp, m_sMp, m_sSp;
	uint8	m_bStats[STAT_COUNT];
	uint8	m_bAuthority;
	int16	m_sPoints; // this is just to shut the compiler up
	uint32	m_iGold, m_iBank;
	int16	m_sBind;
	
	uint8    m_bstrSkill[10];	
	_ITEM_DATA m_sItemArray[INVENTORY_TOTAL];
	_ITEM_DATA m_sWarehouseArray[WAREHOUSE_MAX];

	uint8	m_bLogout;
	DWORD	m_dwTime;

	uint8	m_bAccountStatus;
	uint8	m_bPremiumType;
	uint16	m_sPremiumTime;

	bool	m_bSelectedCharacter;
	bool	m_bStoreOpen;

	int8	m_bMerchantState;
	int16	m_sMerchantsSocketID;
	std::list<uint16>	m_arMerchantLookers;
	_MERCH_DATA	m_arMerchantItems[MAX_MERCH_ITEMS]; //What is this person selling? Stored in "_MERCH_DATA" structure.
	bool	m_bPremiumMerchant;

	uint8	m_bRequestingChallenge, // opcode of challenge request being sent by challenger
			m_bChallengeRequested;  // opcode of challenge request received by challengee
	int16	m_sChallengeUser;

	//Magic System Cooldown checks

	SkillCooldownList	m_CoolDownList;

	bool	m_bIsTransformed; // Is the character in a transformed state?
	bool	m_bIsChicken; // Is the character taking the beginner/chicken quest?
	bool	m_bIsHidingHelmet;

	int8	m_bPersonalRank;
	int8	m_bKnightsRank;

	int16	m_sDirection;

	int64	m_iMaxExp;
	uint32	m_sMaxWeight;
	uint16   m_sSpeed;	// NOTE: Currently unused

	int16	m_sItemMaxHp;
	int16	m_sItemMaxMp;
	uint32	m_sItemWeight;
	short	m_sItemHit;
	short	m_sItemAc;
	short	m_sItemHitrate;
	short	m_sItemEvasionrate;

	int16	m_sStatItemBonuses[STAT_COUNT];
	int8	m_bStatBuffs[STAT_COUNT];

	short	m_iMaxHp, m_iMaxMp;
	
	BYTE	m_bResHpType;
	BYTE	m_bWarp;
	BYTE	m_bNeedParty;

	short	m_sPartyIndex;
	bool	m_bPartyLeader;

	bool	m_bCanSeeStealth;
	uint8	m_bInvisibilityType;

	short	m_sExchangeUser;
	BYTE	m_bExchangeOK;

	ItemList	m_ExchangeItemList;
	_ITEM_DATA	m_MirrorItem[HAVE_MAX];

	bool	m_bBlockPrivateChat;
	short	m_sPrivateChatUser;

	time_t	m_tHPLastTimeNormal;					// For Automatic HP recovery. 
	time_t	m_tHPStartTimeNormal;
	short	m_bHPAmountNormal;
	uint8	m_bHPDurationNormal;
	uint8	m_bHPIntervalNormal;

	time_t	m_tAreaLastTime;			// For Area Damage spells Type 3.
	time_t	m_tAreaStartTime;
	BYTE    m_bAreaInterval;
	int     m_iAreaMagicID;

	uint32	m_nTransformationItem; // item used for transforming (e.g. disguise scroll, totem..)
	time_t	m_tTransformationStartTime;
	uint16	m_sTransformationDuration;

	CMagicProcess m_MagicProcess;

	uint32	m_fSpeedHackClientTime, m_fSpeedHackServerTime;
	BYTE	m_bSpeedHackCheck;

	time_t	m_tBlinkExpiryTime;			// When you should stop blinking.

	short	m_sAliveCount;

	DWORD	m_bAbnormalType;			// Is the player normal,a giant, or a dwarf?

	short	m_sWhoKilledMe;				// Who killed me???
	__int64		m_iLostExp;					// Experience point that was lost when you died.

	time_t	m_tLastTrapAreaTime;		// The last moment you were in the trap area.

	BOOL	m_bZoneChangeFlag;

	BYTE	m_bRegeneType;				// Did you die and go home or did you type '/town'?

	time_t	m_tLastRegeneTime;			// The last moment you got resurrected.

	BOOL	m_bZoneChangeSameZone;		// Did the server change when you warped?

	int					m_iSelMsgEvent[MAX_MESSAGE_EVENT];
	short				m_sEventNid;


public:
	__forceinline bool isBanned() { return getAuthority() == AUTHORITY_BANNED; }
	__forceinline bool isMuted() { return getAuthority() == AUTHORITY_MUTED; }
	__forceinline bool isAttackDisabled() { return getAuthority() == AUTHORITY_ATTACK_DISABLED; }
	__forceinline bool isGM() { return getAuthority() == AUTHORITY_GAME_MASTER; }
	__forceinline bool isLimitedGM() { return getAuthority() == AUTHORITY_LIMITED_GAME_MASTER; }

	virtual bool isDead() { return m_bResHpType == USER_DEAD || m_sHp <= 0; }
	__forceinline bool isBlinking() { return m_bAbnormalType == ABNORMAL_BLINKING; }

	__forceinline bool isInGame() { return GetState() == GAME_STATE_INGAME; }
	__forceinline bool isInParty() { return m_sPartyIndex != -1; }
	__forceinline bool isInClan() { return GetClanID() > 0; }
	__forceinline bool isClanLeader() { return getFame() == CHIEF; }
	__forceinline bool isClanAssistant() { return getFame() == VICECHIEF; }

	__forceinline bool isWarrior() { return JobGroupCheck(1) == TRUE; }
	__forceinline bool isRogue() { return JobGroupCheck(2) == TRUE; }
	__forceinline bool isMage() { return JobGroupCheck(3) == TRUE; }
	__forceinline bool isPriest() { return JobGroupCheck(4) == TRUE; }

	__forceinline bool isTrading() { return m_sExchangeUser != -1; }
	__forceinline bool isStoreOpen() { return m_bStoreOpen; }
	__forceinline bool isMerchanting() { return GetMerchantState() != MERCHANT_STATE_NONE; }
	__forceinline bool isSellingMerchant() { return GetMerchantState() == MERCHANT_STATE_SELLING; }
	__forceinline bool isBuyingMerchant() { return GetMerchantState() == MERCHANT_STATE_BUYING; }
	__forceinline bool isTransformed() { return m_bIsTransformed; }

	__forceinline bool isBlockingPrivateChat() { return m_bBlockPrivateChat; }

	__forceinline int8 GetMerchantState() { return m_bMerchantState; }

	__forceinline BYTE getAuthority() { return m_bAuthority; }
	__forceinline BYTE getFame() { return m_bFame; }

	__forceinline int16 GetClanID() { return m_bKnights; }
	__forceinline void SetClanID(int16 val) { m_bKnights = val; }

	__forceinline GameState GetState() { return m_state; }

	__forceinline uint8 getStat(StatType type)
	{
		ASSERT(type < STAT_COUNT);
		return m_bStats[type];
	}

	__forceinline void setStat(StatType type, uint8 val)
	{
		ASSERT(type < STAT_COUNT);
		m_bStats[type] = val;
	}

	__forceinline int32 getStatTotal() // NOTE: Shares name with another, but lack-of args should be self-explanatory
	{
		int32 total = 0; // NOTE: this loop should be unrolled by the compiler
		foreach_array (i, m_bStats)
			total += m_bStats[i];
		return total;
	}

	__forceinline int16 getStatItemBonus(StatType type)
	{
		ASSERT(type < STAT_COUNT);
		return m_sStatItemBonuses[type];
	}

	__forceinline int16 getStatWithItemBonus(StatType type)
	{
		return getStat(type) + getStatItemBonus(type);
	}

	__forceinline int32 getStatItemBonusTotal()
	{
		int32 total = 0; // NOTE: this loop should be unrolled by the compiler
		foreach_array (i, m_sStatItemBonuses)
			total += m_sStatItemBonuses[i];
		return total;
	}

	__forceinline uint16 getStatBonusTotal(StatType type)
	{
		return getStatBuff(type) + getStatItemBonus(type);
	}

	__forceinline uint8 getStatBuff(StatType type)
	{
		ASSERT(type < STAT_COUNT);
		return m_bStatBuffs[type];
	}

	__forceinline void setStatBuff(StatType type, uint8 val)
	{
		ASSERT(type < STAT_COUNT);
		m_bStatBuffs[type] = val;
	}

	__forceinline uint32 getStatBuffTotal()
	{
		uint32 total = 0; // NOTE: this loop should be unrolled by the compiler
		foreach_array (i, m_bStatBuffs)
			total += m_bStatBuffs[i];
		return total;
	}

	__forceinline uint16 getStatTotal(StatType type)
	{
		return getStat(type) + getStatItemBonus(type) + getStatBuff(type);
	}

	__forceinline _ITEM_DATA * GetItem(uint8 pos) { return &m_sItemArray[pos]; }
	_ITEM_TABLE* GetItemPrototype(uint8 pos);

	__forceinline C3DMap * GetMap() { return m_pMap; }

	CUser(uint16 socketID, SocketMgr *mgr); 

	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual bool HandlePacket(Packet & pkt);

	void SendLoyaltyChange(int32 nChangeAmount = 0);

	BOOL CheckItemCount(int itemid, short min, short max);
	BOOL CheckRandom(short percent);
	void NativeZoneReturn();
	void KickOutZoneUser(BOOL home = FALSE, int nZoneID = 21);
	void TrapProcess();
	BOOL JobGroupCheck(short jobgroupid);
	void SelectMsg(EXEC* pExec);
	void SendNpcSay(EXEC* pExec);
	BOOL CheckClass(short class1, short class2 = -1, short class3 = -1, short class4 = -1, short class5 = -1, short class6 = -1);
	BOOL GiveItem(int itemid, short count, bool send_packet = true);
	BOOL RobItem(int itemid, short count);
	BOOL CheckExistItem(int itemid, short count);
	BOOL CheckExistItemAnd(int32 nItemID1, int16 sCount1, int32 nItemID2, int16 sCount2,
		int32 nItemID3, int16 sCount3, int32 nItemID4, int16 sCount4, int32 nItemID5, int16 sCount5);
	BOOL CheckWeight(int itemid, short count);
	BOOL CheckSkillPoint(BYTE skillnum, BYTE min, BYTE max);
	BOOL GoldLose(unsigned int gold);
	void GoldGain(int gold);
	void SendItemWeight();
	void UpdateVisibility(InvisibilityType bNewType);
	void BlinkStart();
	void BlinkTimeCheck();
	void GoldChange(short tid, int gold);
	CUser* GetItemRoutingUser(int itemid, short itemcount);
	bool GetStartPosition(short & x, short & y, BYTE bZone = 0);
	int FindSlotForItem(uint32 nItemID, uint16 sCount);
	int GetEmptySlot();
	void Type3AreaDuration();
	void SendAllKnightsID();
	void SendStackChange(uint32 nItemID, uint32 nCount /* needs to be 4 bytes, not a bug */, uint16 sDurability, uint8 bPos, bool bNewItem = false);
	void Type4Duration();
	void HPTimeChange();
	void HPTimeChangeType3();
	void ItemDurationChange(uint8 slot, uint16 maxValue, int16 curValue, uint16 amount);
	void SendDurability(uint8 slot, uint16 durability);
	void SendItemMove(uint8 subcommand);
	void ItemWoreOut( int type, int damage );
	void Dead();
	void LoyaltyDivide( short tid );
	void GetUserInfoForAI(Packet & result);
	BOOL ItemEquipAvailable( _ITEM_TABLE* pTable );
	virtual void HpChange(int amount, Unit *pAttacker = NULL, bool bSendToAI = true);
	virtual void MSpChange(int amount);
	void SendPartyHPUpdate();
	void SendAnvilRequest(int nid);

	// packet handlers start here
	void VersionCheck(Packet & pkt);
	void LoginProcess(Packet & pkt);
	void SelNationToAgent(Packet & pkt);
	void AllCharInfoToAgent();
	void ChangeHair(Packet & pkt);
	void NewCharToAgent(Packet & pkt);
	void DelCharToAgent(Packet & pkt);
	void SelCharToAgent(Packet & pkt);
	void SelectCharacter(Packet & pkt); // from the database
	void SetLogInInfoToDB(BYTE bInit);
	void RecvLoginInfo(Packet & pkt); // from the database

	void SpeedHackTime(Packet & pkt);

	void GameStart(Packet & pkt);
	void RentalSystem(Packet & pkt);
	void SkillDataProcess(Packet & pkt);
	void SkillDataSave(Packet & pkt);
	void SkillDataLoad();
	void MoveProcess(Packet & pkt);
	void Rotate(Packet & pkt);
	void Attack(Packet & pkt);

	static void InitChatCommands();
	static void CleanupChatCommands();

	void Chat(Packet & pkt);
	void ChatTargetSelect(Packet & pkt);

	bool ProcessChatCommand(std::string & message);

	COMMAND_HANDLER(HandleGiveItemCommand);
	COMMAND_HANDLER(HandleZoneChangeCommand);

	void RecvRegene(Packet & pkt);
	void Regene(uint8 regene_type, uint32 magicid = 0);
	void RequestUserIn(Packet & pkt);
	void RequestNpcIn(Packet & pkt);
	void RecvWarp(Packet & pkt);
	void Warp(uint16 sPosX, uint16 sPosZ);
	void ItemMove(Packet & pkt);
	void NpcEvent(Packet & pkt);

	void ItemTrade(Packet & pkt);

	void BundleOpenReq(Packet & pkt);
	void ItemGet(Packet & pkt);

	void RecvZoneChange(Packet & pkt);
	void PointChange(Packet & pkt);

	void StateChange(Packet & pkt);
	void StateChangeServerDirect(BYTE bType, uint32 nBuff);

	void PartyProcess(Packet & pkt);
	void PartyDelete();
	void PartyRemove( int memberid );
	void PartyInsert();
	void PartyCancel();
	void PartyRequest( int memberid, BOOL bCreate );

	// Trade system
	void ExchangeProcess(Packet & pkt);
	void ExchangeReq(Packet & pkt);
	void ExchangeAgree(Packet & pkt);
	void ExchangeAdd(Packet & pkt);
	void ExchangeDecide();
	void ExchangeCancel();

	void InitExchange(BOOL bStart);
	BOOL ExecuteExchange();
	int ExchangeDone();

	// Merchant system (both types)
	void MerchantProcess(Packet & pkt);
	void GiveMerchantItems();

	// regular merchants
	void MerchantOpen();
	void MerchantClose();
	void MerchantItemAdd(Packet & pkt);
	void MerchantItemCancel(Packet & pkt);
	void MerchantItemList(Packet & pkt);
	void MerchantItemBuy(Packet & pkt);
	void MerchantInsert(Packet & pkt);
	void CancelMerchant();

	// buying merchants
	void BuyingMerchantOpen(Packet & pkt);
	void BuyingMerchantClose();
	void BuyingMerchantInsert(Packet & pkt);
	void BuyingMerchantInsertRegion();
	void BuyingMerchantList(Packet & pkt);
	void BuyingMerchantBuy(Packet & pkt);

	void RemoveFromMerchantLookers();

	void SkillPointChange(Packet & pkt);

	void ObjectEvent(Packet & pkt);
	BOOL BindObjectEvent(_OBJECT_EVENT *pEvent);
	BOOL GateLeverObjectEvent(_OBJECT_EVENT *pEvent, int nid);
	BOOL FlagObjectEvent(_OBJECT_EVENT *pEvent, int nid);
	BOOL WarpListObjectEvent(_OBJECT_EVENT *pEvent);

	void UpdateGameWeather(Packet & pkt);

	void ClassChange(Packet & pkt);
	void ClassChangeReq();
	void AllPointChange();
	void AllSkillPointChange();

	void CountConcurrentUser();
	void UserDataSaveToAgent();

	void ItemRepair(Packet & pkt);
	void ItemRemove(Packet & pkt);
	void OperatorCommand(Packet & pkt);
	void WarehouseProcess(Packet & pkt);
	void Home();

	void FriendProcess(Packet & pkt);
	void FriendRequest();
	void FriendModify(Packet & pkt, uint8 opcode);
	void RecvFriendModify(Packet & pkt, uint8 opcode);
	void FriendReport(Packet & pkt);
	BYTE GetFriendStatus(std::string & charName, int16 & sid);

	void SelectWarpList(Packet & pkt);
	BOOL GetWarpList( int warp_group );

	void ServerChangeOk(Packet & pkt);

	void PartyBBS(Packet & pkt);
	void PartyBBSRegister(Packet & pkt);
	void PartyBBSDelete(Packet & pkt);
	void PartyBBSNeeded(Packet & pkt, BYTE type);
	void PartyBBSWanted(Packet & pkt);
	uint8 GetPartyMemberAmount();

	void SendPartyBBSNeeded(uint16 page_index, uint8 bType);

	void ClientEvent(uint16 sNpcID);
	int32 GetEventIDByNPC(CNpc *pNpc);

	BOOL CheckEventLogic(EVENT_DATA* pEventData);
	BOOL RunNpcEvent(CNpc* pNpc, EXEC* pExec);
	BOOL RunEvent(EVENT_DATA *pEventData);

	void RecvSelectMsg(Packet & pkt);

	// from the client
	void ItemUpgradeProcess(Packet & pkt);
	void ItemUpgrade(Packet & pkt);
	void ItemUpgradeAccessories(Packet & pkt);
	void BifrostPieceProcess(Packet & pkt); // originally named BeefRoastPieceProcess() -- that's not happening.
	void ItemUpgradeRebirth(Packet & pkt);
	void ItemSealProcess(Packet & pkt);
	void CharacterSealProcess(Packet & pkt);

	void ShoppingMall(Packet & pkt);
	void RecvStore(Packet & pkt); // from the database
	void HandleStoreClose();
	void RecvStoreClose(Packet & pkt); // from the database
	void LetterSystem(Packet & pkt);

	void ReqLetterSystem(Packet & pkt);
	void ReqLetterUnread();
	void ReqLetterList(bool bNewLettersOnly = true);
	void ReqLetterRead(Packet & pkt);
	void ReqLetterSend(Packet & pkt);
	void ReqLetterGetItem(Packet & pkt);
	void ReqLetterDelete(Packet & pkt);

	void HandleHelmet(Packet & pkt);
	void HandleCapeChange(Packet & pkt);

	void HandleChallenge(Packet & pkt);
	void HandleChallengeRequestPVP(Packet & pkt);
	void HandleChallengeRequestCVC(Packet & pkt);
	void HandleChallengeAcceptPVP(Packet & pkt);
	void HandleChallengeAcceptCVC(Packet & pkt);
	void HandleChallengeCancelled(uint8 opcode);
	void HandleChallengeRejected(uint8 opcode);

	void SendNotice();
	void UserLookChange( int pos, int itemid, int durability );
	void SpeedHackUser();
	void LoyaltyChange(short tid);
	void ChangeNP(short sAmount, bool bDistributeToParty = true);
	void ZoneChange( int zone, float x, float z );
	void SendTargetHP( BYTE echo, int tid, int damage = 0 );
	BOOL IsValidSlotPos( _ITEM_TABLE* pTable, int destpos );
	void SetUserAbility(bool bSendPacket = true);
	void LevelChange(short level, BYTE type=TRUE);	// type : TRUE => level up, FALSE => level down
	void SetSlotItemValue();
	void SendTimeStatus(); // TO-DO: Deprecate
	void SendTime();
	void SendWeather();
	void SendPremiumInfo();
	void SetZoneAbilityChange();
	void SetMaxMp();
	void SetMaxHp(int iFlag=0);
	void ExpChange(__int64 iExp);
	void LogOut();
	void SendMyInfo();
	void SendServerChange(char *ip, uint8 bInit);
	void Send2AI_UserUpdateInfo(bool initialInfo = false);

	virtual void GetInOut(Packet & result, uint8 bType);
	void UserInOut(uint8 bType);

	void GetUserInfo(Packet & pkt);
	void SendUserStatusUpdate(UserStatus type, UserStatusBehaviour status);
	virtual void Initialize();
	
	void ChangeFame(uint8 bFame);
	void SendServerIndex();

	void SendToRegion(Packet *pkt, CUser *pExceptUser = NULL);

	virtual void OnDeath(Unit *pKiller);

	// Exchange system
	bool CheckExchange(int nExchangeID);
	bool RunExchange(int nExchangeID);

	// Clan system
	void SendClanUserStatusUpdate(bool bToRegion = true);

	void SendPartyStatusUpdate(uint8 bStatus, uint8 bResult = 0);

	//Magic System - rewrite
	bool CanUseItem(long itemid, uint16 count); //Should place this with other item related functions

	void CheckSavedMagic();
	virtual void InsertSavedMagic(uint32 nSkillID, uint16 sDuration);
	virtual bool HasSavedMagic(uint32 nSkillID);

	void SaveEvent(uint16 sQuestID, uint8 bQuestState);
	void DeleteEvent(uint16 sQuestID);
	bool CheckExistEvent(uint16 sQuestID, uint8 bQuestState);

	void QuestV2MonsterCountAdd(uint16 sNpcID);
	uint8 QuestV2CheckMonsterCount(uint16 sQuestID);
	void QuestV2MonsterDataDeleteAll();

	// Sends the quest completion statuses
	void QuestDataRequest();

	// Handles new quest packets
	void QuestV2PacketProcess(Packet & pkt);
	void QuestV2MonsterDataRequest();
	void QuestV2ExecuteHelper(_QUEST_HELPER * pQuestHelper);
	void QuestV2CheckFulfill(_QUEST_HELPER * pQuestHelper);
	void QuestV2RunEvent(_QUEST_HELPER * pQuestHelper, uint32 nEventID);

	void QuestV2SaveEvent(uint16 sQuestID);
	void QuestV2SendNpcMsg(uint32 nQuestID, uint16 sNpcID);
	void QuestV2ShowGiveItem(uint32 nUnk1, uint16 sUnk1, 
								uint32 nUnk2, uint16 sUnk2,
								uint32 nUnk3, uint16 sUnk3,
								uint32 nUnk4, uint16 sUnk4,
								uint32 nUnk5 = 0, uint16 sUnk5 = 0);
	void QuestV2ShowMap(uint32 nQuestHelperID);
	uint8 CheckMonsterCount(uint8 bGroup);

	//Zone checks
	bool isAttackZone();

	void ResetWindows();

	void CloseProcess();
	virtual ~CUser() {}

	/* Database requests */
	void ReqAccountLogIn(Packet & pkt);
	void ReqSelectNation(Packet & pkt);
	void ReqAllCharInfo(Packet & pkt);
	void ReqChangeHair(Packet & pkt);
	void ReqCreateNewChar(Packet & pkt);
	void ReqDeleteChar(Packet & pkt);
	void ReqSelectCharacter(Packet & pkt);
	void ReqSaveCharacter();
	void ReqUserLogOut();
	void ReqRegisterClanSymbol(Packet & pkt);
	void ReqSetLogInInfo(Packet & pkt);
	void ReqUserKickOut(Packet & pkt);
	// void BattleEventResult(Packet & pkt);
	void ReqShoppingMall(Packet & pkt);
	void ReqLoadWebItemMall(Packet & pkt);
	void ReqSkillDataProcess(Packet & pkt);
	void ReqSkillDataSave(Packet & pkt);
	void ReqSkillDataLoad(Packet & pkt);
	void ReqFriendProcess(Packet & pkt);
	void ReqRequestFriendList(Packet & pkt);
	void ReqAddFriend(Packet & pkt);
	void ReqRemoveFriend(Packet & pkt);
	void ReqChangeCape(Packet & pkt);

//private:
	static ChatCommandTable s_commandTable;
	GameState m_state;

	// quest ID | quest state (need to replace with enum)
	typedef std::map<uint16, uint8> QuestMap;
	QuestMap m_questMap;

	uint8 m_bKillCounts[QUEST_MOB_GROUPS];
	uint16 m_sEventDataIndex;

	UserSavedMagicMap m_savedMagicMap;
	FastMutex m_savedMagicLock;
};

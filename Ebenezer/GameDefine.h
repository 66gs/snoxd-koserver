#ifndef _GAMEDEFINE_H
#define _GAMEDEFINE_H

//////////////////// ��� Define ////////////////////
#define KARUWARRRIOR		101		// ī�����
#define KARUROGUE			102		// ī��α�
#define KARUWIZARD			103		// ī�縶��
#define KARUPRIEST			104		// ī����
#define BERSERKER			105		// ��Ŀ
#define GUARDIAN			106		// �����
#define HUNTER				107		// ����
#define PENETRATOR			108		// ���Ʈ������
#define SORSERER			109		// �Ҽ���
#define NECROMANCER			110		// ��ũ�θǼ�
#define SHAMAN				111		// ����
#define DARKPRIEST			112		// ��ũ�����Ʈ

#define ELMORWARRRIOR		201		// �������
#define ELMOROGUE			202		// ����α�
#define ELMOWIZARD			203		// ���𸶹�
#define ELMOPRIEST			204		// ������
#define BLADE				205		// ���̵�
#define PROTECTOR			206		// �������
#define RANGER				207		// �����
#define ASSASSIN			208		// ��ؽ�
#define MAGE				209		// ������
#define ENCHANTER			210		// ��þ��
#define CLERIC				211		// Ŭ����
#define DRUID				212		// ����̵�
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Race Define
/////////////////////////////////////////////////////
#define KARUS_BIG			1
#define KARUS_MIDDLE		2
#define KARUS_SMALL			3
#define KARUS_WOMAN			4
#define BABARIAN			11
#define ELMORAD_MAN			12
#define ELMORAD_WOMAN		13

// Ÿ�ݺ� ����� //
#define GREAT_SUCCESS			0X01		// �뼺��
#define SUCCESS					0X02		// ����
#define NORMAL					0X03		// ����
#define	FAIL					0X04		// ���� 

// Item Move Direction Define 
#define ITEM_INVEN_SLOT			0x01
#define ITEM_SLOT_INVEN			0x02
#define ITEM_INVEN_INVEN		0x03
#define ITEM_SLOT_SLOT			0x04
#define ITEM_INVEN_ZONE			0x05
#define ITEM_ZONE_INVEN			0x06

// Item Weapon Type Define
#define WEAPON_DAGGER			1
#define WEAPON_SWORD			2
#define WEAPON_AXE				3
#define WEAPON_MACE				4
#define WEAPON_SPEAR			5
#define WEAPON_SHIELD			6
#define WEAPON_BOW				7
#define WEAPON_LONGBOW			8
#define WEAPON_LAUNCHER			10
#define WEAPON_STAFF			11
#define WEAPON_ARROW			12	// ��ų ���
#define WEAPON_JAVELIN			13	// ��ų ���
#define WEAPON_WORRIOR_AC		21	// ��ų ���
#define WEAPON_LOG_AC			22	// ��ų ���
#define WEAPON_WIZARD_AC		23	// ��ų ���
#define WEAPON_PRIEST_AC		24	// ��ų ���
////////////////////////////////////////////////////////////
// User Status //
#define USER_STANDING			0X01		// �� �ִ�.
#define USER_SITDOWN			0X02		// �ɾ� �ִ�.
#define USER_DEAD				0x03		// ��Ŷ�
//#define USER_BLINKING			0x04		// ��� ��Ƴ���!!!
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// Magic State
#define NONE				0x01
#define CASTING				0x02
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// Durability Type
#define ATTACK				0x01
#define DEFENCE				0x02
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// Knights Authority Type
/*
#define CHIEF				0x06
#define VICECHIEF			0x05*/
#define OFFICER				0x04
#define KNIGHT				0x03
//#define TRAINEE				0x02
#define PUNISH				0x01	

#define CHIEF				0x01	// ����
#define VICECHIEF			0x02	// �δ���
#define TRAINEE				0x05	// ���
#define COMMAND_CAPTAIN		100		// ���ֱ���
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// COMMUNITY TYPE DEFINE
#define CLAN_TYPE			0x01
#define KNIGHTS_TYPE		0x02
////////////////////////////////////////////////////////////

#define MAX_CLAN			36
#define MAX_KNIGHTS_BANK	200
#define MAX_KNIGHTS_MARK	2400

#define ITEM_GOLD			900000000	// �� ������ ��ȣ...
#define ITEM_NO_TRADE		900000001	// �ŷ� �Ұ� �����۵�.... �񷯸ӱ� ũ�������� �̹�Ʈ >.<		

////////////////////////////////////////////////////////////
// EVENT TYPE DEFINE
#define ZONE_CHANGE			0x01
#define ZONE_TRAP_DEAD		0x02
#define ZONE_TRAP_AREA		0x03

////////////////////////////////////////////////////////////
// EVENT MISCELLANOUS DATA DEFINE
#define ZONE_TRAP_INTERVAL	   1		// Interval is one second right now.
#define ZONE_TRAP_DAMAGE	   10		// HP Damage is 10 for now :)

////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// USER POINT DEFINE
#define STR					0x01
#define STA					0x02
#define DEX					0x03
#define INTEL				0x04
#define CHA					0x05

#define ORDER_SKILL			0x01
#define MANNER_SKILL		0X02
#define LANGUAGE_SKILL		0x03
#define BATTLE_SKILL		0x04
#define PRO_SKILL1			0x05
#define PRO_SKILL2			0x06
#define PRO_SKILL3			0x07
#define PRO_SKILL4			0x08

/////////////////////////////////////////////////////////////
// ITEM TYPE DEFINE
#define ITEM_TYPE_FIRE				0x01
#define ITEM_TYPE_COLD				0x02
#define ITEM_TYPE_LIGHTNING			0x03
#define ITEM_TYPE_POISON			0x04
#define ITEM_TYPE_HP_DRAIN			0x05
#define ITEM_TYPE_MP_DAMAGE			0x06
#define ITEM_TYPE_MP_DRAIN			0x07
#define ITEM_TYPE_MIRROR_DAMAGE		0x08

/////////////////////////////////////////////////////////////
// ITEM LOG TYPE 
#define ITEM_MERCHANT_BUY			0x01
#define ITEM_MERCHANT_SELL			0x02
#define ITEM_MONSTER_GET			0x03
#define ITEM_EXCHANGE_PUT			0x04
#define ITEM_EXCHANGE_GET			0x05
#define ITEM_DESTROY				0x06
#define ITEM_WAREHOUSE_PUT			0x07
#define ITEM_WAREHOUSE_GET			0x08
#define ITEM_UPGRADE				0x09

/////////////////////////////////////////////////////////////
// JOB GROUP TYPES
#define GROUP_WARRIOR				1
#define GROUP_ROGUE					2
#define GROUP_MAGE					3
#define GROUP_CLERIC				4
#define GROUP_ATTACK_WARRIOR		5
#define GROUP_DEFENSE_WARRIOR		6
#define GROUP_ARCHERER				7
#define GROUP_ASSASSIN				8
#define GROUP_ATTACK_MAGE			9
#define GROUP_PET_MAGE				10
#define GROUP_HEAL_CLERIC			11
#define GROUP_CURSE_CLERIC			12

//////////////////////////////////////////////////////////////
// USER ABNORMAL STATUS TYPES
#define ABNORMAL_NORMAL			0x01
#define ABNORMAL_GIANT			0x02
#define ABNORMAL_DWARF			0x03
#define ABNORMAL_BLINKING		0x04

//////////////////////////////////////////////////////////////
// Object Type
#define NORMAL_OBJECT		0
#define SPECIAL_OBJECT		1

//////////////////////////////////////////////////////////////
// REGENE TYPES
#define REGENE_NORMAL		0
#define REGENE_MAGIC		1
#define REGENE_ZONECHANGE	2

//////////////////////////////////////////////////////////////
// TYPE 3 ATTRIBUTE TYPES
#define ATTRIBUTE_FIRE				1
#define ATTRIBUTE_ICE				2
#define ATTRIBUTE_LIGHTNING			3

//extern CRITICAL_SECTION g_LogFile_critical;

struct _CLASS_COEFFICIENT
{
	int		sClassNum;
	float	ShortSword;
	float	Sword;
	float	Axe;
	float	Club;
	float	Spear;
	float	Pole;
	float	Staff;
	float	Bow;
	float	HP;
	float	MP;
	float	SP;
	float	AC;
	float	Hitrate;
	float	Evasionrate;
};

struct _ZONE_ITEM {		// Bundle unit
	DWORD bundle_index;
	int itemid[6];
	short count[6];
	float x;
	float z;
	float y;
	float time;
};

struct	_EXCHANGE_ITEM
{
	int itemid;
	int count;
	short duration;
	BYTE pos;
	__int64	nSerialNum;
};

struct _ITEM_TABLE
{
	long  m_iNum;
	BYTE  m_bKind;
	BYTE  m_bSlot;
	BYTE  m_bRace;
	BYTE  m_bClass;
	int   m_sDamage;
	int   m_sDelay;
	int   m_sRange;
	int   m_sWeight;
	int   m_sDuration;
	long  m_iBuyPrice;
	long  m_iSellPrice;
	int   m_sAc;
	BYTE  m_bCountable;
	long  m_iEffect1;
	long  m_iEffect2;
	BYTE  m_bReqLevel;
	BYTE  m_bReqRank;
	BYTE  m_bReqTitle;
	BYTE  m_bReqStr;
	BYTE  m_bReqSta;
	BYTE  m_bReqDex;
	BYTE  m_bReqIntel;
	BYTE  m_bReqCha;
	BYTE  m_bSellingGroup;
	BYTE  m_ItemType;
	int   m_sHitrate;
	int   m_sEvarate;
	int   m_sDaggerAc;
	int   m_sSwordAc;
	int   m_sMaceAc;
	int   m_sAxeAc;
	int   m_sSpearAc;
	int   m_sBowAc;
	BYTE  m_bFireDamage;
	BYTE  m_bIceDamage;
	BYTE  m_bLightningDamage;
	BYTE  m_bPoisonDamage;
	BYTE  m_bHPDrain;
	BYTE  m_bMPDamage;
	BYTE  m_bMPDrain;
	BYTE  m_bMirrorDamage;
	BYTE  m_bDroprate;
	int   m_bStrB;
	int   m_bStaB;
	int   m_bDexB;
	int   m_bIntelB;
	int   m_bChaB;
	int   m_MaxHpB;
	int   m_MaxMpB;
	int   m_bFireR;
	int   m_bColdR;
	int   m_bLightningR;
	int   m_bMagicR;
	int   m_bPoisonR;
	int   m_bCurseR;
};

struct	_PARTY_GROUP
{
	WORD wIndex;
	short uid[8];
	short sMaxHp[8];
	short sHp[8];
	BYTE bLevel[8];
	short sClass[8];
	BYTE bItemRouting;
	_PARTY_GROUP() {
		for(int i=0;i<8;i++) {
			uid[i] = -1; sMaxHp[i] = 0; sHp[i] = 0; bLevel[i] = 0; sClass[i] = 0;
		}
		bItemRouting = 0;
	};
};

struct _OBJECT_EVENT
{
	BYTE byLife;
	int sBelong;
	short sIndex;
	short sType;
	short sControlNpcID;
	short sStatus;
	float fPosX;
	float fPosY;
	float fPosZ;
};

struct _REGENE_EVENT
{
	int	  sRegenePoint;
	float fRegenePosX;
	float fRegenePosY;
	float fRegenePosZ;
	float fRegeneAreaZ;
	float fRegeneAreaX;
};

struct _KNIGHTS_USER
{
	BYTE    byUsed;
	CString strUserName;
};

struct _MAGIC_TABLE
{
	long	iNum;
	BYTE	bBeforeAction;
	BYTE	bTargetAction;
	BYTE	bSelfEffect;
	BYTE	bFlyingEffect;
	int		iTargetEffect;
	BYTE	bMoral;
	int		sSkillLevel;	
	int		sSkill;
	int		sMsp;
	int		sHP;
	BYTE	bItemGroup;
	long	iUseItem;
	BYTE	bCastTime;
	BYTE	bReCastTime;
	BYTE	bSuccessRate;
	BYTE	bType1;
	BYTE	bType2;
	int		sRange;
	BYTE	bEtc;
};

struct _MAGIC_TYPE8
{
	long    iNum;
	BYTE    bTarget;
	int		sRadius;
	BYTE    bWarpType;
	int		sExpRecover;
};

struct _MAGIC_TYPE5
{
	long	iNum;
	BYTE	bType;
	BYTE	bExpRecover;
	int		sNeedStone;
};

struct _MAGIC_TYPE4
{
	long    iNum;
	int		sMSP;
	BYTE    bBuffType;
	BYTE    bRadius;
	int		sDuration;
	BYTE    bAttackSpeed;
	BYTE    bSpeed;
	int		sAC;
	BYTE    bAttack;
	int		sMaxHP;
	BYTE    bHitRate;
	int		sAvoidRate;
	BYTE    bStr;
	BYTE    bSta;
	BYTE    bDex;
	BYTE    bIntel;
	BYTE    bCha;
	BYTE    bFireR;
	BYTE    bColdR;
	BYTE    bLightningR;
	BYTE    bMagicR;
	BYTE    bDiseaseR;
	BYTE    bPoisonR;
};

struct _MAGIC_TYPE3
{
	long	iNum;
	BYTE	bRadius;
	int		sAngle;
	int		sFirstDamage;
	int		sEndDamage;
	int		sTimeDamage;
	BYTE	bDirectType;
	BYTE	bDuration;
	BYTE	bAttribute;
};

struct _MAGIC_TYPE2
{
	long    iNum;
	BYTE    bHitType;
	int		sHitRate;
	int		sAddDamage;
	int		sAddRange;
	BYTE    bNeedArrow;
};

struct _MAGIC_TYPE1
{
	long	iNum;
	BYTE	bHitType;
	int		sHitRate;
	int		sHit;
	BYTE	bDelay;
	BYTE	bComboType;
	BYTE	bComboCount;
	int		sComboDamage;
	int		sRange;
};

struct _ZONE_SERVERINFO
{
	short		sServerNo;
	char		strServerIP[20];
	_ZONE_SERVERINFO() {memset(strServerIP, 0x00, 20);};
};

struct _WARP_INFO
{
	short	sWarpID;
	char	strWarpName[32];
	char	strAnnounce[256];
	DWORD	dwPay;
	short	sZone;
	float	fX;
	float	fY;
	float	fZ;
	float	fR;

	_WARP_INFO() {
		sWarpID = 0; sZone = 0;
		fX = fZ = fY = fR = 0.0f;
		memset( strWarpName, 0x00, 32 ); memset( strAnnounce, 0x00, 256 );
	};
};

struct _HOME_INFO
{
	BYTE	bNation;
	long	ElmoZoneX;
	long	ElmoZoneZ;
	BYTE	ElmoZoneLX;
	BYTE	ElmoZoneLZ;
	long	KarusZoneX;
	long	KarusZoneZ;
	BYTE	KarusZoneLX;
	BYTE	KarusZoneLZ;
	long	FreeZoneX;
	long	FreeZoneZ;
	BYTE	FreeZoneLX;
	BYTE	FreeZoneLZ;
//
	long	BattleZoneX;
	long	BattleZoneZ;
	BYTE	BattleZoneLX;
	BYTE	BattleZoneLZ;
//
};

struct _START_POSITION
{
	int	ZoneID;
	int	sKarusX;
	int	sKarusZ;
	int	sElmoradX;
	int	sElmoradZ;
	int	sKarusGateX;
	int	sKarusGateZ;
	int	sElmoradGateX;
	int	sElmoradGateZ;
	BYTE bRangeX;
	BYTE bRangeZ;
};

struct _SERVER_RESOURCE
{
	int nResourceID;
	CString strResource;
};

enum AuthorityTypes
{
	AUTHORITY_GAME_MASTER			= 0,
	AUTHORITY_PLAYER				= 1,
	AUTHORITY_MUTED					= 11,
	AUTHORITY_ATTACK_DISABLED		= 12,
	AUTHORITY_LIMITED_GAME_MASTER	= 250,
	AUTHORITY_BANNED				= 255
};

#endif
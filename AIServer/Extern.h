#pragma once

extern BOOL	g_bNpcExit;

struct	_PARTY_GROUP
{
	WORD wIndex;
	short uid[8];		// �ϳ��� ��Ƽ�� 8����� ���԰���
	_PARTY_GROUP() {
		for(int i=0;i<8;i++)
			uid[i] = -1;
	};
};

struct _MAKE_WEAPON
{
	BYTE	byIndex;		// ���� ���� ����
	short	sClass[MAX_UPGRADE_WEAPON];		// 1������ Ȯ��
	_MAKE_WEAPON() {
		for(int i=0;i<MAX_UPGRADE_WEAPON;i++)
			sClass[i] = 0;
	};
};

struct _MAKE_ITEM_GRADE_CODE
{
	BYTE	byItemIndex;		// item grade
	short	sGrade[9];
};	

struct _MAKE_ITEM_LARE_CODE
{
	BYTE	byItemLevel;			// item level �Ǵ� 
	short	sLareItem;				// lareitem ���� Ȯ��
	short	sMagicItem;				// magicitem ���� Ȯ��
	short	sGereralItem;			// gereralitem ���� Ȯ��
};

#include "../shared/database/structs.h"
#pragma once

enum UserUpdateType
{
	UPDATE_LOGOUT,
	UPDATE_ALL_SAVE,
	UPDATE_PACKET_SAVE,
};

struct _ITEM_TABLE
{
	long  m_iNum;				// item num
	BYTE  m_bCountable;			// ���� ���� ������
};

#define CString std::string
#include "../shared/globals.h"
#pragma once

#include "../shared/stdafx.h"
#include "packet.h"			// packet ����
#include "define.h"			// define
#include "ServerDlg.h"

// temporarily redefine AfxMessageBox
#define AfxMessageBox(s) printf("%s\n", s)
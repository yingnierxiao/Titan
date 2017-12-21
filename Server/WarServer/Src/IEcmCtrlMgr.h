﻿/*******************************************************************
** 文件名: IEcmCtrlMgr.h
** 版  权: (C) 深圳冰川网络技术有限公司(www.weimingtech.com)
** 创建人: 李有红
** 日  期: 2017/11/5
** 版  本: 1.0
** 描  述: 战场真人玩家经济控制接口
** 应  用: 
**************************** 修改记录 ******************************
** 修改人:  
** 日  期: 
** 描  述:  
********************************************************************/
#pragma once

#include "IWarMgr.h"
#include "Vector3.h"
#include "IActorService.h"

class IEcmCtrlMgr : public __IWarMgr
{
public:
	// 计算经济控制系数
	virtual void calcEcmControlRatio() = 0;

	// 计算经济控制线
	virtual void calcEcmControlLine() = 0;


	// 计算经济控制后获得的天赋点
	virtual int controlTelentPoint(PDBID pdbID, int nAddValue, ETalentDropType nDropType) = 0;

	// 控制连杀经济
	virtual int controlContKillTelentPoint(IActorService* pDead, IActorService* pAttacker, int nAddValue, float fDropGainTalentRate, float fContKillEmcCtrlRatio, int nMaxMultiKill, int nContKillCount) = 0;
};
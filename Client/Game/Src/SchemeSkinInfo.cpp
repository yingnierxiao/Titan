﻿/*******************************************************************
** 文件名:	SchemeSkinInfo.cpp
** 版  权:	(C) 深圳冰川网络技术有限公司 2008 - All Rights Reserved
** 创建人:	李界华
** 日  期:	4/2/2015
** 版  本:	1.0
** 描  述:	皮肤配置表 SkinInfo.csv

			
**************************** 修改记录 ******************************
** 修改人: 
** 日  期: 
** 描  述: 
********************************************************************/
#include "StdAfx.h"
#include "SchemeSkinInfo.h"
#include "IClientGlobal.h"
#include "EntityViewDef.h"
#include <string>
using namespace std;

/** 
@param   
@param   
@return  
*/
CSchemeSkinInfo::CSchemeSkinInfo(void)
{	

}

/** 
@param   
@param   
@return  
*/
CSchemeSkinInfo::~CSchemeSkinInfo(void)
{	
}

/** 载入脚本
@param   
@param   
@return  
*/
bool CSchemeSkinInfo::LoadScheme(void)
{
	ISchemeEngine *pSchemeEngine = gClientGlobal->getSchemeEngine();
	if(pSchemeEngine == NULL)
	{
		return false;
	}

    string stringPath = SCP_PATH_FORMAT( SKININFO_SCHEME_FILENAME );
	bool bResult = pSchemeEngine->LoadScheme(stringPath.c_str(), (ISchemeUpdateSink *)this);
	if(!bResult)
	{
		ErrorLn("config file load failed! filename=" << stringPath.c_str());
		return false;
	}

	return true;	
}

/** 关闭
@param   
@param   
@return  
*/
void CSchemeSkinInfo::Close(void)
{	
	// 清空
	m_mapSkinInfoScheme.clear();
}

/** CSV配置载入时通知
@param   pCSVReader：读取CSV的返回接口
@param   szFileName：配置文件名
@param   
@return  
@note     
@warning 不要在此方法释放pCSVReader或者pTiXmlDocument,因为一个文件对应多个sink
@retval buffer 
*/ 
bool CSchemeSkinInfo::OnSchemeLoad(SCRIPT_READER reader,const char* szFileName)
{

	ICSVReader * pCSVReader = reader.pCSVReader;
	if ( pCSVReader==0 || reader.type!=READER_CSV)
		return false;

	// 清空
	m_mapSkinInfoScheme.clear();

	int nLen = 0;
	// 读取
	int nRecordCount = pCSVReader->GetRecordCount();
	for (int nRow=0; nRow<nRecordCount; ++nRow)
	{
		SkinInfoScheme item;

		EntityViewItem entityInfo;
		// 皮肤ID
		item.nSkinID = pCSVReader->GetInt(nRow, SKININFO_COL_ID, 0);

		// 皮肤类型 ENTITY_TYPE
		item.nSkinType = pCSVReader->GetInt(nRow, SKININFO_COL_SKIN_TYPE, 0);
		if(item.nSkinType <= TYPE_ENTITY || item.nSkinType >= TYPE_MAX)
		{
			ErrorLn("read skin config fail for skin type invalid, skin type="<< item.nSkinType);
			return false;
		}

		// 皮肤名
		nLen = sizeof(item.szSkinName);
		pCSVReader->GetString(nRow, SKININFO_COL_SKINNAME, item.szSkinName, nLen);

		// 预设体路径
		nLen = sizeof(item.szSkinPatch);
		pCSVReader->GetString(nRow, SKININFO_COL_PREFABPATH, item.szSkinPatch, nLen);		

		//皮肤缩放
		pCSVReader->GetInt(nRow, SKININFO_COL_SKIN_TYPE, 0);

		// 是否预加载
		item.nPreLoad = pCSVReader->GetInt(nRow, SKININFO_COL_PRELOAD, 0);

		//皮肤缩放
		item.fSkinScale = pCSVReader->GetFloat(nRow, SKININFO_COL_SKINSCALE, 1.0f);

        // 皮肤等级
		item.nSkinLevel = pCSVReader->GetInt(nRow, SKININFO_COL_SKINLEVEL, 0);

        // 皮肤图片ID
		item.nSkinIconID = pCSVReader->GetInt(nRow, SKININFO_COL_SKINICON, 0);

        // 选人光效ID
        item.nSelectEffectID = pCSVReader->GetInt(nRow, SKININFO_COL_SELECT_EFFECT, 0);

		// 选人音效ID
		item.nSoundID = pCSVReader->GetInt(nRow, SKININFO_COL_SELECT_SOUND, 0);

		entityInfo.nSkinID = item.nSkinID;
		entityInfo.EntityType = item.nSkinType;
		entityInfo.ndata1 = item.nPreLoad;  //目前用来表示是否需要预加载
		entityInfo.fSkinScale = item.fSkinScale;

		sstrcpyn(entityInfo.szModelPath,item.szSkinPatch,sizeof(entityInfo.szModelPath));
		sstrcpyn(entityInfo.szName,item.szSkinName,sizeof(entityInfo.szName));

		//加载模型
		gClientGlobal->getRenderView()->loadSkin(entityInfo);
		
		// 插入到列表中
		m_mapSkinInfoScheme[item.nSkinID] = item;		
	}

	return true;
}


/** 配置动态更新时通知
@param   pCSVReader：读取CSV的返回接口
@param   szFileName：配置文件名
@param   
@return  
@note     
@warning 不要在此方法释放pCSVReader或者pTiXmlDocument,因为一个文件对应多个sink
@retval buffer 
*/
bool CSchemeSkinInfo::OnSchemeUpdate(SCRIPT_READER reader, const char* szFileName)
{
	return OnSchemeLoad(reader, szFileName);
}



/////////////////////////ISchemeMonster/////////////////////////
/** 取得皮肤配置信息
@param nMonsterId:	皮肤ID
@return  
*/
SkinInfoScheme* CSchemeSkinInfo::getSkinShemeInfo(int nSkinID)
{
	TMAP_SKININFOSCHEME::iterator iter = m_mapSkinInfoScheme.find(nSkinID);
	if (iter == m_mapSkinInfoScheme.end())
	{
		return NULL;
	}
	return &(iter->second);
}
﻿/*******************************************************************
** 文件名:	CenterConnectorService.h
** 版  权:	(C) 深圳冰川网络技术有限公司 2008 - All Rights Reserved
** 创建人:	
** 日  期:	
** 版  本:	1.0
** 描  述:	连接中心服务器

			
********************************************************************/

#pragma once

#include "IServiceMgr.h"
#include "IFramework.h"
#include "ISocialGlobal.h"
#include "ICenterServerConnector.h"
#include "ICenterConnectorService.h"
#include "IMessageDispatch.h"
#include "INetMessageHandler.h"
#include "net/Reactor.h"
#include "net/net.h"
#include <WinSock.h>

using namespace CenterServer;

/**
	职责: 
	1.连接中心服务器
	2.根据中心服务器上返回的网关数量，对网关服务器发起连接
*/
class CenterConnectorService : public ICenterConnectorService,public ICenterServerMessageHandler,public EventHandler
{
protected:
	ICenterServerConnector *    m_pConnector;
    string                      m_strIp;
    int                         m_nPort;

public:
	CenterConnectorService() : m_pConnector(0)
        , m_nPort(0)
	{
	}

	virtual ~CenterConnectorService()
	{
		Stop();
	}

	bool Start( const char * ip,int port )
	{
		// 挂接网络事件
		Reactor * reactor = gSocialGlobal->getFramework()->get_reactor();
		if ( reactor==0 )
			return false;

		HANDLE hNetEvent = GetNetworkEventHandle(NetworkUserType_User1);
		reactor->AddEvent(hNetEvent);
		reactor->RegisterEventHandler( hNetEvent,this);

        m_strIp = ip;
        m_nPort = port;

		m_pConnector = CreateCenterServerConnectorProc(this,(TimerAxis *)gSocialGlobal->getTimerService()->getAxis());
		if ( m_pConnector==0 )
		{
			ErrorLn(_GT("创建中心服务器连接器失败!"));
			return false;
		}
		m_pConnector->RegisterServer(_GT("社会服"),0,MSG_ENDPOINT_SOCIAL );

		// 中心连接器服务已启动
		IEventEngine* pEventEngine = gSocialGlobal->getEventEngine();
		if(pEventEngine)
		{
			pEventEngine->FireExecute(EVENT_SYSTEM_CENTERCONNECTOR_STARTED, SOURCE_TYPE_SYSTEM, 0, NULL, 0);
		}

		return true;
	}

	void Stop()
	{
		// 中心连接器服务已关闭
		IEventEngine* pEventEngine = gSocialGlobal->getEventEngine();
		if(pEventEngine)
		{
			pEventEngine->FireExecute(EVENT_SYSTEM_CENTERCONNECTOR_STOP, SOURCE_TYPE_SYSTEM, 0, NULL, 0);
		}

		// 挂接网络事件
		Reactor * reactor = gSocialGlobal->getFramework()->get_reactor();
		if ( reactor )
		{
			HANDLE hNetEvent = GetNetworkEventHandle(NetworkUserType_User1);
			reactor->RemoveEvent( hNetEvent );
		}

		if ( m_pConnector )
		{
			ErrorLn(_GT("正在断开与中心服务器连接!") );
			m_pConnector->Release();
			m_pConnector = 0;
		}
	}

    /**
	@name                 : 连接中心服务器
	@brief                :
	@return               :
	*/
	virtual bool            connectServer(void)
    {
        if ( !m_pConnector->ConnectCenterServer(m_strIp.c_str(), m_nPort,NetworkUserType_User1) )
        {
            ErrorLn(_GT("连接中心服务器失败!ip=") << m_strIp.c_str() << " port=" << m_nPort );
            return false;
        }

        return true;
    }

	virtual void OnEvent( HANDLE event )
	{
		DispatchNetworkEvents(100,NetworkUserType_User1);
	}

	/**
	@name                 : 向中心服务器更新服务器信息
	@brief                :
	@param  pServerInfo   : 服务器信息内容,不同的服务器有不同的扩展信息,例如区域服务器就有该服务器负责的区域ID等
	@param  wLen          : 服务器信息长度
	@return               :
	*/
	virtual bool updateServerInfo(ibuffer & info)
	{
		return m_pConnector->UpdateServerInfo(info.data(),info.size());
	}

	/**
	@name         : 取得本地服务器在中心服务器上的ID
	@brief        :
	*/
	virtual CSID  getLocalServerCSID()
	{
		return m_pConnector->GetLocalServerCSID();
	}

	/**
	@name         : 取得网络上所有服务器的个数
	@brief        :
	*/
	virtual DWORD getAllServersCount()
	{
		return m_pConnector->GetAllServersCount();
	}

	/**
	@name                : 取得服务器列表
	@brief               :
	@param serverPtrArray: 服务器指针数组,用来返回指向服务器信息结构的指针
	@param dwArratSize   : 指针数组的大小,防止越界
	@param GroupFilter   : 按组ID进行过滤,如果GroupFilter为INVALID_SERVER_ID则表示不进行组过滤
	@param wTypeFilter   : 按服务器类型过滤,如果wTypeFilter为MSG_SERVERID_UNKNOW则表示不进行服务器类型过滤
	@note                : 例如返回组ID等于1的所有网关服务器:
	@note                : ServerInfo * serverPtrArray[MAX_SERVER_COUNT];
	@note                : DWORD count=ICenterServerConnector::GetServerList(serverPtrArray,1024,1,MSG_SERVER_GATE);
	@return              :
	*/
	virtual DWORD getServerList(ServerData * serverPtrArray[],DWORD dwArratSize,CGID GroupFilter=INVALID_SERVER_ID,WORD wTypeFilter=MSG_ENDPOINT_UNKNOW)
	{
		return m_pConnector->GetServerList(serverPtrArray,dwArratSize,GroupFilter,wTypeFilter);
	}

	/**
	@name                : 取得服务器信息
	@brief               :
	@param  ServerID     : 服务器ID
	@return              : 如果目标服务器存在则返回服务器信息,否则返回0
	*/
	virtual ServerData getServerInfo(CSID ServerID)
	{
        ServerData * pServerData = m_pConnector->GetServerInfo(ServerID);
		return pServerData ? *pServerData : ServerData();
	}


	/**
	@name                : 取得本服务器类型
	@return              : 
	*/
	virtual WORD        getServerType(void)
    {
        return m_pConnector->GetServerType();
    }

	/**
	@name                : 向某个服务器发送一条消息
	@brief               : 服务器之间的消息转发
	@param DestServerID  : 目标服务器ID
	@param data			 : 消息内容
	@param len			 : 消息长度
	@return              : 是否成功发到了中心服务器
	*/
	virtual void  postMessage(CSID DestServerID, void * data, int len)
	{
		m_pConnector->PostMessage(DestServerID, (const char*)data, len);
	}

	/**
	@name                : 向多个服务器广播消息
	@brief               :
	@param DestServerArray:目标服务器数组
	@param wDestServerCount:目标服务器个数
	@param pMessage      : 消息内容
	@param wMessageLen   : 消息长度
	@return              : 是否成功发到了中心服务器
	*/
	virtual void  broadcastMessage(string & id_buff,ibuffer & msg)
	{
		CSID servers[MAX_SERVER_COUNT];

		int count = id_buff.size() / sizeof(CGID);
		if ( count>MAX_SERVER_COUNT || count * sizeof(CGID) != id_buff.size() )
		{
			return;
		}

		memcpy( servers,id_buff.c_str(),count * sizeof(CGID) );

		m_pConnector->BroadcastMessage(servers,count,msg.data(),msg.size());
	}


	/** 向场景服发送消息
	@param   
	@param   
	@return  
	*/
	virtual void sendDataToZoneSvr(LPCSTR pData, size_t nLen, CSID ServerID = INVALID_SERVER_ID)
	{
		if(ServerID == INVALID_SERVER_ID)
		{
			ServerData * svrInfo[MAX_SERVER_COUNT];
			DWORD count = m_pConnector->GetServerList(svrInfo, MAX_SERVER_COUNT, INVALID_SERVER_ID, MSG_ENDPOINT_SCENE);
			if(count <= 0)
			{		
				return;
			}

			for(DWORD i  = 0; i < count; i++)
			{
				m_pConnector->PostMessage(svrInfo[i]->dwServerID, pData, (ushort)nLen);
			}	
		}
		else
		{
			m_pConnector->PostMessage(ServerID, pData, (ushort)nLen);
		}
	}

    /** 向HttpServer服发送消息
    @param   
    @param   
    @return  
    */
    virtual void sendDataToDBHttpSvr(BYTE byKeyModule, BYTE byKeyAction, LPCSTR pData, size_t nLen)
    {
        if(m_pConnector == NULL)
        {
            return;
        }

        SGameMsgHead head;
        head.SrcEndPoint = MSG_ENDPOINT_SOCIAL;
        head.DestEndPoint= MSG_ENDPOINT_DBHTTP;
        head.byKeyModule = byKeyModule;
        head.byKeyAction = byKeyAction;

        obuf8192 obuf;
        obuf.push_back(&head, sizeof(head));
        if (pData != NULL && nLen > 0)
        {
            obuf.push_back(pData, nLen);
        }

        ServerData* svrInfo[MAX_SERVER_COUNT];
        int count = m_pConnector->GetServerList(svrInfo, MAX_SERVER_COUNT, INVALID_SERVER_ID, MSG_ENDPOINT_DBHTTP);
        if(count <= 0)
        {		
            return;
        }

        for(int i  = 0; i < count; i ++)
        {
            m_pConnector->PostMessage(svrInfo[i]->dwServerID, obuf.data(), obuf.size());
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
	/**
	@name         : 通知服务器列表已经更新
	@brief        : 服务器列表更新只会在有服务器进入退出时触发
	@note         : 具体的列表调用ICenterServerConnector::GetServerList获得
	*/
	virtual void HandleServerListUpdated()
	{
		// 如果有网关服务器更新（启动/停止），则连接网关服务器（已连接的会忽略）
		ServerData * svrInfo[MAX_SERVER_COUNT];
		ulong count = m_pConnector->GetServerList(svrInfo, MAX_SERVER_COUNT, INVALID_SERVER_ID, MSG_ENDPOINT_GATEWAY);

		// 逐个去连网关服
		for(ulong i = 0; i < count; i++)
		{
			if(svrInfo[i])
			{
				Assert(svrInfo[i]->wServerInfoLen == sizeof(ServerInfo_Gateway));
				ServerInfo_Gateway * edg = (ServerInfo_Gateway *)svrInfo[i]->pServerInfoData;

				in_addr addr;
				addr.s_addr = edg->dwGatewayIP;

				IGatewayAcceptorService *pGatewayAcceptor = gSocialGlobal->getGatewayAcceptorService();
				if ( pGatewayAcceptor )
				{
					pGatewayAcceptor->connectGateway(inet_ntoa(addr), ntohs(edg->wGatewayPort));				
				}
			}
		}

		gSocialGlobal->getNetMessageHandler()->handleServerListUpdated();
	}

	/**
	@name         : 通知服务器信息更新
	@brief        : 例如服务器人数变化等等
	*/
	virtual void HandleServerInfoUpdated(CSID ServerID,SERVER_STATE nState,ServerData * pInfo)
	{
		gSocialGlobal->getNetMessageHandler()->handleServerInfoUpdated(ServerID, nState, pInfo);
	}

	/**
	@name         : 处理其他服务器通过中心服务器转发的消息
	@brief        : 具体内容不清楚
	@param server : 该消息是哪个服务器发过来的
	@param pData  : 消息内容
	@param wDataLen:消息长度
	*/
	virtual void HandlePostMessage(CGID server,const char * pData,WORD wDataLen)
	{
		gSocialGlobal->getNetMessageHandler()->handlePostMessage(server, pData, wDataLen);
	}

	/**
	@name         : 收到请求,执行一个命令
	@brief        : 
	@param server : 发送命令的服务器ID
	@param szCommand : 命令行
	@return       : 执行后调用ICenterServerConnector::SendCommandResponse返回命令执行结果
	*/
	virtual void HandleCommandRequest(CGID server,const char * szCommand)
	{
		gSocialGlobal->getNetMessageHandler()->handleCommandRequest(server,szCommand);
	}

	/**
	@name          : 处理命令执行的结果
	@brief         :
	@param szCommand:执行的命令字符串
	@param bSuccess: 命令是否执行成功
	@param szOutput: 输出的提示信息
	@return        :
	*/
	virtual void HandleCommandResponse(const char * szCommand,bool bSuccess,const char * szOutput )
	{
		gSocialGlobal->getNetMessageHandler()->handleCommandResponse(szCommand, bSuccess, szOutput);
	}

	virtual void release()
	{
		Stop();
		delete this;
	}
};
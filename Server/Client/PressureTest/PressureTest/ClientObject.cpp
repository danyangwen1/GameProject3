﻿#include "stdafx.h"
#include "ClientObject.h"
#include <complex>
#include "..\Src\Message\Msg_RetCode.pb.h"
#include "PacketHeader.h"
#include "..\Src\Message\Msg_Move.pb.h"
#include "..\Src\Message\Game_Define.pb.h"
#include "..\Src\Message\Msg_Copy.pb.h"
#include "..\Src\Message\Msg_Game.pb.h"
#include "..\Src\Message\Msg_LoginCltData.pb.h"
#include "..\Src\ServerEngine\XMath.h"
#include "..\Src\ServerEngine\CommonFunc.h"

int g_LoginReqCount = 0;
int g_LoginCount = 0;
int g_EnterCount = 0;

CClientObject::CClientObject(void)
{
	m_bLoginOK = FALSE;

	m_dwAccountID = 0;
	m_dwHostState = ST_NONE;
	m_x = 0;
	m_y = 0;
	m_z = 13;

	m_ft = PI * 2 * (rand() % 360) / 360;

	m_ClientConnector.RegisterMsgHandler((IMessageHandler*)this);
}

CClientObject::~CClientObject(void)
{
}

BOOL CClientObject::DispatchPacket(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	BOOL bHandled = TRUE;
	switch(dwMsgID)
	{
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_ACCOUNT_REG_ACK,		OnCmdNewAccountAck);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_ACCOUNT_LOGIN_ACK,		OnMsgAccountLoginAck);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_SELECT_SERVER_ACK,      OnMsgSelectServerAck);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_ROLE_LIST_ACK,			OnMsgRoleListAck);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_NOTIFY_INTO_SCENE,		OnMsgNotifyIntoScene);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_ROLE_CREATE_ACK,		OnMsgCreateRoleAck);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_OBJECT_NEW_NTY,			OnMsgObjectNewNty);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_OBJECT_ACTION_NTY,		OnMsgObjectActionNty);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_OBJECT_REMOVE_NTY,		OnMsgObjectRemoveNty);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_ENTER_SCENE_ACK,		OnCmdEnterSceneAck);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_ROLE_LOGIN_ACK,			OnMsgRoleLoginAck);
			PROCESS_MESSAGE_ITEM_CLIENT(MSG_ROLE_OTHER_LOGIN_NTY,	OnMsgOtherLoginNty);


		default:
		{
			bHandled = FALSE;
		}
		break;
	}

	return bHandled;
}


BOOL CClientObject::OnCmdEnterSceneAck( UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen )
{
	EnterSceneAck Ack;
	Ack.ParsePartialFromArray(PacketBuf, BufLen);
	PacketHeader* pHeader = (PacketHeader*)PacketBuf;
	if(Ack.copyid() == 6)
	{
		m_dwHostState = ST_EnterSceneOK;

		//表示进入主城完成
	}
	else if(Ack.copyid() == m_dwToCopyID)
	{
		m_dwHostState = ST_EnterCopyOK;
	}

	m_dwCopyID = Ack.copyid();
	m_dwCopyGuid = Ack.copyguid();
	//if(Ack.copytype() == 6)
	//{
	//	SendMainCopyReq();
	//}
	//else
	//{
	//	SendAbortCopyReq();
	//}


	return TRUE;
}

BOOL CClientObject::OnMsgNotifyIntoScene(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	NotifyIntoScene Nty;
	Nty.ParsePartialFromArray(PacketBuf, BufLen);
	EnterSceneReq Req;
	Req.set_roleid(Nty.roleid());
	Req.set_serverid(Nty.serverid());
	Req.set_copyguid(Nty.copyguid());
	Req.set_copyid(Nty.copyid());

	m_dwCopySvrID = Nty.serverid();
	m_dwCopyID = Nty.copyid();
	m_dwCopyGuid = Nty.copyguid();

	m_ClientConnector.SendData(MSG_ENTER_SCENE_REQ, Req, Nty.serverid(), Nty.copyguid());
	return TRUE;
}

BOOL CClientObject::OnMsgObjectNewNty(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	return TRUE;
}

BOOL CClientObject::OnMsgObjectActionNty(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	ObjectActionNty Nty;
	Nty.ParsePartialFromArray(PacketBuf, BufLen);
	for(int i = 0; i < Nty.actionlist_size(); i++)
	{
		const ActionItem& Item = Nty.actionlist(i);
		float y = Item.ft();
	}



	return TRUE;
}

BOOL CClientObject::OnMsgObjectRemoveNty(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	return TRUE;
}

BOOL CClientObject::OnUpdate( UINT32 dwTick )
{
	m_ClientConnector.Render();

	if(m_dwHostState == ST_NONE)
	{
		if(m_ClientConnector.GetConnectState() == Not_Connect)
		{
			m_ClientConnector.SetClientID(0);
			//m_ClientConnector.ConnectToServer("127.0.0.1", 5678);
			//m_ClientConnector.ConnectToServer("47.93.31.69", 5678);

			m_ClientConnector.ConnectToServer("47.93.31.69", 8080);	   //account
			//m_ClientConnector.ConnectToServer("47.93.31.69", 9008);  //game
			//m_ClientConnector.ConnectToServer("47.93.31.69", 8083);  //log
			//m_ClientConnector.ConnectToServer("47.93.31.69", 8084);  //logic
			//m_ClientConnector.ConnectToServer("47.93.31.69", 9876);  //proxy
		}

		if(m_ClientConnector.GetConnectState() == Succ_Connect)
		{
			SendNewAccountReq(m_strAccountName, m_strPassword);

			m_dwHostState = ST_Register;
		}
	}

	if(m_dwHostState == ST_RegisterOK)
	{
		SendAccountLoginReq(m_strAccountName, m_strPassword);

		m_dwHostState = ST_AccountLogin;
	}

	if(m_dwHostState == ST_AccountLoginOK)
	{
		SendSelectSvrReq(201);

		m_dwHostState = ST_SelectSvr;
	}

	if(m_dwHostState == ST_SelectSvrOK)
	{
		SendRoleListReq();

		m_dwHostState = ST_RoleList;
	}

	if(m_dwHostState == ST_RoleListOK)
	{
		SendRoleLoginReq(m_RoleIDList[0]);
		m_dwHostState = ST_EnterScene;
	}

	if(m_dwHostState == ST_EnterSceneOK)
	{
		TestMove();
		//TestCopy();
	}

	if(m_dwHostState == ST_EnterCopyOK)
	{
		//TestMove();
		//TestExitCopy();
	}

	if(m_dwHostState == ST_Disconnected)
	{
		int randValue = rand() % 100;
		if((randValue < 80) && (randValue > 70))
		{
			m_dwHostState = ST_NONE;
		}
	}

	if(m_dwHostState == ST_Overed)
	{

	}

	return TRUE;
}


BOOL CClientObject::SendNewAccountReq( std::string szAccountName, std::string szPassword )
{
	AccountRegReq Req;
	Req.set_accountname(szAccountName);
	Req.set_password(szPassword);
	m_ClientConnector.SendData(MSG_ACCOUNT_REG_REQ, Req, 0, 0);
	return TRUE;
}



BOOL CClientObject::SendAccountLoginReq(std::string szAccountName, std::string szPassword)
{
	AccountLoginReq Req;
	Req.set_accountname(szAccountName);
	Req.set_password(szPassword);
	m_ClientConnector.SendData(MSG_ACCOUNT_LOGIN_REQ, Req, 0, 0);

	return TRUE;
}

BOOL CClientObject::SendSelectSvrReq(UINT32 dwSvrID)
{
	SelectServerReq Req;
	Req.set_serverid(dwSvrID);
	m_ClientConnector.SendData(MSG_SELECT_SERVER_REQ, Req, 0, 0);
	return TRUE;
}

BOOL CClientObject::OnMsgAccountLoginAck(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	AccountLoginAck Ack;
	Ack.ParsePartialFromArray(PacketBuf, BufLen);
	PacketHeader* pHeader = (PacketHeader*)PacketBuf;

	if(Ack.retcode() == MRC_FAILED)
	{
		MessageBox(NULL, "登录失败! 密码或账号不对!!", "提示", MB_OK);
		m_dwHostState = ST_Overed;
		return TRUE;
	}
	else
	{
		m_dwAccountID = Ack.accountid();
	}

	m_dwHostState = ST_AccountLoginOK;
	return TRUE;
}


BOOL CClientObject::OnMsgSelectServerAck(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	SelectServerAck Ack;
	Ack.ParsePartialFromArray(PacketBuf, BufLen);
	PacketHeader* pHeader = (PacketHeader*)PacketBuf;
	m_ClientConnector.DisConnect();
	m_ClientConnector.ConnectToServer(Ack.serveraddr(), Ack.serverport());
	m_dwHostState = ST_SelectSvrOK;
	return TRUE;
}

BOOL CClientObject::OnMsgRoleListAck(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	RoleListAck Ack;
	Ack.ParsePartialFromArray(PacketBuf, BufLen);
	PacketHeader* pHeader = (PacketHeader*)PacketBuf;

	for( int i = 0 ; i < Ack.rolelist_size(); i++)
	{
		m_RoleIDList.push_back(Ack.rolelist(i).roleid());
		m_dwHostState = ST_RoleListOK;
	}

	if(Ack.rolelist_size() <= 0)
	{
		m_dwHostState = ST_RoleCreate;

		SendCreateRoleReq(m_dwAccountID, m_strAccountName + CommonConvert::IntToString(rand() % 1000), rand() % 4 + 1);
	}

	return TRUE;
}

BOOL CClientObject::OnMsgCreateRoleAck(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	RoleCreateAck Ack;
	Ack.ParsePartialFromArray(PacketBuf, BufLen);
	PacketHeader* pHeader = (PacketHeader*)PacketBuf;
	m_RoleIDList.push_back(Ack.roleid());
	m_dwHostState = ST_RoleListOK;
	return TRUE;
}

BOOL CClientObject::OnCmdNewAccountAck( UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	AccountRegAck Ack;
	Ack.ParsePartialFromArray(PacketBuf, BufLen);
	PacketHeader* pHeader = (PacketHeader*)PacketBuf;
	m_dwHostState = ST_RegisterOK;
	return TRUE;
}

BOOL CClientObject::SendCreateRoleReq( UINT64 dwAccountID, std::string strName, UINT32 dwCarrerID)
{
	RoleCreateReq Req;
	Req.set_accountid(dwAccountID);
	Req.set_name(strName);
	Req.set_carrer(dwCarrerID);
	m_ClientConnector.SendData(MSG_ROLE_CREATE_REQ, Req, 0, 0);
	return TRUE;
}



BOOL CClientObject::SendDelCharReq( UINT64 dwAccountID, UINT64 u64RoleID )
{
	RoleDeleteReq Req;
	Req.set_accountid(dwAccountID);
	Req.set_roleid(u64RoleID);
	m_ClientConnector.SendData(MSG_ROLE_DELETE_REQ, Req, 0, 0);
	return TRUE;
}

VOID CClientObject::TestCopy()
{
	MainCopyReq Req;
	Req.set_copyid(rand() % 67 + 10001);
	m_dwToCopyID = Req.copyid();
	m_ClientConnector.SendData(MSG_MAIN_COPY_REQ, Req, m_RoleIDList[0], 0);
	m_dwHostState = ST_EnterCopy;
}


VOID CClientObject::TestExitCopy()
{
	AbortCopyReq Req;
	Req.set_copyid(m_dwCopyID);
	Req.set_copyguid(m_dwCopyGuid);
	Req.set_serverid(m_dwCopySvrID);
	m_ClientConnector.SendData(MSG_COPY_ABORT_REQ, Req, m_RoleIDList[0], 0);
	m_dwHostState = ST_EnterCopy;
}

BOOL CClientObject::MoveForward(FLOAT fDistance)
{
	if(m_ft <= 90.0f)
	{
		m_x += fDistance * sin(m_ft * PI / 180);
		m_z -= fDistance * cos(m_ft * PI / 180);
	}
	else if(m_ft <= 180.0f)
	{
		m_x += fDistance * sin(m_ft * PI / 180);
		m_z += fDistance * cos(m_ft * PI / 180);
	}
	else if(m_ft <= 270.0f)
	{
		m_x += fDistance * sin(m_ft * PI / 180);
		m_z += fDistance * cos(m_ft * PI / 180);
	}
	else if(m_ft <= 360.0f)
	{
		m_x += fDistance * sin(m_ft * PI / 180);
		m_z += fDistance * cos(m_ft * PI / 180);
	}

	return TRUE;
}

VOID CClientObject::TestMove()
{
	ObjectActionReq Req;
	ActionItem* pItem =  Req.add_actionlist();
	pItem->set_actionid(AT_WALK);
	pItem->set_objectguid(m_RoleIDList[0]);

	UINT32 dwTimeDiff = CommonFunc::GetTickCount() - m_dwMoveTime;
	if(dwTimeDiff < 160)
	{
		return ;
	}

	m_dwMoveTime = CommonFunc::GetTickCount();

	MoveForward(1.0f);

	if(m_x > 10)
	{
		m_x = 10;
		m_ft = abs(m_ft - 90);
	}
	if(m_z > 20)
	{
		m_z = 20;
		m_ft = abs(m_ft - 90);
	}
	if(m_x < -10)
	{
		m_x = -10;
		m_ft = abs(m_ft - 90);
	}
	if(m_z < 0)
	{
		0;
		m_z = 0;
		m_ft = abs(m_ft - 90);
	}

	pItem->set_x(m_x);
	pItem->set_y(0);
	pItem->set_z(m_z);
	pItem->set_ft(m_ft);

	m_ClientConnector.SendData(MSG_OBJECT_ACTION_REQ, Req, m_RoleIDList[0], m_dwCopyGuid);
}

BOOL CClientObject::SendRoleLogoutReq( UINT64 u64CharID )
{
	return TRUE;
}

BOOL CClientObject::SendRoleLoginReq(UINT64 u64CharID)
{
	RoleLoginReq Req;
	Req.set_roleid(u64CharID);
	m_ClientConnector.SendData(MSG_ROLE_LOGIN_REQ, Req, 0, 0);
	return TRUE;
}

BOOL CClientObject::SendRoleListReq()
{
	if(m_dwAccountID == 0)
	{
		ASSERT_FAIELD;
		m_dwHostState = ST_Overed;
		return TRUE;
	}

	RoleListReq Req;
	Req.set_accountid(m_dwAccountID);
	Req.set_logincode(12345678);
	m_ClientConnector.SendData(MSG_ROLE_LIST_REQ, Req, 0, 0);
	return TRUE;
}

BOOL CClientObject::SendMainCopyReq()
{
	MainCopyReq Req;
	Req.set_copyid(rand() % 67 + 10000);
	m_ClientConnector.SendData(MSG_MAIN_COPY_REQ, Req, m_RoleIDList[0], 0);
	return TRUE;
}

BOOL CClientObject::SendAbortCopyReq()
{
	AbortCopyReq Req;
	Req.set_copyid(m_dwCopyGuid);
	m_ClientConnector.SendData(MSG_COPY_ABORT_REQ, Req, m_RoleIDList[0], 0);

	m_dwHostState = ST_AbortCopy;
	return TRUE;
}

BOOL CClientObject::OnMsgOtherLoginNty( UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen )
{
	m_ClientConnector.CloseConnector();
	return TRUE;
}

BOOL CClientObject::OnMsgRoleLoginAck(UINT32 dwMsgID, CHAR* PacketBuf, INT32 BufLen)
{
	RoleLoginAck Ack;
	Ack.ParsePartialFromArray(PacketBuf, BufLen);
	PacketHeader* pHeader = (PacketHeader*)PacketBuf;
	m_RoleIDList.push_back(Ack.roleid());
	return TRUE;
}
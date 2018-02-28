﻿#include "Server/Base/NetAdapter.h"
#include "Include/Network/Network.hpp"
#include "Server/Base/RouterMgr.h"
#include "Server/Base/Service.h"
#include "Server/Base/ServerContext.h"

//按服务器和网关分组
struct BROADCAST_HEADER
{
	INNER_HEADER oInnerHeader;
	Array<int> oSessionList;
};

typedef std::unordered_map<int, BROADCAST_HEADER> BCHeaderMap;
typedef BCHeaderMap::iterator BCHeaderIter;
static BCHeaderMap oBCHeaderMap;

bool NetAdapter::SendExter(uint16_t uCmd, Packet* poPacket, SERVICE_NAVI& oNavi, uint32_t uPacketIdx /*= 0*/)
{
    assert(poPacket != NULL);
	Service* poService = g_poContext->GetService();
	if ((oNavi.uTarServer == g_poContext->GetServerID() && oNavi.nTarService == poService->GetServiceID()) || uPacketIdx > 0) //uPackeIdx>0表示客户端发出的指令
	{
		poPacket->AppendExterHeader(EXTER_HEADER(uCmd, poService->GetServiceID(), oNavi.nTarService, uPacketIdx));
		if (!poService->GetExterNet()->SendPacket(oNavi.nTarSession, poPacket))
		{
			poPacket->Release();
			return false;
		}
	}
	else
	{
		return SendInner(uCmd, poPacket, oNavi);
	}
	return true;
}

bool NetAdapter::SendInner(uint16_t uCmd, Packet* poPacket, SERVICE_NAVI& oNavi)
{
    assert(poPacket != NULL);
    if (oNavi.nTarService <= 0 || oNavi.nTarService > MAX_SERVICE_NUM)
    {
    	poPacket->Release();
    	return false;
    }
	Service* poService = g_poContext->GetService();
	ROUTER* poRouter = g_poContext->GetRouterMgr()->ChooseRouter(poService->GetServiceID());
	if (poRouter == NULL)
	{
		poPacket->Release();
		return false;
	}
    poPacket->AppendInnerHeader(INNER_HEADER(uCmd, oNavi.uSrcServer, oNavi.nSrcService, oNavi.uTarServer, oNavi.nTarService, 1), &oNavi.nTarSession, 1);
    if (!poService->GetInnerNet()->SendPacket(poRouter->nSession, poPacket))
    {
		poPacket->Release();
		return false;
    }
	return true;
}

bool NetAdapter::BroadcastExter(uint16_t uCmd, Packet* poPacket, Array<SERVICE_NAVI>& oNaviList)
{
    assert(poPacket != NULL && oNaviList.Size() > 0);
	Service* poService = g_poContext->GetService();
	ROUTER* poRouter = g_poContext->GetRouterMgr()->ChooseRouter(poService->GetServiceID());
	if (poService == NULL || poRouter == NULL)
	{
		poPacket->Release();
		return false;
	}

	for (int i = oNaviList.Size() - 1; i >= 0; --i)
	{
		SERVICE_NAVI& oNavi = oNaviList[i];
		oNavi.nTarService = oNavi.nTarSession >> SERVICE_SHIFT;

		int nKey = (int)oNavi.uTarServer << 16 | oNavi.nTarService;
		BROADCAST_HEADER& oBCHeader = oBCHeaderMap[nKey]; //没有会自动生成一个
		if (oBCHeader.oSessionList.Size() == 0)
		{
			oBCHeader.oInnerHeader.uCmd = uCmd;
			oBCHeader.oInnerHeader.uSrcServer = oNavi.uSrcServer;
			oBCHeader.oInnerHeader.nSrcService = oNavi.nSrcService;
			oBCHeader.oInnerHeader.uTarServer = oNavi.uTarServer;
			oBCHeader.oInnerHeader.nTarService = oNavi.nTarService;
			oBCHeader.oInnerHeader.uSessionNum = 0;
		}
		oBCHeader.oSessionList.PushBack(oNavi.nTarSession);
	}

	BCHeaderIter iter = oBCHeaderMap.begin();
	BCHeaderIter iterend = oBCHeaderMap.end();
	BCHeaderIter itertmp = iter;
	BCHeaderIter iternext = ++itertmp;
	for (; iter != iterend; iter++, iternext++)
	{
		BROADCAST_HEADER& oBCHeader = iter->second;
		if (oBCHeader.oSessionList.Size() == 0)
			continue;

		oBCHeader.oInnerHeader.uSessionNum = oBCHeader.oSessionList.Size();

		Packet* poNewPacket = NULL;
		if (iternext == iterend)
			poNewPacket = poPacket;
		else
			poNewPacket = poPacket->DeepCopy();
		poNewPacket->AppendInnerHeader(oBCHeader.oInnerHeader, oBCHeader.oSessionList.Ptr(), oBCHeader.oSessionList.Size());

		if (!poService->GetInnerNet()->SendPacket(poRouter->nSession, poNewPacket))
			poNewPacket->Release();

		oBCHeader.oSessionList.Clear();
		if (iternext == iterend)
			break;
	}
	return true;
}

bool NetAdapter::BroadcastInner(uint16_t uCmd, Packet* poPacket, Array<SERVICE_NAVI>& oNaviList)
{
	assert(poPacket != NULL);
	Service* poService = g_poContext->GetService();
	ROUTER* poRouter = g_poContext->GetRouterMgr()->ChooseRouter(poService->GetServiceID());
	if (poService == NULL || poRouter == NULL)
	{
		poPacket->Release();
		return false;
	}
	for (int i = oNaviList.Size() - 1; i >= 0; --i)
	{
		Packet* poNewPacket = NULL;
		if (i == 0)
			poNewPacket = poPacket;
		else
			poNewPacket = poPacket->DeepCopy();
		poNewPacket->AppendInnerHeader(INNER_HEADER(uCmd, oNaviList[i].uSrcServer, oNaviList[i].nSrcService, oNaviList[i].uTarServer, oNaviList[i].nTarService, 1), &(oNaviList[i].nTarSession), 1);
		if (!poService->GetInnerNet()->SendPacket(poRouter->nSession, poNewPacket))
		{
			poNewPacket->Release();
		}
	}
	return true;
}
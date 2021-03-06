﻿#include "Server/RouterServer/Router.h"
#include "Include/Network/Network.hpp"
#include "Common/DataStruct/XTime.h"
#include "Server/Base/ServerContext.h"
#include "Server/RouterServer/LuaSupport/LuaExport.h"
#include "Server/RouterServer/PacketProc/RouterPacketHanderl.h"
#include "Server/RouterServer/PacketProc/RouterPacketProc.h"

ServerContext* g_poContext;

bool InitNetwork(int8_t nServiceID)
{
	g_poContext->LoadServerConfig();

	ServerNode* poServer = NULL;
	ServerConfig& oSrvConf = g_poContext->GetServerConfig();
	for (int i = 0; i < oSrvConf.oRouterList.size(); i++)
	{
		if (oSrvConf.oRouterList[i].oRouter.uService == nServiceID)
		{
			poServer = &oSrvConf.oRouterList[i];
			break;
		}
	}
	if (poServer == NULL)
	{
		XLog(LEVEL_ERROR, "RouterServer conf:%d not found\n", nServiceID);
		return false;
	}

	Router* poRouter = (Router*)(g_poContext->GetService());
	return poRouter->Init(nServiceID, poServer->oRouter.sIP, poServer->oRouter.uPort);
}

void StartScriptEngine()
{

}

int main(int nArg, char* pArgv[])
{
	assert(nArg >= 2);
	int8_t nServiceID = (int8_t)atoi(pArgv[1]);

#ifdef _WIN32
	::SetUnhandledExceptionFilter(Platform::MyUnhandledFilter);
#endif

	Logger::Instance()->Init();
	NetAPI::StartupNetwork();
	g_poContext = XNEW(ServerContext);

	LuaWrapper* poLuaWrapper = LuaWrapper::Instance();
	poLuaWrapper->Init(Platform::FileExist("./debug.txt"));
	char szWorkDir[256] = {0};
	char szScriptPath[512] = {0};
	Platform::GetWorkDir(szWorkDir, sizeof(szWorkDir)-1);
	sprintf(szScriptPath, ";%s/Script/?.lua;%s/../Script/?.lua;", szWorkDir, szWorkDir);
	poLuaWrapper->AddSearchPath(szScriptPath);

	Router* poService = XNEW(Router);
	g_poContext->SetService(poService);

	RouterPacketHandler* poPacketHandler = XNEW(RouterPacketHandler);
	g_poContext->SetPacketHandler(poPacketHandler);

	NSPacketProc::RegisterPacketProc();

	OpenLuaExport();

	bool bRes = InitNetwork(nServiceID);
	assert(bRes);

	if (!Platform::FileExist("./debug.txt"))
	{
		char sLogName[256] = "";
		sprintf(sLogName, "routerserver%d", g_poContext->GetService()->GetServiceID());
		Logger::Instance()->SetLogName(sLogName);
	}

	printf("RouterServer start successful\n");
	bRes = poService->Start();
	assert(bRes);
	return 0;
}


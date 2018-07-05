﻿#include "SceneMgr.h"
#include "Common/DataStruct/XTime.h"
#include "Common/DataStruct/ObjID.h"
#include "Server/Base/ServerContext.h"
#include "Server/LogicServer/ConfMgr/ConfMgr.h"
#include "Server/LogicServer/Object/Role/RoleMgr.h"
#include "Server/LogicServer/Object/Monster/MonsterMgr.h"
#include "Server/LogicServer/LogicServer.h"


LUNAR_IMPLEMENT_CLASS(SceneMgr)
{
	LUNAR_DECLARE_METHOD(SceneMgr, CreateDup),
	LUNAR_DECLARE_METHOD(SceneMgr, RemoveDup),
	LUNAR_DECLARE_METHOD(SceneMgr, GetDup),
	LUNAR_DECLARE_METHOD(SceneMgr, SetFollow),
	{0, 0}
};

SceneMgr::SceneMgr()
{
}


SceneMgr::~SceneMgr()
{
}


uint32_t SceneMgr::GenSceneMixID(uint16_t uConfID)
{
	static uint16_t uIndex = 0;
	uIndex = uIndex % 0xFFFF + 1;
	uint32_t nSceneIndex = (uint32_t)uIndex << 16 | uConfID;
	return nSceneIndex;
}

Scene* SceneMgr::GetScene(uint32_t uSceneIndex)
{
	SceneIter iter = m_oSceneMap.find(uSceneIndex);
	if (iter != m_oSceneMap.end())
	{
		return iter->second;
	}
	return NULL;
}

void SceneMgr::RemoveScene(uint32_t uSceneIndex)
{
	SceneIter iter = m_oSceneMap.find(uSceneIndex);
	if (iter != m_oSceneMap.end())
	{
		SAFE_DELETE(iter->second);
		m_oSceneMap.erase(iter);
	}
}

void SceneMgr::Update(int64_t nNowMS)
{
	static int nLastUpdateTime = 0;
	if (nLastUpdateTime == (int)time(0))
	{
		return;
	}
	nLastUpdateTime = (int)time(0);

	SceneIter iter = m_oSceneMap.begin();
	SceneIter iter_end = m_oSceneMap.end();
	for (; iter != iter_end; )
	{
		Scene* poScene = iter->second;
		if (poScene->IsTime2Collect(nNowMS))
		{
			iter = m_oSceneMap.erase(iter);
			uint32_t uSceneIndex = poScene->GetSceneMixID();
			LuaWrapper::Instance()->FastCallLuaRef<void>("OnDupCollected", 0, "I", uSceneIndex);
			SAFE_DELETE(poScene);
			continue;
		}
		else
		{
			poScene->Update(nNowMS);
		}
		iter++;
	}
}

void SceneMgr::LogicToScreen(int nLogicX, int nLogicY, int &nScreenX, int &nSreenY)
{
	nScreenX = nLogicX * gnUnitWidth + (nLogicY & 1) * (gnUnitWidth / 2);
	nSreenY = nLogicY * (gnUnitHeight / 2);
}

void SceneMgr::ScreenToLogic(int nScreenX, int nSreenY, int &nLogicX, int &nLogicY)
{
	nLogicY = nSreenY * 2 / gnUnitHeight;
	nLogicX = (nScreenX - (nLogicY & 1) * (gnUnitWidth / 2)) / gnUnitWidth;
}




/////////////////lua export/////////////////
void RegClassScene()
{
	REG_CLASS(SceneMgr, false, NULL); 
	REG_CLASS(Scene, false, NULL); 
}

int SceneMgr::CreateDup(lua_State* pState)
{
	int nDupType = (int)luaL_checkinteger(pState, 1); 
	lua_assert(nDupType == 1 || nDupType == 2);
	int nDupID = (int)luaL_checkinteger(pState, 2); 
	lua_assert(nDupID > 0 && nDupID <= 0xFFFF);
	int nMapID = (int)luaL_checkinteger(pState, 3);
	bool bCanCollected = lua_toboolean(pState, 4) != 0;
	MapConf* poMapConf = ConfMgr::Instance()->GetMapMgr()->GetConf(nMapID);
	if (poMapConf == NULL)
	{
		return LuaWrapper::luaM_error(pState, "Dup:%d map:%d not found!\n", nDupID, nMapID);
	}
	uint32_t uSceneMixID = nDupID;
	if (nDupType == 2) //1:城镇; 2副本
	{
		uSceneMixID = GenSceneMixID(nDupID);
	}
	if (GetScene(uSceneMixID) != NULL)
	{
		return LuaWrapper::luaM_error(pState, "Dup:%d id:%d conflict!!!\n", nDupID, uSceneMixID);
	}
	Scene* poScene = XNEW(Scene)(this, uSceneMixID, poMapConf, bCanCollected);
	if (!poScene->InitAOI(poMapConf->nPixelWidth, poMapConf->nPixelHeight))
	{
		SAFE_DELETE(poScene);  
		return LuaWrapper::luaM_error(pState, "Dup:%d init AOI error\n", nDupID);
	}
	m_oSceneMap[uSceneMixID] = poScene;
	lua_pushinteger(pState, uSceneMixID);
	Lunar<Scene>::push(pState, poScene);
	return 2;
}

int SceneMgr::RemoveDup(lua_State* pState)
{
	uint32_t uSceneIndex = (uint32_t)luaL_checkinteger(pState, 1);
	RemoveScene(uSceneIndex);
	return 0;
}

int SceneMgr::GetDup(lua_State* pState)
{
	uint32_t uSceneMixID = (uint32_t)luaL_checkinteger(pState, 1);
	Scene* poScene = GetScene(uSceneMixID);
	if (poScene != NULL)
	{
		Lunar<Scene>::push(pState, poScene);
	}
	else
	{
		lua_pushnil(pState);
	}
	return 1;
}

int SceneMgr::SetFollow(lua_State* pState)
{
	int64_t nObjMixID = luaL_checkinteger(pState, 1);
	if (!lua_istable(pState, 2))
		return LuaWrapper::luaM_error(pState, "param2 must be table");

	//目前只支持怪物和人物
	FOLLOW oFollowTarget(nObjMixID);
	if (!(oFollowTarget.nObjType == eOT_Role || oFollowTarget.nObjType == eOT_Monster))
	{
		return LuaWrapper::luaM_error(pState, "object type error");
	}

	LogicServer* poLogicServer = (LogicServer*)g_poContext->GetService();
	RoleMgr* poRoleMgr = poLogicServer->GetRoleMgr();
	MonsterMgr* poMonsterMgr = poLogicServer->GetMonsterMgr();

	Object* poTarObj = NULL;
	if (oFollowTarget.nObjType == eOT_Role)
		poTarObj = poRoleMgr->GetRoleByID(oFollowTarget.nObjID);
	else
		poTarObj = poMonsterMgr->GetMonsterByID(oFollowTarget.nObjID);

	if (poTarObj != NULL)
		poTarObj->SetFollowTarget(0);
	else
		XLog(LEVEL_INFO, "follow target object not exist type:%d id:%d\n", oFollowTarget.nObjType, oFollowTarget.nObjID);

	//清理旧的跟随者
	std::unordered_map<int64_t, bool> oClearFollowMap;
	Follow::FollowVec* poFollowVec = m_oFollow.GetFollowList(oFollowTarget.nObjType, oFollowTarget.nObjID);
	if (poFollowVec != NULL)
	{
		for (int i = 0; i < poFollowVec->size(); i++)
		{
			FOLLOW& oFollow = (*poFollowVec)[i];

			Object* poFollowObj = NULL;
			if (oFollow.nObjType == eOT_Role)
				poFollowObj = poRoleMgr->GetRoleByID(oFollow.nObjID);
			else
				poFollowObj = poMonsterMgr->GetMonsterByID(oFollow.nObjID);
			if (poFollowObj == NULL) continue;

			poFollowObj->SetFollowTarget(0);
			oClearFollowMap[oFollow.ToInt64()] = 1;
		}
		poFollowVec->clear();
	}

	//设置新的跟随者
	int nTableLen = (int)lua_rawlen(pState, 2);
	if (nTableLen > 0)
	{
		if (poFollowVec == NULL)
			poFollowVec = m_oFollow.CreateFollowList(oFollowTarget.nObjType, oFollowTarget.nObjID);

		for (int i = 0; i < nTableLen; i++)
		{
			lua_rawgeti(pState, 2, i + 1);
			int64_t nTmpObjID = lua_tointeger(pState, -1);
			if (nTmpObjID == nObjMixID)
			{
				XLog(LEVEL_ERROR, "can not follow oneself type:%d id:%d\n", oFollowTarget.nObjType, oFollowTarget.nObjID);
				continue;
			}

			FOLLOW oFollow(nTmpObjID);
			Object* poFollowObj = NULL;
			if (oFollow.nObjType == eOT_Role)
				poFollowObj = poRoleMgr->GetRoleByID(oFollow.nObjID);
			else
				poFollowObj = poMonsterMgr->GetMonsterByID(oFollow.nObjID);
			if (poFollowObj == NULL) continue;

			poFollowVec->push_back(oFollow);
			poFollowObj->SetFollowTarget(oFollow.ToInt64());
			oClearFollowMap.erase(oFollow.ToInt64());
		}
	} 
	else
	{
		m_oFollow.RemoveateFollowList(oFollowTarget.nObjType, oFollowTarget.nObjID);
	}

	//脱离跟随的角色同步坐标
	while (oClearFollowMap.size() > 0)
	{
		std::unordered_map<int64_t, bool>::iterator iter = oClearFollowMap.begin();

		FOLLOW oFollow(iter->first);
		oClearFollowMap.erase(iter);

		Object* poFollowObj = NULL;
		if (oFollow.nObjType == eOT_Role)
			poFollowObj = poRoleMgr->GetRoleByID(oFollow.nObjID);
		else
			poFollowObj = poMonsterMgr->GetMonsterByID(oFollow.nObjID);

		if (poFollowObj == NULL) continue;
		poFollowObj->BroadcastPos(true);
	}
	return 0;
}

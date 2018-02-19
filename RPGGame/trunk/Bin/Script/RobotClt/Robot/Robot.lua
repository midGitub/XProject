function CRobot:Ctor(nSessionID, sRobotName)
    self.m_nLastKeepAlive = os.time()
	self.m_nSessionID = nSessionID
    self.m_sName = sRobotName
    self.m_nAccountID = 0
    self.m_nRoleID = 0

    self.m_bLogged = false
    self.m_nLoginTime = 0

    self.m_bStartRun = false
    self.m_bEnterScene = false

    self.m_nLastMsgTime = 0
    self.m_nMsgInterval = math.random(3, 9)

    self.m_nStopRunTime = 0
    self.m_nStartRunTime = 0
end

function CRobot:GetSession() return self.m_nSessionID end
function CRobot:GetName() return self.m_sName end
function CRobot:IsLogged() return self.m_bLogged end

function CRobot:PacketID()
    local oCppRobot = goCppRobotMgr:GetRobot(self.m_nSessionID)
    if oCppRobot then
        return oCppRobot:PacketID()
    end
end

function CRobot:GetPos()
    local oCppRobot = goCppRobotMgr:GetRobot(self.m_nSessionID)
    if oCppRobot then
        return oCppRobot:GetPos()
    end
    return 0, 0
end

function CRobot:Release()
    self.m_bLogged = false
    goTimerMgr:Clear(self.m_nTimer)
    self.m_nTimer = nil
end

function CRobot:Update()
    self:KeepAlive()
    self:RandomMsg()
end

function CRobot:CheckRun()
    if not self.m_bLogged or not self.m_bEnterScene then
        return
    end
    local oCppRobot = goCppRobotMgr:GetRobot(self.m_nSessionID)
    if not oCppRobot then
        return
    end

    if not self.m_bStartRun then
        return oCppRobot:StopRun()
    end

    local nTimeNow = os.time()
    if nTimeNow - self.m_nStartRunTime > 6 then
        self.m_nStartRunTime = nTimeNow
        local nDir = math.random(0, 9)
        oCppRobot:StartRun(nDir)
    end
end

function CRobot:KeepAlive()
    local nTimeNow = os.time()
    if nTimeNow - self.m_nLastKeepAlive >= 10 then
        self.m_nLastKeepAlive = nTimeNow
        CmdNet.Clt2Srv("KeepAlive", self:PacketID(), self.m_nSessionID, nTimeNow)
    end
end

function CRobot:RoleListReq()
    self.m_nLoginTime = os.clock()
    local nSource = 0
    CmdNet.PBClt2Srv("RoleListReq", self:PacketID(), self.m_nSessionID, {nSource=0, sAccount=self.m_sName})
end

function CRobot:OnRoleListRet(tData)
    local nSource = 0
    if #tData.tList > 0 then
        print("登录角色请求") 
        local tRole = tData.tList[math.random(#tData.tList)]
        CmdNet.PBClt2Srv("RoleLoginReq", self:PacketID(), self.m_nSessionID, {nAccountID=tData.nAccountID, nRoleID=tRole.nID})
    else
        print("创建角色请求") 
        local nConfID = math.random(#ctRoleInitConf)
        CmdNet.PBClt2Srv("RoleCreateReq", self:PacketID(), self.m_nSessionID, {nAccountID=tData.nAccountID, nConfID=nConfID, sName=self.m_sName})
    end
end

function CRobot:OnLoginRet(tData)
    self.m_nAccountID = tData.nAccountID
    self.m_nRoleID = tData.nRoleID

    goRobotMgr:OnLoginSuccess(self.m_sName, os.clock()-self.m_nLoginTime)
    self.m_nTimer = goTimerMgr:Interval(1, function() self:Update() end)
    self.m_bLogged = true
    self.m_nLoginTime = os.time()
    self.m_nLastMsgTime = 0

end

function CRobot:OnEnterScene(tData)
    -- local oCppRobot = goCppRobotMgr:GetRobot(self.m_nSessionID)
    -- local nPosX, nPosY = tData.nPosX, tData.nPosY 
    -- local tSceneConf = ctSceneConf[tData.nSceneID]
    -- oCppRobot:SetMapID(tSceneConf.nMapID)
    -- oCppRobot:SetName(self.m_sName)
    -- oCppRobot:SetPos(nPosX, nPosY)
    -- self.m_bEnterScene = true
    -- print(self.m_sName, " enter scene")
end

function CRobot:StartRun()
    self.m_bStartRun = not self.m_bStartRun
end

function CRobot:RandomMsg()
    if os.time() - self.m_nLastMsgTime < self.m_nMsgInterval then
        return
    end
    self.m_nLastMsgTime = os.time()
end


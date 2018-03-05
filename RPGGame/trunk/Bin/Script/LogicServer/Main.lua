math.randomseed(os.clock()*1000)
math.random() math.random() math.random()
cjson.encode_sparse_array(true, 1, 1) --稀疏表转换成对象
cjson_raw.encode_sparse_array(true, 1, 1) --稀疏表转换成对象

--打开协议
local function OpenProto()
    local f = io.open("protopath.txt", "r")
    if not f then
        require("../../Data/Protobuf/LoadPBCProto")
        LoadProto("../../Data/Protobuf")
        return
    else
        local sLoaderPath = f:read("l")
        local sProtoPath = f:read("l")
        f:close()
        require(sLoaderPath)
        LoadProto(sProtoPath)
        return
    end
end

--通用脚本
require = gfRawRequire or require  --恢复原生require
require("ServerConf")
require("Config/Main")
require("Common/CommonInc")
OpenProto()

--逻辑服
gfRawRequire = require  --hook require
require = function(sScript)
	gfRawRequire("LogicServer/"..sScript)
end
require("MainRpc")
require("Global/GlobalInc")
require("Player/PlayerInc")
require("Module/ModuleInc")
require("Dup/DupInc")

--全局初始化
local function _InitGlobal()
    goServerMgr:Init()
    goRemoteCall:Init()
    goDBMgr:Init()
    goDupMgr:Init()

end

--全局反初始化
local function _UninitGlobal()
    local bSuccess = true
    local function fnError(sErr)
        bSuccess=false
        LuaTrace(sErr, debug.traceback())
    end

    xpcall(function() goRemoteCall:OnRelease() end, fnError)
    return bSuccess
end

--GC
local nGCIndex = 0
local nGCTime = 10
local function _LuaGC()
    local nClock = os.clock() 
    if nGCIndex % 6 == 0 then
        collectgarbage()
    else
        collectgarbage("step", 256) --k
    end
    if nGCIndex % 60 == 0 then --10分钟打印1次
        local sCostTime = string.format("%.4f", os.clock() - nClock)
        local nLuaMemery = math.floor((collectgarbage("count")/1024))
        LuaTrace("Lua memory: "..nLuaMemery.."M time:"..sCostTime.." index:"..nGCIndex)
    end
    nGCIndex = nGCIndex + 1
end

--主函数
gnGCTimer = gnGCTimer
function Main()
    _InitGlobal()
    collectgarbage()
    collectgarbage("setpause", 100) --开启新的循环前不等待
    collectgarbage("setstepmul", 300) --内存分配速度的3倍
    gnGCTimer = goTimerMgr:Interval(nGCTime, function() _LuaGC() end)

    --加载非法字
    for _, tConf in pairs(ctKeywordConf) do
        GlobalExport.AddWord(tConf.sKey)
    end
    print("启动 LogicServer 完成******")
end

--准备退出进程
function OnExitServer()
    LuaTrace("OnExitServer start***")
    goTimerMgr:Clear(gnGCTimer)
    local bSuccess = _UninitGlobal()
    assert(bSuccess, "注意！！！关服报错了！！！")
    if goTimerMgr:TimerCount() > 0 then
        assert(false, "！！！计时器泄漏！！！剩余:"..goTimerMgr:TimerCount())
    end
    LuaTrace("OnExitServer finish***")
    os.exit()
end

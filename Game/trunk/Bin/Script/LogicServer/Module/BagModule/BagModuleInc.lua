CItemBase = CItemBase or class()
CArmItem = CArmItem or class(CItemBase)
CPropItem = CPropItem or class(CItemBase)
CBagModule = CBagModule or class(CModuleBase)

require("Module/BagModule/ItemBase")
require("Module/BagModule/ArmDef")
require("Module/BagModule/ArmItem")
require("Module/BagModule/ArmBag")
require("Module/BagModule/PropDef")
require("Module/BagModule/PropItem")
require("Module/BagModule/BagModule")
require("Module/BagModule/BagModuleRpc")
require("Module/BagModule/ItemUse")


# 第三阶段 D：独立竞技场与靶场

## 地图与入口

- `/Game/FPSDemo/Maps/LV_FPSFoundry`：48 × 32 米的模块化竞技场，双侧红蓝基地、出生遮挡墙、中央交战区、两条侧路和货箱/低掩体。每队五个出生点，两侧各六个武器台；大厅单机和创建房间进入此图。
- `/Game/FPSDemo/Maps/LV_FPSTraining`：60 × 24 米的独立靶场。一个玩家出生点，三个人机练习目标，六个武器台；大厅靶场入口进入此图。目标不反击，击倒后重生，无正常对局倒计时。直接打开该地图且不带 Mode 参数时，也默认使用 Training 规则。

靶场以 X=-1900 的射击线为纵向距离基准，三条平行练习线的目标距离分别为 10、20、30 米；玩家应在对应练习线的射击线上读距离，斜向射击的实际距离会更长。武器台位于身后，可走近按 F 拾取。

两张图均采用原创布局、引擎基础几何与自制纯色材质，可在关卡编辑器直接编辑各物件。没有替换原 `/Game/Maps/LV_NetSyncFPS` 教学地图，也没有修改其二进制资产。此批为低多边形 Demo 场景，未包含精细贴图、环境音和破坏系统。

## 维护方式

`Tools/create_fps_maps.py` 通过 Unreal Editor Python 创建关卡、材质、灯光、标牌及标记 Actor；若目标地图已存在会停止，避免覆盖人工修改。`Tools/fix_fps_map_rotations.py` 是本轮针对这两张图的朝向修复脚本。Python Rotator 使用显式 pitch/yaw/roll 参数，避免位置参数顺序造成倒置。

运行时 `FPSMatchFlow` 优先使用以下 Actor Tags：

- PlayerStart：`FPSBlue0`–`FPSBlue4`、`FPSRed0`–`FPSRed4`。
- 靶场 PlayerStart：`FPSTrainingPlayer`、`FPSTarget0`–`FPSTarget2`。
- TargetPoint：`FPSWeapon1`–`FPSWeapon6`，依次对应 AK、M4、MP5、UMP45、M9、Heavy Pistol。

改动标记位置即可调整出生点与武器摆放。武器 Actor 只由服务器生成；没有这些标记的旧教学场景保留原坐标回退。地图及材质目录已加入 Cook 配置。

## 检查与边界

`Tools/TestFPSMaps.ps1` 以真实渲染分别启动单机九个人机和单人靶场，验证出生点数量、胶囊是否与障碍重叠、地面、出生方向、角色数量及武器数量，并截图到 `Saved/Screenshots/WindowsEditor/FoundryMap.png` / `TrainingMap.png`。

`Tools/TestFPSFlow.ps1 -Mode LAN` 改在新竞技场检查双端伤害、GAS 得分、重生和结算同步；`-Mode Training` 使用新靶场。`TestFPSWeapons.ps1` 同样改用新竞技场。所有特殊测试只在显式开发参数下运行。

人机已增加适配本图的平面栅格 A* 寻路与胶囊净空检查，详见 [FPSBots.md](FPSBots.md)。地图扩大、地面高度变化或增加楼层时需要同步调整导航方案。未完成十个真实网络客户端压力测试、完整五分钟自然结算及打包版本检查。

2026-09-28 验证记录：

- 编译通过（最终模块 `9285`）；地图创建和旋转修复脚本均由 Unreal 命令工具成功执行。
- 两张地图实际渲染启动检查通过：竞技场 10 个出生点 / 12 把武器 / 10 个角色；靶场 4 个出生点 / 6 把武器 / 4 个角色。出生点地面、胶囊空间与朝向检查通过。
- 检查游戏截图时发现 Python 位置参数导致标牌和目标倒置，已改为命名旋转参数并复拍确认正常。
- 新竞技场真实 LAN 双端击倒、GAS 得分、重生、最终结算快照通过。
- 新靶场击倒、GAS 得分与重生通过。旧测试将选定目标移到固定射击通道时，另一个练习目标刚好位于通道内；测试已将旁观目标移开，正常游戏仍按第一个遮挡物判定命中。
- 新竞技场双端武器检查全部通过。疾跑测试改在南侧空旷通道进行，避免北侧新增掩体造成速度降为零；验证的仍是客户端 SavedMove 到服务器的实际疾跑同步。

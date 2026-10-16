# 可走动的个人工厂初版

- 用途：实施与验证记录
- 适用范围：单人个人工厂地图、第一人称角色、设施交互和行动返回
- 状态：初版地图已交付；2026-10-16 追加第一人称、行动配装、禁火和鼠标捕获修复，验证见下文
- 最后更新：2026-10-16

## 已确认范围

用户要求工厂地图初版，并选择“可走动的小型基地”。以 28×24 米单层整备车间验证空间与动线，四个设施围绕中央通道布置：仓库与整备、加工制造、枪械改装、出征整备。靠近且朝向设施后才能打开对应现有页面。美术延续工业灰黑、金属结构和有限黄色引导，复用当前共享模块；没有新增设施等级、种植经济或机器人战斗能力。

地图归正式玩法用途，存于 `/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase`；初版和白盒性质不改变地图用途分类，也不表示已完成视觉验收。[源目录](../../../ArtSource/FPSDemo/Environment/FactoryBase/README.md)保存可重建脚本、依赖与操作入口。

## 初版接入与边界（历史）

`FPSFactoryBase` 新增基地模式、机器人、设施标记、控制器、HUD 与终端外层。基地模式继承实验模式以维持既有库存权限检查，跳过对战出生和计时；独立行走角色使用标准 CharacterMovement 与既有机器人分件。移动、转向灵敏度、反转和交互读取系统设置；基地初版只提供走动，不复制战斗移动的疾跑、蹲伏和翻越。

设施包装现有 `WBP_ExtractionTerminal`，仓库、加工、改枪和出征均调用原入口。打开时停止行走并锁定移动/视角；Tab 或返回按钮关闭后恢复。设置界面继续使用原系统设置实现。工厂本身不修改主界面或仓库的布局。

`FPSExtractionSubsystem::Travel` 增加会话内的工厂返回分支：只有进入新工厂入口后才启用；原入口继续原有路径。工厂出征仍进入工业关隘，原撤离/死亡与存档规则保持一致。没有为工厂引入第二份库存或新存档格式。

## 第一人称与鼠标捕获修复（2026-10-16）

本节为第一人称修复时的验证快照。同日后续已按用户要求取消回收行动地图的安全区，见 [后续安全区记录](../Extraction/Implementation.md#取消行动地图安全区2026-10-16)；下方“移出原安全区后射击”仅描述当时的测试步骤。

用户要求车间继续使用第一人称、不显示 HUD，装备遵循行动配装，并明确工厂内不能开火。车间现复用 `BP_FPS_Character` 的第一人称相机和输入，原 `AFPSFactoryRobot` 仅用于大厅展示。大厅关闭后回到玩家第一人称视角；车间无生命、弹药、准星与设施提示 HUD，靠近设施后仍可按交互键打开页面，也可用 Tab 打开大厅。

工厂在创建角色前准备武器目录和存档，并将 M4/M9 注册为工厂地图依赖，避免无武器架地图的初始化清空目录；角色出现或存档版本变化后，恢复既有主副武器实例、配件与弹药，沿用行动配装来源。工厂全区域由 `UFPSActionPolicy::CanAttack` 拒绝攻击，包含直接武器请求和 GAS 开火能力；正常行动地图仍允许射击。未新增存档格式或第二套工厂配装。

回收行动原先仅在面板开关变化时更新输入；从大厅换图时，新控制器默认面板关闭，可能跳过初始化并保留视口的菜单捕获状态。现在首次 Tick 必定设置输入，游戏时恢复永久捕获与捕获锁定；工厂／背包／结算／设置页面使用 LockAlways 限制光标在窗口内，关闭菜单恢复游戏输入。设置和背包状态切换还清空旧的输入忽略计数，避免叠加残留。

Editor Development 编译和构建后武器数据检查通过，日志为 `Saved/FactoryFirstPerson/Build.log`，数据检查为 `FPS_WEAPON_DATA PASS definitions=7`。模块 SHA-256：`DD86A11ABCCFFE7924350AAF000937D4AB1688E0593DB672CC887ED519A5CB49`。

`Tools/TestFPSStartMenu.ps1 -Width 1280 -Height 800` 使用冻结模块副本、独立存档和设置运行，记录为 `Saved/StartMenu/Tests/20261016-182521257/run.json`。81 条阶段检查与 `PASS COMPLETE` 通过，覆盖：

- 第一人称相机、行走、无 HUD；正常装配服务安装配件后，持枪实例、配件模型与弹药对应行动配装。
- 工厂拒绝武器开火和直接 GAS 开火，持续扳机不耗弹；回收行动移出原安全区后实际射击耗弹；结算返回后恢复行动配装且仍禁火。
- 初次进入行动、关闭背包、关闭设置、返回车间，均检查永久捕获配置及 `FViewport::HasMouseCapture()` 的实际 Slate 捕获归属；工厂、背包、结算和设置菜单均检查窗口锁定。
- 原开始页、设置、多人对战子页、靶场、单机训练、Listen Server 等待室与返回入口继续通过；没有第二客户端参与。

同一模块的 `Tools/TestFPSFactory.ps1` 在 1600×900 通过，记录为 `Saved/FactoryBase/Tests/20261016-182653074/run.json`：实际行走与地面／围墙碰撞、四处设施的距离／朝向判定、交互打开和 Tab 关闭、设置返回、真实行动地图与结算回厂均通过，十张截图齐全。

当前截图：[第一人称工厂](Images/FactoryFirstPerson.png)。已人工查看入厂、战区与返回车间截图；工厂无准星、弹药、生命或设施提示 HUD。工厂直接不创建 HUD，避免引擎初始化重新启用显示标记。测试不修改用户存档；每轮临时 Runtime、设施测试的两份独立存档及失败轮 24 张过时截图已清理，清单为 `Saved/FactoryFirstPerson/Cleanup.json`。

本轮使用 standalone 和离屏真实渲染，验证权威武器逻辑及本地 Slate 输入状态。测试中移出安全区的位置由夹具设置，结算调用已有权威接口；不声称真人走完整条路线或自然撤离。系统级多显示器拖动／Alt-Tab、硬件鼠标、PIE、双客户端和打包运行未验证。

## 使用与重建

后续大厅接入：`Tools/LaunchFPSExtraction.ps1` 已改为在本地图以 `-FactoryLobby` 打开行动大厅；直接车间入口保持如下。车间 Tab 可打开大厅，大厅“工厂”或终端 Tab 返回车间。大厅画面与验证另见 [大厅实施记录](../UI/LobbyImplementation.md)，下方初版地图测试仍是原日期的历史证据。

- 启动：项目根目录 `./Tools/LaunchFPSFactory.ps1 -Profile Player`；可换 Profile 进行独立试玩。
- 默认操作：WASD 移动、鼠标转向、F 靠近交互、Esc 设置；设施中 Tab 返回车间。
- 地图重建：Unreal Python commandlet 执行 `Tools/create_fps_factory_base.py`。已有地图默认拒绝覆盖；显式 `-FactoryBaseRebuild` 仍需对应生成记录及无手工添加演员。
- 运行检查：`./Tools/TestFPSFactory.ps1`，结果保存在 `Saved/FactoryBase/Tests/`。

## 验证记录

- Editor Development 构建通过：`Saved/FactoryBase/Build/Editor1.log`。
- 首轮 standalone 通过：`Saved/FactoryBase/Tests/20261015-145425891/run.json`，覆盖实际 CharacterMovement 行走、碰撞查询、四处设施打开/关闭、Slate Tab 返回、设置退出和真实出征/结算/返回地图。
- 测试使用独立个人存档；设施接近位置由测试设置，不能代替真人沿所有动线游玩。出征测试通过正式结算接口结束测试行动，不声称该项验证了自然 AI 战斗或玩家撤离过程。
- 已修正首轮过曝、HUD 字号偏小和立柱遮挡标牌。最终记录：`Saved/FactoryBase/Tests/20261015-151641508/run.json`，模块 SHA256 `F6AC11A861A2642B5BE1E1A1BAD5795E8AB977D7D495763F1F4880E9DD69316E`。实际运行与十张采图齐全，进程正常退出；手工查看了行走、仓储、加工、改枪区及设施 UI 截图。
- 最终生成记录 `Saved/FactoryBase/Authoring4.log` 与源目录 `Manifest.json` 核对另外 317 个 `.uasset`/`.umap` 未改动；图中有 189 个静态网格演员，4 处设施和 4 台预览相机。
- 一次独立编辑器采图进程退出码为访问异常，不记为完整成功，交付图片全部替换为最终游戏内采图。一次后续链接因其他 Unreal 实例占用模块失败；共享模块更新后，最终运行直接使用项目当前模块，通过新增多视角采图阶段确认新代码已加载。备用验证副本编译取消并清理，未把它计为验证成功。
- 启动日志仍有既有 Experimental Toolsets Python 错误；生成器有旧编辑器相机 API 的弃用提示，不宣称所有日志无警告或错误。

实机预览：[行走视角](../../../ArtSource/FPSDemo/Environment/FactoryBase/Previews/Walkable.png)、[车间全景](../../../ArtSource/FPSDemo/Environment/FactoryBase/Previews/Workshop.png)、[改枪区](../../../ArtSource/FPSDemo/Environment/FactoryBase/Previews/Gunsmith.png)、[加工区](../../../ArtSource/FPSDemo/Environment/FactoryBase/Previews/Fabrication.png)、[仓储区](../../../ArtSource/FPSDemo/Environment/FactoryBase/Previews/Storage.png)。

本轮未验证编辑器 PIE、完整 Cook/打包、所有设置组合、联网游玩及真人全动线体验。工厂入口只用于单人。

返回 [搜打撤与工厂实验](../Extraction/README.md)。

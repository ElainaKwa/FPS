# 可走动的个人工厂初版

- 用途：实施与验证记录
- 适用范围：单人个人工厂地图、基地机器人、设施交互和出征返回
- 状态：初版地图和设施交互完成，standalone 行走与出征返回通过；PIE 与打包未验证
- 最后更新：2026-10-15

## 已确认范围

用户要求工厂地图初版，并选择“可走动的小型基地”。以 28×24 米单层整备车间验证空间与动线，四个设施围绕中央通道布置：仓库与整备、加工制造、枪械改装、出征整备。靠近且朝向设施后才能打开对应现有页面。美术延续工业灰黑、金属结构和有限黄色引导，复用当前共享模块；没有新增设施等级、种植经济或机器人战斗能力。

地图归正式玩法用途，存于 `/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase`；初版和白盒性质不改变地图用途分类，也不表示已完成视觉验收。[源目录](../../../ArtSource/FPSDemo/Environment/FactoryBase/README.md)保存可重建脚本、依赖与操作入口。

## 接入与边界

`FPSFactoryBase` 新增基地模式、机器人、设施标记、控制器、HUD 与终端外层。基地模式继承实验模式以维持既有库存权限检查，跳过对战出生和计时；独立行走角色使用标准 CharacterMovement 与既有机器人分件。移动、转向灵敏度、反转和交互读取系统设置；基地初版只提供走动，不复制战斗移动的疾跑、蹲伏和翻越。

设施包装现有 `WBP_ExtractionTerminal`，仓库、加工、改枪和出征均调用原入口。打开时停止行走并锁定移动/视角；Tab 或返回按钮关闭后恢复。设置界面继续使用原系统设置实现。工厂本身不修改主界面或仓库的布局。

`FPSExtractionSubsystem::Travel` 增加会话内的工厂返回分支：只有进入新工厂入口后才启用；原入口继续原有路径。工厂出征仍进入工业关隘，原撤离/死亡与存档规则保持一致。没有为工厂引入第二份库存或新存档格式。

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

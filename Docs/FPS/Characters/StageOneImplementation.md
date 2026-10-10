# 角色第一阶段实施与验证

- 用途：实施与验证记录
- 适用范围：角色基础重构、模式资产基础及现有 FPS 地图区域划分
- 状态：第一阶段历史实施及针对性双端回归记录；后续见第二、三阶段记录
- 最后更新：2026-10-06

需求依据为 [设计决策](DesignDecisions.md)，进度入口为 [任务清单](Tasks.md)。本记录只覆盖角色第一阶段，不沿用枪械历史日志作为本轮证据。

后续配装与基地保护已在 [第二阶段记录](StageTwoImplementation.md) 单独记录，物理掉落与清理见 [第三阶段记录](StageThreeImplementation.md)。以下保留第一阶段的版本与结论；`Saved/` 中脚本使用的标准日志名会被后续运行覆盖，应按时间与模块清单区分，当前版本证据以第三阶段保存的快照为准。

## 版本与工作区

基线 HEAD：`a7fdc868f7ab97f8fa454ad43e6c7bd52e2503c5`。交付为工作区增量，未创建提交。开始时已有 AGENTS、Config、大厅/对局、AI、武器、验收脚本及文档改动；保留这些内容，没有恢复 HEAD 或覆盖未跟踪文件。

引擎实际版本：UE **5.8.1 / CL-56057345**；目标 `MCP_TestEditor Win64 Development`；协议 `UFPSWeaponCatalog::ManifestVersion = 3`。最终模块 SHA256：`68e8909e1e6d8bb774a255a263e3fb0da8241947eedf219ac342b801c688d00c`。来源、模式资产、地图和模块的逐文件版本见 [版本清单](../../../Saved/FPSCharacterTests/StageOneManifest.json)，[最终构建日志](../../../Saved/FPSCharacterTests/StageOneBuild.log) 与 [武器构建校验](../../../Saved/Logs/FPSWeaponBuildValidation.log) 可定位本地运行版本。`Saved/` 证据是本机产物，不保证随 Git 分发。

主要玩法/表现回归在北京时间 2026-10-05 22:32–22:34 执行。后续增量构建只增加区域截图测试入口和修正旧动作夹具的生命周期；最终模块于 22:41–22:42 再次通过角色专项、靶场流程和动作复验。2026-10-06 收尾时复核逐文件清单无变化。没有重新打包，不能把本记录当成打包版或专用服务器验收。

## 职责契约

| 职责 | 当前写入方与边界 |
| --- | --- |
| 生命耗尽 | `UFPSDemoAttributeSet::PostGameplayEffectExecute` 在 Health 被 GAS 执行到零后通知目标自身 `UFPSLifeComponent`；伤害来源无需补发淘汰。直接属性测试设置不作为游戏伤害 API。 |
| 一次性死亡 | Life 在广播前提交复制的 `bDead`，防止递归与重复通知；`FFPSDeathContext` 在该时刻快照原始 Instigator、Causer、Controller、PlayerState 和显示来源。 |
| 计分、重生 | `AFPSDemoGameMode::HandleLifeEnded` 消费死亡通知，处理 K/D、回合阶段及 2.5 秒后销毁/创建 Pawn。无攻击者不虚构击杀者，自杀不增加个人击杀。 |
| 身体状态 | Character 的私有死亡处理停止武器动作、禁用移动/胶囊碰撞，复制身体状态、倒计时和来源；ASC 继续留在 Character。 |
| 通用行动 | `UFPSActionPolicy` 检查生命、对局阶段、准入及 Movement/Attack/Drop 限制 Tag。它不依赖武器型号、库存或弹药。 |
| 武器操作 | `UFPSWeaponComponent` 叠加准入内容、动作、ADS、举枪、弹药和实例冷却约束；GAS Ability、预测与权威提交机制保持原入口。 |
| 网络移动 | 原 `UFPSMovementComponent` 的 SavedMove、压缩冲刺标记与网络预测保留。移动权限先消费通用规则；武器仅提供 ADS/操作造成的速度和冲刺限制。空库存仍可移动。 |
| 玩家输入 | `UFPSCharacterInput` 持有映射、绑定及本地上下文注册/移除；调用身体和武器动作。AI 直接调用行为入口，不读取玩家按键。完整可编辑重映射/UI 输入方案留阶段二。 |
| 相机与第一人称 | `UFPSCharacterPresentation` 持有 FOV/ADS/冲刺混合、视觉后坐力和闪光状态，写相机及枪体表现、刷新武器网格；控制朝向的游戏后坐力仍由本地武器提交驱动。 |
| 第三人称 | `UFPSAvatarVisuals` 每帧从定义基准计算绝对枪姿态、身体关节及手部姿态；移除 Character 中的重复枪姿态写入，保留缺失新模型时的旧模型回退。 |
| 教学兼容 | 固定伤害/治疗 RPC 实现隔离在 `FPSTeachingAdapter.cpp`，保留原 RPC 名称和保存的角色身体子对象；教学地图不接入模式配装，额外对战冲刺/蹲伏输入不会改变教学行为。 |

Life 与 ActionPolicy 使用 Actor 上的 ASC/组件契约，不要求持有枪械或继承人形 Character。其他形态需要订阅自身身体死亡反应并使用共享 GAS 属性；本轮没有制作其他形态 AI，也未宣称通用重生身体工厂已实现。

## 模式与区域

两张现有地图各放置一个 `AFPSModeSetup`，显式引用 `/Game/FPSDemo/Modes/DA_TeamMatch` 或 `DA_Training`。默认配装资产 `DA_DefaultSpawnLoadout` 引用 M4/M9；每次出生按定义创建新实例。规则资产记录最终选枪策略、替换策略、死亡不掉落、180 秒寿命和靶场数量 30；对战数量 `-1` 明确表示**待确认**，不是无上限，也没有启用新掉落策略。

Solo/LAN 选项仅控制连接、等待/开局及既有机器人数量；TeamMatch/Training 玩法身份读取地图规则资产。机器人难度伤害规则按对战玩法判定，不再由 Solo 网络选项决定。教学继续使用原独立模式。

依赖收集包括默认配装、全部可选武器以及地图台位；模式武器引用和默认槽位参加校验。握手摘要还覆盖玩法、默认武器 ID、可选顺序、替换/掉落等规则字段，保持武器稳定 ID 与实例模型；不同包路径不作为武器身份。当前两图准入 7 个武器定义。选枪界面及 PlayerState 个人偏好尚未实现。

Foundry 蓝/红区域中心分别为 `(-2050, 0, 150)`、`(2050, 0, 150)`，半尺寸 `(310, 1500, 150)`；每区显式关联原有 5 个 PlayerStart，编号 0 为备用点。区域校验检查两侧阵营唯一性、出生点覆盖/归属、包含关系及备用引用；出生仍按原 Lane 选点。本阶段不随机采样、不做占用等待。靶场保留玩家和三个目标出生位置。

区域边界只有无碰撞地面提示和蓝/红标识 **PROTECTION PENDING**。通行、伤害阻断、持续保护 GE、选枪权限和返回保护均未接入。放置脚本 [setup_fps_character_regions.py](../../../Tools/setup_fps_character_regions.py) 使用 Unreal Editor API 增量保存；已存在资产和区域不会重复覆盖。二进制文件未以文本补丁修改。

## 本轮验证

以下全部使用新运行日志；除渲染检查外使用 `-nullrhi -nosound`。listen server、拥有者客户端及晚加入观察者是真实独立 Unreal 进程，不是直接模拟客户端对象。逻辑夹具注入事件，不能称为自然网络长时测试。

| 检查 | 条件与结果 | 证据 |
| --- | --- | --- |
| 构建/资产 | Development 编译成功；7 个保存武器定义、资源及原生成清单校验通过 | 上述构建日志；[编辑器资产保存](../../../Saved/Logs/FPSCharacterSetup.log) |
| 角色专项 | 0ms/0% 与 100ms/1%；空库存预测冲刺与服务器位移、区域归属、环境 Causer、重复通知只计一次、武器来源归因、新 Pawn/ASC/实例及复制 | [零延迟服务端](../../../Saved/FPSCharacterTests/lag0-loss0/server.log)、[客户端](../../../Saved/FPSCharacterTests/lag0-loss0/client.log)；[延迟服务端](../../../Saved/FPSCharacterTests/lag100-loss1/server.log)、[客户端](../../../Saved/FPSCharacterTests/lag100-loss1/client.log) |
| 教学回归 | 0ms/0%；25 伤害、35 治疗与冷却、淘汰计分、2.5 秒重生及客户端状态全部通过 | [服务端](../../../Saved/FPSFlowTests/Teaching-server.log)、[客户端](../../../Saved/FPSFlowTests/Teaching-client.log) |
| 对战流程 | 0ms/0%；真实射击/友伤拒绝、K/D、击杀提示、重生、结算及回合结束拒绝伤害通过 | [服务端](../../../Saved/FPSFlowTests/LAN-server.log)、[客户端](../../../Saved/FPSFlowTests/LAN-client.log) |
| 靶场流程 | 0ms/0%；实际靶场伤害、目标死亡、重生及结束时拒绝伤害均通过 | [日志](../../../Saved/FPSFlowTests/Training-server.log) |
| GAS 动作 | 0ms/0%；真实 GAS 死亡中断连发、拥有者意图清理，以及换弹/切枪/ADS/实例转移回归均通过 | [服务端](../../../Saved/FPSFlowTests/Abilities-server.log)、[客户端](../../../Saved/FPSFlowTests/Abilities-client.log) |
| 程序表现 | 0ms/0%；ADS/FOV、举枪时长、视觉后坐力、取消换弹、远端姿态、晚加入检视恢复全部通过；实际渲染客户端 | [服务端](../../../Saved/FPSFlowTests/Presentation-server.log)、[客户端](../../../Saved/FPSFlowTests/Presentation-client.log)、[晚加入观察者](../../../Saved/FPSFlowTests/Presentation-observer.log) |
| 地图与视觉 | Foundry 10 starts/14 pickups/10 pawns，Training 4/7/4，出生空间检查均通过；人工查看新截图中的第一人称、目标模型及两侧提示 | [对战](../../../Saved/FPSFlowTests/Map-Solo.log)、[靶场](../../../Saved/FPSFlowTests/Map-Training.log)、[区域采集](../../../Saved/FPSCharacterTests/Regions.log)；[蓝区](../../../Saved/Screenshots/WindowsEditor/CharacterBlueRegion.png)、[红区](../../../Saved/Screenshots/WindowsEditor/CharacterRedRegion.png)、[靶场](../../../Saved/Screenshots/WindowsEditor/TrainingMap.png)、[ADS](../../../Saved/Screenshots/WindowsEditor/WeaponPresentation.png) |
| 内容拒绝 | 客户端容量/射速单端注入不同配置：服务器 `REJECT pawn=0`、客户端收到理由并释放资源，均通过 | [容量服务器](../../../Saved/FPSFlowTests/Content-capacity-server.log)、[容量客户端](../../../Saved/FPSFlowTests/Content-capacity-client.log)、[射速服务器](../../../Saved/FPSFlowTests/Content-interval-server.log)、[射速客户端](../../../Saved/FPSFlowTests/Content-interval-client.log) |

复验入口：`Tools/TestFPSCharacters.ps1`、`Tools/TestFPSWeapons.ps1 -Teaching/-Abilities/-Presentation -Visual`、`Tools/TestFPSFlow.ps1 -Mode LAN/Training`、`Tools/TestFPSMaps.ps1`、`Tools/TestFPSWeaponContent.ps1`、`Tools/CaptureFPSRegions.ps1`。

动作夹具首次失败发生在正常重生之后：新实例重放了旧生命的所有定时步骤，旧体上的真实死亡取消检查已经通过。保存 [修复前服务器日志](../../../Saved/FPSCharacterTests/FixtureRespawnBeforeFix-server.log) 和 [客户端日志](../../../Saved/FPSCharacterTests/FixtureRespawnBeforeFix-client.log)。修正是阻止新生命重放夹具并在重生前提交测试结论，没有改变游戏死亡/重生时间。最终服务端与客户端均为 `FPS_ABILITY_SMOKE PASS ... COMPLETE`，没有失败标记。

文档交付检查：2026-10-06 在项目根运行 `python Tools/CheckDocs.py`，结果为 39 篇文档、0 错误、102 项历史例外。仅删除已修复的教学文档元数据例外，没有新增或扩大豁免；内容状态、证据路径与阶段边界已人工核对。

## 验证边界与后续

这不是 PIE 双窗口人工游玩或新打包版专项验收。未执行专用服务器、新包、十人长时自然网络、音频播放、逐键硬件输入和其他形态角色测试；渲染截图与状态时序检查不代表完整动画审美验收。教学蓝图实际加载/双端行为已测，但输入映射移动到组件后的序列化兼容仍应在其他衍生蓝图上线时单独核对。

阶段二负责实时个人配装、界面输入、首次离区锁定、GAS 保护来源、敌方通行/双向攻击边界、随机出生与重试；阶段三负责权威物理及专项掉落联调，均已有单独实施及实际验证记录。不能用本阶段 Tag 或模式字段证明后续功能。对战上限 30 把、越界直接回收与领取间隔 0.1 秒已在第三阶段由用户确认并落地。

返回 [角色主题](README.md)。

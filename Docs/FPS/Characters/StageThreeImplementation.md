# 物理掉落第三阶段实施与验证

- 用途：实施与验证记录
- 适用范围：主动丢弃、普通拾取、靶场替换、动态掉落清理及已有角色/模式回归
- 状态：2026-10-06 阶段实现及针对性双端回归的历史记录；后续扩展验收见独立记录
- 最后更新：2026-10-10

规则依据为 [设计决策](DesignDecisions.md)，进度入口为 [任务清单](Tasks.md)。延续 [第二阶段](StageTwoImplementation.md) 的配装、安全区和实例事务，不扩展美术、地图尺寸、AI 配装及未来玩法。

本文保留 2026-10-06 模块及当时的验证边界。2026-10-10 的服务器表现组件修复、新包 14 项、实际 Dedicated 十客户端五分钟和连续切图，见 [扩展验收](ExtendedAcceptance.md)；后续结果不改写本文历史版本和日志。

## 版本与范围

使用 UE **5.8.1 / CL-56057345**、`MCP_TestEditor Win64 Development`，准入协议 **6**。基线 HEAD 为 `a7fdc868f7ab97f8fa454ad43e6c7bd52e2503c5`，交付为多人工作区增量，未创建提交。最终 DLL SHA256 为 `543d46a2375cb94dd30da22fcde8cf271bbeccf2db676fa36492574387559b06`，模块于 2026-10-06 **21:16:57** 生成，最终构建于 **21:17** 通过；启动校验包含 7 个定义，命令行报告 0 error / 0 warning。输入专项沿用的旧 API 有 C4996 编译提示，构建成功不代表实际运行完成。

本阶段没有重新制作地图、武器模型或动画。模式资产通过 `UnrealEditor-Cmd` 的 Python 编辑器 API 更新及保存；另起全新编辑器进程读回确认。两种模式的动态掉落上限均为 **30**、寿命 **180 秒**、领取最短间隔 **0.1 秒**。对照第二阶段二进制基线，唯一变化为 `DA_TeamMatch` 的已授权上限；两张地图、Training 及其余基线资产均未变化。

## 实现契约

| 职责 | 实现及边界 |
| --- | --- |
| 实例转移 | 主动丢弃、普通拾取替换和靶场领取替换共用 `UFPSWeaponComponent::SpawnDrop`。完整复制 GUID、型号、弹匣、备弹、NextShotAt、LastShotAt、RecoilIndex 和 ShotSerial。动作取消不补弹，不恢复旧持有者 ADS、换弹或检视。 |
| 原子认领 | `CanPickup` 检查服务器当前距离小于 200cm、无遮挡与可用性；明确目标不要求仍为客户端最近的枪。先 Claim，再准备旧枪掉落，最后提交库存及销毁目标；准备失败 ReleaseClaim，库存不变。所有步骤在服务器游戏线程的一次操作中完成。 |
| 普通拾取 | 替换当前槽或没有手持枪时装备；进入非当前槽时保留现有手持武器。仅手持 GUID 真正改变才激活 Equip。界面领取仍直接装备所选新实例。 |
| 权威物理 | `AFPSWeaponPickup` 用 BoxComponent 作物理根，根据网格及配置变换计算尺寸并居中视觉。服务器开启重力、CCD、阻尼及轻抛速度，使用原生 RepMovement 同步刚体；客户端本地物理用于展示与校正，不能裁定认领、寿命或数量。 |
| 安全释放 | 使用实际网格范围加 2cm 余量，沿角色前方 75cm 做场景 Box sweep 与 overlap 检查，必要时退回释放起点；两处均无空间则拒绝事务。初速为前方 160cm/s、上方 110cm/s，加最高 600cm/s 角色速度的 35%，并施加角速度。 |
| 碰撞 | 掉落根为 PhysicsBody，只阻挡 WorldStatic/WorldDynamic；忽略 Pawn、Visibility 和其他掉落枪。视觉网格无碰撞。角色胶囊和实际子弹查询穿过掉落，不能把它当掩体或踢动。预置武器仍为静态查询对象。 |
| 休眠 | 服务端每 0.1 秒检查物理及边界，静止超过 0.5 秒后 ForceNetUpdate 并进入 DORM_DormantAll；重新唤醒时刷新休眠和同步。晚加入客户端必须收到最终刚体快照。 |
| 回收 | `UFPSDynamicDrops` 按创建顺序管理本回合动态物。超过 30 时销毁最旧、仍可拾取的物；已 Claim 的目标不参与回收，预置武器不注册。180 秒自然 LifeSpan、回合结束、低于 KillZ、非有限位置或刚体范围越过引擎有效边界时销毁并注销。KillZ 检查不依赖地图通用世界边界开关。 |
| 模式与保护 | 对战领取回收旧枪；当前两模式死亡不掉枪；重生不删除旧动态物。安全区主动禁丢与靶场合法领取替换分别消费既有规则；拾丢不写 PlayerState 配装偏好。 |
| 领取限频及内容 | Controller 按玩家检查 0.1 秒间隔，过快拒绝并反馈，库存与偏好不变。模式摘要包含清理配置及间隔；模式默认及所有可选型号仍进入本局内容依赖与准入校验。 |

菜单通过 P/Esc 使用既有交互，footer 和 HUD 告知上限、寿命及最旧回收；界面保持名称、类别图标和点击选择，未制作新美术。

## 专项修复与证据要求

除常规跨持有者冷却检查，还覆盖原玩家持住自动扳机丢出再拾回同 GUID。清理已经离开库存的预测可处理通常复制顺序，但不能单靠中间的空库存快照识别一次转移：该快照可能被属性复制合并或丢包跳过。

确定性双端夹具在服务器同一 tick 调用真实 Drop Ability 和现有 GAS 预测激活 RPC 入口，成功转回原 GUID；客户端注入领先的射击预测。修复前服务器完整实例及唯一 GUID 检查通过，客户端旧预测清理失败，证据保留于 `Saved/FPSDropTests/Attempts/CoalescedPredictionBeforeFix/`。这是复制合并及预测状态注入，不冒充自然 G/F 操作复现。

修复采用库存主、副槽独立的 ownership revision，成功修改对应槽才递增；该修订不属于枪实例，不改变其 GUID、冷却或地面数据。OwnerState 回调按修订变化清理该槽前后 GUID 的预测，避免清除另一槽或普通切枪的预测。最终零延迟和 100ms/1% 双端验证均包含合并快照、未改槽保留、失败不递增和普通切枪不递增。

旧枪械夹具原先把拾到主枪等同于切换主槽，已改成显式切枪；物理枪移动后按其 GUID 的真实位置构造合法拾取。截图耗时、远端角色遮挡和单端完成标记前缀误匹配也已修正测试夹具/脚本，不改变玩法。渲染冷启动时的角色位置前提及低帧时序单独处理：掉落专项明确两端初始位置/朝向、打印距离，掉落和表现专项各使用共同服务器时间的 10 秒预热；保留原事件间隔、合法距离和物理误差断言。晚加入留出冷启动等待，正常掉落专项 78 秒结束；脚本上限为 180 秒，寿命专项 300 秒。地图脚本改为等待本轮实际标记后退出，不再固定 35 秒提前终止。早期尝试保留于 `Saved/FPSDropTests/Attempts/`，不能用于最终模块验收；渲染场景依次执行。

## 实际验证与边界

以下为最终模块的新运行，所有日志时间为 UTC，本节显示北京时间。完整来源与独立证据快照见 [版本及证据清单](../../../Saved/FPSDropTests/StageThreeManifest.json)，包含 223 个输入文件、34 份验证日志和 42 份独立快照，2026-10-06 21:34 生成并逐项核对 SHA256、运行新鲜度及完成标记；`Saved/` 为本机证据，不保证随 Git 分发。不能用前一候选 DLL 的通过日志证明当前模块。

| 检查 | 覆盖及结果 | 独立证据 |
| --- | --- | --- |
| 物理与转移双端 | 21:19 完成：真实场景墙接触、落地休眠、实际 bullet trace/capsule sweep 穿枪、空中 GAS RPC 拾取、完整实例与举枪/原冷却、两人同目标只产生一份库存、非当前槽保留手持 | [服务器](../../../Saved/FPSDropTests/Regressions/Drops-lag0-loss0-visual-observer-server.log)、[拥有者](../../../Saved/FPSDropTests/Regressions/Drops-lag0-loss0-visual-observer-client.log) |
| 事务及清理 | 同一专项：30→31 FIFO、已 Claim 目标不被满额替换回收、安全释放失败回滚、距离/遮挡拒绝、禁用通用边界检查仍执行 KillZ 回收、死亡不掉枪、重生保留旧掉落、回合结束只删动态物 | 同上；预置武器数量前后保持一致 |
| 同 GUID 与预测 | 实际拥有者丢拾 RPC、继续持住自动扳机后的弹药/NextShotAt/ShotSerial 精确收敛；确定性合并快照修复通过，主槽 revision +2、未改副槽预测保留、失败与切枪不递增 | 同上；修复前失败尝试单独保留，不混入 PASS 快照 |
| 复制与晚加入 | 静止位置误差 1.541cm，晚加入 0.042cm，均小于专项 25cm 阈值；新客户端接收休眠物最终刚体快照 | [晚加入](../../../Saved/FPSDropTests/Regressions/Drops-lag0-loss0-visual-observer-observer.log) |
| 延迟及丢包 | 21:24 完成：100ms / 1% 重复完整双端、合并快照和晚加入专项；静止误差 0.945cm，晚加入 0.041cm | [服务器](../../../Saved/FPSDropTests/Regressions/Drops-lag100-loss1-observer-server.log)、[客户端](../../../Saved/FPSDropTests/Regressions/Drops-lag100-loss1-observer-client.log)、[晚加入](../../../Saved/FPSDropTests/Regressions/Drops-lag100-loss1-observer-observer.log) |
| 靶场及界面 | 21:21 完成掉落专项，21:29 配装专项完成；任何位置领取、另一槽立即装备、旧实例动态掉落、P/Esc 真实映射、输入与鼠标、30/180/FIFO footer 人工核对 | [掉落](../../../Saved/FPSDropTests/Regressions/Drops-training-visual-server.log)、[配装](../../../Saved/FPSDropTests/Regressions/Loadout-training-visual-server.log)、[界面](../../../Saved/FPSDropTests/Regressions/DropLoadoutMenu.png) |
| 自然寿命 | 21:31 双端完成真实 185 秒专项；179 秒仍存活，182 秒确认服务器自然回收，183 秒确认客户端销毁，预置物不变；未使用时间缩放 | [服务器](../../../Saved/FPSDropTests/Regressions/Drops-lag0-loss0-lifetime-server.log)、[客户端](../../../Saved/FPSDropTests/Regressions/Drops-lag0-loss0-lifetime-client.log) |
| 既有行为 | 21:22–21:27 新模块回归：配装/基地保护/敌方通行、角色环境死亡、GAS 连发/换弹/ADS/取消、真实对战反馈、原 LV_NetSyncFPS 25 点伤害/35 点治疗/计分/重生、表现及晚加入事件去重均通过 | [本轮回归快照](../../../Saved/FPSDropTests/Regressions) |
| 内容准入与地图 | 21:25 capacity/interval 差异均在 Pawn 出生前拒绝，返回原因及 7 定义/0 句柄释放；21:30 两图渲染通过：Foundry 10 starts/14 pickups/10 pawns，Training 4/7/4，出生空间均有效 | [容量服务端](../../../Saved/FPSDropTests/Regressions/Content-capacity-server.log)、[容量客户端](../../../Saved/FPSDropTests/Regressions/Content-capacity-client.log)、[间隔客户端](../../../Saved/FPSDropTests/Regressions/Content-interval-client.log)、[对战地图](../../../Saved/FPSDropTests/Regressions/Map-Solo.log)、[靶场地图](../../../Saved/FPSDropTests/Regressions/Map-Training.log) |

复验入口为 `Tools/TestFPSDrops.ps1 [-Visual -Observer]`、`-Latency 100 -PacketLoss 1 -Observer`、`-Training -Visual` 与 `-Lifetime`；相关回归见本主题前两阶段记录和版本清单。

本次使用真实独立 Unreal 服务器、拥有者和晚加入客户端；位置、阻挡体、伤害及预测状态由 opt-in 专项注入，动作经过现有 GAS/RPC 路径。实际渲染截图用于人工核对可见掉落、拾取提示和菜单文字，不能代替物理时序断言。

最终日志没有专项 FAIL、Fatal 或 Assertion，也未发现掉落物理碰撞配置警告；既有 ExperimentalToolsets Python 初始化错误仍见部分 `-game` 日志，本阶段未扩展修复该插件，不能称为所有日志均无错误。

未验收专用服务器、新打包/Cook、十人完整五分钟、长时自然网络、逐键硬件输入或新增角色形态。ASC 仍在 Character；未来控制、强制位移、爆破/死斗、美术和输入重映射系统的边界见 [后续想法](FutureIdeas.md)。

交付文档检查：2026-10-06 在项目根运行 `python Tools/CheckDocs.py`，41 份文档、0 errors、102 项既有历史例外，未新增或扩大豁免。人工核对三阶段状态、当前规则、历史版本边界、证据和相对链接；后续完整验收范围仍如上保留。

返回 [角色主题](README.md)。

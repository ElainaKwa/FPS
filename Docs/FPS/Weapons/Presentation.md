# 动作结果与表现时间语义

- 用途：实施与验证记录
- 适用范围：纠正版评审 R6＋R7；新 FPS Demo 武器表现，保留教学路径
- 状态：实现完成，真实监听服务器、拥有者客户端及迟加入客户端针对性验收通过
- 最后更新：2026-10-04

依据：[最终决策](../../FPSWeaponRefactorDecisions.md)、[纠正版评审](../../FPSWeaponArchitectureReview.md)。进度统一见[架构任务清单](ArchitectureTasks.md)。

## 实施边界

`FPSWeaponComponent` 继续拥有库存、动作票据和复制状态；Ability 生命周期及提交校验沿用 R1。`FPSWeaponPresentation.cpp` 接收明确的 Started / Committed / Cancelled 结果，不从弹匣增量推断换弹成功，不用表现回调修改库存或动作。

每个权威结果带服务器事件序号、动作序号、实例 GUID、WeaponId、命令和 GAS 预测键。拥有者预测开始立即播放；对应服务器开始结果按实例、命令、预测键去重。事件序号拒绝重复权威通知。客户端计时完成不播换弹完成音效，只有通过截止时间与票据检查后的服务器提交才播；取消不播完成音效。拒绝清除对应预测去重记录；记录有 256 项上限，避免长期增长。此上限不是任意长断网条件下的保证。

操作结果通过可靠 Multicast 到当前相关客户端；声音按结果里的 WeaponId 查资源，不借当前装备取旧动作音效。本地玩家使用本地路由，其他角色使用空间路由。保留 `OwnerState` / `VisibleState` 的动作与截止时间，姿势由持续状态恢复，初次收到快照不会重播历史音效。第三人称换弹原已存在；本轮保留换弹、检视，并补独立举枪偏移。

## 时间契约

| 参数或状态 | 当前语义 |
| --- | --- |
| `EquipReadyTime` | GAS 举枪准备截止时间；决定能否开火，本轮不改 |
| `EquipBlendTime` | 从复制的 `EquipStartedAt` 起，到举枪视觉偏移归零的确切秒数；零值立即归零 |
| `AimTransitionTime` | ADS 目标改变后，从当前视觉值到新目标的确切秒数；零值立即到位；中途反向从当前值连续开始 |
| `BaseFieldOfView` | 角色相机可编辑设置，默认仍为 92；退出 ADS 回到该值 |
| `AimFOV` | 武器定义中的瞄准目标 FOV |
| `VisualRecoilReturnTime` | 角色表现设置，默认 0.25 秒；单发视觉 kick 回到零的确切时长，与举枪独立 |
| `RecoilRecoveryTime` | 既有玩法参数：停止射击后重置后坐力序号的等待时间，不是视觉回弹时长 |

ADS、举枪和视觉后坐力使用各自的时间，平滑曲线为 `a*a*(3-2*a)`，到截止时间精确达到目标。枪体位置由各偏移叠加，删除用 `EquipBlendTime` 统一控制所有姿势插值的路径。换弹、检视按对应动作截止时间驱动，冲刺另用 0.15 秒表现过渡。教学 Demo 保留原开火、伤害与程序动画路径。

本轮不改定义资产数值或新增玩法字段。内容契约 `ManifestVersion` 升到 2，以区分新的表现时间语义；两端仍须一致，不支持旧客户端兼容。资源哈希来源与 R3＋R4 不变。

## 编译与实际运行记录

最终模块为 `UnrealEditor-MCP_Test-9324.dll`；9322 至 9324 只调整或增加测试断言，游戏实现相同。运行方式是后台 `UnrealEditor.exe -game` 监听服务器和独立 localhost 客户端，**不是交互编辑器 PIE，也不是 Dedicated Server**。专项拥有者客户端开启离屏渲染与音频，设置 `PktLag=50`、`PktLoss=1`。

- 9319 编译通过，但专项测试开关漏接自动开局，停在等待状态；该轮没有执行专项断言，不计通过。
- 9320 双端专项通过：开始确认去重、重复事件拒绝、弹药快照纠正不播完成、切枪取消、真实 GAS 拒绝、远端换弹姿势与空间音效、ADS 正值/零值时长、基础 FOV 恢复、独立视觉回弹。日志为 `Saved/Logs/WeaponR67Presentation9320.txt`，原始日志保存在 `Saved/Logs/WeaponR67Evidence/`。
- 9321 在此基础上清理被拒绝的预测开始记录，增加中途加入客户端验收。入场恢复通过，但原断言把估算时间窗与已经到达的结束状态混合判断，终点检查失败；未将其判定为已复现的玩法故障。
- 9322 将迟加入检查改为动作期间观察到姿势、结束后同时检查 Idle 与权威事件。三进程全部通过：迟加入时检视剩余 6.39 秒，未收到历史开始事件；结束时 `pose=1 action=0 event=4`。同时重跑专项全部断言通过。
- 9322 既有 GAS 动作回归在同样 `PktLag=50` / `PktLoss=1` 下双端通过，覆盖旧动作票据、预测拒绝、三连发、全自动等待/恢复、切枪与死亡取消。原教学 Demo 独立双端回归通过，覆盖固定伤害、治疗、死亡与重生。
- 9323 补充实际切枪后的零/正举枪时长断言。零值通过，但固定输入后 1.2 秒取样的正值终点检查失败；该取样条件不能保证已超过复制后的权威起点及视觉时长，不计为专项通过。
- 9324 将该断言改为依据实际 `EquipStartedAt + EquipBlendTime`，另要求在测试窗口内观察到截止点。三进程专项全部通过：视觉年龄 1.116 秒、时长 1 秒时，枪体位置精确等于 `HipLocation`（42,18,-18）；中途加入检视剩余 6.38 秒，最终恢复到 Idle 并收到事件 4。未通过改玩法数值或关闭断言使测试变绿。

本轮证据入口：

| 检查 | 证据 |
| --- | --- |
| Editor 9324 编译及构建数据检查 | `Saved/Logs/WeaponBuild9324.txt` |
| Game Development 编译 | `Saved/Logs/WeaponGameBuildR67Final.txt` |
| 最终三进程专项，含音频与离屏渲染 | `Saved/Logs/WeaponR67Presentation9324.txt` |
| 既有 GAS 双端回归 | `Saved/Logs/WeaponR67Abilities9322.txt` |
| 原教学 Demo 双端回归 | `Saved/Logs/WeaponR67Teaching9322.txt` |
| 人机实际运行，路由、交战和 GAS 得分 | `Saved/Logs/WeaponR67AI9324.txt` |
| 编辑器命令let导出、全项目七定义校验与负例自测 | `Saved/Logs/WeaponR67Data9324.txt` |

原始日志、模块映射、源文件 SHA256 和音频保存在 `Saved/Logs/WeaponR67Evidence/`。9324 音频样本非静音、无削波，服务器 peak=0.0778，客户端 peak=0.1366；日志确认本地与空间两种路由实际创建声音。人工查看本轮 `Saved/Screenshots/WindowsEditor/WeaponPresentation.png`，确认第一人称瞄准枪体和 HUD 可见；单张截图不代表完整动画观感审查。原教学地图 SHA256 仍为 `86D4F53F872541FCF7D9E810B97EC91F8B7D1D19CDB24756D15ABAE166A65E43`。

`FPSWeaponPresentationSmoke` 的快照重放是接缝检查；单独的中途加入客户端才验证网络创建时恢复持续姿势。测试中的定义时长与弹药调整仅为进程内夹具，不保存为资产。

交付前运行 `python Tools/CheckDocs.py` 和 `git diff --check`。33 份文档、0 错误、106 项历史例外；未新增或扩大豁免。已人工核对本记录的阶段状态、构建号、失败轮次和最终证据，生成清单通过编辑器命令let重新导出。本轮没有手工修改二进制资产。

## 验证边界

不把编译、短时网络测试和单张截图等同于完整手感验收。完整五分钟、十真实客户端、Dedicated Server、完整 Cook/打包、高延迟长时间断线和多次出入相关范围压力测试仍未完成。本轮不据此宣称带宽、性能或全部网络条件通过。

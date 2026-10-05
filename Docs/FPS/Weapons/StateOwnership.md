# R1：状态所有权与动作身份

- 用途：实施与验证记录
- 适用范围：FPS 武器组件与共用 GameplayAbility
- 状态：R1 实现及 9307 构建、真实双端、人机和教学局部回归已通过
- 最后更新：2026-09-30

依据：[纠正版评审 R1](../../FPSWeaponArchitectureReview.md)、[任务清单](ArchitectureTasks.md)。本轮不是复现评审所述风险后才修复，不把静态耦合描述成已发生的玩家故障。

## 改动边界

组件独占 Loadout、OwnerState、PredictedShots 及库存提交。Ability 保留 GAS 生命周期、模式内计数与计时器，通过动作接口访问组件；移除 Ability 的 friend 权限。开发 smoke 仍有白盒接缝，用于可控负例注入。

每次激活取得本地动作票据：单调序号、实例 GUID、命令及预测键。票据不复制，不接受客户端 RPC 传入。开始接口检查所属 Avatar、活跃 Ability、当前开启动作和玩法条件；提交、完成、取消检查当前票据。换弹另以组件保存的截止时间校验，不能利用复制表现时间提前完成。同一个 Ability 对象、同一把枪的后续激活也得到新序号。

计时回调捕获原票据并在触发时核对，过期回调不能结束重激活的 Ability。组件只保留共用 Ability 取消入口，不读取具体 Timed Ability 的命令或实例字段。取消射击只纠正对应枪的运行数据，不整体覆写后来切枪/ADS/动作状态。

R2 的内部 Equip 和轮询、R5 的伤害语义、R3/R4 的加载及 R6/R7 表现仍是独立阶段，未在此声明完成。

## 验证记录

最终构建为 `UnrealEditor-MCP_Test-9307.dll`，以下均为 2026-09-30 本轮新运行结果。

| 入口 | 条件与结果 | 汇总日志（Saved/Logs） |
| --- | --- | --- |
| Editor Development 构建 | UHT、C++、post-build 七定义与地图引用校验通过 | WeaponBuild9307.txt |
| TestFPSWeapons.ps1 -Abilities -Latency 50 -PacketLoss 1 -Visual | 独立服务器/客户端均 COMPLETE PASS；两端日志确认 PktLag=50、PktLoss=1 | WeaponR1Abilities9307.txt |
| TestFPSWeapons.ps1 -Audio -Visual | 七枪、BR-3 三连发、弹药、换弹中断、重复拾取与移动回归通过，双端 COMPLETE PASS | WeaponR1Weapons9307.txt |
| TestFPSWeapons.ps1 -Teaching | 原教学地图双端射击、治疗冷却、击倒计分及重生 COMPLETE PASS | WeaponR1Teaching9307.txt |
| TestFPSAI.ps1 | 人机绕开掩体、接敌射击、GAS 计分通过 | WeaponR1AI9307.txt |

R1 专项包括提前完成、错误实例取消、同枪再次换弹后的旧完成/取消、旧 ADS 结束、旧射击票据和延迟预测拒绝。旧票据由真实激活中采集，再通过组件接口注入过时调用；这证明接口隔离，不能冒称已通过网络自然复现所有回调竞态。另有真实服务器拒绝客户端伪造冷却的 GAS 运行日志，弹药纠正和后续状态收敛均通过。

第一次 9306 回归未通过：固定时间采集早于服务端 RPC 到达，导致三项负例没有取得旧票据；改为观察真实激活后采集。同时修复了权威状态已换实例后旧动作登记未清理的边界：仅退役旧登记，不写新实例状态；Ability 结束若完成被拒绝，仍走有身份校验的取消清理。未将失败日志当成通过证据。

原始 Ability、七枪和 AI 日志另存 `Saved/Logs/WeaponR1Evidence/`，避免后续脚本覆盖。已查看本轮 BR-3 截图，模型及 HUD 30/84 正常。双端录音非静音、无削波检查通过（服务器 RMS 0.010998，客户端 0.009234）；录音窗口不覆盖全部后半段场景，不扩大为所有操作声音验收。

`python Tools/CheckDocs.py` 通过：0 errors，历史豁免从 113 减为 110 项（补齐原清单元数据后删除 3 项失效豁免），未增加；`git diff --check` 通过。新文档按主题创建并经索引可达，未手改生成资料或批量迁移历史文档。内容状态、证据范围和归档边界已人工核对，外部链接与历史内容未逐项重审。

尚未覆盖：全五分钟、十真实客户端、Cook/打包和压力测量不在此局部验收内。资产与原教学实现本轮未修改。

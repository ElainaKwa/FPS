# R1：状态所有权与动作身份

- 用途：实施与验证记录
- 适用范围：FPS 武器组件与共用 GameplayAbility
- 状态：R1 历史回归通过；2026-10-12 PIE 退出修复、真实单机/双端 PIE 与 GAS 回归通过
- 最后更新：2026-10-12

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

## 2026-10-12：换弹中退出 PIE 的生命周期修复

用户报告并由崩溃记录确认：`UFPSTimedWeaponAbility::EndAbility → FinishAction → EmitActionResult → PresentActionResult → Catalog()->Find` 在 PIE 销毁阶段访问已不存在的 GameInstance 武器目录。证据目录：`Saved/Crashes/UECC-Windows-85AEBB3044D0AC435D94CB8DCC256872_0000`、`UECC-Windows-00D2E37E402D81FE50B69EBEEF23D548_0000`。此前游戏内取消/死亡/断线通过，不覆盖正常停止 PIE。

- 角色在 `Super::EndPlay` 前调用组件幂等 `ShutdownActions`，组件自身 `EndPlay` 也兜底。先标记退出、作废动作票据、清空 readiness 委托和预测缓存，再以不复制取消的方式结束所持 Ability，避免清理回调唤醒新动作。
- 世界开始销毁时停止动作提交、预测回滚发布、复制表现和动作事件发送。目录/定义访问允许返回空值，表现入口安全处理空目录。Ability 清理定时器时允许 World 已不存在。
- 保留正常游戏中的取消结果、换弹提交及 GAS 生命周期；不依赖延长目录资产寿命掩盖清理顺序。
- 新增 `MCP.FPS.Weapons.ExitPIE` 自动化测试和 `Tools/TestFPSWeaponPIE.ps1`：真实创建 PIE 世界，发射后在换弹动作活跃时请求正常结束 PIE，并重新创建下一局；另测检视中退出。双端用例必须观察到服务端及客户端动作均活跃才允许验收。
- 9412 生产修复编译通过；50 ms / 1% 的真实服务器/客户端 GAS 回归通过，证据 `Saved/Logs/WeaponPIEFixAbilities9412.txt` 与 `Saved/FPSPIEFixRegression/`。
- PIE 测试早期版本的双端用例因训练模式拒绝联网而失败；随后发现 `AdditionalServerGameOptions` 不用于进程内 PIE，改为编辑器 PIE URL，并在用例结束时恢复。失败日志保留为 `WeaponPIEExit9412Failed.log`、`WeaponPIEExit9413Failed.log`，不计为双端通过。最终 9414 构建通过，`MCP.FPS.Weapons.ExitPIE` 返回 Success：单机换弹、单机检视、监听双端换弹、监听双端检视四项均确认动作活跃后正常退出，编辑器存活。双端每项观察 authority=2、client=1。证据：`Saved/Logs/WeaponPIEExit9414Passed.log`、`WeaponPIEExitHarness9414.txt`、`WeaponPIEFixBuild9414.txt` 和 `Saved/Automation/WeaponPIEExit/index.json`。测试使用 NullRHI，不验收动画画面；不将进程退出代替 PIE 正常结束。

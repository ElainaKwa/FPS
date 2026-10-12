# 扩展验收与结构整理

- 用途：实施与验证记录
- 适用范围：R1–R7 后续验收与纠正版评审第 3 节的适量整理
- 状态：结构整理、完整打包和自动专项运行通过；完整人工手感、外网及独立 Server 包未验收
- 最后更新：2026-10-05

基线提交：`a7fdc86`。依据[纠正版评审](../../FPSWeaponArchitectureReview.md)与[架构任务清单](ArchitectureTasks.md)。保留原教学地图、无关用户改动及既有玩法，不增加穿透、弹道、延迟补偿或动态难度。

## 实施边界

- `AFPSDemoGameState::IsCombatActive` 集中对局战斗门禁。武器组件仍检查自身角色、内容准入和目录状态；GameMode 复用同一对局判断，不再维护另一份倒计时/结束判断。
- AI 执行从对局流程文件分离到 `FPSBotController.cpp`；`UFPSBotSettings` 与 `DefaultGame.ini` 持有巡逻点、感知/交战距离、到达距离及拾取站位。默认值保持原行为；配置无效时 InitGame 明确失败。此配置目前面向 Foundry，不把它描述为任意地图导航系统。
- 原开局要求监听主机本地 Controller，Dedicated Server 无此角色。补充服务器在两队存在且当前连接全部完成内容校验后开局；监听房间保留原主机权限。战斗仍为 GAS Ability 执行。
- 两处 smoke 首个头文件顺序在完整打包构建时暴露，修正为自身头文件在前。
- 新长局工具只使用真实客户端进程与公共 Ability 输入，不改五分钟截止，不手动调用 FinishRound，不造 PlayerState 充数。进程内测试采样和后坐力日志由显式开发参数开启。

## 构建与验证记录

首轮 UAT 尚未进入 Cook 即失败：打开的旧编辑器占用热重载模块。重试跳过 Editor 编译时，又发现首轮清理删除了 Editor target receipt；随后重新构建新 Editor 模块以恢复构建收据。失败日志保留为 `Saved/Logs/WeaponAcceptancePackageBaseline.txt` 和 `WeaponAcceptancePackageBaselineRetry.txt`，不计为打包通过。

### 构建、Cook 与打包运行

- Editor 9333 编译通过；9334 仅修改后坐力测试的断言/采样，生产逻辑相同，亦编译通过。日志：`Saved/Logs/WeaponAcceptanceBuild9333.txt`、`WeaponAcceptanceBuild9334.txt`。
- 9332 的 Cook 进入暂存，但 UAT 无法读取 Zen oplog；不能计为打包通过。9333 通过引擎支持的 `-AdditionalCookerOptions=-SkipZenStore` 使用文件输出，避免该服务阻塞，没有修改项目默认 Zen 配置。
- 首个成功包在实际客户端连接时暴露 `PackageMissing: LV_FPSFoundry`。目录 Cook 配置未带入地图；补充 Foundry、Training、原 FPS 教学地图的显式 `MapsToCook` 后完整重做 Cook、Stage、Archive 成功。最终日志：`Saved/Logs/WeaponAcceptancePackage9333Maps.txt`；输出：`Saved/Packages/WeaponAcceptance/Windows`。不是只改日志或复用失败包。
- 实际打包客户端加载 7 个必需定义、24 个资源，内容摘要与服务器一致，成功通过准入。
- 打包版实际渲染（非 NullRHI）：Foundry 出生点 10、武器台 14、角色 10；Training 出生点 4、武器台 7、角色 4，出生空间检查均通过。`WeaponAcceptancePackagedRender9333.txt`；截图在包内 `MCP_Test/Saved/Screenshots/WindowsEditor/`，已查看地图、持枪模型和 HUD。截图不等于完整手感验收。

### 针对性回归

均为本轮重新启动的实际进程，不使用历史日志替代：

| 检查 | 结果 | Saved/Logs 证据 |
| --- | --- | --- |
| GAS 动作取消、预测拒绝、R2 唤醒与死亡清理，50 ms / 1% | 服务器与客户端通过 | WeaponAcceptanceAbilities9333.txt |
| AI 路径、脱困、索敌开火、GAS 伤害计分 | 通过 | WeaponAcceptanceAI9333.txt |
| Easy/Normal/Hard Solo 与 Hard Training，换弹、掉落及难度隔离 | 通过 | WeaponAcceptanceDifficulty9333.txt |
| 监听房间主机权限、队伍、死亡重生和结算门禁 | 双端通过 | WeaponAcceptanceFlow9333.txt |
| 原教学 Demo 的伤害、治疗、死亡和重生 | 双端通过 | WeaponAcceptanceTeaching9333.txt |

这些回归使用 9333/9334 模块；两者生产实现相同，差异仅为后坐力专项开发测试。原教学地图 SHA256 仍为 `86D4F53F872541FCF7D9E810B97EC91F8B7D1D19CDB24756D15ABAE166A65E43`。

### 移动目标与后坐力

真实监听服务器 + 客户端，双方设置 `PktLag=150`、`PktLoss=3`，不将参数直接描述为实测 RTT。目标横向移动，实际 GAS 自动开火 30 发，记录预测曲线增量和每次服务端射线方向。

- 持续跟踪目标对照（9332）：30 发、16 次命中、400 伤害，双端通过。角度增量与曲线误差最大约 0.000001°。
- 不补偿、不继续追踪（9334）：30 发、0 命中；双端观察到俯仰累计变化约 38.56°，逐发服务器射线同样改变。该用例验证后坐力向权威命中方向传播，不要求移动目标仍在瞄准线上。
- 原 9333 未补偿用例沿用“必须命中”的断言失败，保留失败日志；修正为未补偿方向变化断言后以 9334 新日志复测通过。没有将原失败记录改写为通过。
- 9334 服务器射线与预测施加后方向差值：中位约 1.336°，最大约 1.384°；两个日志中的服务器时间差中位约 0.382 秒。输入复制、网络模拟和施加时序均影响它们，不据此宣称射线完全一致，也不实现延迟补偿。
- 原始双端日志及分析 JSON：`Saved/FPSAcceptance/Evidence/`；复核工具 `Tools/check_fps_recoil.py`，未补偿模式加 `--uncompensated`。

### Dedicated Server 十客户端自然长局

`Tools/TestFPSAcceptance.ps1 -Reconnect -RunName Dedicated9333Maps`，9333 实际 Editor `-server`（日志 `netmode=1`）+ 十个独立打包客户端（`netmode=3`），双方 150 ms / 3% 模拟，30 FPS 上限。没有缩短 300 秒截止，也没有伪造 PlayerState。

- 服务器 `age=300.09`、`observedWall=300.05`、最高/结算人数 10、领奖台 3，最终记录射击序号总数 1644；所有当前客户端均通过自然结算和战斗门禁检查。此总数不是全局累计击发统计，重生/离线会改变求和对象。
- 第一个客户端在第 60–80 秒将发送丢包率设为 100%，随后恢复 3%，可靠 RPC 往返恢复。该条件是发送中断，不宣称双向物理断网。
- 第十个客户端进程在约 120 秒被终止，服务端观察人数降为 9；约 70 秒后新进程加入，通过内容准入，人数恢复 10 并参与结算。重连者仅观察最后约 101 秒，不宣称其连续在线五分钟。
- 最低 PlayerId 的角色在 30–45 秒、90–105 秒由专用测试代码移出相关范围并返回；9 个旁观客户端均记录两次角色消失及有效装备恢复。测试移动不写入地图资产；不把有效装备快照检查扩大成全部蒙太奇人工验收。
- 脚本反复调用实际公开输入接口，未无限补弹；后段存在耗尽弹药后的操作，因此不是五分钟持续满射速压力测试。
- 原始记录：`Saved/FPSAcceptance/Dedicated9333Maps/`；汇总日志：`Saved/Logs/WeaponAcceptanceDedicated9333Maps.txt`；摘要：`Saved/FPSAcceptance/Evidence/Long9333Metrics.json`。
- 此次服务器内存样本 1525.8–1571.8 MB，客户端 343.2–364.3 MB；服务器平均帧间隔 33.44 ms，最大 77.42 ms。采样期间同机有其他回归和渲染进程，不据此判定泄漏或优化收益。初次未启用 `net.EnableNetStats`，其流量 0 无效，另行补采。

### 流量补采与复现

启用 `net.EnableNetStats 1` 后，以 `Tools/TestFPSAcceptance.ps1 -SampleSeconds 40 -RunName NetworkSamples9334` 重新启动十个打包客户端和 Editor 9334 Dedicated Server。全部取得十人在线时的非零收发样本。服务器在第 30 秒采样为入站 32546 B/s、出站 76354 B/s；每客户端范围见 `Saved/FPSAcceptance/Evidence/Network9334Metrics.json`。短采样模式不宣称五分钟结算通过；自然长局证据独立保留。

完整打包复现命令（项目根目录，先构建 Editor；`-skipbuildeditor` 避免重建占用中的编辑器模块）：

```powershell
& 'D:/UE/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat' BuildCookRun '-project=D:/Unreal Projects/MCP_Test/MCP_Test.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -skipbuildeditor -cook '-AdditionalCookerOptions=-SkipZenStore' -stage -pak -archive '-archivedirectory=D:/Unreal Projects/MCP_Test/Saved/Packages/WeaponAcceptance' -unattended -utf8output
```

本轮已运行包的 `MCP_Test.exe` SHA256：`D48D316ADCE50A461B5C828D5B23E65DA7FE53C2BEBE26FA45500E87A2AD17A3`。它包含 9333 对应源码；最终 9334 增量只修正非 Shipping 后坐力测试，未重新生成该包。源码与包版本边界不混写。

## 边界

Dedicated Server 的编辑器 `-server` 进程与独立打包 Server target 是不同交付物；本机安装版引擎的支持范围需明确区分。进程帧间隔不是纯 CPU 帧耗，网络与内存采样不是完整性能基准；同机多进程不等于真实外网。

完整人工手感不能由自动输入或单张截图替代。未经完成的检查保持在任务清单中，不因本轮扩大测试范围自动勾选。

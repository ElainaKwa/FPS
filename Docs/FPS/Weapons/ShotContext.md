# R5：权威射击快照与伤害语义

- 用途：实施与验证记录
- 适用范围：FPS 武器射击提交、GAS 伤害及教学适配
- 状态：R5 实现与针对性验收完成；完整长局、多客户端和打包未覆盖
- 最后更新：2026-09-30

依据：[纠正版评审 R5](../../FPSWeaponArchitectureReview.md)、[最终决策](../../FPSWeaponRefactorDecisions.md)、[阶段清单](ArchitectureTasks.md)。原评审仅指出隐式传参风险，本轮不声称已自然复现切枪串伤害。

## 实施边界

`CommitShot` 仍由有效 Fire Ability 请求，校验票据、实例、状态、弹药和冷却后提交。在发布状态变化通知前，捕获服务器本地的 `FFPSWeaponShotContext`：来源角色、WeaponId、实例 GUID、ShotSerial、基础伤害、射程和开火声音。射击结算不再通过当前装备或 ASC 的展示缓存查找这些值。快照不是开火授权凭据，不经 RPC 接收；生产路径仍经过 Ability 和组件校验、扣弹。

`WeaponDamage` 属性保留为当前装备的复制展示缓存，`Changed` 更新它；修改该缓存不会改变权威伤害。本轮没有新增增益系统：定义提供基础伤害，服务器在命中时根据攻击者和目标应用既有难度规则，将负伤害写入 `UFPSWeaponDamageEffect` 的 SetByCaller，由 GAS 修改和复制 Health。属性名与 SetByCaller 键同名不表示结算读取属性。以后加入增益须明确其快照时机，不应复用展示缓存作为伤害通道。

友伤、赛后和目标存活过滤保留。教学地图显式走固定 25 伤害的 `UFPSDemoDamageEffect`，不会因为结算时装备状态改变而切换 Effect 类型。射线仍使用服务器当前视点和控制朝向，同步即时结算；未新增弹道、延迟命中、补偿或客户端命中上报。

## 验证设计与记录

本轮验收构建为 `UnrealEditor-MCP_Test-9313.dll`；9312 已完成运行代码，9313 只调整测试瞄准时序和整理测试 includes。以下均为 2026-09-30 的新运行证据。

| 入口 | 条件与结果 | 汇总日志（Saved/Logs） |
| --- | --- | --- |
| Editor Development 构建 / post-build 数据校验 | 9313 成功；七定义检查通过 | WeaponBuild9313.txt |
| TestFPSWeapons.ps1 -Damage -Latency 50 -PacketLoss 1 | 监听服务器＋独立客户端 COMPLETE PASS | WeaponR5Damage9313.txt |
| TestFPSWeapons.ps1 -Abilities -Latency 50 -PacketLoss 1 | R1/R2 专项双端 COMPLETE PASS，含实际服务器拒绝与预测纠正 | WeaponR5Abilities9313.txt |
| TestFPSDifficulty.ps1 | Easy/Normal/Hard Solo 与 Hard Training 四组通过 | WeaponR5Difficulty9313.txt |
| TestFPSWeapons.ps1 -Teaching | 原教学地图双端 COMPLETE PASS，伤害、治疗、击倒和重生通过 | WeaponR5Teaching9313.txt |
| TestFPSFlow.ps1 -Mode LAN | 对局流程、赛后过滤、计分和客户端受伤/死亡/重生反馈通过 | WeaponR5Flow9313.txt |
| TestFPSAI.ps1 | 人机实际绕行、射击命中和 GAS 计分 COMPLETE PASS | WeaponR5AI9313.txt |

新增 `-Damage` 测试在远端玩家的 Ability 提交后将 ASC 展示缓存污染为 999，双端目标生命从 100 变为 66，仍是 AK 的 34 点伤害；切到 M9 后注入旧 AK 快照仍为 34，下一次实际 M9 射击为 25。客户端不能本地提交伪造伤害，友伤消耗弹药但不扣血，赛后旧快照不再扣血；两端最终生命均为 41。核对捕获快照的型号、GUID、ShotSerial 和基础伤害。

旧快照重复应用是开发测试边界注入，不代表游戏具有延迟命中，也不作为自然网络复现证据。该用例固定靶位，只验证伤害上下文，不能作为后坐力或移动目标朝向同步的全面验收。

9312 首次专项的首发未命中，其余上下文检查通过，但整轮判定失败。测试将转向和首发放在同一时刻；9313 改为提前一秒瞄准，记录服务器发射方向，实际首发通过。未为此改运行时伤害逻辑；原失败与通过日志均保存在 `Saved/Logs/WeaponR5Evidence/`，不把失败日志作为通过证据。

难度矩阵验证人机对玩家 0.5/0.65/0.85、训练模式 1.0、玩家及人机互击正常伤害、拾取人机武器无倍率残留，并保留友伤/赛后过滤。矩阵是服务器规则检查；双端复制证据来自上述监听服务器进程和独立客户端进程专项。

所有表内运行的原始日志及模块映射均已保存到 `Saved/Logs/WeaponR5Evidence/`。双端测试为同机 `UnrealEditor.exe -game` 监听服务器与独立客户端；普通检查使用 `-nullrhi`，不是编辑器 PIE 或人工操作。网络专项明确传入并在日志确认 `PktLag=50`、`PktLoss=1`。

交付检查：`python Tools/CheckDocs.py` 通过（31 文档，0 错误，110 条既有历史例外），`git diff --check` 通过；未新增或扩大豁免。人工核对文档状态、证据对应关系和主题索引。未改二进制资产或生成资料。

完整长局、十客户端、Dedicated Server、Cook/打包和人工手感验收不在本阶段已验证范围。

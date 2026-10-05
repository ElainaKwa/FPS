# 枪械架构重构第二阶段实施记录

状态：第二阶段实现及下列局部验收已完成（2026-09-29）。当时第三阶段尚未实施；其后已完成的 BR-3 局部验收见 [第三阶段](FPSWeaponRefactorStage3.md)。不把编译成功等同于网络验收。后续结构风险见 [纠正版评审](FPSWeaponArchitectureReview.md)。

## 实施范围

按最终决策迁移已有六枪的行为执行与实例网络状态。没有添加第七把枪、逐发装填、配件、动态难度或设置页面；原教学地图与教学蓝图保留。第一阶段既有变更均保留，本阶段没有重新生成地图。

## 代码职责

- `FPSWeaponData`：保存定义、稳定身份与实例。定义增加 Single / Automatic / Burst、BurstCount、BurstInterval、RecoilRecoveryTime，移除临时 bAutomatic；六枪通过编辑器脚本保存，原数值和资源引用保留。三连发验收仅修改测试进程内的定义，不保存测试型号。
- `FPSWeaponAbilities`：共用的射击、弹匣换弹、检视、切槽/举枪、ADS、丢弃/拾取和取消 Ability。每个行为复用同一实现；没有按枪型号分支。Ability 拥有计时器和取消清理，组件 Tick 不再结算动作或调度射击。
- `FPSWeaponComponent`：定义解析、实例容器、权威校验与原子提交、输入转发和复制快照。射击提交只接受正在运行的 Fire Ability。玩家增强输入、AI 决策都进入 ASC；原结构化模式的 ServerShoot / ServerRequest / FireAuthority 路径已移除。
- `FPSDemoCharacter`：权威射线与 GAS 伤害、现有程序动画及音效。旧教学模式继续使用自己的 ServerFire 和固定伤害效果；该 RPC 在结构化模式不执行开火。
- `FPSDifficultySettings` / `FPSMatchMode`：独立 Config=Game 难度配置和开局快照。单机大厅选择简单/普通/困难，默认普通；倍率 0.5/0.65/0.85 只应用于 Solo 人机攻击玩家，保持友伤和结算过滤，不写入枪械。

## 动作与预测

Ability 使用 InstancedPerActor；可预测操作采用 LocalPredicted。客户端立即更新可预测的模型、动作、ADS 与射击反馈；服务器独立验证并负责弹药、射线、伤害及拾取提交。客户端不能用结束 Ability 的请求提前完成换弹，服务器独占终止权限；输入释放和取消 Ability 仍传递到权威端。

拾取通过 GAS GameplayEvent 携带明确目标引用，服务器再次检查距离、遮挡和认领状态。连续请求不能重新寻找刚换出的另一把枪。丢弃和拾取的库存转移只在权威端提交。

每个实例保留 GUID、WeaponId、弹药、NextShotAt、LastShotAt、RecoilIndex 和 ShotSerial。后坐力只有在最后一发之后达到恢复等待时间才重置。切走或落地不会停表，也不保存未完成动作。

拥有者按射击序号保留尚未确认的本地预测，避免较晚到达的权威快照打断本地连发节奏；终止确认和 GAS prediction key 拒绝会清除预测并恢复权威状态。音效属于即时反馈，已播放的一次声音不能倒放撤销；命中和伤害从不由客户端预测提交。没有加入命中延迟补偿。

ADS 独立于换弹状态；默认保持，SetReloadExitsAim 可切换为开始换弹退出一次，服务器同步该偏好。切枪取消 ADS 后必须释放再按。Ability activation 拥有并计数 State.Weapon.* 标签，复制回调不再覆盖 loose tag 数量。举枪准备和视觉过渡分别使用 EquipReadyTime 与 EquipBlendTime。

## 复制范围

| 接收方 | 实际字段 |
| --- | --- |
| 服务器 | 完整双槽实例及权威 Ability/动作 |
| 拥有者（COND_OwnerOnly） | 双槽实例、动作、ADS、准备时间、射击预测确认及换弹偏好 |
| 其他客户端（COND_SkipOwner） | 当前 WeaponId / GUID、动作与结束时间、举枪起点、ADS；无库存/弹药 |
| 地面观察者 | GUID、WeaponId、弹匣与备弹；完整冷却/后坐力实例只留服务器 |

没有进行带宽抓包测量，不宣称固定的带宽降低百分比。

## 验证入口与结果

最终编辑器模块为 `UnrealEditor-MCP_Test-9303.dll`。主要运行回归使用 9300；9301 只修复只读清单的默认值导出，9302/9303 只修正教学回归测试的远端 Controller 假设和传送落地后的瞄准，没有改变已验收的武器行为。原教学回归使用 9303。

| 入口 | 检查内容 | 结果 |
| --- | --- | --- |
| Editor Development 构建与 post-build 数据校验 | UHT、C++、已保存定义/曲线/地图引用、只读清单一致性 | 9303 通过；数据负例 SelfTest 在 9301 通过 |
| TestFPSWeapons.ps1 -Abilities -Latency 50 -PacketLoss 1 -Visual | 实际双端 Ability、预测三发、冲突/取消、实例延续、拒绝纠正、零准备、死亡 | 9300 双端 COMPLETE PASS，日志确认两端 PktLag=50、PktLoss=1 |
| TestFPSWeapons.ps1 -Audio -Visual | 六枪实际射击、资源、HUD、换弹/检视、拾取/重复请求、移动 | 9300 双端 COMPLETE PASS；六枪截图逐一检查，双端音频检测通过 |
| TestFPSDifficulty.ps1 | 三档 Solo 与不受影响的 Training、实际 GAS 扣血、AI 行为限制、拾取后伤害 | 9300 四种场景全部通过 |
| TestFPSFlow.ps1 -Mode LAN | 服务器/客户端生命、友伤、击倒、得分、重生、结算 | 9298 已通过 |
| TestFPSAI.ps1 | 实际 AI 寻路、交战、击倒 | 9300 通过：脱离掩体、接敌开火、30 秒内得分 3 |
| TestFPSWeapons.ps1 -Teaching | 原教学地图双端射击、治疗冷却、击倒与重生 | 9303 双端 COMPLETE PASS：25 伤害、35 治疗、重复治疗拒绝、击倒计分和满血重生 |

日志位于 Saved/Logs 与 Saved/FPSFlowTests，截图位于 Saved/Screenshots/WindowsEditor。开发测试必须显式指定 smoke 参数，Shipping 不包含测试执行。

主要汇总日志：`WeaponBuild9303.txt`、`WeaponStage2FinalData.txt`、`WeaponStage2FinalAbilities.txt`、`WeaponStage2FinalWeapons.txt`、`WeaponStage2FinalDifficulty.txt`、`WeaponStage2Flow.txt`、`WeaponStage2AI.txt`、`WeaponStage2Teaching.txt`。原始双端日志保留具体断言与权威拒绝记录。音频 WAV 经 `Tools/check_fps_audio.py` 检查非静音、无削波；服务器峰值 0.1638 / RMS 0.010998，客户端峰值 0.1910 / RMS 0.009874。

只读清单导出现在明确包含默认枚举与布尔值（如 Primary、Rifle、False），避免审查时误以为字段缺失。编辑器已保存全部内容后正常关闭并重启，实际进程确认加载 9303，恢复 Entry 地图；没有强制终止用户编辑器。

## 尚未覆盖

完整五分钟自然结束、十个真实客户端、打包/Cook 与性能压力仍不是本次局部验收结果。这里的战斗双端验收使用真正的独立游戏进程和网络驱动，不冒称人工双窗口 PIE 战斗测试。9303 重启后额外在编辑器 PIE 实际点击大厅难度按钮，确认默认普通及普通→困难→简单→普通循环，然后停止 PIE。第三阶段的新三连发步枪仍待制作。

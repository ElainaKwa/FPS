# 部位伤害实施与验证

- 用途：实施与验证记录
- 适用范围：现有 7 把结构化枪械、程序角色、GAS 伤害与命中反馈
- 状态：实现完成；Editor／Game 构建、双端专项及当前根项目集成复验通过；未覆盖范围见文末
- 最后更新：2026-10-15

设计依据为 [部位伤害决策](HitDamageDesign.md)，完成度见 [清单](HitDamageTasks.md)。本轮范围为部位伤害、数据校验与反馈，保留原教学伤害规则。

## 实现与数据入口

`FPSHitDamage.*` 定义三类倍率和 `UFPSHitDamageProfile`。默认资产为 `/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default`，稳定 ID 为 `HitDamage.Default`。AK、BR3、HeavyPistol、M4、M9、MP5、UMP45 均显式引用该资产。首次迁移通过 Unreal Python 编辑器 API 完成；共享资产后来出现引用缺失，恢复与重新核对的证据见后文。脚本 `Tools/migrate_fps_hit_damage.py` 只为缺少引用的现有定义赋值，不重建原枪。

倍率可在该 Data Asset 的 `Multipliers` 中调整；需要另一套规则时创建独立 `FPSHitDamageProfile` 资产并设置唯一 `ProfileId`，在对应武器的 `HitDamageProfile` 属性选择它。三类倍率有限且非负，最终伤害保留浮点值。新资产必须满足 Asset Manager 扫描及身份校验。

武器资源集持有方案，`ContentDigest` 纳入稳定 ID 和实际倍率，ManifestVersion 升至 10；全项目检查发现无关无效方案也失败，本局仅校验其武器引用的方案。`FFPSWeaponShotContext` 在服务器开火提交时复制三类数值，命中后按明确区域计算；不是读取角色 `WeaponDamage` 显示缓存。实际 HitResult 写入 GAS EffectContext，确认消息携带爆头标记。

`FPSHitRegions.*` 管理独立 QueryOnly 命中体，只阻挡新通道 `FPSWeaponHit`。移动胶囊忽略此通道，仍保留 Visibility 响应供原教学路径使用；世界遮挡保持有效。头、躯干／颈／骨盆以及各手脚和肩部消费同一套第三人称程序姿态。专用服务器创建轻量 SceneComponent 节点，渲染端使用相同姿态驱动网格；死亡关闭命中体。当前尺寸是工业角色的代理体积，未把展示用骨骼样品误当作已接入角色。

`FPSCharacterPresentation::ActionPoseFor` 共用动作姿态计算，保留共享工作区新加入的手枪换弹位置调整，并同时用于专用服务器。爆头标记在原命中叉外加菱形，声音仍按现有普通命中／击杀二选一播放。

## 可追溯环境

UE 5.8.1，Win64 Development。共享编辑器处理其他资产工作期间，本轮使用 `Saved/HitDamageBuild` 的源代码、配置及 Content 副本构建。`Saved/HitDamageTests/BuildSnapshotFinal.json` 记录隔离验证输入和 DLL 哈希；各网络运行 `run.json` 另记录测试前后模块哈希校验。隔离验证 Editor DLL SHA256 为 `5d9bffe5a30e0b91671a02f20ed23d76ea5c05a319a05e53a480c6dff9b8f0ed`。共享编辑器退出后，补做根项目构建与实际运行，证据见表格最后两项；这些检查不代表其他并行功能全部验收。

初次 Editor 构建 C++ 链接成功，但资产尚未迁移，内容门禁失败；后续共享生成文档被其他构建重导出造成过期检查失败。资产迁移完成后，连同 Docs 一起冻结并从来源重导出，EditorBuild4、EditorBuild5、EditorBuild6 的完整构建和内置内容门禁通过。失败日志保留在 `Saved/Logs/HitDamage-EditorBuild1.log` 至 `EditorBuild3.log`。

## 本轮证据

| 检查 | 结果与入口 |
| --- | --- |
| 保存迁移 | `Saved/Logs/HitDamage-Migration.log`：7 个定义赋值，编辑器进程退出 0 |
| 方案与内容专项 | `Saved/Logs/HitDamage-ProfileTests.log`：默认值重载、零／负数／无穷、必需缺失／无效、无关错误分级、摘要变化和释放后 GC 均通过 |
| 专用服务器＋2 客户端 | `Saved/HitDamageTests/20261015-160803317/`：服务器和射手客户端 COMPLETE，通过头 50／躯干 25／四肢 18.75、遮挡、边界、快照、蹲伏、移动、伪造拒绝、死亡和反馈 |
| 监听服务器＋渲染客户端 | `Saved/HitDamageTests/20261015-160941282/`：完整专项通过；人工查看 Standing、Crouched、HeadKill 三张 Unreal 截图，确认姿态代理和菱形标记可见 |
| 倍率内容不一致 | `Saved/HitDamageTests/Regression-20261015-161116/Content/`：真实客户端摘要不一致，在生成角色前被拒绝，并释放内容句柄 |
| 原伤害／教学回归 | 上述 Regression 目录的 Damage、Teaching：双端 COMPLETE；原伤害仍不串用切换后的枪，教学射击和重生通过 |
| 最终版专用服务器＋2 客户端 | `Saved/HitDamageTests/20261015-181300436/`：最终共用姿态代码，`-PktLag=100`，服务器及射手客户端全部 PASS、COMPLETE |
| 最终版监听服务器＋渲染客户端 | `Saved/HitDamageTests/20261015-181339697/`：全部 PASS、COMPLETE；人工查看本轮 Standing、Crouched、HeadKill 截图，命中体随蹲伏更新，爆头菱形可见 |
| 最终版 Ability 回归 | `Saved/HitDamageTests/Final-20261015-181300/Abilities/`：按当前设置规则更新菜单检视断言后，服务器和客户端通过；汇总见同级 `run.json` |
| Game 构建 | `Saved/Logs/HitDamage-GameBuild.log` 及 `-console.log`：Win64 Development 链接成功、Result Succeeded；未据此声明 Cook／打包完成 |
| 当前共享资产校验与导出 | `Saved/Logs/HitDamage-CurrentAssetsVerified.log`：从根目录 Content 读取，SelfTest 通过、definitions=7；重新生成 `Docs/FPSWeaponAssetReview.md`，七个引用与 2／1／0.75 均可核对 |
| 根项目 Editor 构建 | `Saved/Logs/HitDamage-RootEditorBuild.log` 及 `-console.log`：Result Succeeded，内置内容门禁通过 |
| 根项目集成运行 | `Saved/HitDamageTests/20261015-182942770/`：直接运行根项目 Dedicated＋两个独立客户端，`-PktLag=100`，服务器和射手客户端全部 PASS、COMPLETE；模块、七个定义、倍率资产与碰撞／扫描配置运行前后哈希一致，见 `RootIntegration.json` |

最终 Dedicated 轮次注入 100 ms 包延迟，没有注入丢包；其余上述轮次无网络模拟。Dedicated 使用独立 `-server` 进程及两个独立客户端；listen 使用独立主机和客户端，渲染客户端采用 RenderOffscreen。测试将双方放到高处并关闭重力；射手 M9 仅在测试进程内关闭后坐力以隔离部位判定，未保存这些数值。实际开火仍经过客户端输入、GAS 和服务器射线；快照变化项明确是边界注入，不能解释为已经实现延迟子弹。截图核对覆盖当前程序角色站立与蹲伏，不代表全部动作姿态逐帧精度通过。

## 失败与修正

`20261015-160002548` 首轮几何检查通过，但脚本同帧转向并开火，服务器仍使用上一帧的已复制朝向；同时蹲伏绕过项目输入意图，客户端快照断言与服务器重置同刻。修正为提前瞄准、使用真实移动输入并分开时间点。

`20261015-160401617` 暴露 PlayerArray 在不同客户端顺序不保证一致，部分进程未选中射手；改为按复制的 PlayerId 选择，不修改游戏规则。上述两轮不计为通过。

扩展 Abilities 回归唯一失败项要求“开菜单立即取消检视”，与 [当前设置规则](../UI/SettingsDesignDecisions.md) 已确认的“已开始检视继续完成”冲突。保留游戏行为，将断言更新为检视继续、攻击／ADS 意图清空及之后正常完成；最终版双端复验通过。

## 共享资产集成记录

首次迁移后，共享定义于 2026-10-15 15:48 再次保存，后续检查发现七个新引用缺失；并行任务使用的模块及迁移状态不一致。使用最终 DLL、当前根目录 Content 的独立项目 `Saved/HitDamageCurrent` 复查，内容门禁明确拒绝这些缺失引用，日志为 `Saved/Logs/HitDamage-CurrentAssets.log`。随后通过编辑器 API 原地补回时，DA_AK 保存遭遇 Windows Error 32；`HitDamage-CurrentMigrationRepair.log` 保留失败证据。Windows Restart Manager 只读查询确认锁持有者是共享 UnrealEditor 进程，未强行终止或关闭其文件句柄。

共享编辑器所属任务正常退出后，七个文件均通过独占读取检查。用户授权发送协调消息，明确这些引用应保存，不能恢复为迁移前的 None。18:24 再次运行幂等迁移时，当前七个定义已恢复引用，日志 `HitDamage-CurrentMigrationRecovered.log` 为 `assigned=0 definitions=7`；这不是再次写入七个资产。18:26 当前资产校验、SelfTest 及重新导出成功。根目录二进制资产未被验证副本覆盖，保留其他工作线的字段改动。

共享根项目当时的一次构建因导出尚未完成而报 Asset review stale；重新导出后，本轮根项目构建和双端检查均通过。根项目运行模块 SHA256 为 `cf2944614ed53c4c7112ff818e495b841c7bf9d4bfd20cc11594b3dcbaae6bc9`；验证资产哈希记录在 `Saved/HitDamageTests/CurrentAssetsVerifiedHashes.json`，本次运行及前后校验记录在 `Saved/HitDamageTests/RootIntegration.json`。交付前 `python Tools/CheckDocs.py` 检查 84 份文档、0 错误，102 项历史豁免未扩大；新文档状态和证据入口另经人工核对。

## 尚未覆盖

完整 Cook／打包、长时压力、全部角色外观及所有动作姿态的逐帧精度、丢包与高延迟极限尚未验收。无延迟补偿；服务器当前姿态与延迟客户端画面不承诺逐帧重合。此记录不代表其他并行工作线或此前配件 A3–A6 全部完成。

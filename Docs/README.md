# 项目文档索引

- 用途：总索引
- 适用范围：项目 Docs 文档
- 状态：现有路径导航有效；已确认实体分类规则，旧文件迁移待单独执行
- 最后更新：2026-10-13

分类与维护以 [文档分类规则](DocumentationRules.md) 为准：先按 Demo/系统主题，再按用途细分。目前索引暂时保留旧路径，迁移时同步更新。“早期 Demo 制作阶段”和“枪械架构重构阶段”是两条不同时间线。

## 快速入口

- 创建、更新或归档文档：[文档分类与维护规则](DocumentationRules.md)。

- 整体进度：[FPS 路线图](FPSDemoRoadmap.md)。
- FPS 分类入口：[FPS 主题索引](FPS/README.md)；[统一美术](FPS/Art/README.md) 包含共同方向、Blender/Unreal 资产与待办；[UI 美术](FPS/UI/README.md) 包含正式 MiSans 字体、六页 UI/HUD 对齐、资产入口、当前运行包和实机验证，旧样稿与接入证据另行标识。
- 接手枪械：[设计决策](FPSWeaponRefactorDecisions.md) → [重构进度清单](FPSWeaponRefactorChecklist.md) → [纠正版架构评审与后续建议](FPSWeaponArchitectureReview.md)。
- 架构后续修正：[枪械维护主题](FPS/Weapons/README.md)，按 R1 → R2 → R5 → R3/R4 → R6/R7 推进。
- 角色与模式：[角色重构、配装和安全区](FPS/Characters/README.md)，三阶段实现、新包 14 项、实际 Dedicated 十客户端五分钟和连续切图通过；版本、证据与人工体验边界见主题验收记录。
- 移动重构：[移动与体力设计决策](FPS/Characters/MovementDesignDecisions.md)，2026-10-12 玩法与首轮参数已确认，待实施；既有角色验收不覆盖本轮。
- 最近交付：[重构第三阶段 BR-3](FPSWeaponRefactorStage3.md)。
- 保存的枪械参数：[自动生成的资产清单](FPSWeaponAssetReview.md)，禁止手工修改作为配置源。

系统设置：[UI 主题索引](FPS/UI/README.md) 中的系统设置设计与实施清单已确认，覆盖画面、声音、控制、键鼠重绑及界面舒适性；菜单清空意图但保留已启动动作的完成规则见设计正文。当前仅落文档，准星自定义另行设计。

## 枪械设计、实施与评审

| 文档 | 用途与状态 |
| --- | --- |
| [设计决策](FPSWeaponRefactorDecisions.md) | 已确认目标和玩法，评审建议不自动覆盖它 |
| [进度及验收清单](FPSWeaponRefactorChecklist.md) | 原三阶段交付记录与待补差距，不等于所有架构问题解决 |
| [第一阶段](FPSWeaponRefactorStage1.md) | 稳定身份、保存资产、校验及迁移证据 |
| [第二阶段](FPSWeaponRefactorStage2.md) | Ability、预测、复制与难度的局部验收 |
| [第三阶段](FPSWeaponRefactorStage3.md) | BR-3、9305 构建、双端和地图验收 |
| [架构评审（纠正版）](FPSWeaponArchitectureReview.md) | 静态复核、旧结论纠正、R1–R7 后续建议；未跑新测试 |
| [待拓展想法](FPSWeaponFutureIdeas.md) | 逐发装填、行为/动态难度等暂缓事项 |
| [资产清单（生成）](FPSWeaponAssetReview.md) | 由保存资产导出，用法见第一阶段，不是第二数据源 |
| [武器操作与早期背景](FPSWeapons.md) | 键位、基础行为及历史数值；结合最新阶段阅读 |

## FPS 功能与原创资源

| 文档 | 用途 |
| --- | --- |
| [路线图](FPSDemoRoadmap.md) | 最新状态入口和早期里程碑 |
| [大厅流程](FPSLobbyFlow.md) | 大厅、建房和加入流程及代码映射 |
| [战斗反馈](FPSCombatFeedback.md) | 受击、重生、HUD 和结算视角记录 |
| [人机](FPSBots.md) | 平面寻路与交战；Ability/难度新入口另见重构第二阶段 |
| [地图](FPSMaps.md) | 地图制作与早期验证；BR-3 台位增量见第三阶段 |
| [枪械美术](FPSWeaponArt.md) | 原六枪模型，BR-3 复用资源见第三阶段 |
| [角色美术](FPSAvatarArt.md) | 角色、手臂与程序动作制作 |
| [音效](FPSAudio.md) | 音频制作和早期接入；当前显式引用见重构记录 |

## 其他 Demo、资料和归档

| 文档/目录 | 用途 |
| --- | --- |
| [网络同步基础课](NetworkBasicsLesson.md) | 独立基础教学关卡 |
| [网络/GAS FPS 教学](NetSyncFPSDemo.md) | 原教学地图，不等同于新大厅 FPS 路线 |
| [知识锁策划](知识锁_玩法策划案.md) | 独立解谜玩法，其“不做战斗”不限制 FPS |
| [指令审计](AstraInstructionAudit/README.md) | 指令审计和相关备份，不是玩法规范 |
| [历史归档](Archive/README.md) | 被替代的评审，仅用于追溯 |
| [大厅流程图](Images/FPSLobbyFlow.png) | 大厅流程配图，保留原资源位置 |

## 阅读与维护规则

统一规则已集中到 [DocumentationRules.md](DocumentationRules.md)，此处不再重复维护。

角色与模式本轮已完成完整 Cook/打包、实际 Dedicated 十客户端五分钟及专项自动运行验收，范围见 [扩展验收](FPS/Characters/ExtendedAcceptance.md)。真人体验、性能压力认证和独立 Server Target 包仍待验证；其他主题的完成状态以各自记录为准。

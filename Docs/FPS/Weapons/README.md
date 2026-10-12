# 枪械架构与配件扩展

- 用途：主题索引
- 适用范围：FPS / Weapons 架构维护与模块化配件扩展
- 状态：R1–R7 针对性验收完成，完整打包与压力范围见各记录；配件 A1 数据基础与 A2 工作台已实施，当前验证边界见记录；历史文档保留原路径
- 最后更新：2026-10-13

架构维护阅读顺序：[重构决策](../../FPSWeaponRefactorDecisions.md) → [纠正版评审](../../FPSWeaponArchitectureReview.md) → [架构任务](ArchitectureTasks.md)。

- [原三阶段清单](../../FPSWeaponRefactorChecklist.md)：历史交付与已有差距。
- [R1 状态所有权记录](StateOwnership.md)：本轮修改、针对性证据与未验证项。
- [R2 射击调度记录](FireScheduling.md)：内部 Equip、等待通知与射击周期。
- [R5 射击上下文记录](ShotContext.md)：单次权威快照与 GAS 伤害语义。
- [R3＋R4 内容生命周期记录](ContentLifecycle.md)：本局依赖、内容准入及资源持有。
- [R6＋R7 表现记录](Presentation.md)：动作结果、预测去重及独立过渡时间。
- [扩展验收与结构整理](ExtendedAcceptance.md)：Cook/打包、长局、Dedicated Server、多客户端及网络专项。

配件扩展阅读顺序：[配件设计决策](AttachmentDesign.md) → [配件任务清单](AttachmentTasks.md) → [后续方向](AttachmentFutureIdeas.md)。这是现有架构上的新工作线，不重新执行旧评审问题列表。

- [配件设计决策](AttachmentDesign.md)：已确认的平台接口、单一数据来源、训练改装、换弹取消、预设发枪、补给与美术保存规则。
- [配件任务清单](AttachmentTasks.md)：M4 首批三类配件、策划工具及训练交付的 A1–A6 阶段。
- [A1 定义与内容准入记录](AttachmentFoundation.md)：本轮实际实现、资产与双端验证边界。
- [A2 策划工作台记录](AttachmentAuthoring.md)：编辑器入口、保存与撤销、实际 UI 回归和边界。
- [A3 手动改装事务记录](AttachmentTransactions.md)：实现与正在执行的双端、界面和 PIE 验证；阶段尚未验收。
- [配件资产清单](AttachmentAssetReview.md)：由保存资产生成的只读审查资料。
- [配件与弹药后续方向](AttachmentFutureIdeas.md)：实体物品、不同弹种、嵌套接口、可切换倍率和完整搜打撤；未纳入首次交付。
- [总索引](../../README.md)。

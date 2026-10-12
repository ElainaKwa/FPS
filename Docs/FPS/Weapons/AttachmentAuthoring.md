# 配件 A2：策划工作台实施与验证

- 用途：实施与验证记录
- 适用范围：武器／配件定义的 Unreal 编辑器工具，不包含游戏内改装
- 状态：Editor／Game 构建及实际编辑器针对性回归通过
- 最后更新：2026-10-13

依据：[配件设计](AttachmentDesign.md)、[任务清单](AttachmentTasks.md)。A1 数据和网络证据见 [数据基础记录](AttachmentFoundation.md)，不能用于证明本阶段 UI 已通过。

## 实现与入口

`FPSWeaponAuthoring.cpp` 提供模板复制、完整批次预校验、事务写回和保存；`FPSWeaponWorkbench.cpp` 提供四个页面。模块只在编辑器注册，使用 `WITH_EDITOR` 隔离 PropertyEditor、AssetTools、ToolMenus。对象持有使用 `TObjectPtr`，支持增量 GC。

编辑器 **工具 → 枪械配件工作台**，或控制台 `FPS.OpenWeaponWorkbench`：

- 武器表／配件表：搜索稳定 ID、平台、接口、类别；空格分隔多条件。中文列名附单位，悬停查看字段名、范围与用途，横向滚动查看更多列。
- 单元格提交直接修改定义资产，`*` 表示未保存；支持撤销／重做，右侧详情编辑其他字段，双击行打开专用资产编辑器。
- 从选中模板创建：复制定义、生成新的稳定 ID，保留明确共享的模型、曲线及声音引用；副本初始未保存。稳定 ID 在工作台详情中只读。
- 批量修改：Ctrl／Shift 多选，输入悬停提示中的字段名及新值，先展示受影响资产路径，再确认应用。整个批次先在临时候选上验证，失败不修改任何原资产。
- 装配预览：使用与运行数据相同的解析器计算，不保存装配。只提供武器显式声明的配件。
- 全项目校验：显示错误和对应资产入口。保存前再次校验；多包磁盘写入若中途失败，会列出已经保存的包，不声称磁盘保存可整体回滚。

没有引入可独立编辑的 Excel／JSON／DataTable 参数副本。保存后应执行已有数据导出工具更新只读报告，构建门禁会拒绝过期武器报告。

## 本轮实际验证

最终 Editor 构建：`Saved/Logs/AttachmentA2-BuildFinal.log`、`AttachmentA2-BuildFinal-Console.log`，结果 Succeeded，内容门禁 7 个武器定义通过。

Game Development 构建：`Saved/Logs/AttachmentA2-GameBuild.log`、`AttachmentA2-GameBuild-Console.log`，Succeeded，确认编辑器专用代码和依赖未阻止 Game 目标编译；这不是 Cook／打包验收。

实际运行 `Tools/TestFPSWeaponWorkbench.ps1 -RunId A2-EditorFinal-20261013`。启动的是完整 `UnrealEditor.exe`，使用 `-RenderOffscreen -unattended` 和 D3D12；不是 commandlet 或仅编译。Slate 键盘事件操作真实表格，截图来自实际窗口内容。最终日志为 `Saved/FPSAttachmentTests/A2-EditorFinal-20261013/Editor.log`，`FPS_WORKBENCH_TEST PASS COMPLETE`，进程退出码 0。

验证覆盖：

- 从 M4 模板创建不同稳定 ID 的副本，共享显式资源；真实单元格输入 `1.75` 并提交。
- 撤销恢复原值、重做恢复新值；拒绝尾随非法字符、小数容量和包含无效后续项的批次，原值不变。
- 注入未保存的非法伤害，校验页定位对应定义；恢复合法值后保存、卸载包并重新加载，数值保持。
- 测试副本通过编辑器删除并确认包文件消失；原有资产与 A1 结束时指纹一致，没有保存覆盖原枪定义或地图。
- M4 固定瞄具＋制退器＋扩容弹匣预览得到容量 45、换弹 2.415 秒、ADS 0.23 秒、FOV 48、后坐力倍率 0.8／0.85；最终校验页 0 项错误。
- 人工查看 `Weapons.png`、`Attachments.png`、`Preview.png`、`Validation.png`：中文和数值可读，长表可横向滚动。前三张底部保留上一次故障注入的校验消息，最终校验页重新计算后为 0；消息不是持续自动校验。

版本入口 `Saved/FPSAttachmentTests/A2-Version.json`。本次 Editor 模块 SHA-256 为 `bdfe8899f0651f81e37022dfc406d1c4ce82851d72b86f61d33b6270b6706bf6`；后续构建会替换二进制，不能用该日志证明后续变更。

## 修复过程与验证边界

初次 UI 测试发现测试代码只删除对象、未清理保存包，以及按列表位置选到了原装配件；已改为编辑器完整包清理和按稳定 ID 选配。Python 强制删除接口曾返回成功但留下文件，实际磁盘检查将它判为失败；最终由 C++ 编辑器流程清理。中间内容门禁因残留测试副本使报告过期而失败，未扩大豁免或导出该副本掩盖错误。中间记录保留在 `A2-Editor-20261012`、`A2-EditorFix-20261013` 和相关 Build／Cleanup 日志。

截图审查后修复英文列名截断和无意义的浮点尾数；最终构建修复增量 GC 原始指针警告，并补充错误定位测试。完整编辑器启动日志另有 13 条启动期 `LogAutomationTest: Error: Condition failed`，发生在工作台测试之前，当前未定位其来源；本记录不把日志称为零错误。工作台断言全部通过，没有 Fatal／Assert，退出正常。

未覆盖人工多选后确认弹窗的鼠标点击、任意外部资产编辑器同时修改、超大资产量、Cook／完整打包。A2 未修改运行时网络／GAS 行为；A3 的真实双端与 PIE 退出验证仍须独立执行。工作台可用不代表游戏内改装已可用。

## 弹药箱交接核对

用户已交付 `SM_AmmoSupplyCrate`，保留实际路径 `/Game/FPSDemo/Art/Environment/AmmoSupplyCrate/Meshes`。制作来源见 [资产交接说明](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/README.md) 和 [美术记录](../Art/AmmoSupplyCrate.md)。

2026-10-13 从当前资产只读检查：大小约 78.8 × 52.55 × 42.95 厘米，最低 Z 为 0，1 个简单碰撞，6 个材质槽。记录为 `Saved/FPSAttachmentTests/CrateInspection-20261013.json`、`Saved/Logs/Attachment-CrateInspection2.log`，标记 `AMMO_CRATE_CURRENT_INSPECTION_PASS`，退出码 0。首次 commandlet 子系统获取失败后改用相同引擎类实例重跑；未改动模型。此项是资源接收核对，补给逻辑、训练地图放置及游戏视觉仍在 A5。

返回 [枪械索引](README.md)。

# 搜打撤与个人工厂 UI 接入

- 用途：当前使用说明
- 适用范围：单人 PvE 搜打撤实验的工厂总览、实体格子仓库/整备背包、装备槽、加工、改装、撤离及结果
- 状态：v2 实体格子和 v3 中央改枪台已保存；独立游戏运行与三分辨率采图通过，完整 OS 输入路由和打包未验证
- 最后更新：2026-10-14

## 当前方向与资产

用户在初版列表终端后明确改为参考《三角洲行动》特勤处的基地结构与背包交互：机器人个人工厂、不同尺寸实体占格、拖放整理、仓库/出征背包/装备栏分离。沿用工业灰黑、有限黄、MiSans 与 Rajdhani，使用自制设施几何与物品名称缩写；不复制商业游戏素材。不增加胸挂、保险箱、负重或未实现的设施升级按钮。

本套件资产位于 `/Game/FPSDemo/Art/UI/ExtractionTerminal/`：`WBP_ExtractionTerminal` 保存终端布局，`WBP_ExtractionRow` 用于配方与结果，v3 新增窄版 `WBP_ExtractionAttachmentRow` 用于配件。终端 v2 保存工厂导航、总览、三个原生格子控件、固定装备栏、背包和结果面板；v3 仅追加改枪台面板。格子控件的容器、CellSize、字体、网格色、物品底色均作为 Widget Blueprint 属性保存；占格和文字由真实快照绘制，不每 0.1 秒重建一套物品子控件。

代码归属：[FPSExtractionUI.h](../../../Source/MCP_Test/FPSExtractionUI.h)、[FPSExtractionUI.cpp](../../../Source/MCP_Test/FPSExtractionUI.cpp)、[FPSExtractionGridUI.h](../../../Source/MCP_Test/FPSExtractionGridUI.h)、[FPSExtractionGridUI.cpp](../../../Source/MCP_Test/FPSExtractionGridUI.cpp)。本 UI 工作不修改后端、类型、旧 UI、控制器或地图；这些由主聊天统一接入。[资产源入口](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/README.md)记录生成方式及共享依赖。

## 页面与真实状态

| 页面 | 内容与入口 | 数据和动作 |
| --- | --- | --- |
| 工厂总览 | 仓储区、加工区、武器台三处设施入口；实际库存、整备背包、生产槽、配件数 | `Overview`、`EnterWarehouse`、`EnterProduction`、`EnterModification` |
| 仓库与整备 | 左侧 8×8 仓库、右侧 5×4 出征背包；选择详情、旋转、跨容器转移、整堆出售 | `WarehouseGrid`、`BaseBackpackGrid`；`MoveStack`、`AutoTransferStack`、`SellStack` |
| 固定装备栏 | 主武器 M4、副武器 M9，真实弹药、配件数和补给 10 资金；不暗示武器可拖动换槽 | `WeaponSummary`、`PrimarySlot`、`SecondarySlot`、`Resupply`、`Depart` |
| 加工制造 | 实际配方材料、资金、时间、产物；单生产槽与领取 | `BaseRows`、`Claim`；`StartProduction`、`ClaimProduction` |
| 枪械改装 | 中央真实 3D 武器与槽位连线，右侧兼容库存，底部当前参数/候选差异 | `WeaponViewport`、`AttachmentRows`；`InstallAttachment(ItemId)`、`RemoveAttachment(AttachmentId)` |
| 局内轻 HUD | 真实容量、交互文字、撤离米数或倒计时 | `RaidCapacity`、`Interaction`、`ExtractionStatus`；不重复旧生命/弹药/体力 HUD |
| 局内背包 | 5×4 真实格子、选择详情、旋转、整堆丢到地面、放弃确认 | `RaidBackpackGrid`、`RaidRotate`、`SelectedDrop`；`DropStack`、`AbortExpedition` |
| 行动结果 | 成功时全部当前背包入库；失败时仅展示实际遗失搜获，恢复出征前整备 | `LastHaul`、`Feedback`、`ReturnToBase` |

`Warehouse/Backpack` 是实体堆叠数组，同 ItemId 可以重复。选择、移动、出售、丢弃使用 StackId，不用 ItemId 查找第一项代替。容量直接显示后端已计算的真实占用面积；不重新使用旧“数量除堆叠上限”的格数算法。材料和配件尺寸来自目录 Width/Height；旋转交换实际占格宽高。配件安装仍按已有 ItemId 接口取一件，由后端决定具体扣除。

死亡文案遵循新规则：恢复出征前整备背包，仅丢本次搜获；枪械及已装配件保留，无维修费。成功将当前背包全部自动入库。UI 不自行计算遗失清单，使用后端已扣除出征整备部分的 LastHaul。

## 拖放和刷新

左键选择物品后，格子 `NativeOnMouseButtonDown` 请求 DetectDrag；`NativeOnDragDetected` 建立 `UFPSExtractionGridDrag`，携带 From、StackId、ItemId、Quantity、宽高、旋转状态和相对抓取格 GrabX/GrabY。目标坐标等于鼠标所在格减去抓取格；不强制以物品左上角贴鼠标。

目标绿色/红色占格预览检查阶段、边界、重叠。与后端一致，同类且能容纳整个来源数量的目标允许整组合并；容量不足拒绝，不部分转移。最终放下仍调用后端 MoveStack，由后端再次验证并原子提交。取消拖拽、越界拒绝或移出网格不会先删掉来源物资。

`SelectedRotate` / `RaidRotate` 提供可点击旋转按钮；R 键在面板 PreviewKeyDown 中调用同一路径，拖动中是否由实际 Slate 路由送达须实机核对，按钮为可用入口。旋转拖动载荷时同步变换抓取格；连续旋转两次回到原朝向和偏移。失败后的具体原因读取 Feedback。

每 0.1 秒刷新快照只更新原生格子的数组和文字，不销毁格子，不覆盖 ActiveDrag。列表仅在相关数据变化且没有拖拽时刷新。格子必须是 Visible；父面板可为 SelfHitTestInvisible。换阶段或关闭背包时取消拖拽。后端完成移动后更新选中容器；合并导致来源 ID 消失时清除失效选择。

## 控制器接口

```cpp
UI = UFPSExtractionUI::Open(this, true);
if (UI) UI->AddToPlayerScreen(40);
UI->SetPanelOpen(!UI->IsPanelOpen()); // Raid 的 Tab 开关
```

Open 不自动加入视口或更改输入模式。控制器负责鼠标、移动/视角/开火限制和 Tab/F 映射。UI 内关闭按钮会改变 IsPanelOpen，控制器应持续同步状态。已打开背包中的 Tab/Esc 在 NativeOnPreviewKeyDown 中消费并返回 Handled，避免焦点位于按钮后无法关闭或控制器双切换。基地 Esc 返回工厂总览；活动拖拽时先取消。打开背包不暂停世界。

## v3 中央 3D 改枪台

改装页独占原中部和装备侧栏的区域；离开改装页后，原侧栏和其他页面照常显示。`ModificationPanel` 追加在既有 BasePanel 中，默认折叠；只在基地 Tab=3 展开。主聊天维护 [FPSExtractionWeaponViewport.h](../../../Source/MCP_Test/FPSExtractionWeaponViewport.h) 与其实现，负责真实 RenderTarget 模型、槽位连线、右键旋转和滚轮缩放。UI 在 NativeConstruct 绑定 OnSlotSelected，在 NativeDestruct 解除绑定并停用预览；Refresh 只传入实际 `V.Loadout.Primary`。新预览模型属实验展示资源，不改变原枪械数据或原始资产。

默认选择瞄具；三个分类按钮和模型槽位点击调用同一 `SelectAttachmentCategory`。右侧按仓库实际数量合并同 ItemId，仅列当前类别且通过 Catalog.Resolve 的兼容配件；当前已装配件单列卸下按钮。安装按钮使用 ItemId，卸下按钮使用 AttachmentId，直接走后端实际操作，没有免费试装或二次确认应用状态。操作成功后的实际快照驱动模型更新。

悬停按钮只计算参数差异：复制已装配件 ID，替换同类别配件或移除当前配件，然后调用 `UFPSWeaponCatalog::Resolve`。容量、换弹时间、开镜时间和纵横后坐力倍率均由解析结果得到；不会将候选快照传给模型或写入库存。移出按钮、换类别或操作后清除候选参数。当前值与差异不代替后端验证。

| 稳定控件/接口 | 类型和测试契约 |
| --- | --- |
| `WeaponViewport` | `UFPSExtractionWeaponViewport`，必须 Visible；点击模型槽位触发 OnSlotSelected，`GetSelectedAttachmentCategory()` 可读回选择 |
| `SlotOptic` / `SlotMuzzle` / `SlotMagazine` | `UFPSExtractionButton`，Action 与控件名相同 |
| `AttachmentRows` | ScrollBox，每张 `WBP_ExtractionAttachmentRow` 中有 RowTitle、RowDetail、RowCount、RowAction、RowActionLabel、RowReason |
| `RowAction` | Install 的 ItemId 为库存物品 ID；Remove 的 ItemId 为配件 ID。OnHovered/OnUnhovered 显示/清除差异，OnClicked 实际安装/卸下 |
| `CurrentWeaponStats` / `CandidateWeaponStats` | TextBlock，当前解析值和候选值/差异；`CandidateStatsTitle` 标明安装后或卸下后 |
| `WeaponWorkbenchLayoutVersion` | 隐藏 TextBlock，值 3；既有 InventoryLayoutVersion 保持 2 |

1600×900 设计基准中，Viewport 为 `(308,275)`、`862×355`；右侧列表为 `(1198,347)`、`336×405`。原 Feedback、Depart、导航和其他页面布局不动。窄版配件卡为 320×206，留出滚动条宽度；字体沿用共享 MiSans/Rajdhani。

## 资产生成与显式升级

首次创建运行 [Tools/build_extraction_ui.py](../../../Tools/build_extraction_ui.py)，其调用套件源脚本和 C++ BuildAssets。已有 v1 资产不能仅重跑创建脚本；应在统一编译后运行 [Tools/upgrade_extraction_ui_inventory.py](../../../Tools/upgrade_extraction_ui_inventory.py)，源脚本为 [upgrade_extraction_ui_inventory.py](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Scripts/upgrade_extraction_ui_inventory.py)。

这次是用户授权的页面结构重做：升级前备份本套件旧终端，再仅重建 WBP_ExtractionTerminal 的控件树，保留行资产、旧 Industrial UI、字体、主题、地图和其他资源。版本标记 InventoryLayoutVersion=2；重复运行不覆盖后续 Designer 修改。脚本检查新控件、网格类型/容器/尺寸/Visible、字体引用及其他资产和地图哈希，记录到 `Saved/FPSExtraction/UI/InventoryUpgrade/<时间>/Report.json`。旧侧栏定向脚本只适用于 v1，不要用于 v2。

当前生成源的 WeaponSummary 高度为 116，超过上一版实测 96.667 的 desired height；副武器与补给区另留空间。数字资金及格中数量引用现有 F_UI_Number，中文引用现有复合 F_UI_Text。没有新增字体副本。

v2 → v3 在统一编译后运行 [Tools/upgrade_extraction_ui_workbench.py](../../../Tools/upgrade_extraction_ui_workbench.py)，转发至 [套件源脚本](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Scripts/upgrade_extraction_ui_workbench.py)。入口为 `FPSExtractionUIBuilder::UpgradeWeaponWorkbenchAssets`：要求已有 v2，备份原终端及已有窄版行，追加独立 ModificationPanel，创建缺失的窄版行，不重建旧控件树。原行资产、其他 UI、字体、主题和地图均保持不变。版本 3 重跑不覆盖 Designer 修改；名称冲突拒绝迁移，不强行覆盖。

脚本比较旧控件父子关系、Canvas 槽位、可见性、文本和字体，检查 Content 中既有资产/地图哈希、增量资产范围和新控件契约。报告入口为 `Saved/FPSExtraction/UI/WorkbenchUpgrade/<时间>/Report.json`。这属于编辑器保存与同进程检查，独立回载、Slate 交互和截图由主聊天单独完成。创建脚本只创建缺失资产；已有 v2 使用升级入口。

## 自动验证入口与当前边界

主聊天的 `FPSExtractionUICapture.cpp` 负责独立 UI 存档和实机事件测试，本 UI 工作不修改此文件。可稳定访问的格子为 WarehouseGrid、BaseBackpackGrid、RaidBackpackGrid。事件 override 均为 public：NativeOnMouseButtonDown、NativeOnDragDetected、NativeOnDragOver、NativeOnDrop；GetCachedGeometry 和 CellSize 可将格中心换算成实际屏幕位置。正常事件和开发测试使用同一路径，没有只为测试绕过 UI 的移动接口。

应覆盖：从物品第二格抓取、旋转后拖入、越界拒绝、拖回仓库、Refresh 不结束拖拽、同类合并及容量不足拒绝、同 ItemId 多实体只操作选中 ID、Raid 不可访问仓库；再核对 Tab/Esc、整堆出售/丢弃、整备带入、失败恢复和成功入库。视觉采图至少为 Overview、Warehouse、Production、Modification、RaidBag、Result，1280×800 / 1920×1080 / 2560×1440。

v2 三分辨率、原生拖放处理、闭环与读档验证通过。v3 编译、增量资产升级、独立回载及三分辨率默认/瞄具/扩容/旋转采图通过：`Saved/ExtractionTests/UI-20261014-013916904/run.json`。覆盖模型挂点分类、真实按钮装卸、候选差异、库存扣还、模型替换、旋转与缩放；沿用正式处理器，但未替代完整操作系统输入路由、真人体验或打包验收。具体模块哈希、修复和验证边界见 [实施记录](../Extraction/Implementation.md)。

返回 [UI 索引](README.md)。

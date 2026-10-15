# 搜打撤与个人工厂 UI 接入

- 用途：当前使用说明
- 适用范围：单人 PvE 搜打撤实验的行动大厅、实体格子仓库/整备背包、装备槽、加工、改装、撤离及结果
- 状态：v4 双层改枪台与 v1 行动大厅已保存；主项目构建、武器数据校验及大厅双分辨率入口验证通过；完整 OS 输入路由、PIE 和打包未验证
- 最后更新：2026-10-15

## 当前方向与资产

用户在初版列表终端后明确改为参考《三角洲行动》特勤处的基地结构与背包交互：机器人个人工厂、不同尺寸实体占格、拖放整理、仓库/出征背包/装备栏分离。沿用工业灰黑、有限黄、MiSans 与 Rajdhani，使用自制设施几何与物品名称缩写；不复制商业游戏素材。不增加胸挂、保险箱、负重或未实现的设施升级按钮。

本套件资产位于 `/Game/FPSDemo/Art/UI/ExtractionTerminal/`：`WBP_ExtractionTerminal` 保存终端布局，`WBP_ExtractionRow` 用于配方与结果，`WBP_ExtractionAttachmentRow` 用于配件。v4 改枪台保留；大厅升级另用 LobbyLayoutVersion=1，只替换 OverviewPanel，格子仓库、加工、装备栏、背包及结果页沿用现有内容。格子控件的容器、CellSize、字体、网格色、物品底色均作为 Widget Blueprint 属性保存；占格和文字由真实快照绘制，不每 0.1 秒重建一套物品子控件。

代码归属：[FPSExtractionUI.h](../../../Source/MCP_Test/FPSExtractionUI.h)、[FPSExtractionUI.cpp](../../../Source/MCP_Test/FPSExtractionUI.cpp)、[FPSExtractionGridUI.h](../../../Source/MCP_Test/FPSExtractionGridUI.h)、[FPSExtractionGridUI.cpp](../../../Source/MCP_Test/FPSExtractionGridUI.cpp)。大厅场景展示由 [FPSExtractionLobby](../../../Source/MCP_Test/FPSExtractionLobby.cpp) 管理，通过既有工厂控制器打开和关闭；不修改库存规则或重建工厂地图。[资产源入口](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/README.md)记录生成方式及共享依赖。

## 页面与真实状态

| 页面 | 内容与入口 | 数据和动作 |
| --- | --- | --- |
| 行动大厅 | 中央真实机器人和工厂背景；顶部出征/仓库/工厂/改枪台，右上实际资金与设置，右下当前地图及整备/出发 | `Overview`、`LobbyWarehouse`、`LobbyFactory`、`LobbyModification`、`LobbySettings`、`LobbyPrepare`、`LobbyDepart`；加工入口 `LobbyProduction` |
| 仓库与整备 | 左侧 8×8 仓库、右侧 5×4 出征背包；选择详情、旋转、跨容器转移、整堆出售 | `WarehouseGrid`、`BaseBackpackGrid`；`MoveStack`、`AutoTransferStack`、`SellStack` |
| 固定装备栏 | 主武器 M4、副武器 M9，真实弹药、配件数和补给 10 资金；不暗示武器可拖动换槽 | `WeaponSummary`、`PrimarySlot`、`SecondarySlot`、`Resupply`、`Depart` |
| 加工制造 | 实际配方材料、资金、时间、产物；单生产槽与领取 | `BaseRows`、`Claim`；`StartProduction`、`ClaimProduction` |
| 枪械改装 | M4/M9 实例概览与真实参数；进入 M4 独占改装画布，点槽位打开配件抽屉 | `WeaponViewport`、`AttachmentDrawer`、`AttachmentRows`；`InstallAttachment(ItemId)`、`RemoveAttachment(AttachmentId)` |
| 局内轻 HUD | 真实容量、交互文字、撤离米数或倒计时 | `RaidCapacity`、`Interaction`、`ExtractionStatus`；不重复旧生命/弹药/体力 HUD |
| 局内背包 | 5×4 真实格子、选择详情、旋转、整堆丢到地面、放弃确认 | `RaidBackpackGrid`、`RaidRotate`、`SelectedDrop`；`DropStack`、`AbortExpedition` |
| 行动结果 | 成功时全部当前背包入库；失败时仅展示实际遗失搜获，恢复出征前整备 | `LastHaul`、`Feedback`、`ReturnToBase` |

`Warehouse/Backpack` 是实体堆叠数组，同 ItemId 可以重复。选择、移动、出售、丢弃使用 StackId，不用 ItemId 查找第一项代替。容量直接显示后端已计算的真实占用面积；不重新使用旧“数量除堆叠上限”的格数算法。材料和配件尺寸来自目录 Width/Height；旋转交换实际占格宽高。配件安装仍按已有 ItemId 接口取一件，由后端决定具体扣除。

死亡文案遵循新规则：恢复出征前整备背包，仅丢本次搜获；枪械及已装配件保留，无维修费。成功将当前背包全部自动入库。UI 不自行计算遗失清单，使用后端已扣除出征整备部分的 LastHaul。

## 行动大厅

`Tools/LaunchFPSExtraction.ps1` 现在进入个人工厂地图，并用 `-FactoryLobby` 打开大厅；`Tools/LaunchFPSFactory.ps1` 保持从可走动车间开始。车间中 Tab 打开大厅，终端中 Tab 返回车间。大厅“工厂”同样关闭终端，恢复原机器人、原位置和行走视角；不会用展示角色替换玩家角色。

大厅参考用户提供的《三角洲行动》布局，使用本项目工业灰黑、黄色、MiSans/Rajdhani 和既有机器人。`OverviewPanel` 是根 Layout 的子项，显示时隐藏旧 BasePanel；其他页面按原契约访问。右下只显示当前已实现的工业关隘，没有虚构多地图选择器、活动、赛季或社交入口。整备进入已有仓库与背包，出发调用 `StartExpedition`；从此入口出征后，结算仍通过原流程返回工厂大厅。

`DA_LobbyPresentation` 保存工厂相机标签、镜头和目标相对偏移、机器人位置/朝向和 FOV；实际展示读取 FactoryCamera0，临时展示演员在离开大厅时销毁。两张 UI 纹理为 `Textures/T_LobbyVignette` 和 `Textures/T_LobbyIndustrialCrossing`，后一张来自本项目工业关隘既有全景截图。共享字体、机器人网格及工厂地图没有复制或重建。

已有 v4 终端运行 [大厅定向升级](../../../Tools/upgrade_extraction_ui_lobby.py)。先备份再仅替换 OverviewPanel 和大厅版本标记，同时把旧侧栏返回项改称“行动大厅”；脚本比对其他控件和资源。普通重跑保留 Designer 编辑，`-LobbyRebuild` 仅用于明确的大厅重建。场景资产可单独编辑，不随重建重置。实机证据与验证限制见 [大厅实施记录](LobbyImplementation.md)。

## 拖放和刷新

左键选择物品后，格子 `NativeOnMouseButtonDown` 请求 DetectDrag；`NativeOnDragDetected` 建立 `UFPSExtractionGridDrag`，携带 From、StackId、ItemId、Quantity、宽高、旋转状态和相对抓取格 GrabX/GrabY。目标坐标等于鼠标所在格减去抓取格；不强制以物品左上角贴鼠标。

目标绿色/红色占格预览检查阶段、边界、重叠。与后端一致，同类堆叠存在余量时允许补满目标，预览提示本次可合并数量，多出的物资保留来源原 ID、位置、方向及选择。已满目标拒绝。最终放下仍调用后端 MoveStack，由后端再次验证并原子提交。取消拖拽、越界拒绝或移出网格不会先删掉来源物资。

Ctrl+左键点击调用既有 `SelectedTransfer` 快速互转，目标剩余总空间不足时整个操作拒绝。右键点击多件堆叠打开 `UFPSExtractionSplitDialog`：默认拆一半，可输入 1 至当前数量减 1 的整数；确认才调用 `SplitStack`。弹窗使用现有字体和主题，在独立的顶层 UMG 中拦截鼠标与 Esc；取消、点击遮罩、关闭背包或源数量变化会关闭弹窗且不修改库存。原生格子控件承接这些交互，无须重建现有 WBP。该迭代的验证入口为 [TestFPSInventory.ps1](../../../Tools/TestFPSInventory.ps1)，结果见 [实施记录](../Extraction/Implementation.md)。

`SelectedRotate` / `RaidRotate` 提供可点击旋转按钮；R 键在面板 PreviewKeyDown 中调用同一路径，拖动中是否由实际 Slate 路由送达须实机核对，按钮为可用入口。旋转拖动载荷时同步变换抓取格；连续旋转两次回到原朝向和偏移。失败后的具体原因读取 Feedback。

每 0.1 秒刷新快照只更新原生格子的数组和文字，不销毁格子，不覆盖 ActiveDrag。列表仅在相关数据变化且没有拖拽时刷新。格子必须是 Visible；父面板可为 SelfHitTestInvisible。换阶段或关闭背包时取消拖拽。后端完成移动后更新选中容器；合并导致来源 ID 消失时清除失效选择。

## 控制器接口

```cpp
UI = UFPSExtractionUI::Open(this, true);
if (UI) UI->AddToPlayerScreen(40);
UI->SetPanelOpen(!UI->IsPanelOpen()); // Raid 的 Tab 开关
```

Open 不自动加入视口或更改输入模式。控制器负责鼠标、移动/视角/开火限制和当前有效按键映射。UI 内关闭按钮会改变 IsPanelOpen，控制器应持续同步状态。已打开背包的关闭按键在 NativeOnPreviewKeyDown 中消费，避免焦点位于按钮后失效或双切换。v4 改枪台 Esc 逐层关闭抽屉、返回选枪、返回工厂；其他基地页面保持现有设置入口，活动拖拽时先取消。打开背包不暂停世界。

## v4 参考布局与导航

用户 2026-10-15 提供两张《三角洲行动》截图，明确要求改为两层结构。参考其大面积武器展示、左侧实例与参数、围绕枪体的槽卡布局；保留本项目 MiSans/Rajdhani、工业灰黄和已接入的槽位能力，不复制 Logo、贴图、皮肤或原游戏功能。

进入改枪台时折叠 BasePanel，显示与它同级的 ModificationPanel；这会临时隐藏工厂侧栏、标题和出征入口，不更改原控件布局。概览展示实际持有的 Primary / Secondary 实例。左侧 M4/M9 卡片复用已有剪影，参数读取对应 WeaponDefinition 和 Catalog.Resolve：伤害、最大射程、射速、容量、换弹、开镜、开火模式和后坐力倍率。右侧真实模型显示当前实际装配，M9 没有受支持的改装接口时明确说明并禁用“进入改装”。不新造枪械实例，不更改玩家活动武器槽。

M4 进入改装后，左侧卡片和属性退出，保留顶部简洁导航与中央模型。Viewport 绘制瞄具、枪口、弹匣、后握把、枪托、枪管六张槽卡，并通过 OnSlotSelected 通知 UI；点击后打开右侧抽屉，含当前已装配件、兼容仓库库存、当前参数及悬停候选差异。安装/卸下仍立即扣还实际库存，模型只接收操作后的真实快照。枪身、内置部件、已装部件和挂点从武器／配件定义读取，射程加入当前值和候选差异。新增三类的实现及验证边界见 [三部位接入记录](../Weapons/ModularParts.md)。关闭抽屉恢复大画布，不创建待应用试装状态。未加入精校、皮肤、方案导入或空的靶场入口。

1600×900 布局基准：概览视口 `(480,150)`、`1080×650`；改装视口 `(40,150)`、`1520×650`；抽屉打开时视口改为 `1150×650`，抽屉位于 `(1210,150)`、`350×650`。三种视口宽度均超过屏幕宽度 65%，抽屉不覆盖模型视口。已保存的 Viewport 槽位为完整改装尺寸，运行时随层级调整。相同类别的槽卡在抽屉关闭后仍可再次打开。

| 操作 / API | 行为与测试契约 |
| --- | --- |
| `Dispatch("Modification")` | 从工厂进入武器概览，默认 Primary，抽屉关闭 |
| `ViewPrimaryWeapon` / `ViewSecondaryWeapon` | 概览选择真实 M4 / M9 实例并重置视角；不切换玩家装备 |
| `EnterWeaponModification` | 仅允许支持改装的当前主武器进入；M9 保持禁用 |
| `SelectAttachmentCategory(Category)` | 改装态槽卡点击入口；打开对应抽屉，当前类别由 GetSelectedAttachmentCategory 读取 |
| `CloseAttachmentDrawer` | 只收起抽屉、清除候选参数，仍在改装层 |
| `WorkbenchOverview` | 返回武器概览，保留当前查看实例 |
| `WorkbenchBack` / `WorkbenchBack()` / Esc | 抽屉 → 改装大图 → 武器概览 → 工厂；重复 Esc KeyDown 不连退 |
| `ResetWorkbenchView` | 调用 Viewport.ResetView，不更改装配 |
| `IsWeaponEditing()` / `IsAttachmentDrawerOpen()` / `GetViewedWeaponSlot()` | 自动验证当前层级、抽屉及查看的实际槽位 |
| `WeaponOverviewPanel` / `WeaponEnterPanel` / `AttachmentDrawer` | 三个稳定分组，随页面层级切换 |
| `AttachmentRows` / `RowAction` | 保留 v3 的实际安装/卸下及悬停参数事件契约 |
| `WeaponViewport` | Visible，调用 SetOverviewMode、SetSlotsVisible、Present；测试通过 static SlotCenter(Category) 取得卡片归一化中心，不硬编码像素坐标 |

卡片及模型只覆盖当前 M4/M9；未实装、未验收的 M7 和其他模型不被替换。`WorkbenchFeedback` 始终显示本次后端结果，包括存档失败。

## v3 历史布局

下述布局与控件位置是 v4 之前的已验证版本，保留供迁移追溯；当前生成器和运行导航按上一节执行。

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

v3 → v4 运行 [Tools/upgrade_extraction_ui_workbench_reference.py](../../../Tools/upgrade_extraction_ui_workbench_reference.py)，转发至 [套件源脚本](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Scripts/upgrade_extraction_ui_workbench_reference.py)，调用 `FPSExtractionUIBuilder::UpgradeWeaponWorkbenchReferenceAssets`。脚本先备份终端，再只替换 ModificationPanel 与 WeaponWorkbenchLayoutVersion；新面板为根 Layout 的子项，版本值为 4，InventoryLayoutVersion 保持 2。旧面板通过编辑器对象操作移出，禁止文本修改 uasset。其他页面的控件层级、相对顺序、Canvas、文本、字体与其他 Content 资产/地图哈希均核对不变。

迁移报告保存于 `Saved/FPSExtraction/UI/WorkbenchReferenceUpgrade/<时间>/Report.json`。v4 普通重跑不覆盖 Designer 编辑。`-WorkbenchReferenceRebuild` 是本轮制作修正使用的显式重建开关，仍先备份，只替换改枪台面板；后续手工编辑不应使用此开关。首次创建直接生成 v4；已有终端不能靠 create-only 脚本升级。旧 v3 脚本允许检查已经升级的 v4，不会降级。

## 自动验证入口与当前边界

主聊天的 `FPSExtractionUICapture.cpp` 负责独立 UI 存档和实机事件测试，本 UI 工作不修改此文件。可稳定访问的格子为 WarehouseGrid、BaseBackpackGrid、RaidBackpackGrid。事件 override 均为 public：NativeOnMouseButtonDown、NativeOnDragDetected、NativeOnDragOver、NativeOnDrop；GetCachedGeometry 和 CellSize 可将格中心换算成实际屏幕位置。正常事件和开发测试使用同一路径，没有只为测试绕过 UI 的移动接口。

应覆盖：从物品第二格抓取、旋转后拖入、越界拒绝、拖回仓库、Refresh 不结束拖拽、同类合并及容量不足拒绝、同 ItemId 多实体只操作选中 ID、Raid 不可访问仓库；再核对 Tab/Esc、整堆出售/丢弃、整备带入、失败恢复和成功入库。视觉采图至少为 Overview、Warehouse、Production、Modification、RaidBag、Result，1280×800 / 1920×1080 / 2560×1440。

v2 三分辨率、原生拖放处理、闭环与读档验证通过。v3 编译、增量资产升级、独立回载及三分辨率默认/瞄具/扩容/旋转采图通过：`Saved/ExtractionTests/UI-20261014-013916904/run.json`。覆盖模型挂点分类、真实按钮装卸、候选差异、库存扣还、模型替换、旋转与缩放；沿用正式处理器，但未替代完整操作系统输入路由、真人体验或打包验收。具体模块哈希、修复和验证边界见 [实施记录](../Extraction/Implementation.md)。

v4 已由主聊天完成统一构建、备份迁移和独立回载。固定模块的三分辨率概览/M9/改装大图/抽屉采图，以及实际装卸、库存、卡片点击、逐层返回和其他页面回归通过：`Saved/ExtractionTests/UI-20261015-150807868/run.json`。`Tools/TestFPSExtractionUI.ps1` 支持 `-RuntimeRoot` 指定固定运行副本，仍从原工程复制测试档案并保存证据；默认使用当前工程。模块哈希、失败重试与地图边界见 [v4 实施证据](../Extraction/Implementation.md)。

返回 [UI 索引](README.md)。

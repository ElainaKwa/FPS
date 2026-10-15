# 搜打撤小闭环实施与验证

- 用途：实施与验证记录
- 适用范围：实验分支的物品、存档、加工、配件和独立模式集成
- 状态：实体格子闭环、v4 改枪台与工厂入口仓库数量操作运行通过；PIE、打包与真人完整游玩未验证
- 最后更新：2026-10-15

基点为 `c255cf5`，工作分支 `codex/extraction-loop-lab`。开工时工作区已有 M7 美术、移动及武器内容等未提交改动，保留且不归入本轮交付。

## 接入

- `FPSExtractionTypes` / `FPSExtractionSubsystem`：UI 快照、统一物品与配方、背包/仓库转移、交替存档、生产领取、库存改装。
- `FPSExtractionMode`：复用工业关隘，分基地与出征阶段；运行时生成物资和守卫，独立处理撤离与死亡，不运行原对战计时和重生。
- `FPSExtractionEquipment`：受实验模式、权限、武器定义和弹药范围约束的装备恢复；不开放通用任意库存 setter。
- `FPSDemoGameMode::HandleLifeEnded` 改为虚函数，由实验模式覆盖；其他模式保持既有实现。
- `FPSWeaponComponent` 按实验模式限制免费选枪、枪械丢弃/拾取和界面期间动作；不改变其他模式规则。
- `FPSInterface` 在实验模式隐藏对战比分，保留现有生命、弹药、体力与命中反馈。
- UI 由用户授权的既有 UI 聊天并行制作，产物集中于独立 `FPSExtractionUI` 文件与新的界面资产。

## 基地与库存修订

用户在首版联调期间明确要求基地参考《三角洲行动》特勤处，并确认不同尺寸占格、拖放、仓库/背包/装备分区；本轮不加入胸挂、保险箱和负重。UI 沿用本项目美术，设施入口服务当前仓储、生产、改装，不复制商业游戏素材，不实现未确认的设施等级树。

`FPSExtractionGrid` 实现堆叠独立标识、矩形占位、旋转、同型合并、连续空位检查、容器权限与原子转移。旧汇总存档迁移在副本上执行；基地整理立即持久化，局内整理在行动结算时处理。

## 验证入口

`Tools/TestFPSExtraction.ps1` 创建独立测试档案，以真实游戏世界完成两次出征、世界拾取/丢弃、撤离取消与自然倒计时、出售、自然生产计时、安装/拆卸、GAS 射击与死亡结算，再启动全新进程验证持久化。

测试中的移动使用确定性位置设置，库存结算调用正式接口；不将其表述为真人游玩。主闭环测试关闭守卫，以隔离库存和结算，不能用该结果证明自然 AI 交战。`-Visual` 生成实际游戏截图用于界面检查。日志、模块哈希与运行状态位于 `Saved/ExtractionTests/`。

共享网络/GAS 代码的旧模式回归须另行运行真实服务器和客户端；编译结果不能代替此项。编辑器 PIE 退出、打包和人工体验仅在有本轮证据时标记通过。

## 首版证据（新版修订前）

- Editor Development 构建通过：`Saved/ExtractionTests/Build/Editor6.log`。
- 实际 standalone 两次出征、GAS 射击/死亡、全新进程存档恢复和独立 AI 战斗通过：`Saved/ExtractionTests/20261014-004659149/run.json`，模块 SHA256 `01FEE1D7324BC46F7CFBB596311E0868EDBDF8D1C2EDB39BBCFDC27343E4B9F6`。
- 旧工业关隘真实服务器/客户端 17 项回归通过：`Saved/IndustrialCrossing/Tests/20261014-003314606/run.json`，覆盖伤害、击杀、重生、GAS 计分和结算锁定。
- 第一版 UI 已取得成功/失败/背包截图，1280×800 可读；1080p 装备摘要预期高度 96.667 超出原 94，三分辨率检查失败记录位于 `Saved/ExtractionTests/UI-20261014-005312556/`，不能记为通过，随新版布局修正后复验。
- 初版资产创建及侧栏升级报告：`Saved/FPSExtraction/UI/20261014-002606-613542/Report.json`、`Saved/FPSExtraction/UI/SidebarUpgrade/20261014-004607-335954/Report.json`；相关脚本核对其他美术包哈希未改变。

上述首版结果与以下 v2 结果分别记录。编辑器 PIE、打包版本和真人完整游玩尚未验证。游戏启动日志还包含既有 Experimental Toolsets Python 插件错误，不能声称日志全部无错误。

## 实体格子 v2 证据

- 实际游戏闭环、全新进程读档和独立 AI 战斗通过：`Saved/ExtractionTests/20261014-011621728/run.json`。模块 SHA256 `ADCAAF0FF4AE8FA89E48451580383C12647EACB10773934E0F832146D625042C`。
- 覆盖实体坐标/旋转、满包、非法移动无损、整备带入、局内禁止仓库访问、失败恢复整备快照及新进程 v2 持久化。
- 三分辨率 1280×800 / 1920×1080 / 2560×1440 的实际 UI 渲染与原生鼠标/拖放处理通过：`Saved/ExtractionTests/UI-20261014-011617739/run.json`。测试调用正常控件事件处理器，不等同于完整操作系统输入路由或真人拖放。
- 上述 UI 测试从旧 v1 存档复制，验证了真实旧档迁移。修正了 SaveGame 默认版本不能改为 2 的兼容问题：旧文件可能省略默认字段；新建档案显式赋值 2。
- v2 资产迁移通过：`Saved/FPSExtraction/UI/InventoryUpgrade/20261014-011303-654680/Report.json`；其他 234 个资产与 9 张地图哈希不变。

## 中央 3D 改枪台

新增 `FPSExtractionWeaponViewport`，使用独立临时场景、六个模块化静态网格和 SceneCapture2D。三个挂点投影到模型位置，点击标记切换类别；右键旋转、滚轮缩放。界面从真实仓库生成兼容配件，参数差异读取已有解析器；安装/卸下继续原子库存事务，预览仅呈现已提交装配。

模型及生成说明见 [M4 预览源](../../../ArtSource/FPSDemo/Weapons/M4/ExtractionPreview/README.md)。目前是实验预览美术，实际第一/第三人称枪械仍使用原表现资产，未把原型外观写入正式武器定义。

### 改枪台 v3 验证

- `Saved/ExtractionTests/Build/Workbench1.log` 编译通过；三分辨率真实渲染和事件验证通过：`Saved/ExtractionTests/UI-20261014-013916904/run.json`，模块 SHA256 `DE722BA2A1178FAF2A7407C30ACBE4B3D9071D84E8ADDFD481D5F4D278B8BE74`。
- 三槽点击、真实候选按钮、参数差异、装卸库存、网格变化、旋转和缩放处理均通过；默认、瞄具、扩容和旋转画面人工查看，模型变化可见。沿用同一正常事件处理器，未声称完整 OS 输入路由测试。
- 增量界面迁移报告：`Saved/FPSExtraction/UI/WorkbenchUpgrade/20261014-013853-546265/Report.json`。保留既有页面控件，随后独立游戏进程回载成功。
- 模型导入报告：`Saved/FPSExtraction/WeaponPreview/20261014-013523/Report.json`；六个网格尺寸匹配源文件，其他 244 个资源哈希不变。初次导入的属性访问和 Blender 材质后缀问题已修复后重跑。

- 最后修正已装配件显示内部英文名的问题，并清理预览代码的浮点字面量警告。最终构建 `Saved/ExtractionTests/Build/Workbench2.log` 和 1080p 定向回归 `Saved/ExtractionTests/UI-20261014-014237092/run.json` 通过，模块 SHA256 `EC6252AD36218172D4F915B166011E43206FD140B23370A8A64C73A63F33E27E`。
- 交付截图：[默认装配](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/WorkbenchStandard.png)、[三类配件安装后](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/WorkbenchEquipped.png)、[实体仓库](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/Warehouse.png)。

## 参考布局改枪台 v4

根据用户提供的两张参考图，改枪台分为武器概览与改装大画布。概览显示持有的 M4/M9、真实参数和大幅模型；M4 进入改装后隐藏工厂外框，三张配件卡片以连线指向安装位置，点击后打开右侧库存抽屉。M9 沿用真实 Pickup 模型，没有兼容接口时禁用改装。保留项目工业灰黑、有限黄与现有字体。

`FPSExtractionWeaponViewport` 调整为枪口朝左，支持 M9、按实际部件生成缩略图、中文名称、模式视角与重置。RenderTarget 等比限制分辨率，避免宽画布拉伸；相机、灯光和背景仅属于临时预览场景。装卸仍由原后端提交，模型只呈现已保存装配。Esc 依次收起抽屉、返回概览、返回工厂。

### v4 证据与边界

- 最终 Editor Development 构建通过：`Saved/GunsmithReferenceTests/Build/Editor6.log`。原有测试代码仍有旧 InputKey API 弃用警告；本次不批量迁移无关测试。
- v3 → v4 迁移及本轮缩略图修正分别记录于 `Saved/FPSExtraction/UI/WorkbenchReferenceUpgrade/20261015-143958-577482/Report.json`、`20261015-144930-290117/Report.json`。每次先备份终端，核对其他 143 个控件与 252 个资源包不变。仅更新本套件终端资产。
- 固定运行副本的三分辨率 1280×800 / 1920×1080 / 2560×1440 全部通过：`Saved/ExtractionTests/UI-20261015-150807868/run.json`。模块 SHA256 为 `1DD3A7F5D9443B4DC572D1E3C400E2C85522657B719A239A22C6026EAE01D9DF`。副本清单与复核位于 `Saved/GunsmithReferenceTests/RuntimeFinal/snapshot.json`、`verification.json`；运行期间本轮源码和改枪台依赖资产没有变化；验收后仅清理一行尾随空格。
- 覆盖真实 M4/M9 模型切换、禁用不支持的改装、三张卡片点击、抽屉保留空间、真实库存装卸、候选差异、配件网格变化、旋转/缩放/重置及逐层返回。仓库原生拖放和加工/总览也回归通过。共取得 30 张游戏截图，并查看概览、安装后大图及抽屉布局；事件测试不替代完整 OS 输入路由或真人操作。
- 前一批 `UI-20261015-145557755` 的逐进程检查通过，但共享 DLL 在测试期间被并行任务替换，汇总失败，不作为最终验收。固定副本第一次启动 `UI-20261015-150249089` 卡在平台 SDK 扫描并超时；加入多进程运行标记后，用同一固定模块完成上述复验。
- 验证入口仍为工业关隘的基地模式。并行任务修改了另一个 FactoryBase 地图，该地图未被此批测试加载；本次结果不证明可走动工厂地图的集成。测试以独立个人档案运行，不修改玩家存档。PIE、正式打包及完整人工游玩仍未验证。
- 实机截图：[武器概览](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/ReferenceOverview.png)、[中央改装与配件连线](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/ReferenceModification.png)、[兼容库存抽屉](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/ReferenceDrawer.png)。M4 仍为已有实验预览模型，正式枪械美术接入单独处理。

## 仓库与背包数量操作

2026-10-15 用户选择继续仓库与背包功能。本轮在现有占格系统中增加指定数量拆分、Ctrl+左键快速互转和同类堆叠部分合并。来源余量保留物理 ID、格子与朝向；整堆自动转移到新格子时保留 ID。拆分在副本上寻找连续空位，只有存档写入成功才发布基地库存，局内拆分只修改本次背包，不改变出征快照。

`FPSExtractionGrid` 负责事务，`FPSExtractionGridUI` 与新增的 `FPSExtractionSplitDialog` 负责原生鼠标事件和数量编辑。弹窗复用既有字体和主题，不迁移或覆盖终端 WBP。`FPSExtractionUI::CommitInventoryDrag` 仅补充余量仍在来源时保持原选择。测试入口 [Tools/TestFPSInventory.ps1](../../../Tools/TestFPSInventory.ps1) 使用独立档案和固定模块副本，调用真实世界中的库存 API、原生格子事件与 UMG 控件事件，并分进程验证存档恢复。

数量操作实现完成并通过隔离 standalone 验证。首轮 `Saved/InventoryTests/20261015-160527971/run.json` 两种分辨率通过；人工查看截图后修正了数量输入框对比度，并更新仓库操作提示。

- 最终记录：`Saved/InventoryTests/20261015-161218022/run.json`，1280×800 与 1920×1080 各通过 37 项操作检查，随后分别启动全新进程，核对数量、物理 ID、位置与旋转完全恢复。两种分辨率各四张截图齐全，进程正常退出，已人工查看仓库、数量弹窗及局内背包弹窗。
- 覆盖无效数量/容器/过期 ID、同容器及跨容器部分合并、满容器拆分失败、快捷转移尝试合并后空间不足的整体回滚、隔离存档写入失败、原生鼠标右键与 Ctrl+点击、取消及 Esc、真实出征后的局内拆分与仓库访问拒绝、失败结算恢复原整备快照。UI 使用原生控件事件，失败结算调用正式接口，不声称是完整 OS 输入路由或自然死亡战斗测试。
- 最终模块 SHA256 `D2BEF8AD5D82D57C606173B83BE413F28FEE2D63A11EC08DCFBBE50FE5969CBF`。`Saved/InventoryTests/Build/Editor5.log` 中本轮源码编译通过，但其他编辑器占用主 DLL，链接失败；随后复用 UBT 生成的响应文件，仅将 DLL/PDB 输出改到 `Saved/InventoryTests/Build/FinalLink/` 独立链接成功。`verification.json` 核对链接前后 46 个对象文件未变化。
- 当时主项目新增的七个武器定义缺少已保存的伤害配置，`Editor3.log`、`Editor4.log` 的构建后内容校验失败，未记为完整构建成功。该批运行使用已通过武器内容校验的 `Saved/HitDamageBuild/Content` 隔离资产副本；测试记录保存了实际内容路径与 UI/武器资产哈希，并核对测试期间未变化。该批尚未完成主项目集成，后续复验见下节。
- 验证脚本默认使用当前项目模块与内容，`-ModulePath` 和 `-ContentRoot` 可显式选择隔离版本。所有测试存档写入本轮运行目录的独立 Runtime，不接触玩家存档。PIE、打包与完整人工游玩仍未验证。大厅 UI 由美术聊天单独实现，本记录不表示大厅已完成。

### 主工程工厂入口复验

2026-10-15 按用户要求继续集成。经用户授权通知 M7 聊天后，其预览编辑器正常退出，释放主 DLL；大厅聊天负责补齐七份武器定义的已保存引用并重新导出武器审阅记录。标准 Editor Development 构建通过，见 `Saved/FPSExtraction/UI/LobbyIntegration/20261015-181434098/EditorBuild2.log`；同目录 `WeaponValidation2.log` 记录 `FPS_WEAPON_DATA PASS definitions=7`。迁移及大厅本身的验收范围见 [大厅实施记录](../UI/LobbyImplementation.md)。

仓库验证默认入口改为真实 `FactoryBase` 地图与 `-FactoryLobby`。`AFPSFactoryMode::Tick` 加入仅由开发测试参数启用的库存验证入口；测试通过工厂控制器取得嵌套终端，经过 Slate 命中测试与鼠标事件点击大厅仓库按钮，再执行数量事务、出征和失败返回，补齐旧工业关隘基地入口没有覆盖的工厂集成。

- 最终记录：`Saved/InventoryTests/20261015-183102982/run.json`，使用主项目 `Content` 和主项目模块的固定运行副本。模块 SHA256 `CF2944614ED53C4C7112FF818E495B841C7BF9D4BFD20CC11594B3DCBAAE6BC9`；`verification.json` 确认交付检查时仍与主 DLL 一致，仓库相关源码未变化。
- 1280×800 与 1920×1080 各通过 41 项检查，再各启动全新进程，精确核对持久化数量、物理 ID、位置与旋转。四个进程均正常退出，UI、武器及工厂地图资产哈希未变化，八张截图齐全。已查看仓库格子、数量弹窗和局内背包弹窗，文字与确认按钮可读。
- 新增覆盖实际工厂大厅、仓库按钮可命中和事件路由、失败结算返回同一工厂大厅；原 37 项事务和背包检查继续通过。测试调用 Slate/UMG 事件和正式结算接口，不等同于完整操作系统输入、自然死亡战斗或真人完整游玩。
- 前一批 `Saved/InventoryTests/20261015-182742777/run.json` 同样通过，但运行期间主 DLL 被后续统一构建更新；其冻结模块为 `5C45C273CB9DB0F7D6B0D0F1F5144FE83325515CE517C5513D468E4D31B48B84`。最终批次针对更新后的主模块重新完成两种分辨率及读档，不将前一批代作当前模块证据。

仓库数量操作的主工程 standalone 集成已验证；PIE、打包与完整人工游玩仍未验证。本轮没有修改网络或 GAS 行为。

2026-10-15 按用户要求清理测试产物：删除以上四批库存测试的 `Runtime` 临时运行副本，以及 `Build/FinalLink` 下的旧 DLL/PDB；只移除临时 `Content` 目录链接，保留其目标资产。原验收报告、日志、截图、链接响应文件和对象哈希记录保留。最终批的四份隔离存档已移至 `Saved/InventoryTests/20261015-183102982/PersistenceSamples/` 并核对哈希；玩家存档未改动。清理清单见 `Saved/InventoryTests/Cleanup-20261015-192317790.json`。历史 `run.json` 仍记录当时的运行路径，其中已删除的运行副本不再可直接启动；再次验证由 `Tools/TestFPSInventory.ps1` 创建新的副本。

返回 [实验索引](README.md)。

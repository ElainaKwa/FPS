# 搜打撤小闭环实施与验证

- 用途：实施与验证记录
- 适用范围：实验分支的物品、存档、加工、配件和独立模式集成
- 状态：实体格子闭环与中央 3D 改枪台运行通过；PIE、打包与真人完整游玩未验证
- 最后更新：2026-10-14

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

返回 [实验索引](README.md)。

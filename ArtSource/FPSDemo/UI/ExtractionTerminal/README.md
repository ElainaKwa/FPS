# 搜打撤基地终端 UI

当前制作方式：程序生成阶段。权威布局及绑定为 `Source/MCP_Test/FPSExtractionUI.h/.cpp` 和 `FPSExtractionGridUI.h/.cpp`，本目录脚本调用其编辑器入口；已保存的 Widget Blueprint 可在 Designer 编辑，生成器只创建缺失资产，不覆盖现有布局。

- Unreal 目标：`/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal`、`WBP_ExtractionRow`、v3 新增 `WBP_ExtractionAttachmentRow`。
- 生成：统一编译新 C++ 后，通过 Unreal Python 运行项目根 `Tools/build_extraction_ui.py`，其转发至本目录 `Scripts/build_extraction_ui.py`；生成逻辑只维护一份。
- v1 → v2 实体格子升级：统一复编后运行 `Tools/upgrade_extraction_ui_inventory.py`。备份后只重建本套件终端，保留其他资产；版本 2 重跑不覆盖 Designer 编辑。创建脚本不会升级已有资产，旧侧栏升级脚本仅适用于 v1。
- v2 → v3 中央 3D 改枪台：统一编译后运行 `Tools/upgrade_extraction_ui_workbench.py`，转发至本目录同名 Scripts 脚本。备份后追加独立改枪面板和缺失的窄版配件行，保留原控件树与其他页面；脚本验证旧控件布局、文本、字体及其他包哈希。版本 3 重跑保留 Designer 编辑。
- v3 → v4 参考布局：运行 `Tools/upgrade_extraction_ui_workbench_reference.py`，备份后只替换 ModificationPanel，迁为工厂面板的同级控件。新增 M4/M9 实例概览、独占改装画布和按槽位打开的配件抽屉；其他工厂页及资源不动。版本 4 重跑不覆盖 Designer 编辑。首次创建直接生成 v4；已有资产必须显式升级。当前制作过程中若要重建本轮改枪台面板，可显式传入 `-WorkbenchReferenceRebuild`；仍先备份并核对其他页面，不作为普通更新步骤使用。
- 共用依赖：现有 `/Game/FPSDemo/Art/Industrial/UI/DA_FPSUITheme`、`Fonts/F_UI_Text`（MiSans，拉丁字形 Rajdhani）。保留既有字体来源与许可，不复制字体。
- 代码依赖：`UFPSExtractionUI`、`UFPSExtractionButton`、`UFPSExtractionGridUI`、`UFPSExtractionGridDrag`、`UFPSExtractionWeaponViewport`、`UFPSExtractionSubsystem` 及其类型、既有 `UFPSUITheme`。3D 预览还依赖主聊天维护的实验模型和预览 Stage；迁移不能只复制 Widget Blueprint 而忽略本地 C++ 类及模型依赖。
- 状态与证据：见 [UI 接入说明](../../../../../Docs/FPS/UI/ExtractionUI.md)。脚本输出至 `Saved/FPSExtraction/UI/`，脚本未执行不能视为已生成资产或验收。
- 大厅升级：`Tools/upgrade_extraction_ui_lobby.py` 转发本目录 `Scripts/upgrade_extraction_ui_lobby.py`。只替换旧 OverviewPanel，保留仓库、加工、v4 改枪台和局内/结果页。普通重跑保留 Designer 编辑；显式 `-LobbyRebuild` 才重建大厅。
- 大厅构图：`DA_LobbyPresentation` 保存相对 FactoryCamera0 的构图参数，运行时 `FPSExtractionLobby.h/.cpp` 创建展示角色和相机，关闭后恢复可走动角色。依赖既有工厂地图与机器人的运行网格。
- 大厅纹理：`Scripts/prepare_lobby_textures.py` 用 Pillow 输出 `Textures/T_LobbyVignette.png` 和 `T_LobbyIndustrialCrossing.png`。前者为数学绘制的透明遮罩，后者为本项目 CrossingKit 的 `Previews/UnrealMapOverview.png` 派生缩略图。`Scripts/lobby_assets.py` 导入缺失资源并建立构图资产，不复制共享字体。首次创建也会准备缺失的大厅资源。
- 重建：需要明确确认后，用编辑器将本套件资产移至版本目录，再运行生成器。不要删除现有手工布局或移动旧 Industrial UI。

## 实机预览

`Previews/WorkbenchStandard.png`、`WorkbenchEquipped.png` 与 `Warehouse.png` 来自 v3 构建的独立 1920×1080 游戏采图，来源 `Saved/ExtractionTests/UI-20261014-014237092/`，并非设计效果图。包含三个真实配件的原型外观；不能作为新增 v4 双层布局的验收图。

当前 v4 的 `Previews/ReferenceOverview.png`、`ReferenceModification.png`、`ReferenceDrawer.png` 来自 `Saved/ExtractionTests/UI-20261015-150807868/` 的 1920×1080 实际游戏截图，分别展示武器概览、三件配件装好后的中央画布、兼容库存抽屉。沿用已有实验模型与工业 UI 资源；截图不表示正式枪械美术已完成。

大厅 v1 的首次预览 `Previews/Lobby.png`、`LobbySettings.png` 来自 `Saved/FPSExtraction/UI/LobbyTests/20261015-162255615/`；`Lobby1280.png` 来自 `20261015-162406500/`。分别为 1920×1080 和 1280×800 的实际游戏采图；当时使用独立链接模块和已校验的武器配置副本。

当前主项目复验图为 `Previews/LobbyIntegrated.png`、`LobbyIntegrated1280.png`，来源分别为 `Saved/FPSExtraction/UI/LobbyTests/20261015-182849580/`、`20261015-183058742/`。主模块和主 Content 已通过构建、数据校验及两种分辨率的大厅往返验证，没有替代武器配置；布局和构图资产保持原版本。完整证据、哈希及尚未验证范围见 [大厅实施记录](../../../../../Docs/FPS/UI/LobbyImplementation.md)。

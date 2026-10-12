# 美术源文件整理记录

- 用途：实施与验证记录
- 适用范围：ArtSource 目录治理第一阶段首批，M4 精细样品与中文字体对比源资料
- 状态：首批迁移和路径验证完成；Industrial 混合库及其他旧目录尚未迁移，Content 迁移未执行
- 最后更新：2026-10-12

## 1. 本批范围

按用户确认的 [美术资产存放与复用规范](AssetOrganization.md)，先处理可独立迁移的两组样品。版本为 `ArtSource-Organization-20261012-batch1`，逐文件旧路径、新路径与 SHA-256 见 [生成核验报告](../../../ArtSource/FPSDemo/SourceMigration.json)。此报告由 [检查脚本](../../../Tools/check_art_source_migration.py) 生成，不手工维护。

本批不改变模型外观、UI 布局、正式字体选型或游戏资产位置。规范已生效不代表历史目录已经全部符合目标结构。

## 2. 已执行映射

| 旧位置 | 新位置 | 处理 |
| --- | --- | --- |
| `ArtSource/FPSDemo/PrecisionSamples/M4` | `ArtSource/FPSDemo/Samples/Weapons/M4` | 样品整体迁移，历史快照保留原内部布局 |
| 上述 M4 根目录中的当前 `.blend` 和 `.blend1` | 新目录的 `Source/` | 文件内容不变 |
| 上述 M4 原 `Source/*.py` | 新目录的 `Scripts/` | 更新当前构建、导出与核验脚本；形体脚本内容不变 |
| `ArtSource/FPSDemo/FontComparisons` | `ArtSource/FPSDemo/Samples/UI/FontComparisons` | 字体、许可、截图及历史生成报告原样迁移 |

M4 的清单和 Exports、Previews 位于资产目录内原有对应位置。两组样品新增资产 README，作为交接入口：[M4](../../../ArtSource/FPSDemo/Samples/Weapons/M4/README.md)、[字体对比](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/README.md)。

同步修改 M4 当前重建命令与样品文档链接，字体对比生成/排版/核验脚本，以及 `FPSUIAssetBuilder.cpp` 的候选字体输入目录。Unreal 的 `/Game/FPSDemo/Art/FontComparisons` 和正式字体路径保持当前入口，第二阶段再按编辑器依赖处理。

历史快照内脚本、报告和预览不改写。它们仍描述各自生成时的布局，尤其旧对比 HTML 不能视为当前导航；当前 M4 对照入口为 [Previews/Compare.html](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/Compare.html)。

## 3. 验证结果

- 移动前记录 66 个文件的路径和 SHA-256，移动后全部找到；其中 63 个内容不变，仅 3 个当前路径/核验脚本按计划修改。所有 Blender、FBX、图片、字体、许可、历史快照及历史 JSON 保持原字节。
- 在新目录打开 M4 Blender 源文件，检查形体生成器哈希、625 个导出网格和 208,276 个三角面，与现有清单一致；源文件和现有 FBX 的哈希均匹配。未重新建模或重导出。
- 新目录中的构建路径预检查通过；从新目录回读 FBX，包围盒误差为零。核验模式不覆盖模型或历史清单。
- 当前两份对照 HTML 的本地资源链接和 M4 视角切换目标存在；候选字体四个输入文件存在，Python 语法与 JavaScript 语法检查通过。
- `MCP_TestEditor Win64 Development` 编译成功，覆盖候选字体输入目录的一处 C++ 路径调整。未重新生成候选 UI 或运行 PIE，不把编译和路径检查当作新一轮 UI 视觉验收。

证据位于 `Saved/ArtOrganization/`：`Before.json`、`DirectoryMoves.json`、`BlenderPaths.log`、`BlenderSource.log`、`BlenderFBX.log`、`EditorBuild.log`。持久报告保存每项迁移的前后哈希；Saved 中的日志是本次本机证据，不作为制作源文件。

复核本批移动使用 `python Tools/check_art_source_migration.py`，需要保留上述迁移前清单和验证日志。该脚本核验的是这一批迁移，不是全项目的通用美术质量检查；后续合法修改资产不应通过改写旧报告伪装成迁移时的状态。

## 4. 后续批次与边界

以下为待执行映射方向，现有文件仍从原路径使用。混合库的生成器、清单、导入器和记录脚本必须一起调整后才能搬迁，不能仅改文件夹名称。

| 现有内容 | 目标方向 | 尚需处理 |
| --- | --- | --- |
| `Industrial/WeaponLibrary.blend` 及七枪导出 | `Weapons/IndustrialSet`，后续有独立制作需求再拆成单枪 | 保持共享 Blender 库只有一个权威源，修复导出和图标生成入口 |
| `Industrial/EnvironmentKit.blend` 与环境导出 | `Environment/IndustrialKit` | 拆分混合导出目录，维护模块清单和导入路径 |
| `Industrial/Operator.blend`、`RuntimeParts.blend` 与人物导出 | `Characters/Operator` | 维护展示源、运行时派生源及各自生成关系 |
| `Industrial/UI`，不含共用字体 | `UI/IndustrialTerminal` | 分清图片、预览、历史报告及 UI 构建输入 |
| `Industrial/Icons` | `UI/WeaponIcons` | 保持与对应枪械模型的派生关系 |
| 正式字体与共用材质源数据 | `Shared/Fonts`、`Shared/Materials` | 调整生成/导入/许可打包入口，历史候选副本保留其证据身份 |
| 旧 `Weapons`、`Avatar` 及根目录 OBJ | 按当前引用和用途归入具体资产或历史样品 | 先识别正在使用与历史资源，不根据名字直接删除或转正 |
| Unreal `Content/FPSDemo` 美术资源 | 规范规定的 Art 类别目录 | 第二阶段通过编辑器迁移，修复引用及源文件重导入信息并验证 |

`ArtSource` 下其他 M7、卡通枪等独立工作线未在本批移动，不能从目录名推断可删除或合并。音频目录也未因本次视觉美术整理而搬迁。

后续状态集中维护在 [美术待办](Tasks.md)。返回 [统一美术索引](README.md)。

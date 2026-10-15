# 搜打撤行动大厅实施记录

- 用途：实施与验证记录
- 适用范围：个人工厂中的行动大厅、现有功能入口、场景展示和资产保存
- 状态：布局、构图及纹理资产已保存；主项目 Editor 构建、武器数据校验及双分辨率大厅往返验证通过；PIE、打包和自然战斗撤离未验证
- 最后更新：2026-10-15

## 本次范围

按用户提供的大厅参考重新制作 OverviewPanel，中央保留实际机器人和车间，顶部四入口为出征、仓库、工厂、改枪台，右上显示实际存档资金和设置，右下显示工业关隘及整备/出发。加工状态与入口位于左下。不加入未支持的活动、社交和地图列表。

复用现有库存和 v4 改枪台，保留真实装配模型与槽位事件；库存数量操作属于另一并行任务。本次未重建工厂地图或精修机器人模型。大厅中的展示机器人复用已有网格，离开后恢复原玩家、相机与输入。

## 入口与可编辑资产

- 使用与控件契约见 [UI 接入说明](ExtractionUI.md)。
- Widget：`/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal`，大厅标记 LobbyLayoutVersion=1，原改枪台标记保持 4。
- 构图：同目录 `DA_LobbyPresentation`；UI 纹理在同目录 `Textures/`，制作脚本在 [资产源目录](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/README.md)。
- 大厅启动：`Tools/LaunchFPSExtraction.ps1`；车间启动：`Tools/LaunchFPSFactory.ps1`。出征均沿用当前单人个人档案。
- 定向验证：`Tools/TestFPSExtractionLobby.ps1`，独立存档，实际 Slate 指针事件点击各入口，并执行测试结算返回。此验证不代替自然战斗和真人撤离体验。

## 首次布局与独立副本验证

本轮版本为 `LobbyLayoutVersion=1`，原 `WeaponWorkbenchLayoutVersion=4` 保留。编辑器升级报告位于 `Saved/FPSExtraction/UI/LobbyUpgrade/20261015-160059-306041/Report.json`，同目录保存升级前 WBP；比较确认大厅以外 173 个控件与 379 个既有资源包保持不变。旧侧栏“个人工厂”返回项按本轮设计改称“行动大厅”。

构图经过实际采图调整，当前 `DA_LobbyPresentation` 保存 CameraOffset=(0,390,-215)、LookAtOffset=(0,840,-255)、RobotOffset=(45,720,-245)、RobotRotation=(0,-78,0)、FOV=65。参数相对既有 FactoryCamera0；缺失资产创建器使用同一初始构图，普通重跑保留已保存值。

### 编译与集成边界

- `Saved/FPSExtraction/UI/LobbyBuild/Editor2.log` 中 C++ 编译和链接完成，但后续必需的武器校验因主项目 7 份 WeaponDefinition 未保存 HitDamageProfile 而失败；不能记为完整构建通过。
- 最后修正了改枪台底部返回按钮重叠和设置截图采集时序。`EditorFinal.log` 中相关 C++ 再次编译完成，主 DLL 被另一编辑器占用导致链接失败。
- 使用 UBT 已生成的链接响应文件，仅将输出 DLL/PDB 路径改到 `Saved/FPSExtraction/UI/LobbyBuild/FinalLink/`；独立链接成功，链接前后核对 46 个对象文件哈希一致。没有结束其他编辑器或覆盖被占用的主 DLL。记录在该目录 `verification.json`、`objects.json` 和 `link.log`。
- 首次测试运行副本读取主项目大厅/工厂资源，但武器 Definitions/Damage 来自已校验的 `Saved/HitDamageBuild/Content/FPSDemo/Weapons` 副本；当时没有修改主项目武器资产。该阶段的集成阻塞在下述主项目复验中解除。

### 实际运行结果

| 分辨率 | 证据目录（项目根目录下） | 结果 |
| --- | --- | --- |
| 1920×1080 | `Saved/FPSExtraction/UI/LobbyTests/20261015-162255615/` | passed；8 张实机截图、完整入口链路 |
| 1280×800 | `Saved/FPSExtraction/UI/LobbyTests/20261015-162406500/` | passed；8 张实机截图、完整入口链路 |

两次运行使用同一固定模块，每次执行 27 项检查并记录 COMPLETE；实际 Slate 指针事件触发仓库、改枪台、设置、加工、工厂、整备和出发。确认实际资金显示，工厂入口恢复原玩家/相机/移动，设置返回仍保持大厅输入锁定；实际加载工业关隘，再通过已有测试结算接口返回工厂大厅。运行时模块与 WBP 哈希前后不变。

人工核对两种分辨率的大厅排版、改枪台底部和设置页；1280×800 沿用 16:9 内容适配，上下保留黑边。图像是实际游戏采图，机器人与武器为既有模型，本次不代表模型精修完成。持久预览：[大厅 1920×1080](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/Lobby.png)、[大厅 1280×800](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/Lobby1280.png)、[设置入口](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/LobbySettings.png)。

可定位哈希（SHA-256）：

| 产物 | 哈希 |
| --- | --- |
| 测试模块 | `38EA5FB904CAB1EACB70E6D80D1CDF3F7FBB6C95C39DE0FFA8B3ED22B6F501EC` |
| WBP_ExtractionTerminal | `F9DD542CF0F3E2C66EA36A60E56710E0212771038431956F821A6AD4CF35DFDF` |
| DA_LobbyPresentation | `0AA93483CF222F741B210736617E3FF02F19BB7DEB9F0F33C55B3F7E550DC578` |

复现独立验证可向 `Tools/TestFPSExtractionLobby.ps1` 传入 `-ModulePath Saved/FPSExtraction/UI/LobbyBuild/FinalLink/UnrealEditor-MCP_Test.dll` 和 `-WeaponContentRoot Saved/HitDamageBuild/Content/FPSDemo/Weapons`；不传参数则使用主项目模块和武器内容。测试建立独立存档和配置。上述证据属于 Standalone 离屏运行、Slate 指针事件及测试结算，不代表完整 OS 键鼠输入、自然战斗撤离、PIE、打包或多人验证；既有工厂初版测试未重复执行。

本轮 Python 入口语法检查通过。文档按项目规则运行 `python Tools/CheckDocs.py`；状态、资产来源与上述运行边界另作人工核对。

## 主项目集成复验（2026-10-15）

主项目武器引用与 DLL 占用已解决。使用主项目 `Binaries/Win64/UnrealEditor-MCP_Test.dll` 和 `Content/` 完成复验，测试参数没有传入 ModulePath 或 WeaponContentRoot 替代项。运行器仍为避免共享占用而复制 DLL、使用独立存档；资产内容来自主项目。

### 集成处理与构建

- 七份 WeaponDefinition 原先 HitDamageProfile 均为 None。先备份原资产及全部公开属性，再使用既有 `Tools/migrate_fps_hit_damage.py`，只为空引用指定已保存的默认方案；默认倍率仍为头部 2、躯干 1、四肢 0.75，没有重新设计伤害规则。
- 原 M7 预览编辑器由对应工作线正常退出。此前通过 live MCP 修改 M4 的保存尝试因文件占用失败，已在退出前恢复为 None；未强制结束该编辑器。随后独立 commandlet 成功迁移全部 7 份定义，日志包含 `FPS_HIT_MIGRATION PASS assigned=7 definitions=7`。
- 首次迁移命令停在 SDK 初始化，第二次显式跳过 SDK 检测后完成；只结束了本轮挂起的辅助进程。导出命令同样改用明确的本地 DDC，保留首次启动失败日志。
- 主工程 C++ 编译、链接成功后，首轮构建的资产门禁报审阅资料过期。通过原生成器重新导出并核对差异，随后标准 UBT 构建 `Result: Succeeded`；另行执行不带 Export 的只读数据校验，得到 `FPS_WEAPON_DATA PASS definitions=7`，覆盖 UBT 无编译动作时可能跳过 post-build 的情况。

上述备份、迁移与构建证据集中在 `Saved/FPSExtraction/UI/LobbyIntegration/20261015-181434098/`：`Before/`、`Migration2.log`、`MigratedFiles.json`、`EditorBuild.log`、`EditorBuild2.log`、`WeaponExport2.log`、`WeaponValidation2.log` 和 `Integration.json`。自动生成的武器审阅资料由生成器导出，没有手工修改。

### 主内容运行证据

| 分辨率 | 证据目录（项目根目录下） | 结果 |
| --- | --- | --- |
| 1920×1080 | `Saved/FPSExtraction/UI/LobbyTests/20261015-182849580/` | passed；27 项检查、8 张截图 |
| 1280×800 | `Saved/FPSExtraction/UI/LobbyTests/20261015-183058742/` | passed；27 项检查、8 张截图 |

两次运行的模块 SHA-256 均为 `CF2944614ED53C4C7112FF818E495B841C7BF9D4BFD20CC11594B3DCBAAE6BC9`，本轮交付前主模块哈希相同。WBP 与构图资产沿用前文哈希，布局未重新生成。两次均通过实际入口点击、真实工业关隘地图加载、测试结算及返回工厂大厅；运行过程中没有替代武器配置。

主项目实机预览：[1920×1080](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/LobbyIntegrated.png)、[1280×800](../../../ArtSource/FPSDemo/UI/ExtractionTerminal/Previews/LobbyIntegrated1280.png)。人工核对了大厅构图和字体布局；1280×800 仍按既有 16:9 内容适配。现可通过 `Tools/LaunchFPSExtraction.ps1` 使用主工程入口。

此次解除的是大厅集成阻塞，不替代枪械工作线的服务器命中验收。完整 OS 输入、PIE、打包及自然战斗撤离仍不在本轮证据内；仓库数量操作的专项结果见 [搜打撤实施记录](../Extraction/Implementation.md)。

返回 [UI 索引](README.md)；工厂初版地图历史证据见 [个人工厂](../Maps/FactoryBase.md)。

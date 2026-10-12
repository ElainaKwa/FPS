# 按样稿细节对齐的 UI 实施记录

- 用途：实施与验证记录
- 适用范围：FPS 大厅、配装、HUD，以及继承公共组件的等待、暂停、结算
- 状态：用户已选定 MiSans；正式字体和六个 UI 资产已更新，重新对齐并通过实机验证；历史原版证据单列保留
- 最后更新：2026-10-12

## MiSans 正式接入与对齐（2026-10-12）

用户从三版实机对比中选定 MiSans，并明确要求重新对齐 UI/HUD。正式 `F_UI_Text` 改为原始 MiSans Regular/Bold，保留 Rajdhani 英文、数字及原有字号层级；重新生成并保存六个完整 Widget Blueprint、主题与字体资产。仅重建字体和控件，没有重导纹理或修改模型。候选目录及原版截图保留为历史证据。

| 区域 | 对齐内容 |
| --- | --- |
| 大厅 | 导航编号与中文视觉中心统一；帮助/退出与图标对齐；难度与开始按钮居中；预览和分节标题微调 |
| 配装 | 标题对齐强调条；`P / Esc` 与“关闭”对齐；基地状态、主副武器标题、装备标记及底栏图标文字重新校准 |
| HUD | 两侧队名对齐色标与比分，保留镜像比分位置；靶场标题居中；生命队名、`R`、换弹和动作提示对齐；拾取键帽与文字整体居中 |
| 其他菜单 | 等待、暂停、结算沿用 MiSans，并调整共用标题和按钮文字位置 |

偏移由 `FPSUIAssetBuilder.cpp` 的 `FPage::AlignMiSans` 写入资产，使用 1600×900 设计单位；无逐帧位置补丁。实机像素核对：1080p 的 `R` 字形中心 y=997，键帽中心 y=997.2；队名字形中心 y=977，色标中心 y=976.8；拾取 `F` 与提示分别距底板中心 0.5、1 像素。1440p 双方比分字形中心的中点与计时器字形中心相差 0.5 像素。阈值测量只辅助判断，六页布局及小窗口截图另经人工检查。

验证入口为 `Saved/MiSansAlignment`：编译和 7 个武器定义校验通过；字体与六个 Widget Blueprint 保存通过。编辑器 `-game` 覆盖 1920×1080 大厅各模式/帮助/配装/暂停/靶场/换弹/拾取/低血/死亡、2560×1440 战斗 HUD、1280×800 配装滚动/等待/结算；真实 Slate 点击、滚轮选中 UMP45、返回 M4 及关闭均通过。没有改动网络或 GAS 行为，本轮未重复双端验证。

独立包为 `Saved/Packages/IndustrialArt/MiSans-20261012-v1/Windows/MCP_Test/Binaries/Win64/MCP_Test.exe`；字体许可 PDF 与来源说明已随包保存。它替代下文 v7 作为当前字体版本，分发时保留整个 `Windows` 目录。证据由 [MiSans 核验脚本](../../../Tools/verify_fps_misans.py) 汇总到 [MiSans 验证记录](../../../ArtSource/FPSDemo/Industrial/UI/MiSansVerification.json)；历史验证报告不覆盖本次字体变化。尚未覆盖超宽屏、手柄、专项性能或 Shipping 验收。

独立包大厅及靶场序列实际启动通过，包含配装点击/关闭、暂停、换弹、拾取和检视，共补拍 9 张图；结合编辑器序列共 44 张实机图、18 项字形位置检查。编辑器 `-game` 日志中实验性 Toolset 启动 Python 缺少 editor-only 类的诊断已单独保留在报告中，未改动引擎插件；这些诊断不出现在本轮独立包日志，UI 截图与操作检查通过。

当前预览：[大厅](../../../ArtSource/FPSDemo/Industrial/UI/Previews/MiSans/Lobby.png)、[配装](../../../ArtSource/FPSDemo/Industrial/UI/Previews/MiSans/Loadout.png)、[HUD](../../../ArtSource/FPSDemo/Industrial/UI/Previews/MiSans/HUD.png)、[拾取与检视](../../../ArtSource/FPSDemo/Industrial/UI/Previews/MiSans/Inspect.png)、[1440p 比分栏](../../../ArtSource/FPSDemo/Industrial/UI/Previews/MiSans/CombatHUD.png)。

## 原版历史依据与修正

用户在首轮资产化后指出几何框、背景细线、字体等仍偏离样稿，要求按样图具体样式修改。依据为 [大厅](Samples/Lobby-v2.png)、[配装](Samples/Loadout-v2.png) 和 [战斗 HUD](Samples/CombatHUD-v2.png)。原有功能及服务器规则保留；高精度枪械、人物建模仍按要求暂缓。

| 部位 | 本轮修正 |
| --- | --- |
| 字体 | 项目内思源黑体常规/粗体，标题字距，Rajdhani 窄体数字和工业英文字形；独立图标替代装饰字符；修复中文提示误用数字字体的问题 |
| 背景 | 低对比微纹理、辅助细线和局部斜线；面板与浅色卡片分别保存表面纹理 |
| 大厅导航 | 透明非选中项、编号与分隔线、窄黄条、小角标和右下斜线；帮助/退出使用线形图标 |
| 大厅内容 | 场景图覆盖标题、独立技术侧栏、细方框加减、时长栏、矩形黄按钮及右下斜线 |
| 配装 | 三列比例、大幅浅色卡片、小圆角、主武器滚动、细滚动轨道、装备圆勾、一体圆角关闭键帽、护盾/提示/实时图标 |
| 配装详情 | 恢复两行操作说明和服务器反馈，移除首轮大统计数字；未装备时不显示装备勾，配装打开时隐藏战斗 HUD |
| HUD | 红队三角与蓝队方块、分隔线、数字字体、生命底板小切角、弹药区键帽及细强调条 |

用户追加指出 `P / Esc` 键帽右边框缺失、英文字形和卡片圆角仍有差距。本轮将键帽与卡片选中边框改为一体圆角 Brush，卡片采用 6 单位小圆角，英文加入复合字体的独立拉丁字族。随后按用户反馈修正键帽文字居中：保持水平居中，文字上移 2.5 个设计单位，补偿字形基线造成的视觉偏下；该位置同步写入配装 Widget Blueprint。

用户随后要求调整局内 HUD 字号和对齐。生命与弹药主数字统一为 35 设计字号，备用弹药为 17，队名为 16、比分为 24、居中计时器为 31；它们随 1600×900 设计画布等比缩放，并非最终屏幕像素。生命数字和血条左边缘统一，备用弹药重新对齐主数字下缘，靶场标题居中，基地提示降为次级字号。右侧短按键提示与较长动作状态分别使用独立文本位置，状态出现时隐藏 `R 换弹`，避免“检视中”等文本挤出面板。

进一步按局部截图修正 `R` 键帽的字形中心、生命区队名字形与色标中心；比分栏使用镜像顺序“色标—蓝队—得分｜计时器｜得分—红队—色标”，两侧得分框距中心相等，队名和图形垂直对齐。拾取提示改为独立 `F` 键帽和动态文本的水平组合，根据实际武器名称宽度整体居中，统一间距，保留在准星下方。

六个界面保留完整可编辑 Widget Blueprint 控件树。字体、图标、面板、背景、武器图片和公共主题均保存为独立资产；没有把参考图直接用作整页 UI。入口与重建约定见 [资产说明](AssetUsage.md)。

字体来自 [Adobe Source Han Sans](https://github.com/adobe-fonts/source-han-sans) 与 [Google Fonts Rajdhani](https://github.com/google/fonts/tree/main/ofl/rajdhani)，原始字体未修改，许可和来源随源资产保留，打包脚本会复制许可。

## 原版历史验证与边界

初轮 1672×941 实机复核发现帮助控件重名、中文提示字体错误、装饰纹理过强等问题，均已修正到源代码。重复生成过程发现并修复旧控件 GUID 残留、字体面重复初始化断言。第二轮生成通过，真实 Slate 滚轮路由和列表底部 UMP45 选择通过。在 1280×800 实机图确认键帽闭合、卡片圆角及中文可读性，鼠标滚轮和底部卡片选择通过。

键帽居中修正后，`Saved/UISampleMatch/Centered/Match-1280x800/Loadout.png` 的文字亮色像素范围为 y=128–140，中心 y=134，与键帽中心一致；水平文字中心 x=1046，键帽中心 x=1046.8，差异不足一个像素。同次实际滚轮、底部武器选择和关闭检查通过。导入时曾遇到其他测试进程持有文件锁，停止本轮导入并待占用释放后重试成功；导入器现检查纹理保存返回值，不再将保存失败当成成功。

HUD 局部对齐后，1920×1080 实机截图中 `R` 的字形中心为 (1806.5, 997)，键帽中心为 (1806, 997.2)；生命区队名的字形垂直中心为 977，色标为 976.8。两者误差均不足一个像素，记录位于 `Saved/UISampleMatch/AlignmentReview`。顶部得分框分别以 129.5 和 378.5 为局部中心，关于计时器中心 254 镜像。另修复用户指出的检视时占位手部球体，见 [美术接入记录](../Art/Integration.md)。

双端检查的初次及隔离重跑均在客户端阶段检查失败，诊断日志实际记录 `weapon=Weapon.UMP45 selected=0 health=73 protected=0`：服务器已执行下一阶段的离区和周期伤害，而客户端定时断言仍要求保护区内的 75 生命。`FPSLoadoutSmoke.cpp` 现改为在保护阶段实际观测武器、拒绝反馈、保护标记与 75 生命，再断言该状态曾完整复制；没有改动游戏网络或 GAS 行为。修正后 `Saved/UISampleMatch/LoadoutFinal/lag0-loss0-visual` 双端均 `PASS COMPLETE`，原失败日志保留在 `Loadout`、`LoadoutRetry` 与 `LoadoutDiagnostic`。

最终构建标识为 `UISampleMatch-20261012-v7`。独立包入口为 `Saved/Packages/IndustrialArt/UISampleMatch-20261012-v7/Windows/MCP_Test/Binaries/Win64/MCP_Test.exe`，分发时保留整个 `Windows` 目录。期间曾因其他构建的互斥锁及编辑器测试文件并发更新而使中间构建未通过一致性检查；最终包输入哈希一致，失败记录仍保留，未用于通过结论。

| 验证 | 结果与证据 |
| --- | --- |
| 编译、武器数据 | `Saved/UISampleMatch/Build.log` 编译成功，7 个武器定义检查通过 |
| UI 资产保存 | `Import.log` 通过；共 46 个 UI `.uasset`，包含 6 个完整 Widget Blueprint、6 个字体资产、公共主题和纹理 |
| 独立包 | v7 完整 BuildCookRun 通过，字体许可随包保留，输入哈希与工作区一致 |
| 最终包实际画面 | `AlignmentPackage` 保存 25 张截图；覆盖 1280×800 配装/等待/结算、1672×941 大厅、1920×1080 靶场、2560×1440 战斗 HUD |
| 交互 | 大厅模式切换、人数/难度、帮助、地址错误提示；真实 Slate 点击选枪/关闭和滚轮选取列表底部 UMP45 均通过 |
| 动作与小球修复 | 最终包靶场覆盖低弹药、换弹、检视、切枪；检视及切枪后的两个占位球均隐藏，当前手套可见 |
| 低血与死亡 | `HUDPackage/Training-1920x1080` 保留 v6 完整序列；v7 只追加键帽、标记、镜像比分及拾取布局，最终包没有重跑完整死亡序列 |
| 双端配装 | 上述 `LoadoutFinal` 的编辑器双端检查通过；该检查先于后续纯显示修正，不宣称最终独立包另跑过一套双端测试 |
| 原建模资产 | 96 个原有模型源文件及非 UI 美术资产哈希保持一致，未制作暂缓的高精模型 |

可复核的日志、截图及资源哈希由 [验证记录生成器](../../../Tools/record_fps_ui_verification.py) 汇总到 [本轮验证报告](../../../ArtSource/FPSDemo/Industrial/UI/SampleMatchVerification.json)。没有覆盖超宽屏、手柄导航、专项 GPU 性能、长局压力或 Shipping 验收。

## 选型与原版历史预览

### 中文字体候选对比（2026-10-12，选型已完成）

追加 [原版 / MiSans / HarmonyOS 三列对比图](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/ThreeWayComparison.png)：原版思源黑体也补拍为 1920×1080；下方按相同坐标展示标题和常规文字的 200% 局部。由 [版面生成脚本](../../../Tools/render_fps_font_comparison.cjs) 将原始截图放入 HTML/SVG 版面并导出 PNG，仅排版与缩放，不重绘字形。原版补拍日志位于 `Saved/FontComparison/Original/Lobby-1920x1080/Capture.log`，实机截图与合成图已人工核对。

按用户要求生成 MiSans 与 HarmonyOS Sans SC 两套候选，均为 Unreal 编辑器 `-game` 实际运行的 1920×1080 截图。保持布局、字号和 Regular/Bold 层级一致，英文与数字沿用 Rajdhani；字体本身的字面、基线不同，故截图保留其真实排版差异，未单独微调位置。当时尚未替换正式思源字体；用户随后选定 MiSans，当前实施与对齐结果见本文首节。

| 候选 | 大厅 | 配装 | 靶场 HUD |
| --- | --- | --- | --- |
| MiSans | [实机图](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/Previews/MiSans/Lobby.png) | [实机图](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/Previews/MiSans/Loadout.png) | [实机图](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/Previews/MiSans/HUD.png) |
| HarmonyOS Sans SC | [实机图](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/Previews/HarmonyOS/Lobby.png) | [实机图](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/Previews/HarmonyOS/Loadout.png) | [实机图](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/Previews/HarmonyOS/HUD.png) |

候选资产分别保存在 `/Game/FPSDemo/Art/FontComparisons/MiSans/UI` 和 `/Game/FPSDemo/Art/FontComparisons/HarmonyOS/UI`，每套 10 个 `.uasset`：6 个完整 Widget Blueprint、主题、复合字体及 2 个字体面。复用正式纹理和数字字体。原始字体、许可及官方来源见 [源文件目录](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/Fonts)；字体文件未经修改，预览界面显示使用字体名称。

构建与资产生成入口为 `Tools/BuildFPS.ps1`、[候选生成器](../../../Tools/build_fps_font_comparison.py)。运行 Unreal Python commandlet 时传入 `-FPSUIFontVariant=MiSans` 或 `-FPSUIFontVariant=HarmonyOS`。截图通过 `Tools/CaptureFPSArt.ps1 -Scene Lobby -FontVariant MiSans -OutputRoot Saved/FontComparison/MiSans`，再以 `-Scene Training` 拍摄 HUD 和配装；另一套替换字体参数和输出目录。非 Shipping 环境显式传入候选参数才切换资产路径。

选型阶段编译及武器数据校验通过；两套候选生成、加载及配装打开检查通过，日志位于 `Saved/FontComparison`。当时人工检查六张截图未见文字裁切，未单独校准字体基线；用户随后选择 MiSans，重新对齐及正式验证已在本文首节记录。历史生成记录见 [MiSans](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/MiSans-Assets.json) 和 [HarmonyOS](../../../ArtSource/FPSDemo/Samples/UI/FontComparisons/HarmonyOS-Assets.json)，其中“46 个正式 UI 资产哈希不变”仅指候选生成时没有改动当时正式资产，不适用于后续已授权的 MiSans 切换。候选自身未另做独立打包、多分辨率或双端验证。

### 正式版本历史截图

- [大厅与英文字体](../../../ArtSource/FPSDemo/Industrial/UI/Previews/SampleMatch/Lobby-1672x941.png)
- [配装圆角与关闭键帽](../../../ArtSource/FPSDemo/Industrial/UI/Previews/SampleMatch/Loadout-1280x800.png)
- [HUD 字号与镜像比分](../../../ArtSource/FPSDemo/Industrial/UI/Previews/SampleMatch/HUD-2560x1440.png)
- [拾取提示、检视与手套](../../../ArtSource/FPSDemo/Industrial/UI/Previews/SampleMatch/Inspect-1920x1080.png)

原始证据保存到 `Saved/UISampleMatch`，旧 `Saved/UIRefinement` 证据不作为本轮最终状态的证明。图片中的计时、配装和弹药来自真实游戏；场景与枪械轮廓来自现有基础资产，因此不声称复现参考图的三维写实质量，也不声称逐像素一致。

返回 [UI 索引](README.md)。

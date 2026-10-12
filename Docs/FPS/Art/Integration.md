# 基础美术游戏接入与验证

- 用途：实施与验证记录
- 适用范围：新 FPS 菜单、HUD、七枪、程序角色与竞技场/靶场；不含参考图级高精建模
- 状态：保留 2026-10-11 基础接入证据，另附 2026-10-12 占位手部修正；UI 已由后续资产化精修替代
- 最后更新：2026-10-12

## 1. 版本与范围

本文保留 2026-10-11 的实现和验证证据。后续 UI 已从 Slate/Canvas 主布局迁移为 Widget Blueprint，当前界面、资产和验证范围见 [UI 样稿对齐记录](../UI/SampleMatch.md)；本页旧截图与包不能作为新 UI 的验证证据。

2026-10-11 按用户“高精模型先记待办，完成其余美术”的要求实施。基于工作区 `6c98bad89fe9c8aec8a3731261033ece0dadc759` 之后的未提交改动；该提交号只定位仓库背景，不代表本批已提交。[交付记录](../../../ArtSource/FPSDemo/Industrial/DeliveryReport.json)记录文件哈希，[接入报告](../../../ArtSource/FPSDemo/Industrial/IntegrationReport.json)记录已保存资源。参考图级精修保持在[待办](Tasks.md)，不冒充已完成。

## 2. 实现与资产

| 工作线 | 本次实现 |
| --- | --- |
| 菜单 | 大厅单机/联机分栏、等待两队编组、暂停操作、个人成绩与前三名结算；统一按钮、色彩和 150 ms 淡入；操作帮助可展开 |
| 配装 | 主副武器分栏、真实模型剪影、当前装备标识、悬停/键盘焦点详情、处理中及服务器反馈；仍点击即领取保存 |
| HUD | 顶部时间与比分、左下生命、右下枪型和弹药；换弹进度、低血、低弹、拾取、击倒、死亡与重生提示；掉落达上限时显示短通知 |
| 枪械 | 七个定义的第一人称、第三人称和拾取模型绑定工业资产；按模型包围盒校准开镜高度与枪口；未修改伤害、射速、弹药、射程和换弹时长 |
| 人物 | 从原机器人派生 15 个运行时部件，适配现有程序关节；同源前臂/手套用于第一人称，覆盖持枪、移动、蹲伏、换弹、检视及死亡隐藏 |
| 地图 | 竞技场/靶场应用工业材质，墙板、立柱、货箱、屏障替换视觉表面；新增外观层不参与碰撞，原碰撞、出生点和玩法 Actor 保留 |

运行时新增 [RuntimeParts.blend](../../../ArtSource/FPSDemo/Industrial/RuntimeParts.blend)、15 个 FBX 与 `/Game/FPSDemo/Art/Industrial/Character/Runtime` 中对应 `.uasset`。原 `Operator.blend` 保留。资产库现在为 **60 个 `.uasset` 加 1 个展示地图**；两张正式游戏地图和七个武器定义另外保存于原位置。

UI 布局保留 C++ Slate/Canvas 实现，七张图标是已保存的 Unreal 纹理；没有另造同功能 UMG 蓝图。人物仍用程序关节表现，没有新增 AnimBP、动作片段或布料模拟。首批模型是工业训练机器人和基础硬表面枪械，未达到用户参考图的近景精细度。

## 3. 入口和复现

- [资产使用说明](AssetUsage.md)：资源位置、制作与更新约定。
- [导出运行时部件](../../../Tools/export_industrial_runtime_parts.py)：通过已连接的 Blender MCP 执行，保存派生文件。
- [Unreal 接入脚本](../../../Tools/integrate_industrial_art.py)：导入部件、修改七枪外观引用、更新两地图。只清理脚本自己的 `IND_ART_` 外观层，重复执行不累积该层。
- [截图入口](../../../Tools/CaptureFPSArt.ps1)：在真实游戏中采集大厅、等待、结算、HUD、配装、开镜、换弹、检视与角色近景；`-ForceRes` 保证指定分辨率。
- [Windows 打包入口](../../../Tools/BuildFPSArt.ps1)：输出新目录，并核对打包前后源代码、配置及相关资源哈希，防止证据混用。

截图模式仅在非 Shipping 构建且显式提供 `-FPSArtCapture` 时启用；测试会实际切枪、开镜、换弹、改变生命并结束测试进程，不是普通游玩入口。不要用截图模式继续正常对局。

## 4. 验证记录

| 验证 | 当前结果与证据 |
| --- | --- |
| Blender 源文件 | 通过。四个源库可重开、38 个 FBX 存在；15 个运行时网格含面和 UV；原骨骼权重与手部形变检查保留。`ArtSource/FPSDemo/Industrial/SourceValidation.json` |
| 编辑器导入 | 通过。15 个部件、7 个武器定义和2张地图已保存；原地图 Transform/碰撞配置与枪械数值校验通过。`IntegrationReport.json` |
| C++ 与武器数据门禁 | 通过。`Saved/IndustrialArt/IntegrationBuild.log`；武器资料由 `ValidateFPSWeaponData.ps1 -Export` 重新导出 |
| 地图运行 | 编辑器及独立包均通过。竞技场 10 出生点/14 拾取物/10 角色，靶场 4/7/4，出生胶囊和地面检查正常。`Saved/IndustrialArt/MapTests`、`PackagedMaps` |
| 配装双端 | 通过。立即选择、保存、同型号补给、快捷键、离区/死亡关闭与重生权限。`Saved/IndustrialArt/LoadoutTests` |
| 分辨率及状态截图 | 1280×800、1920×1080、2560×1440 均通过尺寸核对；逐图检查 HUD、配装、开镜、换弹、检视、手枪。1080p 完整序列额外包含七枪、角色站姿/蹲姿、低血、低弹、死亡。1280×800 大厅/等待/结算在独立包核对。`Saved/IndustrialArt/Captures` |
| 远端武器表现与掉落通知 | 通过。服务器、客户端及晚加入观察端的武器姿态、完成事件与去重；回收上限提示在双端出现且原掉落测试完成。`Saved/IndustrialArt/PackagedPresentation`、`DropTests` |
| Windows Cook 和包内运行 | `ArtIntegration-20261011-v2` 完整 BuildCookRun 及包内功能/截图通过；最终 `ArtIntegration-20261011-v3` 构建通过，只调整红队文字/色标间距，并刷新包内地图截图。记录位于 `Saved/IndustrialArt/Package/ArtIntegration-20261011-v3/Report.json` |

首次实机检查发现 Canvas 使用没有 `UFont` 的 Slate 样式会跳过文字，已改为硬引用运行时 Roboto 字体资产；中文通过引擎字体回退显示。首次截图受窗口尺寸约束变成 888×500，已加入 `-ForceRes`，旧截图不作为目标尺寸通过证据。构建曾因自动生成武器资料过期而失败，重新导出后门禁通过。

配装最初整页滚动会隐藏底部状态，现仅武器列表滚动，关闭和反馈保留在固定区域。测试入口 `TestFPSWeapons.ps1` 修正单元素数组被 PowerShell 展平造成的命令行拼接，并规范日志绝对路径。编辑器晚加入端启动过慢，错过检视窗口；独立包重测后，观察端在动作剩余约 14.6 秒加入，姿态恢复及完成检查均通过。它们属于测试过程中发现并处理的问题，未隐藏失败记录。

游戏模式加载时会出现引擎实验性 Python Toolsets 的类型缺失日志；本轮没有修改该引擎插件。这些日志与本次美术资源加载失败不同，功能通过依据是对应测试明确的结束标记和实际画面。

最终本机验收包入口为 `Saved/Packages/IndustrialArt/ArtIntegration-20261011-v3/Windows/MCP_Test/Binaries/Win64/MCP_Test.exe`。完整分发需要保留对应 `Windows` 目录，不只复制 exe。打包报告包含输入与 exe 哈希；[交付记录](../../../ArtSource/FPSDemo/Industrial/DeliveryReport.json)同时保存本轮测试关键行和证据文件哈希。普通打开 exe 不会启用截图测试序列。

文档检查 `python Tools/CheckDocs.py`：52 份文档、0 错误、102 项既有历史例外；未扩大豁免。补丁空白检查通过。人工核对了分类、索引、历史与当前状态、静态样稿/实机证据的区分及暂缓项目未误勾完成。

## 5. 实际画面与边界

以下是独立 Windows 包的真实截图，未经图像生成或修图。完整预览在 [Game 目录](../../../ArtSource/FPSDemo/Industrial/Previews/Game/)。静态样稿仍放在 [UI 样稿记录](../UI/MockupReview.md)，不与实机图混用。

![大厅，1280×800](../../../ArtSource/FPSDemo/Industrial/Previews/Game/Lobby-1280x800.png)

![战斗 HUD 和靶场，1920×1080](../../../ArtSource/FPSDemo/Industrial/Previews/Game/HUD-1920x1080.png)

![配装，固定关闭入口和反馈栏](../../../ArtSource/FPSDemo/Industrial/Previews/Game/Loadout-1280x800.png)

![游戏内机器人基型与持枪](../../../ArtSource/FPSDemo/Industrial/Previews/Game/Character-1920x1080.png)

大厅当前使用轻量枪械剪影识别入口，没有额外实时场景预览。原场景的功能英文标识保留；玩家界面使用中文，旧教学 HUD 保留原实现。顶部比分最终间距见 [竞技场截图](../../../ArtSource/FPSDemo/Industrial/Previews/Game/Foundry.png)。

本记录的“通过”限于上述测试，不能推导为所有硬件性能、所有极端网络条件或高精模型验收。现有刚性手套、装甲分件和程序换弹只满足基础表现；手指精细握持、独立弹匣动作、织物细节、磨损贴图及完整骨骼动画属于暂缓精修。

未进行专项 GPU 性能基准、10 人真实玩家长局压力或所有武器/动作/光照组合的人工巡检；地图碰撞与出生检查不等于全图 AI 路径穷举。以上不将基础美术接入升级为商业成品质量承诺。

## 6. 2026-10-12：检视时残留的占位手部球体

用户指出检视枪械时下方出现小球。实际复现见 `Saved/UISampleMatch/InspectBefore/Training-1280x800/Inspect.png`。它来自角色构造器中的 `FPLeftHand` / `FPRightHand` 球形占位手，而非枪械零件或碰撞调试显示。

工业手套加载成功后，`UFPSAvatarVisuals::Build` 原来只调用 `SetVisibility(false)` 隐藏旧组件；随后 `UFPSCharacterPresentation::Refresh` 对 `GunRoot` 递归恢复可见性，导致占位手重新出现。现为已替换的旧组件同时设置 `HiddenInGame`，使其不受武器可见性刷新影响；原手套和前臂继续显示，资源缺失时的旧模型回退仍保留。未修改检视、射击、配装或 GAS 网络规则，也未重制网格。

截图回归入口增加检查：检视动作确实处于执行中，两个旧球形手部在检视和切副武器后均隐藏，两个工业手套仍可见。`UISampleMatch-20261012-v5` 独立包实际运行通过，证据为 `Saved/UISampleMatch/Package/Training-1920x1080/Capture.log` 中 `inspect-pose-active` 和两次 `legacy-hand-spheres-hidden-current-gloves-visible` 通过标记；人工复核 `Inspect.png`，原先悬空的小球消失。该目录保留后续 HUD 调整前的画面，当前 UI 版本见 [样稿对齐记录](../UI/SampleMatch.md)。

返回[统一美术索引](README.md)。

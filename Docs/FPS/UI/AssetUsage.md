# UI 资产使用与维护

- 用途：当前使用说明
- 适用范围：FPS 行动终端、配装、HUD、等待、暂停与结算界面
- 状态：六个界面均保存为 Widget Blueprint；正式中文字体为 MiSans；体力条改为下方居中的细白条，其余对齐沿用原版
- 最后更新：2026-10-13

## 资产入口

当前已验证运行包为 `MiSans-20261012-v1`，启动路径、实机截图和验证边界统一维护在 [MiSans 实施记录](SampleMatch.md)。日常运行无需传入 `-FPSUIFontVariant`；该参数会加载历史候选目录，不包含正式版的 MiSans 对齐补偿。

Unreal 内容浏览器进入 `/Game/FPSDemo/Art/Industrial/UI/Widgets`。双击以下资产，在 **Designer** 中编辑布局、字号、图片、颜色、间距与按钮。

| 资产 | 用途 |
| --- | --- |
| `WBP_FPSLobby` | 左侧模式导航、真实场景预览、单机配置、局域网与靶场入口、操作帮助 |
| `WBP_FPSLoadout` | 可滚动的五张主武器卡、两张副武器卡、装备标记、详情与服务器反馈 |
| `WBP_FPSHUD` | 比分时间、生命、体力、弹药、换弹进度、击倒、死亡和拾取提示 |
| `WBP_FPSWaiting` | 阵营成员、本人/房主身份、切换阵营、开始条件 |
| `WBP_FPSPause` | 对局菜单、继续游戏、返回与房主离开说明 |
| `WBP_FPSReport` | 个人击倒/死亡、前三名与返回入口 |

这些 `.uasset` 保存了完整控件树，游戏直接创建这些类；不是空蓝图或仅用于展示的截图。`UFPSInterface` 提供状态刷新与按钮行为，`AFPSFrontController` 保留对局、配装权限与网络请求。旧教学关卡仍使用原教学 HUD；准星、受伤闪烁和命中标记保留轻量 Canvas 绘制。

`UI/DA_FPSUITheme` 保存公共色板与三种按钮样式；动态状态颜色读取该资产。静态控件的样式和字号已写入各 Widget Blueprint，编辑公共色板不会自动改写所有静态控件。

`UI/T_UI_Panel` 是可九宫格缩放的切角面板；`T_UI_Foundry` 是当前铸造场的真实截图。七张 `T_UI_型号` 是从已保存枪械模型渲染的带材质缩略图，配装使用它们；原有 `T_IND_型号` 剪影继续用于 HUD。新增背景、面板、纸质卡片、圆角边框、预览渐变及书本/退出/护盾/勾选/提示/信号/箭头等图标也各自保存为纹理；正文文字、按钮和布局没有烘焙进整页图片。

`UI/Fonts/F_UI_Text` 包含 **MiSans Regular/Bold** 两个字体面，`F_UI_Number` 使用 Rajdhani SemiBold/Bold，合计两个 Font 与四个 FontFace 资产。`F_UI_Text` 的拉丁字符子字族同样指定 Rajdhani，让混排标题中的英文保持工业字形。卡片和关闭键帽使用完整的圆角 Brush 描边，参数随 Widget Blueprint 保存。原始字体及授权位于 [Fonts](../../../ArtSource/FPSDemo/Industrial/UI/Fonts/)，来源见 [SOURCES.txt](../../../ArtSource/FPSDemo/Industrial/UI/Fonts/SOURCES.txt)。字体随项目加载，不依赖目标电脑安装中文字体。独立包的授权文件放在 `Windows/ThirdPartyNotices/FPSUIFonts`，包含 MiSans 的 PDF 许可；大厅页脚标明使用 MiSans。源目录中的思源字体及候选对比截图仅作为历史参考。

MiSans 的导航、按钮、配装标题/装备标记/关闭键帽、HUD 队名/换弹/拾取等位置已在生成器 `FPage::AlignMiSans` 中校准，并保存到正式 Widget Blueprint。运行时不额外移动这些文本；后续换字体仍需重新检查真实字形，不能将这些补偿量直接套用到其他字体。

## 体力条

2026-10-13 按用户要求，将 `WBP_FPSHUD` 中原左下体力面板替换为屏幕下方居中的细长白条。当前保持 `255×9`（1600×900 基准），位置由 `(672.5,700)` 下移 40 至 `(672.5,740)`，横向中心为 X=800。随现有 ScaleBox 等比缩放。

`StaminaBar` 为白色前景，新增 `StaminaDelayBar` 为同尺寸淡灰色过渡层，底槽为 28% 不透明度黑色。消耗时白色先缩短、灰色稍后收回；恢复时灰色先向前、白色稍后追上。方向开始或反转时延迟 0.16 秒，再按 0.20 秒时间常数平滑跟随；快速层时间常数为 0.055 秒，连续消耗/恢复不反复重置延迟。`FFPSStaminaPresentation` 只维护本地显示值，不改实际体力和动作判定。

满体后等待动画收敛再隐藏，死亡、换角色、关闭相关 HUD 时重置动画，避免残留旧体力。旧标题、数值、状态文本和大背景控件均保存为 Collapsed。移动、GAS、同步和体力数值规则未改。

完整样式更新入口为 `FPSUIAssetBuilder::ExtendMovementAssets` 中的 `ApplyMinimalStamina`，通过 `Tools/extend_fps_movement_ui.py` 增量保存；不重建其他菜单、字体或 HUD 控件。仅调整位置和粗细时，通过 [update_fps_stamina_layout.py](../../../Tools/update_fps_stamina_layout.py) 同步面板、白条和已存在的灰条 Canvas 槽位，编译蓝图并保存，不依赖重新编译 C++。动画行为代码变更仍需编译。

当前权威界面是已保存的 `WBP_FPSHUD.uasset`。生成器和 Canvas 后备绘制已同步尺寸与双向平滑行为；重新调用完整样式更新入口前应使用与当前源文件一致的编译模块，避免恢复旧参数。日常运行与 Designer 编辑不需要执行生成器。

本次构建、资产保存及实机验证边界见 [体力条简化记录](../Characters/MovementImplementation.md#体力条简化2026-10-13)；此前 MiSans 独立包不包含本轮体力条修改。

[当前下移及延迟过渡实机截图](Images/StaminaBarSmooth.png) 为 1280×720 移动测试夹具中的体力恢复状态，使用已保存的 HUD 资产。[前版靶场截图](Images/StaminaBar.png) 对应 Y=800、5 像素粗的历史版本。

## 编辑约定

- 按用户要求，每次 UI 视觉修改交付时附上对应已保存版本的最新实机截图；旧图不能作为新布局的效果展示。
- 设计基准为 1600×900，通过 ScaleBox 等比适配；16:10 保持控件比例，额外区域由背景填充。
- 控件名字用于绑定动态文本和状态。移动、缩放和改样式可以直接操作；重命名动态控件时，同步更新 `FPSInterface.cpp` 中的查找名称。
- `FPSActionButton` 的 `Action`、`WeaponId` 是资产中的可编辑字段。配装按钮仍然点击即请求服务器发放，不增加确认步骤；黄标显示已装备，右侧显示正在查看的武器。
- 迁移界面到其他项目时，需要同步 `UFPSInterface`、`UFPSActionButton`、`UFPSUITheme` 及其玩法绑定；控件树和视觉样式保存在资产中，游戏行为依赖对应 C++ 类。
- 模型继续以 `.blend`、FBX 与 Unreal 网格保存，位置见 [美术资产说明](../Art/AssetUsage.md)。本次没有制作暂缓的参考图级枪械、人物精模。

## 重建入口

源图片位于 [UI 源资源](../../../ArtSource/FPSDemo/Industrial/UI/)，资产清单见 [生成报告](../../../ArtSource/FPSDemo/Industrial/UI/AssetReport.json)。

以下生成器用于重建当前样稿对齐版，会覆盖同名图片、主题和界面布局。**在 Designer 中手工调整后，应保留 `.uasset` 作为当前权威版本；需要重新运行生成器时，先另存手工版本，或将修改合入生成器。** 日常使用与编辑不需要重新生成。

```powershell
python Tools/prepare_fps_ui_shapes.py
& 'D:\Blender\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/render_fps_ui_weapons.py
powershell -ExecutionPolicy Bypass -File Tools/BuildFPS.ps1
powershell -ExecutionPolicy Bypass -File Tools/ImportFPSUI.ps1
```

布局生成代码是 [FPSUIAssetBuilder.cpp](../../../Source/MCP_Test/FPSUIAssetBuilder.cpp)，导入入口是 [ImportFPSUI.ps1](../../../Tools/ImportFPSUI.ps1)。场景预览源通过 [capture_fps_ui_background.py](../../../Tools/capture_fps_ui_background.py) 在编辑器中离屏拍摄，不保存或修改地图。

实际验证、截图和未覆盖项见 [样稿对齐记录](SampleMatch.md)；[首轮精修记录](Refinement.md) 保留历史证据。返回 [UI 索引](README.md)。

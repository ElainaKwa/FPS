# UI 精修与资产化记录

- 用途：实施与验证记录
- 适用范围：FPS 六个界面、共享主题、UI 图片；保留现有三维资产和玩法
- 状态：首轮资产化的历史实施记录；用户随后要求进一步对齐样稿，当前版本见样稿对齐记录
- 最后更新：2026-10-12

本文记录首轮资产化，不代表用户已验收视觉精细度。当前字体、几何框及滚动布局由 [样稿对齐记录](SampleMatch.md) 承接；下方哈希和截图仅对应首轮版本。

## 版本与改动

针对“UI 细节仍未达到样稿、UI 和建模都要保存成资产”的反馈，在当前工作区实施。本批未单独提交；最终文件哈希与验证证据见 [验证清单](../../../ArtSource/FPSDemo/Industrial/UI/Verification.json)，生成器为 [record_fps_ui_verification.py](../../../Tools/record_fps_ui_verification.py)。

- 大厅改为三项左侧导航、真实铸造场预览、浅色配置区和黄色主操作按钮；保留单机、人机难度、局域网创建/加入与靶场入口。
- 配装改为浅色切角卡片、深色详情区、黄色装备标记、弹匣/换弹信息。五把主武器同屏显示；查看和已装备状态分开，实际装备仍由服务器确认。
- 七张缩略图从已保存枪械模型重新渲染，呈现已有几何与材质细节；HUD 保留清晰剪影。
- 等待、暂停、结算采用一致的标题、分隔线、队伍行与操作区。HUD 保留顶部比分时间、左下生命、右下弹药，提示按实际游戏状态出现。
- 六个布局保存为可编辑 Widget Blueprint，另保存主题 Data Asset 和九张纹理。游戏直接创建这些蓝图类，不再由 Slate 代码临时拼装主要页面。

资产路径、编辑和生成约定见 [UI 资产说明](AssetUsage.md)。高精度枪械与人物制作继续放在 [暂缓待办](../Art/Tasks.md)，当前贴图来自既有基础模型，不能作为已达到参考图级建模质量的证明。

## 验证记录

本轮日志统一位于 `Saved/UIRefinement`；编辑器编译与导入日志位于 `Saved/IndustrialArt/UIBuild.log` 和 `UIImport.log`。此前 `ArtIntegration-20261011-*` 的包及截图只作为历史基础接入证据。

首轮实机发现了 NullRHI 生成环境下的图片显示尺寸、关闭按钮换行和特殊箭头字形问题，已修复。重复生成时出现的控件 GUID 校验也已补齐。后续验证以修复后的截图与日志为准。

| 检查 | 结果与边界 |
| --- | --- |
| 编译与资产保存 | 编辑器编译和七武器数据校验通过；六个 Widget Blueprint、一个主题、九张新纹理保存成功，重新加载后游戏直接使用对应蓝图类 |
| UI 鼠标事件 | 通过 Slate 实际命中路径依次路由移动、按下与松开；大厅导航、人数调节、难度、帮助、非法地址反馈，以及配装 AK 后服务器确认、关闭按钮均通过；日志含 `FPS_UI_ACTION PASS` |
| 双端与快捷键 | `Saved/UIRefinement/Loadout/lag0-loss0-visual` 的服务器和客户端均 `COMPLETE`；覆盖 P/Esc、立即选择、保存、同型号补给、离区/死亡关闭、重生后权限 |
| 分辨率与状态 | 1280×800、1920×1080、2560×1440；48 张原始截图经尺寸检查。人工查看大厅、配装、战斗 HUD、等待、暂停、结算、低血与死亡；完整 1080p 序列保留换弹/检视和七枪切换画面 |
| 独立包 | `UIRefinement-20261012` 完整 BuildCookRun 与 1080p 状态序列通过；`UIRefinement-20261012-v2` 修复结算提示后再次构建通过，并在包内复核大厅鼠标交互、结算、1440p 配装交互及竞技 HUD |
| 既有建模资产 | 对此前交付记录核对 96 个建模源文件、FBX 及相关 Unreal 网格/材质/展示地图哈希，均未改变；无需重新制作模型即可使用新 UI |
| 文档 | 运行 `python Tools/CheckDocs.py`；自动检查不替代本文对证据和状态的人工核对 |

最终独立包入口：`Saved/Packages/IndustrialArt/UIRefinement-20261012-v2/Windows/MCP_Test/Binaries/Win64/MCP_Test.exe`。完整构建记录位于 `Saved/IndustrialArt/Package/UIRefinement-20261012-v2/Report.json`；验证清单同时核对该包的源代码、配置与相关资产输入哈希。

截图来自真实运行，未绘制伪造游戏数值。以下精选图随资源保存到 [UI 实机预览目录](../../../ArtSource/FPSDemo/Industrial/UI/Previews/)，原始截图与日志位于 `Saved/UIRefinement/Final`。

![大厅 1280×800](../../../ArtSource/FPSDemo/Industrial/UI/Previews/Lobby-1280x800.png)

![配装 1280×800](../../../ArtSource/FPSDemo/Industrial/UI/Previews/Loadout-1280x800.png)

![竞技 HUD 1920×1080](../../../ArtSource/FPSDemo/Industrial/UI/Previews/CombatHUD-1920x1080.png)

## 验证边界

本轮确认布局、图标、字体、主要交互和资源随包加载；未进行超宽屏、手柄导航、性能压力或 Shipping 构建验收。离屏鼠标事件测试不等同于真人长时间可用性评审。场景、枪械和人物仍是此前保存的基础美术，图中精模细节继续按用户要求暂缓。本轮实机是对样稿视觉层次的改进，不宣称达到样稿的全部三维渲染质量。

返回 [UI 索引](README.md)。

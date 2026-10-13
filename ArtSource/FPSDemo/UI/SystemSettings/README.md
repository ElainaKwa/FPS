# 系统设置 UI 套件

制作方式：原生行为类 + 编辑器生成的 Widget Blueprint。新资产保存于 `/Game/FPSDemo/Art/UI/SystemSettings/`，源脚本随本套件维护。

- 布局：`WBP_SystemSettings`；通用滑条、下拉框、绑定/试听选项行：`WBP_SystemSettingsRow`。
- 权威生成器：`Source/MCP_Test/FPSSettingsUI.h/.cpp` 的 `UFPSSettingsUIBuilder::BuildAssets`；行为连接 `UFPSSystemSettings`，选项范围和数值由后端提供。
- 入口：统一编译后在 Unreal Python 运行项目根 `Tools/build_system_settings_ui.py`，转发至本目录 `Scripts/build_system_settings_ui.py`。只创建缺失资产，已有 Designer 修改及其他包保持不变；不通过重跑脚本覆盖现有布局。
- 依赖：既有 `Industrial/UI/DA_FPSUITheme`、`Fonts/F_UI_Text`（MiSans / Rajdhani 复合字体）、`Fonts/F_UI_Number`。授权保留在原字体源目录，不另复制字体。
- HUD 示意使用保存的几何图形与文字，展示缩放比例；没有准星编辑、模型或位图生成依赖。
- 运行依赖：`UFPSSettingsUI`、`UFPSSettingsRow`、`UFPSSettingsButton`、`UFPSSystemSettings` 与 `UFPSUITheme`。跨项目迁移必须携带依赖，不能仅复制两份 `.uasset`。
- 生成记录：`Saved/FPSSettingsTests/UIAssets/<时间>/Report.json`。源码/脚本完成不代表资产已生成或实机验收；集成说明与验证边界见 [系统设置 UI 接入](../../../../../Docs/FPS/UI/SettingsUI.md)。

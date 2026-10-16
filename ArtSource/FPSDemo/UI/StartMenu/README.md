# StartMenu

统一模式选择页的维护源，当前已保存资产为 `/Game/FPSDemo/Art/UI/StartMenu/WBP_FPSStart`。

`Scripts/build_start_menu.py` 通过 `UFPSUIAssetBuilder::BuildStartMenuAssets` 创建缺失的开始界面，并增量保存多人对战返回入口、工厂大厅返回入口和行动文案。已有开始界面保留 Designer 编辑。日常运行不需要执行生成器。

主题和字体复用 `/Game/FPSDemo/Art/Industrial/UI/`；场景图片复用 `T_UI_Foundry` 和 `/Game/FPSDemo/Art/UI/ExtractionTerminal/Textures/T_LobbyIndustrialCrossing`。没有新增外部图像或字体来源。备份、保存摘要放在项目 `Saved/StartMenu/Authoring/`。

入口为 `Tools/build_fps_start_menu.py`。使用与当前源码一致的编译模块在 UE Python commandlet 执行；运行验证为 `Tools/TestFPSStartMenu.ps1`。

2026-10-16 按用户要求删除开始界面字体提示：已有资产通过 `Scripts/remove_font_notice.py` 增量删除 `FontNotice`，新建生成器也不再创建它。字体与许可原文件继续保留。

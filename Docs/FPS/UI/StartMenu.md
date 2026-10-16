# 模式选择与回收行动入口

- 用途：当前使用说明
- 适用范围：统一开始界面、多人对战子页面、个人工厂大厅与行动往返
- 状态：已保存为 Widget Blueprint；三种目标分辨率的实际入口往返通过，验证边界见文末
- 最后更新：2026-10-16

## 从哪里进入

运行项目根目录的 `Tools/LaunchFPSDemo.ps1`，或从默认 Entry 地图启动游戏，首先显示模式选择页。

| 入口 | 去向 |
| --- | --- |
| 多人对战 | 原有建房、IPv4 加入房间、单机训练与靶场；默认选中联机页 |
| 回收行动 | 个人工厂大厅；“行动整备”进入仓库和背包，“开始行动”进入工业关隘 |
| 设置 | 现有五分类设置；返回后仍在模式选择页 |
| 退出游戏 | 关闭游戏 |

“回收行动”为用户确认的暂定名称。顶部模式名、行动整备、行动背包、开始行动、返回工厂均为可编辑文字；内部 `Extraction`、`Depart`、`ReturnToBase` 标识保留，库存与结算规则沿用原实现。

多人对战子页点击“模式选择”或按 Esc 返回开始界面。工厂大厅顶部也有“模式选择”。行动结算中的“返回工厂”会回到工厂并自动打开大厅；从车间按 Tab 可打开大厅。车间使用行动配装的第一人称角色、无 HUD，禁止开火；回收行动的游戏视角会捕获鼠标，菜单光标保持在窗口内。对局菜单原“返回”现回到统一开始界面。

`Tools/LaunchFPSExtraction.ps1` 保留直接打开回收行动大厅的用途；`Tools/LaunchFPSFactory.ps1` 保留从可行走车间开始的用途。

## 编辑与维护

开始界面资产：`/Game/FPSDemo/Art/UI/StartMenu/WBP_FPSStart`。双击后在 Designer 编辑两张大卡片、标题、说明和页脚，不需要重新生成。按钮的 `Action` 分别为 `Multiplayer`、`Recovery`、`InputSettings`、`Quit`。

2026-10-16 用户要求移除开始界面的“本软件使用 MiSans 字体”页脚；字体资源及许可文件保留。增量处理入口为 `ArtSource/FPSDemo/UI/StartMenu/Scripts/remove_font_notice.py`，新建界面生成器也不再添加该控件。

原 `/Game/FPSDemo/Art/Industrial/UI/Widgets/WBP_FPSLobby` 增加 `Modes` 返回按钮。`/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal` 增加 `LobbyModes`，Action 为 `Modes`；已有仓库、加工和改枪台控件树保持原结构。

使用现有共享主题、MiSans / Rajdhani 字体及两张真实场景预览，没有新增外部图片。设计画布为 1600×900，以 ScaleBox 等比适配。源入口为 [build_start_menu.py](../../../ArtSource/FPSDemo/UI/StartMenu/Scripts/build_start_menu.py)，兼容命令入口为 `Tools/build_fps_start_menu.py`。编译后通过 UE Python commandlet 执行。脚本仅在开始界面不存在时创建它；对既有多人对战／工厂界面增量更新返回按钮和命名，并保存工厂的“行动整备”站点名称。运行前会备份涉及的既有资产到 `Saved/StartMenu/Authoring/<时间>/`。

行为代码为 [FPSFrontController.cpp](../../../Source/MCP_Test/FPSFrontController.cpp)、[FPSInterface.cpp](../../../Source/MCP_Test/FPSInterface.cpp)、[FPSFactoryBase.cpp](../../../Source/MCP_Test/FPSFactoryBase.cpp) 与 [FPSExtractionUI.cpp](../../../Source/MCP_Test/FPSExtractionUI.cpp)。`FactoryLobby=1` 随地图 URL 传递，所以从模式选择进入和结算返回不依赖启动时额外传入 `-FactoryLobby`。

## 本轮验证与截图

2026-10-16：UE 5.8 Development Editor 编译、武器数据校验和实际入口往返已通过；分辨率、版本摘要及验证边界见 [开始界面实施记录](StartMenuImplementation.md)。

![当前保存版本的开始界面](Images/StartMenu.png)

![回收行动大厅及新的行动按钮](Images/StartMenuRecovery.png)

复验入口：`Tools/TestFPSStartMenu.ps1 -Width 1920 -Height 1080`（可改为 1280×800 或 2560×1440）。测试使用隔离存档、冻结模块和实际 Slate 按钮路径；保留截图、日志、资产摘要及 `run.json`，结束后自动删除临时运行副本和测试存档。它检查建房等待室，未承诺双客户端联机、真人硬件输入、PIE 或独立打包验证。

设计依据：[UI 设计决策](DesignDecisions.md)。工厂功能：[回收行动与个人工厂 UI](ExtractionUI.md)。

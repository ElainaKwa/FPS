# 开始界面实施与验证

- 用途：实施与验证记录
- 适用范围：2026-10-16 模式选择、多人对战子页和回收行动入口
- 状态：已实现；编译、三种分辨率的实际入口往返和文档检查通过；未做双客户端、PIE 与打包验证
- 最后更新：2026-10-16

## 实施

根据 [已确认设计](DesignDecisions.md#统一开始界面2026-10-16)，新增保存的 `WBP_FPSStart`，默认 Entry 地图和 LobbyMode 显示模式选择。原建房、加入、训练和靶场保留在“多人对战”子页面，默认选中 LAN。`Modes` 按钮和 Esc 返回开始界面。

“回收行动”进入原个人工厂地图，URL 显式传入 `FactoryLobby=1`，工厂控制器据此打开大厅。结算的 `ReturnToBase` 同样携带此选项，回到大厅后可通过 `LobbyModes` 返回开始界面。库存、经济与结算规则不变。

增量保存多人对战返回按钮、工厂大厅返回按钮、静态行动文案及工厂站点 DisplayName；运行提示和既有生成器同步名称。新开始界面复用既有主题、字体和场景纹理。`Config/DefaultGame.ini` 加入 StartMenu 与 ExtractionTerminal 的 cook 目录，实际打包仍未验证。

资产保存及原文件备份：`Saved/StartMenu/Authoring/20261016-160135/Report.json`。报告记录修改前后四个资产的 SHA-256；除新开始页外，其余页面保留现有布局结构。当前实机截图放在 [使用说明](StartMenu.md#本轮验证与截图)。

## 字体提示移除（2026-10-16）

按用户要求，仅删除开始界面 `FontNotice` 控件，保留共享字体与许可文件；生成器也不再创建该提示。增量保存脚本为 `ArtSource/FPSDemo/UI/StartMenu/Scripts/remove_font_notice.py`，修改前备份和保存摘要见 `Saved/StartMenu/Authoring/FontNotice-20261016-212004/Report.json`。

当前保存版本已由 `Saved/StartMenu/Tests/20261016-212437090/` 的真实 1920×1080 运行检查确认控件不存在，并查看 `Start.png` 无页脚提示，已更新使用说明中的截图。本轮模块及完整行动验证见 [回收行动记录](../Extraction/Implementation.md#西侧出生东侧撤离与实战配件2026-10-16)。下方三分辨率数据保留为最初布局交付记录，本次文字删除未重跑另外两种分辨率。

## 验证

以下为开始界面首轮交付记录；后续第一人称工厂、行动配装、禁火及鼠标捕获验证见 [工厂追加记录](../Maps/FactoryBase.md#第一人称与鼠标捕获修复2026-10-16)。

最终编译通过，日志 `Saved/StartMenu/Build.log`；构建后武器数据校验输出 `FPS_WEAPON_DATA PASS definitions=7`。已有测试代码的 InputKey 弃用警告未在本任务扩展修复。

测试脚本为 `Tools/TestFPSStartMenu.ps1`，状态机为 `UFPSStartMenuVerification`。使用冻结模块副本、隔离 Extraction / Settings 存档和独立设置文件，实际经 Slate 命中路径点击保存的按钮，Esc 通过 Slate 键盘事件输入。模拟结算调用既有服务端 `FinishExpedition(true)`，不声称通过自然战斗抵达撤离点。

已覆盖开始界面设置及返回、默认 LAN、无效 IPv4 拒绝、返回按钮、Esc、靶场加载及返回、零 AI 单机训练及返回、真实 Listen Server 等待室及返回、回收行动大厅、行动整备／背包、工业关隘加载、结算返回工厂大厅、工厂返回模式选择、退出游戏。

| 分辨率 | 结果 | 证据目录（项目根目录下） |
| --- | --- | --- |
| 1920×1080 | PASS | `Saved/StartMenu/Tests/20261016-160943146/` |
| 1280×800 | PASS | `Saved/StartMenu/Tests/20261016-161058403/` |
| 2560×1440 | PASS | `Saved/StartMenu/Tests/20261016-161213352/` |
| 1280×800（截图补验） | PASS | `Saved/StartMenu/Tests/20261016-161705696/` |

三轮功能验证使用相同模块 SHA-256：`A8F437D4F859193468214845C18EE1E04A10465BC45A11AE65FEFF9E2A0BC8B1`，每轮 45 条阶段检查及 `PASS COMPLETE` 均通过。已人工查看三种分辨率的开始页及 1920／1280 的工厂大厅，没有观察到新文案截断或按钮重叠。

每轮目录含 `run.json`、`Game.log` 和原始截图，记录模块、三张界面资产及工厂地图摘要；测试完成后删除自己的 Runtime 副本与其中的测试存档，先解除 Content Junction，再删除剩余目录，原 Content 保留。

截图人工核对发现前三轮的 `Inventory.png` 在请求截图的同一帧切回了大厅，不能用作整备页证据。已改为点击“行动整备”后采集，并在下一检查阶段返回大厅，产品行为和资产不变。前三轮的错误命名截图已清理，其他截图仍有效。补验的 45 条阶段检查与完整九张截图均通过；已人工检查整备页实际显示“行动背包”。补验最终模块 SHA-256 为 `C8F19DDF5D088788CB0509D2718CFD1D6BC8DD7C198FDA7463B94070BE9A5FD2`，与前三轮的代码差别仅为验证器截图采集时机。

首次两轮失败来自新验证器的控件定位：原子页按钮实际名是 Nav0/Nav2，以及回收行动控制器继承 FrontController 导致结算页未被验证器找到。定位已修正后重跑；失败日志保留作追溯，不作为功能通过证据。失败轮的过时截图已清理，记录见 `Saved/StartMenu/Cleanup.json`。

本轮没有第二客户端联机、硬件鼠标键盘、PIE 或打包运行验证。UE 自带 Python 测试插件在 `-game` 启动时的 `PythonTestRunner` 报错不影响这些入口断言；没有将它计为本项目功能验证通过。

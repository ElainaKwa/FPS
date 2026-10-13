# 系统设置 UI 接入

- 用途：当前使用说明
- 适用范围：五分类设置页面、选项行、确认弹窗、键鼠捕获及 HUD 缩放示意
- 状态：UI 源码、两个保存的 WBP 和宿主入口已接入；本轮运行边界见实施记录
- 最后更新：2026-10-14

设计依据为 [系统设置设计决策](SettingsDesignDecisions.md)，完整实施边界见 [任务清单](SettingsTasks.md)。UI 仅连接后端 API，不直接改配置、画面、声音、输入映射或游戏数值。版本标记 `SettingsLayoutVersion=1`；当前源码为 [FPSSettingsUI.h](../../../Source/MCP_Test/FPSSettingsUI.h) 与 [FPSSettingsUI.cpp](../../../Source/MCP_Test/FPSSettingsUI.cpp)。

## 保存资产与宿主接入

当前入口、参数、保存位置及本轮验证结果见 [实施与验证记录](SettingsImplementation.md)。以下契约用于后续维护。

统一编译后，在 Unreal Python 执行 [Tools/build_system_settings_ui.py](../../../Tools/build_system_settings_ui.py)，调用 [套件源脚本](../../../ArtSource/FPSDemo/UI/SystemSettings/Scripts/build_system_settings_ui.py) 和 `FPSSettingsUIBuilder::BuildAssets()`。仅创建缺失的 `/Game/FPSDemo/Art/UI/SystemSettings/WBP_SystemSettings`、`WBP_SystemSettingsRow`；已有布局不覆盖。脚本核对控件类型、绑定 Action、共享主题/字体、默认弹窗状态以及其他 Content 资产和地图哈希，输出 `Saved/FPSSettingsTests/UIAssets/<时间>/Report.json`。

套件制作入口与跨项目依赖见 [资产 README](../../../ArtSource/FPSDemo/UI/SystemSettings/README.md)。MiSans/Rajdhani 与工业灰黑、少量黄强调沿用既有主题；未创建新字体副本。基础布局为 1600×900，选项区滚动；低分辨率文字、下拉框和长说明仍需按目标尺寸实测。

```cpp
SettingsUI = UFPSSettingsUI::Open(this); // 内部 AddToPlayerScreen(100)
if (SettingsUI) SettingsUI->OnClosed.BindUObject(this, &ThisClass::OnSettingsClosed);
// 宿主调用或 Esc 返回：
if (SettingsUI) SettingsUI->RequestBack();
```

Open 加入视口，但不改变输入模式、移动限制或来源菜单。宿主保存来源页，阻断游戏操作并处理释放重按，OnClosed 后恢复来源和输入。正常关闭只执行一次 RemoveFromParent 和 OnClosed；销毁时取消捕获，并回退尚未确认的显示切换。主聊天负责大厅、等待、局内与结算入口；UI 子任务未修改这些宿主。

## 选项与确认流程

五分类依次调用 `Options(0..4)`。Id、Label、Description、Choices、Min/Max/Step/Value 来自后端，Kind 映射到 Slider、Choice、Binding、Action；ValueText 优先显示后端文案，未提供时显示当前数字或选项名。通用行长期保留，0.1 秒刷新更新值；正在拖动的滑条不写回显示值，展开的下拉框不重建选项列表。仅分类或选项 Id 序列变化时重建行，活动拖动/下拉框/捕获期间推迟重建。

画面调用 SetValue 修改后端 Draft；切分类不丢弃。应用调用 ApplyGraphics，显示切换通过 IsConfirmingDisplay / DisplaySecondsLeft 展示倒计时，确认与恢复分别走 ConfirmDisplay / RevertDisplay。其余类别由后端即时生效和保存。试听仅点击 Action 时调用 PreviewSound，滑条不触发试听。

带未应用画面修改返回时显示“应用并返回 / 放弃修改 / 继续设置”。应用进入显示确认后保持设置页；仅用户确认且保存成功后返回。拒绝或超时回退保留设置页，不误触 OnClosed。保存失败保留后端 Feedback，并以错误颜色显示；不会假称已保存。恢复本页默认有二次确认，调用 ResetPage(Page)，遵循该页原有生效规则。

## 捕获输入

点击 Binding 行后 BeginCapture，按键提示和冲突文案读取 CapturePrompt。Escape 固定取消捕获，不同时退出设置；冲突显示替换或取消，分别调用 ConfirmBindingReplacement / CancelCapture。清空调用 ClearCapturedBinding；无可用绑定的动作说明由后端 Options 提供。

NativeOnPreviewKeyDown 和 NativeOnPreviewMouseButtonDown 优先截获待绑定输入；支持键盘、鼠标侧键、鼠标按钮与滚轮。单独 Shift/Ctrl/Alt 在松开时提交，按住修饰键再输入其他键则拒绝组合；不接受手柄或 Escape 绑定。弹窗清空/取消按钮区域保持正常点击，其他鼠标点击作为待绑定输入。已消费输入的松开事件也会吞掉，防止落入下层 UI。输入不触发游戏操作还依赖宿主统一阻断，不能仅凭此 Widget 的事件处理宣称完成。

界面与舒适性页的 HUDPreviewContent 按后端 HUDScale 同步缩放；示意只包含生命、弹药和武器标签，注明非实战截图，不编辑准星。实际 HUD 缩放与镜头晃动由主聊天接入。

## 稳定测试契约

| 控件 / 接口 | 契约 |
| --- | --- |
| `Dispatch(FName)` | 页面/弹窗动作入口，受当前弹窗状态约束；GetPage 返回 0–4 |
| `PageGraphics` / `PageSound` / `PageControls` / `PageBindings` / `PageHUD` | 五分类按钮，Action 同名 |
| `SettingsRows` | ScrollBox，子项为 UFPSSettingsRow；FindOptionRow(Id) 返回对应行，GetOptionId 可核对 |
| `OptionLabel` / `OptionDescription` / `OptionValue` | 每行文本控件 |
| `OptionSlider` / `OptionChoice` / `OptionAction` | 每行 Slider / ComboBoxString / UFPSSettingsButton；可驱动真实 OnValueChanged / OnSelectionChanged / OnClicked |
| `ApplyGraphics` / `DiscardGraphics` / `ResetPage` / `Back` | 页脚操作；RequestBack 与 Back 使用同一路径 |
| `SettingsModal` / `ModalTitle` / `ModalBody` / `ModalFeedback` | 弹窗容器、标题、正文、捕获反馈；ModalShield 阻止点击穿透 |
| `ExitApply` / `ExitDiscard` / `ExitContinue` | 未应用画面退出三选项 |
| `DisplayConfirm` / `DisplayRevert` | 显示切换确认/回退；倒计时位于 ModalBody |
| `ResetConfirm` / `ModalCancel` | 恢复默认确认/取消 |
| `CaptureClear` / `CaptureCancel` / `ConflictReplace` / `ConflictCancel` | 捕获清空/取消，冲突替换/取消 |
| `HUDPreview` / `HUDPreviewContent` / `HUDPreviewCaption` | 仅第 4 页显示；Content 的 RenderScale 跟随后端 HUDScale |
| `SettingsFeedback` / `GraphicsStatus` | 后端结果与保存失败提示 / 是否存在画面草稿 |

鼠标和键盘 Native override 为 public，可用真实 Geometry 与输入事件驱动测试；CaptureKey 不通过 UI 内的专用测试捷径调用。单修饰键测试应包含 KeyDown 与 KeyUp。后端的选项 Id 为权威，UI 不维护第二份动作/参数目录。

## 验证边界

本子任务只静态核对 UI API、脚本语法、资源保护范围和文档；未运行编译、资产脚本或 Unreal。后续由主聊天记录对应构建及报告，独立回载已保存资产，并在 1920×1080、2560×1440、1280×800 验证五分类、滚动、长说明、HUD 示意、鼠标/键盘与各弹窗。重点覆盖滑条中连续刷新、显示超时与应用返回链、捕获取消不退出、单修饰键与组合键、侧键/滚轮、冲突替换和保存失败反馈。

设置持久化、实际渲染/音频/输入/动作生命周期、服务器客户端表现和人工手感均属于主聊天集成验证；不能以本文或 Widget 生成成功替代。

返回 [UI 主题索引](README.md)。

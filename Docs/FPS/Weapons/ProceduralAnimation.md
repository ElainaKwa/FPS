# 轻量竞技 FPS 动作与分件换弹

- 用途：实施与验证记录
- 适用范围：FPS Demo 程序动作、Industrial 七枪第一/第三人称表现
- 状态：分件换弹、参考姿势及手部跟随已通过原工程 Editor 编译、实机采集、三端表现与专用服务器针对性回归
- 最后更新：2026-10-15

用户选择利落、轻量的竞技 FPS 风格，并要求换弹包含重新装入弹匣和拉栓。首轮 `20261015-Reload` 汇总为 `Saved/FPSAnimationTests/ReloadVerification.json`；后续参考姿势、手部调整与原工程验证见文末及 `Saved/FPSAnimationTests/ReloadHandsVerification.json`。此前动作验证记录保存在 `Saved/FPSAnimationTests/Verification.json`，不把它当作新增分件的验证结果。

## 实现

[FPSProceduralAnimation.h](../../../Source/MCP_Test/FPSProceduralAnimation.h) 增加速度驱动步态、起停和转向惯性、落地与蹲起反馈。相机位置、瞄准射线和玩法移动不由这些装饰动作驱动；完整 ADS 收敛到原瞄准位置，系统设置中的 BobScale 控制环境性摆动。

换弹使用现有服务器 `ActionStartedAt / ActionEndsAt` 采样，依次抓匣、拔出、在画面下方更换弹匣、插入压实、拉动机柄或套筒、释放回握。动作没有另设弹药结算时钟，不修改换弹时长、容量或服务器提交规则。左手以部件接触点为目标；检视有两个停留角度，中断动作用短时过渡回位。

[FPSWeaponReloadRig](../../../Source/MCP_Test/FPSWeaponReloadRig.h) 作为原整枪网格的 UserData 硬引用枪身、弹匣和运动部件。七枪共 21 个网格保持原枪坐标；手枪使用套筒，长枪使用现有拉机柄。第一/第三人称显示分件，拾取物沿用完整模型。缺少装配定义的武器回退原整枪。独立弹匣配件若已有模型，则替代内置弹匣并跟随同一运动层级。

源文件和重建入口见 [分件资产说明](../../../ArtSource/FPSDemo/Weapons/IndustrialSet/Reload/README.md)。这是现有 Industrial 库的派生制作，不迁移旧库，也不替换 M4/M7 精细样品。资源依赖递归进入现有内容签名，导入后重新保存七个武器定义、重新导出 [武器资产清单](../../FPSWeaponAssetReview.md)。

## 验证入口和边界

- `Tools/TestFPSAnimation.ps1`：弹簧帧率一致性、动作连续性、取消恢复、零时长、七枪装配读取；真实游戏关卡的运动、ADS、步枪和手枪换弹、第三人称部件与截图。
- `Tools/TestFPSWeapons.ps1 -Presentation -Latency 50 -PacketLoss 1`：服务器、持枪客户端和迟加入观察者，含远端弹匣/拉机柄运动、预测去重、取消与弹药结算。
- `Tools/ImportFPSReloadRig.ps1`：逐部件尺寸与材质绑定、原整枪边界和武器数值保持检查、内容签名刷新。

首轮独立验证结果：

- 冻结源码独立编译 `Reload-Isolated-06` 成功，编译后七枪内容验证通过。记录在 `Saved/FPSAnimationTests/Runtime-20261015-Reload/Saved/FPSMovementTests/Build/Reload-Isolated-06/`。
- 自动化 `MotionAndActions` 与 `ReloadAssets` 共 2 项通过、0 失败。实机采集 `Playback-20261015-Reload/Saved/FPSAnimationTests/Reload-03/` 共 294 帧，步枪弹匣/拉机柄、手枪弹匣/套筒、第三人称部件、ADS 和低摆动选项检查通过。证据路径均相对 `Saved/FPSAnimationTests/`。
- `20261015-Reload-Network-02/` 服务器、持枪客户端及迟加入观察者均通过，网络条件为 50 ms 延迟、1% 丢包。远端部件实际位移、换弹取消、结算次数及检视状态恢复均有日志断言。
- 人工复核截图，调整了换弹抬枪位置以露出弹匣插入口，并延长套筒后拉停顿，使低帧率采集也能看到关键姿势。实机 GIF 为 `Saved/FPSAnimationTests/ReloadPreview.gif`。
- 初次原工程链接因 M7 后台编辑器占用 DLL 而失败；因此保留它的工作，使用独立编译和运行副本验证。另一次自动化启动被构建目标查询阻塞，已终止本任务实例并改用运行副本。早期手枪采集未捕获足够套筒行程，修正停顿后最终检查通过；不将这些早期失败计入最终通过结果。

首轮已更新原工程源码和共享 Content；当时 M7 后台编辑器使用旧模块，因此采用独立副本验证，没有关闭其他工作线的编辑器，也没有把独立副本模块覆盖回原工程。该编辑器后来正常退出，原工程追加构建与实机结果见下文。

首轮核对发现另一工作线同时修改了 `FPSAvatarVisuals.h/.cpp`，用于命中区域绑定、专服关节构建和组件类型调整。这些并行修改均保留；首轮冻结模块不含其新增部分，差异记录在首轮验证汇总中。下文的追加检查已直接使用当前原工程模块，但不代表所有并行功能全部验收。

自动运行与截图检查不能替代真人键鼠手感评价。角色仍使用现有刚性程序关节，未新增手指骨骼动画、弹匣抛落物理或独立阶段音效；原换弹起止音效继续使用。本轮不把 Editor 运行视作已完成打包/Cook 验证。

## 参考姿势与手部跟随（2026-10-15 追加）

根据用户提供的参考图，换弹时向右上移动枪身，并沿枪管轴线向右翻出机匣侧面；装匣期间侧倾约 22–26°，横向转角保持为零，拉栓后逐步回正。没有改变瞄准射线、换弹时长或弹药结算。早期大幅横转、完全取消侧倾的试调版本已被此姿势替代。

左手在弹匣与拉栓接触点之间直接衔接，去掉原先连续两次插值带来的托枪位置绕行。离开抓握点时使用绕枪弧线，抓匣与拉动机柄／套筒期间弧线偏移归零，保持手掌和部件同步。手腕随阶段转动，前臂连接手腕并跟随手掌位置及朝向；右手持续锁定握把。第一人称接触点包含武器模型的局部变换；第三人称及专用服务器继续共用姿态节点与命中体。

原工程最终证据：

- `Saved/FPSMovementTests/Build/Reload-Hands-02/run.json`：Editor 编译通过，编译前后 Source、Config 和工程文件一致。首次手部编译的受保护成员访问错误已修复，使用模型变换和组件查询，不扩大角色成员的访问权限。
- `Saved/FPSAnimationTests/Reload-Hands-01/`：2 项动作／资产自动化通过，实机采集 335 帧。新增实际组件接触检查覆盖步枪和手枪的第一／第三人称左手抓匣、拉栓，以及右手握把固定；原 ADS、取消与分件检查继续通过。
- `Saved/FPSAnimationTests/20261015-Reload-Hands-Network-01/`：50 ms 延迟、1% 丢包下，服务器、客户端和迟加入观察者通过；远端枪身右翻且枪口不横转，分件运动和换弹提交保持正确。
- `Saved/HitDamageTests/20261015-184500564/run.json`：原工程专用服务器及两个客户端，100 ms 延迟下现有部位伤害专项通过。用于回归本次手腕／前臂姿态所带动的命中体，不替代全部动作逐帧碰撞精度验证。

最终模块与源码哈希汇总在 `Saved/FPSAnimationTests/ReloadHandsVerification.json`；人工复核了抓匣、转去拉栓、回握和第三人称截图，预览为 `Saved/FPSAnimationTests/ReloadHandsPreview.gif`。本次直接编译原工程，未覆盖或删除其他工作线的改动。

## 测试产物清理（2026-10-15）

按用户要求，清理本任务临时 Runtime／Playback 工程中的编译输出、缓存、源码与工具副本，以及原始逐帧截图、重复预览和临时脚本，释放约 5.1 GB。临时工程内的 Content／ArtSource 目录链接仅移除链接本身，正式资源未删除；主工程的构建输出、共享缓存及其他任务产物未作为清理目标。

保留历史及最终验证 JSON、自动化报告、运行日志、最终 GIF／拼图和资源备份，并核对保留文件哈希。上文帧数表示采集时的数量，原始 `Frames` 目录已删除；临时工程路径现仅保留证据目录，不再是可运行副本。清理清单见 `Saved/FPSAnimationTests/CleanupReport.json`，后续重跑仍使用主工程 `Tools/TestFPSAnimation.ps1`。

## 2026-10-15 提交前集成复验

本轮完整工作区在原工程重新构建后，`Saved/FPSAnimationTests/ReviewCommit-FixedStep-20261015/run.json` 记录 2 项自动化及 408 帧实际关卡采集通过。ADS／检视取消、步枪与手枪换弹部件、第一／第三人称左手接触和右手固定的原断言全部通过；模块 SHA-256 为 `AE5F2B244FFD72A694DB589DDD307158131019C54FD9A72D3D621A70AC69FA5B`。

首次 `ReviewCommit-20261015` 实时截图约每秒 4 帧，跳过短暂套筒阶段，并让相邻动作与检查在同一帧执行。采集脚本改用引擎固定 60 Hz 模拟步长、保留实际渲染和原断言，允许截图耗时而不跳过模拟采样；未调整动作或玩法来满足测试。此结果用于动作正确性复验，不代表实时渲染性能达标。新采集与历史清理记录分开保存。

返回 [枪械主题索引](README.md)。

# 移动与体力重构实施与验证

- 用途：实施与验证记录
- 适用范围：对战和靶场玩家移动、体力预测、武器衔接、输入设置与 HUD；原教学兼容
- 状态：实现完成，实际双端自动验证与渲染专项通过；人工手感、全部地图越障路线与新包未验证
- 最后更新：2026-10-13

行为依据为 [移动设计决策](MovementDesignDecisions.md)。本轮基线 HEAD 为 `6c98bad89fe9c8aec8a3731261033ece0dadc759`，工作区已有武器清理、UI 和工业风资源增量，均保留；不重做地图，不覆盖此前角色阶段记录，也不以旧运行结果证明新移动规则。

## 实现职责

| 入口 | 本轮职责 |
| --- | --- |
| `FPSMovementSettings` | 集中首轮参数：480/720/240/312、500、弱跳高度 0.5、100 体力、疾跑 6/秒、跳 20、蹲 10、启动门槛 20、恢复 1.5/25、间隔 0.2/0.35/0.8、射击准备 0.15。 |
| `FPSMovementComponent` | 沿用 CharacterMovement 物理、碰撞和预测；区分意图和实际前向地面位移，按成功动作收费，恢复和间隔随有效移动时间推进。AI 不收费、不触发弱跳或低体力蹲间隔。 |
| `FPSDemoAttributeSet` | GAS 权威 Stamina/MaxStamina；服务器发布统一模拟结果，拥有者 HUD 使用独立即时预测资源，属性复制不覆盖更新的预测。 |
| `FPSCharacterInput` / `FPSInputSettings` | 默认按住，分别配置疾跑/蹲伏切换；本地保存，不复制用户偏好，不扩展键位重映射或手柄。 |
| `FPSWeaponComponent` / Abilities | 武器动作优先，真实疾跑转射击有独立准备门槛；半自动有效按下保留、自动松键取消，与实例原冷却及 GAS 就绪条件共同约束。 |
| `FPSCharacterPresentation` / `FPSAvatarVisuals` / HUD | 读取实际状态；初版体力反馈放生命旁，2026-10-13 改为中心下方细白条，见 [当前编辑说明](../UI/AssetUsage.md#体力条)；满体且无相关限制时隐藏，保留教学显示。 |

只有结构化对战/靶场启用新规则；原教学仍使用原 570/650 与原动作路径。新规则的参数初始化在生成后生效，避免角色蓝图旧默认值覆盖 500 的起跳参数。己方基地使用普通体力消耗/恢复，不补满、不加速恢复。

## 预测与服务器裁定

动作触发序号随自定义 packed CMC move 传输，服务器不接受客户端体力值。成功进入蹲伏和成功起跳只在消费新序号后执行一次；失败请求不排队、不收费、不重置恢复。停步和顶墙保留有效疾跑意图，已有意图不重查 20 点；侧移/后退、动作中断和耗尽清意图。

SavedMove 保存输入与模拟结束资源。首版关闭 move 合并，防止位置合并回滚时资源未回滚；仍保留原生移动发送、时间戳校验、碰撞与纠正。资源快照与 CMC 确认边界绑定，纠正被原生 CMC 接受后才恢复一次资源并重放未确认移动。过期快照不能覆盖新预测；多个响应在同一帧抵达时，以最新已确认边界为准。重放不发武器状态通知，也不重复延长射击准备。

启用引擎 movement time discrepancy detection/resolution，服务器有效模拟时间限制消耗、恢复及间隔；没有接收客户端提交的体力数值。当前准入协议为 9（包含并行武器配件改动），移动模拟协议修订为 2，模式摘要包含全部 21 项移动配置；旧协议和不一致配置必须在拥有 Pawn 前拒绝。本轮仅验证移动相关兼容，不据此声明配件系统验收完成。时间差处理沿用引擎机制并经静态核对，未另做伪造时间戳的渗透测试。

## 验证记录

构建和运行证据独立保存于 `Saved/FPSMovementTests/`。测试以真实服务器、拥有者客户端及原生移动/GAS 请求观察；位置、体力初值和碰撞场景为 opt-in 夹具注入，不能称为真人游玩或地图手感验收。

统一验证版本为 `Build/20261013-TenthMovementCoordinated/run.json`：模块 SHA-256 `7f6f35278dea4cf72383ca9518020d5c75c017536a2a456bbd5696d1f31b31dc`。构建前后全部 162 个 Source、Config 和 uproject 输入一致。原生移动快照/纠正、GAS 操作、输入设置及以下主要专项使用同一模块，模块起止哈希一致。

当前验证基于 UE 5.8.1 / CL 56057345 的 Win64 Development Editor `-game` 独立进程，不是新 Cook 包。构建记录包含全部 Source、Config 与 uproject 输入哈希，运行记录包含启动参数、拥有的进程、日志及 DLL 前后哈希。原始证据只保存在本机 `Saved/FPSMovementTests/`；[生成证据索引](Generated/MovementVerification.json) 保存两次构建、十项闭合运行、两项编辑器 UI 资产记录及日志/图像哈希。生成入口为 `Tools/RecordFPSMovementVerification.py`，完整重生成参数写在索引中，禁止手工改结果。

| 专项 | 结果与可观察证据 | 本机原始记录（相对 `Saved/FPSMovementTests/`） |
| --- | --- | --- |
| LAN，0 ms / 0% | 真实 listen server 与独立客户端，25 场完整 COMPLETE；速度、位移计费、纠正/重放、动作边界、武器准备、菜单、真实 GAS 死亡及新生命均通过 | `20261013-005954-716-lag0-loss0/run.json` |
| LAN，100 ms / 1% | 两端配置 PktLag=100/PktLoss=1，25 场完整 COMPLETE；资源参考按原生已处理/已预测移动时间戳对齐，离散动作次数一致 | `20261013-005934-177-lag100-loss1/run.json` |
| 靶场与 AI | 25 场 COMPLETE；零体力的实际 Bot 正常疾跑、正常跳及正常蹲间隔，免体力限制；普通玩家仍受限制 | `20261013-010018-954-training-visual/run.json` |
| HUD 实际渲染 | 1280×720 五图逐张检查：满体隐藏、0 点弱跳与耗尽、0 点耗尽、17 点恢复与疾跑启动受限、73 点普通恢复；位于生命旁且不遮准星，文案无裁切 | 同上记录中的 `Captures/` |
| 输入设置 | 实际已保存菜单控件 OnClicked、两项开关、标签更新、磁盘保存、故意改内存后 ReloadConfig、独立进程读回及实际设置截图均通过；原用户 INI 哈希与恢复后的隔离 INI 一致 | `InputUI/20261013-010455-902/run.json` |
| 安全区边界 | 100 ms / 1% 双端：实际移动 RPC 阻止敌方进入；拥有者预测边界、异常传送驱逐、扫掠通行、攻击边界、返回保护及断开清理通过 | `BaseBoundary-20261013-010011289/run.json` |
| 既有武器/GAS | 更新旧测试中被本轮覆盖的静止疾跑/285 蹲速/疾跑阻塞开火断言；其余实例、爆发、空仓、换弹、旧票据、死亡断言保留并通过 | `AbilitiesRegression-20261013-Current10/run.json`、`WeaponsRegression-20261013-Current10/run.json` |
| 原教学 GAS | 原 25 伤害、35 治疗及重复拒绝、死亡计分、重生 100HP 和双端复制通过 | `TeachingRegression-20261013-Current10/run.json` |
| 原教学移动 | 基础 CharacterMovementComponent，两端实际 walking 570 / JumpZ 650 / 上升高度 215.555 cm；不启用结构化移动与体力，双端 COMPLETE | `20261013-012237-365-teaching/run.json`，第十三编 |
| 移动配置准入 | 客户端仅覆盖 SprintDrain=7，在 Pawn=0 时明确 MISMATCH/REJECT，返回并释放 7 定义/0 句柄，无 gameplay READY | `20261013-012309-644-lag0-loss0-mismatch-movement/run.json`，第十三编 |

正常跳与弱跳实测上升高度约 127.5 / 63.8 cm；660 cm/s 的疾跑提速交叉约 0.22 秒。延迟重放场景的实际疾跑 3.5898 秒对应体力 78.460，符合每秒 6 点；该场观察到 20 次原生重放/纠正和 42 个未确认移动的峰值。这里只为迫使零延迟也出现未确认移动，将客户端帧率短时设为 120、服务器维持 60，并启用引擎原生纠正概率；阶段结束恢复原帧率，不手工调用重放。

GAS 额外回归中，客户端疾跑转射击按下时间 60.613839、准备截止 60.763839、首枪提交 60.763848；服务器截止 60.804889、首枪提交 60.821558。自动松键取消初始待发，半自动有效按下保留，三连发仍恰好三发；切枪、菜单、死亡与回合结束均不能将旧待发带入新枪或新生命。

教学移动与配置不一致准入的最终专测使用第十三编 `Build/20261013-ThirteenthTeachingReady/run.json`，模块 `bd41ec36550b2b904fe97cd60b68c91c019baa9d49d0070696f109b87de06c84`，两项均已通过且模块起止哈希一致。相对第十编只有 `FPSMovementSmoke.cpp`、`FPSAttachmentSmoke.cpp` 和并行 `FPSCustomizationMenu.cpp/.h` 变化；本轮移动、GAS、输入、HUD 生产文件哈希相同。其余通过项目保留第十编证据，没有将后续测试夹具与改装菜单的补编伪称为全部项目重复运行。教学 BP 资产保持原样。

UI 资产由 `FPSUIAssetBuilder::ExtendMovementAssets` 及 `Tools/extend_fps_movement_ui.py` 经实际编辑器增量处理。保存和独立读回记录分别为 `UIAssets/20261012-233421-371962/Report.json`、`UIAssets/20261012-233606-126100/Report.json`：五个原菜单/HUD BP 增量保存，506 个原控件及父级/槽身份保留，41 个非目标 UI 资产字节不变；最终运行读取这些已保存资产。本轮没有文本补丁处理 uasset/umap。

## 体力条简化（2026-10-13）

用户要求将体力显示改为简单细白条，追加指定“位置比生命条略高”，随后要求再加粗、再上移。本阶段布局为 1600×900 基准中的 `(700,800)`、`200×5`，横向居中，比生命条上沿高 37；最新追加调整见本节末尾；编辑说明见 [体力条](../UI/AssetUsage.md#体力条)。替换原数值/状态面板，白色填充比例仍读取实际体力，显示时机和移动规则未改。旧的左下截图及本节之前的移动验收属于原版本记录。

仅修改 `FPSUIAssetBuilder.cpp` 的增量样式、`FPSInterface.cpp` 的白色填充刷新，以及 `FPSDemoHUD.cpp` 的一致后备绘制。通过 Unreal 编辑器 API 保存现有 `WBP_FPSHUD`，原控件树保留，旧标题/数值/状态/背景设为 Collapsed。资产记录为 `Saved/FPSMovementTests/UIAssets/20261013-013854-228425/Report.json`：仅 HUD 文件变化，四个菜单与 41 个其他 UI 资产字节不变。

下方位置构建 `Build/20261013-MinimalStaminaUI-Lower/Build.log` 的 UBT 编译、链接及既有武器数据构建检查成功，使用模块 SHA-256 为 `ba65020b11334cd1b86f9d82dfddaa07c1dbce10de51ec698c88b58424866319`。构建包装器因并行 Bot 导航源文件变化将整体记录标为 FAIL；已逐一核对本次三个 UI 生产文件与编译输入哈希一致。后续 `LowerStable` 重编受其他编辑器的 Live Coding 阻止，没有停止该编辑器或改动 Bot 工作。故不声明整个工作区通过了新的冻结输入构建。

前版 `(700,815)`、`200×3` 的靶场实机检查已通过：`Saved/FPSMovementTests/MinimalStaminaUI-Lower/20261013-013916-825-training-visual/run.json`，25 个场景及五张截图完整；满体隐藏、耗尽空槽、低体力与普通恢复白色填充均人工核对。最初靠近准星的预览按用户追加要求中止，保留其失败记录，不作为最终验证证据。

最终加粗上移版通过 `Tools/update_fps_stamina_layout.py` 保存，记录为 `Saved/FPSMovementTests/StaminaLayout/20261013-014652/Report.json`；该脚本只编辑原有两个槽位，编译蓝图、保存并核对其他 45 个 UI 资产不变。第一次脚本尝试因 WidgetTree 属性未暴露给 Python 而失败，改为通过编辑器对象查找后保存成功。生成器和 Canvas 后备源文件同步为 `(700,800)`、`200×5`，但本轮 C++ 重编因正在使用的热重载 DLL 无法清理而受阻，因此最终尺寸的 Canvas 后备路径未做运行验证；正常运行使用已保存的 Widget Blueprint。

最终资产在独立靶场进程中重新加载，`Saved/FPSMovementTests/MinimalStaminaUI-Thicker/20261013-014739-429-training-visual/run.json` 为 PASS，25 个场景完成并生成五张新截图。实际模块由启动日志确认是 `UnrealEditor-MCP_Test-7314.dll`，SHA-256 为 `32c9432b15974fbcc40146f34613c203abbb34805c119dc7dafdc169e6ff0b3c`，运行前后哈希一致；补充记录为 `Saved/FPSMovementTests/MinimalStaminaUI-ThickerActualModule.json`。原测试脚本的 `editor_module` 字段记录基础 DLL，因此该字段不能替代此处的实际加载模块证据。

人工核对 [最终恢复状态截图](../UI/Images/StaminaBar.png)：1280×720 中空槽总宽 160 像素、粗 4 像素、Y=640，居中且高于生命条，显示约 73% 白色填充，无原数值面板。文档检查通过。本轮未重新打包或进行新的联机专项。

2026-10-13 随后按用户明确尺寸再次调整：粗细改为 15，Y 从 800 上移 100 到 700，宽度仍为 200。已保存 `WBP_FPSHUD`，记录为 `Saved/FPSMovementTests/StaminaLayout/20261013-015359/Report.json`；独立进程回载核对位置、尺寸、白色及旧面板隐藏，证据为 `Saved/FPSMovementTests/Stamina15-Readback2.log`（修正 Python 颜色属性读取后通过）。同步更新生成器和后备绘制源文件，本次数值布局调整未重编 C++ 或重跑完整移动专项；此前实机截图保留为历史证据，不冒充新尺寸截图。

2026-10-13 再按反馈改为 `240×12`：长度增加 20%，粗细从 15 减至 12，Y=700 保持不变，X 从 700 改为 680 维持横向居中。保存记录为 `Saved/FPSMovementTests/StaminaLayout/20261013-020202/Report.json`，独立回载证据为 `Saved/FPSMovementTests/Stamina12-Readback.log`，核对尺寸、位置、白色及旧面板隐藏。布局生成器与后备绘制源文件同步；本次数值布局调整没有重编 C++。

随后补拍 [当前布局实机截图](../UI/Images/StaminaBar12.png)，人工核对恢复中的白色填充、长度、粗细及位置。首次预览在弱跳高度断言失败后中断，记录保留于 `Saved/FPSMovementTests/Stamina12Preview/20261013-020428-079-training-visual/run.json`；随后单独运行截图夹具完成采集，并关闭该次预览进程。截图日志及范围说明为 `Saved/FPSMovementTests/Stamina12Preview/VisualOnly/Preview.log`、同目录 `Scope.txt`。本次是视觉采集，不代表完整移动回归通过。

2026-10-13 继续按反馈改为 `255×9`，位置 `(672.5,700)`，保持横向居中。保存记录为 `Saved/FPSMovementTests/StaminaLayout/20261013-021355/Report.json`，其他 45 个 UI 资产哈希不变；独立回载 `Saved/FPSMovementTests/Stamina9-Readback.log` 通过。已人工核对 [255×9 最新实机截图](../UI/Images/StaminaBar9.png)，采集日志为 `Saved/FPSMovementTests/Stamina9Preview/VisualOnly/Preview.log`。生成器与后备绘制源文件同步，本次没有重编 C++，验证覆盖已保存的 HUD 资产和视觉效果，不声明完整移动回归通过。

### 下移与双向延迟过渡（2026-10-13）

保持 `255×9`，由 Y=700 下移 40 至 Y=740，X=672.5。保存的 `WBP_FPSHUD` 新增淡灰色 `StaminaDelayBar`，其上叠加透明背景的白色 `StaminaBar`。`FFPSStaminaPresentation` 供 Widget 与 Canvas 后备路径共同使用：方向变化延迟 0.16 秒，快层和延迟层分别按 0.055 / 0.20 秒时间常数跟随；消耗时灰色拖尾，恢复时白色追赶。满体等待收敛才隐藏，换角色、死亡或隐藏 HUD 重置显示状态。未修改移动、GAS 或网络同步数值。

- 完整编译通过：`Saved/FPSMovementTests/Build/20261013-StaminaSmooth-Guid/run.json`。首次中间构建期间继续补充了后备显示和专项测试，冻结输入检查失败，后续完整构建已通过。
- 自动化 `FPS.UI.StaminaSmoothing` 通过，日志 `Saved/FPSMovementTests/StaminaSmooth-Tests.log`：首次显示、下降延迟、上升延迟、方向反转、持续消耗、满体收敛、长帧、30/120 FPS 一致性及重置。此项验证显示状态算法，不替代网络回归。
- 资产保存记录：`Saved/FPSMovementTests/UIAssets/20261013-030453-938952/Report.json`，仅 HUD 改变。首次保存触发新增 Widget GUID ensure，已在生成器补充 `OnVariableAdded`；修复后重新运行无 ensure、退出码 0，记录 `Saved/FPSMovementTests/UIAssets/20261013-030629-095712/Report.json`，重复执行保持资产不变。
- 独立回载通过：`Saved/FPSMovementTests/StaminaSmooth-Readback.log`，核对位置、尺寸、白/灰图层顺序和前景透明背景。已人工核对 [下移及延迟过渡实机截图](../UI/Images/StaminaBarSmooth.png)，恢复中白条后方可见灰色过渡段；消耗中的截图另存于 `Saved/FPSMovementTests/StaminaSmoothPreview/VisualOnly/Drain.png`。采集日志为同目录 `Preview.log`，不声明完整移动专项重新通过。

## 失败记录与修复边界

所有失败样本保留在 Saved，后续通过记录没有覆盖它们。早期 C++ 错误已修复：生命周期复制宏参数名、UHT 构造时不完整类型的 unique pointer、夹具读取不存在的装备字段。生成武器清单过期时通过权威命令let `-Export` 重新导出，没有手写生成资料或扩大文档豁免。

网络参考曾错误地按采集时的世界时间外推服务器资源。100 ms / 1% 场景中，原世界年龄 0.654 秒忽略未确认移动；改为原生模拟时间年龄 0.9084 秒后，客户端与权威外推体力均为 89.648。保留 2.5 点容差、每秒 6 点检查及离散次数检查，没有调大阈值。射击检查同样改为枪实例 `LastShotAt`：权威首枪 174.4463，而观察到复制时 GS 估计时间为 174.4109；旧观察时钟会误报提前射击，新的提交时刻检查通过。

UI 夹具最初把 UE 5.8 的配置 known key `GameUserSettings` 当作物理文件路径，已使用引擎解析后的实际路径验证；保存逻辑仍使用原 known key。截图最初以 ShowUI=false 隐藏 UMG，已改为实际 UI 渲染并重新采集五种状态。原用户配置在各次专项均未变化。

教学 BP 保留基础 `CharacterMovementComponent`，早期夹具强制转换为新组件导致 Fatal；已改为基础组件与可选新组件检查。第二次夹具启动过早，客户端尚未进入稳定物理状态时检查参数失败，但后续实际行走已为 570。最终增加最多 10 秒的真实落地前提，输出类、角色、MovementMode 和全部实际参数；客户端比服务器晚 0.654 秒物理就绪，随后原参数及跳高断言均通过。没有等参数等于期望后才执行断言。

并行构建期间，源码变更、DLL 占用或模块更新导致的构建/运行守卫失败均不作为最终验收版本。用户明确授权后协调稳定窗口，才取得上述统一版本的闭合结果。启动时对自有子进程使用 `UE_SKIP_UBT_SDK_SETUP=1` 跳过编辑器开发 SDK 探测，并立即恢复启动器环境；不修改用户全局环境。每个工具只关闭自己启动的进程；日志 COMPLETE 后退出码 -1 是工具有意终止自有进程，脚本本身成功退出为 0。

## 提交前复核（2026-10-13）

当前工作区通过 Editor Development 冻结输入构建，记录为 `Saved/FPSMovementTests/Build/CommitReview-20261013-Unsandboxed/run.json`；实际加载 `UnrealEditor-MCP_Test.dll` 的 SHA-256 为 `3032f991f15bea9a2ec97099d2e6ca2ef31d355d1b2c18ed1f7cb12450f53ba0`。Game Development 构建成功，日志为 `Saved/CommitReview/GameBuild.log`。

`Saved/CommitReview/Movement/20261013-033436-628-lag100-loss1/run.json` 为 PASS：真实监听服务器和独立拥有者客户端在两端设置 PktLag=100／PktLoss=1，25 个场景均 COMPLETE，涵盖速度、体力计费、原生纠正／重放、跳蹲边界、疾跑转射击、菜单、死亡与回合结束。模块起止哈希一致；碰撞场景和初始资源仍由 opt-in 夹具注入，没有保存地图。此轮未采集移动视觉图、重做原教学专项、人工手感或 Cook／打包，不替代上述历史验收的独立范围。

## 使用与维护入口

Shift 疾跑、Ctrl 蹲伏、Space 跳跃沿用现有键位。菜单的输入设置分别切换疾跑与蹲伏的“按住 / 切换”，默认按住；偏好写入本机 GameUserSettings 的 `/Script/MCP_Test.FPSInputSettings`，下一进程读回。菜单、死亡或回合结束清除旧意图，关闭菜单后需要重新触发。

首轮移动参数由 `UFPSMovementSettings` 管理，可在项目设置 FPS Movement 中查看；Config 覆盖节为 `/Script/MCP_Test.FPSMovementSettings`。参数及移动协议修订进入本局准入摘要，服务器与客户端使用不同配置时必须在拥有 Pawn 前拒绝，不能只调整客户端 HUD 或速度。仍需依据真人试玩决定未来调参，不将自动数值检查当作最终平衡确认。

用户在 2026-10-13 要求后续单独制作完整设置界面，已列入 [后续想法](FutureIdeas.md)。本轮交付仍是两个输入方式开关。

## 未验证边界

真人手感、逐键硬件输入、手柄、外部网络、完整新包 Cook、独立 Server Target、性能/带宽压力和正常/弱跳在现有地图的全部越障路线尚未验证；弱跳落地后走出自然边缘的具体体验也未单独实测。Canvas 缺资产回退路径已实现、编译，未故意删除控件制造降级。逻辑注入、数值轨迹和截图各有用途，不能单独等同于全面体验验收。此前角色包/压测不能证明本轮新移动系统的这些项目。本轮不新增无缝切图或未来机动玩法。

返回 [角色主题](README.md)。

# 手动改装与弹匣事务实施记录

- 用途：实施与验证记录
- 适用范围：配件工作线 A3，训练改枪界面、GAS 动作及实例装配复制
- 状态：改装与生命周期双端、实际拖放取消及迟加入专项通过；完整阶段回归待验收
- 最后更新：2026-10-13

依据：[设计决策](AttachmentDesign.md)、[任务清单](AttachmentTasks.md)。这是未提交工作区上的阶段记录，与其他移动及 UI 工作并存。构建、测试和未验证范围分开记录，不以旧架构日志代替新增功能验收。

## 实现边界

`FPSWeaponCustomization` 只传操作 ID、实例 ID、配置版本、请求序号、配件 ID／类别及取消目标，不接受客户端数值、弹药或截止时间。`UFPSModifyAttachmentAbility`、`UFPSModifyMagazineAbility` 和 `UFPSCancelReloadAbility` 沿用 GAS 本地预测与服务器裁定；组件提供经校验的事务接口。普通配件即时提交，弹匣先进入私有挂起状态，到期时重新解析并按当前枪内加备弹总数原子提交。取消只丢弃挂起装配。

动作票据增加配置版本；过时请求、取消目标及定时回调不能覆盖后续动作。武器 GUID、射击冷却和后坐力索引在手动改装中保留。普通换弹也携带关联操作 ID，以便界面精确取消当前换弹。已提交配件及配置版本加入持有者、旁观者和拾取物状态；预测射击快照不会跨配置版本合并。

有效容量、换弹时间、ADS 时间与 FOV、瞄准位置、后坐力倍率和可选开火声音统一取自装配解析结果；服务器单次射击上下文仍捕获提交时数据。内容协议版本提升为 9，避免旧客户端按旧解释使用相同定义。

训练模式使用 `B` 打开 Slate 改枪界面：普通配件点击应用，弹匣拖入武器区，换弹中的武器按钮用于取消。打开时停止开火、ADS 和检视，保留换弹；关闭不恢复旧输入、不取消换弹。界面提供当前已提交属性与悬停候选对比。首批配件仍显式沿用基础枪外观，不能将新增可选模型挂接代码当作独立配件美术验收。

2026-10-13 用户将正式改枪界面明确留给独立后续工作；此处 Slate 仅为功能验证入口。本阶段停止扩展正式 UI 设计，相关待办见任务清单。

## 当前证据

引擎为本机 UE 5.8.1。网络测试使用训练地图 `/Game/FPSDemo/Maps/LV_FPSTraining` 的训练规则集，经 `Mode=LAN` 测试入口启动 listen server 和独立客户端，端口 17801。正常 `Training` 入口仍保持原单机限制。测试只在带专用启动标记的进程中修改 M4 换弹时间和原装弹匣容量，未将夹具数值保存到资产。

| 证据 | 条件及结果 |
| --- | --- |
| `Saved/Logs/AttachmentA3-Build6.log`、`AttachmentA3-Build6-Console.log` | Editor Development 构建成功，包含真实 GAS 绕过本地标志的拒绝测试；后续界面及截图时序修正不包含在此构建结论中 |
| `Saved/FPSAttachmentTests/A3-NetworkInitial-20261013/` | 首次客户端被正常单机入口拒绝连接；不算功能通过，改用 LAN 测试入口 |
| `Saved/FPSAttachmentTests/A3-NetworkLAN-20261013/` | 客户端检查通过；服务器一条断言误用 `TryActivateAbilityByClass` 的远端转发返回值，整轮不算通过；后改为激活条件检查并增加真正客户端越权 RPC 测试 |
| `Saved/FPSAttachmentTests/A3-NetworkVisual-20261013/` | 真实渲染客户端与服务器均通过预测改装拒绝、开火／ADS 越权拒绝、有效 FOV 和后坐力消费等检查；截图读回卡顿使定时取消晚于到期提交，导致弹药预期链失败，整轮未通过 |
| `Saved/Automation/WeaponPIEExit-AttachmentA3-20261013/Editor.log` | 真正编辑器 PIE：单机和双端的换弹、检视、挂起弹匣改装共六种退出场景均有 armed 和 PASS，Automation Result Success，编辑器正常退出码 0 |
| `Saved/FPSAttachmentTests/A3-DragLifecycle-20261013/` | 切枪、丢弃拾取、死亡清理单项通过，但 Slate 夹具错误使用合成移动事件，引擎跳过拖拽检测；死亡前客户端断言也晚于服务器击杀复制，整轮未通过 |
| `Saved/Logs/AttachmentA3-Build8.log`、`AttachmentA3-Build8-Console.log` | Editor Development 构建成功，包含拖拽事件标记和测试时序修正 |
| `Saved/FPSAttachmentTests/A3-DragFixed-20261013/` | listen server＋D3D12 独立客户端，未注入延迟／丢包；两端全部断言 PASS COMPLETE，测试脚本退出码 0。真实 Slate 鼠标按下、拖拽检测、武器区落点和点击取消均通过；切枪、丢弃拾取和 GAS 致死后的事务清理通过 |
| `Saved/FPSAttachmentTests/A3-LateObserver-20261013/` | 原双端流程通过，但第三进程冷启动晚于死亡重生，错过已提交装配观察窗口；客户端夹具还错误驱动了新生角色。整轮未通过，已修正夹具等待和驱动身份，待复跑 |

`A3-DragFixed-Module.json` 记录本轮模块 SHA-256：`11ec2eae896dfa71b2ceef9ed3bf2f68dec856d3540686df9e5ad3a5eb6282e2`，测试结束后重新核对未变。该目录 `Menu.png` 已人工查看，背景及文字可读，三类配件与武器操作入口完整；这是最小功能验证界面，不是正式 UI 美术验收。

首张改枪界面截图位于上述 Visual 目录的 `Menu.png`：发现背景过透明及字小，已在 DragFixed 轮次修正并复查。截图移出精确取消时序窗口，避免 GPU 读回改变待验证操作到达服务器的时间。保留失败日志，不修改预期弹药数量来掩盖取消未及时到达的事实。

## 提交前复核（2026-10-13）

基线仍为 `6c98bad`，复核当前未提交增量。Editor Development 冻结输入构建成功，记录为 `Saved/FPSMovementTests/Build/CommitReview-20261013-Unsandboxed/run.json`；实际加载模块为 `UnrealEditor-MCP_Test.dll`，SHA-256 `3032f991f15bea9a2ec97099d2e6ca2ef31d355d1b2c18ed1f7cb12450f53ba0`。Game Development 构建也成功，日志为 `Saved/CommitReview/GameBuild.log`，不代表 Cook／打包通过。

- `Saved/FPSAttachmentTests/CommitReview-20261013/`：listen server、D3D12 拥有者客户端和迟加入观察者均 PASS COMPLETE；无延迟／丢包注入。覆盖真实 Slate 拖放与取消、弹药守恒、过时请求、预测拒绝、开火／ADS 越权拒绝、切枪、丢弃／拾取和 GAS 死亡清理。模块前后哈希一致，`Menu.png` 已人工查看，文字与操作入口可读。
- `Saved/FPSAttachmentTests/CommitReview-Data-20261013/Data.log`：当前资产重新加载及 22 项解析／内容自测 PASS COMPLETE，未保存测试注入。
- `Saved/Automation/WeaponPIEExit-CommitReview-20261013/Editor.log`：真正单机和双端 PIE 中，换弹、检视、挂起改装共六种退出场景均 armed 后 PASS，Automation Result Success、退出码 0。

上述迟加入复跑闭合了历史 `A3-LateObserver` 的待验证项，未覆盖实际距离裁剪引起的相关性恢复、完整原武器回归或 A4–A6；历史失败记录保留。

## 尚待验收

- 实际距离裁剪导致的相关性丢失／恢复与快照重建。
- 原开火模式、普通换弹、表现和伤害的当前版本回归；延迟／丢包条件另记。
- 独立配件模型、安装位置和音效美术接入属于 A5；本阶段不能据基础外观判定通过。

A4 预设、A5 补给站、A6 Cook／整体回归仍按任务清单执行。已接收弹药箱模型并完成只读资源核对，尚未连接补给玩法，见 [A2 交付资源核对](AttachmentAuthoring.md)。

返回 [枪械索引](README.md)。

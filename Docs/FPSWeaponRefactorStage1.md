# 枪械重构第一阶段实施记录

日期：2026-09-29。范围仅为第一阶段；设计决策仍以 [FPSWeaponRefactorDecisions.md](FPSWeaponRefactorDecisions.md) 为准。

## 接手核对与迁移映射

接手时 Git 工作区干净；Unreal MCP 可用，编辑器没有运行 PIE。原模块为 9288。
定义和曲线由 Catalog 运行时创建；地图数字标记转枚举；模型和开火声使用型号拼接路径。新模式出生装备为 M4/M9，玩家与人机均使用该装备。

| 原标记 | 稳定 WeaponId | 定义 | 类别 / 独立槽位 | 弹匣 / 备弹 | 伤害 / 发间隔 / 换弹 |
| --- | --- | --- | --- | --- | --- |
| FPSWeapon1 | Weapon.AK | DA_AK | Rifle / Primary | 30 / 90 | 34 / 0.11 / 2.4 |
| FPSWeapon2 | Weapon.M4 | DA_M4 | Rifle / Primary | 30 / 120 | 25 / 0.085 / 2.1 |
| FPSWeapon3 | Weapon.MP5 | DA_MP5 | SMG / Primary | 30 / 150 | 20 / 0.065 / 1.8 |
| FPSWeapon4 | Weapon.UMP45 | DA_UMP45 | SMG / Primary | 25 / 125 | 29 / 0.105 / 2.2 |
| FPSWeapon5 | Weapon.M9 | DA_M9 | Pistol / Secondary | 15 / 60 | 25 / 0.2 / 1.4 |
| FPSWeapon6 | Weapon.HeavyPistol | DA_HeavyPistol | Pistol / Secondary | 7 / 35 | 55 / 0.38 / 1.9 |

定义路径：`/Game/FPSDemo/Weapons/Definitions`。独立 CurveVector 路径：`/Game/FPSDemo/Weapons/Curves/CR_<原型号>`。
上表和一次性迁移器保留历史映射；运行时已删除型号枚举和数字强转，不依赖文件名推断身份。

Foundry 原有 12 个台位、Training 原有 6 个台位已原位迁为 `AFPSWeaponStand`：保存 Definition 硬引用及独立 DisplayOrder。位置、旋转、标签以外的场景内容保留；原台位显示标签可仍叫 FPSWeapon1，这只是编辑器标签，不参与选枪。不再在缺标记时自动生成六枪台位。地图制作脚本同步更新，但本次没有重建地图。

## 已实现的契约

- `GetPrimaryAssetId()` 返回 `WeaponDefinition:<WeaponId>`。实例保存 WeaponId、GUID 和各自弹药；切换、丢弃、拾取不根据展示顺序解析型号。
- 已保存定义是唯一参数编辑源，按 Identity、Firing、Ammunition、Operations、Recoil、Presentation、Audio 分类。槽位不再由类别推导。
- 六枪初始装填量明确保存。旧数值保留；检视 2 秒、准备与视觉过渡 0.35 秒、开镜过渡 0.2 秒是本阶段迁移初值。
- 每枪独立曲线保留原 0–7 发增量；后续键延伸原第 7 发末值，覆盖满弹匣；执行采样显式夹到曲线端点。无后坐力必须显式勾选 bNoRecoil 并清空曲线。
- 第一、第三人称及拾取各自保存 Mesh 和 Transform。原拾取 0.65 倍写入 Pickup.Transform；第三人称基础位置、腰射、瞄准和枪口位置从定义读取。
- 定义直接引用开火、换弹开始/结束、切枪、检视、空仓音效。音频服务只保留非武器的 Hit/Kill 反馈目录。开火 Multicast 携带明确声音对象，不拼型号名。
- Asset Manager 扫描 /Game 内 UFPSWeaponDefinition，AlwaysCook；定义硬引用曲线、网格、声音，打包依赖可被追踪。Catalog 按类发现并同步加载当前目录，校验后才提供实例。现有两图使用全部六枪，因此本阶段将目录作为本局必需内容整体校验。
- DefaultGame.ini 用 StartingPrimary/StartingSecondary 稳定 ID 显式配置出生装备；未找到或槽位不符会报错。
- 编辑器 IsDataValid、构建 Commandlet、运行时 Catalog 共用数值/资源校验。重复 ID、缺失资源、非法数值、错误出生槽位、曲线覆盖不足、Asset Manager 注册不匹配均失败。
- MatchMode 在生成角色、武器台拾取物或启动对局前检查数据与台位。错误写入复制的 WeaponDataError，HUD 显示原因；服务器拒绝加入无效对局，客户端自身无效目录也禁止操作并显示错误。
- [FPSWeaponAssetReview.md](FPSWeaponAssetReview.md) 是单向自动生成的审查输出，包含全部定义字段、曲线键、出生 ID、台位定义和位置。禁止手工修改或导入为另一数据源。

## 操作与构建入口

```powershell
# 在 Unreal 编辑器修改并保存定义、曲线或台位后，导出审查清单
./Tools/ValidateFPSWeaponData.ps1 -Export

# 严格校验，不改资产和审查输出；包含负例自测
./Tools/ValidateFPSWeaponData.ps1 -SelfTest

# C++ + 内容校验；Editor Target 的 PostBuildSteps 也会执行同一个内容门禁
./Tools/BuildFPS.ps1

# 网络与资源回归；Visual 仅让客户端渲染，服务器保持 NullRHI
./Tools/TestFPSWeapons.ps1 -Audio -Visual
./Tools/TestFPSWeaponInvalid.ps1
./Tools/TestFPSWeaponIdentity.ps1
./Tools/TestFPSMaps.ps1
./Tools/TestFPSFlow.ps1 -Mode LAN
```

`-Migrate -Export` 是一次性历史导入入口：保留已有定义，遇到冲突曲线不覆盖；不能拿它重新同步或覆盖已编辑资产。首次创建资产或 schema 改动使构建后门禁失败时，编译产物仍可用于运行导出/迁移，再重新运行严格检查。无效内容不会因为使用 -Export 而被接受。

校验命令行单独禁用 MCP 自动监听，不修改现有编辑器设置；否则已打开的编辑器占用 8000 端口会使 UE Commandlet 返回失败。

纯资产修改可能被 UBT 视作 C++ 已是最新而跳过 PostBuildSteps，因此 CI/项目验收统一使用 BuildFPS.ps1 或显式调用 ValidateFPSWeaponData.ps1，不能以一次无编译动作的 UBT 返回成功代替内容检查。

稳定 ID 替换/删除流程：先枚举出生配置、地图 Definition 引用、测试与审查清单，明确旧 ID → 新 ID；通过编辑器迁移引用并保存，更新出生 ID，导出审查清单，运行严格校验和双端回归。局内实例不热迁移；客户端和服务器必须同一内容版本。资产改名/移动使用编辑器重命名工具并修复引用/Redirector，不修改 WeaponId。

## 第二阶段必须退出的适配

此阶段**没有完成 GameplayAbility 调度与 GAS 预测迁移**。现有组件状态机、RPC、bAutomatic 两态和 AI FireAuthority 暂时保留；第二阶段按既定规则统一迁入 Ability，玩家和 AI 共用执行路径，不能让旧调度与新 Ability 并行工作。

后坐力序号/射击冷却仍是原组件生命周期；第二阶段迁入实例并加入恢复等待时间。瞄准与换弹共存、切枪后重新按右键、精确准备/视觉计时、预测纠正和分接收方复制也属于第二阶段，本阶段不以资源迁移测试宣称这些语义已通过。不加入未消费的连发参数或未来功能占位字段。

## 验证记录

环境：Windows、UE 5.8.1（CL 56057345），Development Editor。最终模块为 **9294**，Binaries/Win64/UnrealEditor.modules 已指向该模块。六枪/地图/LAN 主回归使用 9293；9294 只补充校验负例、错误信息精简和构建门禁，已对这些变化再次验证。

| 检查 | 实际结果与证据 |
| --- | --- |
| 最终编译 + 构建后内容门禁 | PASS；Saved/Logs/WeaponBuild9294.txt，FPSWeaponBuildValidation.log；6 个定义，0 error / 0 warning |
| 严格内容检查与清单一致性 | PASS；未使用 -Export 的检查确认清单与保存资产一致；FPSWeaponDataValidation.log |
| 负例与编辑器验证 | PASS；非法容量/间隔、缺失声音/曲线、曲线覆盖不足、重复 ID 被拒绝；IsDataValid 的非法间隔与重复 ID 路径也通过负例测试；显式无后坐力有效，曲线越界夹末值 |
| 构建检查失败路径 | PASS；进程级配置 StartingPrimary=Weapon.Missing，内容门禁返回 **1**，明确报告缺失 ID；FPSWeaponInvalidBuild.log；没有修改项目配置或资产 |
| 无效数据实际开局 | PASS；9294 注入 FireInterval=-1，仅影响测试内存；pawns=0、pickups=0、combat=0，HUD 显示定义路径、字段和值；WeaponInvalid.log、WeaponDataBlocked.png |
| 真实资产改名/移动/重排 | PASS；独立测试资产和地图经过保存、重命名、移动、显示名/DisplayOrder 修改、地图重载；PrimaryAssetId、Asset Manager 对象解析、台位引用保持正确；FPSWeaponIdentity.log；测试资产已清理 |
| 六枪真实双端回归 | PASS；监听服务器 + 独立客户端，双方 COMPLETE；六枪依次从定义创建、客户端请求开火、服务器扣弹、拥有者复制纠正，第一/第三人称网格引用检查通过；Weapons-server.log、Weapons-client.log |
| 实例与既有操作回归 | PASS；换弹中断不增弹、部分换弹、瞄准/检视、独立弹药、GUID 丢拾保留、距离拒绝、重复拾取拒绝、菜单限制、疾跑/下蹲；同上日志。只验证既有语义，不代表第二阶段最终操作规则 |
| 模型/HUD/音效 | 已检查六枪实际客户端截图 Weapon.*.png；HUD 型号/弹药一致。六枪开火的 LOCAL/SPATIAL 声音组件均 active=1，操作声播放正常；录制输出 Audio-server.wav / Audio-client.wav 经 check_fps_audio.py 确认非静音、未削波；未做主观听感平衡验收 |
| Foundry / Training 地图 | PASS；Foundry starts=10 / pickups=12 / pawns=10；Training starts=4 / pickups=6 / pawns=4；clear=1；Map-Solo.log、Map-Training.log；已查看地图截图 |
| GAS 与对局基础 | PASS；TestFPSFlow -Mode LAN：服务器伤害/友伤拒绝/得分，客户端生命、死亡信息、重生与最终结算同步；LAN-server.log、LAN-client.log |
| 原教学地图 | 最终模块启动 LV_NetSyncFPS 成功，GameMode 仍是 FPSDemoGameMode，实际生成 BP_FPS_Character_C；Teaching-stage1.log。原地图与教学蓝图无 Git 改动；未重跑该旧教学路径完整双端射击/治疗交互 |
| 静态检查 | git diff --check 通过；迁移/身份 Python 脚本语法通过，PowerShell 测试与构建入口语法通过；无临时测试资产残留 |

日志根目录为 Saved/Logs、Saved/FPSFlowTests；截图为 Saved/Screenshots/WindowsEditor。它们是本地验证产物，不纳入版本控制。

验证过程修复了 MCP 端口冲突导致 Commandlet 非零返回、截图帧阻塞导致固定时刻断言落到下一把枪、固定等待时间提前终止测试，以及 UE Commandlet 删除测试包后残留文件的问题。最终测试改用客户端单独渲染、等待完成标志；临时目录清理先走编辑器资产删除，再核对注册表与本测试的明确文件路径。身份测试中重命名产生短暂的重复 PrimaryAssetId/目录清理 warning，但最终对象解析、重载与后续正式内容校验通过，无正式重复定义残留。

`-game` 日志仍包含启用的 UE Toolset Python 插件找不到 editor-only 类型（例如 ToolsetDefinition）的启动报错；上述游戏检查正常完成。这不属于枪械重构，本轮未修改插件。

**未验证/未实施：** 编辑器当前窗口仍加载旧模块 9288，需重启编辑器后再编辑新 schema/资产；本次运行证据来自独立游戏进程，未做该窗口内手动双窗口 PIE。未做 Cook/打包运行、高延迟/丢包、十真实客户端、完整五分钟自然结束与性能验收。Ability 核心、GAS 动作预测、实例运行限制、难度和三连发新枪均留在第二/三阶段。

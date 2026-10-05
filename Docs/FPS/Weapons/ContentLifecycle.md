# R3＋R4：本局内容契约与加载生命周期

- 用途：实施与验证记录
- 适用范围：FPS 武器依赖、两端内容核对、分级校验和资源加载
- 状态：R3＋R4 实现与针对性验收完成；Cook/打包、多客户端压力未覆盖
- 最后更新：2026-10-04

依据：[纠正版评审](../../FPSWeaponArchitectureReview.md)、[最终决策](../../FPSWeaponRefactorDecisions.md)、[阶段清单](ArchitectureTasks.md)。本阶段保留 R1/R2/R5 和现有玩法，不把旧静态风险写成已复现故障。

## 实施边界

定义内网格、曲线和声音，以及地图武器台的定义引用改为软引用。运行时按起始主副武器和本地图武器台明确引用计算依赖集；教学地图只依赖其展示使用的起始主武器；无关世界不加载武器定义。Asset Manager 解析稳定 PrimaryAssetId，其 StreamableManager 为本局定义和资源分别持有加载句柄，开局前同步就绪，世界清理和 GameInstance 结束释放。同步加载发生在准入阶段，不在开火/换弹中临时补载；没有宣称异步无卡顿或按角色拆分所有表现资源。

全项目编辑器/发布检查继续发现并校验全部定义；运行校验只检查本局必需定义，同时用资产注册表元数据识别必需 ID 的缺失和冲突。无关定义错误不得升级为本局错误。必需错误不发布目录、不给战斗角色和武器台生成半有效实例。

监听服务器对新玩家发送依赖列表、协议版本、摘要和一次性挑战标识。客户端独立计算本地图依赖与摘要，完成确认前服务器不给其生成战斗角色，也不能开始包含未确认玩家的回合。确认有截止时间；不同内容明确拒绝并保留原因。此握手用于同版本内容准入，不是对恶意修改客户端的反作弊证明。

## 摘要契约

本阶段验收时格式版本为 1；后续 [R6＋R7](Presentation.md) 因表现时间语义变化将契约版本提升为 2，以下编码与资源来源规则保持。稳定 WeaponId 排序；固定字段顺序、类型化二进制编码。包含身份、原始显示文本、槽位/分类、弹药、射击、操作、后坐力、各视角变换、位置、FOV，以及曲线全键值/切线/权重/外推模式。网格和声音使用保存资源及其包依赖的版本摘要。编辑器保存定义时生成 `SavedResourceVersions` 并随定义进入 Cook；编辑器/发布校验检查缺失或过期版本，普通 Game 不读取编辑器专用包哈希。修改引用资源后需保存相关定义；不能手填版本缓存或只修改只读文本清单。定义包路径、时间戳、当前本地化显示字符串不参与；同内容的定义改名或目录移动不改变型号身份。资源重新保存可能产生新的资源版本，需要两端同步更新。格式变动必须提升协议版本；局中热更新不支持。

## 验证记录

七定义和两个 FPS 地图已通过编辑器命令let重存软引用；原教学地图的 SHA-256 在迁移前后相同。原资产备份位于 `Saved/WeaponContentMigration/BeforeSoftReferences/`。只读清单已从资产重新导出，生成器补齐文首标识，相应四项历史豁免已删除。

9316 完成初轮回归后，源码复核发现 UE 的 PackageSavedHash 不进入普通运行时；9317 改为保存生成版本并在发布阶段检查过期，重新运行准入和玩法回归。9318 仅进一步收紧无后坐力的曲线引用校验：必须真正留空，失效的非空软引用不能冒充空值；新增负例、两种构建、依赖自测和双端正常准入/射击重新通过。以下明确区分版本，不用中间版本证明后来改动。

负例修复记录：首次命令行故障参数未引用，PowerShell 把 Weapon.AK 拆开；修正传参后，必需错误与发布拒绝通过。9315 内容不一致已在角色生成前拒绝，但客户端显示通用断线原因；9316 保留具体拒绝原因，双端负例通过。失败记录与修正后日志分开，不把失败当成通过。

| 验证入口 | 构建与结果 | 日志（Saved/Logs） |
| --- | --- | --- |
| Editor Development 构建＋数据检查 | 9318 成功，七定义验证通过 | WeaponBuild9318.txt |
| MCP_Test Win64 Development | 普通 Game 目标成功，未执行 Cook/打包 | WeaponGameBuildR34Final.txt |
| FPSWeaponData -SelfTest | 9318 完整数据与负例自测通过，含非空失效曲线 | WeaponR34Publish9318.txt |
| FPSWeaponData -ContentSelfTest -FPSInvalidWeapon=Weapon.AK | 9318 依赖隔离、摘要变化、重命名摘要稳定、GC 回收、必需错误阻止通过 | WeaponR34ContentSelf9318.txt |
| FPSWeaponData -FPSInvalidWeapon=Weapon.AK | 9317 全项目发布校验按预期失败，明确 FireInterval=-1；不是构建通过证据 | WeaponR34InvalidPublish9317.txt |
| TestFPSWeaponContent.ps1 | 9317 容量/射速两组真实双端拒绝；服务器 pawn=0，客户端收到具体原因且离局释放句柄 | WeaponR34Admission9317.txt |
| TestFPSWeaponInvalid.ps1 | 9317 必需错误下 pawns=0、pickups=0、combat=0 | WeaponR34Invalid9317.txt |
| TestFPSWeapons.ps1 -Abilities -Latency 50 -PacketLoss 1 | 9317 R1/R2 双端 COMPLETE PASS | WeaponR34Abilities9317.txt |
| TestFPSWeapons.ps1 -Damage -Latency 50 -PacketLoss 1 | 9318 正常内容握手及 R5 双端伤害 COMPLETE PASS | WeaponR34Damage9318.txt |
| TestFPSWeapons.ps1 -Teaching -InvalidWeapon Weapon.AK | 9317 无关 AK 故障下教学双端伤害、治疗、重生通过 | WeaponR34Teaching9317.txt |
| TestFPSWeapons.ps1 -Audio -Visual | 9317 七枪身份、资源、预测、换弹和转移通过；BR3 截图模型/HUD 正常 | WeaponR34Weapons9317.txt |
| TestFPSAI.ps1 | 9317 Solo 人机实际绕行、接敌、GAS 计分通过 | WeaponR34AI9317.txt |

网络运行条件为同机后台 `UnrealEditor.exe -game` 监听服务器＋独立客户端；专项日志确认 `PktLag=50`、`PktLoss=1`。不是编辑器 PIE、Dedicated Server 或人工操作。错误与客户端本局差异使用进程内开发注入，不把测试修改保存到资源资产。对象改名/移动测试是内存对象测试，没有在本轮重新执行实体资产重命名往返。

观察样本：9317 Foundry 加载 7 定义、24 个直接资源，服务器约 14.76ms、客户端约 13.96ms；教学仅加载 1 定义、8 个直接资源，约 8.23ms/7.89ms。依赖命令let加载 M4/M9 后实际驻留 2 个定义，释放句柄并 GC 后为 0；拒绝客户端返回大厅也观察到 7 定义句柄释放。资源数不含传递依赖，不等于总 UObject、显存或总内存。缓存条件未控制，不把这些样本宣称为性能优化幅度。

9317 输出音频非静音且无削波，服务器/客户端 peak 0.1196/0.1910；只人工查看 BR3 截图，不声称所有远端表现均完成视觉审查。原始双端与人机日志按版本保存在 `Saved/Logs/WeaponR34Evidence/`；命令let及构建汇总保存于表内位置。

交付检查：`python Tools/CheckDocs.py` 和 `git diff --check` 通过；32 文档、0 错误，历史例外从 110 减至 106，未新增豁免。人工核对阶段状态、日志版本、生成来源和主题索引。

Dedicated Server、完整 Cook/打包、多客户端压力和整体加载性能仍需单独验证；加载日志只描述本次进程与本局样本。

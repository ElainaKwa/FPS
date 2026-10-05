> 历史归档：2026-09-29 原评审，含已纠正或过时的结论，请勿作为当前待改清单。当前版本见 [纠正后的架构评审](../FPSWeaponArchitectureReview.md)。本文件保留原始判断以便追溯。

# FPS Demo 架构评审：现状、不足与耦合

评审日期：2026-09-29（当轮）
评审对象：FPS Demo 新架构（`AFPSLobbyMode` / `AFPSMatchMode` 路线），重点是枪械重构后的武器系统
评审方式：**纯代码阅读**（`Source/MCP_Test`），未运行编辑器、未编译、未跑 PIE 或双端测试
事实基准：第三阶段代码状态，`FPSWeaponSmoke.cpp`、`FPSWeaponDataCommandlet.cpp`、`FPSMatchFlow.cpp` 停在 09-29 21:51 前后

与其他文档的关系：

- 本文是**外部评审记录**，不是设计依据。设计规则仍以 [枪械系统重构：设计决策与交接](../FPSWeaponRefactorDecisions.md) 为准。
- 分阶段实施与验证结果见 [第一阶段](../FPSWeaponRefactorStage1.md)、[第二阶段](../FPSWeaponRefactorStage2.md)、[第三阶段](../FPSWeaponRefactorStage3.md) 与 [待改清单](../FPSWeaponRefactorChecklist.md)。
- 尚未实现的功能项不在本文重复，只在 §6 标注边界。

标注约定：**[已知]** = 相关实施文档已写明；**[新]** = 本次评审新指出或文档未提及的部分。

---

## 1. 架构现状（一页速览）

分层与依赖方向（头文件级基本单向）：

```text
入口层     AFPSFrontController（Slate 大厅/结算） · AFPSDemoHUD · 6 个 Smoke 入口
  ↑
模式层     AFPSLobbyMode → AFPSMatchMode（单机/联机/靶场）  ·  AFPSBotController（自研寻路 AI）
  ↑
角色层     AFPSDemoCharacter ──┬── UFPSAvatarVisuals（程序化动画）
                              ├── UFPSMovementComponent（SavedMove 疾跑同步）
                              └── UFPSWeaponComponent（库存 + 权威提交）
  ↑
行为层     FPSWeaponAbilities.h（共用 Fire / Reload / Inspect / Primary / Secondary / Equip / Aim / Drop / Pickup / Cancel）
  ↑
数据层     FPSWeaponData.h（UFPSWeaponDefinition 资产 · UFPSWeaponCatalog 目录 · FFPSWeaponInstance 实例 · AFPSWeaponStand）
```

运行形态要点：

- 对局规则与旧教学完全分离，靠 GameState 上复制的 `bStructuredMatch` 切换（`Source/MCP_Test/FPSDemoGameState.h:23`）。
- 武器数据来源唯一：已保存的 `UPrimaryDataAsset` + `UCurveVector`，Asset Registry 自动发现（`Source/MCP_Test/FPSWeaponData.cpp:72-86`）。
- 动作调度以 GameplayAbility 为核心，组件只保留库存与权威提交（`Source/MCP_Test/FPSWeaponComponent.h:17`）。
- 复制按接收方拆分：拥有者拿完整双槽快照，其他客户端只拿当前武器与动作/ADS（`Source/MCP_Test/FPSWeaponComponent.cpp:88-90`）。
- 服务器独占命中、伤害、拾取提交；本地只做预测表现（`Source/MCP_Test/FPSWeaponComponent.cpp:258-281`）。

---

## 2. 不足清单

### 2.1 结构性

| # | 问题 | 位置 | 影响 |
| --- | --- | --- | --- |
| S1 | **伤害有两个真相源**：射击时用 `Def->Damage` 写入 ASC 属性，结算时反过来读 `Attributes->GetWeaponDamage()` | 写 `FPSWeaponComponent.cpp:100`；读 `FPSDemoCharacter.cpp:413` | 定义变更与结算之间无类型保障，只靠「每次改动都调 `Changed()`」维持一致 |
| S2 | **组件与 Ability 互相穿透**，无接缝 | `FPSWeaponComponent.h:54-82`；`FPSWeaponAbilities.cpp:135, 201` | 任一侧无法独立替换或单测 |
| S3 | **输入双份复制**：`bReplicateInputDirectly=true` + 手动 `AbilityLocalInputPressed/Released` | `FPSWeaponAbilities.cpp:19`；`FPSWeaponComponent.cpp:174, 179, 213` | 每次按键走引擎内建输入复制，而非单条自定义 RPC |
| S4 | **`Command` 枚举被当复用槽**：Equip Ability 借用 `EFPSWeaponRequest::AimOff`，导致 `CanStart(AimOff)` 恒真并带 `// internal equip` 注释 | `FPSWeaponAbilities.cpp:65`；`FPSWeaponComponent.cpp:56` | 请求语义与内部状态语义混用，后续加动作易踩 |
| S5 | **`UFPSFireAbility::Step()` 职责过载**：计时、射速、模式、跨换弹/举枪边界、切枪重绑、预测记账集中一处，并含 0.02 秒轮询等待 | `FPSWeaponAbilities.cpp:91-122` | 三连发/单发边界逻辑难以独立验证 |
| S6 | **共享 ASC 承担非属性职责** **[新]** | `FPSDemoCharacter.cpp:413` | 武器的伤害数值经角色属性通道传递，扩大耦合面（详见 §3.2） |

### 2.2 数据与内容契约

| # | 问题 | 位置 | 影响 |
| --- | --- | --- | --- |
| D1 | **Asset Manager 只是旁观者**：裸 Asset Registry 全量扫描 + 同步加载全部定义，由 GameInstanceSubsystem 长期强持有；编辑器的 `PrimaryAssetTypesToScan` 仅用于一次路径断言 | 扫描 `FPSWeaponData.cpp:72-86`；加载 `:109-129`；断言 `:97` | 资产包切分、按需加载、按地图裁剪均不可用；启动成本随枪数线性上升 |
| D2 | **校验全有或全无**：任何一条定义无效即阻断所有对局 | `FPSWeaponComponent.cpp:40`；`FPSMatchFlow.cpp:65` | 与设计目标「本局必需定义无效才阻止」不一致 |
| D3 | **无内容版本一致性检查** **[已知]** | 定义无内容哈希，复制只有 `WeaponId` | 客户端/服务器定义数值不同会静默产生不同伤害与弹药 |
| D4 | **制作初值是第二真值源** **[已知]** | `FPSWeaponDataCommandlet.cpp:120` 起 | 命令侧初值与已保存资产可漂移，靠纪律而非工具保证 |
| D5 | 定义资产内部仍是固定三视角 + 固定 6 个音效槽位 | `FPSWeaponData.h:56-68` | 与「不塞未消费占位字段」的目标有偏差，但可接受 |

### 2.3 玩法与表现完整度

| # | 问题 | 位置／依据 | 影响 |
| --- | --- | --- | --- |
| G1 | **无延迟补偿** **[已知]** | `FPSDemoCharacter.cpp:377` | 服务器按开火瞬间朝向射线，高延迟打移动目标必空 |
| G2 | **无伤害衰减** **[新]**：`Range` 只决定射线长度，伤害恒定 | `FPSDemoCharacter.cpp:373-401` | 远近距离手感无差别 |
| G3 | **无部位/爆头倍率、无弹道与穿透** **[新]** | 同上 | 命中反馈层次单一 |
| G4 | **无散布/后坐力对命中的实际影响** **[新]** | 后坐力只作用于本地镜头与枪体 | 连发精度不随时间变化，压枪无收益 |
| G5 | **远端玩家只有枪体与枪口灯光** **[新]**：换弹/检视/切枪动作与音效仅本机播放 | `FPSWeaponComponent.cpp:140-152` | 旁观者看不出对手在换弹，战术可读性缺失 |
| G6 | **表现层硬编码与隐式耦合** **[新]**：非开镜 FOV 固定 `92.f`；姿势过渡把 `EquipBlendTime` 除以魔法常数，且 `= 0` 时速率退化为 0 | `FPSDemoCharacter.cpp:245, 263-264` | 零值是隐藏地雷；FOV 缺数据通道 |
| G7 | **换弹完成靠启发式判定** **[新]**：以 `Magazine` 是否变大推断 | `FPSWeaponComponent.cpp:150` | 满弹匣换弹或数值跳变会漏播/误播 |
| G8 | **武器台一次性**：拾取物认领后销毁，靠固定 180 秒 `SetLifeSpan` 兜底 | `FPSWeaponPickup.cpp:41-47`；`FPSWeaponComponent.cpp:333` | 无重生规则、无配置项 |
| G9 | **AI 与规则/地图写死** **[新]**：巡逻点、交战距离、寻弹逻辑内联 | `FPSMatchFlow.cpp:343, 363-378` | 加第二张竞技图必须改 AI |
| G10 | **模式判断用字符串比较** **[新]** | `FPSMatchFlow.cpp:61-62` | `ModeName == TEXT("Training")` 一类判断脆弱 |

### 2.4 验证缺口

| # | 问题 | 依据 |
| --- | --- | --- |
| V1 | 没有带宽/复制次数测量，「拆分接收范围」的收益目前只是设计意图 | 第二阶段文档自述未抓包 |
| V2 | 测试覆盖功能与时长，未覆盖武器数量增长下的启动与加载成本 | 本次阅读所见 |
| V3 | 第三阶段 BR-3 的最终构建、真实双端运行与画面验收仍待记录 | [第三阶段](../FPSWeaponRefactorStage3.md) |
| V4 | 完整 5 分钟自然结束、10 真实客户端、打包/Cook 与性能压力未覆盖 | 各阶段文档一致的未覆盖项 |

---

## 3. 耦合分析

### 3.1 拓扑：编译期大体分层，但存在四个环

| 环 | 位置 | 说明 |
| --- | --- | --- |
| C1 | `FPSWeaponAbilities.h:4` ← → `FPSWeaponComponent.cpp:2` | 行为层反向包含库存层（头文件环） |
| C2 | `FPSDemoGameMode.cpp:8` ← → `FPSMatchFlow.h:4` | 基础 GameMode 为询问规则而包含派生模式 |
| C3 | `FPSDemoCharacter.cpp:13` ← → `FPSMatchFlow.h:31` | 角色为 `CanDamage` 包含模式层 |
| C4 | `FPSDemoCharacter.cpp:12` ← → `FPSDemoGameMode.cpp:3` | 角色 ↔ 基础 GameMode |

C2/C3/C4 主要源于实现细节（`FPSMatchFlow.h` 同时容纳 LobbyMode、MatchMode、BotController、FrontController 五个类型），并非接口需要。代价：`FPSDemoCharacter.cpp` 无法单独编译，`AFPSDemoGameMode` 无法脱离派生模式复用。

节点度：

| 角色 | 文件 | 事实 |
| --- | --- | --- |
| 被依赖最多（fan-in ≈ 14） | `FPSWeaponComponent.h/.cpp` | 被角色、Abilities、Movement、AvatarVisuals、HUD、MatchFlow、Pickup、4 个 Smoke 引用 |
| 依赖最多（fan-out） | `FPSDemoCharacter.cpp` | 单文件 16 个自有 include |
| 双向枢纽 | Character ↔ WeaponComponent | 头文件之间干净（仅前置声明），实现文件互相调用成环 |
| 完全孤立（好） | `FPSDemoArena` | 纯几何，零耦合 |
| 零耦合（好） | `FPSAudioSubsystem`、`FPSWeaponData.h`、网络基础类 | 不反向依赖玩法系统 |
| 与旧教学的耦合 | 几乎为零 | 仅共享 GameState/GameMode 基类与角色类，各自可独立运行 |

### 3.2 用共享可变状态代替接口（最隐蔽）

1. **`UFPSDemoAttributeSet::WeaponDamage` 当传参通道。** 写入在组件（`FPSWeaponComponent.cpp:100`），读取在角色（`FPSDemoCharacter.cpp:413`）。武器→伤害结算本是一条调用链，被拆成对同一复制属性的两次跨模块访问，等于把武器参数塞进角色属性集。
2. **`bStructuredMatch` 一旗多用。** 定义 `FPSDemoGameState.h:23`；读取扩散到 5 个文件、经 `IsEnabled()` 展开 8 处：`FPSWeaponComponent.cpp:31/39/46/67/77/162/201`、`FPSDemoCharacter.cpp:233/240/354/364/408/421/506`、`FPSMovementComponent.cpp:45`、`FPSAvatarVisuals.cpp:78`、`FPSDemoHUD.cpp:20`。语义在「结构化对局中」与「用哪套伤害效果」（`FPSDemoCharacter.cpp:408`）之间漂移。
3. **世界查找作为隐式依赖。** `TActorIterator` 遍历：`AFPSWeaponPickup` 3 处（`FPSWeaponComponent.cpp:311`、`FPSMatchFlow.cpp:363`、另见匹配流程计数）、`AFPSWeaponStand` 2 处（`FPSMatchFlow.cpp:67/100`）。「谁能捡」的判据散在一次最近距离+遮挡遍历里，无所有权归属。
4. **目录获取点隐式。** 4 处 `GetWorld()->GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>()`（`FPSWeaponComponent.cpp:22`、`FPSDemoHUD.cpp:22`、`FPSMatchFlow.cpp:64/81`、`FPSWeaponPickup.cpp:57`）。`MatchMode::InitGameState()` 读 `Catalog->ValidationError`（`FPSMatchFlow.cpp:65`）隐含「GameInstance 子系统先于任何地图初始化」的时序约束，代码顺序看不出。
5. **测试与生产直连。** 7 处 `friend struct FFPS*Smoke`：`FPSDemoCharacter.h:120`、`FPSMatchFlow.h:49/69`、`FPSWeaponComponent.h:54-55`，以及各 Smoke 头。Smoke 直接读写内部私有状态，不是外部测试框架。

### 3.3 成环的运行时调用

| 环 | 双向事实 |
| --- | --- |
| 库存 ↔ 行为 | 组件持 `ActionAbility/AimAbility/FireAbility`（`FPSWeaponComponent.h:80-82`）；能力经 `friend` 直改 `Loadout`/`OwnerState`/`PredictedShots`（`FPSWeaponAbilities.cpp:114, 135, 201`）；组件又主动取消能力（`FPSWeaponComponent.cpp:218`） |
| 角色 ↔ 武器 | 组件驱动角色表现与结算：`Character()->PlayWeaponShot(...)`、`ResolveWeaponShot(Def->Range)`（`FPSWeaponComponent.cpp:274, 279`）；角色反向取武器数据：`Weapons->Definition()`、`Weapons->Catalog()`（`FPSDemoCharacter.cpp:348, 383, 408`）；两侧都靠 `Cast<AFPSDemoCharacter>(GetOwner())` 互取（`FPSWeaponComponent.cpp:21`） |
| 判据倒挂 | 传输 Ability 的可用性由组件搜世界决定（`CanStart(Pickup)` 要求 `FindPickup()`，`FPSWeaponComponent.cpp:53`），真正提交又回到组件 `Transfer()`（`:282`） |

---

## 4. 落到几个具体设计决定上的观察

1. **Catalog 的初始化时机把不相关关卡也绑进来了。** 全量 `SearchAllAssets` + 同步加载发生在 GameInstance 创建时，即任何地图加载之前（`FPSWeaponData.cpp:109-129`）。系统边界不只由代码划分，也由加载时机划分——平台跳跃与教学关卡同样承担武器数据的加载成本。
2. **角色类同时背负新旧两套语义。** `ResolveWeaponShot` 内部按 `IsEnabled()` 分叉教学/结构化两条路径（`FPSDemoCharacter.cpp:383, 408`），教学模式还在角色里保留 `ServerFire`、`ServerRequestHeal` 与治疗站遍历（`:362, 434-447`）。同一函数服务两种设计。
3. **`UFPSWeaponCatalog` 是事实上的静态全局。** 一次初始化、长期强持有（`FPSWeaponData.h:147`），复制发布语义（`WeaponDataError`）与纯查询混在同一对象上。
4. **AI 决策与地图几何写在一起。** 巡逻点与交战距离内联在 BotController（`FPSMatchFlow.cpp:343, 378`），地图是隐式参数。
5. **模式身份的表达方式偏弱。** `FPSDemoGameState.h:23-25` 用 `bStructuredMatch` + `bTraining` 两个独立布尔表达状态，加上字符串比较（`FPSMatchFlow.cpp:61-62`），缺少一个统一的对局配置对象。

---

## 5. 建议处理顺序

按「收益 / 成本」排序，前两项互相独立、都不牵动玩法语义：

1. **拆掉库存 ↔ 行为的环。** 能力只通过组件公开方法请求状态变更，不再直改 `Loadout`/`OwnerState`；组件不再持有能力指针，改为事件/委托通知。移除 `friend` 依赖。
2. **伤害单一真相源。** `ResolveWeaponShot(Range, Damage)` 直接传参，`WeaponDamage` 属性退出结算路径（可保留用于 HUD 或删除）。
3. **角色只做装配。** 角色不再直接调用结算与表现实现，武器系统经接口/子系统交互；顺便把教学模式的 `ServerFire`/`ServerRequestHeal` 与治疗站依赖从角色剥离。
4. **`bStructuredMatch` 收敛为显式对局配置对象**（模式、规则、伤害效果选择集中一处），消除一旗多用。
5. **Catalog 改为按需加载**，并让「本局必需定义」的校验粒度对应对局而非全局；补上内容版本检查（D3）。
6. **AI 参数化**，把巡逻点、交战距离、寻弹阈值移到地图或模式配置，为第二张地图做准备。
7. **表现层去硬编码**：FOV 进数据、过渡常数收敛、零时长走显式分支、换弹结束改为显式事件而非弹药数启发式。

---

## 6. 不在本次评审范围内的已知边界

以下属决策文档明确排除或标注未实施的内容，不算缺陷，列此避免重复提出：投掷物/弹道、逐发装填、配件、耐久、穿透、变倍镜、自定义准星、每武器独立重装动画、房间列表与公网穿透（Steam/EOS/自动发现）、局中热更新与旧客户端兼容、持久武器存档。

---

## 7. 本次评审未验证的部分

- 未编译、未运行编辑器、未执行 PIE 或双端网络测试；所有结论来自代码阅读。
- 未测量带宽、复制频率、加载耗时与帧耗；V1、V2 因此只能标为「未覆盖」，不是「已确认劣化」。
- 行号以本文件记录的代码版本为准，后续改动会使行号漂移，定位时请以符号名（类名/函数名）为准。
- 第 2、3 节中标注 **[新]** 的条目是对代码的判断，未经口头确认优先级；标注 **[已知]** 的条目在实施文档中已有对应记载。

# UE 5.8 网络同步 FPS 教学 Demo

## 打开与游玩

- 地图：`/Game/Maps/LV_NetSyncFPS`。本地图单独覆盖 `AFPSDemoGameMode`，不改变原来的地图。
- 在编辑器的播放设置中选 **2 Players**、**Play As Listen Server**、**New Editor Window (PIE)**，然后播放。一个窗口为 Server 0，另一个为 Client 1。
- `WASD` 移动，鼠标瞄准，左键开火，`Space` 跳跃。站到地图中央绿色治疗站附近按 `E` 治疗。
- 每枪造成 25 点伤害，初始生命 100；第四枪击倒目标，2.5 秒后服务器重生目标。治疗恢复 35 点，服务器有 3 秒冷却。
- 观察两端 HUD：顶部 `SERVER` / `CLIENT` 是当前窗口的网络角色；生命条来自 GAS 属性；击倒总数和双方 K/D 来自复制的 GameState、PlayerState。命中提示只发给开枪者。

## 一次射击如何同步

```text
拥有 Pawn 的客户端输入
  → 本地立即播放枪械后坐力、枪口闪光（不等待网络）
  → ServerFire RPC 发给服务器
  → 服务器校验射速和存活状态，并从服务器控制朝向做射线检测
  → 命中时服务器向目标 ASC 应用 UFPSDemoDamageEffect
  → GAS AttributeSet 的 Health 复制到客户端
  → 服务器只向射手发送 Client RPC 命中确认
  → 服务器向所有端发送不可靠 Multicast 射击线与远端枪口效果
  → 生命归零时 GameMode 判定击倒、记分和计时重生
```

教学关键点：客户端请求“开枪”，不提交“我打中了谁”。伤害和击倒由服务器决定。本地动画是预测式即时反馈，远端特效可以丢包，所以用不可靠 Multicast；生命与比分是可靠的状态复制。这里的射线检测是入门版，并未实现延迟补偿。

## Gameplay Framework：创建位置与同步范围

| 类 | 谁创建，在哪里存在 | 本 Demo 的职责与同步 |
| --- | --- | --- |
| `AFPSDemoGameMode` | 地图加载时仅服务器创建 | 设置 Pawn、Controller、HUD、GameState、PlayerState 类型；判定击倒与重生。**GameMode 本身不会复制给客户端**。 |
| `AFPSDemoGameState` | 服务器创建，客户端各有复制实例 | `Eliminations` 和 `LastEvent` 由服务器写，复制给所有玩家。用于所有人都该看到的对局状态。 |
| `AFPSDemoPlayerController` | 服务器有每个玩家的 Controller；每个客户端只拥有自己的 Controller | 负责所有权和 `ClientConfirmHit`，只给射手显示命中结果。不能用它广播全体比分。 |
| `AFPSDemoPlayerState` | 服务器为每位玩家创建，所有客户端都收到副本 | 击倒数、死亡数由服务器写并复制，供双方 HUD 显示。重生后状态继续保留。 |
| `AFPSDemoCharacter` / `BP_FPS_Character` | 服务器从已保存的角色蓝图生成并 Possess，客户端获得复制的 Pawn | CharacterMovement 同步移动，`bEliminated` 复制，拥有者可调用 Server RPC。第一人称枪体仅拥有者可见，第三人称模型只给其他人看。 |
| `AFPSDemoHUD` | 每个本地 PlayerController 自己创建 | 每帧从本地可见的复制状态绘制；HUD Actor 本身不参与网络复制。 |
| `AFPSDemoArena` | 地图中放置，随关卡加载 | 地面、围墙、掩体和说明字样由 C++ 组件建模；它是静态关卡内容，没有运行时状态要复制。 |
| `AFPSDemoHealStation` / `BP_FPS_HealStation` | 地图中独立放置，随关卡加载 | 使用保存的治疗站网格体、提示文字和脉冲灯光。服务器根据站点位置检查治疗范围，再应用 GAS 治疗效果。 |

## GAS 在哪里起作用

`AFPSDemoCharacter` 实现 `IAbilitySystemInterface`，持有复制的 `UAbilitySystemComponent` 和 `UFPSDemoAttributeSet`。属性集定义 `Health`、`MaxHealth`，使用 `ReplicatedUsing`、`DOREPLIFETIME_CONDITION_NOTIFY` 与 `GAMEPLAYATTRIBUTE_REPNOTIFY` 传播变化。服务器应用两个即时 `GameplayEffect`：`UFPSDemoDamageEffect` 对 Health 加 `-25`，`UFPSDemoHealEffect` 加 `+35`，属性集把结果限制到 `0..MaxHealth`。

这里用到了 GAS 的 ASC、AttributeSet、GameplayEffect 和属性复制；**没有把开枪做成 `UGameplayAbility`**。这样可清楚区分 RPC/命中判定和 GAS 数值修改。后续扩展弹药、换弹、技能冷却时，可把开枪与治疗迁入 GameplayAbility，并用 GameplayTag 表示状态。

官方参考：[Gameplay Ability System](https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-system-for-unreal-engine)、[Attributes and Attribute Sets](https://dev.epicgames.com/documentation/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system-in-unreal-engine)、[Ability System Component](https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-system-component-and-gameplay-attributes-in-unreal-engine)。

## 模型与动画实现

`AFPSDemoArena` 用 StaticMeshComponent 在 C++ 构造竞技场和掩体。`BP_FPS_Character` 保存了 C++ 拼装的头盔、护甲、四肢、背包以及玩法组件。枪体步行摆动、开火后坐力、远端手脚摆动、枪口灯光由 `Tick` 中的插值与正弦运动程序驱动；服务器 Multicast 使旁观者看到枪口动作与短暂射击线。此处是程序动画示例，没有骨骼动画资产。

## 可复用的已保存资产

| 资产 | 项目路径 | 用途 |
| --- | --- | --- |
| 角色蓝图 | `/Game/FPSDemo/Blueprints/BP_FPS_Character` | `AFPSDemoGameMode` 实际生成此蓝图；可在编辑器中查看或继承角色外形与组件。 |
| 优化后的枪械网格体 | `/Game/FPSDemo/Meshes/SM_FPS_Rifle` | 机匣、护木、散热口、枪管、枪口制退器、枪托、握把、弹匣和瞄具组成的独立网格体；第一人称和第三人称共用。 |
| 治疗站蓝图 | `/Game/FPSDemo/Blueprints/BP_FPS_HealStation` | 可独立拖入其它地图；包含建模网格体、脉冲灯光和 `HEAL [E]` 提示。 |
| 治疗站网格体 | `/Game/FPSDemo/Meshes/SM_FPS_HealStation` | 平台、能量核心、支架、顶部罩和地面信标；可单独用作场景道具。 |

OBJ/MTL 建模源文件保存在项目的 `ArtSource/FPSDemo`，`Tools/generate_fps_models.py` 可重新生成两份源文件；游戏运行依赖的是已导入并保存的 `.uasset`。`Tools/finalize_fps_assets.py` 是一次性编辑器自动化脚本，Gameplay 逻辑仍由 C++ 实现。

## 可直接讲的实验

1. 双窗口同时观察：客户端开枪时立即出现枪口动作；服务器确认命中后才变色显示命中提示。
2. 射击对手四次：两边生命条、击倒数和 K/D 变化；2.5 秒后重生，个人比分保留。
3. 让受伤玩家到中心按 `E`：服务器验证距离、冷却并应用治疗 Effect；两边看到 Health 变化。
4. 比较两端 `SERVER` / `CLIENT` 标签，并在代码中定位 `ServerFire_Implementation`、`MulticastShotFX_Implementation`、`OnRep_Health`、`HandleElimination`，对应 RPC、状态复制和 GameMode 权限。

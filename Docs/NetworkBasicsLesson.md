# UE 5.8 网络同步基础教学关卡

关卡：`/Game/Maps/LV_NetworkBasics`。此关卡使用独立的 `ANetLessonGameMode`，不会改变项目原有横版关卡的 GameMode。

## 运行

1. 在编辑器中打开 `LV_NetworkBasics`。
2. 点击“播放”右侧的选项，将玩家数量设为 **2**，网络模式设为 **Play As Listen Server**，选择“新建编辑器窗口 (PIE)”。
3. 启动 PIE。两个窗口标题分别标明 `Server 0` 和 `Client 1`。
4. 在任一窗口中按 **A/D** 移动、**Space** 跳跃；点击窗口后按数字键 **1/2/3** 触发教学示例。

## 讲解顺序

| 按键 | 观察现象 | C++ 路径 | 要点 |
| --- | --- | --- | --- |
| 1 | 两个窗口中的计数和青色标签一起加一 | `ANetLessonCharacter::ServerRequestCounter` → `ANetLessonStage::ServerIncrementCounter` → `Counter` / `OnRep_Counter` | 客户端先通过拥有的角色发送 Server RPC；服务器更改权威状态，属性再复制给客户端。服务器手动刷新标签，客户端由 RepNotify 刷新。 |
| 2 | 两个窗口短暂显示红色 Multicast 提示 | `ServerRequestPulse` → `ServerBroadcastPulse` → `MulticastPulse` | 服务器发起 Multicast，用于向当前相关的连接播放瞬时效果。它不是持久状态；晚加入的客户端不会补播。 |
| 3 | 只有按键的窗口显示绿色提示 | `ANetLessonCharacter::PressLocal` | 普通本地变量和效果不会自动同步。 |

`ANetLessonGameMode` 只在服务器存在；玩家角色和演示台会复制到客户端。场景中的演示台、Framework 展板、文字和地板由 `ANetLessonStage` 的 C++ 组件构造。HUD 也由 C++ 绘制，显示当前窗口的服务器/客户端身份和复制计数。

## Gameplay Framework：谁创建、在哪里、怎样同步

沿着关卡右侧的七个展台从左到右讲解。表中的“所有端”指服务器和当前相关的客户端；服务器始终是权威端，客户端副本不能直接改写服务器状态。

| 类 | 谁/何时创建 | 实例在哪里 | 同步方式；本关卡实例 |
| --- | --- | --- | --- |
| `UGameInstance` | 引擎启动游戏实例时创建；跨地图保留 | 服务器与每个客户端各自有一份 | **不复制**，不能把它当作共享比赛状态。本关卡保留引擎默认类。 |
| `AGameModeBase` | 加载地图时，服务器按该地图的 World Settings 创建 | 仅服务器 | 自身**不复制**；通过 `GameState` 向客户端公开共享数据。`ANetLessonGameMode` 指定下方各类。 |
| `AGameStateBase` | GameMode 在关卡初始化时创建 | 服务器与所有客户端 | Actor 和标记的属性由服务器复制；`ANetLessonGameState::SharedRequestCount` 在两个窗口相同。 |
| `APlayerController` | 玩家连接/登录时为该玩家创建 | 服务器有每位玩家的 Controller；客户端只有自己的 | 依靠拥有连接传 Server/Client RPC，不适合存放所有人都要看到的数据。`ANetLessonPlayerController` 收到仅拥有者可见的确认。 |
| `APlayerState` | 服务器为每位玩家创建 | 服务器与所有客户端都有每位玩家的 PlayerState | 适合玩家名、比分等公开个人状态。`ANetLessonPlayerState::PersonalRequestCount` 在两端的 `GameState->PlayerArray` 中可见。 |
| `APawn` / `ACharacter` | GameMode 的玩家重生流程在服务器生成，并由 PlayerController 占有 | 服务器与相关客户端 | 开启 Actor 复制；`ACharacter` 的移动组件处理玩家移动的网络同步。`ANetLessonCharacter` 是拥有者发送 Server RPC 的入口。 |
| `AHUD` | 本地 PlayerController 为自己的视口创建 | 拥有该玩家视口的一端 | **不复制**；从复制到本地的 GameState、PlayerState、Pawn 读取数据并绘制。`ANetLessonHUD` 展示这些值。 |

```text
引擎启动                 地图加载（服务器）                 玩家加入（服务器）
GameInstance ──────────── GameMode ─── GameState ───────── PlayerController
每端独立                 仅服务器      复制给所有端             ├─ PlayerState → 所有端
                                                          ├─ Pawn/Character → 相关客户端
                                                          └─ 本地 HUD → 仅所属视口
```

**按 1 做现场对照：**服务器在同一次请求中更新演示台计数、GameState 全局请求数、发起者的 PlayerState 个人请求数，然后通过 PlayerController 发回 Client RPC 确认。两边 HUD 的全局数与所有玩家的个人数一致；确认只出现在发起者的窗口。另一个客户端没有该发起者的 PlayerController，但能看到其 PlayerState。

其它常见类：`AActor` 只有开启复制并由服务器生成/管理时，运行时生成与复制属性才按网络规则送到客户端；地图预置 Actor 原本由各端加载地图获得。`AAIController` 通常在服务器运行。`UUserWidget`、`UActorComponent`、普通 `UObject` 并不会因为所属 Gameplay Framework 类存在就自动共享所有变量；UI 通常读取复制数据，组件和子对象按各自的复制机制配置。

## 代码位置

- `Source/MCP_Test/NetLessonCharacter.*`：输入、拥有者发起的 Server RPC、本地效果。
- `Source/MCP_Test/NetLessonStage.*`：复制属性、RepNotify、Multicast、场景组件。
- `Source/MCP_Test/NetLessonHUD.*`：每个窗口的观察面板。
- `Source/MCP_Test/NetLessonGameMode.*`：关卡专用 Framework 类配置。
- `Source/MCP_Test/NetLessonGameState.*`：向所有玩家公开的全局状态。
- `Source/MCP_Test/NetLessonPlayerState.*`：每位玩家的公开个人状态。
- `Source/MCP_Test/NetLessonPlayerController.*`：只发给所属玩家的 Client RPC。

注意：按键 1/2 是面向教学的全场触发键，可在关卡任意位置使用。实际游戏应由服务器校验交互距离、权限与触发频率。

参考：[Epic Gameplay Framework](https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-in-unreal-engine)、[Game Mode 与 Game State](https://dev.epicgames.com/documentation/unreal-engine/game-mode-and-game-state-in-unreal-engine)、[Framework Quick Reference](https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-quick-reference-in-unreal-engine)、[Actor Owner 与 Owning Connection](https://dev.epicgames.com/documentation/unreal-engine/actor-owner-and-owning-connection-in-unreal-engine)。

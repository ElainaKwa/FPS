# 工业关隘大型对战地图

- 用途：实施与验证记录
- 适用范围：`LV_FPSIndustrialCrossing`、CrossingKit 环境模块、按地图配置的平面 AI 导航
- 状态：地图与环境套件已接入，独立进程实机和双端联机验证通过；视觉验收待用户确认，未验证打包与性能
- 最后更新：2026-10-13

## 目标与边界

用户于 2026-10-13 要求新增更大的 FPS 对战地图，可参考 CS2 沙二；随后明确地图风格和产物须符合既有美术规范。布局借鉴长道、短道、中门、侧翼隧道与两个交战区的连接关系；视觉采用 [统一美术设计决策](../Art/DesignDecisions.md) 的工业设施方向，未使用 Valve 模型、贴图或地图文件，也不声称一比一复刻。

正式地图位置为 `/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing`。平面范围 112 × 96 米，是既有 48 × 32 米 Foundry 占地面积的 7 倍；面积包含建筑与外围边界，不等于可行走面积。A 装卸区、B 设备场、中门、长道、短道和 B 维护通道形成多条连接路线。主要可行走地面为 Z=0，屋顶和背景设施不是可达楼层。

沿用当前 5v5 团队对战、基地、配装、武器拾取与重生规则；A/B 是区域标识，未新增拆弹规则。地图设有两处基地、每队五个出生点和七个武器台。大厅选图菜单未改动，新图通过专用启动脚本或直接打开关卡游玩。

## 资产和维护入口

| 内容 | 位置 |
| --- | --- |
| 正式地图 | `Content/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing.umap` |
| 环境源文件及交接说明 | [CrossingKit](../../../ArtSource/FPSDemo/Environment/CrossingKit/README.md) |
| Unreal 环境网格 | `Content/FPSDemo/Art/Environment/CrossingKit/Meshes/` |
| 地图生成器 | [create_fps_industrial_crossing.py](../../../Tools/create_fps_industrial_crossing.py) |
| 游玩入口 | [LaunchFPSIndustrialCrossing.ps1](../../../Tools/LaunchFPSIndustrialCrossing.ps1) |
| 实机验证入口 | [TestFPSIndustrialCrossing.ps1](../../../Tools/TestFPSIndustrialCrossing.ps1) |

地图使用可编辑 StaticMeshActor、明确的玩法标记和已保存的原生模式配置。基础建筑使用引擎几何，新增套件包括门架、货箱、通风百叶、设备管线和防撞柱；共享材质仍引用现有 Industrial 目录。原有 Foundry、Training、教学地图及用户资产不迁移、不替换。

首次创建通过 Unreal Editor Python 执行生成器。默认拒绝覆盖已有地图；`-IndustrialCrossingRebuild` 仅用于仍由生成器维护的本图，会核对生成记录及导航标记后重新生成 Actor，手工修改前须停用此流程。按 [资产组织规范](../Art/AssetOrganization.md) 管理权威源文件与制作交接。

## 游玩和导航

在项目根执行：

```powershell
powershell -ExecutionPolicy Bypass -File Tools/LaunchFPSIndustrialCrossing.ps1
# 0–9 个人机；局域网建房仍按当前大厅等待/开局规则
powershell -ExecutionPolicy Bypass -File Tools/LaunchFPSIndustrialCrossing.ps1 -Bots 5
powershell -ExecutionPolicy Bypass -File Tools/LaunchFPSIndustrialCrossing.ps1 -Listen
```

`AFPSNavigationRegion` 在本地图保存 111 × 95 个、格宽 100 厘米的导航节点范围和 11 个巡逻路径槽位。公共交战节点在路径中重复，与侧路交替访问，避免双方沿同向环线长时间追不上。`UFPSBotNavigation` 优先读取该 Actor；无该 Actor 的旧地图仍使用原 47 × 31 范围和全局巡逻点。仍是静态平面导航，不支持多层、楼梯或动态障碍重建；扩展后的通路必须实际验证胶囊净空与 AI 行走，不能只扩大数字。

现有 Cook 配置包含 `/Game/FPSDemo/Maps`，新地图通过资产引用携带环境网格与材质；本轮不将这一配置等同于打包验证。新增 C++ 类需由新启动的编辑器进程加载当前模块，旧编辑器会话不能以刷新内容浏览器替代模块加载。

## 验证与证据

2026-10-13 使用 UE 5.8.1、`UnrealEditor-MCP_Test-7316.dll` 完成以下验证。文件哈希、导入尺寸、独立日志入口和当前地图对应的运行回执由 [交付报告](../../../ArtSource/FPSDemo/Environment/CrossingKit/DeliveryReport.json) 记录，报告生成器位于同套件 `Scripts/record_delivery.py`，禁止手改结果。

| 检查 | 观察结果与证据 |
| --- | --- |
| C++ 编辑器模块构建 | 成功；`Saved/IndustrialCrossing/Build.log` |
| 环境套件 | 五个 FBX 与 Unreal 网格尺寸一致；`Saved/IndustrialCrossing/KitImport.json` |
| 实际渲染与持续对战 | `Tests/20261013-020701007/`：10 个出生点净空、2 处基地、14 个武器台和全部巡逻连接通过；9 个人机全部活动，85 秒内 46 次击杀，未越界 |
| 实际服务器与客户端 | `Tests/20261013-020936327/`：17 项对局规则检查通过，客户端收到伤害、死亡、重生、计分和最终快照；这是双进程 LAN 验证，不是编辑器 PIE |
| 旧地图导航回归 | 原 Foundry 的 `Tools/TestFPSAI.ps1 -Crowd` 通过；9 个人机活动，模拟 80 秒时 43 次击杀，`Saved/FPSFlowTests/AI-crowd.log` |
| 画面核对 | [套件预览入口](../../../ArtSource/FPSDemo/Environment/CrossingKit/README.md) 提供 Blender 源模型预览、Unreal 俯视及四个地面视角、HUD 同屏图；已检查工业配色、门洞、明暗区域及环境与当前角色/武器的同屏效果 |

上表中 `Tests/` 路径均相对于 `Saved/IndustrialCrossing/`。先前复测发现同向环线可能无交火，已修正巡逻路径后重新生成地图并完成上述验证；联机测试还修复了测试脚本在武器内容校验完成前尝试开局的时序问题，未放宽实际开局规则。

运行入口：

```powershell
powershell -ExecutionPolicy Bypass -File Tools/TestFPSIndustrialCrossing.ps1 -Visual
powershell -ExecutionPolicy Bypass -File Tools/TestFPSIndustrialCrossing.ps1 -LAN
```

构建、导入和地图生成日志位于 `Saved/IndustrialCrossing/`；每次实机测试使用 `Tests/<时间>/` 保存独立日志及模块哈希。测试覆盖出生净空、模式接入、巡逻连通、九个人机活动与计分，以及实际服务器/客户端的伤害、重生、结算与反馈复制。截图检查需覆盖俯视图和四个地面视角，不能以 Blender 预览代替。

2026-10-13 提交前在当前 Editor 模块重新运行 `Tools/TestFPSIndustrialCrossing.ps1`，`Tests/20261013-033712626/run.json` 为 passed。模块 SHA-256 为 `3032f991f15bea9a2ec97099d2e6ca2ef31d355d1b2c18ed1f7cb12450f53ba0`；10 个出生点、2 处基地、14 个武器台与全部巡逻连接通过，9 个人机均活动，模拟 85 秒时记录 28 次击杀且未越界。这是 NullRHI 单机专项复核，本轮未重做地图渲染、双端 LAN 或打包。

尚未完成用户视觉验收、十名真实客户端压力测试、完整自然五分钟对局、打包运行与专门性能分析。

返回 [FPS 文档索引](../README.md)；原地图历史背景见 [地图制作记录](../../FPSMaps.md)。

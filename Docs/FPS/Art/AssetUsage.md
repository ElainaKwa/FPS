# 工业风资产库使用说明

- 用途：当前使用说明
- 适用范围：`ArtSource/FPSDemo/Industrial` 与 `/Game/FPSDemo/Art/Industrial` 首批资源
- 状态：源文件与 Unreal 资产已保存；七枪、角色部件、双地图已接入，UI 已保存为 Widget Blueprint
- 最后更新：2026-10-12

## 1. 从哪里打开

本页描述仍位于 Industrial 的现有资产库，不表示已完成新目录规范迁移。新增与整理资产遵循 [资产组织规范](AssetOrganization.md)，已迁移样品入口和旧目录后续安排见 [源文件整理记录](SourceOrganization.md)。

在 Unreal 内容浏览器进入 `/Game/FPSDemo/Art/Industrial`，打开 `Maps/LV_IndustrialShowcase` 查看组合场景。该关卡用于资产展示，不是对局地图；枪械、环境与人物可以从对应文件夹拖入其他关卡。

实际游玩从现有行动终端进入竞技场或靶场，两者已引用本批美术。菜单与 HUD 主布局现已保存为六个可编辑 Widget Blueprint，游戏直接使用它们；样式和图片也有独立资产。编辑入口与当前验证见 [UI 资产说明](../UI/AssetUsage.md) 和 [样稿对齐记录](../UI/SampleMatch.md)。三维基础接入及此前验收包见 [接入记录](Integration.md)。

| 内容 | 可编辑源文件 | Unreal 子目录 |
| --- | --- | --- |
| 七种枪械 | [WeaponLibrary.blend](../../../ArtSource/FPSDemo/Industrial/WeaponLibrary.blend) | `Weapons` |
| 十二种环境模块 | [EnvironmentKit.blend](../../../ArtSource/FPSDemo/Industrial/EnvironmentKit.blend) | `Environment` |
| 工业装甲机器人与手臂 | [Operator.blend](../../../ArtSource/FPSDemo/Industrial/Operator.blend) | `Character` |
| 程序关节运行时部件 | [RuntimeParts.blend](../../../ArtSource/FPSDemo/Industrial/RuntimeParts.blend) | `Character/Runtime` |
| 七张透明武器剪影 | [PNG 源目录](../../../ArtSource/FPSDemo/Industrial/Icons/) | `UI` |
| 六个 UI 布局、主题与细节纹理 | [UI 资产说明](../UI/AssetUsage.md)、[图片源目录](../../../ArtSource/FPSDemo/Industrial/UI/) | `UI/Widgets`、`UI` |
| 正式 MiSans 中文与 Rajdhani 英文/数字字体 | [字体原文件与许可](../../../ArtSource/FPSDemo/Industrial/UI/Fonts/)，使用约定以 [UI 资产说明](../UI/AssetUsage.md) 为准 | `UI/Fonts` |
| 公共材质 | Blender 文件中的同名材质；[参数清单](../../../ArtSource/FPSDemo/Industrial/manifest.json) | `Materials` |

本机 Blender 安装入口为 `D:\Blender\blender.exe`，制作版本为 5.2.2 LTS。每个 `.blend` 保留独立物件、集合及倒角修改器，可以直接打开编辑；FBX 是用于交换与导入的网格导出结果。

## 2. 资源约定

- 文件名前缀：`SM_IND_` 静态网格、`SK_IND_` 骨骼网格、`MI_IND_` 材质实例、`T_IND_` UI 图片。
- Blender 以米制作，Unreal 以厘米使用。枪械长轴为 X、枪口指向 +X；环境 Z 向上。人物源模型正面朝 Blender 的 -Y，导入后为 Unreal 的 +Y，展示关卡已旋转人物面向镜头；挂到游戏角色时需要显式校准朝向，不能假设它沿角色 +X。
- 地板和墙模块以 4 米网格为主，柱、栏杆、货箱等按实际尺寸制作。相对导出原点的包围盒记录在 `manifest.json`；导入检查逐轴比对 Blender 米制包围盒与 Unreal 厘米制包围盒。
- 环境使用复杂碰撞作为简单碰撞，保留门洞和栏杆空隙，适合静态摆放。它不是可模拟刚体的通用碰撞方案，也不等于已经通过玩家通行、导航或射线玩法验收。
- `M_IND_Surface` 的 `BaseColor`、`Metallic`、`Roughness`、`Emissive` 控制共同材质；实例区分石墨、钢、聚合物、陶瓷、混凝土、黄色、橡胶、织物、面罩、蓝、红和灯光。首批采用材质分块，未制作磨损贴图。
- UI 图标为 512×256 透明 PNG，与同批枪械模型对应；导入资源使用 UI 纹理组。它们已绑定配装与 HUD，显示保持等比。

## 3. 人物与接入限制

`SM_IND_Operator` 用于静态展示；`SK_IND_Operator` 带 16 根骨骼、Skeleton 和自动生成的 PhysicsAsset。首批是工业装甲训练机器人基型，按硬质分件绑定权重；没有动画片段、AnimBP、IK 或已验证的布娃娃效果。不能直接宣称兼容 UE 标准人体动画。

`SM_IND_FPArm_L/R` 是左右前臂与手套资源，保持角色源坐标。它们不是旧运行时按段缩放的直接替代物；持枪位置、握持、开镜、换弹、遮屏和穿插需要在接入时调整。

游戏实际使用 `Character/Runtime/SM_IND_RT_*`：派生部件已将前向和关节原点调整到程序动作坐标，四肢长轴归一后由关节跨度驱动。不要直接把它们当成正常人体比例整体摆放；静态展示使用 `SM_IND_Operator`。

竞技场、靶场已应用工业材质和外观层；原教学关卡保留。七枪的外观引用与新 UI 已接入，配装立即发放规则保留。当前运行使用派生 `RuntimeParts.blend` 的 15 个独立部件，沿用原程序关节，不直接以展示用骨骼网格替换角色。验收与限制见 [游戏接入记录](Integration.md)。

## 4. 重建与维护

以下命令在项目根目录运行。生成器会重建本批同名 `.blend`、FBX 和预览；Unreal 导入脚本会重建本批材质与 `IND_` 前缀展示物件。**已有手工修改时先另存版本，或把修改合入生成器，再运行重建。** 不要一边把源模型当手工权威版本，一边无备份地运行生成器覆盖它。

```powershell
& 'D:\Blender\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/build_industrial_art.py
& 'D:\Blender\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/render_industrial_icons.py
& 'D:\Blender\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/validate_industrial_blender.py
powershell -ExecutionPolicy Bypass -File Tools/ImportIndustrialArt.ps1
powershell -ExecutionPolicy Bypass -File Tools/CaptureIndustrialArt.ps1
python Tools/record_industrial_delivery.py
```

可以向建模脚本传入 `-- weapons`、`-- environment` 或 `-- character`，只重建对应库。完整重建后应依次重新导出图标、验证和导入，不手改自动生成的清单与报告。

[导入入口](../../../Tools/ImportIndustrialArt.ps1) 使用 Unreal 5.8 的编辑器 Python 与 FBX 导入流程，将缓存、日志放在项目的 `Saved/IndustrialArt`；不要求 Unreal MCP 连接。它关闭该进程的 MCP 自动启动，不改全局编辑器设置。导入报告保存在 `Saved/IndustrialArt/ImportReport.json`；[交付记录生成器](../../../Tools/record_industrial_delivery.py) 将报告和文件哈希保存到源目录的 `DeliveryReport.json`，以便随资产保留证据。

真实 Unreal 截图通过 [CaptureIndustrialArt.ps1](../../../Tools/CaptureIndustrialArt.ps1) 启动编辑器，再执行 [capture_industrial_art.py](../../../Tools/capture_industrial_art.py)，需要图形渲染设备和着色器缓存，不能用 `-nullrhi`。Blender 三张预览与 Unreal 截图分别标注，不互相替代。

## 5. Blender MCP 交互连接

2026-10-11 已安装社区 [MCP for Blender](https://github.com/ahujasid/mcp-for-blender) 2.1.9（并非 Blender 官方插件）。插件安装于本机 Blender 5.2 用户插件目录，已启用并保存偏好；默认打开 Blender 时自动启动本机 `127.0.0.1:9876` 连接。关闭 Blender 后连接自然不可用。

项目 [.codex/config.toml](../../../.codex/config.toml) 已增加 `blender` STDIO 服务，保留原 `unreal-mcp`；Python 环境位于 `.tools/blender-mcp/venv`，不随 Unreal `Saved` 缓存清理。设置了 `DISABLE_TELEMETRY=true`，插件内容遥测关闭，外部素材库及付费生成服务未启用。该连接可以执行 Blender Python，按当前任务范围操作和保存模型。

配置写入后需让 Codex 重新加载 MCP：在设置中的 MCP servers 对该服务选择 Restart；若当前会话仍未出现工具，重启 Codex 后重新进入项目。客户端配置方式见 [OpenAI MCP 说明](https://learn.chatgpt.com/docs/extend/mcp?surface=cli)。Blender 侧可以在三维视图侧栏的 MCP 面板检查或手动启动服务。

连接验证使用 [check_blender_mcp.py](../../../Tools/check_blender_mcp.py)，经过真实 MCP STDIO 初始化、工具列表、插件版本握手、场景读取及临时网格创建/修改/删除。2026-10-11 最终路径验证通过，读取枪械源场景 473 个对象，源 `.blend` 的 SHA-256 未改变；结果位于 `Saved/BlenderMCP/ConnectionReport.json`。这验证了 MCP 服务链路，不表示旧聊天的原生工具列表已自动刷新。

```powershell
& '.tools/blender-mcp/venv/Scripts/python.exe' Tools/check_blender_mcp.py
```

返回 [统一美术索引](README.md)。

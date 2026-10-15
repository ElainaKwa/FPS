# M7 Unreal 资产使用说明

- 用途：当前使用说明
- 适用范围：M7 模块化美术样品、Unreal Engine 5.8 美术导入与配件分类
- 状态：UE 5.8.1 已导入并保存 22 个静态网格、57 张贴图和 24 个材质资源，保存后新编辑器进程回读与实际引擎渲染检查通过；未接入游戏，未完成用户视觉验收
- 最后更新：2026-10-15

## 1. 范围与当前入口

本次按用户要求将已制作的 M7 枪身、独立配件和三套整枪预设作为 Unreal 美术资产导入，枪械按类别与型号分类，配件独立按部位分类。当前为静态网格样品，不包含开火逻辑、伤害配置、动画或骨骼绑定；导入状态与游戏接入、用户视觉验收分别记录。

制作源仍由 [M7 样品目录](../../../ArtSource/FPSDemo/Samples/Weapons/M7/README.md) 的 `Source/M7_Material_Study.blend` 统一持有。本次不拆分多资产源库，不重建已冻结的堤风枪管，也不搬迁其他枪械资产。归类依据见 [美术资产规范](AssetOrganization.md) 第 4.2 节。

当前导入入口为 [ue_import_m7.py](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Scripts/ue_import_m7.py)，材质入口为 [ue_m7_materials.py](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Scripts/ue_m7_materials.py)。它们使用 Unreal Python 编辑器 API，派生 `.uasset`；源网格与 PNG 不由导入器写回。2026-10-15 的 [UnrealImportReport.json](../../../ArtSource/FPSDemo/Samples/Weapons/M7/UnrealImportReport.json) 记录完整导入和保存成功；引擎版本为 `5.8.1-56057345+++UE5+Release-5.8`。

## 2. MCP 操作入口

当前编辑器已通过 [ue_m7_mcp_tools.py](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Scripts/ue_m7_mcp_tools.py) 向 Unreal 的 `ToolsetRegistry` 注册本资产库专用的 `M7ArtLibraryTools`。此入口是在项目中新增的固定功能工具集，不表示原有 Unreal MCP 工具包含任意 Python 执行能力。注册脚本由编辑器的 `-ExecutePythonScript` 入口加载；注册本身不会启动导入。

| 工具操作 | 功能与前置条件 |
| --- | --- |
| `import_library` | 按固定清单导入 M7 组件、贴图、材质和整枪预设；仅能写入本库既定目录，遇到同名且不属于本任务的资产即停止 |
| `verify_library` | 只读核对网格、材质引用、纹理、尺度和挂点，输出 `UnrealVerification.json`；不修改资产 |
| `build_preview` | 完整导入成功后，创建本型号的专属检查地图；不修改玩法地图 |
| `capture_preview` | 对本次检查地图安排实际 Unreal 渲染截图，输出本机 `Saved/M7UnrealImport/Previews/` 证据；需要启用渲染的编辑器，不能使用 `-nullrhi` |

以上工具不接收外部文件路径或任意代码参数，作用范围固定为当前 M7 库。重新运行导入会按当前清单更新本任务持有的资源，不能用于覆盖其他资产。也可用 Unreal Python commandlet 直接运行导入脚本，但渲染预览必须由有渲染能力的编辑器执行。

完整导入耗时可能超过 MCP 客户端单次等待时间。若调用超时，先检查编辑器是否仍在工作、导入报告及日志中的 `M7_UE_IMPORT_COMPLETE`，再决定是否重试；客户端超时本身不能判定后台导入失败。

同一编辑器中调用验证可检查当前加载内容，不能代替重新打开项目后的持久化验证。[ue_verify_m7.py](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Scripts/ue_verify_m7.py) 另行区分导入后新进程回读；需要此项证据时在新编辑器进程运行，并保留报告中的进程验证状态。

## 3. 内容浏览器分类

以下路径以 Unreal `/Game/` 为根，对应项目 `Content/`。已导入 19 个独立部件和 3 个整枪预设，共 22 个静态网格，并通过保存后新编辑器进程回读。

| 内容 | 路径 | 已导入数量 |
| --- | --- | --- |
| M7 枪身 | `/Game/FPSDemo/Art/Weapons/Rifles/M7/Meshes/` | 1 |
| 默认、满改堤风、满改标准整枪 | `/Game/FPSDemo/Art/Weapons/Rifles/M7/Presets/` | 3 |
| 后托 | `/Game/FPSDemo/Art/Weapons/Attachments/Stocks/M7/Meshes/` | 3 |
| 弹匣 | `/Game/FPSDemo/Art/Weapons/Attachments/Magazines/M7/Meshes/` | 2 |
| 后握把 | `/Game/FPSDemo/Art/Weapons/Attachments/RearGrips/M7/Meshes/` | 3 |
| 瞄具 | `/Game/FPSDemo/Art/Weapons/Attachments/Optics/M7/Meshes/` | 2 |
| 前握把 | `/Game/FPSDemo/Art/Weapons/Attachments/Foregrips/M7/Meshes/` | 2 |
| 枪口配件 | `/Game/FPSDemo/Art/Weapons/Attachments/Muzzles/M7/Meshes/` | 3 |
| 机械瞄具 | `/Game/FPSDemo/Art/Weapons/Attachments/Sights/M7/Meshes/` | 1 |
| 枪管／护木组件 | `/Game/FPSDemo/Art/Weapons/Attachments/Barrels/M7/Meshes/` | 2 |

每个型号或配件目录持有自己的 `Materials/` 与 `Textures/`。同一部件的 BaseColor、Normal、ORM 烘焙图由该部件的材质实例引用，整枪预设引用这些已有材质，不复制整套贴图。

跨项目迁移完整库时，选择 `Rifles/M7` 与表中各 `Attachments/<部位>/M7` 目录中的资产，并保留 Unreal Migrate 列出的依赖。只迁移一个合并整枪预设会带上其材质依赖，不会自动包含全部独立配件网格和变体。源文件与制作工具仍需另行携带样品源目录。

## 4. 材质与装配约定

已导入 57 张专属烘焙贴图和 24 个材质资源，其中有 19 个材质实例、1 个 PBR 主材质、4 个共用常量材质。BaseColor 采用 sRGB；Normal 与 ORM 采用线性空间，ORM 的 R、G、B 分别为环境遮蔽、粗糙度、金属度。源 Normal PNG 为 OpenGL +Y，Unreal 纹理设置反转绿色通道；源图片保持原字节。

共用主材质 `M_M7_BakedPBR` 和确有复用的黑色凹槽、刻字、玻璃、镀膜镜片材质由 `Rifles/M7/Materials/` 持有。跨文件夹迁移配件时，应使用 Unreal 的 Migrate 并保留其材质、主材质及贴图依赖。

导入目标为厘米、枪口朝 +X、Z 向上。枪身与枪管组件建立对应 Socket，配件网格原点与安装点对齐。前握把和枪口配件随所选枪管的挂点装配；标准枪管与堤风组件的挂点不同，不使用同一世界坐标强行对齐。整套机械瞄具目前按默认枪管配套，不能从已有 Socket 推断它已兼容所有枪管。

满改堤风和满改标准预设使用影袭导轨后托与已加长的幻影后握把；默认整枪与既有配件继续保留。预设是方便摆放和观察的合并静态网格，独立配件仍从各部位目录取用。

## 5. 验证与使用边界

本次导入先用枪身核对 FBX 轴向与厘米尺度，再检查 22 个网格的包围盒、UV、材质槽覆盖与保存结果，以及纹理设置和 3 个带 Socket 的网格。[UnrealVerification.json](../../../ArtSource/FPSDemo/Samples/Weapons/M7/UnrealVerification.json) 记录 2026-10-15 在导入完成后新启动的编辑器进程中，通过 MCP `verify_library` 完成只读回读：22 个网格、57 张贴图、19 个材质实例、5 个其他材质、3 个带挂点网格通过，错误数为零，`fresh_process_verified=true`。这是 M7 专项资产验证，不代表全项目启动、打包或游戏功能通过。

专属检查地图已保存为 `/Game/FPSDemo/Art/Weapons/Rifles/M7/Maps/LV_M7Preview`。2026-10-15 通过 MCP `capture_preview` 完成四张实际 Unreal 渲染图，使用 UE 5.8.1、D3D11/SM5、长焦透视侧机位与灯箱；[截图报告](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Previews/UECaptureReport.json) 记录成功，场景与机位见 [UEPreviewScene.json](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Previews/UEPreviewScene.json)。

| 检查内容 | 实际 Unreal 截图 |
| --- | --- |
| 默认整枪 | [默认配置长焦侧视](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Previews/UE_M7_Default_Side.png) |
| 满改堤风 | [堤风配置长焦侧视](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Previews/UE_M7_Tidal_Side.png) |
| 满改标准 | [标准配置长焦侧视](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Previews/UE_M7_Standard_Side.png) |
| 机匣近景 | [机匣近景](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Previews/UE_M7_Receiver_Detail.png) |

四张均已检查轮廓、装配和黑色／深炭黑材质分层，金属与聚合物可辨。当前 [ue_preview_m7.py](../../../ArtSource/FPSDemo/Samples/Weapons/M7/Scripts/ue_preview_m7.py) 的长焦与灯箱配置已实际产图并完成本次渲染检查；用户最终美术验收仍待确认。

FBX 包含多边形，Unreal 三角化后的面数不总与 Blender 相同。[UEGeometryPreflight.json](../../../ArtSource/FPSDemo/Samples/Weapons/M7/UEGeometryPreflight.json) 按源 SHA 记录严格零面积、厘米 float32 精度下零面积／重合候选和近零面积统计；自动数量门禁只采用前两类强候选预算，不以近零面积阈值扩大许可。该门禁用于发现异常，不能证明每个三角面的拓扑或位置相同。

LPVO 超出上述数量预算，另经 [UESurfaceReview.json](../../../ArtSource/FPSDemo/Samples/Weapons/M7/UESurfaceReview.json) 对 Unreal LOD0 导出与源 FBX 复核面积和尺寸。其总表面积相对误差约 `1.65e-7`，低于 `1e-4` 门限，尺寸误差低于 `0.01 cm`；报告绑定本次导入报告 SHA。这是总表面积与尺寸检查，不证明逐面对应、拓扑、法线或 UV 插值完全一致。

源库的 `Validation.json` 描述 Blender 烘焙、导出和独立包阶段。本次当前 `.blend` 与旧验证源文件的字节哈希不同；[SourceContentComparison.json](../../../ArtSource/FPSDemo/Samples/Weapons/M7/SourceContentComparison.json) 保存对旧归档源和当前源的只读比较，网格、UV、修改器、材质、贴图与场景装配的已检查内容一致。新的 Unreal 导入报告独立记录实际输入哈希，不把旧验证报告改写为本次结果。

这些模型保留制作精度，尚未完成运行时 LOD、碰撞、骨骼和动画制作，也未做游戏性能验收。已有运行时武器和玩法地图不由本次美术导入替换。

返回 [统一美术索引](README.md)。

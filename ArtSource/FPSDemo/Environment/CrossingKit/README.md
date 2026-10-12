# CrossingKit 工业关隘环境套件

- 当前制作方式：程序生成；`Scripts/build_crossing_kit.py` 及项目共享建模工具 `Tools/build_industrial_art.py` 为权威来源。
- 状态：五个模块已生成并导入 Unreal，实际尺寸检查通过；新地图实机与双端 LAN 验证通过，视觉验收待用户确认。
- Unreal 位置：`/Game/FPSDemo/Art/Environment/CrossingKit/Meshes/`。
- 地图位置：`/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing`。

## 文件与依赖

| 位置 | 内容 |
| --- | --- |
| `Source/CrossingKit.blend` | 可编辑分件、倒角与法线修改器；模块在展示布局中分开放置 |
| `Exports/` | 五个 FBX：门架、货箱、百叶、管线、防撞柱；导出原点为放置原点，单位米 |
| `Previews/BlenderKit.png` | Blender 套件渲染，不作为 Unreal 实机证据 |
| `Previews/UnrealMap*.png` | Unreal 实机：俯视图、长道、装卸区、维护通道、中门，1600 × 900 |
| `Previews/UnrealHUD.png` | 当前场景与持枪/HUD 同屏的 Unreal 实机截图 |
| `Scripts/` | 生成、导入和交付记录脚本 |
| `manifest.json` | 生成器导出的尺寸、三角面、材质槽与 UV 信息，禁止手工维护 |
| `DeliveryReport.json` | 当前文件哈希、导入记录与两次已通过实机验证回执；由 `Scripts/record_delivery.py` 从实际文件和运行日志生成，禁止手工维护 |

模型遵循灰色混凝土、石墨涂装金属、裸露钢和少量工业黄的既有方向。没有外部贴图或 Valve 资产。FBX 使用 `MI_IND_*` 材质槽；Unreal 引用现有 `/Game/FPSDemo/Art/Industrial/Materials/` 材质，未复制另一套共用材质。其当前源数据位于 `ArtSource/FPSDemo/Industrial/manifest.json`；该旧目录尚未迁移，不能当作规范目标结构已落地。

静态门架和管线采用复杂碰撞作为简单碰撞，门洞保持开放；不作为物理模拟刚体。主地图通过引用使用套件，玩法代码不归美术源目录。

## 重建

在项目根执行；重建会更新本套件同名文件，手工精修后须先按规范切换权威来源，不能直接覆盖精修内容。

```powershell
& 'D:\Blender\blender.exe' --background --factory-startup --python-exit-code 1 --python ArtSource/FPSDemo/Environment/CrossingKit/Scripts/build_crossing_kit.py
```

通过 Unreal Editor Python 执行 `Scripts/import_crossing_kit.py`，关闭 Interchange FBX（`Interchange.FeatureFlags.Import.FBX=0`），使用项目已有的 FBX 导入流程。导入报告写入 `Saved/IndustrialCrossing/KitImport.json`，检查五个网格的厘米尺寸与源清单一致。

地图权威布局是 [地图生成器](../../../../Tools/create_fps_industrial_crossing.py)，默认拒绝覆盖已有地图；交给手工编辑后不要传入重建参数。玩法与实机证据见 [地图记录](../../../../Docs/FPS/Maps/IndustrialCrossing.md)。

跨项目复用时在 Unreal 中对所需网格执行 Migrate，并保留其共用材质依赖。继续制作还需携带本目录、共享建模工具及现有工业材质源清单；Migrate 不会自动复制这些项目源文件。遵循 [资产存放与复用规范](../../../../Docs/FPS/Art/AssetOrganization.md)。

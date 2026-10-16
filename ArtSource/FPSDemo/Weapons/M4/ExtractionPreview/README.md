# M4 实验改枪台预览

本目录最初供搜打撤实验的中央 3D 改枪台使用。枪身、默认枪口与弹匣派生自 `ArtSource/FPSDemo/Industrial/WeaponLibrary.blend` 的现有 M4；瞄具、制退器、扩容弹匣是原型几何。2026-10-16 起这些独立配件也由保存的定义用于实战外观；原库保持不变，正式外观后续可替换。

- `Scripts/build_preview.py`：使用独立 Blender 后台进程运行，依赖项目 `Tools/build_industrial_art.py`，导出六个 FBX、派生 Blender 工程和 `Manifest.json`。Blender 5.2.2 LTS 已执行。
- `Source/M4_ExtractionPreview.blend`：可检查的派生模块模型；重生成仍以脚本和原始库为权威。
- `Exports/`：对应 Unreal `/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview` 六个静态网格。
- `Scripts/import_preview.py`：在 Unreal Python commandlet 中运行；只创建缺失模型并映射既有 `Art/Industrial/Materials/MI_IND_*` 材质，不导入材质副本。校验尺寸和所有其他资产哈希，记录到 `Saved/FPSExtraction/WeaponPreview/`。
- `Manifest.json` 是生成资料，禁止手工修改；记录原库哈希、模型边界、材质及生成入口。

运行时 `FPSExtractionWeaponViewport` 根据已保存的 M4 配件定义组合这些网格，SceneCapture2D 渲染至界面。实战接入由 [模块化枪身导入脚本](../../Rifles/M4/Modular/Scripts/import_gameplay_bodies.py) 设置第一人称、第三人称和掉落外观引用；历史 Manifest 的 preview scope 记录首次导出用途，不代表当前游戏接入限制。

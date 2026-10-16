# M4 模块化原型

权威输入为 `Tools/build_industrial_art.py` 的现有 M4 形体和本目录 `Scripts/build_modular.py`；生成器在隔离 Blender 后台进程内派生，不保存原 Industrial 库。`Source/M4_Modular.blend` 是可检查的派生源，Exports 和 Manifest.json 从生成器重新导出，禁止手工修改清单。

模型包括可替换后握把、枪托、枪管各两种及原装部件，枪身分为正常、分件换弹和改枪台三种装配底座。制作与接入状态见 [实施记录](../../../../../../Docs/FPS/Weapons/ModularParts.md)。这是现有工业 M4 的功能原型，未替换精细 M4 样品或 M7。

Unreal 枪身归 `Art/Weapons/Rifles/M4/Modular/Meshes`；独立部件归 `Art/Weapons/Attachments/{RearGrips,Stocks,Barrels}/M4/Meshes`，共用现有工业材质。`Scripts/import_modular.py` 通过编辑器导入并维护保存的定义引用。六个配置初始值只在创建资产时使用，之后由资产／策划工作台维护，重复迁移保留已有调参。

2026-10-16 实战枪口／弹匣拆分使用 `Scripts/build_gameplay_bodies.py`，在独立 Blender 后台进程生成 `Source/M4_GameplayBodies.blend`、两个 Gameplay FBX 和 `GameplayBodiesManifest.json`。原三种底座保留作历史资产；当前 M4 定义使用 GameplayBody／GameplayReloadBody，全部可替换部件独立装配。新增 `Scripts/import_gameplay_bodies.py` 通过 Unreal 编辑器导入、登记资源版本并保存定义，复用既有 ExtractionPreview 的瞄具、枪口与弹匣，不修改调参或其他武器。先执行原模块导入，再执行 gameplay 导入；回收行动的运行证据见 [实验实施记录](../../../../../../Docs/FPS/Extraction/Implementation.md#西侧出生东侧撤离与实战配件2026-10-16)。

# 弹药补给箱

- 制作方式：程序生成，权威来源为 [Scripts/build_crate.py](Scripts/build_crate.py)。手工精修前应先切换权威来源，避免重建覆盖。
- 版本：`AmmoSupplyCrate-20261012-v1`。
- 源文件：[Source/AmmoSupplyCrate.blend](Source/AmmoSupplyCrate.blend)，可编辑分件，集合已标记为 Blender 资产。
- 交换文件：[Exports/SM_AmmoSupplyCrate.fbx](Exports/SM_AmmoSupplyCrate.fbx)。
- Unreal：`/Game/FPSDemo/Art/Environment/AmmoSupplyCrate/Meshes/SM_AmmoSupplyCrate`，拖入关卡即创建静态网格 Actor。
- 验证关卡：`/Game/FPSDemo/Art/Environment/AmmoSupplyCrate/Maps/LV_AmmoSupplyCratePreview`。
- 用途：可摆放的闭合箱体外观，无补弹逻辑、开箱动画或内部弹药模型；视觉评审不由文件保存状态替代。

采用石墨灰箱体、工业黄锁扣与识别条、金属提手、橡胶护角和中英文补给标识。原点位于底部中心；Blender 正面朝 -Y，FBX 左右手坐标转换后 Unreal 正面朝 +Y。源文件单位为米，Unreal 按厘米导入。

Unreal 六个专属材质实例复用现有 `/Game/FPSDemo/Art/Industrial/Materials/M_IND_Surface`，未修改该公共母材质。跨项目复用使用 Migrate 并保留此依赖。字体从项目现有 MiSans Bold 和 Rajdhani Bold 读取并打包进 Blender，FBX/Unreal 中标识已转为网格，不需要运行时字体；许可随 [FontLicenses](Source/FontLicenses/) 保存。

完整使用和验证状态见 [制作与验证记录](../../../../Docs/FPS/Art/AmmoSupplyCrate.md)。当前清单和检查结果分别为 [AssetManifest.json](AssetManifest.json) 与 [UnrealValidation.json](UnrealValidation.json)，由脚本生成，不手工修改。

实际预览：[Blender 整体渲染](Previews/Hero.png)、[Unreal 展示关卡渲染](Previews/UnrealPlacement.png)。导入及放置验证通过；当前 Capture 命令可生成截图，但进程结束阶段存在非零退出异常，详情见上述验证记录。

从项目根执行：

```powershell
& 'D:/Blender/blender.exe' --background --factory-startup --python-exit-code 1 --python ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Scripts/build_crate.py
& 'D:/Blender/blender.exe' --background ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Source/AmmoSupplyCrate.blend --python-exit-code 1 --python ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Scripts/render_crate.py
& 'ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Scripts/UnrealAsset.ps1' -Action Import
& 'ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Scripts/UnrealAsset.ps1' -Action Capture
```

导入脚本只更新本资产的目标目录与专用验证关卡；重新执行会重建该关卡中 `AMC_` 前缀的展示对象。正式使用时将资产拖入需要的关卡，不在验证关卡里保存需要长期保留的玩法编辑。

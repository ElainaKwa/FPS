# Industrial 武器换弹部件

这是现有七把 Industrial Demo 武器的派生装配，不是新的整枪美术权威，也不修改 Samples 中的 M4/M7。

- 形状权威：项目根 `Tools/build_industrial_art.py`；分件规则：`Scripts/export_reload_parts.py`。
- `Source/*.blend` 是可检查的派生分件快照；修改原有外形应回到形状生成器再导出，避免维护两份外形。
- `Exports/*.fbx` 和 `manifest.json` 自动生成。所有部件保持同一枪械原点；源文件米，Unreal 厘米，X 向前、Z 向上。
- 与原整枪相同的 FBX 导入会转换坐标手性：Unreal 的 Y 对应源文件的负 Y。清单分别记录源边界与引擎边界，手部接触点采用引擎坐标。
- 长枪拆为枪身、弹匣、拉机柄；手枪拆为枪架、弹匣、套筒。手枪补出原握把内不可见的弹匣匣体。
- 使用项目根 `Tools/ImportFPSReloadRig.ps1` 导入、设置材质与 `UFPSWeaponReloadRig`、刷新武器资源签名并重新导出清单。需要先编译包含此类的 Editor 模块。
- 枪身与拉机柄/套筒进入 `Content/FPSDemo/Art/Weapons/{Rifles,SMGs,Pistols}/<型号>/Meshes`，弹匣进入 `Weapons/Attachments/Magazines/<型号>/Meshes`。
- 原 Industrial 整枪继续用于拾取展示；UserData 硬引用装配部件，运行时第一/第三人称使用分件，不重复显示整枪弹匣。

后台导出命令（从项目根执行）：

```powershell
& D:/Blender/blender.exe --background --python ArtSource/FPSDemo/Weapons/IndustrialSet/Reload/Scripts/export_reload_parts.py
& Tools/ImportFPSReloadRig.ps1
```

导入证据输出到 `Saved/FPSAnimationTests/ReloadImport.json`，动画与取消动作测试入口为 `Tools/TestFPSAnimation.ps1`。

实现和验证边界见 [动作与分件换弹记录](../../../../../Docs/FPS/Weapons/ProceduralAnimation.md)。

# 弹药补给箱制作与验证

- 用途：实施与验证记录
- 适用范围：FPS 美术 / 可在 Unreal 编辑器中摆放的静态弹药补给箱
- 状态：源文件与 Unreal 资产已保存；碰撞、LOD、放置、保存回读及实际渲染已核对；截图进程退出异常见验证边界
- 最后更新：2026-10-12

## 1. 范围与造型

用户要求按项目当前美术风格制作弹药补给箱，保存为可在编辑器中放置的资产。版本为 `AmmoSupplyCrate-20261012-v1`，按 [资产组织规范](AssetOrganization.md) 保存于 `Environment/AmmoSupplyCrate`，作为本次直接制作的环境道具，而非复用 M4 样品目录。

造型采用石墨灰硬壳箱、工业黄锁扣及识别条、金属提手、橡胶护角和连续箱盖接缝，配有 AMMO、弹药补给、运输编号和弹药图形。侧面提手、前部锁扣、背面铰链和顶部叠放条均为实际几何。

箱体为闭合静态外观；本次未制作补弹功能、开箱动画或箱内弹药，也未向已有对局地图投放道具。

## 2. 资产入口与使用

在 Unreal 内容浏览器打开 `/Game/FPSDemo/Art/Environment/AmmoSupplyCrate/Meshes`，将 `SM_AmmoSupplyCrate` 拖入关卡。它会作为 StaticMeshActor 放置；原点在箱底中心，旋转 Z 轴调整方向。Blender 正面标识朝本地 -Y，经 FBX 左右手坐标转换后，Unreal 正面朝本地 +Y。

独立展示关卡为 `/Game/FPSDemo/Art/Environment/AmmoSupplyCrate/Maps/LV_AmmoSupplyCratePreview`。

| 内容 | 文件或目录 |
| --- | --- |
| 可编辑源资产 | [AmmoSupplyCrate.blend](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Source/AmmoSupplyCrate.blend) |
| 导出网格 | [SM_AmmoSupplyCrate.fbx](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Exports/SM_AmmoSupplyCrate.fbx) |
| Blender 整体预览 | [Hero.png](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Previews/Hero.png) |
| Blender 正面与背面 | [Front.png](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Previews/Front.png)、[Rear.png](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Previews/Rear.png) |
| Unreal 展示关卡渲染 | [UnrealPlacement.png](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/Previews/UnrealPlacement.png) |
| 制作清单 | [AssetManifest.json](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/AssetManifest.json)，生成器维护 |
| Unreal 核验 | [UnrealValidation.json](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/UnrealValidation.json)，导入脚本维护 |
| 重建入口 | [资产 README](../../../ArtSource/FPSDemo/Environment/AmmoSupplyCrate/README.md)，包含建模、渲染、导入与截图命令 |

源文件包含 158 个可编辑分件，导出为单个静态网格，LOD0 为 29,894 个三角面、六个材质槽。包含提手和锁扣的外包围尺寸约为 78.8 × 52.55 × 42.95 厘米，单位转换以导入核验为准。

材质实例位于同资产目录的 Materials 中；公共母材质依赖现有 `/Game/FPSDemo/Art/Industrial/Materials/M_IND_Surface`。按依赖迁移，不复制修改公共母材质。文字已转为几何，游戏显示不依赖用户电脑安装字体；Blender 中打包了制作字体，其许可与来源见资产 README。

## 3. 验证与边界

Blender 5.2.2 LTS 已保存源文件并重新打开，输出三张 1920×1440 实际模型渲染，检查主要轮廓、提手/锁扣连接、中英文标识和底部落地点。未使用生成示意图作为模型完成证据。

Unreal 5.8 中已核验：六个材质槽正确绑定；包围尺寸与源文件一致，底面 Z 为 0；包含一个简单盒体碰撞，摆放后简单碰撞射线命中；三级 LOD 顶点数逐级减少；两组 UV 中第二组为独立展开的光照 UV。实际创建 StaticMeshActor，验证旋转和缩放，并保存、重新打开专用关卡核对网格引用。

2026-10-12 在离屏 Unreal 编辑器中重新打开已保存的展示关卡，输出 1600×1200 正面实际渲染，人工核对中英文标识、锁扣、提手、材质和底部落点。展示相机已按导入后的 +Y 正面校准。源文件和 FBX 的 SHA-256 与制作清单一致。

验证边界：导入命令正常退出，`UnrealValidation.json` 为 `success: true`。正面截图文件已生成并人工查看，日志出现 `AMMO_CRATE_CAPTURE_PASS`；但两次正面截图进程均在结束阶段返回 `-1073741819`（`0xC0000005`），延迟退出后仍复现，因此截图命令整体不标记为通过。当前未定位这一退出异常的根因，也不把截图生成等同于编辑器稳定性验证。截图包装脚本保留非零退出错误，没有忽略失败。

日志保存在 `Saved/AmmoSupplyCrate/`，包括 `Import.log`、`Capture.log` 和前次退出异常的 `Capture-ExitFailure.log`。未运行全项目 Cook、多人同步或大量实例性能测试，这些不属于本次静态道具交付的已验证范围。文档检查 `python Tools/CheckDocs.py` 通过；整体视觉质量仍由用户评审。

返回 [统一美术索引](README.md)。

# M4 独立外观样品

- 状态：第三版外观样品，视觉待评审，未实装。
- 版本：`M4-Exterior-Study-20261012-v3`。
- 制作方式：程序生成；当前权威来源为 `Scripts/build_sample.py` 和 `Scripts/refine_shapes.py`。尚未转入手工精修。
- 源文件：[Source/M4_Precision_Study.blend](Source/M4_Precision_Study.blend)。
- 交换文件：[Exports/M4_Precision_Study.fbx](Exports/M4_Precision_Study.fbx)。
- 预览：[Previews/Compare.html](Previews/Compare.html)，第二版与第三版实际 Blender 渲染对照。
- Unreal 对应路径：无。不要替换现有游戏中的 M4。
- 依赖：Blender 5.2.2 LTS；材质为源文件内的程序材质，本样品无外置 Shared 贴图依赖。FBX 未携带等效烘焙纹理。

构建、渲染、导出和回读命令见 [样品实施记录](../../../../../Docs/FPS/Art/PrecisionSample.md)。生成器会覆盖当前样品；若进入手工精修，应先切换权威来源并修改生成器的输出位置。

2026-10-12 按 [资产规范](../../../../../Docs/FPS/Art/AssetOrganization.md) 迁入本目录，脚本移至 Scripts，当前 Blender 移至 Source。Revisions/v1 和 v2 内部布局与字节保持原样；它们是历史快照，查看或恢复时请复制到独立工作位置，不在快照中执行会覆盖文件的脚本。快照内旧对比 HTML 可能沿用当时相对路径，当前对照使用上述 Previews 入口。

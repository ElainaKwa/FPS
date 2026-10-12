# M4 精细外观样品

- 用途：实施与验证记录
- 适用范围：FPS 美术 / 独立 M4 外观样品，不包含角色与游戏接入
- 状态：第三版弹匣与枪托形体修订完成，源资产、六视角渲染与 FBX 回读已核验；视觉质量待用户评审，未达到参考图最终验收状态
- 最后更新：2026-10-12

## 范围与版本

当前版本为 `M4-Exterior-Study-20261012-v3`。用户先要求“试着做一下参考图级精细模型，做个样品出来，先不实装”，随后要求继续精修，因此本轮仍限于单把枪的独立外观试制，未恢复整套枪械、第一人称双手和人物的制作与接入。

以用户提供的工业竞技场 FPS 参考图为视觉目标，选择 M4 为代表。参考只提供一个主要视角，样品的背面、分件和比例为美术补充设计，不是对某款实物的精确复刻。第二版针对首版的厚板轮廓与反光过亮进行精修；材质仍主要依赖程序微表面，尚未完成手工纹理、丰富的使用痕迹与最终比例验收，不能以面数或渲染分辨率替代视觉验收。

## 第三版弹匣与枪托修订

- 用户指出弹匣过弯：保留上段插入位置，将上半段调整为近直轮廓，下半段仅作轻微前弯；压槽、边缘折线、编号与底板同步沿新曲线调整。
- 枪托改为较饱满的贴腮套筒和连续托体，缩小镂空，增加圆角背带槽、侧面加强层次及贴合曲面的饰板；调节片与托体相接，替代上一版视觉上悬空的薄片。
- 调整橡胶尾垫和背面纹路的贴合，增加枪托特写。原有整枪和侧面相机、摄影棚灯光保持不变，便于直接比较轮廓。
- 上一版完整源文件、导出、脚本、清单和预览保存在 `ArtSource/FPSDemo/Samples/Weapons/M4/Revisions/v2/`；见 [第二版源文件](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Revisions/v2/M4_Precision_Study.blend) 与 [第二版 / 第三版对照](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/Compare.html)。

## 第二版精修内容（历史记录）

- 机匣改用弧面顶部、平整侧面和收窄端部的截面结构，调整外部按钮的斜向轮廓，移除成排的亮色装饰划痕。
- 枪托重做为圆润套筒、后框与开放式支撑，增加独立尾垫和下方调节片，替代原先大块三角厚板。
- 握把改为连续掌形曲面；弹匣改为连续弯曲外壳及真实浅凹槽，去掉外贴细棒。
- 用户随后指出后握把偏细，并指定参考《三角洲行动》。参考 [M4A1 游戏截图所在图鉴](https://ol.3dmgame.com/gl/282319_6.html) 中后握把的视觉比例，进一步加宽掌部、加厚侧面、缩短下端，补充机匣连接过渡与前侧指托，防滑面改成贴合握把的曲面。仅参考外观，不采用攻略中的数值或配装建议，也未提取该游戏的模型或贴图。
- 用户进一步要求弹匣、枪体也参考该游戏。依据同一截图，收紧机匣的长高比例，重做下机匣浅凹面、局部加强边和小型字标，并重新对齐握把、枪托、弹匣与瞄具；弹匣改为较浅的压槽、横向压痕和独立的哑光金属涂层，去除可能悬空的边缘小点。具体外观比例由样品渲染判断，并非游戏原资产尺寸。
- 瞄具采用圆角外壳、与窗口形状匹配的镜片和密封圈；降低前方电池包，把后备用瞄具收为折叠轮廓，改善第一人称窗口可见区域。
- 降低聚合物与涂层的反射强度，增加克制的表面颜色和粗糙度变化，同时降低摄影棚后方补光。

第一版源文件、导出、生成器、清单与四张预览保存在 `ArtSource/FPSDemo/Samples/Weapons/M4/Revisions/v1/`，用于回退与对照。入口见 [第一版源文件](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Revisions/v1/M4_Precision_Study.blend)。第一版到第二版的相机保持相同，但灯光/材质也有变化；第一人称预览移除了摄影棚地面背景，不将这些差异全部归因于网格。

## 资产与预览

| 交付 | 入口与说明 |
| --- | --- |
| 可编辑源资产 | [M4_Precision_Study.blend](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Source/M4_Precision_Study.blend)，主集合 `M4_PRECISION_SAMPLE` 已标记为 Blender 资产 |
| 通用网格 | [M4_Precision_Study.fbx](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Exports/M4_Precision_Study.fbx)，仅模型分件，不包含摄影棚、相机或灯光 |
| 整枪预览 | [Hero.png](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/Hero.png) |
| 侧面预览 | [Profile.png](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/Profile.png) |
| 机匣近景 | [Receiver.png](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/Receiver.png) |
| 后握把近景 | [Grip.png](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/Grip.png)，核对加厚后的掌部、曲面防滑区域及机匣连接 |
| 枪托近景 | [Stock.png](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/Stock.png)，核对贴腮曲面、镂空、调节片与橡胶尾垫 |
| 第一人称构图 | [FirstPerson.png](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Previews/FirstPerson.png)，Blender 相机预览，不是 Unreal 实机截图，不含手臂 |
| 自动核验清单 | [AssetManifest.json](../../../ArtSource/FPSDemo/Samples/Weapons/M4/AssetManifest.json)，包含分件、面数、尺寸、哈希和 FBX 回读结果；由下述脚本生成，不手工编辑 |

模型包含护木实际开孔、分件导轨、瞄具窗口与镜片、枪口外观开孔、机匣开口和防尘盖、六角凹槽螺钉、枪托镂空、握把防滑纹、弹匣外观筋线与少量编号。材质区分金属、聚合物、橡胶与光学玻璃；摄影棚独立位于 `STUDIO_NOT_FOR_EXPORT` 集合。

第三版源文件保留 570 个网格分件以及曲线和字标，导出求值后为 625 个网格、208,276 个三角面，使用 11 种模型材质。第二版对应为 567 / 618 个网格、205,662 个三角面；第一版为 572 / 627 个网格、194,802 个三角面。历史核验清单保留在各自版本快照中。它是可编辑的高模样品，分件与面数不代表游戏性能预算。

## 验证与限制

- 在 Blender 5.2.2 LTS 中重新生成、保存并重新打开源文件；第三版 Cycles 输出六张 2560×1440 实际模型渲染，覆盖整枪、侧面、机匣、第一人称、后握把与枪托特写。第二版保留五视角，第一版保留原四视角。
- 第一版曾修正枪托/瞄具偏高、枪口布尔开孔、悬浮导轨标记及编号越界问题；第二版继续重塑主要形体，平滑机匣与瞄具曲面，并单独处理第一人称预览的背景。
- 第三版实际检查侧面与整枪比例，并通过枪托近景修正饰板与曲面的穿插、尾垫纹路的贴合；外观仍需用户确认。生成、渲染、导出与回读日志位于 `Saved/M4Precision/` 中的 `Build-v3.log`、`Render-v3.log`、`Export-v3.log` 和 `Verify-v3.log`。
- 导出前检查求值网格非空、坐标有限及源网格 UV 存在，并校验源文件内记录的形体生成器哈希与当前脚本一致；在独立 Blender 进程回读 FBX，分件数量、三角面及包围盒一致，尺寸误差低于 0.00001 米。
- v3 模型制作只处理独立样品、相关文档与 `Saved/M4Precision/` 日志，没有修改 Unreal 的 Content、代码或地图，也未覆盖原有枪械库或当前 M7 场景。样品当前位于 `ArtSource/FPSDemo/Samples/Weapons/M4/`；其后的目录整理属于另一次变更，见下述迁移记录。
- 完整程序材质以 Blender 源文件为准。FBX 带基础材质参数，尚未烘焙等效的 PBR 纹理；自动 UV 仅为后续制作起点，不是已人工整理的最终 UV。
- 未制作游戏低模、LOD、骨骼、动画、碰撞、握持点或 Unreal 导入验证；本轮不实装，不以源文件验证冒充游戏内验证。

## 重建入口

2026-10-12 按 [资产组织规范](AssetOrganization.md) 将样品迁至 `Samples/Weapons/M4`：当前 Blender 文件在 Source，制作脚本在 Scripts。模型与历史快照内容未变；迁移验证见 [源文件整理记录](SourceOrganization.md)。下列命令已改为当前路径。

从项目根运行，替换本机 Blender 路径即可。先完成上一条，再执行下一条。生成器会重建此样品，手工修改源模型后应先另存，以免重建覆盖。

```powershell
& 'D:/Blender/blender.exe' --background --factory-startup --python-exit-code 1 --python ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/build_sample.py
& 'D:/Blender/blender.exe' --background ArtSource/FPSDemo/Samples/Weapons/M4/Source/M4_Precision_Study.blend --python-exit-code 1 --python ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/render_sample.py
& 'D:/Blender/blender.exe' --background ArtSource/FPSDemo/Samples/Weapons/M4/Source/M4_Precision_Study.blend --python-exit-code 1 --python ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/export_sample.py
& 'D:/Blender/blender.exe' --background --factory-startup --python-exit-code 1 --python ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/verify_export.py
```

脚本分别负责 [建模](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/build_sample.py)、[形体精修](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/refine_shapes.py)、[渲染](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/render_sample.py)、[导出](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/export_sample.py) 和 [回读核验](../../../ArtSource/FPSDemo/Samples/Weapons/M4/Scripts/verify_export.py)。形体精修脚本由建模入口调用，不能独立运行。

后续范围见 [美术待办](Tasks.md)。返回 [统一美术索引](README.md)。

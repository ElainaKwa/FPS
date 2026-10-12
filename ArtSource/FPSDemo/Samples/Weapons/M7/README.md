# M7 真实材质试作

- 状态：未验收样品；本次已授权材质与几何试作，尚未完成用户视觉验收。
- 当前制作方式：程序生成阶段。
- 风格依据：[FPS 统一美术设计决策](../../../../../Docs/FPS/Art/DesignDecisions.md)，第 2、5 节。
- 当前模型入口：`Source/M7_Material_Study.blend`。
- 验证状态：当前几何已修订；最终贴图、导出和渲染核对以匹配当前源文件的 `Validation.json` 为准，尚未验证 Unreal 或完成用户视觉验收。
- Unreal 状态：未导入本次样品，未接入游戏；暂无对应 Unreal 资产路径。
- 动画状态：本次不制作动画或动画骨架。
- 最后更新：2026-10-12。

本目录承接 M7 的真实材质试作，并按用户最新截图重建堤风配件几何。材质沿用项目既有的克制工业风格：以黑白灰和低饱和材质本色为主，区分涂装金属、裸露五金、聚合物和橡胶，用细微表面颗粒与局部磨损辅助读形。避免大面积脏污、镜面反射和全身发光，统一边缘处理与细节密度，枪身不加入“MCP”测试代号刻字。

材质由本次制作流程生成，不包含从《三角洲行动》提取的原始材质或贴图，也不承诺与游戏原贴图一致。

## 目录与入口

以下为本次交接布局；文件是否已经产出及实际参数以生成脚本、配置和本次验证记录为准。

| 位置 | 用途 |
| --- | --- |
| `Source/M7_Material_Study.blend` | 本次当前材质样品，供打开、渲染和检查；由权威流程重建 |
| `Textures/Baked/` | 此 M7 样品专属的 BaseColor、Normal、ORM 烘焙 PNG；通道、颜色空间、分辨率和 UV 集由生成配置及清单注明 |
| `Scripts/` | 本样品的材质生成、堤风几何重建、烘焙、导出与检查脚本，以及 `material_settings.json` 等输入配置 |
| `Previews/` | 实际 Blender 渲染的外观前后对照；使用相同视角、灯光和显示设置，注明前后版本 |
| `Previews/Compare.html` | 四组同机位前后渲染的本地滑块对照入口；显示几何与材质的共同变化 |
| `Previews/Preview_Tidal_Nose_Detail.png` | Blender 真实渲染的前端轮廓单图近景，用于检查斜收与圆转过渡；不是前后图像合成 |
| `Previews/Preview_Tidal_Front_Profile.png` | Blender 枪口正视图，用于检查上半段三角肩、向窄顶轨台汇拢的两斜面及中段圆鼓 |
| `Previews/Preview_Equipped_Tidal_Side.png` | Blender 整枪正交侧视图，用于核对整体比例、护木长度及轮廓 |
| `Previews/Preview_Tidal_Nose_Side.png` | Blender 前端局部正交侧视图，用于核对前突弧度的局部微调 |
| `Revisions/GeometryV2/Source/M7_PreMaterial.blend` | 本次材质制作前的几何快照，作为只读重建输入 |
| `Reference/Tidal_User.png` | 用户提供的堤风近照，作为细节参照 |
| `Reference/Tidal_Side_User.png` | 用户追加的橙色侧视参照，校准整体比例和下侧高度 |
| `Reference/Tidal_Rail_Connection_User.png` | 用户追加的导轨连接参照，明确顶导轨中间断段须连接机匣 |
| `Reference/Tidal_Nose_User.png` | 用户追加的橙色前端裁切参照，校准上缘斜收、中部圆转和下缘后收的轮廓 |

如流程产生交换格式，它们由当前源结果导出，保存于本样品的 `Exports/`，不形成另一份独立编辑来源。只建立实际产出的目录。

本样品的材质与贴图均为 M7 专属资源，随本目录交接。尚未被多个资产实际共用，因此不放入 `Shared/`；以后产生实际共用依赖时，再明确其唯一维护位置并更新本入口。

## 权威来源与历史基线

`Scripts/` 中的生成脚本、配置，以及 `Revisions/GeometryV2/Source/M7_PreMaterial.blend`，共同构成本阶段的权威来源。堤风几何根据最新用户截图由 `Scripts/rebuild_tidal_reference.py` 重建。修改材质或几何时更新相应脚本或配置，重新生成当前 `.blend`、烘焙结果和相关导出。当前 `Source/` 模型是派生制作结果，不与生成流程并行维护两份当前材质或几何版本。

当前堤风完整重建的证据为 [TidalReferenceReview.json](TidalReferenceReview.json)。旧 `FrontRefinement.json` 仅描述重建之前的局部倒角调整，保留为过程记录，不代表当前堤风修法。用户截图作为视觉参照；本次按既有安装尺度估计比例与深度，未进行透视校正。

当前堤风取消上一轮后部低屋顶，顶导轨直接连接机匣导轨；横向与上侧高度按同一尺度重新映射，下侧高度按追加侧视图校准，并重建前端收斜轮廓、浅侧板和相关配件挂点。开孔贯通、导轨连接和曲面的技术检查以最新 `TidalReferenceReview.json` 为准，不将历史迭代的对象数或测量值视为当前版本的固定指标。这些检查不代表用户已接受外观，也不能沿用上一轮烘焙和导出的验证结果。

前端依据新增的 `Reference/Tidal_Nose_User.png` 修正连接轮廓，连续外凸弧面由参数曲面直接生成，弧顶圆润，保留已校准的孔位和顶导轨。最新要求加长的是护木／镂空枪管外壳：在后部与中部增加 `0.120 m` 长度，前端整体平移；从枪口正视，上方三角肩进一步收窄，两直斜面向窄顶轨台汇拢，收肩处有明确折线和坡度转折。仅中段圆鼓，并沿整段护木长度保持相同鼓度，下缘回收，侧长板和细纹跟随曲面。外露圆管恢复参考比例，挂点与配件随护木修订更新；最新制作方式、位置与几何检查记录以同一报告为准。前端近景原图见 [Preview_Tidal_Nose_Detail.png](Previews/Preview_Tidal_Nose_Detail.png)，上半段截面另由 `Previews/Preview_Tidal_Front_Profile.png` 枪口正视图核对。最终几何、烘焙和导出重新验证后，才能确认渲染与当前源文件一致。

前鼻连接去掉凹弧，改为连续外凸弧，弧顶圆润；截面、护木长度、沿全长一致的中段鼓度和收肩折线保持不变，整枪与前端局部的正交侧视图用于核对本次调整。

只读几何快照不写回，也不作为另一个当前可编辑版本。若后续转入手工精修，应明确记录转换时间与新的权威制作工程，并避免旧生成器覆盖精修源。

旧目录 [ArtSource/M7ReplicaV2](../../../../M7ReplicaV2/) 保留为本次材质试作之前的历史基线；其中的模型、导出、预览与旧报告不移动、不改写。它们的检查结果和生成日期属于当时版本，不能替代本次材质样品的验证。当前制作入口以本目录为准。

## 重建与核对

完整流程按顺序运行：`prepare_sample.py` → `build_materials.py` → `rebuild_tidal_reference.py` → `align_style.py` → `bake_atlases.py` → `export_library.py` → `render_sample.py` → `finalize_material.py` → `verify_library.py` → `package_sample.py`。`align_style.py` 校准统一灯光与功能标记；`render_sample.py` 对当前样品和只读基线使用同一灯光配方。

准备阶段由 `prepare_sample.py` 创建隔离样品并核对只读快照。当旧 `ArtSource/M7ReplicaV2` 存在时，同时核对旧几何；旧目录不存在时，使用包内 `Revisions/GeometryV2/Source/M7_PreMaterial.blend`，并按包内 `AssetManifest.json` 的 `baseline.sha256` 校验。两种几何来源都缺失，或已有基线哈希不符时停止，不覆盖基线，也不改写旧资产或已有当前源文件、清单。独立解压的交付包保留其清单、基线、脚本和配置即可重建。

运行 `build_materials.py` 前必须从只读基线开始，不能用已经重新赋材质的当前样品作为本步骤输入。此后堤风重建、烘焙、导出、渲染和收尾步骤使用 `Source/M7_Material_Study.blend`。烘焙和导出可在独立后台 Blender 进程离线运行，不需要 Unreal。

从项目根目录的 PowerShell 执行下列完整重建命令。独立包可从解压后的样品目录执行，将第一行改为 `$m7Sample = (Get-Location).Path`。`D:/Blender/blender.exe` 是本机已核对的入口；换机器时调整该变量。命令会重建本样品的当前派生 `.blend`、烘焙贴图、导出、渲染和报告，不写回旧几何或只读快照。固定生成版本的几何比例和曲线参数保存在脚本中，不依赖原项目旧目录或交付包未携带的参考图。

```powershell
$m7Sample = Join-Path (Get-Location) 'ArtSource/FPSDemo/Samples/Weapons/M7'
$m7Blender = 'D:/Blender/blender.exe'
$m7Baseline = Join-Path $m7Sample 'Revisions/GeometryV2/Source/M7_PreMaterial.blend'
$m7Current = Join-Path $m7Sample 'Source/M7_Material_Study.blend'
function Invoke-M7Step($m7Input, $m7Script, [string[]]$m7Extra = @()) {
    & $m7Blender --background --python-exit-code 1 $m7Input --python (Join-Path $m7Sample "Scripts/$m7Script") @m7Extra
    if ($LASTEXITCODE -ne 0) { throw "M7 step failed: $m7Script" }
}
python (Join-Path $m7Sample 'Scripts/prepare_sample.py')
if ($LASTEXITCODE -ne 0) { throw 'M7 sample preparation failed.' }
Invoke-M7Step $m7Baseline 'build_materials.py'
Invoke-M7Step $m7Current 'rebuild_tidal_reference.py'
Invoke-M7Step $m7Current 'align_style.py'
Invoke-M7Step $m7Current 'bake_atlases.py'
Invoke-M7Step $m7Current 'export_library.py'
Invoke-M7Step $m7Current 'render_sample.py'
$m7BeforeViews = @('--', '--baseline', 'Preview_Equipped_Tidal.png', 'Preview_Receiver_Magazine_Detail.png', 'Preview_ResonanceGen1_Detail.png', 'Preview_UpperRail_Detail.png')
Invoke-M7Step $m7Baseline 'render_sample.py' $m7BeforeViews
Invoke-M7Step $m7Current 'finalize_material.py'
& $m7Blender --background --factory-startup --python-exit-code 1 --python (Join-Path $m7Sample 'Scripts/verify_library.py')
if ($LASTEXITCODE -ne 0) { throw 'M7 verification failed.' }
python (Join-Path $m7Sample 'Scripts/package_sample.py')
if ($LASTEXITCODE -ne 0) { throw 'M7 packaging failed.' }
```

基线渲染命令中的 `-- --baseline` 只将输出改为 `Before_*.png`；该脚本不保存输入 `.blend`。普通当前样品渲染生成 `Preview_*.png`。两者均为 Blender 实际渲染，未通过图像编辑添加材质或结构。只需对照四个视角时，可给普通渲染步骤同样追加四个文件名筛选，但不带 `--baseline`。

Normal PNG 使用 OpenGL 切线空间 **+Y**。接入 Unreal 的 DirectX 法线工作流时须反转绿色通道，并在引擎中实际核对；本次尚未完成 Unreal 验证，不能把 Blender 预览当作引擎材质表现。

外观前后对照必须固定视角、灯光、曝光和显示设置；以实际渲染检查金属与非金属区分、粗糙度、贴图接缝和细节强度。本轮堤风几何也发生重建，因此全枪前后对照同时反映几何与材质变化，不能把全部差异归因于材质。若要单独评价材质，应对同一几何做材质对照。

对照入口为 [Previews/Compare.html](Previews/Compare.html)，包括堤风全枪、机匣与弹匣、共振一代握把、上导轨四组视图。`Before_*.png` 从只读基线用统一灯光重渲染；本轮上导轨近景相机目标由 `x=0.546 m` 改为 `x=0.480 m`，Before 与 After 使用新版机位成对渲染。`prepare_sample.py` 从旧目录补入的历史图只负责保留初始对照，完整流程会重新生成改前图。当前几何已修订，最终核对结果以匹配当前源文件的 [Validation.json](Validation.json) 为准；仅看已有图片不能证明其对应最新源版本。保存、烘焙或验证成功不等于视觉验收，本次仍需用户确认最终外观与项目风格的一致性。当前仍未导入或验证 Unreal。

交接时携带本目录的当前源结果、专属贴图、脚本与配置、只读重建基线、本 README，以及本次生成和验证记录。交付 ZIP 不包含 `Reference/` 下的用户截图；若需要在另一机器继续按图修形，应另行保留这些参照。旧目录不作为第二个当前制作入口。未完成 Unreal 导入与实际使用验证前，不宣称本样品已接入游戏。

归类依据：[美术资产存放与复用规范](../../../../../Docs/FPS/Art/AssetOrganization.md)；现有旧目录的迁移状态见 [源文件整理记录](../../../../../Docs/FPS/Art/SourceOrganization.md)。

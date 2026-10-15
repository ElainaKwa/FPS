# M7 模块化枪身与配件样品

- 状态：未验收样品；本次已授权材质与几何试作，尚未完成用户视觉验收。
- 当前制作方式：程序生成阶段。
- 风格依据：[FPS 统一美术设计决策](../../../../../Docs/FPS/Art/DesignDecisions.md)，第 2、5 节。
- 当前模型入口：`Source/M7_Material_Study.blend`。
- 验证状态：Blender 制作与导出记录保留于 `Validation.json`；本次 Unreal 导入保存、新编辑器进程回读和实际引擎渲染检查通过，尚未完成用户视觉验收。
- Unreal 状态：已导入 UE 5.8.1，含 19 个独立部件、3 个整枪静态网格预设、57 张贴图及 24 个材质资源；未接入游戏。
- 动画状态：本次不制作动画或动画骨架。
- 最后更新：2026-10-15。

本目录承接 M7 的几何与材质样品。本轮将现有堤风枪管组件作为独立可替换配件保存，并按用户机匣截图重做原枪枪身。材质沿用项目既有的克制工业风格：区分黑色涂装金属、裸露五金、聚合物和橡胶，用细微颗粒与局部接触磨损辅助读形。

材质由本次制作流程生成，不包含从《三角洲行动》提取的原始材质或贴图，也不承诺与游戏原贴图一致。

## 目录与入口

以下为本次交接布局；文件是否已经产出及实际参数以生成脚本、配置和本次验证记录为准。

| 位置 | 用途 |
| --- | --- |
| `Source/M7_Material_Study.blend` | 本次当前材质样品，供打开、渲染和检查；由权威流程重建 |
| `Textures/Baked/` | 此 M7 样品专属的 BaseColor、Normal、ORM 烘焙 PNG；通道、颜色空间、分辨率和 UV 集由生成配置及清单注明 |
| `Scripts/` | 本样品的枪身和堤风几何生成、材质生成、烘焙、冻结检查、导出与验证脚本 |
| `Previews/` | 实际 Blender 渲染的外观前后对照；使用相同视角、灯光和显示设置，注明前后版本 |
| `Previews/Compare.html` | 五组同机位前后渲染的本地滑块对照入口；枪身侧视为本轮开始前与本轮结果的对照 |
| `Previews/Preview_Core_Reference_Side.png` | 枪身、弹匣与后握把的正交侧视，便于对照用户截图 |
| `Previews/Preview_RearRail_Detail.png` | 机匣尾部导轨收口的正交侧视，检查末端与顶部支撑面的关系 |
| `AttachmentRegistry.json` | 已冻结堤风组件的原始网格、评估形体、UV、材质、安装点及独立导出/贴图指纹 |
| `CoreReferenceReview.json` | 本轮枪身构造、参考映射、固定接口与实际开孔检查记录 |
| `Reference/Core_Receiver_User.png` | 本轮用户提供的机匣参考，供视觉修形，不随 ZIP 分发 |
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

## Unreal 资源与分类

本次通过项目注册的 M7 专用 MCP 工具集执行导入。枪身位于 `/Game/FPSDemo/Art/Weapons/Rifles/M7/Meshes/`，三套整枪位于同一型号的 `Presets/`。配件独立归入 `/Game/FPSDemo/Art/Weapons/Attachments/<部位>/M7/`，部位为 `Stocks`、`Magazines`、`RearGrips`、`Optics`、`Foregrips`、`Muzzles`、`Sights`、`Barrels`；各自材质与贴图跟随对应目录。

[UnrealImportReport.json](UnrealImportReport.json) 记录实际保存的 22 个静态网格、57 张贴图、19 个材质实例、1 个 PBR 主材质和 4 个共用常量材质。枪身与两种枪管建立安装 Socket。完整目录、MCP 操作和验证边界见 [M7 Unreal 资产使用说明](../../../../../Docs/FPS/Art/M7UnrealImport.md)。源库继续由当前 `.blend` 统一持有，不因引擎分类拆出多份当前源文件。

[UnrealVerification.json](UnrealVerification.json) 记录 2026-10-15 新编辑器进程中的只读检查：22 个网格、57 张贴图、19 个材质实例、5 个其他材质和 3 个带挂点网格均通过，错误数为零，`fresh_process_verified=true`。本项仅覆盖 M7 资产，不代表完整项目运行或游戏功能验收。

检查地图为 `/Game/FPSDemo/Art/Weapons/Rifles/M7/Maps/LV_M7Preview`。2026-10-15 已通过 MCP 完成实际 Unreal 渲染，使用 UE 5.8.1、D3D11/SM5、长焦透视侧机位与灯箱：[默认整枪](Previews/UE_M7_Default_Side.png)、[满改堤风](Previews/UE_M7_Tidal_Side.png)、[满改标准](Previews/UE_M7_Standard_Side.png)、[机匣近景](Previews/UE_M7_Receiver_Detail.png)。四张均已检查轮廓、装配和黑色／深炭黑材质分层，金属与聚合物可辨；这属于引擎渲染检查，仍不等于用户美术验收。截图与场景证据见 [UECaptureReport.json](Previews/UECaptureReport.json)、[UEPreviewScene.json](Previews/UEPreviewScene.json)。

跨项目迁移完整库时，选择 `Rifles/M7` 与上述各 `Attachments/<部位>/M7` 目录中的资产，并保留 Unreal Migrate 列出的依赖。只迁移一个合并整枪预设会携带其材质依赖，不会自动包含所有独立配件网格与变体。

源库旧 [Validation.json](Validation.json) 是 Blender 烘焙、导出及独立包阶段的证据。本次当前 `.blend` 与旧归档源的字节哈希不同，[SourceContentComparison.json](SourceContentComparison.json) 保存只读比较：网格、UV、修改器、材质、贴图与场景装配的已检查内容一致；不把它表述为源文件字节相同。Unreal 报告独立记录实际输入，不改写旧验证记录。

## 影袭后托与幻影后握把

本轮增加两个独立 Blender Collection Asset：`SM_M7_Stock_ShadowRail`（影袭导轨后托）和 `SM_M7_Grip_Phantom`（幻影后握把）。对应安装点分别为 `Stock` 与 `RearGrip`；各自的 GLB/FBX 位于 `Exports/`，各自的三张专属烘焙贴图位于 `Textures/Baked/`。它们与既有组件共同保存在同一当前源库中。

侧轮廓以用户追加的 `Reference/ShadowRail_Side_User.png` 与 `Reference/Phantom_Grip_Side_User.png` 为主；先前的 `Reference/ShadowRail_Stock_User.png` 与 `Reference/Phantom_Grip_User.png` 补充立体结构。上一版外形被用户否定，本轮重新校准后托斜上梁、水平下轨、握把后倾骨架和斜收尾部。参考图片不随 ZIP 分发。深度与尺度根据现有机匣接口推定，生成器固定这些美术比例，不依赖原图在另一机器运行。

`Scripts/build_shadow_stock.py` 和 `build_phantom_grip.py` 是两件配件的几何来源；由 `add_shadow_phantom.py` 注册、组装并保存。两件均有真实开放窗口、分件倒角和黑色材质分层。`ShadowPhantomReview.json` 记录开孔射线检查和既有资产的网格/UV保留指纹；`Validation.json` 保留 Blender 阶段技术核对，Unreal 导入状态由上述独立报告记录，仍未完成用户视觉验收。

满改堤风、满改标准枪管两个预设改用新后托与新握把，其他已选配件由清单继续管理。原枪默认配置及旧后托、D2 握把仍作为原有独立资产保留。库目前含 19 个资产、3 个整枪预设；预期导出 38 份独立组件文件和 6 份整枪文件，共 44 份，贴图为 57 张。

新增配件流程：在当前源库运行 `add_shadow_phantom.py` → `bake_atlases.py -- SM_M7_Stock_ShadowRail SM_M7_Grip_Phantom` → `export_library.py` → `render_sample.py` → `finalize_material.py` → `verify_library.py` → `package_sample.py`。只烘焙这两个新资产。独立预览分别为 `Preview_ShadowRail_ThreeQuarter.png` / `_Side.png` 和 `Preview_Phantom_ThreeQuarter.png` / `_Side.png`；`Preview_Shadow_Phantom_Side.png` 是两件配件等比例并排放置的真实 Blender 正交渲染，满改整枪侧视见 `Preview_Equipped_Tidal_Side.png`。

`Preview_Shadow_Phantom_Installed_Side.png` 展示两件配件在实际机匣挂点上的外露侧轮廓。幻影握把有 Y±0.010 m 的内部连接舌，它在独立预览中可见，安装后被机匣覆盖；外露上肩以追加侧视中的短连接肩为准。

用户随后要求后握把加长；生成器 `BODY_LENGTH_SCALE=1.20` 将接口下方握持段增长约 20%，并随倾角向后延伸，外露整体高度约 0.115 m。固定接口和上肩不拉高，开窗、支撑杆、细纹与尾垫通过同一映射更新。

## 已保留的堤风配件与枪身

堤风组件是源库中标记为 Blender Asset 的独立 `SM_M7_Barrel_Tidal` Collection，包含护木、外露圆管、连续导轨、内部可见件和三个前端配件挂点。安装原点和其他可选组件的接口由 `AssetManifest.json` 声明。独立交换文件为 `Exports/SM_M7_Barrel_Tidal.glb` 与 `.fbx`，专属三张烘焙贴图位于 `Textures/Baked/`。

保持一份当前可编辑源库 `Source/M7_Material_Study.blend`，避免复制出另一个同时维护的枪管 `.blend`。`freeze_tidal_attachment.py` 与 `AttachmentRegistry.json` 在枪身生成、导出、验证及打包环节检查枪管形体、材质、UV 和安装点；独立枪管 GLB、FBX 与三张贴图保持原字节。整枪预设按新的枪身重新导出。

本轮枪身由 `Scripts/rebuild_core_reference.py` 生成：压薄上机匣视觉层次，加入两条真实细长凹槽、前部机械座、跨上下机匣的竖件与横臂，修整斜弹匣井、细弧形扳机护圈、后吊带通孔及后握把连接肩。按追加反馈将机匣尾部导轨缩短两个齿位，底座尾端由 `x=-.162` 收到 `-.136`，保留约 0.5 mm 倒角，消除斜收区域上方的悬挑。导轨前端连接、高度、固定配件挂点及独立家具配件保持原位置；尾端支撑面以 `CoreReferenceReview.json` 的实际测量为准。参考截图只提供一侧外观；横向深度与不可见一侧按现有安装尺度作美术推定。

本样品的材质与贴图均为 M7 专属资源，随本目录交接。尚未被多个资产实际共用，因此不放入 `Shared/`；以后产生实际共用依赖时，再明确其唯一维护位置并更新本入口。

## 权威来源与历史基线

`Scripts/` 中的生成脚本、配置，以及 `Revisions/GeometryV2/Source/M7_PreMaterial.blend`，共同构成本阶段的权威来源。堤风几何由 `rebuild_tidal_reference.py` 生成后按注册表冻结；枪身由 `rebuild_core_reference.py` 重建。当前枪身迭代使用已经冻结的当前源库作输入，只重新烘焙 Core，不重建或重新冻结枪管。修改几何或材质时更新相应脚本或配置，再导出派生结果。

当前堤风完整重建的证据为 [TidalReferenceReview.json](TidalReferenceReview.json)。旧 `FrontRefinement.json` 仅描述重建之前的局部倒角调整，保留为过程记录，不代表当前堤风修法。用户截图作为视觉参照；本次按既有安装尺度估计比例与深度，未进行透视校正。

当前堤风取消上一轮后部低屋顶，顶导轨直接连接机匣导轨；横向与上侧高度按同一尺度重新映射，下侧高度按追加侧视图校准，并重建前端收斜轮廓、浅侧板和相关配件挂点。开孔贯通、导轨连接和曲面的技术检查以最新 `TidalReferenceReview.json` 为准，不将历史迭代的对象数或测量值视为当前版本的固定指标。这些检查不代表用户已接受外观，也不能沿用上一轮烘焙和导出的验证结果。

前端依据新增的 `Reference/Tidal_Nose_User.png` 修正连接轮廓，连续外凸弧面由参数曲面直接生成，弧顶圆润，保留已校准的孔位和顶导轨。最新要求加长的是护木／镂空枪管外壳：在后部与中部增加 `0.120 m` 长度，前端整体平移；从枪口正视，上方三角肩进一步收窄，两直斜面向窄顶轨台汇拢，收肩处有明确折线和坡度转折。仅中段圆鼓，并沿整段护木长度保持相同鼓度，下缘回收，侧长板和细纹跟随曲面。外露圆管恢复参考比例，挂点与配件随护木修订更新；最新制作方式、位置与几何检查记录以同一报告为准。前端近景原图见 [Preview_Tidal_Nose_Detail.png](Previews/Preview_Tidal_Nose_Detail.png)，上半段截面另由 `Previews/Preview_Tidal_Front_Profile.png` 枪口正视图核对。最终几何、烘焙和导出重新验证后，才能确认渲染与当前源文件一致。

前鼻连接去掉凹弧，改为连续外凸弧，弧顶圆润；截面、护木长度、沿全长一致的中段鼓度和收肩折线保持不变，整枪与前端局部的正交侧视图用于核对本次调整。

只读几何快照不写回，也不作为另一个当前可编辑版本。若后续转入手工精修，应明确记录转换时间与新的权威制作工程，并避免旧生成器覆盖精修源。

旧目录 [ArtSource/M7ReplicaV2](../../../../M7ReplicaV2/) 保留为本次材质试作之前的历史基线；其中的模型、导出、预览与旧报告不移动、不改写。它们的检查结果和生成日期属于当时版本，不能替代本次材质样品的验证。当前制作入口以本目录为准。

## 重建与核对

当前枪身迭代顺序为：冻结检查 → `rebuild_core_reference.py` → `bake_atlases.py -- SM_M7_Core` → `export_library.py` → `render_sample.py` → `finalize_material.py` → `verify_library.py` → `package_sample.py`。此前从几何基线全库赋材质与重建堤风的流程保留为历史制作阶段；它不适用于本轮已冻结枪管的局部枪身修改。

准备阶段由 `prepare_sample.py` 创建隔离样品并核对只读快照。当旧 `ArtSource/M7ReplicaV2` 存在时，同时核对旧几何；旧目录不存在时，使用包内 `Revisions/GeometryV2/Source/M7_PreMaterial.blend`，并按包内 `AssetManifest.json` 的 `baseline.sha256` 校验。两种几何来源都缺失，或已有基线哈希不符时停止，不覆盖基线，也不改写旧资产或已有当前源文件、清单。独立解压的交付包保留其清单、基线、脚本和配置即可重建。

当前样品不要执行全库 `build_materials.py` 或 `rebuild_tidal_reference.py`，否则会触发冻结检查。若另行授权修订枪管，需要保留旧冻结证据，再建立新的配件版本。烘焙和导出可在独立后台 Blender 进程运行，不需要 Unreal。

从项目根目录的 PowerShell 执行下列枪身重建命令。独立包可从解压后的样品目录执行，将第一行改为 `$m7Sample = (Get-Location).Path`；换机器时调整 Blender 入口。保留包内当前源库、注册表、独立枪管导出与贴图，枪身比例已保存在脚本中，不依赖未携带的参考图。

```powershell
$m7Sample = Join-Path (Get-Location) 'ArtSource/FPSDemo/Samples/Weapons/M7'
$m7Blender = 'D:/Blender/blender.exe'
$m7Current = Join-Path $m7Sample 'Source/M7_Material_Study.blend'
function Invoke-M7Step($m7Input, $m7Script, [string[]]$m7Extra = @()) {
    & $m7Blender --background --python-exit-code 1 $m7Input --python (Join-Path $m7Sample "Scripts/$m7Script") @m7Extra
    if ($LASTEXITCODE -ne 0) { throw "M7 step failed: $m7Script" }
}
python (Join-Path $m7Sample 'Scripts/prepare_sample.py')
if ($LASTEXITCODE -ne 0) { throw 'M7 sample preparation failed.' }
Invoke-M7Step $m7Current 'freeze_tidal_attachment.py' @('--', '--check')
Invoke-M7Step $m7Current 'rebuild_core_reference.py'
Invoke-M7Step $m7Current 'bake_atlases.py' @('--', 'SM_M7_Core')
Invoke-M7Step $m7Current 'export_library.py'
Invoke-M7Step $m7Current 'render_sample.py'
Invoke-M7Step $m7Current 'finalize_material.py'
& $m7Blender --background --factory-startup --python-exit-code 1 --python (Join-Path $m7Sample 'Scripts/verify_library.py')
if ($LASTEXITCODE -ne 0) { throw 'M7 verification failed.' }
python (Join-Path $m7Sample 'Scripts/package_sample.py')
if ($LASTEXITCODE -ne 0) { throw 'M7 packaging failed.' }
```

渲染参数 `-- --baseline` 只将输出改为 `Before_*.png`，不保存输入 `.blend`。本轮枪身侧视的 Before 从改动前当前样品渲染，其他四组 Before 保留 GeometryV2 基线渲染，均由同一脚本的对应机位和灯光生成。日常重建保留这些已生成的改前图，不以新的枪身覆写 Before。

Normal PNG 使用 OpenGL 切线空间 **+Y**。本次 Unreal 导入设置法线纹理反转绿色通道，源 PNG 保持原字节；BaseColor 使用 sRGB，Normal 与 ORM 为线性，ORM 的 RGB 分别连接环境遮蔽、粗糙度、金属度。实际引擎材质表现以本次 Unreal 渲染图为证据，Blender 预览单独保留其制作阶段含义。

外观前后对照必须固定视角、灯光、曝光和显示设置；以实际渲染检查金属与非金属区分、粗糙度、贴图接缝和细节强度。本轮堤风几何也发生重建，因此全枪前后对照同时反映几何与材质变化，不能把全部差异归因于材质。若要单独评价材质，应对同一几何做材质对照。

对照入口为 [Previews/Compare.html](Previews/Compare.html)，包括本轮枪身侧视及此前四组视图；实际机位以 `render_sample.py` 为准。Blender 制作阶段核对见 [Validation.json](Validation.json)，当前源内容比较及 Unreal 导入记录见前述独立报告。保存、烘焙、导入或文件验证成功不等于用户视觉验收。

交接时携带本目录的当前源结果、专属贴图、脚本与配置、只读重建基线、本 README，以及本次生成和验证记录。原 `M7_Material_Sample.zip` 是此前 Blender 独立包快照，不自动包含本次新增导入工具和 Unreal 资产；引擎资源通过 Unreal Migrate 连同依赖迁移。交付 ZIP 不包含 `Reference/` 下的用户截图；若需要在另一机器继续按图修形，应另行保留这些参照。旧目录不作为第二个当前制作入口。当前仍是独立静态美术资产，未制作骨骼、动画和游戏接入，不能作为已接入游戏的证据。

归类依据：[美术资产存放与复用规范](../../../../../Docs/FPS/Art/AssetOrganization.md)；现有旧目录的迁移状态见 [源文件整理记录](../../../../../Docs/FPS/Art/SourceOrganization.md)。

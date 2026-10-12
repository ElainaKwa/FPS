# 工业风首批资产制作与验证

- 用途：实施与验证记录
- 适用范围：FPS 工业风独立资产库、源文件与展示关卡
- 状态：源文件与 Unreal 资产已保存，导入、Blender 局部验证和独立关卡真实渲染完成；本记录描述资产库阶段，后续游戏接入另见专门记录
- 最后更新：2026-10-11

后续基础游戏接入见 [接入与验证](Integration.md)。下文保留资产库交付时的范围，不代表今天尚未接入。

## 1. 版本与实施范围

2026-10-11，在用户指定的 Blender 安装目录可用后制作本批资产。记录时项目基准提交为 `6c98bad89fe9c8aec8a3731261033ece0dadc759`；本批资源与脚本属于工作区未提交内容，不将该提交当作包含这些资产的交付提交。具体文件、大小、SHA-256 与报告快照见 [DeliveryReport.json](../../../ArtSource/FPSDemo/Industrial/DeliveryReport.json)，由 [记录脚本](../../../Tools/record_industrial_delivery.py) 生成。

按 [统一设计方向](DesignDecisions.md) 制作灰白/石墨色硬表面结构与克制黄色标识，不含测试代号。使用 Blender 5.2.2 LTS 建模、导出 FBX，再由 Unreal Engine 5.8 编辑器导入并保存二进制资源。

| 类别 | 本批保存内容 |
| --- | --- |
| 枪械 | M4、AK、BR3、MP5、UMP45、M9、HeavyPistol 七个静态模型及对应透明剪影 |
| 环境 | 地板、墙、柱、货箱、屏障、栏杆、门框、踏板、武器架、靶架、工作灯、基地标记，共十二个模块 |
| 人物 | 工业装甲训练机器人静态模型、16 骨骼网格，以及左右第一人称前臂/手套静态资源 |
| 材质 | 一个参数化母材质、十二个材质实例 |
| 源文件 | 三个可编辑 `.blend`、二十三个 FBX、七张 512×256 透明 PNG |
| Unreal | 二十二个静态网格、一个骨骼网格、Skeleton、PhysicsAsset、七张纹理及十三个材质资源；合计 45 个 `.uasset`，另有一个展示 `.umap` |

文件位置与重建方法见 [使用说明](AssetUsage.md)。展示关卡独立保存为 `/Game/FPSDemo/Art/Industrial/Maps/LV_IndustrialShowcase`，没有替换 `LV_FPSFoundry`、`LV_FPSTraining` 或原教学地图。

## 2. 已执行验证

- Blender 三份 `.blend` 均可重新打开；23 个 FBX 已写入，静态导出网格存在面与 UV。
- 人物 16 根骨骼，全部蒙皮顶点权重总和为 1；转动左前臂后，附属手部顶点发生超过 5 厘米位移。测试不覆盖源文件。
- 七张剪影具有非透明枪形和透明外围。
- Unreal 逐个导入并保存模型/纹理，按材质槽重新绑定本批材质；静态网格每个轴的厘米包围盒与源文件米制数据在 1% 或 0.2 厘米容差内一致。
- 骨骼网格存在 Skeleton 与 PhysicsAsset；展示关卡保存成功。上述检查来自实际编辑器执行，导入报告 `success=true`、`errors=[]`。
- 在另一个启用图形渲染的 Unreal 编辑器进程中重新加载展示关卡并截图，关卡 MapCheck 为 0 错误、0 警告；图像人工复核后修正了补光、人物/展示架朝向和相机裁切。

源验证证据：[SourceValidation.json](../../../ArtSource/FPSDemo/Industrial/SourceValidation.json)。导入证据快照包含在 [DeliveryReport.json](../../../ArtSource/FPSDemo/Industrial/DeliveryReport.json)；原日志位于 `Saved/IndustrialArt/Import.log`。导入初次发现 FBX 轴转换导致尺寸交换，已使用保留 XYZ 的导入策略重做并逐轴核对。

截图脚本修复了等待资源加载期间 Slate 回调重入导致的重复截图请求；最终执行只发出一次请求，输出 `INDUSTRIAL_CAPTURE PASS`，启动脚本退出码为 0。原始日志为 `Saved/IndustrialArt/Capture.log`，关键截图/地图检查行随交付报告保留。引擎启动阶段仍输出 `LogAutomationTest: Error: Condition failed`，发生在资产截图脚本运行之前，本次未定位该引擎自检问题；这里的通过结论仅覆盖所列资产检查与截图结果，不代表整个引擎自动化测试通过。

## 3. 实际模型预览

以下为保存后的独立展示关卡在 Unreal 中实际渲染的 1600×900 截图，不是正式对局场景。展示背景采用黑色，便于查看模块组合。

![Unreal 独立资产展示关卡](../../../ArtSource/FPSDemo/Industrial/Previews/UnrealShowcase.png)

以下图片由本批 Blender 模型实际渲染，用于查看造型和材质分块；不是生成式概念图，也不是 Unreal 对局截图。

![七种枪械模型](../../../ArtSource/FPSDemo/Industrial/Previews/Weapons.png)

![十二种工业模块](../../../ArtSource/FPSDemo/Industrial/Previews/Environment.png)

![工业装甲训练机器人](../../../ArtSource/FPSDemo/Industrial/Previews/Operator.png)

## 4. 验收边界

本批完成的是可继续编辑、可导入使用的首批模型资产库。人物仍是机器人基型，材质尚无磨损纹理；静态库的形成不代表所有造型已经获得用户最终视觉验收。

尚未将资源绑定到正式地图、武器定义、人物运行时或 UI；没有新增人物动画、动画蓝图、握持/ADS 校准、换弹联调或实战碰撞/导航验收，也没有新增 Cook、性能或多人验收。旧项目的打包和多人通过记录不能作为本批资源的验收证据。后续接入需要按 [设计决策中的验收要求](DesignDecisions.md#7-制作衔接与验收) 补齐游戏内检查。

返回 [统一美术索引](README.md)。

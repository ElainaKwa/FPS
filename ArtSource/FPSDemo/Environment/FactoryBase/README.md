# 个人工厂整备车间

- 用途：当前使用说明
- 适用范围：FactoryBase 初版地图、生成源、共享依赖和独立启动入口
- 状态：初版地图、行走、四设施与出征返回已通过 standalone 自动运行；PIE 和打包未验证
- 最后更新：2026-10-15

## 入口

Unreal 地图：`/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase`。

在项目根目录运行 `./Tools/LaunchFPSFactory.ps1 -Profile Player`。工厂使用当前搜打撤个人存档；希望独立试玩可传入其他 Profile 名称。机器人可走动并靠近设施交互；当前默认 WASD 移动、鼠标转向、F 交互、Esc 设置。移动和交互读取系统设置里的主副绑定。设施界面的 Tab 或右上角“返回车间”退出终端，原地继续走动。

新入口单独启用工厂；旧搜打撤启动脚本仍从原基地界面进入。从工厂出征的本次游戏会话结束行动后返回工厂。该返回位置不写入旧存档结构，重新启动时由使用的启动入口选择。

## 空间与依赖

28×24 米、净高约 6.6 米的单层整备车间。中央通道连接仓储、改枪、加工和出征终端，设施打开已有真实页面。厂内不运行战斗计时、守卫、免费武器台或搜获生成；机器人是基地专用行走角色，沿用现有十五个机器人部件并呈现简单关节步态。

本目录没有复制共享资产。地图引用 `Content/FPSDemo/Art/Industrial/Environment` 的地板、墙体、支柱、箱体、枪架与工作灯，以及 `Industrial/Materials` 材质；角色引用 `Industrial/Character/Runtime`。共享源仍在 `ArtSource/FPSDemo/Industrial`，未按目标目录提前迁移。

## 维护

当前权威来源是 [地图生成器](Scripts/create_factory_base.py)。项目入口 [create_fps_factory_base.py](../../../../Tools/create_fps_factory_base.py) 通过 Unreal Python 执行，不能用普通 Python 直接生成 `.umap`。默认保留已有地图；只有显式传 `-FactoryBaseRebuild`、对应 Authoring 收据存在且地图仅包含本生成器演员时才允许重建。手工精修前应先变更权威来源，不能覆盖精修内容。

生成器核对其他地图和资源的哈希，并写入 `Saved/FactoryBase/Authoring.json`。验证脚本 [TestFPSFactory.ps1](../../../../Tools/TestFPSFactory.ps1) 使用唯一测试存档、真实独立游戏进程和游戏内截图。更详细的实现与证据见 [工厂地图记录](../../../../Docs/FPS/Maps/FactoryBase.md)。

## 实机预览

以下均来自 `Saved/FactoryBase/Tests/20261015-151641508/` 的实际游戏采图，非概念图：

- [机器人行走视角](Previews/Walkable.png)
- [车间全景](Previews/Workshop.png)
- [改枪区](Previews/Gunsmith.png)
- [加工区](Previews/Fabrication.png)
- [仓储区](Previews/Storage.png)

[生成清单](Manifest.json)由地图生成器导出；记录的是地图保存与其他包未变更检查，运行结论见上述独立测试记录。

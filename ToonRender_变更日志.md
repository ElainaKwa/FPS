# 卡通渲染实施变更日志

> 目的：每一步改动都可追溯、可回退。原则：只增不改，不碰 Main 关卡，不改现有资产。

## 环境快照（动手前）

- 引擎：UE 5.8.1（Launcher 版，非源码版）
- 项目：D:\Unreal Projects\MCP_Test（C++ 工程，Platformer 模板）
- 版本控制：无（已用 ToonRenderBackup/ 做文件级备份）
- 关键渲染设置（原始值，来自 DefaultEngine.ini）：
  - r.Substrate=True（Substrate 材质开启）
  - r.DynamicGlobalIlluminationMethod=1（Lumen）
  - r.ReflectionMethod=1（Lumen）
  - r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=True（自动曝光扩展亮度）
  - EditorStartupMap=/Game/Maps/Template_Default
- 当前关卡：Main（干净、已保存，未做任何改动）

## 备份

| 文件 | 备份位置 |
|---|---|
| Config/DefaultEngine.ini | ToonRenderBackup/DefaultEngine.ini.bak |
| Config/DefaultEditor.ini | ToonRenderBackup/DefaultEditor.ini.bak（原文件为空） |
| Config/DefaultGame.ini | ToonRenderBackup/DefaultGame.ini.bak |

## 已执行操作

| # | 操作 | 类型 | 回退方式 |
|---|---|---|---|
| 1 | 创建文件夹 /Game/ToonRender | 新增 | 删除该文件夹（确认无引用后） |
| 2 | 复制 /Game/Maps/Template_Default → /Game/Maps/ToonDemo | 新增（未碰原地图） | 删除 ToonDemo.umap |
| 3 | 创建 /Game/ToonRender/PPM_ToonOutline：域=PostProcess，BlendableLocation=BeforeTranslucency，HLSL 深度+法线 Sobel 描边；参数 OutlineThickness(2.0)/DepthThreshold(0.02)/NormalThreshold(0.25)/OutlineColorR,G,B(0,0,0) | 新增 | 删除该资产 |
| 4 | 创建 /Game/ToonRender/MPC_ToonLight：ToonLightDirection=(0.577,-0.577,0.577)，ToonLightIntensity=1.0 | 新增 | 删除该资产 |
| 5 | 创建 /Game/ToonRender/M_ToonMaster：Unlit 卡通母材质，MPC 主光 NdotL 量化（BandEdge=0.05/BandSoft=0.02）+ LitColor(0.95,0.97,1)/ShadowColor(0.35,0.42,0.55) 色阶 lerp + Fresnel 轮廓光（RimMin=0.3/RimMax=0.7/RimColor/RimIntensity=0.6） | 新增 | 删除该资产 |

注：以上资产在创建后均立即保存（吸取 16:59 崩溃时未落盘导致资产丢失的教训）。

## 事件记录

### 2026-09-04 16:59 编辑器崩溃（已查明原因）

- 现象：调用 load_level 加载 /Game/Maps/ToonDemo 时，编辑器崩溃（EXCEPTION_ACCESS_VIOLATION）。
- 崩溃堆栈：`ABreakablePlatform::~ABreakablePlatform()`（项目 C++ 类），发生在卸载 Main 关卡、销毁 Actor 期间。
- 分析：模块名 `UnrealEditor_MCP_Test_patch_6` 表明本会话已 Live Coding 热重载 6 次，疑为热重载新旧代码布局不一致导致；不排除该类的析构路径存在潜在问题。
- 影响：Main.umap 完好（未在本次会话中被保存/修改）。新建资产均在崩溃前保存或可重建。
- 处置：见后续记录（待定）。

### 2026-09-04 17:10 MCP 桥接会话失效（待用户重启 DSH Host）

- 现象：编辑器崩溃重启后，DSH Host 的 mcp-client 仍持旧会话 ID 请求编辑器 MCP（http://127.0.0.1:8000/mcp），服务端返回 -32600 "Unknown session id ... client should reinitialize"。
- 排查：DSH mcp-client 插件只在连接断开/重连时重建会话，工具调用错误不触发重建；HMR 探针（改 cordis.patch.yml、touch 插件文件）未生效。
- 验证：编辑器侧 MCP 服务健康（直接 POST 探针拿到新会话 184b4dcc…）。
- 处置：需用户重启 DSH Host 进程后重连；编辑器（PID 35464）保持运行等待。

### 2026-09-04 17:30 DSH mcp-client 自愈补丁（一劳永逸方案）

- 问题根源：DSH 的 mcp-client 插件只在连接断开时重建会话，工具调用收到"会话失效"错误不会自动重连；UE 侧 MCP 插件会话纯内存（Epic 引擎插件，不改）。
- 补丁内容：修改 `dsh-mcp-client/lib/index.js`（npx 缓存中的 DSH 检查副本）：
  1. 新增 `isStaleSessionError()` / `staleSessionRetryError()` 辅助函数；
  2. 工具调用的三处失败路径检测 "Unknown session id / client should reinitialize" 错误；
  3. 命中时调用 `staleSessionRecovery()`（`startConnection` 内新增）→ `client.close()` → 触发 onclose → generationDown → 自动重连并重建会话。
- 配置调整：`C:\Users\97853\.dsh\profiles\web\cordis.patch.yml` 增加 `reconnect.maxAttempts: 60`。
- 备份：`ToonRenderBackup/dsh-mcp-client-index.js.patched`（若 DSH 升级/重装 npx 缓存被清，需把此文件拷回 `node_modules/@deepseek-ai/dsh-mcp-client/lib/index.js` 重新应用）。
- 生效条件：需要重启 DSH Host 一次以加载补丁；之后编辑器崩溃/重启时，MCP 会话失效会自动重连，**无需再重启 DSH**。

### 2026-09-04 18:00 演示场景搭建与验证（第二轮）

- 编辑器重启后：发现 Template_Default.umap 内部装的是 Main 关卡内容（历史遗留），启动即加载 Main 内容。
- 加载 ToonDemo 未崩溃（上次崩溃实为 Live Coding 热重载所致），但 ToonDemo.umap 被识别为"资产文件"，编辑器开了一个空的 Untitled 世界——直接在 Untitled 世界中搭建演示场景（不保存，关闭即消失，零回退成本）。
- 场景内容：2000×2000×20 地板 + 2 球 1 立方 1 锥体（均挂 M_ToonMaster）+ 方向光 + 无限范围 PostProcessVolume。
- 修复 M_ToonMaster：Lerp A/B 接反（亮面输出了 ShadowColor），已交换并保存。
- 验证结果（像素级分析，无视觉模型可用）：
  - M_ToonMaster 母材质正确：资产预览球体上部亮 (210,211,212)、下部蓝灰暗 (162,173,189)，色阶明显。
  - 场景近景图确认物体上有亮面/暗面色阶分界，卡通光照在场景内生效。
  - PPM_ToonOutline 之前从未挂上后期体（weightedBlendables 一直为空——首次设置静默失败）；用 {"array":[...]} 包装格式重设后成功挂上。
  - PPM 挂上后画面变化，但效果不对：屏幕四周出现灰色边框而非物体描边，HLSL 的 UV 偏移/边缘采样有 bug，待修复。
- 截图存档：ToonRenderBackup/toon_demo_01~07_*.png。

### 2026-09-04 19:00 MainToon 关卡方案（按用户要求放弃屏幕描边）

- 用户决策：放弃屏幕空间描边（PPM_ToonOutline），改为复制 Main 关卡直接试卡通材质效果。
- 操作：
  1. AssetTools.duplicate 复制 /Game/Maps/Main → /Game/Maps/MainToon（未动原 Main）。
  2. 加载 MainToon（完整内容验证通过：天空大气、雾、体积云、10+ StaticMeshActor、BreakablePlatform_2 等）。
  3. 给全部 StaticMeshActor 的 StaticMeshComponent0 + BreakablePlatform 的 PlatformMesh 应用 M_ToonMaster。
  4. BreakablePlatform 的 ShakeVisual 组件 refPath 解析失败（非根 C++ 组件），未赋材质——游戏中该组件仍显示默认材质，待处理。
  5. 已保存 MainToon。
- 视觉模型验收（deepseek-v4-flash-vision-exp）：平台顶面/侧面有清晰的亮暗硬分界（色阶生效），但整体卡通感不足：天空/雾/渐晕仍是写实渐变、色调偏灰、亮暗反差不够。
- 建议（视觉模型）：背景色阶化/关雾、亮面提白暗面加饱和、色阶加到 3-4 级、接触阴影/AO 色阶化。
- 截图：ToonRenderBackup/toon_demo_10_maintone.png。
- PPM_ToonOutline 资产保留未使用（如以后想要描边可继续修或改用倒置外壳方案）。

## 待办/注意

- MainToon 微调方向（可选）：关雾/体积云、天空色阶化、提对比度（改 M_ToonMaster 的 LitColor/ShadowColor 默认值或建材质实例）。
- ShakeVisual 组件材质待处理（Blueprint 或 C++ 方式）。
- BP_ToonLightUpdater 蓝图未建；MPC 默认方向 (0.577,-0.577,0.577) 暂代。
- 视觉验证现可用 deepseek-v4-flash-vision-exp（工作流指定 model 路由）。
- 在从 Main 切走关卡会崩溃的问题解决前，不要对 Main 做"卸载"操作。
- 如后续改动 Config/*.ini 中的渲染设置，逐条记录原始值于此。


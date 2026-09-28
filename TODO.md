# MCP_Test 项目待办文档

> 引擎：UE 5.8 ｜ 类型：C++ 工程（模块 `MCP_Test`） ｜ 默认地图：`/Game/Maps/Template_Default`
> 说明：文档为 AI 协作待办，按优先级分为"当前玩法（做）"与"远期 GASP（只记录，暂不做）"。

---

## 一、当前进度（已完成 ✅）

### 2D 侧视平台跳跃 · 可玩闭环
- **角色** `Source/MCP_Test/PlatformerCharacter.h/.cpp`
  - 胶囊碰撞体（半径 34 / 半高 88）+ 可见胶囊外观（`/Engine/BasicShapes/` 圆柱+两半球拼接，无碰撞，纯引擎资产）
  - 2D 移动：A/D / ←→ / 手柄左摇杆 X 轴沿世界 X 移动，Space 跳跃（早松=低跳）
  - 手感参数：MaxWalkSpeed=650、JumpZVelocity=850、GravityScale=1.6、AirControl=0.5
  - 相机：正侧面（yaw=-90°、pitch=0、零偏移）+ 平滑拖尾 CameraLag（LagSpeed=5、MaxDistance=450），角度不再跟随角色
  - 输入全部代码内建（Enhanced Input，零资产）
  - **坠落重生**：Z < -300 → 瞬移回检查点（默认=出生点 `RespawnZ` 可在类默认值调整）
- **GameMode** `PlatformerGameMode.h/.cpp` + `Config/DefaultEngine.ini`（`GlobalDefaultGameMode=/Script/MCP_Test.PlatformerGameMode`）
- **关卡** `Template_Default`：起点站台 `StartPad`（顶面 Z=0，x:-400~100）+ 6 块悬空平台（顶面 140→880，台阶 140~160cm、间隙 ≤350cm），地面已删除（跳空=坠落=重生）
- **工作流已验证**：MCP 直驱动编辑器；C++ 改代码 → 编辑器内 **Ctrl+Alt+F11（Live Coding）** → 新 PIE 生效（无需重启编辑器）；UBT 完整编译/重启编辑器备用流程可用

---

## 二、当前玩法待办（近期专注，按建议顺序）

- [ ] **终点判定**：终平台放终点标志（Cube 拼旗帜/光柱），到达 → 显示"过关"（PrintString / HUD 或简单 C++ 事件）
- [ ] **通关后重玩**：胜利后 3~5 秒重置回出生点（或按 R 手动重置）
- [ ] **金币/收集物**：悬浮小球（`/Engine/BasicShapes/Sphere`）摆几颗在跳跃路径上，吃到计数并旋转/消失（Tick 旋转 + Overlap 检测）
- [x] **中途检查点**（已完成 ✅：关卡放置 2 个 `TriggerCapsule` + `Checkpoint` 标签，位于 Platform2 / Platform4，覆盖整平台；角色进入即推进重生点，只前进不倒退，坠落回到最近检查点）
- [x] **平台 Y 归 0 + 易碎平台**（已完成 ✅：全部 6 块平台 Y→0（曾因"只传位置"的移动把 Platform2/3/5 缩放误重置为 1×1×1 正方体，已恢复 600×300×40 板状并教训记录见下）；新增 C++ 类 `ABreakablePlatform`（`Source/MCP_Test/BreakablePlatform.*`）：踩上即触发 1.25s 倒计时（离开不取消，重生后才重新武装；倒计时内**仅视觉网格**抖动渐强、碰撞体不动所以玩家不晃，`ShakeAmplitude` 可调）→ 碎成 8 块物理小立方从中心水平爆开、平抛弧线坠落（`ShardBurstSpeedMin/Max` 可调；几何碎块表现碎裂，引擎无裂纹贴图）→ 3.5s 后原位重生；易碎平台现放在**终点 PlatformEnd 之后**（`BreakablePlatform_2`：x=5350、顶面 880，200×240×30，距终点 350cm 空隙）供测试）
- [ ] **手感打磨备选**：二段跳 / 踩头跳 / Coyote Time（离开平台边缘 ~0.1s 内仍可跳）/ 跳跃缓冲；跑速与跳高再按试玩反馈微调
- [x] **检查点可见标志**（已完成 ✅：旗帜网格体资产 `/Game/Assets/SM_CheckpointFlag`——旗杆+旗面(85cm)+顶球、**无底座**、高 230cm、宽仅 ±8cm；已插在 Platform2 / Platform4 **左缘内侧**，旗面朝左悬于平台间缺口上方，不覆盖平台表面，无碰撞、底部贴平台顶面）
- [ ] **美术氛围（可选，仍纯自带资产）**：平台配色区分（复制 BasicShapeMaterial_Inst 调色）、背景大色块分层、终点光柱效果
- [ ] **音效（可选）**：引擎自带音效资产（/Engine/EngineSounds/…）做跳跃/收集/过关反馈
- [ ] （未来 3D 化前的知识储备，可选）录制玩法视频 / 写一页玩法设计说明

---

## 三、远期：接入 GASP（Game Animation Sample Project） — 仅记录，暂不做 🕐

> 结论：GASP 是 **3D 第三人称人形骨骼动画** 系统（Motion Matching + 动捕动画），与当前 2D 胶囊玩法不兼容。
> 适合**未来做 3D 动作游戏**时引入；引入前先跑通官方示例并评估动画资产/性能成本。

### GASP 是什么（速记）
- Epic 官方示例工程，UE 5.8 有对应版本；社区含 Mover 变体讨论（Mover 为实验性移动框架）
- 技术栈：Motion Matching（Pose Search Database + Chooser 过滤）＋ 轨迹预测 ＋ Orientation Warping / Leg IK / Offset Root Bone（实验）＋ 攀爬翻越动捕动作集
- 官方角色 `CBP_SandboxCharacter`（BP）+ `ABP_SandboxCharacter`（动画 BP）；支持导入自建角色（IK Rig 重定向）与迁移动画资产到目标工程

### 获取与体验（第一步）
- [ ] 从 Fab 下载（免费）：<https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016>
- [ ] Epic Launcher → Vault → Create Project（必须选 **UE 5.8**，与当前引擎一致）
- [ ] 跑通内置障碍赛道，开 Game Animation Widget 观察各系统开关效果

### 之后迁移到 MCP_Test 的路线（参考）
- [ ] Migrate 动画资产：`Content/Characters/UEFN_Mannequin/Animations`、Pose Search Databases、Chooser、ABP 等
- [ ] 导入自己的 Skeletal Mesh → IK Rig → Retargeter（源骨架 UEFN_Mannequin）
- [ ] 子类化 `CBP_SandboxCharacter`，挂自建网格 + 打 `RTG_xxx` 组件标签
- [ ] 把 `PlatformerGameMode` 的 `DefaultPawnClass` 换成 GASP 角色（现有 GameMode 接线直接复用）

### 注意点
- 许可：Epic 示例内容条款，商用前以 Fab 页面许可为准
- 性能：Motion Matching 数据库吃内存与构建时间；版本跟随引擎更新（5.7/5.8 改动较大）
- 参考链接：
  - 官方 5.8 文档：<https://dev.epicgames.com/documentation/en-us/unreal-engine/game-animation-sample-project-in-unreal-engine?application_version=5.8>
  - 官方 5.8 下载博客：<https://www.unrealengine.com:8443/tech-blog/download-the-latest-game-animation-sample-project-now-updated-for-ue-5-8>
  - UE 5.7 更新博客：<https://www2.unrealengine.com/tech-blog/explore-the-updates-to-the-game-animation-sample-project-in-ue-5-7>
  - 社区变体 GASP-ALS（UE5.5，ALS 分层+复制，版本滞后）：<https://github.com/BarryHanson/GASP-ALS>
  - 中文资料：<https://blog.csdn.net/weixin_43757333/article/details/160997616>、<https://www.cnblogs.com/lanyelinxiang/p/19688913>

---

## 四、开发备忘（踩坑记录）

- **UBT 在受限 shell 下会静默崩溃**（dotnet 0xE0434352）→ 编译需完整文件访问权限（已授权流程可用）
- 编辑器重启会令 GUI 的 MCP 会话失效 → 需要 Web GUI 侧重建连接；**改已有 C++ 文件优先用 Live Coding（Ctrl+Alt+F11）避免重启；新增 .cpp/.h 文件 Live Coding 编不了，必须走完整重编译（关编辑器→UBT→重启→GUI 重连）**
- 引擎路径：`D:\UE\UE_5.8`；编辑器进程 `UnrealEditor.exe`
- ⚠️ **`set_actor_transform` 必须传完整 xform（location+rotation+scale）**：工具对缺省字段按默认值处理而非"保持不变"——只传 location 会把 actor 缩放重置成 1（曾把平台变正方体的教训）
- PIE 内对象引用路径前缀为 `/Game/Maps/UEDPIE_0_Template_Default...`，编辑器世界为 `/Game/Maps/Template_Default...`（ObjectTools 只认编辑器世界对象）
- MCP 原生层仅暴露 `list_toolsets / describe_toolset / call_tool` 三个入口（原始 HTTP 调试时用 `tools/call → name:"call_tool"` 分发）

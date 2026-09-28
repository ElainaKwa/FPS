# 第三阶段 C：原创音效与射击显示

移除 `MulticastShotFX` 中的 `DrawDebugLine`，不再绘制橙色射击调试线。服务器的 LineTrace、伤害、枪口闪光和命中反馈保留。

## 资源

`Tools/generate_fps_audio.py` 使用确定性噪声、衰减冲击和音调合成 13 段原创音效，没有使用外部采样。六种枪各有不同音高和衰减的枪声，另有换弹开始/结束、装备、检视、空仓、命中和击杀提示。声音是 Demo 合成风格，并非真实枪声录音。

可编辑生成脚本与 WAV 源文件：`Tools/generate_fps_audio.py`、`ArtSource/FPSDemo/Audio`。源文件为单声道、48 kHz、16 bit PCM，峰值限制为 0.6。通过 Unreal Python 导入 `/Game/FPSDemo/Audio/S_*`，共 13 个 SoundWave。导入脚本为 `Tools/import_fps_audio.py`，该目录已加入 Cook 配置。

## 接入

- `UFPSAudioSubsystem` 管理资源和并发播放。自己开火本地立即播放；服务器多播携带射击时的声音 ID，其他客户端播放有方位和距离衰减的枪声。本地玩家收到多播时不重复播放；人机也走空间枪声路径。
- 换弹、检视、装备音由本地玩家接收到的动作状态触发；换弹完成要求同一把枪的弹药实际增加，取消换弹不会播放完成声。空仓点击每次按下触发，另有 0.25 秒节流。
- 服务器确认命中/击杀后，仅攻击者通过现有 Client RPC 播放提示音；同队或未造成有效伤害不触发此提示。
- 空间音效采用 200 cm 内圈及 4000 cm 衰减距离；当前无墙体声音遮挡、混响区域、弹着点材质音效或脚步音。
- 音频为纯表现层，不改变 GAS 数值、得分与武器判定。专用服务器不创建播放实例。

## 检查

`Tools/TestFPSWeapons.ps1 -Audio` 运行真实双端武器检查并启用音频；`Tools/TestFPSFlow.ps1 -Mode LAN -Audio` 检查命中、击杀提示与计分流程。脚本将帧率限制为 60，避免 NullRHI 无限制帧率造成测试中的移动缓存警告。

显式传入开发参数 `-FPSAudioTrace` 时，记录声音路由、AudioComponent 是否正在播放，并从第一段声音开始录制引擎主混音输出到 `Saved/FPSFlowTests/Audio-server.wav` / `Audio-client.wav`。该路径不在普通游戏中录音，不使用麦克风；不同测试会覆盖同名录音。`Tools/check_fps_audio.py` 检查 WAV 非静音和削波。录音片段验证引擎输出，不代表对用户扬声器、耳机的人工听感验收。

2026-09-28 本次结果：

- UE 5.8 编译成功，模块 `9283`。
- 13 个 SoundWave 导入与时长检查通过；源 WAV 均非静音，峰值为 0.6。导入命令的退出码为 1，原因是并行编辑器的 MCP 插件端口 8000 已占用；资产导入脚本完整执行并保存了全部资源。
- `TestFPSWeapons.ps1 -Audio` 双端全部 PASS，60 FPS 复测无移动缓存溢出警告。服务器播放远端空间枪声，客户端播放本地枪声、换弹、检视与装备音。
- `TestFPSFlow.ps1 -Mode LAN -Audio` 的击倒、GAS +1、重生、结算快照通过；日志确认 3 次本地命中提示、1 次击杀提示和客户端 4 次空间枪声。
- 已检查真实主混音导出片段：`WeaponsAudio-client.wav` / `WeaponsAudio-server.wav`、`FlowAudio-client.wav` / `FlowAudio-server.wav`（均在 `Saved/FPSFlowTests`）；均非静音、未削波。
- 这是独立游戏进程验证；尚未验证打包版本、声音遮挡、不同用户设备的主观听感，也未等待完整 5 分钟自然结算。

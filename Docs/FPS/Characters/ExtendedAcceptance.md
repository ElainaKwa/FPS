# 角色与模式扩展验收

- 用途：实施与验证记录
- 适用范围：三阶段角色、配装、安全区及物理掉落的当前打包版本和完整对局
- 状态：当前版本构建、新包 14 项、连续切图及实际 Dedicated 十客户端五分钟通过；人工体验和独立 Server Target 包未验收
- 最后更新：2026-10-10

用户要求补充 [三阶段记录](StageThreeImplementation.md) 之外的完整运行验收，并在验收后提交本次改动。规则仍以 [设计决策](DesignDecisions.md) 为准；不扩展美术、地图尺寸、AI 配装、无缝切图或未来玩法。提交仅包含本任务及必要的 FPS 依赖，其他工作区改动保留。

## 范围与版本

使用 UE **5.8.1 / CL-56057345**，准入协议 **6**。2026-10-10 最新 Editor 和 Development Game 已构建，完整 Cook/Stage/Pak/Archive 返回 0；开局校验覆盖 7 个武器定义。构建前后 131 个 Source/Config/uproject 输入的哈希完全一致。

| 产物 | SHA256 |
| --- | --- |
| Editor DLL | `44a03dff422679c70db2f139b2dbc537f9eae85729c40be499c223b67b201425` |
| 新包内层 Game EXE | `2eb12413d4e7c8021a80f412bb3f75a300e454bf058c392a169b919d6afc5b64` |
| 新包内容容器 ucas | `70b87584a87a9651abfefab0849b40df3b15742c35421e62f35332a15be21642` |

全部产物与构建命令见 [构建记录](../../../Saved/FPSCharacterAcceptance/Build/20261010-DedicatedFix/run.json)。可随 Git 保存的 [验收索引](Generated/AcceptanceSummary.json) 由 `Tools/RecordFPSCharacterAcceptance.py` 从构建、采样、完整对局、切图及 14 项包记录导出，核对产物、131 个构建输入、日志和截图/WAV 的记录哈希；同时保存失败尝试，禁止手工改生成结果。它索引既有运行断言，不增加新的验收覆盖。此前第三阶段的 Editor DLL `543d46a2…` 及通过日志保留为历史证据，不代替本版本的新运行结果。

生成器还要求构建退出码、专项非空完成标记与退出码、真实 Dedicated 网络模式、活跃参与者完成、300 秒观测、断开/重入事件以及五阶段切图标记。重新导出时使用上列构建记录，`--sample` 指向两人前提采样，`--round` 指向十人完整记录，`--travel` 指向新包切图记录，`--package-run` 指向新包 14 项目录；`--failed-attempt` 可重复指定历史失败记录。所有确切输入路径保存在生成索引中。

本机为安装版引擎，InstalledPlatforms 未包含 Server Target，UBT 明确拒绝该分发版的独立 Server Target。Game Target 的 `-server` 实测未建立专用服务器，源码 `CoreMisc.h` 的 GameOnly 分支也直接返回 false。此次改用同一冻结源码构建的 Editor `-server` 进程连接新打包客户端，必须由日志确认 `NM_DedicatedServer=1` 和客户端 `NM_Client=3`；它与独立 Server Target 包分别记录。

## 验收项目

| 项目 | 条件 | 当前结果 |
| --- | --- | --- |
| 构建与内容 | 最新 Editor、完整 Cook/Stage/Archive、新包包含对战/靶场/原教学及模式和所有可选武器 | 通过；完整 Cook 587 包，7 武器定义加载，容量/限频不一致准入拒绝 |
| 打包双端专项 | 新包服务器和客户端：配装、基地规则、权威物理、实例冷却、原子拾取及清理；100ms/1% 与晚加入 | 通过；GUID/弹药/冷却、合并复制预测、185 秒真实寿命、限额及越界回收 |
| 地图、教学与表现 | 新包实际地图渲染、原教学双端、GAS、远端及晚加入表现；音效事件和波形导出 | 通过；两图渲染、教学 25/35/死亡重生、表现去重及两端 WAV 检查 |
| 完整对局 | 当前 Editor Dedicated Server 加十个新打包客户端，300 秒不缩短；150ms/3%、发送中断恢复、断开重入、相关性恢复、结算与动态物清理 | 通过；实际 300.03 秒、10→9→10 人、十个最终 owner 完成 |
| 现有切图路径 | 新包同一进程，对局→大厅→靶场→大厅→新对局；旧角色/ASC 退出、新实例和默认配装、P/Esc 及输入上下文 | 通过，五阶段 21.32 秒；原生输入注入、Standalone OpenLevel |

所有结果须记录新鲜日志、进程参数、模块/包哈希、退出状态及适用版本。场景、预测和输入注入与自然网络收发分开说明；完整 300 秒对局也不称为真人游玩。

## 新包 14 项专项

[专项目录](../../../Saved/FPSCharacterAcceptance/Package/20261010-2eb12413-all) 覆盖 2026-10-10 21:21:55–21:38:19：Drops、DropsLag、DropsLifetime、TrainingDrops、Loadout、TrainingLoadout、Characters、Maps、Content、Weapons、Abilities、Teaching、Presentation、Audio 全部 `PASS / exit=0`。31 份主日志、6 张 PNG 和 2 份 WAV 的最终 SHA 复核一致，均使用上表新 Game EXE；本轮启动进程已退出。精确参数、完成标记和证据路径见生成索引。

掉落零延迟及 100ms/1% 两套除服务器断言外，拥有者客户端均确认实际 `AFPSMatchMode::FinishRound` 后 `bFinished`、动态物为 0、预置物数量不变；此前测试准备调用的 `ClearRound` 与该结算断言分开。晚加入观察者确认休眠刚体位置快照，不包含结算删除断言。自然寿命专项不缩放时间，179 秒仍存在，182 秒服务器回收、183 秒客户端销毁。

人工查看 6 张截图：界面图标/名字、选择标记、P/Esc 和 30/180/FIFO 提示清晰；掉落、拾取提示、靶场、ADS 枪体和 HUD 可见。Foundry 当前随机出生截图被既有蓝色掩体遮住大部分远景，不能当成全地图视觉验收；世界固定朝向标签从背面看反向，保留为现有视觉局限，不扩展地图和美术修改。

音频事件与导出检查通过，[波形分析](../../../Saved/FPSCharacterAcceptance/Package/20261010-2eb12413-all/Audio/audio-analysis.log) 的服务器 WAV 实际 9.28 秒、peak=0.1801、RMS=0.006905，客户端 5.33 秒、peak=0.1268、RMS=0.009032。两份非静音且未削波；脚本的录制请求不等于完整 20 秒音频，更不等于人工听感验证。

## Dedicated 完整对局

[完整对局记录](../../../Saved/FPSCharacterAcceptance/DedicatedTenFullRound-20261010-212642-244/run.json) 与 [服务器日志](../../../Saved/FPSCharacterAcceptance/DedicatedTenFullRound-20261010-212642-244/server.log) 覆盖 2026-10-10 21:26:42–21:32:18。服务器实测 `netmode=1`，十个最终客户端为 `netmode=3`，准入协议 6，Bots=0，未缩短五分钟。

最终服务器 `age=300.09 observedWall=300.03 peakPlayers=10 players=10 validPlayers=10 podium=3 shots=731 dynamic=0 presets=14`。十个活跃 owner 全部完成；被主动结束的旧客户端记录没有完成标记，符合断开场景，且没有玩法失败。脚本只清理自己启动的进程，退出状态与最终日志 SHA 保存在记录中。

- 每人统一默认型号进入，服务器成功选枪后立即保存各自型号、新 GUID、副槽保持；真实本地移动经 CharacterMovement RPC 离开基地，旧选枪请求被拒。
- 对客户端 1 的发送模拟设置 `PktLoss=100` 持续 20 秒，再恢复 3%，可靠 RPC 往返恢复通过。常规两端启动日志明确应用 `PktLag=150` 与 `PktLoss=3`。
- 对一位玩家位置作两次相关性场景注入，8 个持续在线观察者均看到对象移除后重新恢复。这是确定性注入，非自然跑远。
- 第十个旧客户端在 21:29:16 结束，服务器看到 9 人；21:30:29 启动全新客户端重入，恢复到 10 人，新 PlayerState、默认配装及新生命选枪权限通过。
- 原在线玩家实际移动返回基地，保护恢复但选枪不恢复，禁攻/禁丢/拒绝请求通过；再次离开后不会恢复旧扳机意图。
- 52 秒附近真实 GAS 丢主枪，220 秒仍有 10 个动态物，242 秒全部自然到期；275 秒再丢副枪，285 秒有 10 个动态物，真实 FinishRound 清理为 0。14 个预置物始终保留。

十客户端完整场景验证了服务器自然结算清理和客户端结算/领奖台状态，但未逐客户端断言结算后的动态物数量；不能声称十客户端自然结算删除均已验证。拥有者复制删除由上节新包短专项的实际 FinishRound 验证，客户端自然到期删除由完整对局采样及寿命专项验证。

另有 [两人 40 秒前提采样](../../../Saved/FPSCharacterAcceptance/DedicatedFixPrerequisiteFinal-20261010-212338-108/run.json)，结果 `PASS_SAMPLE_ONLY`，仅证明加载、准入、输入、配装及网络前提，独立于完整对局结果。帧间隔和内存样本包含帧率上限等待，只作运行观测，不作为性能预算或压力认证。本场景受控瞄向地面保持角色存活，伤害、死亡和重生另由专项验证。

## 连续切图证据与边界

[新包切图记录](../../../Saved/FPSCharacterAcceptance/TravelPackageFixed20261010/run.json) 与 [实际日志](../../../Saved/FPSCharacterAcceptance/TravelPackageFixed20261010/travel.log) 包含五次世界进入与最终 `PASS SERVER COMPLETE phases=5`。角色、PlayerState、控制器结束生命周期，旧 ASC 失效，旧输入上下文移除；大厅释放内容会话，新局恢复 M4/M9 默认型号并创建新 GUID。原生 P/Esc 映射注入经过当前控制器，打开界面时鼠标、移动、视角与攻击输入状态正确。

早期夹具误用 Effect 资产 Tag，保护使用授予 Tag，曾捕获到空集合。修复后的夹具查询 owning/granted Tag，并要求对局捕获数量大于 0、靶场与大厅为 0；当前记录实际捕获 `regionEffects=1`。切图后旧 ASC 失效，旧保护无法延续到新身体，新模式保护状态也正确。独立保护来源的施加、重叠和移除另见阶段二及配装专项。

## 验收发现与必要修复

真实 Dedicated 加载发现 `AFPSBaseRegion::OnConstruction` 访问被服务器剥离的 TextRender 组件，触发空指针崩溃。基地标牌与边界标识增加空值检查；拾取物网格和标签分别检查，服务器物理盒仍独立从武器资产计算；治疗站灯光检查空值；纯角色表现入口在 Dedicated 模式直接返回。没有强制服务器加载表现组件，也没有跳过基地 Bounds、碰撞、伤害或动作规则。

修复前的 `ec9d4c2d…` / `f8f5ee36…` 构建、切图及掉落结果保留为历史，全部最终自动验收重跑于上表 `44a03dff…` / `2eb12413…` 版本。上述 5 个模式/地图资产仍与第三阶段快照一致，未因服务器修复重写二进制资产。

## 失败尝试

失败记录保留，不覆盖为通过。早期 Editor 编译因测试夹具调用受保护的 Enhanced Input API 失败，改用公开 API 后本版本编译通过。首次 Editor 切图未进入玩法，原因是 DDC 无可写节点。Game `-server` 预检暴露上述目标类型限制；Editor 启动预检还发现脚本单元素数组拼接及临时内存 DDC 加载失败。恢复可写共享缓存后取得上述明确的基地崩溃调用栈，按生产问题修复。日志哈希采证与进程退出存在句柄竞争，脚本改为退出后采证或有界等待稳定文件，不降低运行断言。运行验收以修复版本完成后的新鲜通过记录为准。

## 保留边界

手柄和逐键硬件输入、真人手感、音效听感及动画审美需要人工体验。现有地图流转使用 OpenLevel/ClientTravel；项目未实现无缝 ServerTravel，此次不为验收增加玩法路径。其他形态角色、未来控制/强制位移、新模式和美术未纳入。

帧时间/内存观测不等于性能压力认证；独立 Server Target 包、外部网络和人工 PIE 游玩仍未验证。阶段三历史日志存在既有 ExperimentalToolsets Python 初始化错误；本轮自动通过条件为专项完成且没有专项 FAIL、Fatal、Assertion 或 TravelFailure，不宣称项目所有插件日志零错误。

证据保存在 [本轮独立目录](../../../Saved/FPSCharacterAcceptance)，不覆盖此前阶段快照。`Saved/` 为本机产物，不保证随 Git 分发。

2026-10-10 交付前运行 `python Tools/CheckDocs.py`：48 份文档、0 errors、102 项历史例外。本任务仅移除 NetSync 使用说明已修复的四项元数据豁免，未新增或扩大历史豁免；人工核对规则、完成状态、版本和上述证据边界。

返回 [角色主题](README.md)。

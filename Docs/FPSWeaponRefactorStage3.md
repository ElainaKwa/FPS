# 枪械架构重构第三阶段

状态：第三阶段实现及下列局部验收已完成，2026-09-30 收尾。原六枪与教学地图保留。

## 实施选择

新枪命名为 BR-3，稳定 ID 为 `Weapon.BR3`，主槽步枪，使用已有共用 Burst Ability。这些名称和初值为本次实施选择，不是访谈指定值。

| 参数 | 初值 |
| --- | --- |
| 弹匣 / 初始装填 / 备弹 | 30 / 30 / 90 |
| 伤害 / 射程 | 28 / 9000 cm |
| 发间隔 / 每轮数量 / 末发后冷却 | 0.09 秒 / 3 / 0.35 秒 |
| 换弹 / 举枪准备 / 视觉过渡 | 2.3 / 0.25 / 0.4 秒 |
| 后坐力恢复等待 | 0.65 秒 |

独立 `CR_BR3` 三通道曲线覆盖 0–29 发，末端保持末值；俯仰随连发内序号和连续射击进度增加，水平为三发左右小幅变化。其余表现参数由 M4 复制为 BR-3 自己的可编辑字段，三视角明确复用 M4 网格，开火复用 `S_Fire_M4`，操作音效复用现有共享资源。没有制作独有枪模或音色，也没有隐藏按名字拼接路径。

地图增量位置：Foundry `(-2280,-840,75)` 和 `(2280,-840,75)`，Training `(-2650,-700,75)`，展示序号 6。每处新增标记、底座和饰条；保留原六枪武器台与所有已有 Actor，未重建地图。原教学地图不修改。

## 扩展涉及的文件与资产

- 唯一可编辑枪械数据：`Content/FPSDemo/Weapons/Definitions/DA_BR3.uasset`、`Curves/CR_BR3.uasset`。
- 地图：既有 `LV_FPSFoundry.umap`、`LV_FPSTraining.umap` 各自增量保存。
- 编辑器制作：`FPSWeaponDataCommandlet.cpp` 的显式 `-CreateBurstRifle` 入口，只创建缺失资产，不覆盖后续调参；`Tools/add_br3_weapon_stands.py` 只增量添加标记和底座。
- 开发验收：`FPSWeaponSmoke.cpp` 新增已保存 BR-3 的地图拾取、预测连发、冷却、换弹和丢拾检查；`FPSMatchFlow.cpp` 仅调整开发地图验收的预期数量为 7/14。
- 审查：`FPSWeaponAssetReview.md` 从已保存资产重新导出，禁止独立编辑作为第二来源。

角色、音频服务、Ability、组件调度、HUD、运行时目录均未添加 BR-3 型号分支。新增定义由 Asset Registry 自动发现，资源通过定义引用进入现有加载/Cook 配置。武器台是关卡明确内容配置，不是运行时型号枚举。出生 M4/M9 不变。

早期 `Legacy` 六枪表只用于显式 `-Migrate` 的历史数字标记迁移；运行不读取它，不应向其中加入 BR-3。原地图重建脚本是历史制作入口，不在本轮执行；如将来主动重建，需随后执行增量武器台脚本。已保存资产是日常调参来源，制作初值代码不与资产双向同步。

## 验证

- 七个已保存定义、三处 BR-3 引用及数据负例校验通过（9304）。
- Editor Development 9305 编译及 post-build 数据检查通过，见 `Saved/Logs/WeaponBuild9305.txt`。
- `TestFPSWeapons.ps1 -Audio -Visual -Latency 50 -PacketLoss 1`：9305 独立服务器/客户端均 COMPLETE PASS，原六枪回归及 BR-3 地图实例拾取、松开完成三发、冷却点击拒绝、再次按下、30/84 换弹结果和丢拾 GUID 延续均通过。本地三发呈现计数与权威扣弹一致。见 `Saved/Logs/WeaponStage3Runtime.txt` 和 `Saved/FPSFlowTests/Weapons-server.log`、`Weapons-client.log`。
- `TestFPSMaps.ps1`：Foundry 10 个出生点、14 把地图武器、10 个角色；Training 4 个出生点、7 把武器、4 个角色，出生点净空检查均通过。见 `Saved/Logs/WeaponStage3MapRuntime.txt`。
- 已查看 `Saved/Screenshots/WindowsEditor/Weapon.BR3.png`：复用网格正常，HUD 显示 BR-3 30/84；两张地图启动截图也正常。新枪实际开火时双端记录了复用音效的 LOCAL / SPATIAL 播放；录音工具的前 20 秒采样不包含后半段 BR-3，不把录音检测当成其独立音色验收。
- `git diff --check` 通过。第三阶段未更改 GameplayAbility 或运行时调度，第二阶段已有中断/死亡/标签清理证据仍见第二阶段记录；本轮没有重新声称执行所有历史用例。
- 编辑器已正常关闭后重新启动，实际进程模块确认加载 `UnrealEditor-MCP_Test-9305.dll`。

完整五分钟自然结束、十真实客户端、打包/Cook 与性能压力仍未覆盖。本轮不实现暂缓的逐发装填、配件或动态难度。

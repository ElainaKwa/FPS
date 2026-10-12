# 配件 A1：定义、解析与内容准入记录

- 用途：实施与验证记录
- 适用范围：配件工作线 A1 数据基础、显式依赖及服务器／客户端内容准入
- 状态：数据实现、Editor／Game 构建、命令let与双端针对性验证通过；游戏改装入口尚未接入
- 最后更新：2026-10-12

依据：[设计决策](AttachmentDesign.md)、[任务清单](AttachmentTasks.md)。补给箱模型改由用户安排其他制作者，本工作线负责玩法、接入与验证。

## 实施范围

新增 `UFPSAttachmentDefinition` 保存型 PrimaryDataAsset，包含稳定配件 ID、平台白名单／显式通用标记、接口、类别、强类型修正及表现软引用。武器定义保存平台、平面接口、显式可用配件引用和默认组合 ID；实例结构增加安装 ID 与配置版本。挂起改装、实例配置的行为消费和提交取消在 A3 接入，当前没有玩家改装入口，现有 M4 默认组合为空，原枪参数与行为保持。

`FFPSAttachmentResolver` 统一验证候选组合并计算容量、换弹时间、ADS 时间／位置／FOV、后坐力倍率及开火声音。容量用整数相加，时长与后坐力用倍率相乘；按稳定 ID 排序，装配顺序不改变结果。失败不改写输出。平台与接口分开检查，通用配件仍需满足接口。首批平面接口每个类别及接口最多安装一个配件。

本局依赖由使用到的武器及其显式可用配件共同构成，包含可选但尚未安装的配件；不扫描并加载全项目所有配件来决定一局能否开始。Catalog 分别持有定义、配件及资源加载句柄，失败不发布部分目录，结束释放。必需 ID 在资产注册表中检查缺失／重复；全项目编辑器发布校验继续检查全部配件。内容协议从 7 升为 8，摘要纳入平台、接口、组合、可用配件 ID 和配件数据；资源版本沿用保存时生成的机制，共用 `FPSContentHash` 实现。

Editor 构建原有的 `FPSWeaponData` 校验入口也已接入全项目配件校验。补充 `NetCore` 私有模块依赖，解决工作区已有移动代码使用量化向量序列化时的链接缺项；未为此改写移动行为。

## 资产与生成资料

通过 `UnrealEditor-Cmd.exe -run=FPSAttachmentData -CreateM4 -Export` 创建六个资产，位于 `/Game/FPSDemo/Weapons/Attachments/`：标准／固定视野瞄具、标准／制退枪口、标准／扩容弹匣。命令只创建缺失资产，已存在的配件及已有可用列表由策划维护，不从初始化常量重置。

初始调试数值：固定瞄具 FOV 48、ADS 时间倍率 1.15；制退器垂直／水平倍率 0.8／0.85；扩容弹匣容量加 15、换弹时间倍率 1.15。这些是资产中的起始调试值，不代表平衡已验收。三类配置均显式沿用原枪表现，独立模型尚未接入，不能将参数配置计为六种独立外观已完成。

本轮现有资产只保存了 M4 定义以加入显式平台与配件引用；另外六个武器定义、Foundry、Training 和原教学地图与本轮开始时的 SHA-256 一致。迁移前 M4 备份及资产哈希位于 `Saved/FPSAttachmentTests/A1-Before/`。

[配件清单](AttachmentAssetReview.md) 由新命令let从资产导出，[武器清单](../../FPSWeaponAssetReview.md) 由原命令let重新导出；没有手改生成正文或二进制资产。

## 验证与证据

本轮基于未提交工作区。Editor 模块及源码／配置指纹记录在 `Saved/FPSAttachmentTests/A1-Version.json`；执行双端测试前记录的模块 SHA-256 为 `e9c809c1180ef35c001af46a6e1589da4ea2c6b8b515cd1220471839dbc256ce`。这不是整个工作区只含本轮改动的声明。

| 检查 | 结果 | 本轮证据 |
| --- | --- | --- |
| Editor Development 完整构建及项目内容检查 | 通过 | `Saved/Logs/AttachmentA1-BuildFinal.log`、`AttachmentA1-BuildFinal-Console.log` |
| 普通 Game Development 构建 | 通过，非 Cook／打包验收 | `Saved/Logs/AttachmentA1-GameBuild.log`、`AttachmentA1-GameBuild-Console.log` |
| 创建资产、保存、全项目校验与导出 | 六配件、七枪通过 | `Saved/FPSAttachmentTests/A1-Create-20261012/Data.log` |
| 新进程重新加载及配件自测 | 22 项检查及 COMPLETE 通过；最终模块复验见同名前缀 Final 目录 | `Saved/FPSAttachmentTests/A1-SelfTest-20261012/Data.log`、`Saved/FPSAttachmentTests/A1-SelfTestFinal-20261012/Data.log` |
| 原武器定义自测与重新导出 | 七定义通过；非法值、资源、重复身份负例通过 | `Saved/Logs/AttachmentA1-WeaponExport.log`、`AttachmentA1-ExportFinal.txt` |
| 配件内容不一致，真实服务器＋客户端 | 服务器 `pawn=0` 拒绝，客户端收到原因并释放依赖 | `Saved/FPSAttachmentTests/A1-Content-20261012/` |
| 原枪 GAS 单次伤害上下文，真实双端 | SERVER／CLIENT COMPLETE PASS，最终生命值均为 41 | `Saved/FPSAttachmentTests/A1-Damage-20261012/` |
| 原教学地图，真实双端 | 射击、治疗及重生，双方 COMPLETE PASS | `Saved/FPSAttachmentTests/A1-Teaching-20261012/` |

配件自测覆盖：空装配等同基础参数，三类属性合成，反转顺序一致；重复、缺失、不兼容平台／接口、零容量、溢出、NaN 和非法 FOV 拒绝；通用配件仍检查接口；参数变化改变摘要；内存对象改名不改变身份／摘要；重复 ID 发布失败；无关错误不阻止 M9 局但发布检查失败；必需配件错误阻止 M4 局；修复后可重载，释放后可 GC。错误注入仅在测试进程内，不保存到资产。

双端条件：同机后台 `UnrealEditor.exe -game -nullrhi -nosound`，监听服务器及独立客户端；内容负例端口 17781，伤害与教学依次使用 17779，未注入延迟或丢包。这是真实网络进程，不是编辑器 PIE，也不是可见画面／声音验收。脚本末尾的通用 PASS 文案不扩大本次测试场景的覆盖范围。

初次构建发现新局部变量遮蔽并修正；随后发现现有移动模块缺少 NetCore 链接依赖并补齐；生成字段与日期改变后构建校验正确拒绝旧清单，使用生成器重新导出后完整构建通过。失败日志保留，不将这些中间失败记为通过。

## 未验证与后续边界

尚未进行本轮 Cook／完整打包、Dedicated Server、压力／丢包、PIE 或视觉验收。配件自测中的改名是内存对象测试，不是磁盘资产移动往返。当前六配件沿用基础表现，新增配件独立模型及资源版本变化的实机检查留待资源接入。

A2 将提供策划表格式工具；A3 才接入实例装配、挂起弹匣、GAS 行为、复制和游戏改枪界面。当前属性解析通过不表示枪械已能在游戏中改装。训练预设、补给交互及模型接入仍按后续阶段实施。

返回 [枪械索引](README.md)。

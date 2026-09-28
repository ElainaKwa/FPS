# Astra 持久指令审计

日期：2026-09-27。范围：当前 Codex 桌面会话和 `D:\Unreal Projects\MCP_Test`。

## 阅读依据

- [OpenAI 模型指南](https://developers.openai.com/api/docs/guides/latest-model)：已读完当前 GPT-6 页面正文，包括提示词建议与迁移检查。
- [Eric Provencher 同名文章的 OpenAI 官网版本](https://developers.openai.com/blog/rethinking-skills-and-prompts-for-gpt-6-astra)：已读完正文。
- [X 原帖](https://x.com/pvncher/status/2095991462416490862)直接访问失败；另核对了[原帖文字镜像](https://en.rattibha.com/thread/2095991462416490862)。没有将搜索摘要当成全文。

两篇材料支持按需加载指令、缩小技能触发范围、明确完成条件、校准测试工作量和减少不必要的确认。具体落地以本机实际文件为准，不整段复制指南提示词。

## 实际发现与处理

| 位置或层级 | 审计时状态 | 处理 |
| --- | --- | --- |
| `C:\Users\97853\.codex\AGENTS.md` | 存在，0 字节 | 写入简短协作偏好；写入前备份并确认仍为空 |
| 全局 `AGENTS.override.md` | 不存在 | 不创建覆盖层 |
| 当前项目及其 D 盘祖先的 AGENTS 文件 | 未发现 | 新建项目根 `AGENTS.md`，只保留项目事实和条件式文档入口 |
| 用户自建技能 | 在已检查的用户和项目技能位置未发现 | 不为本次审计制造新技能 |
| 会话技能目录 | 18 个可用技能，来自 `.system` 与插件缓存 | 检查会话注入的名称和描述；按相关性读取正文。可用不等于正文已加载，未宣称完整审查所有参考文件 |
| 本次使用的 OpenAI Docs | 已完整读取根文件与 `references/model-migration.md` | 保留；已有模型保留、来源核实和按需路由约束 |
| Skill Creator | 已完整读取根文件 | 保留；已包含精简描述、渐进披露、按风险调整流程的建议 |
| 其他已检查的技能正文 | imagegen、plugin-creator、skill-installer、computer-use、pdf、plugin-management；另检查 documents 和 presentations 的相关部分 | 只用于审计，不执行这些技能的业务流程；不修改供应方缓存 |
| 全局 `config.toml` | `gpt-6-astra`，`medium`；无额外自定义指令文件配置 | 无需模型迁移，保留 |
| 项目 `.codex/config.toml` | Unreal MCP 本地服务地址 | 保留；不将配置存在当成连接成功 |
| `C:\Users\97853\.codex\memories` | 目录为空 | 无可修改的文件型记忆；不声称审计了所有内部数据库记忆 |
| `C:\Users\97853\.codex\rules\default.rules` | 历史命令允许规则 | 已读；属于执行权限，不是模型提示词，不改动 |
| 桌面状态中的相关设置 | 未发现额外自定义指令正文；历史输入不是全局指令 | 不将聊天历史提升为持久约束 |
| 应用注入的开发者与工具指令 | 已包含多项 Astra 行为建议 | 不能通过编辑用户 Markdown 改写；保留当前指令层级 |

## 修改的理由与边界

本机没有庞大的个人 AGENTS 或自建 SKILL 可删减。此次补充最小的全局协作偏好，并给项目添加真实、按场景使用的入口。特别将知识锁策划案的范围限制限定在该玩法，避免影响网络 FPS Demo。

部分供应方技能存在较长的模板流程、反复渲染要求或固定后续建议。未将其一律删除：视觉产物仍需要验证，工具契约仍需遵守，缓存文件也不是稳定的用户维护入口。全局偏好仅明确需求优先和按任务取舍，不替代供应方技能、不改变插件启用状态，不放宽权限。

没有改写任何 SKILL.md，没有禁用任何技能，也没有强制多代理；当前会话对多代理另有明确限制。本次未执行独立模型行为评测，因此不宣称已测得速度或质量提升。

## 验证与恢复

文件检查覆盖 UTF-8 内容回读、全局文件与准备稿一致、项目文档链接存在、没有更改模型与权限设置。此次仅修改指令，不需要 UE 构建或 PIE 测试。

`global-AGENTS.proposed.md` 是全局文件的可审阅副本，不会作为 AGENTS 自动加载。`global-AGENTS.before.bak` 保存修改前的原始空文件。恢复时先确认没有后续用户改动，再用备份恢复全局文件；项目新增文件可单独撤销。

验证结果：全局文件与准备稿 SHA-256 一致，新增 Markdown 均可严格按 UTF-8 解码，项目入口引用均存在；模型仍为 `gpt-6-astra`，推理强度仍为 `medium`。写入后，本会话收到新的 AGENTS 指令，内容与全局偏好一致，因此全局偏好已确认重新注入。项目 AGENTS 的自动加载尚未在新聊天中验证。

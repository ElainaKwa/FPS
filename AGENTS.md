# MCP_Test

这是 Unreal Engine 5.8 项目，主 C++ 模块为 `Source/MCP_Test`；引擎关联和插件以 `MCP_Test.uproject` 为准。

按任务读取资料：网络同步基础关卡看 `Docs/NetworkBasicsLesson.md`；FPS/GAS 教学 Demo 看 `Docs/NetSyncFPSDemo.md`；知识锁玩法看 `Docs/知识锁_玩法策划案.md`。知识锁文档中的“不做战斗”等范围仅适用于该玩法，不限制其他 Demo。无需每次修改前通读所有资料。

Unreal MCP 的连接配置位于 `.codex/config.toml`。需要编辑器操作时，先确认本轮工具是否可用；配置存在不代表服务已连接。源代码与文本配置可直接通过文件工具修改。

修改网络同步或 GAS 行为时，按相关 Demo 文档检查服务器和客户端的实际表现；编译成功不能代替运行验证。修改普通文档或提示词时无需启动引擎、全量构建或运行整个 Demo。无法完成必要的编译或 PIE 验证时，说明未验证部分。

在现有任务范围内保留无关地图、资产和用户改动。二进制 Unreal 资产通过适用的编辑器工具处理；不要以文本补丁修改 `.uasset` 或 `.umap`。

## 文档维护

新增、整理或跨项目复用美术资产时，先读取 [美术资产存放与复用规范](Docs/FPS/Art/AssetOrganization.md)。现有目录按 [源文件整理记录](Docs/FPS/Art/SourceOrganization.md) 分批迁移；不能将目标结构当作已经完成的状态。

创建、修改、移动或归档 `Docs/` 内容前，必须阅读并遵守 [文档分类与维护规则](Docs/DocumentationRules.md)。按主题归类，维护文首标识和索引；移动文件时搜索并修复文档、代码及脚本中的旧路径引用。自动生成资料只能修改权威来源或生成器后重新导出。

文档改动交付前，在项目根目录运行 `python Tools/CheckDocs.py`，修复失败并说明仍需人工核对的内容。修改检查器时另运行 `python -m unittest discover -s Tools -p test_check_docs.py`。普通文档检查无需启动 Unreal。

`Tools/DocCheckBaseline.json` 仅记录规则生效前的逐文件历史欠账，禁止为让新问题通过而新增或扩大豁免；修复、迁移对应文件时删除已失效的豁免。不要在无关任务中批量迁移旧文档。检查通过不能代替内容、证据、状态及归档理由的人工审查。

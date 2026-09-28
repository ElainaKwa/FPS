# MCP_Test

这是 Unreal Engine 5.8 项目，主 C++ 模块为 `Source/MCP_Test`；引擎关联和插件以 `MCP_Test.uproject` 为准。

按任务读取资料：网络同步基础关卡看 `Docs/NetworkBasicsLesson.md`；FPS/GAS 教学 Demo 看 `Docs/NetSyncFPSDemo.md`；知识锁玩法看 `Docs/知识锁_玩法策划案.md`。知识锁文档中的“不做战斗”等范围仅适用于该玩法，不限制其他 Demo。无需每次修改前通读所有资料。

Unreal MCP 的连接配置位于 `.codex/config.toml`。需要编辑器操作时，先确认本轮工具是否可用；配置存在不代表服务已连接。源代码与文本配置可直接通过文件工具修改。

修改网络同步或 GAS 行为时，按相关 Demo 文档检查服务器和客户端的实际表现；编译成功不能代替运行验证。修改普通文档或提示词时无需启动引擎、全量构建或运行整个 Demo。无法完成必要的编译或 PIE 验证时，说明未验证部分。

在现有任务范围内保留无关地图、资产和用户改动。二进制 Unreal 资产通过适用的编辑器工具处理；不要以文本补丁修改 `.uasset` 或 `.umap`。

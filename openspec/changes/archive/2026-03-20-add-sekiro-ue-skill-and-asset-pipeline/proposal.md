## 为什么

固定 Unreal 命令行与测试工程引擎路径为 `D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`，后续实现、命令行验证和文档约定都以该路径为准。

当前导出链路已经能产出 Sekiro 的部分模型、动画、纹理和技能配置，但 UE 工程 `SekiroSkillEditor` 还缺少把这些正式产物完整接回来的契约：材质和纹理导入没有被要求形成可工作的 UE 材质实例，技能 JSON 虽然可导出，但 UE 侧没有被要求完整复刻 Sekiro 的战斗语义、DummyPoly 关联、义手/战技覆盖结果，以及像原编辑器那样可编辑的事件时间轴。当前链路也没有把 root-motion 视为正式交付物，因此动画在 UE 中即使可播放，也可能无法正确复刻位移语义。

截至 2026-03-18，仓库内已经存在一条“部分正式化”的实现链路：`SekiroExporter` 已能输出 `asset_package.json`、`material_manifest.json`、PNG 纹理、glTF 动画、`skill_config.json` 与 root-motion 数据；`SekiroSkillEditor` 已存在 commandlet 导入、角色/技能 DataAsset、时间轴浏览控件、预览视口和事件检查器。这些能力证明正式链路并非从零开始，但它们仍停留在“可导入、可浏览、可预览”的状态，尚未达到本变更要求的“正式材质装配 + 动画完整性闭环 + 资产编辑器式技能会话 + 角色级 fail-closed 验收”。

继续只做“导出成功”或“UE 里看得到资产”的交付，会让导出端与 UE 端长期脱节。现在需要把导出工具、导入脚本和 UE 编辑器插件收敛成一条完整的 Sekiro 资产与技能复刻链路，明确什么才算正式完成。

## 变更内容

- **BREAKING** 将 `SekiroSkillEditor` 的完成标准从“能导入部分资产并显示时间轴”提升为“能基于正式导出产物完整复刻模型、动画、材质、纹理和技能语义”；缺少任一正式输入或任一关键复刻结果时，导入与验收都必须失败
- **BREAKING** 将 `SekiroSkillEditor` 的时间轴要求从“浏览导入后的技能数据”提升为“提供一个以 `skill_config` 为正式序列化来源的自定义 Sekiro 时间轴编辑器”，其交互与信息密度必须能覆盖原工作流中的事件分轨、区间事件、属性编辑和播放头检查场景
- 扩展导出工具正式合同，要求 `SekiroExporter` 为 UE 侧交付完整且可追溯的 Sekiro 资产包，包括模型、动画、PNG 纹理、材质清单、技能配置、参数关联和复刻所需的元数据
- 扩展动画正式合同，要求导出链路正确交付 Sekiro 动画 root-motion，并让 UE 导入、预览、验证和编辑器会话都消费同一份 root-motion 结果，禁止把 root-motion 丢失、烘焙错轴或在 UE 侧临时重建视为成功
- 收紧动画完整性合同，要求角色级成功以“解析出的应交付 clip 集合全部导出”为准，禁止仅以“导出到至少一个动画文件”或“共享动作池可人工补看”视为成功
- 收紧 UE5 commandlet / importer 导入合同，要求其不仅导入骨骼资产，还必须建立纹理、材质实例、技能数据资产、参数索引和资源间引用关系
- 收紧 UE 材质装配合同，要求导入链路使用唯一正式 Master Material 和正式 manifest 驱动的参数绑定；禁止继续使用 `DefaultMaterial`、命名后缀猜测或自动补槽位作为正式成功路径
- 收紧 `SekiroSkillEditor` 插件合同，要求其在编辑器内完整重建 Sekiro 技能系统的关键语义，包括战斗事件、DummyPoly 位置、参数链路、义手忍具覆盖和战技样式检查
- 收紧编辑器打开方式合同，要求技能资产以可双击进入的资产编辑器会话打开，而不是仅提供一个全局 Nomad Tab 浏览器
- 补充角色级端到端验收，要求对单个角色明确验证“导出资产完整性 + UE 导入完整性 + 编辑器复刻能力”三者同时成立

## 功能 (Capabilities)

### 新增功能
- `sekiro-ue-replica-validation`: 定义 Sekiro 角色在 UE 侧的完整复刻验收合同，覆盖导出产物完整性、导入完整性和技能语义复刻结果

### 修改功能
- `sekiro-exporter-cli`: 将 CLI 正式交付物扩展为 UE 可直接消费且可验证的 Sekiro 资产包，要求角色级导出摘要明确记录材质、纹理、技能与参数链路完整性
- `skill-config-export`: 将技能配置导出从“事件 JSON”提升为“可在 UE 侧复刻 Sekiro 战斗语义的正式交付物”，要求输出 DummyPoly、BehaviorParam、AtkParam、SpEffect、义手覆盖和战技关联的可追溯关系
- `skill-config-export`: 将技能配置导出从“事件 JSON”提升为“Sekiro 自定义时间轴编辑器的正式序列化格式”，要求输出可回放、可编辑、可复刻的事件轨、参数链路、DummyPoly/覆盖结果以及与动画 root-motion 对齐所需的时间基准
- `material-manifest-export`: 将材质清单从纹理槽位映射扩展为可驱动 UE 材质实例重建的正式清单，要求记录材质实例创建和纹理绑定所需的稳定键
- `texture-export`: 将纹理导出要求收紧为服务 UE 材质重建的正式输入，要求 PNG 输出、命名和清单映射能一一对齐且失败可追溯
- `ue5-import-scripts`: 将当前 commandlet / importer 导入链路从“批量导入资产”扩展为“建立角色级资源关系和材质/技能装配”的正式导入管线
- `ue5-skill-editor-plugin`: 将插件正式目标收紧为 Sekiro 技能系统复刻器，而不只是浏览器；要求编辑器能消费正式导出资产并验证战斗语义、材质配置和角色级复刻结果
- `sekiro-exporter-cli`: 将 CLI 正式交付物扩展为包含 root-motion 完整性的角色级导出摘要，要求逐动画记录 root-motion 来源、坐标轴约定、是否成功导出以及失败原因

## 影响

- 受影响代码主要位于 `SekiroExporter/`、`DSAnimStudioNETCore/Export/`、`SekiroSkillEditor/Source/` 以及当前 commandlet / importer 导入链路所在目录
- 现有正式 specs `sekiro-exporter-cli`、`skill-config-export`、`material-manifest-export`、`texture-export`、`ue5-import-scripts` 和 `ue5-skill-editor-plugin` 都会被收紧；现有只保证“资产存在”或“能浏览时间轴”的实现不再满足完成标准
- 这项变更会把角色级成功定义改为 fail-closed：导出端缺材质/纹理/技能关联，或 UE 端未生成正确材质实例、参数索引、技能资产、视口复刻结果时，都不得报告成功
- UE 工程 `SekiroSkillEditor` 将从演示型工具提升为正式验收入口，因此需要同步更新编辑器 UI、数据资产、导入验证器和批处理工作流，并补齐自定义时间轴编辑器与 root-motion 预览/校验链路

## 继承的正式化遗留项

本变更继续承接 `formalize-sekiro-export-pipeline` 中尚未完成、但已经明确属于 UE 资产闭环的剩余范围，避免在归档后丢失正式化上下文：

- 继续收紧 `texture-export`，补齐 Sekiro 实际纹理编码支持矩阵、PNG 正式命名契约以及与材质清单的一一对应关系
- 将 UE 导入验收从“可导入”提升为“必须形成 SkeletalMesh + Skeleton + PhysicsAsset + 正确动画绑定”的角色级硬性判定
- 把 `c0000` 之外的角色回归、正式统计基线重建，以及旧日志/旧选项清理纳入后续正式化闭环
- 承接原变更中需要在 DSAnimStudio/UE 编辑器内手动验证的菜单、时间轴、3D 视口、攻击判定、特效点、音效点与正式语义复刻工作
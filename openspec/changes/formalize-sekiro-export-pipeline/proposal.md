## 为什么

当前 Sekiro 正式导出链路虽然已经能产出可用于验证的模型、动画、纹理和技能配置，但仍然依赖多格式降级导出、兼容式 HKX 读取、导出后修补、纹理解码回退、可选模板和 hack 参数解析等非正式实现。继续在这条链路上叠加功能，只会把“能跑”和“正式正确”混在一起，导致交付格式、失败语义和 UE 导入契约长期不稳定。

现在需要把这 8 类问题收敛成一份独立变更，明确唯一正式交付格式、唯一正式解析路径和唯一正式失败标准，为后续实现提供不可含糊的规范边界。

## 变更内容

- **BREAKING** 规定模型与动画的唯一正式导出格式为 `glTF 2.0 (.gltf + .bin)`，不再把 `fbx`、`fbxa`、`glb`、`dae/collada` 视为正式交付物
- **BREAKING** 规定纹理的唯一正式导出格式为 `PNG`，不再允许正式链路在 PNG 失败时回退导出 `DDS`
- **BREAKING** 正式导出链路改为 fail-closed：HKX 解析失败、动画采样失败、glTF 结构修复失败、纹理解码失败、模板缺失、参数表读取失败时，必须显式失败并进入验收报告，禁止静默跳过、自动降级或填充 identity/占位数据
- 移除模型与动画导出的多格式 fallback 链，统一为单一 glTF 2.0 导出契约，并把当前导出后修补逻辑逐步前移为原生正确的导出构建行为
- 统一 HKX 骨架与动画读取链路，定义 Sekiro 正式支持的单一解析策略，禁止继续用“哪个变体能读就用哪个”的兼容回退树
- 用正式参数定义读取替换 `PARAM_Hack`，同时把技能配置导出改为模板和参数定义驱动的严格 schema 输出，禁止无模板时退回 raw bytes、未知字段按 `s32` 猜读
- 为每个角色输出正式验收报告，逐项记录模型、动画、纹理、技能、参数和 UE 导入可用性；任何一项未满足正式契约都不得记为成功

## 功能 (Capabilities)

### 新增功能
- `formal-export-contract`: 定义 Sekiro 正式导出交付物的唯一格式、目录结构、失败语义和禁止 fallback 的规则
- `formal-hkx-parse-pipeline`: 定义 Sekiro 骨架与动画的唯一正式 HKX 解析路径、错误语义和采样约束
- `formal-texture-export`: 定义纹理的唯一正式输出格式、像素语义、解码覆盖范围和失败标准
- `formal-skill-param-schema`: 定义技能配置、参数表和模板解析的正式 schema 与严格字段约束
- `character-export-validation`: 定义角色级导出验收报告和通过/失败判定标准

### 修改功能

- `flver-fbx-export`: 现有模型导出规范需要改为单一正式 glTF 2.0 交付契约，并移除多格式 fallback 作为完成标准的一部分
- `animation-fbx-export`: 现有动画导出规范需要改为单一正式 glTF 2.0 clip 交付契约，并禁止静默跳过和 identity 兜底
- `texture-export`: 现有纹理导出规范需要改为单一正式 PNG 交付契约，并禁止 DDS 回退作为正式成功结果
- `skill-config-export`: 现有技能导出规范需要改为模板/参数定义强约束输出，禁止缺模板时降级为 raw bytes
- `skeletal-import-contract`: 现有 UE 导入契约需要同步到新的单一 glTF 2.0 / PNG 正式交付标准与 fail-closed 验收标准

## 影响

- 受影响代码主要位于 `SekiroExporter/Program.cs`、`DSAnimStudioNETCore/Export/AnimationToFbxExporter.cs`、`DSAnimStudioNETCore/Export/FlverToFbxExporter.cs`、`DSAnimStudioNETCore/Export/GltfPostProcessor.cs`、`DSAnimStudioNETCore/Export/TextureExporter.cs`、`DSAnimStudioNETCore/Export/SkillConfigExporter.cs`
- 现有 OpenSpec 变更 `sekiro-export-ue5-skill-editor` 中与格式选择、成功标准和 fallback 容忍度相关的描述需要被新的正式化规范覆盖和收紧
- UE 导入侧需要继续以 glTF 2.0 和 PNG 为唯一正式输入；任何历史上依赖 FBX、DDS、Collada 或宽松导入规则的路径都要降级为非正式或删除
- 这项变更会改变 CLI 成功判定、日志语义和验收统计方式，历史上“有产物即算成功”的统计结果将不再有效
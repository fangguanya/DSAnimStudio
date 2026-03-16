## 上下文

DSAnimStudio 是一个基于 .NET 6.0 + MonoGame (DirectX 11) 的 FromSoftware 游戏动画编辑器。它已经具备完整的 Sekiro 资产加载能力：FLVER2 模型（网格/骨骼/材质）、HKX Havok 动画（样条压缩/交错未压缩）、TAE 时间轴事件（185 种 Sekiro 事件类型）、TPF 纹理（DDS 格式）。

当前导出能力有限：仅支持 HKX 动画导出（Havok 2010 XML/Packfile/2016 Tagfile），不支持 FBX 模型导出、纹理导出或结构化技能配置导出。FBX 导入通过 AssimpNet 4.1.0 已经可用（在 MonoGame Content Pipeline 中）。

Sekiro 游戏安装在 `C:\Program Files (x86)\Steam\steamapps\common\Sekiro`，包含 130+ 角色（c0000-c9990）的 `.chrbnd.dcx`（模型）、`.anibnd.dcx`（动画+TAE）、`.tpf.dcx`（纹理）文件。

UE5.7 引擎位于 `D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`。

**关键源文件参考：**
- `FlverSubmeshRenderer.cs:328-527` — FLVER 顶点数据提取逻辑（Position/Normal/Tangent/UV/BoneIndices/BoneWeights）
- `FlverShaderVertInput.cs` — 顶点格式定义（16 个属性字段）
- `FlverMaterial.cs:577-706` — 材质纹理类型分类（ALBEDO/NORMAL/SPECULAR/EMISSIVE）
- `NewAnimSkeleton_FLVER.cs:95-108` — FLVER 骨骼层级加载
- `NewHavokAnimation.cs` — HKX 动画数据封装
- `ToolExportAllAnims.cs` — 现有 HKX 导出管线
- `TaeEditorScreen.cs:348-369` — 现有导出 UI 模式和事件迭代模式
- `MenuBar.050.Tools.cs` — Tools 菜单 ImGui 集成模式
- `Res/TAE.Template.SDT.xml` — Sekiro 全部 185 种 TAE 事件类型定义
- `ParamData/*.cs` — 游戏参数数据结构（AtkParam/BehaviorParam/SpEffectParam/EquipParamWeapon）

## 目标 / 非目标

**目标：**
- 建立完整的 Sekiro 资产导出管线，输出行业标准格式（FBX/PNG/JSON）
- 支持全部 130+ 角色的批量导出（CLI 工具）
- 在 DSAnimStudio 中提供交互式导出界面
- 在 UE5.7 中构建专用技能编辑器插件，完整导入并准确复刻战斗技能配置
- 实现端到端流水线：Sekiro 游戏文件 → DSAnimStudio 导出 → UE5 导入 → 骨骼模型/动画/战斗事件复刻
- 为导出器、导入器和验证器定义统一的数据契约，避免 schema、命名和字段定义分叉

**非目标：**
- 不修改 Sekiro 游戏文件（只读导出）
- 不实现完整的可玩战斗控制器或 AI 逻辑
- 不支持 Sekiro 以外的 FromSoftware 游戏（DS1/DS3/ER 等）
- 不修改 SoulsFormats 或 SoulsAssetPipeline 子模块的源代码
- 不追求完整游戏级实时战斗模拟，但编辑器内必须能基于真实参数复刻技能时序、攻击点位和事件触发
- 不进行模型/动画的编辑修改功能，仅导出和查看

## 决策

### 决策 1：FBX 导出使用 AssimpNet 4.1.0

**选择：** AssimpNet 4.1.0（Open Asset Import/Export Library 的 .NET 绑定）

**替代方案考虑：**
- **Autodesk FBX SDK**：功能最完整，但需要额外安装 SDK、许可证复杂、C++ 互操作开销大
- **自定义 FBX ASCII 写入器**：完全可控，但 FBX 格式文档不公开，需要大量逆向工程
- **glTF 2.0 导出**：开放标准，但 UE5 对 glTF 的骨骼动画导入支持不如 FBX 成熟

**理由：** AssimpNet 已经是项目依赖（MonoGame Content Pipeline），FBX 导入路径已验证可用。Assimp 的 FBX 导出器支持网格、骨骼、骨骼权重和动画。如果骨骼权重保真度不足，可回退到 FBX ASCII 格式作为备选。

### 决策 2：纹理解码使用 Pfim 库

**选择：** Pfim NuGet 包

**替代方案考虑：**
- **DirectXTexNet**：微软官方 DirectXTex 的 .NET 绑定，最完整但安装较重
- **TexConv 命令行工具**：通过外部进程调用，依赖额外可执行文件
- **手动 BC 解码**：最轻量但需要实现多种压缩算法

**理由：** Pfim 是纯 .NET 实现，支持 BC1/BC3/BC5/BC7（覆盖 Sekiro PC 版全部常用格式），NuGet 安装简单，无外部依赖。BC6H（HDR 格式）较少见于角色纹理，如遇到可后续添加 DirectXTexNet 支持。

### 决策 3：技能配置使用 JSON 格式

**选择：** JSON（Newtonsoft.Json）

**替代方案考虑：**
- **XML**：与 TAE.Template.SDT.xml 格式一致，但冗余度高
- **Protocol Buffers**：高效二进制格式，但增加构建复杂度
- **SQLite 数据库**：结构化查询能力强，但导入 UE5 不便

**理由：** JSON 在 C#（Newtonsoft.Json 13.0.1 已是项目依赖）和 UE5 C++（使用 FJsonObject）中均有优秀的解析支持。人类可读，便于调试和 review。结构灵活，能表达 TAE 事件的异构参数类型。

### 决策 4：UE5 插件使用 Slate UI 而非 UMG

**选择：** Slate（UE5 底层 UI 框架）

**替代方案考虑：**
- **UMG (Unreal Motion Graphics)**：蓝图友好，但自定义绘制能力有限
- **ImGui (UnrealImGui 插件)**：DSAnimStudio 已用 ImGui，但非 UE5 官方支持
- **Web UI (Chromium Embedded)**：跨平台但性能开销大

**理由：** 时间轴编辑器需要高度自定义的绘制逻辑（多轨道、拖拽、缩放、事件选择），Slate 提供最底层的控制能力。UE5 内置编辑器（Sequencer、Animation Editor）均使用 Slate 实现。编辑器插件天然使用 Slate。

### 决策 5：导出目录结构按角色分组

**选择：** 按角色 ID 分目录，每个角色下分 Model/Animations/Textures/Skills 子目录

**理由：** 与 Sekiro 游戏文件组织方式一致（chr/c0000 等），便于按角色批量导入 UE5，也便于选择性导出特定角色。

### 决策 6：正式交付物必须是单一、可导入的骨骼资产集合

**选择：** 每个角色的最终交付物只保留一套可被 UE5 正式消费的模型文件、一套动画文件集合、纹理、材质清单和技能配置；中间格式仅允许在显式调试开关下额外保留。

**替代方案考虑：**
- **同时保留 glTF / GLB / DAE 等全部中间产物**：便于调试，但会放大磁盘占用，也会模糊“哪一套才是正式导入输入”
- **把 DAE 当最终模型格式**：实现简单，但无法作为正式的骨骼蒙皮交付物验收

**理由：** 这次变更的目标不是“导出尽可能多的中间文件”，而是“生成一套能被 UE5 稳定导入并复刻 Sekiro 技能/战斗配置的正式资产”。调试产物不应混入正式交付。

### 决策 7：导出器与 UE5 导入器必须共享同一 JSON 契约

**选择：** 将 skill config 的顶层结构、动画字段、事件字段和参数表结构定义为单一 canonical schema，并要求 C# 导出器与 UE5 导入器都严格遵守。

**替代方案考虑：**
- **导出端和导入端各自适配不同 schema**：短期灵活，但极易造成“代码都在、链路不通”的假完成
- **导入器做宽松兼容**：能吞更多输入，但会掩盖导出错误，无法形成正式验收标准

**理由：** 当前链路已经暴露出 exporter/importer 对 `characters`、`frameCount`、`fileName`、`startFrame` 等字段理解不一致的问题。正式方案必须用一份 schema 消除歧义。

### 决策 8：UE 预览复刻以参数解析和 DummyPoly 解析为核心，不以占位可视化替代

**选择：** 编辑器预览层必须先把 TAE 事件解析为可执行的 battle model，再根据 BehaviorParam、AtkParam、SpEffectParam、EquipParamWeapon、DummyPoly 和骨骼姿态驱动攻击框、特效点和音效标记。

**替代方案考虑：**
- **仅按事件类别显示色块或默认球体**：实现快，但无法证明技能/战斗配置被正确复刻
- **只显示事件文本，不解析参数关联**：适合调试，不满足“完整/正确复刻”的目标

**理由：** Sekiro 的战斗语义不只存在于 TAE 事件本身，而是分布在事件参数、参数表引用和 DummyPoly 点位中。缺少参数解析就只能得到可视化占位，而不是正式复刻。

## 风险 / 权衡

### AssimpNet FBX 导出骨骼权重保真度
**[风险]** AssimpNet 的 FBX 导出器在处理复杂骨骼权重（4 权重/顶点）时可能存在精度损失或格式兼容性问题。
→ **缓解**：Phase 1A 完成后立即用 c0000（玩家角色）测试，导入 Blender 验证骨骼权重。如果不合格，回退到 FBX 7.4 ASCII 格式手写器（格式有据可查）。

### BC6H (HDR) 纹理不支持
**[风险]** Pfim 可能不支持 BC6H（半精度浮点 HDR）格式的纹理，而某些环境/特效纹理可能使用此格式。
→ **缓解**：BC6H 在角色纹理中极少出现。遇到时跳过并记录警告，后续可引入 DirectXTexNet 补充支持。

### 130+ 角色批量导出耗时
**[风险]** 全部角色的完整导出（模型+动画+纹理+技能配置）可能需要 2-4 小时。
→ **缓解**：CLI 工具使用 `Task.WhenAll` 并行处理独立角色。支持 `--chr` 参数选择性导出单个角色用于测试。

### UE5.7 Slate API 变更
**[风险]** UE5.7 的 Slate API 可能与 5.4/5.5 文档有差异，自定义 Widget 的基类可能变化。
→ **缓解**：先创建最小可行 Widget 验证编译，然后逐步添加功能。使用 UE5.7 源码中的 Sequencer 模块作为 Slate 时间轴实现的参考。

### SoulsFormats 子模块兼容性
**[风险]** 子模块固定在特定 commit（79692b8），与当前 .NET 6.0 项目的 API 兼容性未经验证。
→ **缓解**：Phase 0 中优先初始化子模块并完整构建验证，确认所有 API 调用正常。

### TAE 事件参数类型异构性
**[风险]** 185 种 TAE 事件类型的参数数量和类型各不相同（s32/f32/u8/s16 等），JSON schema 需要灵活处理。
→ **缓解**：参数统一存储为 `Dictionary<string, object>`，JSON 序列化时根据 TAE.Template 的类型信息保留原始类型。UE5 端解析时按 TypeName 进行类型分派。

### 导出器 / 导入器 schema 漂移
**[风险]** C# 导出器和 UE5 导入器如果分别演化出不同字段名、数组结构或帧定义，会导致“代码存在但端到端不可用”的假完成。
→ **缓解**：将 schema 作为 OpenSpec 的规范级契约管理；导出和导入两端都以相同示例 JSON 和验证器测试为准，不允许宽松吞错。

### DummyPoly 与战斗参数缺失导致预览失真
**[风险]** 如果只导入 TAE 事件而未导入 Behavior/Atk/SpEffect/Equip 参数表，UE 端无法确定真实攻击点位、攻击类型、特效引用和义手覆盖关系。
→ **缓解**：把参数表导出和导入提升为正式必需项，预览层从解析后的 battle model 读取，而不是直接从原始字符串参数临时猜测。

### 静态网格或未绑定动画被错误计为成功
**[风险]** 如果验证逻辑将 StaticMesh、缺少 Skeleton 或未绑定目标 Skeleton 的动画视为可接受，将直接掩盖链路缺陷。
→ **缓解**：验证器在存在模型输入时必须将缺失 SkeletalMesh/Skeleton/PhysicsAsset、动画未绑定和零轨道资产视为硬错误。

## 上下文

当前 Sekiro 导出链路已经覆盖模型、动画、纹理、技能配置和 UE 导入验证，但正式链路仍混杂着兼容读取、导出格式降级、导出后修补、纹理回退和参数 hack 读取。它的主要问题不是“没有功能”，而是缺少单一的正式契约：同一个角色可能因为不同 fallback 路径得到不同产物，失败也可能被静默吞掉，导致 CLI 统计与 UE 真正可用性不一致。

进一步分析 DSAnimStudio 编辑器的真实运行链路后，可以确认两件此前未被规范充分表达的事实：
- 编辑器选中的 `DSAProj.Animation` / TAE 动画条目只是行为入口；实际播放时会先经过引用链求解和 `GetHkxID(...)` 解析，再绑定到真正的 HKX 动画。
- 主角等角色的可见模型不是“ANIBND 自带的单个模型”，而是 `CHRBND + ChrAsm + 装备槽/直装 + SkeletonMapper/BoneGluer` 共同构成的运行时装配结果。

这次设计不仅要冻结交付格式，还要把这两层语义转成 formal contract，避免 exporter 继续把“TAE 条目”、“HKX 动画”、“当前预览装配”和“正式交付装配”混为一谈。

这次设计要解决的是正式交付物定义，而不是单点 bug。决策必须能同时约束 C# 导出器、UE 导入器、验收报告和后续 OpenSpec 任务。

## 目标 / 非目标

**目标：**
- 为模型与动画确定唯一正式导出格式，并给出替代方案比较结论
- 为纹理确定唯一正式导出格式，并给出替代方案比较结论
- 将导出链路改为 fail-closed，禁止静默跳过、自动回退、identity 填充和 rawBytes 降级进入正式交付物
- 为 HKX 解析、参数读取、技能 schema、UE 导入验收和角色级报告建立统一约束
- 把当前依赖后处理修补的行为收敛为可替换、可消除的过渡层，而不是长期正式契约的一部分
- 为动画条目、HKX 动画、技能源 ANIBND 和动画源 ANIBND 建立统一且可追溯的正式解析关系
- 为多部件角色定义规范化正式装配结果，并明确它与 DSAnimStudio 当前预览状态不是同一概念

**非目标：**
- 本变更不直接实现所有代码修改
- 本变更不要求保留 FBX、DDS、DAE、GLB 作为正式输出
- 本变更不追求面向 DCC 工具链的广泛交换格式兼容性
- 本变更不处理与正式导出链路无关的 DSAnimStudio 编辑器历史调试逻辑
- 本变更不要求 formal export 完全复刻 DSAnimStudio 当前 UI 中任意时刻的预览装配状态

## 决策

### 决策 1: 模型与动画的唯一正式导出格式选择 glTF 2.0 (`.gltf + .bin`)

选择 `glTF 2.0`，不选择 `FBX` 作为正式交付物。

`FBX` 的优点：
- UE 原生支持历史更长
- DCC 工具兼容面更广
- 单文件工作流在部分工具里更直观

`FBX` 的缺点：
- 格式私有且行为依赖导出器实现，难以形成可验证的正式契约
- 当前链路依赖 Assimp 时稳定性不足，已经出现大骨架/多资产情况下的导出失败与格式降级
- 二进制内容不易检查和 diff，不利于建立严格的结构校验和自定义导入器
- 不适合作为“导出器与 UE 导入器共享同一份可审计合同”的核心格式

`glTF 2.0` 的优点：
- 开放规范，结构可读，可直接做 JSON/二进制一致性校验
- 便于精确表达骨骼层级、accessor、skin、animation channel 等正式约束
- 当前 UE 侧已经具备基于 glTF 的导入与修复经验，更适合继续收敛为单一路径
- 更容易实现逐角色结构化验收报告和差异分析

`glTF 2.0` 的缺点：
- 当前 Interchange 对独立动画 glTF 绑定已有 Skeleton 的支持不完整
- Assimp 导出存在已知结构问题，当前仍需要后处理修补
- `.gltf + .bin` 是双文件交付，不如单文件形式直观

最终选择 `glTF 2.0` 的原因是：本项目最核心的目标是“正式、可验证、可审计、与 UE 导入器共享合同”，而不是“尽量兼容所有外部工具”。FBX 更适合作为兼容输出，glTF 更适合作为规范驱动的正式输出。

### 决策 2: 纹理的唯一正式导出格式选择 PNG

选择 `PNG`，不选择 `DDS` 作为正式交付物。

`DDS` 的优点：
- 最接近游戏源格式，保留压缩容器与源像素布局
- 对部分纹理无需重新编码即可转存

`DDS` 的缺点：
- UE 对通用 2D 纹理的 DDS 导入链路并不适合作为唯一正式契约，工具行为和压缩变体支持不稳定
- 纹理自动化、材质清单和人工校验都不如 PNG 直观
- 一旦把 DDS 作为正式输出，就会把“源格式兼容性问题”继续推给 UE 侧而不是在导出侧完成归一化

`PNG` 的优点：
- UE 导入稳定，适合作为统一的内容生产输入格式
- 可视化、审计、diff 和材质自动化都更简单
- 更适合作为角色级验收报告中的最终纹理交付物

`PNG` 的缺点：
- 文件体积通常大于 DDS
- 需要在导出侧实现完整、正式的源 DDS 解码覆盖
- 不保留原始 GPU 压缩容器语义

最终选择 `PNG` 的原因是：本项目需要的不是“源纹理容器归档”，而是“稳定导入 UE 的正式纹理交付”。因此规范必须要求导出器承担解码责任，而不是把 DDS 兼容性问题留给下游。

### 决策 3: 正式链路统一为 fail-closed

任何正式步骤失败都必须中止该资产导出并写入验收报告，不允许以下行为继续被计为成功：
- 多格式逐个尝试直到某个成功
- 多种 HKXVariation 轮询读取
- 动画帧采样失败时写 identity
- 缺模板时导出 raw bytes
- 参数未知时按 `s32` 猜读
- PNG 导出失败时保存 DDS
- glTF 修补失败时仅打印 warning

替代方案是保留现有“最大化出货率”的兼容策略，但那会继续让正式成功率失真，因此不采纳。

### 决策 4: glTF 后处理层仅作为过渡实现，不属于长期正式契约

`GltfPostProcessor` 当前解决了 Unreal 导入所需的关键结构问题，因此短期内可以作为迁移期的受控阶段保留；但规范要求它最终被前移或消除，不能长期以“导出后修一遍”为正式定义。

需要进一步区分两类问题：
- 可以在 scene 构建阶段前移的问题：正式骨架根选择、joint 集合收敛、mesh root 组织、动画条目与交付 clip 的关系归一化。
- 当前更像 writer/Assimp 输出限制的问题：`JOINTS_0` accessor typing、稳定写出 `skin.skeleton`、避免动画节点落成 `matrix` 而非 TRS、buffer URI 异常等。

因此规范不再要求“所有后处理必须立刻前移”，而是要求逐项分类、逐项消除；在原生 writer 或替代 writer 准备好之前，受控后处理仍可存在，但必须是 formal pipeline 中可验证的一段，而不是补丁层。

替代方案是将后处理直接写入正式合同并长期保留。这会固化“先导错再修”的架构，不采纳。

### 决策 5: 技能与参数导出必须转为定义驱动

技能事件、模板、参数表和字段类型必须由正式模板与正式参数定义驱动，`PARAM_Hack`、raw bytes 降级和未知类型猜读都只允许出现在迁移期间的诊断工具，不允许进入正式导出合同。

### 决策 6: 正式动画解析必须区分 TAE 入口标识与最终 HKX/交付标识

正式链路必须显式区分以下概念：
- 用户/工具选择的 TAE 动画条目 ID
- 引用链求解后的实际 TAE 动画条目 ID
- `GetHkxID(...)` 求解得到的实际 HKX 动画 ID
- `AnimFileName` / `SourceAnimFileName` 指向的最终交付动画文件名

这些标识在 Sekiro 主角链路中不总是相同，规范必须禁止继续把它们视为同一字段。

替代方案是继续以单一 anim id 贯穿导出链路。这会在 `c0000` 之类的多 ANIBND、多 TAE 文件角色上持续产生错误绑定，因此不采纳。

### 决策 7: 正式模型定义为规范化装配结果，而不是编辑器当前预览状态

DSAnimStudio 的预览链路允许通过 `ChrAsm`、`SkeletonMapper` 和 `BoneGluer` 把多部件角色在运行时拼成一套可见模型。formal export 不要求复制这一整套 UI 状态机，但必须输出一个经过规范约束的、单义的最终装配结果。

对主角等多部件角色，formal contract 应约束：
- 使用哪一组装配来源和参数表
- 哪个骨架是正式主骨架
- 哪些 follower/parts 关系必须在导出前收敛到最终 skin/joints 语义

替代方案是直接导出 DSAnimStudio 当前预览状态。这会把 UI 状态、调试按钮和临时装备状态带入正式交付定义，不采纳。

## 风险 / 权衡

- [短期成功率可能下降] → 旧链路里被吞掉的错误会暴露出来；通过角色级验收报告和阶段性迁移开关控制推进
- [glTF 仍有现存修补依赖] → 先把修补层纳入受控正式步骤并要求失败即中止，再逐步前移到原生构建逻辑
- [TAE 条目与 HKX 标识不一致] → 在 schema、日志和验收报告中同时记录 request id、resolved id、HKX id 和 source anim file，禁止单字段复用
- [多部件角色的预览装配与正式装配可能不同] → 明确 formal assembly profile 与 preview parity 是两个独立目标，避免验收标准漂移
- [PNG 需要补齐更多 DDS 解码覆盖] → 在规范中明确支持矩阵和失败语义，优先实现 Sekiro 当前实际使用格式
- [FBX 不再是正式交付物可能影响外部工具使用] → 如有需要，仅保留显式 debug/compat 输出，不计入正式验收
- [参数正式定义替换 `PARAM_Hack` 需要额外梳理字段偏移与类型] → 先锁定当前正式交付所需参数表，再逐步扩展

## Migration Plan

1. 先冻结正式交付格式和成功判定，禁止新增任何 fallback 行为进入正式链路。
2. 为 CLI 和 UE 导入链增加角色级验收报告，把历史“成功但降级”的结果显式暴露。
3. 用单一路径替换 HKX 解析、纹理解码和技能参数读取，并把 TAE 条目到 HKX/交付动画的解析链纳入正式 schema。
4. 为主角等多部件角色冻结 formal assembly profile，明确它与 DSAnimStudio 预览装配状态的边界。
5. 将 glTF 后处理从“失败可忽略的补丁”改成“受控且失败即中止的过渡步骤”，并按 scene-side / writer-side 分类逐项消除。
6. 在原生导出构建阶段补齐 skin、animation、TRS 和 buffer 结构，使后处理逐步为空实现并最终删除。

## 实施细化

### 正式动画求解管线

为了避免动画导出、技能导出和报告记录各自推断一次，正式链路需要引入共享的 animation resolution 结果对象。最小字段应包括：
- `request_tae_id`: 外部请求的 TAE 动画条目 ID
- `resolved_tae_id`: 经 `ImportOtherAnim`/引用链闭合后的真实 TAE 条目 ID
- `resolved_hkx_id`: 经 `GetHkxID(...)` 等规则求解后的 HKX 动画 ID
- `source_anim_file`: `SourceAnimFileName` 或等价正式来源文件名
- `deliverable_anim_file`: 最终写入正式交付目录的动画文件名
- `animation_source_anibnd`: 动画采样来源容器
- `skill_source_anibnd`: 技能事件来源容器

推荐的正式求解顺序：
1. 以外部请求的 TAE 条目作为 `request_tae_id`。
2. 在声明的 skill source 中闭合 TAE 引用链，得到 `resolved_tae_id`。
3. 基于求解后的条目而不是原始请求条目计算 `resolved_hkx_id`。
4. 若条目声明 `SourceAnimFileName`，则优先把它作为最终交付动画来源，并与 animation source 中的真实 HKX 文件建立一一映射。
5. 只有当上述字段之间的关系唯一且可追溯时，才允许生成 `deliverable_anim_file`。

任何一步出现歧义、缺失或需要名称猜测，都属于正式失败，而不是兼容成功。

### 主角多 TAE 文件身份规则

Sekiro 主角 `c0000.anibnd.dcx` 不是单一 TAE 文件容器。编辑器现有语义会先按每个 TAE binder file 的 `taeBindIndex` 建 category，再把动画条目标识建模为 `SplitAnimID(category, subId)`，也就是 full ID：

- `full_tae_id = taeBindIndex * 1_000000 + local_tae_id`
- `request_tae_id` / `resolved_tae_id` / `resolved_hkx_id` 在主角链路里都必须使用这个 full-ID 语义
- `ImportOtherAnim.ImportFromAnimID` 与 `Standard.ImportHKXSourceAnimID` 也必须按同一 full-ID 语义解释

因此，正式链路禁止把多个主角 TAE 文件先 merge 成“只有本地 `TAE.Animation.ID`”的扁平集合，再按裸 `aXXX_YYYYYY` 过滤。这样会把不同 category 下的同名条目压成同一个候选，直接破坏编辑器已有的解析模型。

允许的做法只有两种：

- 在 merge 前先把主角本地 TAE ID 规范化为 full ID，再统一求解
- 或者完全保留 source TAE/bind 上下文，在未扁平化的情况下按 `SplitAnimID` 求解

两种做法都必须保证正式导出与编辑器里的 `SAFE_SolveAnimRefChain(...)` / `GetHkxID(...)` 语义一致。

### formal assembly profile

多部件角色不能继续以“当前看到什么就导出什么”的方式定义正式模型。formal assembly profile 至少需要冻结以下维度：
- 主骨架来源：哪个 `CHRBND/FLVER/HKX` 被视为正式 skeleton root
- 装配来源：哪些部件来自 `ChrAsm`、哪些来自直装、哪些来自参数表驱动
- follower 关系：哪些附属 part 通过 `SkeletonMapper/BoneGluer` 绑定到主骨架
- 导出收敛结果：哪些 part 必须在正式 glTF 中收敛为同一 skeleton/skin 语义
- 报告标识：角色报告中如何唯一标识当前使用的 assembly profile

`c0000` 的 formal assembly profile 应被视为首个标准样本，而不是单次调试特例。后续其他 multipart 角色应复用同一建模方式。

### glTF 正式结构下限

为了让“正式模型成功”和“UE 真能导入骨骼资产”是同一个判断，glTF 结构至少需要满足以下下限：
- 模型 glTF 必须显式写出单一 formal skeleton root，并由 `skin.skeleton` 或等价正式关系可追溯
- `skin.joints`、inverse bind matrices、节点层级和网格绑定必须引用同一套正式骨架节点
- 动画 glTF 必须只包含单 clip，且所有 channel 都绑定到 formal skeleton 节点的 TRS 通道
- 正式交付物中的 buffer URI、mesh root 和 scene root 必须在不依赖额外猜测的前提下可由 UE 导入器消费

这里的“formal skeleton root”不是 DSAnimStudio 某次预览会话里的任意根节点，而是 formal assembly profile 明确声明的主骨架根。

### 当前实现中的 scene-side / writer-side 分类

为了把 `GltfPostProcessor` 从“事后补丁层”收敛为受控的 writer normalization，当前实现明确区分了两类责任：

- scene-side（已前移到导出构建阶段）
	- formal assembly profile 的选择与报告记录
	- formal animation resolution 的统一求解与交付命名
	- formal skeleton root 的声明：模型/动画导出在构建 Assimp scene 时先解析并传递 declared skeleton root name
	- 动画/技能/报告共用同一份 resolution 元数据，而不是各自独立猜名
- writer-side（仍保留在 glTF writer normalization 中）
	- Assimp 写出的绝对 buffer URI 归一化为相对路径
	- `JOINTS_0` accessor 从 float 重编码为 `UNSIGNED_SHORT`
	- `skin.skeleton`、`inverseBindMatrices`、scene roots 与 mesh roots 的 writer 输出归一化
	- 动画节点 `matrix -> TRS` 归一化与多 animation entry 合并为单 clip
	- 基于 declared skeleton root 的 formal glTF 结构校验与失败即中止

这意味着当前 formal pipeline 已经不再依赖“后处理猜 root”的旧模式。writer normalization 仍然存在，但它消费的是 exporter 预先声明的 formal root 和 formal contract，而不是重新定义 contract。

### 当前正式命名规则

为避免动画、技能和报告各自使用不同名称，当前 formal deliverable 命名固定为：

- 模型：`Model/{character_id}.gltf` 与同名 `.bin`
- 动画：`Animations/{deliverable_anim_file}.gltf` 与同名 `.bin`，其中 `deliverable_anim_file` 直接来自 shared formal animation resolution
- 技能：`Skills/skill_config.json`
- 纹理：`Textures/{texture_stem}.png`，其中 `texture_stem` 与材质清单中的 `exportedFile` 一致

角色报告、技能配置中的动画元数据和目录中的实际文件名必须复用同一命名结果，不允许额外推断。

### 角色验收报告最小 schema

为了让报告真正可用于回归和诊断，角色级报告除了成功/失败外，还应至少包含：
- `assembly_profile`
- `animation_source_anibnd`
- `skill_source_anibnd`
- `request_tae_id`
- `resolved_tae_id`
- `resolved_hkx_id`
- `source_anim_file`
- `deliverable_anim_file`
- `formal_skeleton_root`
- 每个子项的 `success`、`failure_code` 和 `failure_message`

这样才能把“请求错了”“求解错了”“交付命名错了”“骨架根错了”区分开，而不是全部坍缩为一个 `export failed`。

## Open Questions

- 是否需要为极少数无法无损转为 PNG 的 HDR/特殊纹理定义单独的正式异常策略，还是统一视为当前版本不支持并显式失败
- 现有 UE 侧 glTF 动画导入器在应用新合同后，是否还需要保留与历史导出物兼容的读取分支
- 是否需要显式定义 preview parity/debug export 模式，用于未来复刻 DSAnimStudio 当前可视装配状态，但明确排除在 formal contract 之外
## 上下文

当前 UE 侧除了 `SekiroImportCommandlet` 之外，还存在 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 等独立代码路径。它们分别承担补救、校验或验收的一部分职责，但这些职责与正式导入主链重叠，导致实现分散、边界模糊。与此同时，`SekiroImportCommandlet` 本身通过 Interchange 直接把 glTF 模型和动画导入为最终 `USkeletalMesh`、`USkeleton` 和 `UAnimSequence` 资产，然后只做结果验证，例如 torso chain、可见人形姿态、动画覆盖率和 root motion 开关。该链路的优点是简单，但它有四个根本限制：

1. 它没有正式的数据阶段来对模型、骨骼和动画应用同一组横向镜像、坐标系矩阵和最终 UE 对象空间朝向修正。
2. 它没有在 UE 内重建 Sekiro 骨架到 Unreal Humanoid 目标层级，只是假定导出侧已经给出足够接近的骨架组织。
3. 它没有在父子关系改变后逐帧重算动画局部变换，因此无法证明重组后的 `USkeleton` 和 `UAnimSequence` 仍然是严格一致的。
4. 它把导入后的修正和检查分散到独立 commandlet/验证器里，导致正式路径与实际验收路径不一致。

最近针对 `RTG_SekiroToManny` 的诊断进一步证明了这一点：IK Retargeter 资源中的手腕/手掌朝向问题可以被 pose probe 直接观察到，但那类问题的根源仍然是导入侧动画与骨架归一化语义不完整，而不是“正式路径应该改由 RTG 或后置 retarget 来修”。一旦 `SekiroImportCommandlet` 真正输出了符合 Unreal Humanoid 组织且逐帧局部轨道已重算的 Skeleton/Animation，RTG 侧只应承担诊断与验收职责，而不应承担正式修复职责。

最近一轮端到端执行把问题进一步收敛了：formal exporter release、17 clip preview re-export、UE import commandlet 重跑、import report host-side 验收和 import-side branch-basis 检查已经跑通；RTG 静态 pose validator 也通过。剩余失败只出现在 RTG 动态动画 validator，并且在把 arm chain 延伸到 hand 之后，之前的 `forearm -> hand` 链段方向失败已经消失，失败收敛为 palm forward、palm lateral 和 palm normal。再结合 importer palm-basis rebase 已把右手 dynamic palm basis 拉回阈值内、左手仍显著低于阈值这一结果，当前观察到的残留问题已经不再像“左右手共用的 generic wrist offset”，而更像“镜像侧 hand / finger subtree 的局部旋转语义仍未完全归一化”。这说明当前观察到的残留问题不是“正式 import 主链没有完成”，而是“RTG 对 wrist/hand 动态旋转语义的观测结果仍显示存在 mirrored-side palm basis 传播不一致”。

从当前实现来看，这个结论还有一层更具体的含义：`BuildHandComponentRotationRebases` / `ApplyComponentRotationRebasesToWorldMap` 目前直接修改的是 `Hand` 骨骼的 component rotation，然后再基于现有 world map 回推 local bind / local track；但它没有显式把同一 basis 修正传播为 finger-root subtree 的整体 world-space 旋转/位移更新。对于 palm basis 这类依赖 `Hand -> MiddleFingerRoot` 和 `IndexFingerRoot -> PinkyFingerRoot` 几何关系的验收，这意味着“只把 hand 自身朝向掰正”与“让整个 hand/finger subtree 的动态语义同步归一化”并不等价。右手已经回到阈值内而左手镜像侧仍失败，说明剩余缺口很可能就在这类 mirrored subtree propagation 语义上。

这里提到的“targeted wrist compensation in the RTG build step”，并不是引入新的正式修复路径，而是一个受控的诊断实验：在 `build_sekiro_to_manny_retarget_assets.py` 中，对 target wrist 或 hand 相关链施加一组确定性的局部旋转补偿，用来判断 RTG 动态误差是否主要表现为常量腕部局部偏置。如果这种补偿能显著改善 palm basis 指标，那么它证明正式导入路径里仍缺少与 wrist basis 对齐等价的语义；反之，如果补偿无效，或者只能改善一侧而另一镜像侧仍失败，则问题更可能在镜像侧 hand/finger subtree 的局部轨道语义、局部基底传播或 probe 观测配置。无论结果如何，这种补偿都只能作为 post-import 诊断手段，不能构成 formal success，也不能替代 `SekiroImportCommandlet` 内的动画重算实现。

因此本次变更应当删除这些冗余 commandlet/验证器，把必要能力整合进一个正式、单路径、不可回退的导入归一化流程。若需要检查结果，则在 import 结束后调用按需检查逻辑，而不是维持独立的长期并行 commandlet。

约束如下：

- 正式路径不能依赖猜测、兼容分支、人工后处理或“导入成功后再单独 retarget”。
- 归一化必须同时覆盖模型、骨架、动画，不能只修骨架名字或只修动画轨道。
- 动画归一化验收必须覆盖分支基底语义，尤其是 hand / wrist / finger-root 的掌面横向和掌面法线；不能只用 arm chain 主方向通过就视为动画重算正确。
- hand / wrist / finger-root 分支的验收必须按左右侧独立记录；若一侧通过而另一镜像侧失败，必须继续把问题归因到 mirrored-side local-semantics 缺口，而不是以“整体已经接近”判定通过。
- 如果 RTG 静态 pose 已通过而动态 animation validator 仍失败，必须把该失败解释为“动态局部轨道语义仍未完全证明正确”；禁止因为静态 pose 正确就把 formal import 或 retarget 视为完成。
- 冗余 commandlet/验证器需要删除，不能在实现后继续保留与正式路径重复的旧代码。
- 需要保留现有 formal import 的严格失败语义，任何缺失、歧义或结果不一致都必须硬失败。

## 目标 / 非目标

**目标：**
- 在 `SekiroImportCommandlet` 中引入明确的一步式 Humanoid Normalization 导入流程，并把它纳入正式导入成功条件。
- 以一份确定性的归一化配置定义目标骨架父子关系、横向方向镜像矩阵规则和统一矩阵变换，使同一角色的模型、参考姿态和动画都经过完全一致的处理，并在 UE 中额外保持绕对象 Z 轴 180° 的最终朝向。
- 在骨架重组后，重新计算参考姿态局部变换、bind pose / inverse bind pose，以及每个动画 clip 的逐帧局部骨骼轨道。
- 删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 等重复职责代码，并把必要的检查改为 import 后按需执行的独立检查流程。
- 扩展导入报告和 import 后检查，证明最终资产满足 Unreal Humanoid 组织、横向镜像结果正确性、方向正确性和动画绑定完整性。

**非目标：**
- 不把本次变更变成通用任意骨架 retarget 系统；范围仅限正式 Sekiro humanoid 导入链路。
- 不引入运行时自动修正；所有归一化都在离线 commandlet 导入阶段完成。
- 不继续保留“仅基于源 glTF 数据直出资产也算正式成功”的旧合同。
- 不保留旧的独立 retarget/验证 commandlet 作为正式路径旁路。

## 决策

### 决策 1：采用“一步导入 + 内存归一化生成”而不是“两阶段导入”

正式链路必须保持为单一步骤：

1. `Read Source Data`：读取 glTF 模型、骨架和动画所需的源数据，但不发布中间资产。
2. `Normalize In Memory`：在内存中完成沿角色横向方向的镜像矩阵变换、基础矩阵变换、最终对象空间绕 Z 轴 180° 的统一朝向修正、目标父子关系重建、参考姿态重算和动画逐帧局部轨道重算。
3. `Generate Final Assets`：直接由内存中的归一化结果生成最终 `USkeletalMesh`、`USkeleton` 和 `UAnimSequence`，然后输出报告并在需要时执行 import 后检查。

这样做的原因是正式路径不应创建和维护一套未归一化直出资产再进行第二次构建。源数据可以先读入内存，归一化过程也可以在内存中完成，并只落最终正式资产。与“两阶段导入”相比，这样可以避免中间资产污染内容目录、避免未归一化资产与正式资产并存，也更符合你要求的单路径导入模型。与直接在已经落盘的最终资产上原地修补相比，内存归一化仍然保留了清晰的数据边界和可重复性。

备选方案：
- 直接修改导出器，让导入器继续只做验证。否决，因为用户明确要求导入工具/commandlet 负责该能力，且需要 UE 侧最终骨架组织作为正式结果。
- 采用“两阶段导入”，先生成未归一化直出资产再生成正式资产。否决，因为用户明确要求禁止这种方案，正式路径必须一步完成。
- 导入后继续调用独立 retarget 命令。否决，因为这会把正式成功拆成多个步骤，且无法统一处理模型 bind pose、骨架层级和动画轨道。
- 依赖 `RTG_*` / IK Retargeter 资源作为正式修复链路。否决，因为 RTG 只能观察或放大导入语义错误，不能替代正式 import 对目标骨架与逐帧局部轨道的一次性构建。
- 保留现有独立验证 commandlet/验证器并在新链路外继续维护。否决，因为它们与正式导入共享同一语义域，只会制造重复逻辑和分叉验收标准。

### 决策 2：使用确定性的 Humanoid 归一化配置，而不是运行时启发式猜测

新增 `FSekiroHumanoidNormalizationProfile` 一类的数据结构，至少定义：

- 全局基础变换矩阵 `M_basis`，用于把源坐标系映射到 UE Humanoid 目标坐标系。
- 横向镜像矩阵 `M_mirror`，用于沿角色横向方向执行整体镜像；在 Unreal 结果中该方向必须对应 X 轴，源 glTF 侧的横向轴必须通过源骨架左右手连线等几何关系分析确定，而不是硬编码假设。
- 最终对象空间朝向矩阵 `M_objectYaw180`，用于在完成基础坐标系映射和横向镜像后，再对模型、骨架和动画统一追加一次绕 UE Z 轴的 180° 旋转。
- 目标父子关系表 `TargetParent[BoneName]`，显式声明最终 Skeleton 中每个受支持骨骼的目标父节点。
- 必选骨骼集合，缺失时必须硬失败。
- 非 Humanoid 辅助骨骼的保留策略，必须是显式映射，而不是“挂到最近的父骨骼”这类猜测行为。

这样做可以满足正式路径不能依赖猜测的约束，并让导入失败点可解释。与之相比，按骨骼命名做 `L/R` 对调、动态猜测父子关系或直接猜一个固定横向轴，只会把导入结果建立在不可验证的启发式之上。

### 决策 3：动画重算以“保持归一化后世界姿态”为准则，重新求局部轨道

对于参考姿态和每一帧动画，都执行同一套步骤：

1. 从原始 Skeleton / AnimSequence 读取源局部变换。
2. 对每个骨骼应用 `M_mirror`、`M_basis` 和 `M_objectYaw180`，得到归一化后的源世界姿态；其中 `M_mirror` 表示沿角色横向方向的整体镜像，而不是按骨骼名称进行对调。
3. 按新的目标父子关系自顶向下重建局部变换。

重建公式为：若归一化后某骨骼在时间 `t` 的目标世界变换为 $W_b(t)$，其目标父骨骼为 $p$，则新的局部变换为

$$
L_b(t) = W_p(t)^{-1} \cdot W_b(t)
$$

参考姿态同理，只是时间维度退化为单帧 bind pose。该决策保证更换父节点后，骨骼在世界空间中的期望人形姿态保持一致，而不是简单复制旧局部轨道导致姿态漂移。

对于 hand / wrist / finger-root 分支，验收不能只看 `clavicle -> hand`、`upperarm -> lowerarm` 一类主链方向，还必须证明重算后的局部轨道在世界空间中保留了掌面基底：

- hand 到 middle finger root 的掌面前向
- index finger root 到 pinky finger root 的掌面横向
- 由二者叉积得到的掌面法线

如果这些基底在归一化后与源动画的目标语义不一致，即使 arm chain 主方向仍然接近，也必须视为动画重算未完成。

并且该判断必须按左右侧分别成立。对于 mirror 后的人形分支，不能因为右手或非镜像侧已经通过，就把左手或镜像侧剩余失败降级解释成同一个通用 wrist offset。只要任一侧的 palm forward、palm lateral 或 palm normal 仍低于阈值，就必须继续定位该侧 local bind / local track / finger-root subtree 的归一化语义。

对当前实现而言，这种“继续定位”不能只停留在 hand 自身旋转补偿，而必须进一步区分两类语义是否都被修正：

- hand bone 自身的 component/local rotation frame 是否对齐
- 由 hand 到 finger-root subtree 构成的世界空间几何关系是否也沿同一 basis 被传播和重建

如果前者已经改善而后者没有同步改善，那么 palm basis 指标仍然会失败，并且这种失败更容易在 mirror 后的单侧分支上暴露出来。

备选方案：
- 直接复制原始局部轨道到新父节点。否决，因为父节点改变后局部轨道语义已变，结果必然错位。
- 仅对根骨做整体矩阵乘法。否决，因为用户要求骨架父子关系重组后所有受影响骨骼和动画帧都要重新计算。
- 通过 `L_* <-> R_*` 命名表做骨骼交换。否决，因为目标不是按名字对调骨骼，而是沿角色横向方向做统一镜像矩阵变换。

### 决策 4：模型归一化必须重建 bind pose，并修正镜像后的几何手性

归一化后的 `USkeletalMesh` 不能只换一个 Skeleton 引用，而必须和新参考姿态一起重建：

- 顶点位置、法线、切线必须应用同一 `M_basis`、`M_mirror` 和 `M_objectYaw180` 规则。
- 若矩阵行列式为负，必须同步修正三角面绕序或切线手性，避免法线反转。
- 绑定骨骼名、权重、参考骨架和 inverse bind pose 必须全部对应到归一化后的骨架。

备选方案是只更新 `USkeleton` 而保持网格 bind pose 不变；这会导致蒙皮和动画在 UE 中出现系统性错位，因此不能接受。

### 决策 5：检查从内嵌冗余验证器升级为 import 后按需检查

`SekiroHumanoidValidation` 和 `SekiroValidationCommandlet` 不再作为长期保留的独立代码路径。正式实现改为两层：

1. import 主链内部只保留构建成功所必需的最小失败条件。
2. 如需进一步验收，则在 import 完成后调用按需检查逻辑，对归一化产物执行明确的 post-import 检查。

post-import 检查至少覆盖：

- 目标 Skeleton 的父子关系是否与归一化配置完全一致。
- 横向镜像后的手臂、肩部和其他人形横向标志是否与归一化后的角色左/右语义一致，且最终对象空间结果已额外绕 Unreal Z 轴旋转 180°。
- 每个导入动画是否绑定到归一化 Skeleton，且轨道集合覆盖所有必需骨骼。
- 每个导入动画在 hand / wrist / finger-root 分支上的掌面前向、掌面横向和掌面法线是否与源动画归一化目标一致，而不是只通过上臂/前臂链段方向检查。
- 上述 hand / wrist / finger-root 指标是否按左右侧分别记录、分别比较，并在一侧通过另一侧失败时直接暴露侧别差异，而不是折叠成一个总分或单一 wrist offset 假设。
- 模型的参考姿态、网格 bounds 和脚底落点是否仍满足当前可见姿态/grounding 约束。
- 导入报告是否记录源数据读取、内存归一化、最终资产生成、检查失败点的分阶段结果。

`RTG_*` pose / animation probes 可以继续保留，但它们在规范中属于 post-import 诊断与验收工具。若 probe 暴露 wrist/hand 问题，修复应回到 `SekiroImportCommandlet::RewriteAnimationFromTranslatedSource` 及其共享归一化数据，而不是改由正式 retarget 资产兜底。

若需要在 RTG builder 中增加针对 wrist 的定向补偿、额外日志或持久化探针，其唯一允许目的也是帮助判断剩余误差是否属于恒定局部 wrist basis 偏置。该类补偿必须满足：

- 必须是显式、确定性、可关闭的诊断实验，而不是隐式兜底逻辑。
- 必须在设计和报告里标识为 RTG 诊断步骤，而不是 formal import 或 final retarget success 的一部分。
- 一旦实验确认了缺失的旋转语义，正式修复必须回写到 import commandlet 的归一化配置、bind pose 重建或逐帧局部轨道重算流程。
- 如果实验没有改善指标，则必须撤回该诊断补偿，继续从骨架拓扑、轨道语义或 probe 定义上定位，而不是把无效补偿长期留在 RTG 构建脚本里。

这意味着现有 `ValidateImportedVisibleHumanoidPose` 一类逻辑不能以原样保留为独立组件；若其中有必要规则，应重写或并入新的 post-import 检查实现。

## 风险 / 权衡

- [镜像矩阵导致几何手性翻转] → 在网格重建阶段显式检测负行列式，并同步修正绕序、法线和切线。
- [额外 Z 轴 180° 后仍沿用绝对 X 正负号解释左/右] → 验收统一按归一化后角色朝向的左/右语义解释，避免把绝对坐标符号误当成左右侧结论。
- [骨骼重组后动画重算成本高] → 按角色和 clip 分批保存，沿用 commandlet 现有的 GC/批量处理模式，并在报告中区分失败 clip。
- [部分 Sekiro 骨骼无法无损映射到目标 Humanoid 组织] → 通过显式配置区分“正式支持骨骼”和“必须保留的辅助骨骼”；未在配置中的关键骨骼直接失败，不使用临时兜底。
- [内存归一化过程失败后难以定位问题] → 报告中显式记录源数据读取、骨架重组、网格生成、动画重算的阶段性失败点，但不生成原始中间资产。
- [删除旧 commandlet/验证器后遗漏必要检查] → 先识别可复用的最小分析逻辑，再并入 import 主链或新的 post-import 检查模块，不保留旧入口。
- [RTG 诊断结果被误当成正式修复入口] → 在规范中明确 RTG/IK Retargeter 仅用于诊断与验收，所有正式修复必须回到 import commandlet 的骨架归一化与逐帧动画重算实现。
- [静态 pose 通过但动态 palm basis 仍失败，被误判为“只差一点”] → 规范中必须把动态 palm basis failure 视为未完成状态，并要求通过 frame-level wrist/hand diagnostics 继续定位，而不是以静态 pose 正确或 arm chain 部分收敛作为完成依据。
- [右手恢复后把左手镜像侧失败继续解释成通用 wrist offset] → 诊断和报告必须按侧别拆分；一旦结果表现出单侧残差，就必须把问题回溯到 mirrored-side subtree 的 local-semantics，而不是继续沿用双侧统一补偿假设。

## Migration Plan

1. 为导入 commandlet 增加源数据读取与内存归一化生成入口，并引入归一化配置数据结构。
2. 把骨架重组、参考姿态重建、动画逐帧重算和网格 bind pose 更新拆成独立的内存处理模块，先跑单角色样本验证。
3. 删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 及其入口，把保留下来的必要检查重写为 import 后按需检查。
4. 扩展导入报告，记录内存归一化后的骨架关系、动画覆盖和横向镜像/方向校验结果。
5. 用现有 c0000 样本批次执行一步式模型/动画导入和 post-import 检查，确认正式产物直接生成为“归一化 UE Humanoid 结果”。
6. 移除或拒绝旧的“只验证不归一化也可成功”判断分支。

回滚策略：在归一化路径无法稳定通过之前，不变更导出合同；如需回滚，实现上只需禁用新变更引入的发布阶段并恢复旧导入验证门槛，但该回滚不应在正式归档后的规范中长期保留。

## Open Questions

- Unreal Humanoid 目标组织是否需要保留 `Master/RootPos/RootRot*` 作为非人形包装骨，还是正式产物必须收敛为单一 Humanoid 根节点。这会影响网格 root motion 和技能系统引用路径，需要在实现前确认。
- 对于没有标准左右配对的武器、义手或 Dummy 辅助骨骼，正式配置是保留原命名并挂到显式拥有者，还是同样沿分析出的横向轴参与整体镜像，需要在实现前确认。
- 是否需要额外导出调试快照来帮助分析内存归一化失败；如果需要，必须保证这类快照不是正式资产，也不会参与内容引用。
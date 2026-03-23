## ADDED Requirements

### 需求:UE 导入必须对模型骨架动画执行统一的人形归一化
UE 正式导入 commandlet 必须对同一角色的模型、骨架和动画应用同一份确定性的横向镜像矩阵、基础矩阵变换和最终 UE 对象空间 Z 轴 180° 旋转，禁止只变更其中一类资产或在不同资产类型上使用不一致的变换。

#### 场景:导入角色模型与动画时执行统一归一化
- **当** 导入 commandlet 处理一个包含模型和至少一个动画的正式角色导出目录
- **那么** 它必须使用同一份归一化配置对 SkeletalMesh、USkeleton 和全部 UAnimSequence 执行沿角色横向方向的镜像矩阵变换、基础矩阵变换和最终 UE 对象空间 Z 轴 180° 旋转，并且生成的正式资产必须共享同一个归一化后 Skeleton

### 需求:UE 导入后的最终对象空间必须额外绕 Z 轴旋转180度
UE 正式导入 commandlet 必须把归一化完成后的最终对象空间结果再统一绕 Unreal Z 轴旋转 180°，并且该旋转必须同时作用于模型、参考骨架和动画轨道，不能只改根节点显示或只改单一资产类型。

#### 场景:发布带额外 Z 轴 180 度旋转的归一化资产
- **当** 导入 commandlet 完成角色的横向镜像、基础矩阵变换和骨架重组
- **那么** 它必须对最终用于生成 SkeletalMesh、USkeleton 和 UAnimSequence 的统一归一化结果额外追加一次 Unreal Z 轴 180° 旋转，使导入到 UE 后的整体对象坐标系满足最新验收方向

### 需求:UE 导入必须通过源骨架几何关系确定镜像轴
UE 正式导入 commandlet 必须分析源 glTF 骨架中代表角色左右展开方向的横向轴，并以该轴构造镜像矩阵；在 Unreal 结果中，该镜像后的横向语义必须对应 X 轴，禁止直接假定 glTF 的某个固定坐标轴天然等于角色横向方向。

#### 场景:根据源骨架左右手方向确定镜像轴
- **当** 导入 commandlet 读取正式角色的源骨架数据
- **那么** 它必须通过左右手、肩部或其他明确的人形横向几何关系分析出源骨架的横向方向，并基于该方向构造统一镜像矩阵，而不是仅按 glTF 坐标轴名做静态判断

### 需求:UE 导入必须一步完成内存归一化与最终资产生成
UE 正式导入 commandlet 必须先读取所需源数据，在内存中完成归一化和骨架重组，然后直接生成最终模型、骨骼和动画资产；禁止采用先导入原始中间资产、再进行第二阶段重建的两阶段导入方案。

#### 场景:一步生成归一化正式资产
- **当** 导入 commandlet 处理正式角色导出目录
- **那么** 它必须先在内存中完成模型、骨骼和动画的归一化处理，再一次性生成最终 `USkeletalMesh`、`USkeleton` 和 `UAnimSequence`，且不得把未归一化直出结果作为中间正式资产写入内容路径

### 需求:UE 导入必须重建 Unreal Humanoid 目标骨架组织
对于正式支持的 Sekiro humanoid 角色，UE 导入 commandlet 必须按显式定义的目标父子关系重建骨骼层级，禁止依赖未归一化直出结果的现有父子关系或运行时启发式猜测来决定最终 Humanoid 组织。

#### 场景:按目标父子关系生成归一化 Skeleton
- **当** 导入 commandlet 识别到角色属于正式支持的人形骨架配置
- **那么** 它必须根据显式目标父子关系表生成归一化后的参考骨架；如果任何必需骨骼缺失、重复或映射歧义，则必须直接失败

#### 场景:导入时必须直接产出 UE humanoid 四肢主链
- **当** 导入 commandlet 为正式支持的 Sekiro humanoid 角色重建目标层级
- **那么** 它必须直接产出 `L/R_Clavicle -> L/R_UpperArm -> L/R_Forearm -> L/R_Hand` 与 `L/R_Thigh -> L/R_Calf -> L/R_Foot -> L/R_Toe0` 这两组 UE humanoid 四肢主链；Sekiro 原始的 `L/R_Shoulder`、`L/R_Knee` 等中间骨只能作为保留的辅助分支存在，不能继续决定正式主链父子关系

#### 场景:导入后必须直接校验 canonical 角色资产层级
- **当** formal import 完成并写出 `/Game/SekiroAssets/Characters/c0000` 一类正式角色资产
- **那么** post-import self-test probe 必须直接读取该 canonical 内容路径下的 `USkeletalMesh` / `USkeleton` / `UAnimSequence`，并校验躯干链、双侧 arm 主链、双侧 leg 主链以及 `face_root -> Head`；不得继续从旧的 `SelfTest` 内容路径读取陈旧资产并据此宣告通过或失败

### 需求:骨架重组后必须逐帧重算动画局部轨道
在骨骼父子关系被重组后，UE 导入 commandlet 必须依据新的目标层级重新计算每个受影响骨骼在每一帧的局部位置、朝向和缩放，禁止直接复用旧父子关系下的局部动画轨道。

#### 场景:导入动画到重组后的 Skeleton
- **当** 一个动画 clip 被导入到已完成 Humanoid 骨架重组的角色 Skeleton
- **那么** 命令链路必须基于归一化后的世界姿态重新求解新层级下的局部骨骼轨道，并将结果写入绑定到归一化 Skeleton 的 UAnimSequence

#### 场景:formal 支持骨骼缺少 baked 轨道时必须失败
- **当** source animation 在某个 formal 支持骨骼上存在 baked 运动语义，但导入侧没有拿到有效 payload、没有生成对应 local track，或只能回退到 bind local、time-zero 或 target reference pose
- **那么** 命令链路必须把该 clip 判定为失败，并在报告中记录缺失骨骼与回退原因；不得导入一个“成功绑定但该骨骼实际被冻结”的动画结果

#### 场景:Sekiro 自身动画必须保持归一化后世界运动等价
- **当** 一个 Sekiro 自身动画 clip 被导入到重映射后的同角色 Skeleton
- **那么** 系统必须把 source normalized world pose 视为真值，并验证 imported target world pose 在 root motion、torso/limb 方向、hand / wrist / finger-root 分支基底以及 formal 支持的代表性辅助分支上，与该真值保持一致；只满足“能播放”或“已绑定”不得视为正确匹配

#### 场景:root motion 不得在重映射后与肢体运动脱节
- **当** 一个正式支持的动画 clip 含有可观察的根位移、转向或落点变化
- **那么** 导入后的动画必须在额外 Z 轴 180° 后仍保持与 source normalized motion 一致的 root motion 位移与朝向增量；如果角色整体运动方向、转向或落点与肢体动作语义脱节，则必须判定为失败

#### 场景:手腕与手掌分支必须按重算后的世界姿态保持正确
- **当** 一个正式支持的人形动画包含 hand、wrist 和 finger-root 分支
- **那么** 命令链路必须保证重算后的 `hand -> middle-finger-root` 掌面前向、`index-finger-root -> pinky-finger-root` 掌面横向，以及由二者得到的掌面法线，与归一化后的源动画世界姿态保持一致；只保留 arm chain 主方向但丢失掌面基底的结果必须判定为失败

#### 场景:左右镜像 hand branch 必须独立满足局部语义
- **当** 左右 hand / wrist / finger-root 分支在 mirror 后表现出不同的动态 palm basis 误差
- **那么** 命令链路必须对左右侧分别保留并验证掌面前向、掌面横向和掌面法线语义；禁止把单侧残差继续解释为对两侧都适用的通用 wrist offset，也禁止仅因另一侧已通过就接受镜像侧失败

#### 场景:正式支持的辅助分支不得静默丢失动画语义
- **当** weapon、sheath、dummy、twist、face-root 或其他被正式支持的辅助骨骼在骨架重组后仍参与 Sekiro 自身动作语义
- **那么** 归一化流程必须为这些骨骼提供显式 parent 策略和逐帧 local-track 重算；若某骨骼未被正式支持，则必须在配置期或导入期直接失败，而不是让该分支以 reference-like 静止状态混入正式成功结果

#### 场景:代表性辅助分支必须显式落入 formal world-space 检查
- **当** formal import 为 Sekiro 自身动画输出 world-space 语义验收结果
- **那么** 它必须至少对 `L_Clavicle`、`R_Clavicle` 以及当前角色存在的 `face_root` / `FaceRoot`、`W_Sheath` / `weapon_sheath` 等代表性辅助分支输出显式 world-space 差异；这些指标不得再以“可选但不记录”的方式缺席报告

#### 场景:辅助分支网格尖刺必须能被独立定位
- **当** 导入后的 Sekiro 自身动画在主干骨架语义通过时仍出现局部网格尖刺、飘带/头发/挂点分支炸开或大范围离体
- **那么** 系统必须提供面向代表性辅助分支的独立诊断产物，能够把异常定位到具体 branch root（例如 clavicle、face_root、weapon_sheath）及其动画帧，而不是只给出笼统的 self-animation mismatch 结论

### 需求:RTG 资源只能作为导入后的诊断工具
正式 UE 导入链路如果使用 IK Retargeter 资源、retarget pose probe 或 retarget animation probe 观测归一化结果，则这些资源必须只用于诊断与验收，并且禁止承担正式修复职责。

#### 场景:RTG 诊断发现 wrist 或 hand 朝向问题
- **当** 导入后的 RTG pose probe 或 animation probe 暴露 wrist、hand 或 finger-root 朝向错误
- **那么** 修复必须回到 `SekiroImportCommandlet` 的归一化配置、目标骨架组织和逐帧动画重算实现，而不能通过新增正式 retarget 资产步骤、导入后手工 retarget，或其他与 import 主链并行的修补路径来达成通过

#### 场景:RTG build step 使用 targeted wrist compensation
- **当** post-import RTG 诊断需要判断剩余 palm basis 误差是否主要表现为恒定 wrist/hand 局部空间偏置
- **那么** RTG build step 可以临时施加显式、确定性、可关闭的 targeted wrist compensation 作为诊断实验，但该补偿必须只用于定位误差来源，不能被记为正式修复，也不能成为 formal success、import success 或 final retarget success 的前置条件

#### 场景:targeted wrist compensation 只能改善一侧
- **当** RTG 诊断补偿只能让一侧 hand / wrist 分支回到阈值内，而另一镜像侧仍失败
- **那么** 系统必须将该结果解释为 mirrored-side local bind、local track 或 finger-root subtree 语义仍未对齐；不得把这种单侧改善宣告为通用修复成立，也不得把补偿固化为正式路径的一部分

#### 场景:RTG 静态 pose 正确但动态 palm basis 仍失败
- **当** retarget pose validator 通过，但 retarget animation validator 仍在 palm forward、palm lateral 或 palm normal 上低于阈值
- **那么** 该角色不得被判定为 retarget 完成；系统必须继续通过 frame-level wrist/hand diagnostics、局部轨道语义分析或 import-side 动画重算检查定位问题，而不能因为静态 pose 正确或 arm chain 主方向已收敛就视为通过

#### 场景:RTG 肩部预览残差来自骨架拓扑非等价
- **当** RTG Previewing Reference Pose 中 source 的 `L_Shoulder` / `R_Shoulder` 与目标 Manny 的 `clavicle_l` / `clavicle_r` 在肩根区域无法完全重合，但 `upperarm -> hand`、`forearm -> hand` 等等价链段指标已经通过
- **那么** 系统必须把该现象解释为 source/target 肩部拓扑非等价造成的结构性可视残差，而不是默认把它判定为 retarget 失败；此时真正的失败条件应继续落在等价链段方向、掌面基底和动态语义上

### 需求:模型 bind pose 必须与归一化骨架保持一致
归一化后的 SkeletalMesh 必须重建与目标 Skeleton 一致的参考姿态、骨骼绑定和几何手性修正，禁止只替换 Skeleton 引用而保留旧 bind pose。

#### 场景:镜像后发布 SkeletalMesh
- **当** 归一化流程对模型应用沿角色横向方向的镜像矩阵变换
- **那么** 发布阶段必须同步更新网格顶点、法线、切线、三角绕序和 inverse bind pose，使蒙皮结果与归一化 Skeleton 和动画一致

#### 场景:骨架语义通过但局部网格蒙皮仍被旧 inverse bind 拉飞
- **当** 导入后的 Skeleton 参考姿态、动画 world-space 语义与代表性辅助分支指标已经通过，但某个局部网格分区在动画播放中仍出现离体、尖刺或被错误骨骼空间拉伸的现象
- **那么** 系统必须把该问题继续定位为 SkeletalMesh skin binding / inverse bind pose 与归一化后 Skeleton 不一致，而不是把它降级解释为“只是可视化误差”；正式路径必须重建并校验导入后 SkeletalMesh 的 inverse bind 矩阵与最终参考骨架 world bind 一致

## 修改需求

### 需求:正式导入成功必须显式依赖归一化产物与 C++ 后检查

`SekiroImportCommandlet` 的正式成功判定必须直接依赖归一化后的 `USkeletalMesh`、`USkeleton`、`UAnimSequence` 以及 C++ post-import 检查结果；禁止把旧直出资产、旧内容目录或任何导入前已存在的非归一化产物当成成功依据。

#### 场景:归一化资产未完成校验时不得宣告成功
- **当** import commandlet 完成了模型或动画落盘，但归一化骨架层级、inverse bind、payload 覆盖率、world-space 语义或 self-animation 诊断尚未通过
- **那么** 报告必须把该角色判为失败，并显式记录是哪个归一化检查未通过；不得因为旧资产仍可加载、动画可播放或内容目录里已有同名资产就继续返回成功

#### 场景:formalSuccess=false 但导入所需 skeletal deliverable 完整
- **当** 导出包由于 `skills` 之类与 skeletal import 无关的 deliverable 缺失而 `formalSuccess=false`
- **那么** commandlet 可以继续执行受控的 skeletal import，但最终成功仍必须由归一化后的 mesh/skeleton/animation 检查决定；不得把“旧资产还能用”或“原始 glTF 可直接导入”解释为正式成功

### 需求:Sekiro 自身动画诊断必须由 C++ 导入链路生成可审计报告

`SekiroImportCommandlet` 必须在重写动画轨道时直接输出 clip 级和角色级的 self-animation 诊断，至少覆盖 payload 覆盖率、静默回退、direction dot、hand basis、world-position 差异、root-motion delta 与 representative auxiliary branch 指标，禁止继续依赖 Python validator 聚合正式结论。

#### 场景:导入角色动画后生成角色级 self-animation 诊断
- **当** commandlet 为正式支持角色导入至少一个 Sekiro 自身动画 clip
- **那么** 它必须在角色级报告中输出汇总的 `passed/failed clip` 计数、失败 clip 名单、聚合后的最差 direction/hand-basis/rotation 指标、最大 world-position/root-motion 误差，以及 payload 覆盖率与 fallback 统计

#### 场景:formal 支持骨骼发生静默回退时报告必须能直接定位
- **当** 某个 formal 支持骨骼缺少 baked payload、被合成为 bind/reference-like 轨道，或在重写后表现为冻结
- **那么** 报告必须把对应 clip、骨骼和 fallback 类型显式落盘，并将该 clip 计入失败；不得只在日志里留下模糊提示

### 需求:formal c0000 回归必须支持 canonical 17 动画预设并显式写入报告范围

当前 formal c0000 回归范围固定为 canonical 17 clip 预览批次，commandlet 必须提供无需手工复制长 CSV 的正式预设入口，并在 `ue_import_report.json` 中显式写出当前 animation selection scope，防止 17 clip 结果与历史全量导入报告混淆。

#### 场景:使用 canonical 17 预设执行 c0000 正式导入
- **当** 复核者对 c0000 执行正式 17 clip 导入回归
- **那么** commandlet 必须允许通过单一正式预设参数选中 `a000_201030`、`a000_201050`、`a000_202010`、`a000_202011`、`a000_202012`、`a000_202035`、`a000_202100`、`a000_202110`、`a000_202112`、`a000_202300`、`a000_202310`、`a000_202400`、`a000_202410`、`a000_202600`、`a000_202610`、`a000_202700`、`a000_202710` 这 17 个 clip，而不是要求每次手工复制完整 `AnimFilter` 列表

#### 场景:17 clip 预设报告必须显式区分于全量导入
- **当** commandlet 以 canonical 17 预设或等价 filter 集执行 c0000 导入
- **那么** `ue_import_report.json` 必须写出 `animationSelection.scope=canonical-c0000-preview-17`、`animationSelection.isCanonicalC0000Preview17=true`，并保证 `expectedAnimationCount` 与 `animationCount` 都等于 17

### 需求:UE 资产人工复核必须通过 UE 原生工具和正式内容路径完成

正式人工验收必须直接打开 `/Game/SekiroAssets/Characters/<chr>` 下的 canonical Skeleton、SkeletalMesh 与代表性动画资产，通过 UE 原生编辑器视图完成逐帧复核和抓帧记录；禁止继续依赖 Python 编辑器脚本构造旁路场景或读取陈旧路径。

#### 场景:人工复核 canonical skeleton 与 animation
- **当** 需要对 formal import 做人工并排复核
- **那么** 操作说明必须要求从 canonical 内容路径打开 Skeleton、SkeletalMesh 和代表性动画，在骨架树、预览窗口和关键帧位置直接检查躯干链、双侧四肢链、hand/finger subtree 与代表性辅助分支，而不是打开旧 `SelfTest`、旧 RTG probe 内容或 Python 临时场景

#### 场景:记录人工抓帧验证结果
- **当** 复核者在 UE 编辑器中对代表性动画进行关键帧检查
- **那么** 验收记录必须至少包含被检查的资产路径、关键帧位置、观察结论和抓帧产物路径，使“程序化通过但肉眼明显错义”的情况可以被追溯
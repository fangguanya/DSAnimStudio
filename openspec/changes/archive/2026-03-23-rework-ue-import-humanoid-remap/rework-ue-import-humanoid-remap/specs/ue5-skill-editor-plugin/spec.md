## ADDED Requirements

### 需求:导入报告必须记录人形归一化阶段结果
UE5 插件的正式导入报告必须区分源数据读取、骨架归一化、最终模型生成、动画重算和 import 后按需检查等阶段结果，禁止只输出单一成功/失败状态而无法定位归一化失败点。

#### 场景:生成角色级导入报告
- **当** 插件对单个角色执行正式导入
- **那么** 它必须在角色级报告中记录归一化配置、额外 Z 轴 180° 目标朝向、重建后的骨架层级检查、动画重算覆盖结果以及任何失败 clip 或失败骨骼的原因

#### 场景:报告必须暴露 Sekiro 自身动画错配的具体来源
- **当** 插件记录正式导入后的 animation validation 结果
- **那么** 它必须显式输出 formal 支持骨骼的 baked payload 覆盖率、bind/reference 静默回退计数、疑似冻结骨骼、root motion delta 差异、torso/limb direction 最小 dot、hand basis 最小 dot，以及正式支持的辅助分支差异；不得只给出一个笼统的 clip success/failure 布尔值
- **那么** 它还必须显式输出导入后 SkeletalMesh 的代表性 inverse bind 一致性指标，并区分“骨架/动画语义通过”与“mesh skin binding 仍错误”这两类问题；不得让后者继续被 `success=true` 掩盖

#### 场景:报告必须记录代表性辅助分支指标是否真的落盘
- **当** 插件生成角色级导入报告
- **那么** `postImportChecks` 必须显式记录代表性辅助分支 world-space 指标是否已对每个 clip 落盘；至少包括 `left_clavicle`、`right_clavicle`，以及当前角色存在的 `face_root` / `FaceRoot`、`W_Sheath` / `weapon_sheath` 等代表性辅助分支

## MODIFIED Requirements

### 需求:骨骼模型和动画导入契约
插件必须将角色资产正式导入为可播放、可检查且符合 Unreal Humanoid 组织的骨骼模型和绑定动画，不接受静态网格降级，也不接受未经过导入侧人形归一化的直接生成 Skeleton 结果作为正式完成状态；同时必须删除与正式路径重复的独立 retarget/验证 commandlet 或验证器代码。

#### 场景:模型导入结果必须是骨骼资产
- **当** 导入角色模型且导出目录中存在模型文件
- **那么** 导入结果必须至少创建 `USkeletalMesh`、`USkeleton` 和 `UPhysicsAsset`，并且 `USkeleton` 的参考骨架必须符合正式定义的 Humanoid 父子组织；如果只生成 StaticMesh、未生成 Skeleton、或 Skeleton 未完成人形归一化，则导入必须判定为失败

#### 场景:动画必须绑定到角色Skeleton
- **当** 导入角色动画
- **那么** 每个动画资产必须绑定到该角色的归一化 `USkeleton`，并与归一化后的模型骨骼名称和父子关系一一对应；未绑定 Skeleton、零轨道、错 Skeleton、仍保留重组前局部轨道语义，或在 hand / wrist / finger-root 分支上丢失掌面基底语义的动画都必须视为错误

#### 场景:formal 支持骨骼不得以 reference-like 静止状态混入成功结果
- **当** 某个 formal 支持骨骼在 source clip 中存在运动，但导入后的 clip 在该骨骼上缺少有效重算轨道、只保留 reference-like 静止状态或依赖 bind/time-zero 回退
- **那么** 插件必须把该 clip 标记为错误，并在角色级报告中记录具体骨骼、clip 与回退原因

#### 场景:角色级报告必须暴露左右侧 branch-basis 差异
- **当** 插件记录 hand / wrist / finger-root 分支的动画验收结果
- **那么** 它必须按左右侧分别输出 palm forward、palm lateral、palm normal 与相关阈值结果；若一侧通过另一侧失败，报告必须显式暴露该侧别差异，而不能只汇总为单一 branch-basis 成功/失败布尔值

#### 场景:角色级报告必须暴露 Sekiro 自身动画的 root motion 与关键分支差异
- **当** 插件记录自动画 formal 验收结果
- **那么** 它必须对代表性 clip 输出 root motion 位移/朝向差异，以及 torso、四肢、hand、代表性 weapon / dummy 分支的关键 world-space 差异，确保“mesh/skeleton 正确但整段动作不对”可以被报告直接解释

#### 场景:RTG 肩部预览不应使用非等价肩根重合作为唯一失败依据
- **当** 插件或诊断工具展示 RTG Previewing Reference Pose 的 source/target 骨架叠加结果
- **那么** 它必须把肩根区域的可视残差与真正的 retarget 失败条件区分开：若残差来自 `L_Shoulder/R_Shoulder` 与 Manny clavicle 拓扑非等价，而等价链段动态指标已通过，则该残差只能作为诊断说明，不能单独推翻 retarget 成功结论

#### 场景:import 后检查必须硬性失败
- **当** 在角色导入完成后运行按需检查
- **那么** 只要角色存在模型输入但缺少 SkeletalMesh、Skeleton、PhysicsAsset，存在未绑定/空轨道动画，或归一化后骨架层级、横向镜像结果、额外 Z 轴 180° 目标朝向、参考姿态校验失败，或 hand / wrist / finger-root 分支基底校验失败，检查必须返回失败状态，而不能把这类结果当作警告或可接受降级

#### 场景:无关 deliverable 不得污染 skeletal import 成功判定
- **当** 角色级导出包存在与 UE skeletal import 主链无关的 deliverable 缺失或失败，例如 `skills` / `params` 等非模型、非骨架、非动画输入
- **那么** `SekiroImportCommandlet` 必须继续基于当前请求所需的模型/纹理/材质清单/动画输入完成 skeletal import，并把这类无关问题记录为报告中的 warning；只要 skeletal import 主链本身通过，它们不得进入 blocking `errors`，也不得把 UE import report 顶层 `success` 置为 `false`

## REMOVED Requirements
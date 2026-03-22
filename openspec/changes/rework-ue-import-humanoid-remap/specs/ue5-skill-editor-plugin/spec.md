## ADDED Requirements

### 需求:导入报告必须记录人形归一化阶段结果
UE5 插件的正式导入报告必须区分源数据读取、骨架归一化、最终模型生成、动画重算和 import 后按需检查等阶段结果，禁止只输出单一成功/失败状态而无法定位归一化失败点。

#### 场景:生成角色级导入报告
- **当** 插件对单个角色执行正式导入
- **那么** 它必须在角色级报告中记录归一化配置、额外 Z 轴 180° 目标朝向、重建后的骨架层级检查、动画重算覆盖结果以及任何失败 clip 或失败骨骼的原因

## MODIFIED Requirements

### 需求:骨骼模型和动画导入契约
插件必须将角色资产正式导入为可播放、可检查且符合 Unreal Humanoid 组织的骨骼模型和绑定动画，不接受静态网格降级，也不接受未经过导入侧人形归一化的直接生成 Skeleton 结果作为正式完成状态；同时必须删除与正式路径重复的独立 retarget/验证 commandlet 或验证器代码。

#### 场景:模型导入结果必须是骨骼资产
- **当** 导入角色模型且导出目录中存在模型文件
- **那么** 导入结果必须至少创建 `USkeletalMesh`、`USkeleton` 和 `UPhysicsAsset`，并且 `USkeleton` 的参考骨架必须符合正式定义的 Humanoid 父子组织；如果只生成 StaticMesh、未生成 Skeleton、或 Skeleton 未完成人形归一化，则导入必须判定为失败

#### 场景:动画必须绑定到角色Skeleton
- **当** 导入角色动画
- **那么** 每个动画资产必须绑定到该角色的归一化 `USkeleton`，并与归一化后的模型骨骼名称和父子关系一一对应；未绑定 Skeleton、零轨道、错 Skeleton、仍保留重组前局部轨道语义，或在 hand / wrist / finger-root 分支上丢失掌面基底语义的动画都必须视为错误

#### 场景:角色级报告必须暴露左右侧 branch-basis 差异
- **当** 插件记录 hand / wrist / finger-root 分支的动画验收结果
- **那么** 它必须按左右侧分别输出 palm forward、palm lateral、palm normal 与相关阈值结果；若一侧通过另一侧失败，报告必须显式暴露该侧别差异，而不能只汇总为单一 branch-basis 成功/失败布尔值

#### 场景:import 后检查必须硬性失败
- **当** 在角色导入完成后运行按需检查
- **那么** 只要角色存在模型输入但缺少 SkeletalMesh、Skeleton、PhysicsAsset，存在未绑定/空轨道动画，或归一化后骨架层级、横向镜像结果、额外 Z 轴 180° 目标朝向、参考姿态校验失败，或 hand / wrist / finger-root 分支基底校验失败，检查必须返回失败状态，而不能把这类结果当作警告或可接受降级

#### 场景:无关 deliverable 不得污染 skeletal import 成功判定
- **当** 角色级导出包存在与 UE skeletal import 主链无关的 deliverable 缺失或失败，例如 `skills` / `params` 等非模型、非骨架、非动画输入
- **那么** `SekiroImportCommandlet` 必须继续基于当前请求所需的模型/纹理/材质清单/动画输入完成 skeletal import，并把这类无关问题记录为报告中的 warning；只要 skeletal import 主链本身通过，它们不得进入 blocking `errors`，也不得把 UE import report 顶层 `success` 置为 `false`

## REMOVED Requirements
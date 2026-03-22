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

### 需求:骨架重组后必须逐帧重算动画局部轨道
在骨骼父子关系被重组后，UE 导入 commandlet 必须依据新的目标层级重新计算每个受影响骨骼在每一帧的局部位置、朝向和缩放，禁止直接复用旧父子关系下的局部动画轨道。

#### 场景:导入动画到重组后的 Skeleton
- **当** 一个动画 clip 被导入到已完成 Humanoid 骨架重组的角色 Skeleton
- **那么** 命令链路必须基于归一化后的世界姿态重新求解新层级下的局部骨骼轨道，并将结果写入绑定到归一化 Skeleton 的 UAnimSequence

#### 场景:手腕与手掌分支必须按重算后的世界姿态保持正确
- **当** 一个正式支持的人形动画包含 hand、wrist 和 finger-root 分支
- **那么** 命令链路必须保证重算后的 `hand -> middle-finger-root` 掌面前向、`index-finger-root -> pinky-finger-root` 掌面横向，以及由二者得到的掌面法线，与归一化后的源动画世界姿态保持一致；只保留 arm chain 主方向但丢失掌面基底的结果必须判定为失败

#### 场景:左右镜像 hand branch 必须独立满足局部语义
- **当** 左右 hand / wrist / finger-root 分支在 mirror 后表现出不同的动态 palm basis 误差
- **那么** 命令链路必须对左右侧分别保留并验证掌面前向、掌面横向和掌面法线语义；禁止把单侧残差继续解释为对两侧都适用的通用 wrist offset，也禁止仅因另一侧已通过就接受镜像侧失败

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

### 需求:模型 bind pose 必须与归一化骨架保持一致
归一化后的 SkeletalMesh 必须重建与目标 Skeleton 一致的参考姿态、骨骼绑定和几何手性修正，禁止只替换 Skeleton 引用而保留旧 bind pose。

#### 场景:镜像后发布 SkeletalMesh
- **当** 归一化流程对模型应用沿角色横向方向的镜像矩阵变换
- **那么** 发布阶段必须同步更新网格顶点、法线、切线、三角绕序和 inverse bind pose，使蒙皮结果与归一化 Skeleton 和动画一致

## 修改需求
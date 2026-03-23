## ADDED Requirements

### 需求:导出骨骼变换表必须支持 Reparent 操作
导出管线必须支持在导出阶段将指定骨骼的 parentIndex 修改为另一个骨骼的索引，且骨骼在数组中的位置索引不变。Reparent 规则以 (骨骼名, 新父骨骼名) 对的形式配置。

#### 场景:RootRotY 被 reparent 到 Pelvis 下
- **当** 导出管线处理包含 `RootRotY` 和 `Pelvis` 骨骼的角色
- **那么** 导出产物中 `RootRotY`（或其重命名后的名称）的 parent 必须是 `Pelvis`（或其重命名后的名称），而非原始的 `RootPos`

#### 场景:目标骨骼不存在时跳过 reparent
- **当** Reparent 规则中引用的骨骼名在当前角色骨架中不存在
- **那么** 该条 reparent 规则必须被跳过，不影响其他骨骼的导出，并记录警告日志

### 需求:导出骨骼变换表必须支持 Rename 操作
导出管线必须支持在导出阶段将骨骼名映射为新名称。重命名必须一致地作用于：skeleton 节点名、mesh bone 引用名、animation channel 节点名。

#### 场景:脊柱链骨骼被重命名
- **当** 导出包含 `RootRotY`、`RootRotXZ`、`Spine`、`Spine1`、`Spine2` 的角色
- **那么** 导出产物中这些骨骼的名称必须分别为 `spine_01`、`spine_02`、`spine_03`、`spine_04`、`spine_05`

#### 场景:重命名不存在的骨骼时静默跳过
- **当** Rename 规则中引用的骨骼名在当前角色骨架中不存在
- **那么** 该条 rename 规则必须被跳过，不影响其他骨骼

### 需求:Reparent 后的 local transform 必须保持世界空间位置不变
当骨骼被 reparent 到新的 parent 下时，其 local transform 必须被重新计算，使得该骨骼的世界空间位置/朝向/缩放与 reparent 前完全一致。禁止直接使用原始 local transform 而不做修正。

#### 场景:Bind Pose 中的 local transform 重算
- **当** `RootRotY` 被从 `RootPos` 下 reparent 到 `Pelvis` 下（bind pose）
- **那么** `RootRotY` 的新 local transform 必须满足：`newLocal = inverse(Pelvis_local) * oldLocal`（在 source 空间中计算）

#### 场景:矩阵求逆失败时的 fallback
- **当** 新 parent 骨骼的 local transform 矩阵不可逆（退化矩阵）
- **那么** 必须保留该骨骼的原始 local transform 并记录错误警告，禁止导出崩溃

### 需求:Inverse bind matrix 必须从 Skeleton 节点树计算
mesh skinning 中的 inverse bind matrix 必须从已构建的 Assimp skeleton 节点树（由 HKX 或 FLVER 构建的同一数据源）计算世界矩阵，禁止从 FLVER.Nodes 独立计算。这确保 IBM 与 skeleton hierarchy 使用完全相同的数据源，消除 FLVER/HKX bind pose 差异导致的蒙皮偏移。

#### 场景:蒙皮骨骼的 offset matrix 一致性
- **当** 导出包含蒙皮 mesh 的模型
- **那么** 每个 bone 的 `OffsetMatrix`（inverse bind matrix）必须等于 `inverse(worldMatrix)`，其中 `worldMatrix` 从 skeleton 节点树逐级累乘 local transform 得到，禁止从 FLVER.Nodes 的 parent chain 独立计算

#### 场景:FLVER 与 HKX bind pose 存在差异的骨骼
- **当** 某骨骼的 FLVER bind pose 与 HKX bind pose 不一致（如 RootRotY/RootRotXZ 等纯旋转控制骨骼）
- **那么** IBM 必须以 skeleton 节点树的世界矩阵为准，不得使用 FLVER 的世界矩阵

### 需求:骨骼变换必须在坐标系转换之前应用
所有骨骼变换（reparent、rename、local transform 重算）必须在 source 空间中完成，然后再经过 `ConvertSourceMatrixToGltf()` 转换到 glTF 空间。禁止在 glTF 空间中执行 reparent 矩阵计算。

#### 场景:坐标系转换顺序
- **当** 导出管线处理 reparented 骨骼的 local transform
- **那么** 必须先在 source 空间完成 `newLocal = inverse(newParent_local_source) * old_local_source`，再将 `newLocal` 传入 `ConvertSourceMatrixToGltf()`

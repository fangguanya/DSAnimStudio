## 为什么

当前导出的骨骼层级中，`Pelvis` 和 `RootRotY` 是 `RootPos` 的平级子骨骼。这不符合 UE Humanoid 骨骼的标准拓扑结构——UE 期望 spine 链从 pelvis 延伸而非与之平行。此外，原始 FromSoftware 骨骼名（RootRotY、RootRotXZ 等）语义不明确，不利于 UE Retarget/IK 流程识别。现在需要在导出阶段解决这两个问题，使导出产物可直接用于 UE Humanoid 标准流程。

## 变更内容

1. **骨骼层级重构（导出时）**：将 `RootRotY` 从 `RootPos` 的子骨骼移动为 `Pelvis` 的子骨骼。仅影响导出产物，不修改原始 FLVER/HKX 数据。
2. **Bind Pose 本地变换重算**：`RootRotY` 的 parent 变更后，其 local transform 必须重新计算以保持世界空间位置不变。公式：`newLocal = inverse(Pelvis_local) * oldLocal`（因为旧 parent 和新 parent 的 parent 相同，均为 `RootPos`）。
3. **动画帧级变换重算**：对每一帧动画，`RootRotY` 的 local transform 同样需要按上述公式重算，确保动画效果不变。
4. **蒙皮权重索引一致性**：骨骼在数组中的索引不变（仅修改 parentIndex），但 inverse bind matrix 依赖 parent chain，必须用新的 parent chain 重新计算。
5. **骨骼重命名（导出时）**：**BREAKING** — 脊柱链骨骼导出名变更：
   - `RootRotY` -> `spine_01`
   - `RootRotXZ` -> `spine_02`
   - `Spine` -> `spine_03`
   - `Spine1` -> `spine_04`
   - `Spine2` -> `spine_05`

## 功能 (Capabilities)

### 新增功能
- `export-bone-hierarchy-transform`: 导出时骨骼层级重构引擎——支持 reparent、rename、local transform 重算，作用于 skeleton/animation/skinning 全链路

### 修改功能
- `flver-fbx-export`: 模型导出必须通过骨骼变换层处理 skeleton 和 mesh skinning
- `animation-fbx-export`: 动画导出必须通过骨骼变换层重算每帧 reparented 骨骼的 local transform

## 影响

- **核心文件**: `FormalSceneExportShared.cs` — `BuildSkeletonHierarchy()`, `BuildSkeletonHierarchyFromFlver()`, `BuildSkeletonHierarchyFromHkx()`, `BuildFormalMesh()`, `ComputeWorldMatrix()` 均受影响
- **动画导出**: `AnimationToFbxExporter.cs` — `BuildAnimation()` 需要在采样后对 reparented 骨骼做帧级变换修正
- **模型导出**: `FlverToFbxExporter.cs` — 导出流程需集成骨骼变换层
- **glTF 输出**: `GltfSceneWriter.cs` — bone name 变更将自动传播到 skin.joints 和 animation channels
- **下游 UE 导入**: 骨骼名变更是 BREAKING change，UE 侧的 Skeleton/Retarget 配置需同步更新

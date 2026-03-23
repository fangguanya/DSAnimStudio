## 上下文

当前导出管线（`FormalSceneExportShared.cs`）直接将 FLVER/HKX 中的骨骼层级 1:1 映射到 glTF 输出。Sekiro 原始骨骼层级中 `Pelvis` 与 `RootRotY` 同为 `RootPos` 的子骨骼，而 UE Humanoid 标准要求 spine 链从 pelvis 向上延伸。

当前层级：
```
RootPos
├── Pelvis
│   ├── L_Hip / L_Thigh / R_Hip / R_Thigh ...
│   └── Sheath
└── RootRotY          ← 与 Pelvis 平级
    └── RootRotXZ
        └── Spine
            └── Spine1
                └── Spine2 → (Collar, Neck, 武器槽 ...)
```

目标层级：
```
RootPos
└── Pelvis
    ├── L_Hip / L_Thigh / R_Hip / R_Thigh ...
    ├── Sheath
    └── spine_01 (原 RootRotY)    ← 移为 Pelvis 子骨骼 + 改名
        └── spine_02 (原 RootRotXZ)
            └── spine_03 (原 Spine)
                └── spine_04 (原 Spine1)
                    └── spine_05 (原 Spine2)
```

关键约束：
- 骨骼在数组中的索引不变（不重排数组），只修改 parentIndex
- 所有变换仅在导出阶段应用，不修改原始 FLVER/HKX 数据
- skeleton、animation、mesh skinning 三条链路必须使用同一套变换规则

## 目标 / 非目标

**目标：**
- 导出时将 `RootRotY` reparent 为 `Pelvis` 的子骨骼，保持世界空间位置不变
- 导出时重命名 5 个脊柱骨骼为 UE 友好的名称
- 动画每帧的 local transform 必须正确反映新的 parent 关系
- Inverse bind matrix 必须基于新的 parent chain 计算
- 方案具备可扩展性，未来可添加更多 reparent/rename 规则

**非目标：**
- 不修改原始 FLVER.Nodes 或 HKX.HKASkeleton 数据
- 不重新排列骨骼数组索引（因此蒙皮顶点的 boneIndex 无需 remap）
- 不处理 Pelvis 及 spine 链以外的骨骼变换
- 不改变坐标系转换逻辑（SourceToGltf）

## 决策

### 决策 1：引入 `ExportBoneTransformTable` 中间层

**选择**：在读取原始骨骼数据后、构建 Assimp 层级前，插入一个虚拟骨骼表 `ExportBoneTransformTable`，封装 (name, parentIndex, localTransform) 的修改逻辑。

**替代方案 A — 直接修改 FLVER.Nodes**：深拷贝 FLVER.Nodes 并修改。问题：HKX skeleton 路径无法复用同一逻辑，两条路径需要各自维护修改代码。

**替代方案 B — 后处理 Assimp Node tree**：在 `BuildSkeletonHierarchy()` 返回后修改 Node 的 parent-child 关系。问题：Assimp Node tree 的 reparent 操作不方便，且 `ComputeWorldMatrix()` 读的是原始 FLVER 数据而非 Assimp 节点，会导致 inverse bind matrix 与实际层级不一致。

**理由**：中间层方案统一了 FLVER 和 HKX 两条路径，通过 lambda 包装将修改注入 `BuildSkeletonHierarchy()` 的 `getBoneName`、`getParentIndex`、`getLocalMatrix` 三个回调中，改动最小且一致性最高。

### 决策 2：Local Transform 重算公式

对于 reparented 的骨骼（`RootRotY`），由于旧 parent (`RootPos`) 正好是新 parent (`Pelvis`) 的 parent，公式简化为：

```
newLocal = inverse(Pelvis_local) * oldLocal
```

对于 HKX 动画路径，同样适用——每帧采样时：
```
newLocal(frame) = inverse(Pelvis_local(frame)) * RootRotY_oldLocal(frame)
```

这是因为 `RootPos_world` 在等式两侧完全抵消。

**数学推导**：
- 旧 world = RootPos_world * RootRotY_local
- 新 world = Pelvis_world * RootRotY_newLocal = (RootPos_world * Pelvis_local) * RootRotY_newLocal
- 保持 world 不变 → RootPos_world * RootRotY_local = RootPos_world * Pelvis_local * RootRotY_newLocal
- → RootRotY_newLocal = inverse(Pelvis_local) * RootRotY_local

### 决策 3：变换应用时机——在坐标系转换之前

骨骼变换（reparent + rename）在 **source 空间** 完成，然后再经过 `ConvertSourceMatrixToGltf()` 转到 glTF 空间。这保证了坐标系转换逻辑完全不受影响。

### 决策 4：骨骼数组索引保持不变

不重排骨骼数组。`RootRotY` 在 FLVER.Nodes / HKX.Bones 中的数组位置不变，仅修改其 parentIndex 值。因此：
- 蒙皮顶点的 `BoneIndices[w]` → `globalBoneIdx` 映射不需要改变
- `flverMesh.BoneIndices[]` palette 不需要改变
- 仅 `ComputeWorldMatrix()` 和 `BuildSkeletonHierarchy()` 需要使用新的 parentIndex

### 决策 5：配置驱动的 Reparent/Rename 规则

将 reparent 和 rename 规则定义为静态配置（或代码常量），而非硬编码在各个函数中。格式：

```csharp
// Reparent rules: (boneName, newParentName)
static readonly (string Bone, string NewParent)[] ReparentRules = {
    ("RootRotY", "Pelvis"),
};

// Rename rules: (oldName, newName)
static readonly Dictionary<string, string> RenameRules = new() {
    ["RootRotY"] = "spine_01",
    ["RootRotXZ"] = "spine_02",
    ["Spine"] = "spine_03",
    ["Spine1"] = "spine_04",
    ["Spine2"] = "spine_05",
};
```

## 风险 / 权衡

| 风险 | 缓解措施 |
|------|----------|
| Pelvis 的 local transform 在某些特殊动画中为零/退化矩阵，导致 inverse 失败 | 在 `inverse()` 调用后检查是否成功（`Matrix4x4.Invert` 返回 bool），失败时 fallback 到原始 transform 并记录警告 |
| 骨骼名在 FLVER 和 HKX 中可能拼写不完全一致 | 使用 case-sensitive exact match（与现有代码一致），并在找不到目标骨骼时跳过变换并记录警告 |
| 下游 UE Retarget 配置硬编码了旧骨骼名 | 这是 BREAKING change，需同步更新 UE 侧配置。在 proposal 中已标记 |
| 其他角色（非主角）可能没有 RootRotY/Pelvis 骨骼 | 变换表按名称匹配，不存在的骨骼自动跳过，不影响其他角色导出 |

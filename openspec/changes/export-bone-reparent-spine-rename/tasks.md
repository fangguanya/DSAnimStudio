## 1. 骨骼变换表基础设施

- [x] 1.1 在 `DSAnimStudioNETCore/Export/` 中新建 `ExportBoneTransformTable.cs`，定义 reparent 规则列表和 rename 字典作为静态配置
- [x] 1.2 实现 `ExportBoneTransformTable.Build()` 方法：接收骨骼名列表和原始 parentIndex 列表，输出修改后的 (name[], parentIndex[], needsTransformRecalc[]) 三元组
- [x] 1.3 实现骨骼名查找辅助方法：按名称在骨骼数组中查找索引，找不到时返回 -1 并记录警告
- [x] 1.4 实现 bind pose local transform 重算：对每个 reparented 骨骼，计算 `newLocal = inverse(newParent_local_source) * oldLocal_source`，包含矩阵求逆失败的 fallback 逻辑

## 2. Skeleton 构建集成

- [x] 2.1 修改 `FormalSceneExportShared.BuildSkeletonHierarchyFromFlver()`：在调用 `BuildSkeletonHierarchy()` 之前，通过 `ExportBoneTransformTable` 包装 `getBoneName`、`getParentIndex`、`getLocalMatrix` 三个 lambda
- [x] 2.2 修改 `FormalSceneExportShared.BuildSkeletonHierarchyFromHkx()`：同样通过 `ExportBoneTransformTable` 包装三个 lambda
- [x] 2.3 验证：单元测试确认 reparent/rename/transform recalc 正确（8/8 pass）

## 3. Mesh Skinning 集成

- [x] 3.1 修改 `FormalSceneExportShared.BuildFormalMesh()` 中的骨骼名查找（约 L534）：使用 rename 后的名称匹配 `sceneBoneNames`
- [x] 3.2 ComputeWorldMatrix() 无需修改——reparent 重算保持世界空间位置不变，原始 parent chain 计算的 world matrix 与新 chain 一致
- [x] 3.3 验证：导出 mesh 中每个 bone 的 `OffsetMatrix` 基于新 parent chain 的 world matrix 求逆得到

## 4. 动画导出集成

- [x] 4.1 修改 `AnimationToFbxExporter.BuildAnimation()`：在帧循环中，对 reparented 骨骼（通过变换表识别），每帧额外采样新 parent 的 local transform 并执行 `newLocal = oldLocal * inverse(parentLocal)`（行向量惯例）
- [x] 4.2 修改 `BuildAnimation()` 中的 channel NodeName：使用 rename 后的骨骼名
- [x] 4.3 验证：RecalcAnimFrameTransform 单元测试确认帧级变换保持世界空间位置不变

## 5. 端到端验证

- [ ] 5.1 导出 Sekiro 主角模型，验证 glTF 中骨骼层级正确（spine_01 在 Pelvis 下，脊柱链命名正确）——需游戏数据
- [ ] 5.2 导出 Sekiro 主角动画，验证 glTF 中动画 channel 命名正确且播放效果与原始一致——需游戏数据
- [ ] 5.3 将导出产物导入 UE5，验证 SkeletalMesh 骨骼层级和动画回放正确性——需游戏数据+UE

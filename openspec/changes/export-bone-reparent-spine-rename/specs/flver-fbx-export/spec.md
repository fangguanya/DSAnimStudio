## MODIFIED Requirements

### 需求:模型导出必须生成可导入 UE 的骨骼模型
模型导出必须生成可被 UE 识别为骨骼模型的正式交付物，且骨架、skin、joints、inverse bind matrices、mesh root 和材质引用必须满足正式导入契约。导出管线必须在构建骨架和 mesh skinning 之前应用骨骼变换表（reparent + rename），使导出产物的骨骼层级和骨骼名符合 UE Humanoid 标准拓扑。禁止依赖多格式 fallback 或未受控的导出后补丁把无效产物修成"看起来可用"。

#### 场景:模型导入 UE
- **当** 导出的正式模型被导入 UE
- **那么** UE 必须形成 SkeletalMesh、Skeleton 和 PhysicsAsset，且骨骼层级中 `spine_01` 必须是 `Pelvis` 的子骨骼，脊柱链骨骼名必须为 `spine_01` 至 `spine_05`

### 需求:模型 glTF 必须显式写出 formal skeleton root
正式模型 glTF 必须包含唯一 formal skeleton root，并保证 `skin.skeleton`、`skin.joints`、inverse bind matrices 和 mesh 绑定都指向同一正式骨架语义。骨骼变换表中的 rename 必须一致地反映在 skeleton root 解析和 skin joints 列表中。

#### 场景:正式模型导出完成
- **当** 一个角色模型被正式导出为 glTF
- **那么** 验证器必须能直接识别 formal skeleton root，skin.joints 中的骨骼名必须使用变换后的名称（如 `spine_01` 而非 `RootRotY`）

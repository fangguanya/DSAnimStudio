## ADDED Requirements

### 需求:模型正式导出必须交付 glTF 2.0
模型导出能力在正式模式下必须以 glTF 2.0 交付骨骼模型，禁止把 FBX、GLB、Collada 或任何其他格式视为正式成功结果。

#### 场景:正式模型导出
- **当** 用户运行正式模型导出
- **那么** 模型交付物必须是符合正式合同的 glTF 2.0 文件集

## MODIFIED Requirements

### 需求:模型导出必须生成可导入 UE 的骨骼模型
模型导出必须生成可被 UE 识别为骨骼模型的正式交付物，且骨架、skin、joints、inverse bind matrices、mesh root 和材质引用必须满足正式导入契约。禁止依赖多格式 fallback 或未受控的导出后补丁把无效产物修成“看起来可用”。

#### 场景:模型导入 UE
- **当** 导出的正式模型被导入 UE
- **那么** UE 必须形成 SkeletalMesh、Skeleton 和 PhysicsAsset，且该结果来源于正式 glTF 交付物
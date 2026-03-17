## ADDED Requirements

### 需求:模型正式导出必须交付 glTF 2.0
模型导出能力在正式模式下必须以 glTF 2.0 交付骨骼模型，禁止把 FBX、GLB、Collada 或任何其他格式视为正式成功结果。

#### 场景:正式模型导出
- **当** 用户运行正式模型导出
- **那么** 模型交付物必须是符合正式合同的 glTF 2.0 文件集

### 需求:模型正式导出必须绑定 canonical assembly profile
正式模型导出必须基于已声明的 canonical assembly profile 生成交付物，而不是读取 DSAnimStudio 当前 UI 会话中的临时预览状态。对多部件角色，正式模型必须明确主骨架和 follower part 的收敛关系。

#### 场景:导出主角等多部件角色模型
- **当** 正式链路导出需要 `ChrAsm` / part follower 才能形成最终可见角色
- **那么** 交付物和验收报告必须可追溯到唯一的 canonical assembly profile

### 需求:模型 glTF 必须显式写出 formal skeleton root
正式模型 glTF 必须包含唯一 formal skeleton root，并保证 `skin.skeleton`、`skin.joints`、inverse bind matrices 和 mesh 绑定都指向同一正式骨架语义。

#### 场景:正式模型导出完成
- **当** 一个角色模型被正式导出为 glTF
- **那么** 验证器必须能直接识别 formal skeleton root，而无需后续按名称猜测或补丁推断

## MODIFIED Requirements

### 需求:模型导出必须生成可导入 UE 的骨骼模型
模型导出必须生成可被 UE 识别为骨骼模型的正式交付物，且骨架、skin、joints、inverse bind matrices、mesh root 和材质引用必须满足正式导入契约。禁止依赖多格式 fallback 或未受控的导出后补丁把无效产物修成“看起来可用”。

#### 场景:模型导入 UE
- **当** 导出的正式模型被导入 UE
- **那么** UE 必须形成 SkeletalMesh、Skeleton 和 PhysicsAsset，且该结果来源于正式 glTF 交付物
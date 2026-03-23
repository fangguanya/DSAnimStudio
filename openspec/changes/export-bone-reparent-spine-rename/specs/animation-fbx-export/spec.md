## MODIFIED Requirements

### 需求:动画 glTF 只能绑定正式骨架节点
正式动画 glTF 的全部 channel 必须绑定到 formal skeleton root 之下的正式骨架节点，并仅使用 TRS 通道表达。动画 channel 的 NodeName 必须使用骨骼变换表中 rename 后的名称。对于被 reparent 的骨骼，每帧的 local transform 必须按新 parent 关系重新计算，确保动画效果在世界空间中不变。禁止出现游离节点、矩阵动画或无法映射到正式骨架的通道。

#### 场景:动画 clip 正式导出完成
- **当** 一个 clip 被写出为正式 glTF
- **那么** 该 clip 中的动画节点必须全部可映射到正式模型骨架，且 channel 名必须使用变换后的骨骼名（如 `spine_01` 而非 `RootRotY`）

#### 场景:Reparented 骨骼的动画帧级变换
- **当** 动画 clip 包含 `RootRotY`（导出名 `spine_01`）的动画轨道
- **那么** 每帧导出的 local transform 必须满足：`newLocal(frame) = inverse(Pelvis_local(frame)) * oldLocal(frame)`，其中 `Pelvis_local(frame)` 和 `oldLocal(frame)` 均从 HKX 动画数据中采样

### 需求:动画导出不得静默跳过或填充占位帧
动画导出必须对每个 clip 明确成功或失败。禁止因为 HKX 读取失败、clip 构建失败、节点不匹配或帧采样异常而静默跳过该 clip，也禁止使用 identity 帧或其他占位数据让失败 clip 看起来可导入。当 reparent 变换中的矩阵求逆失败时，必须将该 clip 标记为失败。

#### 场景:动画 clip 存在异常
- **当** 某个动画 clip 在正式导出过程中出现异常（包括 reparent 变换失败）
- **那么** 该 clip 必须被标记为失败并写入验收报告

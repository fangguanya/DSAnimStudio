## ADDED Requirements

### 需求:动画正式导出必须交付单 clip glTF 2.0
动画导出能力在正式模式下必须为每个 clip 生成单文件逻辑上的 glTF 2.0 动画交付物，并与目标角色正式骨架合同一致。

#### 场景:正式动画导出
- **当** 用户正式导出一个动画 clip
- **那么** 系统必须输出对应的 glTF 2.0 动画交付物，并与正式骨架合同匹配

### 需求:动画正式导出必须复用 formal animation resolution
正式动画导出必须基于共享的 formal animation resolution 结果生成 clip，禁止在导出器内部重新按 anim id、文件名或容器名进行独立猜测。

#### 场景:导出带引用链的主角动作
- **当** 请求的 TAE 条目需要经引用链或 `SourceAnimFileName` 才能找到真实 HKX 动画
- **那么** 动画导出必须使用共享求解结果中的 resolved HKX 和 deliverable anim file

### 需求:动画 glTF 只能绑定正式骨架节点
正式动画 glTF 的全部 channel 必须绑定到 formal skeleton root 之下的正式骨架节点，并仅使用 TRS 通道表达。禁止出现游离节点、矩阵动画或无法映射到正式骨架的通道。

#### 场景:动画 clip 正式导出完成
- **当** 一个 clip 被写出为正式 glTF
- **那么** 该 clip 中的动画节点必须全部可映射到正式模型骨架，且无需额外补丁才能导入

## MODIFIED Requirements

### 需求:动画导出不得静默跳过或填充占位帧
动画导出必须对每个 clip 明确成功或失败。禁止因为 HKX 读取失败、clip 构建失败、节点不匹配或帧采样异常而静默跳过该 clip，也禁止使用 identity 帧或其他占位数据让失败 clip 看起来可导入。

#### 场景:动画 clip 存在异常
- **当** 某个动画 clip 在正式导出过程中出现异常
- **那么** 该 clip 必须被标记为失败并写入验收报告
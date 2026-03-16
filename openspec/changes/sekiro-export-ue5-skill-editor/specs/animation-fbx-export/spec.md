## 新增需求

### 需求:HKX动画到FBX动画导出
系统必须能将 HKX (Havok) 格式的骨骼动画数据转换为 FBX 动画格式，保留完整的逐帧骨骼变换信息。

#### 场景:导出骨骼动画关键帧
- **当** 用户指定一个 HKX 动画文件和对应的 HKX 骨骼进行导出
- **那么** 导出的 FBX 必须包含一个 Animation Take，其中每个骨骼轨道（bone track）包含逐帧的 Position (VectorKey)、Rotation (QuaternionKey) 和 Scale (VectorKey) 关键帧数据

#### 场景:正确采样动画帧
- **当** HKX 动画使用样条压缩格式（SplineCompressed）
- **那么** 系统必须对每一帧调用 `GetTransformOnFrame(boneIndex, frame)` 解压缩获取 NewBlendableTransform，然后转换为 Assimp 关键帧格式。帧率必须保持与原始动画一致（通常 30fps）

#### 场景:处理交错未压缩动画
- **当** HKX 动画使用交错未压缩格式（InterleavedUncompressed）
- **那么** 系统必须直接读取每帧的 Transform 数据并转换为 FBX 关键帧

### 需求:根运动烘焙
系统必须支持将 HKX 动画中的根运动（Root Motion）数据烘焙到 FBX 根骨骼的动画轨道中。

#### 场景:烘焙根运动到根骨骼
- **当** HKX 动画包含 HKADefaultAnimatedReferenceFrame（根运动轨迹）
- **那么** 系统必须将根运动数据叠加到根骨骼的 Position 和 Rotation 关键帧中

#### 场景:无根运动时的处理
- **当** HKX 动画不包含根运动数据
- **那么** 系统必须正常导出骨骼动画，根骨骼仅包含骨架定义的静态变换

### 需求:骨骼匹配验证
导出时必须验证动画骨骼与目标骨骼的匹配关系。

#### 场景:骨骼名称匹配
- **当** HKX 动画的骨骼轨道引用特定骨骼索引
- **那么** 系统必须通过 HKX Skeleton 的骨骼名称列表将轨道索引映射到正确的骨骼名称，FBX 中的 NodeAnimationChannel.NodeName 必须与骨骼节点名称完全一致

### 需求:批量动画导出
系统必须支持从 ANIBND 归档文件中批量导出所有动画。

#### 场景:从ANIBND导出全部动画
- **当** 用户指定一个 .anibnd.dcx 文件进行批量导出
- **那么** 系统必须解包 ANIBND，对其中每个 HKX 动画文件生成独立的 FBX 文件，文件名使用原始动画 ID（如 `a000_003000.fbx`）

### 需求:最终动画交付物可绑定到UE Skeleton
动画导出必须生成可被 UE 正式绑定到角色 Skeleton 的最终交付物，而不是仅供中间转换的片段文件。

#### 场景:每个动画文件对应单个完整 clip
- **当** 导出一个动画片段
- **那么** 最终交付物必须是单个完整 clip 文件，包含该动画涉及的全部骨骼通道；不得把每骨骼或每通道的中间片段当成正式交付物

#### 场景:动画帧数和持续时间可验证
- **当** 动画导出完成
- **那么** 最终文件必须保留可验证的 `frameCount`、`frameRate` 和 clip 时长，使 UE 导入后可以与技能配置中的动画元数据逐项对齐

#### 场景:动画可绑定到目标Skeleton
- **当** UE5 导入器将动画导入到角色 Skeleton
- **那么** 动画中的骨骼名称、根骨骼、关键帧轨道数和根运动定义必须足以成功绑定到该角色 Skeleton；无法绑定的动画不得计为导出成功

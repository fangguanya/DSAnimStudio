## 新增需求

### 需求:FLVER模型到FBX格式转换
系统必须能将 FLVER2 格式的 3D 模型文件转换为 FBX 格式，保留完整的网格几何数据、骨骼层级和材质引用信息。

#### 场景:导出包含完整顶点数据的网格
- **当** 用户指定一个 FLVER2 模型文件进行 FBX 导出
- **那么** 导出的 FBX 文件必须包含所有子网格（Submesh），每个子网格保留以下顶点属性：Position (Vector3)、Normal (Vector3)、Tangent (Vector4)、VertexColor (RGBA)、最多 8 个 UV 通道、4 个 BoneIndices 和 4 个 BoneWeights

#### 场景:导出骨骼层级结构
- **当** FLVER2 模型包含 Nodes（骨骼）数据
- **那么** 导出的 FBX 必须包含完整的骨骼层级树，每个骨骼保留 Name、ParentIndex 关系、Translation、Rotation 和 Scale 变换，且层级关系与原始 FLVER.Node 列表一致

#### 场景:导出骨骼权重绑定
- **当** FLVER2 网格包含骨骼权重数据（BoneIndices + BoneWeights）
- **那么** FBX 文件中每个顶点必须正确绑定到对应的骨骼节点，权重值与原始 FLVER 数据一致。对于 `Header.Version > 0x2000D` 的模型，必须使用全局骨骼索引映射

#### 场景:导出三角形面数据
- **当** FLVER2 网格包含 FaceSets
- **那么** 导出的 FBX 必须使用 LOD 0 的三角形列表（Triangle List），正确处理 16 位和 32 位索引格式，跳过 MotionBlur 类型的 FaceSet

### 需求:材质引用保留
系统必须在 FBX 导出中保留 FLVER2 材质信息，使材质名称和纹理路径可被下游工具（UE5）识别。

#### 场景:导出材质名称和MTD引用
- **当** FLVER2 模型包含 Materials 列表
- **那么** FBX 文件中每个网格必须关联正确的材质，材质包含 Name 和 MTD（材质定义）路径

#### 场景:导出纹理路径引用
- **当** FLVER2 材质包含 Textures 列表（Type + Path）
- **那么** FBX 材质必须包含纹理路径引用：Diffuse 纹理取自 Type 包含 "ALBEDO" 或 "DIFFUSE" 的条目，Normal 纹理取自包含 "NORMALMAP" 或 "BUMPMAP" 的条目

### 需求:坐标系和缩放处理
导出的 FBX 必须使用标准坐标系，确保在 UE5 中导入后模型方向和比例正确。

#### 场景:坐标系转换
- **当** 执行 FLVER 到 FBX 导出
- **那么** 系统必须将 FLVER 的坐标系（右手 Y-Up）正确转换为 FBX 标准坐标系，确保模型在 UE5 中导入后朝向正确

#### 场景:自定义缩放因子
- **当** 用户指定缩放因子（默认 1.0）
- **那么** 导出的 FBX 中所有顶点位置和骨骼位移必须乘以该缩放因子

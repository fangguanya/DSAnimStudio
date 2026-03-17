## 新增需求

### 需求:材质到纹理映射清单导出
系统必须导出一份 JSON 格式的材质清单文件，记录每个 FLVER2 材质使用的全部纹理及其用途分类。

#### 场景:导出材质纹理映射
- **当** 用户导出一个角色的材质清单
- **那么** 清单文件必须包含每个 FLVER2.Material 的条目，每个条目记录：材质名称（Name）、MTD 路径、及其 Textures 列表中每个纹理的类型（Type）和文件路径（Path）

#### 场景:纹理类型分类
- **当** 材质包含纹理采样器引用
- **那么** 系统必须按照以下规则将纹理 Type 分类为 UE5 材质槽位：
  - 包含 "ALBEDO" 或 "DIFFUSE" → `BaseColor`
  - 包含 "NORMALMAP" 或 "BUMPMAP" → `Normal`
  - 包含 "SPECULAR" 或 "REFLECTANCE" 或 "METALLIC" → `Specular`
  - 包含 "EMISSIVE" → `Emissive`
  - 包含 "SHININESS" → `Roughness`
  - 包含 "BLENDMASK" → `BlendMask`
  - 其他 → `Other`

#### 场景:纹理路径与导出文件对应
- **当** 生成材质清单时
- **那么** 清单中的纹理路径必须使用导出后的实际文件名（与 texture-export 模块导出的文件名一致），而非游戏内部路径

### 需求:清单文件格式
材质清单必须使用规范的 JSON 格式。

#### 场景:JSON清单结构
- **当** 导出材质清单文件
- **那么** 文件必须遵循以下结构：顶层 `materials` 对象，键为材质名称，值包含 `mtd`（字符串）和 `textures`（对象，键为 UE5 槽位名，值为导出纹理文件名）

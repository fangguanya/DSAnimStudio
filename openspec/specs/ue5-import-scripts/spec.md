## 新增需求

### 需求:Python批量FBX导入脚本
系统必须提供 Python 脚本，通过 UE5 Python API 批量导入导出的 FBX 文件。

#### 场景:批量导入骨骼网格
- **当** 用户在 UE5 Editor 中运行 batch_import.py 并指定导出目录
- **那么** 脚本必须扫描目录下所有 `*_model.fbx` 文件，使用 `unreal.AssetTools.import_asset_tasks()` 导入为 SkeletalMesh 资产，导入路径按角色 ID 组织（如 `/Game/Sekiro/Characters/c0000/`）

#### 场景:批量导入动画
- **当** 脚本处理角色的 Animations/ 目录
- **那么** 必须将每个 `.fbx` 动画文件导入为 UAnimSequence 资产，关联到对应角色的 Skeleton 资产，导入路径为 `/Game/Sekiro/Characters/<chrID>/Animations/`

#### 场景:批量导入纹理
- **当** 脚本处理角色的 Textures/ 目录
- **那么** 必须将每个 PNG/DDS 文件导入为 UTexture2D 资产，Normal Map 类型的纹理必须设置 `CompressionSettings` 为 `TC_Normalmap`，导入路径为 `/Game/Sekiro/Characters/<chrID>/Textures/`

### 需求:材质自动配置脚本
系统必须提供 Python 脚本，根据材质清单自动创建和配置 UE5 材质实例。

#### 场景:创建材质实例
- **当** 用户运行 setup_materials.py 并指定 material_manifest.json
- **那么** 脚本必须为清单中的每个材质创建 UMaterialInstanceConstant，基于一个预定义的 Master Material（支持 BaseColor/Normal/Specular/Emissive 纹理槽位）

#### 场景:自动分配纹理
- **当** 创建材质实例时
- **那么** 脚本必须根据清单中的纹理映射，将已导入的 UTexture2D 资产分配到材质实例的对应槽位（BaseColor → TextureParameterValue "BaseColor"，Normal → "Normal"，Specular → "Specular"，Emissive → "Emissive"）

### 需求:技能配置导入脚本
系统必须提供 Python 脚本，将导出的技能 JSON 转换为 UE5 数据资产。

#### 场景:创建技能数据资产
- **当** 用户运行 import_skill_configs.py 并指定 skill_config.json
- **那么** 脚本必须解析 JSON 中每个动画的事件数据，创建对应的 USekiroSkillDataAsset 实例，自动关联已导入的 UAnimSequence 资产（通过动画文件名匹配）

#### 场景:处理大量技能数据
- **当** JSON 文件包含数百个动画的事件数据
- **那么** 脚本必须正确处理全部数据，对每个动画创建独立的数据资产，并在 Content Browser 中按类别组织（`/Game/Sekiro/Skills/<Category>/`）

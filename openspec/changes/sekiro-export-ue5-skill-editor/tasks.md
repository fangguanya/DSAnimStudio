> 状态说明：当前仓库和外部 UE 项目中已经存在大量原型代码，但其中一部分仅证明“代码路径存在”或“可视化跑通”，还不满足本 spec 现在定义的正式完成标准。以下新增的纠偏任务优先级高于前文的历史勾选结果。

## 1. 环境准备

- [x] 1.1 初始化 Git 子模块（`git submodule update --init --recursive`），确保 SoulsAssetPipeline、SoulsFormats、Havoc 源代码可用
- [x] 1.2 验证现有解决方案构建成功（`dotnet build DSAnimStudioNETCore.sln -c Release`）
- [x] 1.3 在 DSAnimStudioNETCore.csproj 中添加 AssimpNet 4.1.0 NuGet 依赖
- [x] 1.4 创建 `DSAnimStudioNETCore/Export/` 目录用于存放全部导出器类

## 2. FLVER 模型导出（glTF/FBX）

- [x] 2.1 创建 `Export/FlverToFbxExporter.cs`：实现 ExportOptions 配置类（缩放因子、坐标系转换开关）
- [x] 2.2 实现骨骼层级导出：遍历 `flver.Nodes`，构建 Assimp.Node 层级树，正确设置 Translation/Rotation/Scale 变换矩阵（S * RX * RZ * RY * T）
- [x] 2.3 实现网格顶点数据导出：Position/Normal/Tangent/UV/VertexColor
- [x] 2.4 实现骨骼权重导出：处理 BoneIndices/BoneWeights，正确区分全局骨骼索引（`Header.Version > 0x2000D`）和局部骨骼索引
- [x] 2.5 实现三角形面数据导出：从 FaceSet LOD 0 提取三角形索引，处理 TriangleStrip/TriangleList
- [x] 2.6 实现材质引用导出：为每个网格创建 Assimp.Material，设置 Diffuse/Normal/Specular/Emissive 纹理路径
- [x] 2.7 实现多格式导出回退链：FBX → FBX ASCII → glTF2 → glTF binary → Collada（Assimp FBX 导出在 400+ 骨骼时失败，glTF2 作为主要格式成功）
- [x] 2.8 验证 c1010 模型导出：81 meshes, 400 nodes, 108 skin joints → glTF2 格式正确

## 3. 动画导出（glTF/FBX）

- [x] 3.1 创建 `Export/AnimationToFbxExporter.cs`：实现 HKX 骨骼到 Assimp 骨骼节点的转换
- [x] 3.2 实现样条压缩动画（SplineCompressed）的逐帧采样
- [x] 3.3 实现交错未压缩动画（InterleavedUncompressed）的帧数据读取
- [x] 3.4 实现根运动烘焙：HKADefaultAnimatedReferenceFrame → 叠加到根骨骼关键帧
- [x] 3.5 实现 ANIBND 批量动画导出：解包 .anibnd.dcx，tagfile + compendium 解析
- [x] 3.6 实现多格式导出回退链（同模型）：FBX → glTF2 → Collada
- [x] 3.7 创建 `GltfAnimationMerger.cs`：后处理合并 Assimp glTF2 导出的 per-bone 动画为单个动画 clip（127 bones × 3 TRS = 381 channels）
- [x] 3.8 验证 c1010 动画导出：350 HKX → 349 glTF clips，全部正确合并

## 4. 纹理导出

- [x] 4.1 创建 `Export/TextureExporter.cs`：Pfim NuGet 依赖
- [x] 4.2 实现 DDS 直接导出
- [x] 4.3 实现 PNG 导出：BC1/BC3/BC5/BC7 解码
- [x] 4.4 从 .chrbnd.dcx 解包内嵌 TPF 纹理
- [x] 4.5 从独立 .texbnd.dcx 文件提取纹理
- [x] 4.6 BC6H 等不支持格式跳过+警告
- [x] 4.7 处理重名纹理去重
- [x] 4.8 验证 PNG 纹理正确

## 5. 技能配置导出

- [x] 5.1 创建 `Export/SkillConfigExporter.cs`
- [x] 5.2 TAE.Template.SDT.xml 加载和事件类型解析
- [x] 5.3 全部 185 种 TAE 事件类型参数序列化
- [x] 5.4 攻击/特效/音效/武器技艺/Sekiro 专有事件完整导出
- [x] 5.5 创建 `Export/ParamExporter.cs`
- [x] 5.6 义手忍具 DummyPoly 配置导出
- [x] 5.7 JSON 输出 schema
- [x] 5.8 验证事件数量一致

## 6. 材质清单导出

- [x] 6.1 创建 `Export/MaterialManifestExporter.cs`
- [x] 6.2 纹理类型分类（ALBEDO→BaseColor, NORMALMAP→Normal 等）
- [x] 6.3 纹理路径转换
- [x] 6.4 输出 material_manifest.json

## 7. CLI 批量导出工具

- [x] 7.1 创建 `SekiroExporter/SekiroExporter.csproj`（net6.0-windows）
- [x] 7.2 添加到 DSAnimStudioNETCore.sln 解决方案
- [x] 7.3 实现 `export-all` 命令：--game-dir、--chr、--output
- [x] 7.4 实现子命令：export-model、export-anims、export-textures、export-skills、convert-to-fbx
- [x] 7.5 游戏目录扫描：自动识别 chr/*.chrbnd.dcx
- [x] 7.6 顺序处理（避免并行输出交错）+ 进度报告
- [x] 7.7 错误处理：损坏文件跳过+日志，oo2core DLL 检测
- [x] 7.8 动态格式检测：FindExportedFile 自动检测实际输出格式（.fbx/.gltf/.glb/.dae）
- [x] 7.9 创建 `DaeToFbxConverter.cs`：DAE→FBX 后转换工具
- [x] 7.10 验证完整导出：c1010 模型(glTF)+349 动画(glTF)+20 纹理(PNG)+技能 JSON

## 8. DSAnimStudio UI 集成

- [x] 8.1 修改 `MenuBar.050.Tools.cs`：添加导出菜单项
- [x] 8.2 导出对话框方法
- [x] 8.3 文件/文件夹选择对话框
- [x] 8.4 导出进度显示和完成通知
- [ ] 8.5 验证 UI 菜单项正确显示且在正确条件下启用/禁用（需要实际运行 DSAnimStudio 验证）

## 9. UE5.7 项目创建

- [x] 9.1 在 E:\Sekiro\SekiroSkillEditor 创建 C++ 项目
- [x] 9.2 验证项目能正确编译
- [x] 9.3 创建 SekiroSkillEditorPlugin 插件（.uplugin + Build.cs）

## 10. UE5 数据资产和导入管线

- [x] 10.1 FSekiroTaeEvent USTRUCT
- [x] 10.2 USekiroSkillDataAsset UDataAsset
- [x] 10.3 USekiroCharacterData UDataAsset
- [x] 10.4 SekiroAssetImporter：skill_config.json 解析 → USekiroSkillDataAsset
- [x] 10.5 SekiroAssetFactory：UFactory 支持 JSON 拖拽导入
- [x] 10.6 SekiroMaterialSetup：material_manifest.json → UMaterialInstance
- [x] 10.7 SekiroImportCommandlet：支持 glTF (Interchange) / FBX / DAE 导入
  - 模型：glTF 通过 UInterchangeManager::ImportAsset() → SkeletalMesh + USkeleton + PhysicsAsset
  - 动画：glTF 通过自定义解析器 → IAnimationDataController → AnimSequence（直接读取 glTF JSON + binary，逐骨骼通道重采样为均匀帧）
  - Interchange 在 Commandlet 模式下的 Slate 崩溃问题：改用 UInterchangeManager 直接 API 绕过 AssetTools/ContentBrowser 通知层
  - Assimp glTF 导出绝对路径 Bug 修复：FixGltfBufferUris() 后处理
  - 自动格式检测：FindBestModelFile（FBX > glTF > GLB > DAE）
  - 骨骼自动发现：FindSkeletonInPackage
- [x] 10.8 验证 glTF → UE5 SkeletalMesh + AnimSequence 完整导入流程
  - 129 角色全部导入成功，0 错误
  - 119 SkeletalMesh + Skeleton，8858 AnimSequence，1630 纹理，1964 材质
  - 总计 12822 个 .uasset，2.6GB

## 11. UE5 时间轴编辑器

- [x] 11.1 SSekiroSkillTimeline 基础 Slate Widget
- [x] 11.2 多轨道事件渲染：按 EventCategory 分组，色块条着色
- [x] 11.3 播放头拖拽和动画同步
- [x] 11.4 时间轴缩放和水平滚动
- [x] 11.5 事件点击选择和高亮
- [x] 11.6 SSekiroEventInspector 属性面板

## 12. UE5 3D 视口和技能浏览器

- [x] 12.1 SekiroSkillPreviewActor（SkeletalMeshComponent + AnimInstance）
- [x] 12.2 动画回放与时间轴同步
- [x] 12.3 SekiroHitboxDrawComponent（攻击判定框可视化）
- [x] 12.4 SekiroEffectMarkerComponent（特效/音效标记）
- [x] 12.5 SSekiroSkillBrowser（树形视图+搜索过滤）
- [x] 12.6 SekiroSkillEditorTab（主编辑器标签页布局）

## 13. Python 导入脚本

- [x] 13.1 batch_import.py：支持 glTF/FBX/DAE 批量导入（SkeletalMesh + AnimSequence）
  - find_model_file: 自动选择最佳格式（FBX > glTF > GLB > DAE）
  - find_animation_files: 收集动画文件（FBX > DAE，glTF 动画需通过 Commandlet 导入）
  - find_skeleton_from_mesh: 从导入的 SkeletalMesh 自动获取 Skeleton
  - import_character: 完整角色导入流程（纹理 → 模型 → 动画 → 技能）
  - 注意：glTF 动画在 Python 脚本中不支持（Interchange 无法绑定到已有 Skeleton），需使用 Commandlet
- [x] 13.2 setup_materials.py：材质清单 → MaterialInstance
- [x] 13.3 import_skill_configs.py：JSON → USekiroSkillDataAsset
- [x] 13.4 Normal Map 纹理 TC_Normalmap 配置
- [x] 13.5 端到端流程已通过 Commandlet 验证（C# 导出 → Commandlet 导入 → 资产创建成功）

## 14. 端到端验证

- [x] 14.1 使用 CLI 工具导出全部 129 角色：模型 glTF + 动画 glTF + 纹理 PNG + 技能 JSON + 材质清单
  - 129 succeeded, 0 failed in 750.9s
- [x] 14.2 在 UE5 中运行 SekiroImport Commandlet 导入全部资产
  - 129 角色导入完成，约 5 分钟
  - 关键技术突破：
    - UInterchangeManager 直接 API 避免 Commandlet 模式 Slate 崩溃
    - 自定义 glTF → AnimSequence 解析器（Interchange 无法将独立动画 glTF 绑定到已有 Skeleton）
    - Assimp glTF 绝对路径 Bug 修复（buffer URI 从绝对路径改为相对文件名）
- [x] 14.3 验证 SkeletalMesh 导入正确：119 个 SkeletalMesh + Skeleton + PhysicsAsset 创建成功
- [x] 14.4 验证 AnimSequence 导入正确：8858 个动画资产通过自定义 glTF 解析器创建
- [ ] 14.5 验证技能编辑器：时间轴、3D 视口、攻击判定框（需在 UE5 编辑器中手动验证）
- [x] 14.6 测试多个角色完整流程：c1010(349 动画), c1020(289 动画), c9800(151 动画), c9820(161 动画) 等全部成功

## 15. 正式化纠偏任务

- [ ] 15.1 统一导出器与 UE 导入器的 skill config schema，确定唯一 canonical JSON 结构（`characters[]`、`animations[]`、`frameCount`、`frameRate`、`fileName`、`startFrame`、`endFrame`、`params`）并同步更新 C# 与 UE5 两端
- [ ] 15.2 将 `ParamExporter` 正式接入 CLI 全量导出链，确保 `AtkParam`、`BehaviorParam`、`SpEffectParam`、`EquipParamWeapon` 和义手覆盖配置进入最终 skill config 交付物
- [ ] 15.3 修复 SkillConfigExporter 的事件时间字段，输出帧域 `startFrame/endFrame`，并补齐 `frameCount/fileName/frameRate`
- [ ] 15.4 为全部 185 种 TAE 事件建立正式参数名映射；对模板缺失字段输出数据类型与偏移，不允许静默以占位名掩盖缺口
- [ ] 15.5 梳理模型导出正式格式，确保默认导出只保留可导入 UE 骨骼模型的单一交付物；DAE/其他中间格式仅在 `--keep-intermediates` 下保留
- [ ] 15.6 验证 FLVER 导出结果在 UE 中确实生成 SkeletalMesh + Skeleton + PhysicsAsset；存在模型输入但未形成骨骼网格时必须判失败
- [ ] 15.7 修复动画导出/导入契约，确保每个 clip 是单文件完整动画，并与角色 Skeleton 正确绑定；零轨道、错 Skeleton、空动画不得计为成功
- [ ] 15.8 正规化 UE 自定义 glTF 动画导入器，补齐对正式输入格式的解析约束；若支持 `.glb`，必须实现真实的 GLB 容器解析而非按 JSON 读取
- [ ] 15.9 收紧 `SekiroValidationCommandlet` 的通过标准：StaticMesh 降级、缺失 Skeleton、缺失 PhysicsAsset、未绑定动画、零轨道动画均应视为错误
- [ ] 15.10 扩展 `USekiroSkillDataAsset` / 参数缓存层，使 UE 端保存可复刻的结构化战斗数据，而不是仅保存字符串化参数映射
- [ ] 15.11 让 3D 视口基于真实 DummyPoly、BehaviorParam 和 AtkParam 驱动攻击判定可视化，禁止以 Actor 原点偏移球体替代正式复刻
- [ ] 15.12 让特效点、音效点、WeaponArt、义手覆盖在属性面板和视口中显示解析后的实际生效结果，而不是仅显示原始事件类别
- [ ] 15.13 为 CLI 和导入命令增加角色级验收报告，逐角色输出模型、动画、纹理、技能配置、参数表和 UE 资产导入结果，禁止只给总数
- [ ] 15.14 重新跑全量导出与全量导入，基于新验收标准生成真实统计，替换掉历史上不准确的“全部成功”结论

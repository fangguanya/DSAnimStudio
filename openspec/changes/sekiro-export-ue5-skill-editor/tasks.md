## 1. 环境准备

- [x] 1.1 初始化 Git 子模块（`git submodule update --init --recursive`），确保 SoulsAssetPipeline、SoulsFormats、Havoc 源代码可用
- [x] 1.2 验证现有解决方案构建成功（`dotnet build DSAnimStudioNETCore.sln -c Release`）
- [x] 1.3 在 DSAnimStudioNETCore.csproj 中添加 AssimpNet 4.1.0 NuGet 依赖
- [x] 1.4 创建 `DSAnimStudioNETCore/Export/` 目录用于存放全部导出器类

## 2. FLVER 模型到 FBX 导出

- [x] 2.1 创建 `Export/FlverToFbxExporter.cs`：实现 ExportOptions 配置类（缩放因子、坐标系转换开关）
- [x] 2.2 实现骨骼层级导出：遍历 `flver.Nodes`，构建 Assimp.Node 层级树，正确设置 Translation/Rotation/Scale 变换矩阵
- [x] 2.3 实现网格顶点数据导出：反向 FlverSubmeshRenderer.cs:328-527 的顶点提取逻辑，填充 Assimp.Mesh 的 Position/Normal/Tangent/UV/VertexColor
- [x] 2.4 实现骨骼权重导出：处理 BoneIndices/BoneWeights，正确区分全局骨骼索引（`Header.Version > 0x2000D`）和局部骨骼索引
- [x] 2.5 实现三角形面数据导出：从 FaceSet LOD 0 提取三角形索引，处理 16/32 位索引格式
- [x] 2.6 实现材质引用导出：为每个网格创建 Assimp.Material，设置 Diffuse/Normal 纹理路径
- [x] 2.7 使用 AssimpContext.ExportFile 写出 FBX 文件，验证 c0000 模型导出后在 Blender 中正确显示

## 3. 动画到 FBX 导出

- [x] 3.1 创建 `Export/AnimationToFbxExporter.cs`：实现 HKX 骨骼到 Assimp 骨骼节点的转换
- [x] 3.2 实现样条压缩动画（SplineCompressed）的逐帧采样：对每个骨骼轨道的每一帧调用 GetTransformOnFrame 获取 NewBlendableTransform
- [x] 3.3 实现交错未压缩动画（InterleavedUncompressed）的帧数据读取
- [x] 3.4 实现根运动烘焙：检测 HKADefaultAnimatedReferenceFrame，将根运动叠加到根骨骼的关键帧中
- [x] 3.5 实现 ANIBND 批量动画导出：解包 .anibnd.dcx，对每个 HKX 动画文件生成独立 FBX，文件名使用 aXXX_YYYYYY 格式
- [x] 3.6 验证导出动画在 Blender 中正确回放（选取 c0000 的 3 个代表性动画测试）

## 4. 纹理导出

- [x] 4.1 创建 `Export/TextureExporter.cs`：在 SekiroExporter.csproj 中添加 Pfim NuGet 依赖
- [x] 4.2 实现 DDS 直接导出：将 TPF.Texture.Bytes 写出为 .dds 文件
- [x] 4.3 实现 PNG 导出：使用 Pfim 解码 BC 压缩纹理（DXT1/DXT3/DXT5/BC4/BC5/BC7）为 RGBA，编码为 PNG
- [x] 4.4 实现从 .chrbnd.dcx 解包 BND4 归档提取内嵌 TPF 纹理
- [x] 4.5 实现从独立 .tpf.dcx 文件解压并提取纹理
- [x] 4.6 处理不支持格式（BC6H 等）的跳过和警告日志
- [x] 4.7 处理重名纹理的文件名后缀去重
- [x] 4.8 验证导出的 PNG 纹理在图像查看器中正确显示

## 5. 技能配置导出

- [x] 5.1 创建 `Export/SkillConfigExporter.cs`：实现 TAE 文件遍历（参考 TaeEditorScreen.ImmediateExportAllEventsToTextFile 模式）
- [x] 5.2 实现 TAE.Template.SDT.xml 加载和事件类型名称/参数名称解析
- [x] 5.3 实现全部 185 种 TAE 事件类型的参数序列化，按类型（s32/f32/u8/s16/b）正确保持参数值类型
- [x] 5.4 实现攻击事件（ID 1/2/5）、特效事件（ID 95-123）、音效事件（ID 128-132）、武器技艺事件（ID 330-332）、Sekiro 专有事件（ID 708-720）的完整参数导出
- [x] 5.5 创建 `Export/ParamExporter.cs`：实现 AtkParam、BehaviorParam、SpEffectParam、EquipParamWeapon 参数表的 JSON 序列化
- [x] 5.6 实现义手忍具 DummyPoly 覆盖配置（SekiroProstheticOverrideDmy0-3）的导出
- [x] 5.7 实现 JSON 输出 schema：version/gameType/characters/params 顶层结构，动画使用 aXXX_YYYYYY 格式作键名
- [x] 5.8 验证导出 JSON 的事件数量与 DSAnimStudio 中显示的一致

## 6. 材质清单导出

- [x] 6.1 创建 `Export/MaterialManifestExporter.cs`：遍历 flver.Materials 和 mat.Textures[]
- [x] 6.2 实现纹理类型分类逻辑（参考 FlverMaterial.cs:655-702）：ALBEDO→BaseColor, NORMALMAP→Normal, SPECULAR→Specular, EMISSIVE→Emissive, SHININESS→Roughness, BLENDMASK→BlendMask
- [x] 6.3 实现纹理路径转换：将游戏内部路径映射为导出后的实际文件名
- [x] 6.4 输出 material_manifest.json，验证与导出纹理文件名一致

## 7. CLI 批量导出工具

- [x] 7.1 创建 `SekiroExporter/SekiroExporter.csproj` 控制台项目（net6.0-windows），添加 SoulsFormats/SoulsAssetPipeline 项目引用和 AssimpNet/Pfim/Newtonsoft.Json NuGet 包
- [x] 7.2 将 SekiroExporter 项目添加到 DSAnimStudioNETCore.sln 解决方案
- [x] 7.3 实现 `export-all` 命令：接受 --game-dir、--chr（可选）、--output 参数，编排全部导出步骤
- [x] 7.4 实现 `export-model`、`export-anims`、`export-textures`、`export-skills` 子命令
- [x] 7.5 实现游戏目录扫描：自动识别 chr/ 下全部 .chrbnd.dcx 文件
- [x] 7.6 实现并行批量导出：使用 Task.WhenAll 并行处理独立角色
- [x] 7.7 实现错误处理：损坏文件跳过+日志记录，进度报告（当前角色/已完成/总数）
- [x] 7.8 验证 CLI 工具对 c0000 的完整导出流程和输出目录结构

## 8. DSAnimStudio UI 集成

- [x] 8.1 修改 `MenuBar.050.Tools.cs`：添加 "Export Model to FBX..."、"Export Textures..."、"Export Skill Config (JSON)..."、"Export All for UE5..." 菜单项，使用 ClickItem + MainThreadLazyDispatch 模式
- [x] 8.2 在 `TaeEditorScreen.cs` 中添加导出对话框方法：ShowExportModelToFbxDialog、ShowExportTexturesDialog、ShowExportSkillConfigDialog、ShowExportAllForUE5Dialog
- [x] 8.3 实现文件/文件夹选择对话框和格式选择
- [x] 8.4 实现导出进度显示和完成通知
- [x] 8.5 验证 UI 菜单项正确显示且在正确条件下启用/禁用

## 9. UE5.7 项目创建

- [x] 9.1 使用 UnrealEditor-Cmd.exe 在 D:\SekiroSkillEditor 创建空白 C++ 项目
- [x] 9.2 验证项目能正确编译和启动
- [x] 9.3 创建插件目录结构 `Plugins/SekiroSkillEditor/`，编写 .uplugin 描述文件和 Build.cs 模块构建文件

## 10. UE5 数据资产和导入管线

- [x] 10.1 创建 `Data/SekiroTaeEvent.h`：定义 FSekiroTaeEvent USTRUCT（Type/TypeName/StartFrame/EndFrame/Parameters/EventCategory）
- [x] 10.2 创建 `Data/SekiroSkillDataAsset.h`：定义 USekiroSkillDataAsset UDataAsset（SkillName/Animation/Events/FrameCount/FrameRate）
- [x] 10.3 创建 `Data/SekiroCharacterData.h`：定义 USekiroCharacterData UDataAsset（CharacterID/SkeletalMesh/Materials/Skills）
- [x] 10.4 创建 `Import/SekiroAssetImporter.h/cpp`：实现 skill_config.json 解析和 USekiroSkillDataAsset 创建
- [x] 10.5 创建 `Import/SekiroAssetFactory.h/cpp`：实现 UFactory 支持 JSON 拖拽导入
- [x] 10.6 创建 `Import/SekiroMaterialSetup.h/cpp`：实现 material_manifest.json 解析和 UMaterialInstance 自动创建/纹理分配
- [x] 10.7 验证从导出 JSON 到 UE5 数据资产的完整导入流程

## 11. UE5 时间轴编辑器

- [x] 11.1 创建 `UI/SSekiroSkillTimeline.h/cpp`：实现基础 Slate Widget 骨架，包含时间轴画布和播放头
- [x] 11.2 实现多轨道事件渲染：按 EventCategory 分组，每个事件绘制为色块条（红/蓝/绿/黄/紫着色）
- [x] 11.3 实现播放头拖拽和动画同步控制（播放/暂停/逐帧）
- [x] 11.4 实现时间轴缩放（鼠标滚轮）和水平滚动
- [x] 11.5 实现事件点击选择和高亮反馈
- [x] 11.6 创建 `UI/SSekiroEventInspector.h/cpp`：实现选中事件的属性面板，显示全部 Parameters 键值对

## 12. UE5 3D 视口和技能浏览器

- [x] 12.1 创建 `Visualization/SekiroSkillPreviewActor.h/cpp`：实现 SkeletalMeshComponent + AnimInstance 的预览角色
- [x] 12.2 实现动画回放与时间轴播放头的同步
- [x] 12.3 创建 `Visualization/SekiroHitboxDrawComponent.h/cpp`：实现 AttackBehavior 事件的红色球体/胶囊体判定框可视化
- [x] 12.4 创建 `Visualization/SekiroEffectMarkerComponent.h/cpp`：实现 SpawnFFX 蓝色标记和 PlaySound 绿色脉冲标记
- [x] 12.5 创建 `UI/SSekiroSkillBrowser.h/cpp`：实现树形视图（按类别分组）和搜索过滤功能
- [x] 12.6 创建 `UI/SekiroSkillEditorTab.h/cpp`：实现主编辑器标签页布局（浏览器 | 视口+时间轴 | 检查器），注册到 Window 菜单

## 13. Python 导入脚本

- [x] 13.1 创建 `Scripts/batch_import.py`：实现批量 FBX 导入（SkeletalMesh + AnimSequence），按角色 ID 组织 Content 路径
- [x] 13.2 创建 `Scripts/setup_materials.py`：实现材质清单解析、Master Material 基础上创建 MaterialInstance、自动分配纹理到正确槽位
- [x] 13.3 创建 `Scripts/import_skill_configs.py`：实现 JSON 解析和 USekiroSkillDataAsset 批量创建
- [x] 13.4 为 Normal Map 纹理自动设置 TC_Normalmap 压缩配置
- [x] 13.5 验证完整端到端流程：C# 导出 → Python 导入 → 技能编辑器中可视化预览

## 14. 端到端验证

- [x] 14.1 使用 CLI 工具完整导出 c0000（玩家角色）：模型 FBX + 全部动画 FBX + 纹理 PNG + 技能 JSON + 材质清单
- [x] 14.2 在 UE5 中通过 Python 脚本导入 c0000 全部资产，验证骨骼网格/动画/纹理/材质正确
- [x] 14.3 在技能编辑器中验证：时间轴事件显示正确、3D 视口动画回放正常、攻击判定框正确出现
- [x] 14.4 测试至少 3 个敌人/Boss 角色（c5000/c7000 系列）的完整导出导入流程
- [x] 14.5 使用 CLI 工具批量导出全部 130+ 角色，验证无崩溃且错误文件被正确跳过

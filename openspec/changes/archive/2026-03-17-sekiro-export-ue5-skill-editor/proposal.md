## 为什么

DSAnimStudio 目前只能**导入** FBX 模型/动画并**导出 HKX 格式动画**，缺乏将 Sekiro 游戏资产（FLVER 模型、骨骼、纹理、动画、TAE 事件）导出为行业标准格式（FBX/PNG/JSON）的能力。同时，没有工具能在 UE5 环境中完整、准确地可视化 Sekiro 的战斗技能配置（Combat Arts、Prosthetic Tools、弹反/架势系统及全部 185 种 TAE 事件类型）。需要构建一套完整的导出管线和专用的 UE5.7 技能编辑器来填补这一空白。

## 变更内容

- **修正** 本变更的完成标准：只有当角色以骨骼蒙皮模型导入 UE，全部动画正确绑定到同一 Skeleton，技能/战斗配置按统一 schema 导入并可在编辑器中复刻时，才视为完成
- **修正** 导出交付物要求：最终交付物必须是可直接导入 UE 的正式资产，不接受 Collada/DAE 静态网格、宽松 fallback 或仅供调试的中间产物作为“完成结果”
- **修正** 实现约束：正式链路禁止继续使用猜测、临时、兼容、替代、简化性质的实现；凡是依赖硬编码角色特例、伪造载体网格、导出后修补、宽松兜底或“尽量猜一个能跑”的逻辑，都必须替换为可追溯的正式数据来源和正式格式契约，否则该链路必须显式失败
- **修正** 导出/导入契约：C# 导出器与 UE5 导入器必须共享同一份 JSON schema、动画命名规则、帧数定义和参数表结构，禁止导出器与导入器各自假定不同字段
- **新增** FLVER 模型→FBX 导出器（网格、骨骼、骨骼权重、材质引用）
- **新增** HKX 动画→FBX 动画导出器（逐帧骨骼关键帧、根运动烘焙）
- **新增** TPF 纹理→PNG/DDS 导出器（支持 DXT1/DXT3/DXT5/BC4/BC5/BC7 解码）
- **新增** TAE 事件 + 游戏参数→JSON 技能配置导出器（全部 185 种 Sekiro 事件类型 + AtkParam/BehaviorParam/SpEffectParam/EquipParamWeapon）
- **新增** 材质清单导出器（材质→纹理路径映射，供 UE5 自动创建材质）
- **新增** SekiroExporter CLI 批量导出工具（支持导出全部 130+ 角色）
- **新增** DSAnimStudio UI 集成（Tools 菜单新增导出选项）
- **新增** UE5.7 项目 SekiroSkillEditor 及其同名编辑器插件
- **新增** UE5 自定义 Slate 时间轴编辑器（多轨道 TAE 事件可视化，按类别着色）
- **新增** UE5 3D 视口预览（骨骼网格动画回放、攻击判定框可视化、特效/音效标记）
- **新增** UE5 技能浏览器（树形视图，分类展示所有战斗技能、义手忍具、攻击动作）
- **新增** Python 批量导入脚本（FBX/纹理/技能配置自动导入 UE5）

## 功能 (Capabilities)

### 新增功能

- `skeletal-import-contract`: 导出器与 UE5 导入器共享的正式资产契约，要求模型导入结果必须为 SkeletalMesh + Skeleton + PhysicsAsset，动画必须全部绑定到目标 Skeleton，技能 JSON 必须满足统一 schema
- `battle-reconstruction`: 在 UE5 中基于 TAE 事件、参数表、DummyPoly 和骨骼姿态复刻攻击判定、特效生成点、音效触发和义手/战技覆盖逻辑
- `flver-fbx-export`: FLVER 模型到 FBX 格式的完整导出，包含网格、骨骼层级、骨骼权重、多 UV 通道、顶点色和材质引用
- `animation-fbx-export`: HKX Havok 动画到 FBX 动画的导出，包含逐帧骨骼变换关键帧采样和根运动烘焙
- `texture-export`: TPF 纹理包到 PNG/DDS 格式的导出，支持 BC 压缩纹理的软件解码
- `skill-config-export`: TAE 时间轴事件和游戏参数到结构化 JSON 的完整导出，涵盖全部 185 种 Sekiro 事件类型及关联的攻击/行为/特效/武器参数
- `material-manifest-export`: 材质到纹理路径的映射清单导出，支持纹理类型分类（Albedo/Normal/Specular/Emissive）
- `sekiro-exporter-cli`: 独立命令行批量导出工具，支持从 Sekiro 游戏目录批量提取和导出全部角色资产
- `dsanimstudio-export-ui`: DSAnimStudio 编辑器中新增的导出菜单集成，提供交互式导出界面
- `ue5-skill-editor-plugin`: UE5.7 专用编辑器插件，包含技能数据资产、JSON 导入管线、Slate 时间轴编辑器、3D 视口可视化和技能浏览器
- `ue5-import-scripts`: Python 自动化脚本，批量导入 FBX 模型/动画、自动配置材质、创建技能数据资产

### 修改功能

（无需修改现有功能的规范级行为）

## 影响

- **DSAnimStudioNETCore 项目**: 新增 `Export/` 目录下的 6 个导出器类，修改 `MenuBar.050.Tools.cs` 和 `TaeEditorScreen.cs` 添加 UI 入口
- **解决方案**: 新增 `SekiroExporter` 控制台项目到 `DSAnimStudioNETCore.sln`
- **NuGet 依赖**: DSAnimStudioNETCore 新增 `AssimpNet 4.1.0`；SekiroExporter 新增 `AssimpNet`、`Pfim`（DDS 解码）
- **子模块**: 需要初始化 `SoulsAssetPipeline`（含 `SoulsFormats` 和 `Havoc`）
- **外部项目**: 在 `D:\SekiroSkillEditor\` 创建全新 UE5.7 C++ 项目及 `SekiroSkillEditor` 插件
- **游戏文件访问**: 读取 `C:\Program Files (x86)\Steam\steamapps\common\Sekiro` 下的 `.chrbnd.dcx`、`.anibnd.dcx`、`.tpf.dcx` 等游戏资产文件
- **UE5 引擎**: 使用 `D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe` 创建项目

## 修正后的验收基线

- 角色模型导入 UE 后必须形成 `USkeletalMesh`、`USkeleton`、`UPhysicsAsset` 三件套；如果导出目录存在模型而 UE 侧未形成骨骼网格，则视为失败
- 动画必须以角色 Skeleton 为绑定目标创建 `UAnimSequence`，禁止出现“动画资产存在但未绑定 Skeleton”或“只能依赖单独静态网格浏览”的结果
- 技能配置导出必须同时包含 `characters`、`params`、动画 `frameCount/frameRate/fileName`、以及可解析的事件参数；缺少参数表或字段不匹配即视为失败
- UE 侧预览必须以真实骨骼姿态和 DummyPoly/参数表为依据复刻攻击判定、特效生成点和音效触发；仅基于事件类别画占位球体不构成完成
- CLI 全量导出结果必须是单一正式交付集；调试/中间格式只能通过显式开关保留，不能计入最终导出结果
- 正式链路中的角色装配、动画交付和材质/参数关联必须来自游戏文件、参数表、规范配置或明确定义的格式契约；无法从正式来源得出的内容不得用猜测值或补丁值代填

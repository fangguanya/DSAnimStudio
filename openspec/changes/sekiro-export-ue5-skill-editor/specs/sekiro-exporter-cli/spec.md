## 新增需求

### 需求:CLI批量导出工具
系统必须提供一个独立的命令行工具（SekiroExporter），能从 Sekiro 游戏目录批量导出全部角色资产。

#### 场景:全量导出单个角色
- **当** 用户执行 `SekiroExporter export-all --game-dir "<Sekiro路径>" --chr c0000 --output ./Export/`
- **那么** 系统必须自动从游戏目录定位并加载该角色的 `.chrbnd.dcx`（模型）、`.anibnd.dcx`（动画+TAE）、相关 `.tpf.dcx`（纹理），并导出完整的正式模型资产、全部正式动画资产、全部纹理（PNG/DDS）、技能配置（JSON）和材质清单（JSON）到指定输出目录；只有可被 UE 正式消费的资产才计入完成结果

#### 场景:批量导出全部角色
- **当** 用户执行 `SekiroExporter export-all --game-dir "<Sekiro路径>" --output ./Export/` （不指定 --chr）
- **那么** 系统必须扫描游戏目录 `chr/` 下的全部 `.chrbnd.dcx` 文件，对每个角色执行完整导出。独立角色的导出必须使用并行处理以提高效率

#### 场景:选择性导出
- **当** 用户使用子命令指定导出类型（`export-model`、`export-anims`、`export-textures`、`export-skills`）
- **那么** 系统必须仅执行指定类型的导出操作，跳过其他类型

### 需求:导出目录结构
CLI 工具必须按照规范的目录结构组织导出文件。

#### 场景:按角色组织输出
- **当** 导出完成
- **那么** 输出目录必须按照以下结构组织：`<output>/<chrID>/Model/`、`<output>/<chrID>/Animations/`、`<output>/<chrID>/Textures/`、`<output>/<chrID>/Skills/`；其中 Model 和 Animations 目录默认只保留正式交付格式，`material_manifest.json` 和 `skill_config.json` 必须与正式导出资产一一对应

#### 场景:中间文件默认不进入最终导出
- **当** 用户未显式指定调试参数（如 `--keep-intermediates`）
- **那么** CLI 不得默认保留 DAE、重复的 glTF/GLB、中间合并产物或其他调试文件；这些文件不得计入最终导出体积和文件数

#### 场景:导出清单反映完整性
- **当** 一个角色导出结束
- **那么** CLI 必须输出或写入角色级导出清单，明确记录模型、动画、纹理、技能配置、参数表和材质清单是否全部成功，以及最终采用的正式交付格式

### 需求:错误处理和日志
CLI 工具必须提供完整的错误处理和进度报告。

#### 场景:处理损坏的游戏文件
- **当** 某个角色的游戏文件无法正确解析
- **那么** 系统必须记录错误信息（包含角色 ID 和文件路径），跳过该角色并继续处理剩余角色

#### 场景:存在不完整导出时的退出状态
- **当** 某个角色缺少模型、动画、纹理、技能配置或参数表中的任意正式交付物
- **那么** CLI 必须在控制台和最终摘要中明确标记该角色失败，批量导出的最终状态不得报告“全部成功”

#### 场景:进度报告
- **当** 批量导出过程中
- **那么** CLI 必须在控制台输出当前处理的角色 ID、已完成数量/总数量、以及每个导出步骤（模型/动画/纹理/技能）的完成状态

### 需求:项目集成
SekiroExporter 必须作为独立的控制台项目集成到现有解决方案中。

#### 场景:解决方案集成
- **当** 构建 DSAnimStudioNETCore.sln
- **那么** SekiroExporter 项目必须包含在解决方案中，目标框架为 net6.0-windows，引用 SoulsFormats 和 SoulsAssetPipeline 项目，以及 AssimpNet、Pfim、Newtonsoft.Json NuGet 包

## 环境配置

### 已确认路径
- **UE5 编辑器命令行**: `D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`
- **UE 工程**: `E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject`
- **导入源目录**: `E:\Sekiro\Export`

### 运行导入 Commandlet
```
"D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject -run=SekiroImport -ExportDir=E:\Sekiro\Export -ChrFilter=c0000 -unattended -nopause -nosplash -nullrhi
```

## 新增需求

### 需求:UE 导入必须由正式 Commandlet 批量处理
系统必须通过 `SekiroImportCommandlet` 批量导入 formal 导出产物，禁止再依赖 UE Python 脚本批量导入模型、纹理或动画。

#### 场景:批量导入骨骼网格与纹理
- **当** 用户对 formal 导出目录运行 `SekiroImportCommandlet`
- **那么** commandlet 必须按角色 ID 组织内容路径，正式导入 SkeletalMesh、Skeleton、纹理与材质实例，并将模型导入与 post-import 校验写入统一报告

#### 场景:批量导入动画
- **当** commandlet 处理角色的 Animations deliverable
- **那么** 它必须为每个正式动画生成绑定到 canonical Skeleton 的 `UAnimSequence`，并在同一次导入中完成轨道重写、payload 覆盖检查与 world-space 语义检查

### 需求:材质与技能配置必须由正式导入链路完成
系统必须由正式导入链路完成材质实例创建、纹理绑定和技能 JSON 到数据资产的转换，禁止继续依赖独立 Python 脚本。

#### 场景:创建材质实例并分配纹理
- **当** commandlet 读取 `material_manifest.json`
- **那么** 它必须通过 C++ 导入逻辑创建 `UMaterialInstanceConstant`，并将导入纹理绑定到对应参数槽位

#### 场景:创建技能数据资产
- **当** commandlet 读取 formal `skill_config.json`
- **那么** 它必须创建对应的 `USekiroSkillDataAsset`，并关联已导入的 `UAnimSequence`，同时把缺失关联作为 warning 或 error 写入正式报告

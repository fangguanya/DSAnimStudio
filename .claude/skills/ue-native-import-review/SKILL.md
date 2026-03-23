---
name: ue-native-import-review
description: 用 UE 原生 C++ 导入链路做正式导入、报告检查和人工抓帧复核。适用于禁止 Python、要求直接在 Unreal 中验证 Skeleton/Mesh/Animation 的场景。
license: MIT
---

这个技能用于 formal Sekiro UE 导入验收，前提是正式路径禁止 Python，所有结论都必须来自 C++ commandlet 报告和 UE 编辑器原生复核。

## 目标

- 构建最新的 `SekiroSkillEditor` C++ 插件
- 运行 `SekiroImportCommandlet` 生成 canonical 资产与 `ue_import_report.json`
- 检查 `selfAnimationValidation`、`animationValidation`、`skeletalMeshValidation`、`postImportChecks`
- 在 UE 编辑器中打开 canonical Skeleton、SkeletalMesh、Animation 做关键帧抓帧复核

## 步骤

1. 构建编辑器

```powershell
"D:\UE_OFFICIAL\UE_5.7\Engine\Build\BatchFiles\Build.bat" SekiroSkillEditorEditor Win64 Development E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject
```

2. 运行正式导入

```powershell
"D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject -run=SekiroImport -ExportDir=E:\Sekiro\Export -ChrFilter=c0000 -Canonical17Only -ImportAnimationsOnly=true -unattended -nopause -nosplash -nullrhi
```

3. 读取导入报告，至少检查以下字段

- `animationSelection.scope == canonical-c0000-preview-17`
- `animationSelection.isCanonicalC0000Preview17 == true`
- `expectedAnimationCount == 17`
- `animationCount == 17`
- `postImportChecks.normalizedAssetsRequiredForSuccess == true`
- `postImportChecks.legacyRawAssetFallbackAccepted == false`
- `postImportChecks.animationWorldSpaceSemanticsValidated == true`
- `skeletalMeshValidation.maximumInverseBindMatrixErrors`
- `animationValidation.clips[*].issues`
- `selfAnimationValidation.failedClipCount == 0`

4. 在 UE 编辑器中做人工复核

- 打开 `/Game/SekiroAssets/Characters/c0000/Mesh/...` 下的 SkeletalMesh
- 打开 `/Game/SekiroAssets/Characters/c0000/Mesh/...` 下的 Skeleton
- 打开 `/Game/SekiroAssets/Characters/c0000/Animations/...` 下的代表性动画

建议至少覆盖这些 clip 类型：

- 待机
- 转身或位移
- 攻击
- 持武器或收刀
- 明显依赖手部语义的 clip

5. 每个 clip 至少检查 3 个关键帧

- 首帧
- 中段代表姿势
- 末段或动作落点

必须观察：

- torso 链是否保持一致
- 左右手 hand/finger subtree 是否与动作语义一致
- root motion 是否与肢体方向脱节
- `face_root` / `weapon_sheath` 等代表性辅助分支是否离体或尖刺

6. 保存抓帧记录

记录以下信息：

- 资产路径
- 关键帧编号或时间点
- 观察结论
- 抓帧文件路径

## 护栏

- 不要用 Python probe、Python validator 或 `-ExecutePythonScript`
- 不要把旧 `SelfTest` 或 RTG 临时内容路径当成正式结论来源
- 如果人工复核与报告冲突，以 canonical 资产的实际表现为准，并把缺口回写到 C++ 报告逻辑
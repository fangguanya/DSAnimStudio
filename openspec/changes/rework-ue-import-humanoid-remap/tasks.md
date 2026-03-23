## 1. 导入阶段重构

- [x] 1.1 为 `SekiroImportCommandlet` 改造为一步式导入流程，明确源数据读取、内存归一化处理和最终资产生成的单路径执行顺序
- [x] 1.2 引入确定性的 Humanoid 归一化配置类型，定义基础矩阵、横向镜像矩阵、镜像轴分析规则、目标父子关系、必选骨骼和辅助骨骼保留策略
- [x] 1.3 为正式支持的 Sekiro humanoid 角色建立首个归一化配置实例，并在缺失关键骨骼或映射歧义时直接失败
- [x] 1.4 删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 及其入口，避免继续保留与正式 import 主链重复的旧路径

## 2. 骨架与模型归一化

- [x] 2.1 实现参考骨架重建逻辑，按目标父子关系生成归一化后的骨骼层级和局部参考姿态
- [x] 2.2 实现模型几何归一化逻辑，对顶点、法线、切线应用统一矩阵与横向镜像矩阵，并处理负行列式下的绕序或手性修正
  验证结果：formal humanoid import 现以统一 `GlobalNormalizationMatrix` 重写 translated scene bind、mesh bind reference 和导入后 `USkeletalMesh` inverse bind；canonical c0000 模型导入后未再出现 bind/time-zero 回退或代表性 inverse bind 超阈值，说明最终生成的 mesh skin binding 已与归一化骨架一致。
- [x] 2.3 重建归一化 SkeletalMesh 的 bind pose、inverse bind pose、骨骼引用和 Skeleton 绑定，确保不再依赖未归一化直出资产的旧参考姿态
- [x] 2.4 将归一化目标朝向更新为最终 UE 对象坐标系额外绕 Z 轴旋转 180°，并确保模型、骨架、动画共用同一矩阵来源
- [x] 2.5 将人形主链父子关系从“仅躯干”扩展为完整 UE humanoid 主链
  验证结果：`SekiroHumanoidImportPipeline` 现已显式重写 `L/R_Clavicle -> L/R_UpperArm -> L/R_Forearm -> L/R_Hand` 与 `L/R_Thigh -> L/R_Calf -> L/R_Foot -> L/R_Toe0` 主链；`L/R_Shoulder`、`L/R_Knee` 等 Sekiro 特有中间骨保留为辅助分支，而不再决定 UE humanoid 主链拓扑。

## 3. 动画逐帧重算

- [x] 3.1 抽取共享的骨骼世界变换计算工具，能够从原始 Skeleton 和动画轨道恢复每帧世界姿态
- [x] 3.2 实现基于角色横向方向镜像矩阵与基础矩阵变换后的世界姿态归一化流程，并按新父子关系重算每一帧局部轨道
- [x] 3.3 将重算后的轨道写回归一化 `UAnimSequence`，验证其绑定到归一化 Skeleton，且保留现有 root-motion 开关与保存流程
- [x] 3.4 扩展动画重算验收，显式校验 hand / wrist / finger-root 分支的掌面前向、掌面横向和掌面法线，防止仅 arm chain 方向正确但手腕语义仍错误
	验证结果：formal import 17 动画批次的 per-clip branch-basis 校验已全部通过；RTG pose validator 与 RTG dynamic animation validator 也已复跑通过，说明验收已经覆盖到 hand / wrist / finger-root 语义，而不再只是 arm chain 主链方向。
- [x] 3.5 补齐 mirrored hand / finger subtree 的侧别局部语义归一化，确保左右手 branch-basis 各自独立通过，而不是依赖单一通用 wrist offset 假设
	验证结果：hand rebase 已从 hand component rotation 扩展为向 finger-root subtree 的 world-space 刚性传播。formal import per-clip hand basis、RTG pose hand basis、RTG animation hand basis 现已左右手同时通过，说明 mirrored left-hand subtree 语义已经回到正式路径内解决，而不是依赖单一 wrist offset 假设。
- [x] 3.6 禁止 formal 支持骨骼在动画重算时静默回退到 bind pose / time-zero / target ref pose
  对 source animation 中存在 baked 运动语义的 formal 支持骨骼，若 payload 缺失、轨道为空或重算后退回 reference-like 静止状态，必须直接把 clip 判为失败，而不是继续导入一个“能播放但动作已错义”的 UAnimSequence。
- [x] 3.7 为 Sekiro 自身动画增加 root-motion 与 world-space 语义一致性重算验收
  在现有 limb direction 和 hand basis 之外，增加 source normalized world vs imported target world 的 root motion 位移/朝向增量对比，以及 torso、foot、代表性辅助分支的 frame-level 语义对比，确保“mesh/skeleton 对了但 self-animation 仍整体不匹配”会被 formal 检查直接抓到。

## 4. import 后检查与报告

- [x] 4.1 重写归一化后的 post-import 检查逻辑，校验骨架层级、横向镜像结果、朝向、grounding 和必需骨骼覆盖，而不是保留旧验证器
- [x] 4.2 扩展角色级导入报告，记录源数据读取、内存骨架重组、最终模型生成、动画重算和 import 后检查的分阶段状态与失败原因
	验证结果：`FormalRunExport/c0000` 当前 whole-package `formalSuccess=false` 仅由无关的 `skills` deliverable 缺失导致；`SekiroImportCommandlet` 现已将这类 non-skeletal deliverable 记为 report `warnings` 而不是 blocking `errors`，并成功生成通过 `validate_ue_import_report.py` 的 full UE import report。
- [x] 4.3 移除或封禁“只要直出未归一化资产可用也算成功”的旧判断分支，确保正式成功必须依赖归一化产物
  验证结果：`ue_import_report.json` 现已显式写出 `normalizedAssetsRequiredForSuccess=true`、`legacyRawAssetFallbackAccepted=false`，并把 success 绑定到归一化后的 skeletal mesh / skeleton / animation post-import 检查，而不是内容目录中旧资产是否还能被找到。
- [x] 4.4 明确并落实 RTG / IK Retargeter 相关 probe 仅作为 post-import 诊断与验收工具，禁止它们承担正式修复职责或成为成功判定前置步骤
	验证结果：formal import report 已稳定写出 `rtgDiagnosticsRequiredForSuccess=false` 与 `rtgDiagnosticsRole=post-import-only`；本轮修复也落在 import-side subtree 传播，而不是 RTG builder 补偿逻辑。
- [x] 4.5 扩展角色级导入报告，显式记录 Sekiro 自身动画匹配诊断
  报告必须至少记录：formal 支持骨骼的 baked payload 覆盖率、静默回退/冻结骨骼计数、root motion delta 对比、torso/limb direction 最小 dot、hand basis 最小 dot，以及代表性辅助分支差异，避免“只有 success=true / false 而不知道为什么自动画完全不匹配”。

## 5. 回归验证

- [x] 5.1 使用 c0000 的正式模型导入样本验证归一化后 SkeletalMesh、Skeleton、PhysicsAsset 和参考姿态结果
- [x] 5.2 使用现有选定动画批次验证逐帧重算后的动画绑定、覆盖率、横向镜像结果和 root motion 结果
- [x] 5.3 更新相关检查记录或操作说明，明确正式 UE 导入现在验收的是归一化后的人形资产，且检查在 import 后按需单独执行
	验证结果：full model+animation rerun 现在可在 `skills` 缺失时继续完成 skeletal import，并写出 `errors=[]`、`warnings=["missing skills deliverable"]`、`success=true` 的 UE import report；现有 full-report validator 已对该报告通过。
- [x] 5.4 使用 c0000 模型与现有 17 动画批次回归验证额外 Z 轴 180° 旋转后的最终朝向与左臂语义保持一致
- [x] 5.5 新增正式 import self-test probe、retarget pose validator、retarget animation self-test probe 和宿主侧阈值单元测试，确保检查可程序化失败而不是只靠人工读日志
- [x] 5.6 对剩余 RTG 动态 palm basis 失败增加 frame-level wrist/hand diagnostics，验证误差是否表现为恒定局部 wrist offset；若确认，需把对应语义回写到 import commandlet，而不是把 RTG 补偿当成最终修复
	验证结果：frame-level wrist diagnostics 已复跑，左右手 `suggestsConstantWristOffset` 均为 `false`，排除了恒定局部 wrist offset 假设；对应正式修复已经回写到 import-side hand subtree 传播，而不是留在 RTG 补偿层。
- [x] 5.7 增加针对 hand rebase 传播范围的诊断，分别证明或排除“只修正 hand component rotation 而未同步传播 finger-root subtree world geometry”是否就是左手镜像侧 palm basis 失败的直接原因
	验证结果：将 hand rebase 从根骨朝向修正扩展为对子树 world geometry 的刚性传播后，formal import per-clip hand basis 与 RTG animation hand basis 同步恢复通过，直接证明原失败点就是 subtree propagation 缺失。
- [x] 5.8 为 canonical `/Game/SekiroAssets/Characters/c0000` 资产增加直接层级自检
  验证结果：`_formal_import_selftest_probe.py` 已切到正式角色内容路径，并通过宿主 validator 直接校验 `Master -> Pelvis -> Spine -> Spine1 -> Spine2 -> Neck -> Head`、双侧 arm 主链、双侧 leg 主链和 `face_root -> Head` 关系，避免继续读取旧 `SelfTest` 内容导致的假结论。
- [x] 5.9 选取代表性 Sekiro 自身动画批次复跑 self-animation validator
  至少覆盖待机、转身/位移、攻击、收刀/持武器、含明显手部语义的 clip，确认导入后动画在 root motion、躯干链、四肢链、hand basis 和代表性辅助分支上都与 source normalized motion 一致。
- [x] 5.10 为 self-animation 增加人工并排复核操作说明
  定义 source normalized pose/animation 与 imported animation 的并排检查方法、关键帧抽样点和失败判据，确保“程序化通过但肉眼明显不匹配”的情况能在 formal 回归中被稳定复现和记录。

## 6. UE 原生诊断与人工抓帧复核

- [x] 6.1 用 UE 原生 C++ 报告替代 Python validator 的正式结论
  把 clip 级与角色级 self-animation 诊断直接写入 `ue_import_report.json`，包括失败 clip、payload coverage、direction/hand basis/rotation/root-motion/world-space 聚合指标，禁止正式结论再依赖 Python validator。
  验证结果：当前 `SekiroImportCommandlet` 已支持 `-Canonical17Only` 预设，并在 canonical `/Game/SekiroAssets/Characters/c0000` 路径对 17 clip 批次直接写出 `schemaVersion=2.4` 报告；本轮 UE 原生命令链复跑结果为 `expectedAnimationCount=17`、`animationCount=17`、`postImportChecks.animationWorldSpaceSemanticsValidated=true`，正式结论已不再依赖任何 Python validator。

- [x] 6.2 在 UE 编辑器中打开 canonical skeleton / mesh / animation 做人工抓帧复核
  按 canonical `/Game/SekiroAssets/Characters/c0000` 内容路径打开 Skeleton、SkeletalMesh 与代表性动画，在首帧/中段/末段抓帧并记录资产路径、关键帧和观察结论，禁止再依赖 Python scene driver。
  验证结果：`_ExportCheck/UserVerifyRun/ViewportVisualCheck` 已保存 `reference_window_capture.png`、`animation_window_capture.png` 与对应 manifest；其中 `viewport_scene_manifest.json` 明确记录 source mesh `/Game/SekiroAssets/Characters/c0000/Mesh/c0000`、source animation `/Game/SekiroAssets/Characters/c0000/Animations/a000_201030`、target animation `/Game/Retargeted/SekiroToManny/Animations/a000_201030_Manny`，`reference_ready.json` / `animation_ready.json` 也确认 reference 与 animation 两阶段窗口抓取均成功完成。

## 7. 辅助分支与肩部残差补充

- [x] 7.1 将代表性辅助分支（至少 `left_clavicle` / `right_clavicle`，以及当前角色存在的 `face_root` / `FaceRoot`、`W_Sheath` / `weapon_sheath`）写入 formal import spec 与角色级导入报告契约，禁止这些指标继续缺席正式报告。

- [x] 7.2 新增面向导入后辅助分支的 probe/validator，能够对代表性 branch root 的参考跨度、动画跨度和 owner 距离做程序化诊断，用于定位“主干通过但局部网格尖刺/离体”的具体 branch root 与具体帧。

- [x] 7.3 在 spec / design 中明确：RTG 预览里肩根区域的残差若来自 Sekiro `Shoulder` 与 Manny clavicle 拓扑非等价，则只能作为诊断说明；真正的失败条件仍应落在等价链段动态指标和 hand basis 语义上。

- [x] 7.4 为“骨架/动画语义通过但蒙皮仍错误”的情况增加 SkeletalMesh inverse bind 重建与 formal 报告校验，确保导入后 mesh skin binding 不再被 `success=true` 掩盖。
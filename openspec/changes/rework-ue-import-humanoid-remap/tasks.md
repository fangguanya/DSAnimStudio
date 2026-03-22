## 1. 导入阶段重构

- [x] 1.1 为 `SekiroImportCommandlet` 改造为一步式导入流程，明确源数据读取、内存归一化处理和最终资产生成的单路径执行顺序
- [x] 1.2 引入确定性的 Humanoid 归一化配置类型，定义基础矩阵、横向镜像矩阵、镜像轴分析规则、目标父子关系、必选骨骼和辅助骨骼保留策略
- [x] 1.3 为正式支持的 Sekiro humanoid 角色建立首个归一化配置实例，并在缺失关键骨骼或映射歧义时直接失败
- [x] 1.4 删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 及其入口，避免继续保留与正式 import 主链重复的旧路径

## 2. 骨架与模型归一化

- [x] 2.1 实现参考骨架重建逻辑，按目标父子关系生成归一化后的骨骼层级和局部参考姿态
- [ ] 2.2 实现模型几何归一化逻辑，对顶点、法线、切线应用统一矩阵与横向镜像矩阵，并处理负行列式下的绕序或手性修正
- [x] 2.3 重建归一化 SkeletalMesh 的 bind pose、inverse bind pose、骨骼引用和 Skeleton 绑定，确保不再依赖未归一化直出资产的旧参考姿态
- [x] 2.4 将归一化目标朝向更新为最终 UE 对象坐标系额外绕 Z 轴旋转 180°，并确保模型、骨架、动画共用同一矩阵来源

## 3. 动画逐帧重算

- [x] 3.1 抽取共享的骨骼世界变换计算工具，能够从原始 Skeleton 和动画轨道恢复每帧世界姿态
- [x] 3.2 实现基于角色横向方向镜像矩阵与基础矩阵变换后的世界姿态归一化流程，并按新父子关系重算每一帧局部轨道
- [x] 3.3 将重算后的轨道写回归一化 `UAnimSequence`，验证其绑定到归一化 Skeleton，且保留现有 root-motion 开关与保存流程
- [x] 3.4 扩展动画重算验收，显式校验 hand / wrist / finger-root 分支的掌面前向、掌面横向和掌面法线，防止仅 arm chain 方向正确但手腕语义仍错误
	验证结果：formal import 17 动画批次的 per-clip branch-basis 校验已全部通过；RTG pose validator 与 RTG dynamic animation validator 也已复跑通过，说明验收已经覆盖到 hand / wrist / finger-root 语义，而不再只是 arm chain 主链方向。
- [x] 3.5 补齐 mirrored hand / finger subtree 的侧别局部语义归一化，确保左右手 branch-basis 各自独立通过，而不是依赖单一通用 wrist offset 假设
	验证结果：hand rebase 已从 hand component rotation 扩展为向 finger-root subtree 的 world-space 刚性传播。formal import per-clip hand basis、RTG pose hand basis、RTG animation hand basis 现已左右手同时通过，说明 mirrored left-hand subtree 语义已经回到正式路径内解决，而不是依赖单一 wrist offset 假设。

## 4. import 后检查与报告

- [x] 4.1 重写归一化后的 post-import 检查逻辑，校验骨架层级、横向镜像结果、朝向、grounding 和必需骨骼覆盖，而不是保留旧验证器
- [x] 4.2 扩展角色级导入报告，记录源数据读取、内存骨架重组、最终模型生成、动画重算和 import 后检查的分阶段状态与失败原因
	验证结果：`FormalRunExport/c0000` 当前 whole-package `formalSuccess=false` 仅由无关的 `skills` deliverable 缺失导致；`SekiroImportCommandlet` 现已将这类 non-skeletal deliverable 记为 report `warnings` 而不是 blocking `errors`，并成功生成通过 `validate_ue_import_report.py` 的 full UE import report。
- [ ] 4.3 移除或封禁“只要直出未归一化资产可用也算成功”的旧判断分支，确保正式成功必须依赖归一化产物
- [x] 4.4 明确并落实 RTG / IK Retargeter 相关 probe 仅作为 post-import 诊断与验收工具，禁止它们承担正式修复职责或成为成功判定前置步骤
	验证结果：formal import report 已稳定写出 `rtgDiagnosticsRequiredForSuccess=false` 与 `rtgDiagnosticsRole=post-import-only`；本轮修复也落在 import-side subtree 传播，而不是 RTG builder 补偿逻辑。

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
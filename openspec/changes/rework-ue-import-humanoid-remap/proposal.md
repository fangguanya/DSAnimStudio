## 为什么

当前 UE 导入链路除了 `SekiroImportCommandlet` 之外，还散落着 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 等独立补救或验证代码。这些路径把“导入”“修正”“验证”拆成了多套重复职责，但正式导入路径本身仍然没有完成沿角色横向方向的统一镜像矩阵变换、Sekiro 非标准骨架向 Unreal Humanoid 骨架组织的重建，也不会在骨架重组后逐帧重算动画局部变换。结果是导入资产即使“可导入”，也仍然不满足 UE 人形骨架组织和后续工具链复用的要求。

现在需要把这些变换正式纳入单一导入合同，并删除冗余 commandlet/验证器代码，而不是继续依赖导出侧姿态、导入后人工处理、独立 retarget 命令或内嵌验证器补救路径。这样才能让模型、骨骼、动画在导入完成时就具备一致的 Humanoid 结构；如需检查，则在 import 结束后按需要单独执行检查逻辑。

## 变更内容

- 为 UE 导入 commandlet 增加正式的一步导入归一化流程：先读取模型、骨骼和动画数据，在内存中基于角色横向方向执行统一镜像矩阵变换、固定基变换、最终 UE 对象坐标系绕 Z 轴 180° 旋转和骨架重组，再一次性生成最终模型、骨骼和动画资产。
- 在导入阶段按 Unreal Humanoid 的目标骨架组织重建 Sekiro 骨骼父子关系，而不是只检查现有参考骨架是否“看起来正确”。
- 在骨架重组后，对所有受影响骨骼的参考姿态和每一帧动画局部变换执行坐标系重算，保证新父子关系下的世界姿态与期望 Humanoid 朝向一致，并在最终 UE 对象坐标系中额外绕 Z 轴旋转 180°。
- 在动画重算与验收中，显式覆盖 hand、wrist 与 finger-root 分支基底，而不是只验证上臂/前臂主链方向；若掌面横向、掌面法线或腕部朝向不一致，则必须视为导入侧动画重算失败。
- 删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 等冗余 commandlet/验证器路径，把修正与构建职责收敛到正式 import 主链；如需检查，则在 import 后按需单独执行检查。
- 修正现有导入工具中与上述正式路径冲突的实现和检查逻辑，包括只验证不修正、模型与动画变换路径不一致、以及无法证明重组后动画仍与 Skeleton 正确绑定的问题。
- 明确 `RTG_*` / IK Retargeter 资源、retarget pose probe 和 retarget animation probe 仅用于诊断与验收，不得成为正式修复路径，也不得替代 `SekiroImportCommandlet` 内的骨架重组与逐帧动画重算。
- 明确所谓“targeted wrist compensation in the RTG build step”仅指在 RTG 构建脚本里对 target wrist/hand 链临时施加确定性的局部补偿，用来判断剩余误差是否表现为恒定的腕部局部空间偏置；它只能作为 post-import 诊断实验，不能成为正式成功条件或长期修复路径。
- 明确 hand / wrist / finger-root 分支的验收必须按左右侧分别记录和判定；如果一侧通过、另一侧仍失败，则该结果必须被解释为 mirrored-side local-semantics 缺口，而不是可被单一通用 wrist offset 掩盖的“基本通过”。
- **BREAKING**: UE 正式导入合同将不再把“直接消费导出 glTF 并仅做姿态验证”视为最终成功条件；正式成功必须包含导入侧的人形骨架归一化结果。

## 功能 (Capabilities)

### 新增功能
- `ue-import-humanoid-normalization`: 定义 UE 导入 commandlet 如何对模型、骨架和动画执行基于横向方向的镜像矩阵变换、统一矩阵变换、最终对象空间 Z 轴 180° 旋转、Humanoid 骨架重组，以及逐帧动画重算。

### 修改功能
- `skeletal-import-contract`: 正式导入合同需要从“验证 formal skeleton root 与动画绑定”扩展为“允许并要求导入侧执行受控的人形骨架归一化，然后再验证绑定和结果”。
- `ue5-skill-editor-plugin`: 插件侧骨骼模型和动画导入契约需要明确要求导入结果输出 Unreal Humanoid 组织的 Skeleton，并保证重组后的动画仍然可播放、可验证。

## 影响

- UE 导入主链：`SekiroImportCommandlet` 的数据读取、内存归一化处理、最终资产生成、报告生成和检查顺序。
- 参考姿态与动画变换计算：需要新增或抽取骨架重组、局部/世界变换重算、横向镜像矩阵应用的共享逻辑。
- 朝向验收语义：需要明确正式验收针对的是额外 Z 轴 180° 后的最终对象空间结果，左/右语义按归一化后的角色朝向判定，而不是继续把某个绝对 X 正负号固定等同于左/右。
- 代码收敛：删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 及其重复职责，将 import 后检查改为按需触发的独立检查逻辑。
- 现有 retarget/FK 采样代码：其中可复用的逐帧轨道处理思路需要并入正式 import 实现，而不是继续保留为独立 commandlet。
- 现有 RTG/IK Retargeter 诊断资产：仅作为发现导入逻辑缺陷的观测工具存在；若 RTG 资源看起来不正确，应回溯到 `SekiroImportCommandlet` 的归一化配置、目标骨架组织和逐帧动画重算，而不是引入新的正式 retarget 修补步骤。
- 当前调试状态：formal exporter、formal importer 和 import-side branch-basis 验收已经重新跑通；RTG pose probe 通过，剩余失败集中在 RTG 动态动画的 palm basis / wrist rotation semantics。最近把 arm chain 延伸到 hand 后，`forearm -> hand` 主链方向问题已经收敛，剩余失败主要是 palm forward、palm lateral 和 palm normal；再结合 importer palm-basis rebase 已把右手 dynamic palm basis 拉回阈值内、左手仍显著低于阈值这一结果，当前瓶颈已经进一步缩小到 mirrored left-hand hand/finger subtree 的局部旋转语义，而不是 import 主链是否完成，也不是一个对左右手都同样成立的 generic wrist offset。
- OpenSpec 规格：新增人形归一化能力，并修改现有正式导入与插件导入契约。
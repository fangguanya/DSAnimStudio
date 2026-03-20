## 为什么

当前 UE 导入链路除了 `SekiroImportCommandlet` 之外，还散落着 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 等独立补救或验证代码。这些路径把“导入”“修正”“验证”拆成了多套重复职责，但正式导入路径本身仍然没有完成沿角色左右方向的统一镜像矩阵变换、Sekiro 非标准骨架向 Unreal Humanoid 骨架组织的重建，也不会在骨架重组后逐帧重算动画局部变换。结果是导入资产即使“可导入”，也仍然不满足 UE 人形骨架组织和后续工具链复用的要求。

现在需要把这些变换正式纳入单一导入合同，并删除冗余 commandlet/验证器代码，而不是继续依赖导出侧姿态、导入后人工处理、独立 retarget 命令或内嵌验证器补救路径。这样才能让模型、骨骼、动画在导入完成时就具备一致的 Humanoid 结构；如需检查，则在 import 结束后按需要单独执行检查逻辑。

## 变更内容

- 为 UE 导入 commandlet 增加正式的一步导入归一化流程：先读取模型、骨骼和动画数据，在内存中基于角色左右方向执行统一镜像矩阵变换、固定基变换和骨架重组，再一次性生成最终模型、骨骼和动画资产。
- 在导入阶段按 Unreal Humanoid 的目标骨架组织重建 Sekiro 骨骼父子关系，而不是只检查现有参考骨架是否“看起来正确”。
- 在骨架重组后，对所有受影响骨骼的参考姿态和每一帧动画局部变换执行坐标系重算，保证新父子关系下的世界姿态与期望 Humanoid 朝向一致。
- 删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 等冗余 commandlet/验证器路径，把修正与构建职责收敛到正式 import 主链；如需检查，则在 import 后按需单独执行检查。
- 修正现有导入工具中与上述正式路径冲突的实现和检查逻辑，包括只验证不修正、模型与动画变换路径不一致、以及无法证明重组后动画仍与 Skeleton 正确绑定的问题。
- **BREAKING**: UE 正式导入合同将不再把“直接消费导出 glTF 并仅做姿态验证”视为最终成功条件；正式成功必须包含导入侧的人形骨架归一化结果。

## 功能 (Capabilities)

### 新增功能
- `ue-import-humanoid-normalization`: 定义 UE 导入 commandlet 如何对模型、骨架和动画执行基于左右方向的镜像矩阵变换、统一矩阵变换、Humanoid 骨架重组，以及逐帧动画重算。

### 修改功能
- `skeletal-import-contract`: 正式导入合同需要从“验证 formal skeleton root 与动画绑定”扩展为“允许并要求导入侧执行受控的人形骨架归一化，然后再验证绑定和结果”。
- `ue5-skill-editor-plugin`: 插件侧骨骼模型和动画导入契约需要明确要求导入结果输出 Unreal Humanoid 组织的 Skeleton，并保证重组后的动画仍然可播放、可验证。

## 影响

- UE 导入主链：`SekiroImportCommandlet` 的数据读取、内存归一化处理、最终资产生成、报告生成和检查顺序。
- 参考姿态与动画变换计算：需要新增或抽取骨架重组、局部/世界变换重算、左右镜像矩阵应用的共享逻辑。
- 代码收敛：删除 `SekiroRetargetCommandlet`、`SekiroHumanoidValidation`、`SekiroValidationCommandlet` 及其重复职责，将 import 后检查改为按需触发的独立检查逻辑。
- 现有 retarget/FK 采样代码：其中可复用的逐帧轨道处理思路需要并入正式 import 实现，而不是继续保留为独立 commandlet。
- OpenSpec 规格：新增人形归一化能力，并修改现有正式导入与插件导入契约。
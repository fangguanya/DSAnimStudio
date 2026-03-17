> 归档说明：本变更的未完成任务已续接到 `add-sekiro-ue-skill-and-asset-pipeline`，当前文件保留原始勾选状态作为历史记录。

## 1. 正式合同冻结

- [x] 1.1 将 CLI 正式导出模式的唯一模型/动画格式固定为 `glTF 2.0 (.gltf + .bin)`，移除正式模式下的 `fbx/fbxa/glb/dae` 成功判定
- [x] 1.2 将 CLI 正式导出模式的唯一纹理格式固定为 `PNG`，移除正式模式下的 `dds` 成功判定
- [x] 1.3 为正式模式增加显式契约检查，禁止任何 fallback、silent skip、warning-only 成功路径进入正式统计
- [x] 1.4 明确区分正式模式与调试/兼容模式，确保历史兼容输出不再混入正式交付目录和正式验收

## 2. 模型与动画导出正式化

- [x] 2.1 删除模型导出中的多格式轮询逻辑，并将失败原因标准化输出到角色级验收报告
- [x] 2.2 删除动画导出中的多格式轮询逻辑，并将失败原因标准化输出到角色级验收报告
- [x] 2.3 盘点并收紧 `GltfPostProcessor` 的职责，把其失败从 warning 改为正式失败
- [x] 2.4 将当前 glTF 后处理修补项按 scene-side / writer-side 分类；可前移项前移到原生构建阶段，writer-side 项形成替换计划，减少正式链路对后处理层的依赖
- [x] 2.5 为模型 glTF 增加结构校验，验证 skin root、joint 列表、inverse bind matrices、scene roots 和 buffer URI 全部满足正式合同
- [x] 2.6 为动画 glTF 增加结构校验，验证单 clip、节点映射、TRS 通道和骨架一致性全部满足正式合同
- [x] 2.7 为正式导出冻结多部件角色的 canonical assembly profile，并禁止把 DSAnimStudio 当前预览装配状态直接当作正式模型定义
- [x] 2.8 将 formal skeleton root 写入模型构建阶段而不是事后猜测修补，并确保 `skin.skeleton`、scene root 和 mesh 绑定引用同一主骨架
- [x] 2.9 定义正式模型/动画交付文件的命名规则，使其与 animation resolution 结果和角色报告字段一致

## 3. HKX 解析与动画采样正式化

- [x] 3.1 确认 Sekiro 骨架 HKX 与动画 HKX 的唯一正式解析路径，并形成代码级入口约束
- [x] 3.2 移除 `Program.cs` 中多 `HKXVariation` 回退树，改为单一路径解析与失败上报
- [x] 3.3 移除 `AnimationToFbxExporter.cs` 中 tagfile/legacy 混合回退树，改为与正式解析入口一致的读取方式
- [x] 3.4 将动画帧采样中的 identity 填充替换为显式失败，并补充失败上下文信息
- [x] 3.5 为骨架节点集、动画节点集和 glTF skin/joints 建立一致性检查，禁止后续猜测式补齐
- [x] 3.6 将 TAE 条目 ID、引用链求解结果、HKX ID 和 `SourceAnimFileName` 纳入统一正式解析函数，并在动画与技能导出两端共用
- [x] 3.7 为统一正式解析函数定义最小结果对象和失败码，覆盖 request_tae_id、resolved_tae_id、resolved_hkx_id、source_anim_file、deliverable_anim_file、animation_source_anibnd、skill_source_anibnd
- [x] 3.8 对“无条目、无 HKX、无 source file、结果歧义、多候选冲突”建立 fail-closed 行为和角色报告映射
- [x] 3.9 对主角等多 TAE 文件 ANIBND 保留 `taeBindIndex/category` 身份，按编辑器现有 `SplitAnimID` 语义求解引用链与 HKX，禁止先 merge 本地 TAE ID 再按裸 `aXXX_YYYYYY` 过滤

## 4. 纹理导出正式化

- [x] 4.1 移除 `TextureExporter` 中 `SkipUnsupported = true` 的正式模式默认行为
- [x] 4.2 移除 PNG 失败时转存 DDS 的正式路径，改为显式失败与验收记录
- [ ] 4.3 建立 Sekiro 当前使用纹理编码的正式支持矩阵，并在代码中按支持矩阵判定成功/失败
- [ ] 4.4 为正式 PNG 输出定义统一命名、色彩/通道语义和材质引用契约
- [x] 4.5 在角色报告中记录每张纹理的源格式、正式输出文件名和失败原因，避免“角色纹理失败但定位不到具体纹理”

## 5. 技能与参数正式化

- [x] 5.1 将技能导出的模板加载从“有则加载”改为正式前置条件，并在缺失时显式失败
- [x] 5.2 移除 `SkillConfigExporter` 中 `rawBytes` 降级输出和未知类型按 `s32` 猜读逻辑
- [x] 5.3 用正式参数定义驱动读取替换 `PARAM_Hack`，覆盖当前正式交付所需参数表
- [x] 5.4 收紧技能配置 schema，确保动画元数据、事件字段、参数字段、TAE→HKX→交付动画求解关系和类型定义全部可追溯
- [x] 5.5 将 TAE 参数块中的尾随零填充正式化：允许显式全零 padding，禁止把保留字节猜成正式字段
- [x] 5.6 让技能配置直接复用 formal animation resolution 结果对象，禁止在 `SkillConfigExporter` 内部再次独立推断动画文件名

## 6. 验收与报告

- [x] 6.1 为每个角色生成结构化验收报告，覆盖模型、动画、纹理、技能、参数和 UE 可用性
- [x] 6.2 将角色成功判定改为所有正式子项全通过才算成功
- [ ] 6.3 收紧 UE 导入验证标准，确保 SkeletalMesh、Skeleton、PhysicsAsset 和动画绑定全部纳入正式通过条件
- [x] 6.4 更新 CLI 汇总统计，禁止部分成功、兼容成功和调试成功混入正式成功数
- [x] 6.5 在角色报告中记录动画源、技能源、解析后的 HKX/交付动画标识和 formal assembly profile
- [x] 6.6 为角色报告定义稳定 schema 和 failure code 列表，保证回归脚本可以按字段自动比较而不是依赖日志文本

## 7. 回归与清理

- [x] 7.1 选择 `c0000` 作为首个正式化回归样本，完成规范化多部件装配、单动画、纹理、技能和验收报告的全链验证
- [ ] 7.2 对至少一个非主角角色执行同样的正式化回归，验证正式合同不依赖 `c0000` 特例
- [ ] 7.3 清理已经不再属于正式链路的导出选项、日志措辞和历史注释，避免误导后续开发
- [ ] 7.4 重新生成正式统计基线，替换历史上基于 fallback 和宽松成功标准得到的结论
- [x] 7.5 增加专门回归用例验证 formal skeleton root 存在、唯一且与动画节点绑定一致，防止 glTF 骨架根问题回归

## 8. 从 `sekiro-export-ue5-skill-editor` 转移的遗留任务

- [ ] 8.1 验证 DSAnimStudio UI 菜单项正确显示且在正确条件下启用/禁用（原 8.5，需要实际运行 DSAnimStudio 验证）
- [ ] 8.2 验证技能编辑器时间轴、3D 视口、攻击判定框（原 14.5，需要在 UE5 编辑器中手动验证）
- [ ] 8.3 梳理模型导出正式格式，确保默认导出只保留可导入 UE 骨骼模型的单一交付物；DAE/其他中间格式仅在 `--keep-intermediates` 下保留（原 15.5）
- [ ] 8.4 验证 FLVER 导出结果在 UE 中确实生成 SkeletalMesh + Skeleton + PhysicsAsset；存在模型输入但未形成骨骼网格时必须判失败（原 15.6）
- [ ] 8.5 修复动画导出/导入契约，确保每个 clip 是单文件完整动画，并与角色 Skeleton 正确绑定；零轨道、错 Skeleton、空动画不得计为成功（原 15.7）
- [ ] 8.6 让 3D 视口基于真实 DummyPoly、BehaviorParam 和 AtkParam 驱动攻击判定可视化，禁止以 Actor 原点偏移球体替代正式复刻（原 15.11）
- [ ] 8.7 让特效点、音效点、WeaponArt、义手覆盖在属性面板和视口中显示解析后的实际生效结果，而不是仅显示原始事件类别（原 15.12）
- [ ] 8.8 为 CLI 和导入命令增加角色级验收报告，逐角色输出模型、动画、纹理、技能配置、参数表和 UE 资产导入结果，禁止只给总数；并在新验收标准下重新跑全量导出与全量导入，替换掉历史上不准确的“全部成功”结论（合并原 15.13、15.14）
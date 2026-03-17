## 新增需求

### 需求:TAE事件完整导出
系统必须能将 Sekiro TAE (TimeAct Editor) 文件中的全部事件数据导出为结构化 JSON 格式，涵盖全部 185 种事件类型。

#### 场景:导出单个动画的全部事件
- **当** 用户指定一个角色的 ANIBND 文件进行技能配置导出
- **那么** 系统必须遍历 ANIBND 中所有 TAE 文件的所有动画，对每个动画导出其全部事件（Event），每个事件包含：事件类型ID（type）、类型名称（typeName，从 TAE.Template.SDT.xml 解析）、起始帧（startFrame）、结束帧（endFrame）、全部参数键值对（parameters）

#### 场景:正确解析事件参数类型
- **当** TAE 事件包含不同类型的参数（s32/f32/u8/s16/u16/u32/b 等）
- **那么** 系统必须根据 TAE.Template.SDT.xml 中定义的参数类型正确解析和序列化每个参数值，整数参数保持整数类型，浮点参数保持浮点类型，布尔参数保持布尔类型

#### 场景:导出攻击行为事件
- **当** 动画包含 AttackBehavior (ID 1)、BulletBehavior (ID 2)、CommonBehavior (ID 5) 事件
- **那么** 导出数据必须包含完整的攻击参数：AtkBehaviorSubID、DummyPolyID、BehaviorJudgeID、攻击类型、方向等所有子参数

#### 场景:导出特效和音效事件
- **当** 动画包含 SpawnFFX (ID 95-123)、PlaySound (ID 128-132) 事件
- **那么** 导出数据必须包含特效 ID、生成点 DummyPoly ID、循环类型、音效类型等完整参数

#### 场景:导出Sekiro专有事件
- **当** 动画包含 Sekiro 专有事件（ID 708-720, 730：忍具覆盖、武器隐藏、邪教类型等）
- **那么** 导出数据必须正确识别和包含这些 Sekiro 特有事件的全部参数

### 需求:游戏参数导出
系统必须导出与战斗技能相关的全部游戏参数表。

#### 场景:导出攻击参数表
- **当** 用户请求导出技能配置
- **那么** 系统必须从游戏文件中读取并导出 AtkParam（攻击参数），包含 HitType、HitSourceType、DummyPolySource 等字段

#### 场景:导出行为参数表
- **当** 用户请求导出技能配置
- **那么** 系统必须导出 BehaviorParam（行为参数），包含 VariationID、BehaviorJudgeID、EzStateBehaviorType、RefType（Attack/Bullet/SpEffect）及关联的 Stamina/MP/HeroPoint 消耗值

#### 场景:导出特效参数表
- **当** 用户请求导出技能配置
- **那么** 系统必须导出 SpEffectParam（特殊效果参数）和 EquipParamWeapon（武器装备参数），保留所有字段值

### 需求:武器技艺和义手忍具配置导出
系统必须导出 Sekiro 特有的武器技艺（Combat Arts）和义手忍具（Prosthetic Tools）配置信息。

#### 场景:导出武器技艺事件
- **当** 动画包含 WeaponArt 相关事件（ID 330 WeaponArtFPConsumption、ID 331 AddSpEffect_WeaponArts、ID 332 WeaponArtWeaponStyleCheck）
- **那么** 导出数据必须包含武器技艺的 SpEffectID、SpEffectID_LowFP、消耗值和样式检查参数

#### 场景:导出义手忍具DummyPoly覆盖配置
- **当** 角色装备义手忍具时使用 SekiroProstheticOverride
- **那么** 系统必须导出 SekiroProstheticOverrideDmy0-3 的 DummyPoly 索引配置，记录每个义手忍具对应的攻击点位覆盖

### 需求:导出JSON格式规范
导出的 JSON 必须遵循统一的 schema，便于 UE5 端解析。

#### 场景:JSON输出结构
- **当** 完成技能配置导出
- **那么** JSON 文件必须遵循以下顶层结构：`version`（格式版本）、`gameType`（"SDT"）、`characters`（角色数组）、`params`（各参数表对象）、`prosthetics`（义手和覆盖配置，可选但一旦存在必须结构化）

#### 场景:JSON数组与字段契约
- **当** UE5 导入器消费 skill config JSON
- **那么** `characters` 必须是数组，每个元素至少包含 `id` 和 `animations`；`animations` 必须是数组，每个元素至少包含 `name`、`fileName`、`frameCount`、`frameRate`、`events`

#### 场景:动画ID规范化
- **当** 导出动画事件数据
- **那么** 每个动画必须使用 Sekiro 标准命名格式 `aXXX_YYYYYY`（如 `a000_003000`）作为 `name` 字段，并包含 `frameCount`、`frameRate` 和 `fileName` 字段；`fileName` 必须与最终导出的动画资产文件名一致

### 需求:事件时间和参数命名一致性
导出的事件数据必须使用 UE 导入侧可直接消费的帧字段和语义化参数键名。

#### 场景:使用帧域字段而不是秒域字段
- **当** 导出 TAE 事件
- **那么** 每个事件必须输出 `startFrame` 和 `endFrame` 字段，数值与源动画帧域一致；不得仅输出 `startTime`、`endTime` 而缺少帧字段

#### 场景:禁止未解释的占位参数名
- **当** TAE.Template.SDT.xml 中存在参数名称定义
- **那么** 导出结果必须使用模板定义的参数名；只有模板确实缺失定义时，才允许回退到 `param_<offset>`，并且必须同时输出原始字节偏移和数据类型

### 需求:战斗参数关联可复刻
技能配置导出不仅要保存原始 TAE 事件，还必须输出足以在 UE 中复刻战斗语义的参数关联。

#### 场景:导出行为到参数表的引用关系
- **当** 动画包含 AttackBehavior、BulletBehavior、CommonBehavior 或 Sekiro 专有战斗事件
- **那么** 导出结果必须保留可解析的 `BehaviorJudgeID`、`AtkParamID`、`BulletID`、`SpEffectID`、`DummyPolyID`、`DummyPolySource`、`VariationID` 等关联字段，使 UE 端能够顺着事件 → BehaviorParam → AtkParam/SpEffectParam/EquipParamWeapon 的链路重建战斗含义

#### 场景:导出DummyPoly与义手覆盖关系
- **当** 事件或参数表引用 DummyPoly，或角色存在义手/战技覆盖配置
- **那么** 导出结果必须同时给出原始 DummyPoly 索引、覆盖后的 DummyPoly 索引，以及覆盖生效条件；UE 端不能依赖硬编码猜测这些点位

#### 场景:参数表成为正式交付物的一部分
- **当** 执行单角色或全量技能配置导出
- **那么** `params` 中必须至少包含 `AtkParam`、`BehaviorParam`、`SpEffectParam`、`EquipParamWeapon` 四类参数；缺少任意一类都不视为完整导出

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
- **那么** JSON 文件必须遵循以下顶层结构：`version`（格式版本）、`gameType`（"SDT"）、`characters`（角色→动画→事件的嵌套结构）、`params`（各参数表数组）

#### 场景:动画ID规范化
- **当** 导出动画事件数据
- **那么** 每个动画必须使用 Sekiro 标准命名格式 `aXXX_YYYYYY`（如 `a000_003000`）作为键名，并包含 `frameCount` 和 `fileName` 字段

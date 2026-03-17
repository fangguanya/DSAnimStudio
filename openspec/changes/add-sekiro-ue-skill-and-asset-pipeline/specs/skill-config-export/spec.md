## 新增需求

### 需求:UE 复刻语义链路导出
技能配置导出必须提供足以在 UE 中复刻 Sekiro 战斗语义的正式链路，而不只是原始事件列表。

#### 场景:导出事件到参数链路
- **当** 动画包含 AttackBehavior、BulletBehavior、CommonBehavior、SpawnFFX、AddSpEffect、WeaponArt 或其他 Sekiro 战斗事件
- **那么** 导出结果必须同时给出事件原始参数、可追溯的 `BehaviorParam`、`AtkParam`、`SpEffectParam`、`EquipParamWeapon` 关联字段，以及 UE 侧复刻所需的解析结果键

### 需求:DummyPoly 与覆盖结果显式导出
技能配置导出必须显式记录 DummyPoly 原始索引、解析来源和覆盖结果，禁止 UE 端自行猜测。

#### 场景:导出义手和战技覆盖结果
- **当** 某个技能事件、参数表或角色配置涉及义手忍具覆盖、战技样式检查或 DummyPoly 覆盖
- **那么** 导出结果必须同时包含原始 DummyPoly 索引、覆盖后的 DummyPoly 索引、覆盖生效条件和解析来源；禁止仅输出未解释的原始数值
## 新增需求

### 需求:技能数据资产系统
UE5 插件必须定义专用的数据资产类型来存储 Sekiro 技能配置信息。

#### 场景:创建技能数据资产
- **当** 从导出的 JSON 文件导入技能配置
- **那么** 系统必须创建 USekiroSkillDataAsset 实例，包含：技能名称（FString）、关联动画引用（UAnimSequence*）、TAE 事件列表（TArray<FSekiroTaeEvent>）、动画帧数和帧率

#### 场景:TAE事件数据结构
- **当** 存储 TAE 事件
- **那么** FSekiroTaeEvent 结构体必须包含：Type (int32)、TypeName (FString)、StartFrame (float)、EndFrame (float)、Parameters (TMap<FString, FString>)，以及 EventCategory (枚举：Attack/Effect/Sound/Movement/State)

#### 场景:角色数据资产
- **当** 导入角色数据
- **那么** 系统必须创建 USekiroCharacterData 资产，包含：角色 ID、骨骼网格引用、材质映射、全部关联技能数据资产引用

### 需求:JSON导入管线
插件必须支持从导出的 JSON 文件自动创建技能数据资产。

#### 场景:导入技能配置JSON
- **当** 用户在 Content Browser 中导入 skill_config.json 文件
- **那么** 系统必须解析 JSON 中的全部动画和事件数据，为每个动画创建一个 USekiroSkillDataAsset，自动关联已导入的 UAnimSequence 资产（通过动画 ID 匹配）

#### 场景:导入材质清单并自动配置材质
- **当** 用户导入 material_manifest.json 文件
- **那么** 系统必须为每个材质创建 UMaterialInstance，自动将已导入的纹理资产分配到正确的材质槽位（BaseColor/Normal/Specular/Emissive）

### 需求:多轨道时间轴编辑器
插件必须提供自定义 Slate 时间轴界面，以多轨道方式可视化 TAE 事件。

#### 场景:显示动画时间轴
- **当** 用户在技能编辑器中选择一个技能
- **那么** 时间轴必须显示一个可拖动的播放头（scrubber），时间轴范围对应动画的总帧数，支持播放/暂停/逐帧控制

#### 场景:按类别显示事件轨道
- **当** 技能包含 TAE 事件
- **那么** 事件必须按类别分组显示在不同轨道上，每个事件显示为一个色块条（bar），长度对应 StartFrame 到 EndFrame。颜色编码：红色=攻击/伤害（AttackBehavior/BulletBehavior/CommonBehavior）、蓝色=特效（SpawnFFX/AddSpEffect）、绿色=音效（PlaySound）、黄色=移动/镜头（RumbleCam/TurnSpeed/LockedOn）、紫色=状态（IFrames/Opacity/WeaponArt/AnimFlags）

#### 场景:事件选择和属性查看
- **当** 用户在时间轴上点击一个事件色块
- **那么** 该事件必须被高亮选中，右侧属性面板（SSekiroEventInspector）必须显示该事件的全部属性：Type、TypeName、StartFrame、EndFrame 及全部 Parameters 键值对

#### 场景:时间轴缩放和滚动
- **当** 用户在时间轴区域滚动鼠标滚轮或拖拽
- **那么** 时间轴必须支持水平缩放（放大/缩小时间精度）和水平滚动（查看不同时间段的事件）

### 需求:3D视口预览
插件必须提供 3D 视口用于预览角色动画和战斗事件的空间表现。

#### 场景:骨骼网格动画回放
- **当** 用户选择一个技能并播放
- **那么** 3D 视口中必须显示角色骨骼网格，按照关联的 FBX 动画进行实时回放，播放位置与时间轴播放头同步

#### 场景:攻击判定框可视化
- **当** 时间轴播放头处于 AttackBehavior 事件的活跃帧范围内
- **那么** 3D 视口中必须在对应的 DummyPoly 位置显示红色半透明球体/胶囊体表示攻击判定框；当播放头离开活跃帧时判定框必须消失

#### 场景:特效生成点标记
- **当** 时间轴播放头处于 SpawnFFX 事件的活跃帧范围内
- **那么** 3D 视口中必须在对应的 DummyPoly 位置显示蓝色标记图标表示特效生成点

#### 场景:音效触发标记
- **当** 时间轴播放头经过 PlaySound 事件的起始帧
- **那么** 3D 视口中必须短暂显示绿色脉冲标记表示音效触发

### 需求:技能浏览器
插件必须提供树形结构的技能浏览面板。

#### 场景:按类别分组显示技能
- **当** 用户打开技能编辑器
- **那么** 左侧面板必须以树形视图展示所有已导入的技能，按类别分组：普通攻击（Light/Heavy Attacks）、Combat Arts（战技）、Prosthetic Tools（义手忍具）、其他动作

#### 场景:选择技能加载预览
- **当** 用户在浏览器中点击一个技能条目
- **那么** 系统必须加载该技能的数据资产，时间轴显示其事件，3D 视口加载对应的动画

#### 场景:技能搜索过滤
- **当** 用户在浏览器顶部搜索框输入关键词
- **那么** 树形视图必须实时过滤，仅显示名称或动画 ID 包含关键词的技能条目

### 需求:编辑器Tab布局
插件必须提供统一的编辑器标签页布局。

#### 场景:主编辑器布局
- **当** 用户打开 SekiroSkillEditor 标签页
- **那么** 界面必须包含四个区域：左侧技能浏览器、中上方 3D 视口、中下方时间轴编辑器、右侧事件属性检查器。各区域必须支持 Slate Splitter 拖拽调整大小

#### 场景:编辑器注册
- **当** SekiroSkillEditor 插件加载
- **那么** 系统必须在 UE5 编辑器的 Window 菜单或工具栏中注册一个入口，用户点击后可打开 SekiroSkillEditor 标签页

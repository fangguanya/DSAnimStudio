using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using SoulsAssetPipeline.Animation;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports TAE (TimeAct Editor) events and game parameters to structured JSON.
    /// Handles all 185 Sekiro TAE event types with full parameter serialization.
    /// </summary>
    public class SkillConfigExporter
    {
        /// <summary>
        /// TAE template parsed from TAE.Template.SDT.xml.
        /// Maps event type ID to its definition (name, parameter types/names).
        /// </summary>
        private Dictionary<int, TaeEventTemplate> _eventTemplates;

        public class TaeEventTemplate
        {
            public int TypeId { get; set; }
            public string TypeName { get; set; }
            public List<TaeParamDef> Parameters { get; set; } = new List<TaeParamDef>();
        }

        public class TaeParamDef
        {
            public string Name { get; set; }
            public string DataType { get; set; } // s32, f32, u8, s16, b
            public Dictionary<int, string> EnumEntries { get; set; }
            public int? AssertValue { get; set; }
        }

        public SkillConfigExporter()
        {
        }

        /// <summary>
        /// Load TAE template from XML file (e.g., TAE.Template.SDT.xml).
        /// </summary>
        public void LoadTemplate(string templatePath)
        {
            _eventTemplates = new Dictionary<int, TaeEventTemplate>();

            var doc = XDocument.Load(templatePath);
            var root = doc.Root;

            foreach (var actionElem in root.Elements("action"))
            {
                var template = new TaeEventTemplate();
                template.TypeId = int.Parse(actionElem.Attribute("id").Value);
                template.TypeName = actionElem.Attribute("name")?.Value ?? $"Event_{template.TypeId}";

                foreach (var paramElem in actionElem.Elements())
                {
                    // Skip <entry> elements (they are children of param elements)
                    if (paramElem.Name.LocalName == "entry") continue;

                    var paramDef = new TaeParamDef();
                    paramDef.DataType = paramElem.Name.LocalName; // s32, f32, u8, s16, b
                    paramDef.Name = paramElem.Attribute("name")?.Value ?? "";

                    // Parse assert value
                    var assertAttr = paramElem.Attribute("assert");
                    if (assertAttr != null && int.TryParse(assertAttr.Value, out int assertVal))
                        paramDef.AssertValue = assertVal;

                    // Parse enum entries
                    var entries = paramElem.Elements("entry");
                    if (entries.Any())
                    {
                        paramDef.EnumEntries = new Dictionary<int, string>();
                        foreach (var entry in entries)
                        {
                            var nameAttr = entry.Attribute("name");
                            var valueAttr = entry.Attribute("value");
                            if (nameAttr != null && valueAttr != null &&
                                int.TryParse(valueAttr.Value, out int val))
                            {
                                paramDef.EnumEntries[val] = nameAttr.Value;
                            }
                        }
                    }

                    template.Parameters.Add(paramDef);
                }

                _eventTemplates[template.TypeId] = template;
            }
        }

        /// <summary>
        /// Export all TAE actions from a TAE file to a structured JSON object.
        /// </summary>
        public JObject ExportTaeEvents(TAE tae, string characterId = null)
        {
            var result = new JObject();
            result["version"] = "1.0";
            result["gameType"] = "SDT";
            result["characterId"] = characterId ?? "";

            var animationsObj = new JObject();

            foreach (var anim in tae.Animations)
            {
                string animKey = FormatAnimationKey(anim.ID);
                var animObj = new JObject();
                animObj["id"] = anim.ID;

                // Get anim file name from header
                string animFileName = "";
                if (anim.Header is TAE.Animation.AnimFileHeader.Standard stdHeader)
                    animFileName = stdHeader.AnimFileName ?? "";
                animObj["name"] = animFileName;

                var eventsArray = new JArray();

                if (anim.Actions != null)
                {
                    foreach (var action in anim.Actions)
                    {
                        var eventObj = SerializeAction(action);
                        eventsArray.Add(eventObj);
                    }
                }

                animObj["events"] = eventsArray;
                animObj["eventCount"] = eventsArray.Count;

                animationsObj[animKey] = animObj;
            }

            result["animations"] = animationsObj;
            result["totalAnimations"] = tae.Animations.Count;
            result["totalEvents"] = tae.Animations.Sum(a => a.Actions?.Count ?? 0);

            return result;
        }

        /// <summary>
        /// Serialize a single TAE action to JSON, using template for parameter names/types.
        /// </summary>
        private JObject SerializeAction(TAE.Action action)
        {
            var obj = new JObject();
            obj["type"] = action.Type;
            obj["startTime"] = action.StartTime;
            obj["endTime"] = action.EndTime;

            // Category classification
            obj["category"] = ClassifyEventCategory(action.Type);

            // Get template for this event type
            TaeEventTemplate template = null;
            _eventTemplates?.TryGetValue(action.Type, out template);

            obj["typeName"] = template?.TypeName ?? $"Event_{action.Type}";

            // Serialize parameters
            var paramsObj = new JObject();

            if (action.ParameterBytes != null && template != null)
            {
                var paramBytes = action.ParameterBytes;
                int offset = 0;

                foreach (var paramDef in template.Parameters)
                {
                    object value = null;
                    string paramName = !string.IsNullOrEmpty(paramDef.Name)
                        ? paramDef.Name : $"param_{offset}";

                    try
                    {
                        switch (paramDef.DataType)
                        {
                            case "s32":
                                if (offset + 4 <= paramBytes.Length)
                                {
                                    value = BitConverter.ToInt32(paramBytes, offset);
                                    offset += 4;
                                }
                                break;
                            case "f32":
                                if (offset + 4 <= paramBytes.Length)
                                {
                                    value = BitConverter.ToSingle(paramBytes, offset);
                                    offset += 4;
                                }
                                break;
                            case "u8":
                                if (offset + 1 <= paramBytes.Length)
                                {
                                    value = (int)paramBytes[offset];
                                    offset += 1;
                                }
                                break;
                            case "s16":
                                if (offset + 2 <= paramBytes.Length)
                                {
                                    value = (int)BitConverter.ToInt16(paramBytes, offset);
                                    offset += 2;
                                }
                                break;
                            case "b":
                                if (offset + 1 <= paramBytes.Length)
                                {
                                    value = paramBytes[offset] != 0;
                                    offset += 1;
                                }
                                break;
                            default:
                                // Unknown type, try reading as s32
                                if (offset + 4 <= paramBytes.Length)
                                {
                                    value = BitConverter.ToInt32(paramBytes, offset);
                                    offset += 4;
                                }
                                break;
                        }
                    }
                    catch
                    {
                        value = null;
                    }

                    if (value != null)
                    {
                        paramsObj[paramName] = JToken.FromObject(value);
                    }
                }
            }
            else if (action.ParameterBytes != null)
            {
                // No template - dump raw bytes as hex
                paramsObj["rawBytes"] = BitConverter.ToString(action.ParameterBytes).Replace("-", "");
            }

            obj["parameters"] = paramsObj;
            return obj;
        }

        /// <summary>
        /// Classify event type into a category for UI grouping/coloring.
        /// </summary>
        public static string ClassifyEventCategory(int eventType)
        {
            if (eventType == 1 || eventType == 2 || eventType == 4 || eventType == 5)
                return "Attack";
            if (eventType == 0 || eventType == 16 || eventType == 17 ||
                (eventType >= 24 && eventType <= 32))
                return "Animation";
            if (eventType >= 95 && eventType <= 123)
                return "Effect";
            if (eventType >= 128 && eventType <= 132)
                return "Sound";
            if (eventType >= 64 && eventType <= 94)
                return "Movement";
            if (eventType >= 224 && eventType <= 256)
                return "State";
            if (eventType >= 330 && eventType <= 332)
                return "WeaponArt";
            if (eventType >= 700 && eventType <= 720)
                return "SekiroSpecial";
            if (eventType >= 300 && eventType <= 310)
                return "Camera";
            return "Other";
        }

        /// <summary>
        /// Format animation ID to aXXX_YYYYYY key format.
        /// </summary>
        private static string FormatAnimationKey(long animId)
        {
            int prefix = (int)(animId / 1000000);
            int suffix = (int)(animId % 1000000);
            return $"a{prefix:D3}_{suffix:D6}";
        }

        /// <summary>
        /// Export complete skill configuration for a character including TAE events.
        /// </summary>
        public void ExportToFile(TAE tae, string outputPath, string characterId = null)
        {
            var json = ExportTaeEvents(tae, characterId);

            var dir = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(outputPath,
                json.ToString(Formatting.Indented));
        }
    }
}

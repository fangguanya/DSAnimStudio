using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using SoulsFormats;
using SoulsAssetPipeline.Animation;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports TAE (TimeAct Editor) events and game parameters to structured JSON.
    /// Handles all 185 Sekiro TAE event types with full parameter serialization.
    /// </summary>
    public class SkillConfigExporter
    {
        public sealed class SkillConfigExportContext
        {
            public string CharacterId { get; set; }
            public float FrameRate { get; set; } = 30.0f;
            public JObject Params { get; set; }
            public JObject Prosthetics { get; set; }
            public IReadOnlyList<FLVER2> ModelFlvers { get; set; }
            public Func<string, string> ResolveAnimationFileName { get; set; }
        }

        /// <summary>
        /// TAE template parsed from TAE.Template.SDT.xml.
        /// Maps event type ID to its definition (name, parameter types/names).
        /// </summary>
        private Dictionary<int, TaeEventTemplate> _eventTemplates;

        private static readonly Dictionary<string, int> ParamTypeSizes = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase)
        {
            ["b"] = 1,
            ["u8"] = 1,
            ["x8"] = 1,
            ["s8"] = 1,
            ["u16"] = 2,
            ["x16"] = 2,
            ["s16"] = 2,
            ["u32"] = 4,
            ["s32"] = 4,
            ["f32"] = 4,
            ["x32"] = 4,
            ["u64"] = 8,
            ["x64"] = 8,
            ["s64"] = 8,
            ["f64"] = 8,
            ["f32grad"] = 8,
        };

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

            if (_eventTemplates.Count == 0)
                throw new InvalidOperationException($"No TAE event templates were loaded from '{templatePath}'.");
        }

        /// <summary>
        /// Export all TAE actions from a TAE file to the canonical skill config JSON schema.
        /// </summary>
        public JObject ExportSkillConfig(TAE tae, SkillConfigExportContext context)
        {
            if (tae == null)
                throw new ArgumentNullException(nameof(tae));

            if (_eventTemplates == null || _eventTemplates.Count == 0)
                throw new InvalidOperationException("Formal skill export requires a loaded TAE template.");

            context ??= new SkillConfigExportContext();

            var result = new JObject
            {
                ["version"] = "2.0",
                ["gameType"] = "SDT",
                ["characters"] = new JArray(),
                ["params"] = context.Params ?? new JObject(),
            };

            if (context.Prosthetics != null)
                result["prosthetics"] = context.Prosthetics;

            var characterObj = new JObject
            {
                ["id"] = context.CharacterId ?? string.Empty,
                ["animations"] = new JArray(),
            };

            var dummyPolys = ExportDummyPolys(context.ModelFlvers);
            if (dummyPolys.Count > 0)
                characterObj["dummyPolys"] = dummyPolys;

            foreach (var anim in tae.Animations)
            {
                string animName = FormatAnimationKey(anim.ID);
                string sourceAnimFileName = GetSourceAnimationFileName(anim);
                string resolutionKey = NormalizeAnimationReference(sourceAnimFileName);
                if (string.IsNullOrWhiteSpace(resolutionKey))
                    resolutionKey = animName;

                string resolvedFileName = context.ResolveAnimationFileName?.Invoke(resolutionKey);
                if (string.IsNullOrWhiteSpace(resolvedFileName))
                    throw new InvalidOperationException($"Formal skill export requires a matching exported animation deliverable for '{animName}' (resolved via '{resolutionKey}').");

                var animObj = new JObject
                {
                    ["id"] = anim.ID,
                    ["name"] = animName,
                    ["fileName"] = Path.GetFileName(resolvedFileName),
                    ["frameRate"] = context.FrameRate,
                    ["frameCount"] = ResolveFrameCount(anim, context.FrameRate),
                };

                if (!string.IsNullOrWhiteSpace(sourceAnimFileName))
                    animObj["sourceAnimFileName"] = sourceAnimFileName;

                var eventsArray = new JArray();

                if (anim.Actions != null)
                {
                    foreach (var action in anim.Actions)
                    {
                        try
                        {
                            var eventObj = SerializeAction(action, context.FrameRate);
                            eventsArray.Add(eventObj);
                        }
                        catch (Exception ex)
                        {
                            int paramByteCount = action?.ParameterBytes?.Length ?? 0;
                            throw new InvalidOperationException(
                                $"Formal skill export failed for animation '{animName}' (id={anim.ID}) event type {action?.Type} with {paramByteCount} parameter bytes: {ex.Message}",
                                ex);
                        }
                    }
                }

                animObj["events"] = eventsArray;
                animObj["eventCount"] = eventsArray.Count;
                ((JArray)characterObj["animations"]).Add(animObj);
            }

            ((JArray)result["characters"]).Add(characterObj);

            return result;
        }

        /// <summary>
        /// Serialize a single TAE action to JSON, using template for parameter names/types.
        /// </summary>
        private JObject SerializeAction(TAE.Action action, float frameRate)
        {
            var obj = new JObject
            {
                ["type"] = action.Type,
                ["startFrame"] = action.StartTime * frameRate,
                ["endFrame"] = action.EndTime * frameRate,
            };

            // Category classification
            obj["category"] = ClassifyEventCategory(action.Type);

            // Get template for this event type
            TaeEventTemplate template = null;
            _eventTemplates?.TryGetValue(action.Type, out template);

            if (template == null)
                throw new InvalidOperationException($"No formal TAE template definition exists for event type {action.Type}.");

            int expectedByteCount = GetExpectedParameterByteCount(template);
            int actualByteCount = action.ParameterBytes?.Length ?? 0;
            if (actualByteCount < expectedByteCount)
            {
                string rawBytes = actualByteCount == 0
                    ? "<empty>"
                    : BitConverter.ToString(action.ParameterBytes).Replace("-", string.Empty);
                throw new InvalidOperationException(
                    $"Template for event type {action.Type} expects at least {expectedByteCount} parameter bytes, but TAE action contains {actualByteCount} bytes (raw={rawBytes}).");
            }

            obj["typeName"] = template.TypeName;

            // Serialize parameters
            var paramsArray = new JArray();

            if (action.ParameterBytes != null && template != null)
            {
                var paramBytes = action.ParameterBytes;
                int offset = 0;

                foreach (var paramDef in template.Parameters)
                {
                    int byteOffset = offset;
                    var parsedParam = TryReadParameterValue(paramBytes, ref offset, paramDef.DataType);
                    paramsArray.Add(CreateParamEntry(
                        string.IsNullOrWhiteSpace(paramDef.Name) ? $"param_{byteOffset}" : paramDef.Name,
                        paramDef.DataType,
                        byteOffset,
                        parsedParam,
                        string.IsNullOrWhiteSpace(paramDef.Name) ? "template-missing-name" : "template"));
                }

                ValidateTrailingZeroPadding(action.Type, paramBytes, offset);
            }

            obj["params"] = paramsArray;
            return obj;
        }

        private static JObject CreateParamEntry(string name, string dataType, int byteOffset, object value, string source)
        {
            var result = new JObject
            {
                ["name"] = name,
                ["dataType"] = dataType,
                ["byteOffset"] = byteOffset,
                ["source"] = source,
            };

            if (value == null)
            {
                result["value"] = JValue.CreateNull();
                return result;
            }

            result["value"] = JToken.FromObject(value);
            return result;
        }

        private static int GetExpectedParameterByteCount(TaeEventTemplate template)
        {
            int total = 0;
            foreach (var param in template.Parameters)
            {
                if (!ParamTypeSizes.TryGetValue(param.DataType ?? string.Empty, out int size))
                    throw new NotSupportedException($"Formal skill export does not support TAE param type '{param.DataType}'.");

                total += size;
            }

            return total;
        }

        private static void ValidateTrailingZeroPadding(int actionType, byte[] paramBytes, int consumedByteCount)
        {
            if (paramBytes == null || consumedByteCount >= paramBytes.Length)
                return;

            for (int index = consumedByteCount; index < paramBytes.Length; index++)
            {
                if (paramBytes[index] != 0)
                {
                    string rawBytes = BitConverter.ToString(paramBytes).Replace("-", string.Empty);
                    throw new InvalidOperationException(
                        $"Template for event type {actionType} consumed {consumedByteCount} bytes, but remaining bytes were not zero padding (raw={rawBytes}).");
                }
            }
        }

        private static object TryReadParameterValue(byte[] paramBytes, ref int offset, string dataType)
        {
            if (!ParamTypeSizes.TryGetValue(dataType ?? string.Empty, out int size))
                throw new NotSupportedException($"Formal skill export does not support TAE param type '{dataType}'.");

            if (offset + size > paramBytes.Length)
                throw new InvalidOperationException($"TAE parameter data ended unexpectedly while reading type '{dataType}' at byte offset {offset}.");

            object result;
            switch ((dataType ?? string.Empty).ToLowerInvariant())
            {
                case "b":
                    result = paramBytes[offset] != 0;
                    break;
                case "u8":
                    result = (int)paramBytes[offset];
                    break;
                case "x8":
                    result = paramBytes[offset].ToString("X2");
                    break;
                case "s8":
                    result = (int)(sbyte)paramBytes[offset];
                    break;
                case "u16":
                    result = (int)BitConverter.ToUInt16(paramBytes, offset);
                    break;
                case "x16":
                    result = BitConverter.ToUInt16(paramBytes, offset).ToString("X4");
                    break;
                case "s16":
                    result = (int)BitConverter.ToInt16(paramBytes, offset);
                    break;
                case "u32":
                    result = (long)BitConverter.ToUInt32(paramBytes, offset);
                    break;
                case "f32":
                    result = BitConverter.ToSingle(paramBytes, offset);
                    break;
                case "f32grad":
                    result = new JObject
                    {
                        ["start"] = BitConverter.ToSingle(paramBytes, offset),
                        ["end"] = BitConverter.ToSingle(paramBytes, offset + 4),
                    };
                    break;
                case "x32":
                    result = BitConverter.ToString(paramBytes, offset, 4).Replace("-", string.Empty);
                    break;
                case "u64":
                    result = BitConverter.ToUInt64(paramBytes, offset);
                    break;
                case "x64":
                    result = BitConverter.ToUInt64(paramBytes, offset).ToString("X16");
                    break;
                case "s64":
                    result = BitConverter.ToInt64(paramBytes, offset);
                    break;
                case "f64":
                    result = BitConverter.ToDouble(paramBytes, offset);
                    break;
                case "s32":
                default:
                    result = BitConverter.ToInt32(paramBytes, offset);
                    break;
            }

            offset += size;
            return result;
        }

        private static string GetSourceAnimationFileName(TAE.Animation anim)
        {
            if (anim?.Header is TAE.Animation.AnimFileHeader.Standard stdHeader)
                return stdHeader.AnimFileName ?? string.Empty;

            if (anim?.Header is TAE.Animation.AnimFileHeader.ImportOtherAnim importHeader)
                return importHeader.AnimFileName ?? string.Empty;

            return string.Empty;
        }

        private static string NormalizeAnimationReference(string animationReference)
        {
            if (string.IsNullOrWhiteSpace(animationReference))
                return string.Empty;

            string normalized = Path.GetFileNameWithoutExtension(animationReference.Trim());
            if (normalized.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase)
                || normalized.EndsWith(".hkt", StringComparison.OrdinalIgnoreCase))
            {
                normalized = Path.GetFileNameWithoutExtension(normalized);
            }

            return normalized;
        }

        private static int ResolveFrameCount(TAE.Animation anim, float frameRate)
        {
            if (anim?.Actions == null || anim.Actions.Count == 0)
                return 0;

            float maxEndFrame = anim.Actions.Max(action => Math.Max(action.StartTime, action.EndTime) * frameRate);
            return (int)Math.Ceiling(maxEndFrame);
        }

        private static JArray ExportDummyPolys(IReadOnlyList<FLVER2> modelFlvers)
        {
            var result = new JArray();
            if (modelFlvers == null)
                return result;

            var seen = new HashSet<string>(StringComparer.Ordinal);
            for (int flverIndex = 0; flverIndex < modelFlvers.Count; flverIndex++)
            {
                var flver = modelFlvers[flverIndex];
                if (flver?.Dummies == null)
                    continue;

                foreach (var dummy in flver.Dummies)
                {
                    string attachBoneName = ResolveBoneName(flver, dummy.AttachBoneIndex);
                    string key = $"{dummy.ReferenceID}:{dummy.AttachBoneIndex}:{dummy.Position.X:F4}:{dummy.Position.Y:F4}:{dummy.Position.Z:F4}";
                    if (!seen.Add(key))
                        continue;

                    result.Add(new JObject
                    {
                        ["referenceId"] = dummy.ReferenceID,
                        ["parentBoneIndex"] = dummy.ParentBoneIndex,
                        ["parentBoneName"] = ResolveBoneName(flver, dummy.ParentBoneIndex),
                        ["attachBoneIndex"] = dummy.AttachBoneIndex,
                        ["attachBoneName"] = attachBoneName,
                        ["useUpwardVector"] = dummy.UseUpwardVector,
                        ["flverIndex"] = flverIndex,
                        ["position"] = SerializeVector(dummy.Position),
                        ["forward"] = SerializeVector(dummy.Forward),
                        ["upward"] = SerializeVector(dummy.Upward),
                    });
                }
            }

            return result;
        }

        private static JObject SerializeVector(System.Numerics.Vector3 value)
        {
            return new JObject
            {
                ["x"] = value.X,
                ["y"] = value.Y,
                ["z"] = value.Z,
            };
        }

        private static string ResolveBoneName(FLVER2 flver, short boneIndex)
        {
            if (flver?.Nodes == null || boneIndex < 0 || boneIndex >= flver.Nodes.Count)
                return string.Empty;

            return flver.Nodes[boneIndex].Name ?? string.Empty;
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
        public void ExportToFile(TAE tae, string outputPath, SkillConfigExportContext context)
        {
            var json = ExportSkillConfig(tae, context);

            var dir = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(outputPath,
                json.ToString(Formatting.Indented));
        }

        public void ExportToFile(TAE tae, string outputPath, string characterId = null)
        {
            ExportToFile(tae, outputPath, new SkillConfigExportContext
            {
                CharacterId = characterId,
            });
        }
    }
}

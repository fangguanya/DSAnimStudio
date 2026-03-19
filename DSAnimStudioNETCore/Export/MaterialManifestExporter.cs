using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using SoulsFormats;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports material-to-texture path mapping manifest.
    /// Maps each material's textures to their exported file names and classified types
    /// (BaseColor, Normal, Specular, Emissive, Roughness, BlendMask).
    /// </summary>
    public class MaterialManifestExporter
    {
        public const string FormalMasterMaterialPath = "/Game/SekiroAssets/Materials/M_SekiroMaster.M_SekiroMaster";

        private sealed class ManifestMaterialEntry
        {
            public int Index { get; set; }
            public string Name { get; set; }
            public string Mtd { get; set; }
            public JArray Textures { get; set; }
            public JObject TextureBindings { get; set; }
            public JObject ScalarParameters { get; set; }
            public JArray MeshIndices { get; } = new JArray();
        }

        /// <summary>
        /// Texture slot type classification matching FlverMaterial.cs classification logic.
        /// </summary>
        public enum TextureSlotType
        {
            BaseColor,
            Normal,
            Specular,
            Emissive,
            Roughness,
            BlendMask,
            Unknown
        }

        /// <summary>
        /// Generate material manifest from a FLVER2 model.
        /// </summary>
        public JObject GenerateManifest(FLVER2 flver, string textureFileExtension = ".png")
        {
            return GenerateManifest(new[] { flver }, textureFileExtension);
        }

        /// <summary>
        /// Generate material manifest from the final assembled FLVER set for a character.
        /// </summary>
        public JObject GenerateManifest(IReadOnlyList<FLVER2> flvers, string textureFileExtension = ".png")
        {
            if (flvers == null)
                throw new ArgumentNullException(nameof(flvers));

            var validFlvers = flvers.Where(flver => flver != null).ToList();
            if (validFlvers.Count == 0)
                throw new ArgumentException("At least one non-null FLVER is required.", nameof(flvers));

            var result = new JObject();
            result["version"] = "2.0";
            result["deliveryMode"] = "formal-only";
            result["textureFormat"] = textureFileExtension.TrimStart('.');
            result["requiredOutputFormat"] = "png";
            result["masterMaterialPath"] = FormalMasterMaterialPath;

            var materialsArray = new JArray();
            var manifestEntries = new Dictionary<string, ManifestMaterialEntry>(StringComparer.Ordinal);
            int nextMaterialIndex = 0;
            int globalMeshIndexOffset = 0;

            foreach (var flver in validFlvers)
            {
                for (int matIdx = 0; matIdx < flver.Materials.Count; matIdx++)
                {
                    var mat = flver.Materials[matIdx];
                    string key = BuildManifestSlotKey(mat.Name);

                    if (!manifestEntries.TryGetValue(key, out var manifestEntry))
                    {
                        manifestEntry = new ManifestMaterialEntry
                        {
                            Index = nextMaterialIndex++,
                            Name = mat.Name ?? string.Empty,
                            Mtd = mat.MTD ?? string.Empty,
                            Textures = BuildTextureArray(mat, textureFileExtension),
                            TextureBindings = BuildTextureBindings(mat, textureFileExtension),
                            ScalarParameters = BuildScalarParameters(mat, textureFileExtension),
                        };
                        manifestEntries.Add(key, manifestEntry);
                    }
                    else
                    {
                        manifestEntry.Name = mat.Name ?? string.Empty;
                        manifestEntry.Mtd = mat.MTD ?? string.Empty;
                        manifestEntry.Textures = BuildTextureArray(mat, textureFileExtension);
                        manifestEntry.TextureBindings = BuildTextureBindings(mat, textureFileExtension);
                        manifestEntry.ScalarParameters = BuildScalarParameters(mat, textureFileExtension);
                    }

                    for (int meshIdx = 0; meshIdx < flver.Meshes.Count; meshIdx++)
                    {
                        if (flver.Meshes[meshIdx].MaterialIndex == matIdx)
                            manifestEntry.MeshIndices.Add(globalMeshIndexOffset + meshIdx);
                    }
                }

                globalMeshIndexOffset += flver.Meshes.Count;
            }

            foreach (var manifestEntry in manifestEntries.Values.OrderBy(entry => entry.Index))
            {
                var matObj = new JObject();
                matObj["index"] = manifestEntry.Index;
                matObj["name"] = manifestEntry.Name;
                matObj["mtd"] = manifestEntry.Mtd;
                matObj["materialInstanceKey"] = BuildMaterialInstanceKey(manifestEntry.Name, manifestEntry.Index);
                matObj["slotName"] = manifestEntry.Name;
                matObj["textures"] = manifestEntry.Textures;
                matObj["textureBindings"] = manifestEntry.TextureBindings;
                matObj["scalarParameters"] = manifestEntry.ScalarParameters;
                matObj["meshIndices"] = manifestEntry.MeshIndices;
                materialsArray.Add(matObj);
            }

            result["materials"] = materialsArray;
            result["materialCount"] = materialsArray.Count;

            return result;
        }

        private static JArray BuildTextureArray(FLVER2.Material mat, string textureFileExtension)
        {
            var texturesArray = new JArray();

            foreach (var binding in FormalMaterialTextureResolver.Resolve(mat, textureFileExtension))
            {
                var texObj = new JObject();
                texObj["type"] = binding.TextureType ?? string.Empty;
                texObj["gamePath"] = binding.GamePath ?? string.Empty;
                texObj["effectiveGamePath"] = binding.EffectiveGamePath ?? string.Empty;
                texObj["exportedFile"] = binding.ExportedFileName ?? string.Empty;
                texObj["exportedFileName"] = binding.ExportedFileName ?? string.Empty;
                texObj["relativePath"] = binding.RelativePath ?? string.Empty;
                texObj["slotType"] = binding.SlotType ?? string.Empty;
                texObj["slotIndex"] = binding.SlotIndex;
                texObj["parameterName"] = binding.ParameterName ?? string.Empty;
                texObj["colorSpace"] = binding.ColorSpace ?? string.Empty;

                texObj["scale"] = new JObject
                {
                    ["x"] = binding.Scale.X,
                    ["y"] = binding.Scale.Y
                };

                texturesArray.Add(texObj);
            }

            return texturesArray;
        }

        private static JObject BuildTextureBindings(FLVER2.Material mat, string textureFileExtension)
        {
            var bindings = new JObject();
            foreach (JObject textureEntry in BuildTextureArray(mat, textureFileExtension).OfType<JObject>())
            {
                string parameterName = (string)textureEntry["parameterName"];
                string slotType = (string)textureEntry["slotType"];
                string textureType = (string)textureEntry["type"] ?? "";
                
                // For unknown types, use the original texture type as parameter name
                if (string.IsNullOrWhiteSpace(parameterName) && !string.IsNullOrWhiteSpace(textureType))
                {
                    parameterName = textureType;
                }
                
                if (string.IsNullOrWhiteSpace(parameterName) || bindings.ContainsKey(parameterName))
                    continue;

                bindings[parameterName] = new JObject
                {
                    ["slotType"] = slotType ?? string.Empty,
                    ["exportedFileName"] = (string)textureEntry["exportedFileName"] ?? string.Empty,
                    ["relativePath"] = (string)textureEntry["relativePath"] ?? string.Empty,
                    ["colorSpace"] = (string)textureEntry["colorSpace"] ?? string.Empty,
                    ["textureType"] = textureType,
                };
            }

            return bindings;
        }

        private static JObject BuildScalarParameters(FLVER2.Material mat, string textureFileExtension)
        {
            var scalars = new JObject();

            foreach (JObject textureEntry in BuildTextureArray(mat, textureFileExtension).OfType<JObject>())
            {
                string parameterName = (string)textureEntry["parameterName"];
                if (string.IsNullOrWhiteSpace(parameterName))
                    continue;

                JObject scaleObject = textureEntry["scale"] as JObject;
                if (scaleObject == null)
                    continue;

                float scaleX = scaleObject.Value<float?>("x") ?? 1.0f;
                float scaleY = scaleObject.Value<float?>("y") ?? 1.0f;

                scalars[$"{parameterName}_UScale"] = scaleX;
                scalars[$"{parameterName}_VScale"] = scaleY;
            }

            return scalars;
        }

        private static string BuildMaterialInstanceKey(string materialName, int index)
        {
            string sanitized = (materialName ?? string.Empty)
                .Replace(' ', '_')
                .Replace('|', '_')
                .Replace('/', '_')
                .Replace('\\', '_');

            if (string.IsNullOrWhiteSpace(sanitized))
                sanitized = $"Material_{index:D3}";

            return $"MI_{sanitized}";
        }

        private static string BuildManifestSlotKey(string materialName)
        {
            return (materialName ?? string.Empty).Trim();
        }

        public static string GetParameterName(TextureSlotType slotType)
        {
            return slotType switch
            {
                TextureSlotType.BaseColor => "BaseColor",
                TextureSlotType.Normal => "Normal",
                TextureSlotType.Specular => "Specular",
                TextureSlotType.Emissive => "Emissive",
                TextureSlotType.Roughness => "Roughness",
                TextureSlotType.BlendMask => "BlendMask",
                _ => string.Empty,
            };
        }

        /// <summary>
        /// Classify a texture type string into a TextureSlotType.
        /// Mirrors the logic in FlverMaterial.cs:617-706.
        /// </summary>
        public static TextureSlotType ClassifyTextureType(string typeString,
            ref int indexAlbedo, ref int indexSpecular, ref int indexNormal,
            ref int indexEmissive, ref int indexShininess)
        {
            if (string.IsNullOrEmpty(typeString))
                return TextureSlotType.Unknown;

            var upper = typeString.ToUpper();

            if ((upper.Contains("DIFFUSE") || upper.Contains("ALBEDO")) && indexAlbedo < 2)
            {
                indexAlbedo++;
                return TextureSlotType.BaseColor;
            }

            if ((upper.Contains("SPECULAR") || upper.Contains("REFLECTANCE") || upper.Contains("METALLIC"))
                && indexSpecular < 2)
            {
                indexSpecular++;
                return TextureSlotType.Specular;
            }

            if (((upper.Contains("BUMPMAP") && !upper.Contains("DETAILBUMP")) ||
                 upper.Contains("NORMALMAP"))
                && indexNormal < 2)
            {
                indexNormal++;
                return TextureSlotType.Normal;
            }

            if (upper.Contains("EMISSIVE") && indexEmissive < 2)
            {
                indexEmissive++;
                return TextureSlotType.Emissive;
            }

            if (upper.Contains("SHININESS") && indexShininess < 2)
            {
                indexShininess++;
                return TextureSlotType.Roughness;
            }

            if (upper.Contains("BLENDMASK") || upper.Contains("BLEND"))
            {
                return TextureSlotType.BlendMask;
            }

            return TextureSlotType.Unknown;
        }

        /// <summary>
        /// Export manifest to a JSON file.
        /// </summary>
        public void ExportToFile(FLVER2 flver, string outputPath, string textureFileExtension = ".png")
        {
            ExportToFile(new[] { flver }, outputPath, textureFileExtension);
        }

        /// <summary>
        /// Export manifest for the final assembled FLVER set to a JSON file.
        /// </summary>
        public void ExportToFile(IReadOnlyList<FLVER2> flvers, string outputPath, string textureFileExtension = ".png")
        {
            var manifest = GenerateManifest(flvers, textureFileExtension);

            var dir = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(outputPath, manifest.ToString(Formatting.Indented));
        }
    }
}

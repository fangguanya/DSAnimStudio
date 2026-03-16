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
            var result = new JObject();
            result["version"] = "1.0";
            result["textureFormat"] = textureFileExtension.TrimStart('.');

            var materialsArray = new JArray();

            for (int matIdx = 0; matIdx < flver.Materials.Count; matIdx++)
            {
                var mat = flver.Materials[matIdx];
                var matObj = new JObject();
                matObj["index"] = matIdx;
                matObj["name"] = mat.Name ?? "";
                matObj["mtd"] = mat.MTD ?? "";

                var texturesArray = new JArray();

                // Track indices for multi-texture slots (Albedo1/2, Normal1/2, etc.)
                int indexAlbedo = 0, indexSpecular = 0, indexNormal = 0;
                int indexEmissive = 0, indexShininess = 0;

                foreach (var tex in mat.Textures)
                {
                    if (string.IsNullOrEmpty(tex.Path)) continue;

                    var texObj = new JObject();
                    texObj["type"] = tex.Type ?? "";
                    texObj["gamePath"] = tex.Path;

                    // Extract file name from game path
                    string texFileName = FlverToFbxExporter.GetTextureFileName(tex.Path);
                    texObj["exportedFile"] = texFileName + textureFileExtension;

                    // Classify texture type
                    var slotType = ClassifyTextureType(tex.Type,
                        ref indexAlbedo, ref indexSpecular, ref indexNormal,
                        ref indexEmissive, ref indexShininess);
                    texObj["slotType"] = slotType.ToString();

                    texObj["scale"] = new JObject
                    {
                        ["x"] = tex.Scale.X,
                        ["y"] = tex.Scale.Y
                    };

                    texturesArray.Add(texObj);
                }

                matObj["textures"] = texturesArray;

                // Find which meshes use this material
                var meshIndices = new JArray();
                for (int meshIdx = 0; meshIdx < flver.Meshes.Count; meshIdx++)
                {
                    if (flver.Meshes[meshIdx].MaterialIndex == matIdx)
                        meshIndices.Add(meshIdx);
                }
                matObj["meshIndices"] = meshIndices;

                materialsArray.Add(matObj);
            }

            result["materials"] = materialsArray;
            result["materialCount"] = flver.Materials.Count;

            return result;
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
            var manifest = GenerateManifest(flver, textureFileExtension);

            var dir = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(outputPath, manifest.ToString(Formatting.Indented));
        }
    }
}

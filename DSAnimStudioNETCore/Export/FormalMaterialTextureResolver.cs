using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using SoulsAssetPipeline;
using SoulsFormats;

namespace DSAnimStudio.Export
{
    public static class FormalMaterialTextureResolver
    {
        public sealed class ResolvedTextureBinding
        {
            public string TextureType { get; init; }
            public string GamePath { get; init; }
            public string EffectiveGamePath { get; init; }
            public string TextureStem { get; init; }
            public string ExportedFileName { get; init; }
            public string RelativePath { get; init; }
            public string ModelRelativePath { get; init; }
            public string SlotType { get; init; }
            public int SlotIndex { get; init; }
            public string ParameterName { get; init; }
            public string ColorSpace { get; init; }
            public Vector2 Scale { get; init; }
        }

        public static IReadOnlyList<ResolvedTextureBinding> Resolve(FLVER2.Material material, string textureFileExtension = ".png")
        {
            if (material == null)
                return Array.Empty<ResolvedTextureBinding>();

            int indexAlbedo = 0;
            int indexSpecular = 0;
            int indexNormal = 0;
            int indexEmissive = 0;
            int indexShininess = 0;
            int indexBlendMask = 0;
            int indexBlendMask3 = 0;
            var result = new List<ResolvedTextureBinding>();

            foreach (var texture in material.Textures ?? new List<FLVER2.Texture>())
            {
                string textureType = texture?.Type ?? string.Empty;
                string parameterCheck = textureType.ToUpperInvariant();
                string exportedTextureStem = NormalizeTextureStem(texture?.Path);
                string effectiveTextureStem = exportedTextureStem;
                Vector2 scale = new Vector2(
                    texture != null ? texture.Scale.X : 1.0f,
                    texture != null ? texture.Scale.Y : 1.0f);

                if (UsesMaterialBinderResolution()
                    && !string.IsNullOrWhiteSpace(material.MTD)
                    && !string.IsNullOrWhiteSpace(textureType))
                {
                    var sampler = FlverMaterialDefInfo.LookupSampler(material.MTD, textureType);
                    if (sampler != null)
                    {
                        if (!string.IsNullOrWhiteSpace(sampler.TexPath))
                            effectiveTextureStem = NormalizeTextureStem(sampler.TexPath) ?? effectiveTextureStem;

                        scale *= new Vector2(sampler.UVScale.X, sampler.UVScale.Y);

                        if (string.IsNullOrWhiteSpace(exportedTextureStem))
                            exportedTextureStem = effectiveTextureStem;
                    }
                }

                if (string.IsNullOrWhiteSpace(exportedTextureStem))
                    continue;

                string slotType = string.Empty;
                int slotIndex = 1;

                if ((parameterCheck.Contains("DIFFUSE") || parameterCheck.Contains("ALBEDO")) && indexAlbedo < 2)
                {
                    slotType = "BaseColor";
                    slotIndex = ++indexAlbedo;
                }
                else if ((parameterCheck.Contains("SPECULAR") || parameterCheck.Contains("REFLECTANCE") || parameterCheck.Contains("METALLIC")) && indexSpecular < 2)
                {
                    slotType = "Specular";
                    slotIndex = ++indexSpecular;
                }
                else if (((parameterCheck.Contains("BUMPMAP") && !parameterCheck.Contains("DETAILBUMP")) || parameterCheck.Contains("NORMALMAP")) && indexNormal < 2)
                {
                    slotType = "Normal";
                    slotIndex = ++indexNormal;
                }
                else if (parameterCheck.Contains("EMISSIVE") && indexEmissive < 2)
                {
                    slotType = "Emissive";
                    slotIndex = ++indexEmissive;
                }
                else if (parameterCheck.Contains("SHININESS") && indexShininess < 2)
                {
                    slotType = "Roughness";
                    slotIndex = ++indexShininess;
                }
                else if ((parameterCheck.Contains("BLENDMASK") || parameterCheck.Contains("MASK1")) && indexBlendMask < 1)
                {
                    slotType = "BlendMask";
                    slotIndex = ++indexBlendMask;
                }
                else if (parameterCheck.Contains("MASK3") && indexBlendMask3 < 1)
                {
                    slotType = "BlendMask3";
                    slotIndex = ++indexBlendMask3;
                }
                else
                {
                    slotType = "Unknown";
                    slotIndex = result.Count(binding => string.Equals(binding.SlotType, slotType, StringComparison.OrdinalIgnoreCase)) + 1;
                }

                string parameterName = BuildParameterName(slotType, slotIndex, textureType);
                string exportedFileName = FormalTextureContract.SanitizeTextureAssetName(exportedTextureStem) + textureFileExtension;

                result.Add(new ResolvedTextureBinding
                {
                    TextureType = textureType,
                    GamePath = texture?.Path ?? string.Empty,
                    EffectiveGamePath = effectiveTextureStem ?? string.Empty,
                    TextureStem = exportedTextureStem,
                    ExportedFileName = exportedFileName,
                    RelativePath = FormalTextureContract.BuildRelativeTexturePath(exportedFileName),
                    ModelRelativePath = FormalTextureContract.BuildRelativeModelTexturePath(exportedFileName),
                    SlotType = slotType,
                    SlotIndex = slotIndex,
                    ParameterName = parameterName,
                    ColorSpace = FormalTextureContract.GetColorSpace(slotType),
                    Scale = scale,
                });
            }

            return result;
        }

        private static bool UsesMaterialBinderResolution()
        {
            var currentDocument = zzz_DocumentManager.CurrentDocument;
            if (currentDocument?.GameRoot == null)
                return false;

            return currentDocument.GameRoot.GameType is SoulsGames.SDT or SoulsGames.ER or SoulsGames.ERNR or SoulsGames.AC6;
        }

        private static string NormalizeTextureStem(string texturePath)
        {
            string shortName = Utils.GetShortIngameFileName(texturePath);
            return string.IsNullOrWhiteSpace(shortName) ? null : shortName.ToLowerInvariant();
        }

        private static string BuildParameterName(string slotType, int slotIndex, string textureType)
        {
            if (string.IsNullOrWhiteSpace(slotType) || string.Equals(slotType, "Unknown", StringComparison.OrdinalIgnoreCase))
            {
                string sanitized = new string((textureType ?? string.Empty)
                    .Select(ch => char.IsLetterOrDigit(ch) ? ch : '_')
                    .ToArray())
                    .Trim('_');
                if (string.IsNullOrWhiteSpace(sanitized))
                    sanitized = "Texture";
                return $"Tex_{sanitized}";
            }

            return slotIndex <= 1 ? slotType : $"{slotType}{slotIndex}";
        }
    }
}
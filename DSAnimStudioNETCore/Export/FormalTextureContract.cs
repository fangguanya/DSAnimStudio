using System;
using System.Collections.Generic;
using System.IO;

namespace DSAnimStudio.Export
{
    public static class FormalTextureContract
    {
        public sealed class SupportedFormat
        {
            public string SourceFormat { get; init; }
            public string Decoder { get; init; }
            public string OutputFormat { get; init; }
            public bool IsFormal { get; init; }
        }

        private static readonly Dictionary<string, SupportedFormat> SupportedFormatsInternal
            = new Dictionary<string, SupportedFormat>(StringComparer.OrdinalIgnoreCase)
            {
                ["DXT1"] = new SupportedFormat { SourceFormat = "DXT1", Decoder = "Pfim", OutputFormat = "png", IsFormal = true },
                ["DXT3"] = new SupportedFormat { SourceFormat = "DXT3", Decoder = "Pfim", OutputFormat = "png", IsFormal = true },
                ["DXT5"] = new SupportedFormat { SourceFormat = "DXT5", Decoder = "Pfim", OutputFormat = "png", IsFormal = true },
                ["BC4"] = new SupportedFormat { SourceFormat = "BC4", Decoder = "Pfim", OutputFormat = "png", IsFormal = true },
                ["BC5"] = new SupportedFormat { SourceFormat = "BC5", Decoder = "Pfim", OutputFormat = "png", IsFormal = true },
                ["BC7"] = new SupportedFormat { SourceFormat = "BC7", Decoder = "Pfim", OutputFormat = "png", IsFormal = true },
            };

        public static IReadOnlyDictionary<string, SupportedFormat> SupportedFormats => SupportedFormatsInternal;

        public static bool IsFormalSourceFormat(string sourceFormat)
        {
            string normalized = NormalizeSourceFormat(sourceFormat);
            return normalized != null && SupportedFormatsInternal.TryGetValue(normalized, out var supported) && supported.IsFormal;
        }

        public static string NormalizeSourceFormat(string sourceFormat)
        {
            if (string.IsNullOrWhiteSpace(sourceFormat))
                return null;

            string normalized = sourceFormat.Trim().ToUpperInvariant();
            if (normalized.StartsWith("Pfim.ImageFormat.", StringComparison.OrdinalIgnoreCase))
                normalized = normalized.Substring("Pfim.ImageFormat.".Length);

            return normalized;
        }

        public static string BuildFormalTextureFileName(string textureName)
        {
            if (string.IsNullOrWhiteSpace(textureName))
                throw new ArgumentException("Formal texture naming requires a non-empty texture name.", nameof(textureName));

            return $"{SanitizeTextureAssetName(textureName)}.png";
        }

        public static string SanitizeTextureAssetName(string textureName)
        {
            if (string.IsNullOrWhiteSpace(textureName))
                throw new ArgumentException("Formal texture naming requires a non-empty texture name.", nameof(textureName));

            string trimmed = textureName.Trim();
            char[] sanitizedChars = new char[trimmed.Length];
            int index = 0;
            foreach (char ch in trimmed)
            {
                sanitizedChars[index++] = char.IsLetterOrDigit(ch) || ch == '_' ? ch : '_';
            }

            string sanitized = new string(sanitizedChars, 0, index).Trim('_');
            while (sanitized.Contains("__", StringComparison.Ordinal))
                sanitized = sanitized.Replace("__", "_", StringComparison.Ordinal);

            if (string.IsNullOrWhiteSpace(sanitized))
                throw new ArgumentException("Formal texture naming produced an empty sanitized name.", nameof(textureName));

            return sanitized;
        }

        public static string BuildRelativeTexturePath(string textureFileName)
        {
            if (string.IsNullOrWhiteSpace(textureFileName))
                throw new ArgumentException("Formal texture path requires a non-empty file name.", nameof(textureFileName));

            return Path.Combine("Textures", textureFileName).Replace('\\', '/');
        }

        public static string BuildRelativeModelTexturePath(string textureFileName)
        {
            if (string.IsNullOrWhiteSpace(textureFileName))
                throw new ArgumentException("Formal model texture path requires a non-empty file name.", nameof(textureFileName));

            return Path.Combine("..", "Textures", textureFileName).Replace('\\', '/');
        }

        public static string GetColorSpace(string slotType)
        {
            return slotType switch
            {
                "BaseColor" => "sRGB",
                "BaseColor2" => "sRGB",
                "Emissive" => "sRGB",
                "Emissive2" => "sRGB",
                _ => "Linear",
            };
        }
    }
}
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using Pfim;
using SoulsFormats;
using PfimImageFormat = Pfim.ImageFormat;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports TPF textures to PNG format for the formal export pipeline.
    /// Uses Pfim for BC compressed texture decoding (DXT1/DXT3/DXT5/BC4/BC5/BC7).
    /// </summary>
    public class TextureExporter
    {
        public sealed class TextureExportRecord
        {
            public string TextureName { get; init; }
            public string SourceContainer { get; init; }
            public string SourceFormat { get; init; }
            public string DecodedPixelFormat { get; init; }
            public string OutputFileName { get; init; }
            public string FailureCode { get; init; }
            public string FailureMessage { get; init; }

            public bool Success => string.IsNullOrWhiteSpace(FailureCode);
        }

        public enum ExportFormat
        {
            DDS,
            PNG
        }

        public class ExportOptions
        {
            public ExportFormat Format { get; set; } = ExportFormat.PNG;

            /// <summary>If true, skip unsupported formats instead of throwing</summary>
            public bool SkipUnsupported { get; set; } = false;
        }

        private static bool IsRgb24CompatibleFormat(PfimImageFormat format)
        {
            return format == PfimImageFormat.Rgb24
                || string.Equals(format.ToString(), "Rgb8", StringComparison.OrdinalIgnoreCase);
        }

        private readonly ExportOptions _options;
        private readonly List<string> _warnings = new List<string>();

        public IReadOnlyList<string> Warnings => _warnings;

        public TextureExporter(ExportOptions options = null)
        {
            _options = options ?? new ExportOptions();
        }

        /// <summary>
        /// Export all textures from a TPF archive.
        /// </summary>
        public void ExportTpf(TPF tpf, string outputDir,
            Action<string, int, int> progressCallback = null,
            Action<TextureExportRecord> recordCallback = null)
        {
            if (!Directory.Exists(outputDir))
                Directory.CreateDirectory(outputDir);

            var usedNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            int total = tpf.Textures.Count;

            for (int i = 0; i < tpf.Textures.Count; i++)
            {
                var tex = tpf.Textures[i];
                progressCallback?.Invoke(tex.Name, i + 1, total);

                try
                {
                    string baseName = !string.IsNullOrEmpty(tex.Name) ? tex.Name : $"texture_{i}";

                    // Deduplicate names
                    string uniqueName = baseName;
                    int suffix = 1;
                    while (usedNames.Contains(uniqueName))
                    {
                        uniqueName = $"{baseName}_{suffix}";
                        suffix++;
                    }
                    usedNames.Add(uniqueName);

                    string sourceFormat = TryGetTpfTextureFormat(tex);
                    string outputFileName;
                    string decodedPixelFormat = string.Empty;

                    if (_options.Format == ExportFormat.DDS)
                    {
                        outputFileName = $"{uniqueName}.dds";
                        ExportAsDds(tex.Bytes, uniqueName, outputDir);
                    }
                    else
                    {
                        outputFileName = $"{uniqueName}.png";
                        decodedPixelFormat = ExportAsPng(tex.Bytes, uniqueName, outputDir);
                    }

                    recordCallback?.Invoke(new TextureExportRecord
                    {
                        TextureName = baseName,
                        SourceContainer = "DDS",
                        SourceFormat = sourceFormat,
                        DecodedPixelFormat = decodedPixelFormat,
                        OutputFileName = outputFileName,
                    });
                }
                catch (Exception ex)
                {
                    string msg = $"Warning: Failed to export texture '{tex.Name}': {ex.Message}";
                    _warnings.Add(msg);
                    System.Diagnostics.Debug.WriteLine(msg);
                    recordCallback?.Invoke(new TextureExportRecord
                    {
                        TextureName = tex.Name ?? $"texture_{i}",
                        SourceContainer = "DDS",
                        SourceFormat = TryGetTpfTextureFormat(tex),
                        OutputFileName = string.Empty,
                        FailureCode = "TEXTURE_EXPORT_FAILED",
                        FailureMessage = ex.Message,
                    });

                    if (!_options.SkipUnsupported)
                        throw new InvalidOperationException(msg, ex);
                }
            }
        }

        /// <summary>
        /// Export all textures from a .chrbnd.dcx file (BND4 archive containing TPF).
        /// </summary>
        public void ExportFromChrbnd(string chrBndPath, string outputDir,
            Action<string, int, int> progressCallback = null)
        {
            byte[] data = File.ReadAllBytes(chrBndPath);

            // Try to decompress DCX if needed
            if (DCX.Is(data))
                data = DCX.Decompress(data);

            if (BND4.Is(data))
            {
                var bnd = BND4.Read(data);
                foreach (var file in bnd.Files)
                {
                    if (file.Name != null && file.Name.EndsWith(".tpf", StringComparison.OrdinalIgnoreCase))
                    {
                        try
                        {
                            byte[] tpfBytes = file.Bytes;
                            if (DCX.Is(tpfBytes))
                                tpfBytes = DCX.Decompress(tpfBytes);

                            var tpf = TPF.Read(tpfBytes);
                            ExportTpf(tpf, outputDir, progressCallback);
                        }
                        catch (Exception ex)
                        {
                            _warnings.Add($"Warning: Failed to read TPF from '{file.Name}': {ex.Message}");
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Export textures from a standalone .tpf.dcx file.
        /// </summary>
        public void ExportFromTpfDcx(string tpfDcxPath, string outputDir,
            Action<string, int, int> progressCallback = null)
        {
            byte[] data = File.ReadAllBytes(tpfDcxPath);

            if (DCX.Is(data))
                data = DCX.Decompress(data);

            var tpf = TPF.Read(data);
            ExportTpf(tpf, outputDir, progressCallback);
        }

        /// <summary>
        /// Export raw DDS bytes directly to a .dds file.
        /// </summary>
        private void ExportAsDds(byte[] ddsBytes, string name, string outputDir)
        {
            string outPath = Path.Combine(outputDir, $"{name}.dds");
            File.WriteAllBytes(outPath, ddsBytes);
        }

        /// <summary>
        /// Decode DDS texture using Pfim and export as PNG.
        /// </summary>
        private string ExportAsPng(byte[] ddsBytes, string name, string outputDir)
        {
            using (var ms = new MemoryStream(ddsBytes))
            {
                IImage image;
                try
                {
                    image = Pfimage.FromStream(ms);
                }
                catch (Exception ex)
                {
                    string msg = $"Unsupported DDS format for '{name}': {ex.Message}";
                    _warnings.Add(msg);
                    if (_options.SkipUnsupported)
                            return string.Empty;
                    throw;
                }

                // Determine pixel format
                PixelFormat pixelFormat;
                byte[] pixelData = image.Data;

                switch (image.Format)
                {
                    case PfimImageFormat.Rgba32:
                        pixelFormat = PixelFormat.Format32bppArgb;
                        // Pfim returns RGBA, System.Drawing needs BGRA
                        SwapRedBlue32(pixelData, image.Width, image.Height, image.Stride);
                        break;
                    case var _ when IsRgb24CompatibleFormat(image.Format):
                        pixelFormat = PixelFormat.Format24bppRgb;
                        SwapRedBlue24(pixelData, image.Width, image.Height, image.Stride);
                        break;
                    case PfimImageFormat.R5g5b5a1:
                        pixelFormat = PixelFormat.Format16bppArgb1555;
                        break;
                    case PfimImageFormat.R5g5b5:
                        pixelFormat = PixelFormat.Format16bppRgb555;
                        break;
                    case PfimImageFormat.R5g6b5:
                        pixelFormat = PixelFormat.Format16bppRgb565;
                        break;
                    default:
                        _warnings.Add($"Unsupported pixel format '{image.Format}' for '{name}'.");
                        if (_options.SkipUnsupported)
                            return image.Format.ToString();
                        throw new NotSupportedException($"Unsupported pixel format '{image.Format}' for '{name}'.");
                }

                using var bmp = CreateBitmap(image.Width, image.Height, pixelFormat, pixelData, image.Stride);
                string outPath = Path.Combine(outputDir, $"{name}.png");
                bmp.Save(outPath, System.Drawing.Imaging.ImageFormat.Png);
                return image.Format.ToString();
            }
        }

        private static string TryGetTpfTextureFormat(TPF.Texture texture)
        {
            if (texture == null)
                return string.Empty;

            var formatProperty = texture.GetType().GetProperty("Format");
            return formatProperty?.GetValue(texture)?.ToString() ?? string.Empty;
        }

        private static Bitmap CreateBitmap(int width, int height, PixelFormat pixelFormat, byte[] pixelData, int sourceStride)
        {
            var bitmap = new Bitmap(width, height, pixelFormat);
            var rect = new Rectangle(0, 0, width, height);
            BitmapData bitmapData = null;

            try
            {
                bitmapData = bitmap.LockBits(rect, ImageLockMode.WriteOnly, pixelFormat);
                int srcStride = Math.Abs(sourceStride);
                int dstStride = Math.Abs(bitmapData.Stride);
                int rowCopyLength = Math.Min(srcStride, dstStride);

                for (int y = 0; y < height; y++)
                {
                    IntPtr destination = IntPtr.Add(bitmapData.Scan0, y * bitmapData.Stride);
                    Marshal.Copy(pixelData, y * srcStride, destination, rowCopyLength);
                }

                return bitmap;
            }
            catch
            {
                bitmap.Dispose();
                throw;
            }
            finally
            {
                if (bitmapData != null)
                    bitmap.UnlockBits(bitmapData);
            }
        }

        /// <summary>
        /// Swap R and B channels in 32bpp RGBA pixel data (Pfim RGBA → System.Drawing BGRA).
        /// </summary>
        private static void SwapRedBlue32(byte[] data, int width, int height, int stride)
        {
            for (int y = 0; y < height; y++)
            {
                int rowStart = y * stride;
                for (int x = 0; x < width; x++)
                {
                    int offset = rowStart + x * 4;
                    if (offset + 2 >= data.Length) break;
                    byte r = data[offset];
                    data[offset] = data[offset + 2];
                    data[offset + 2] = r;
                }
            }
        }

        /// <summary>
        /// Swap R and B channels in 24bpp RGB pixel data.
        /// </summary>
        private static void SwapRedBlue24(byte[] data, int width, int height, int stride)
        {
            for (int y = 0; y < height; y++)
            {
                int rowStart = y * stride;
                for (int x = 0; x < width; x++)
                {
                    int offset = rowStart + x * 3;
                    if (offset + 2 >= data.Length) break;
                    byte r = data[offset];
                    data[offset] = data[offset + 2];
                    data[offset + 2] = r;
                }
            }
        }
    }
}

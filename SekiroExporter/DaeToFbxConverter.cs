using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Assimp;

namespace SekiroExporter
{
    /// <summary>
    /// Converts DAE (Collada) files to FBX format using AssimpNet.
    /// Falls back to fbx2013 (older format) if standard FBX fails.
    /// </summary>
    public static class DaeToFbxConverter
    {
        private static readonly string[] FbxFormats = { "fbx", "fbxa" };

        public static int ConvertDirectory(string rootDir, bool deleteOriginals = false)
        {
            if (!Directory.Exists(rootDir))
            {
                Console.Error.WriteLine($"Directory not found: {rootDir}");
                return 0;
            }

            var daeFiles = Directory.GetFiles(rootDir, "*.dae", SearchOption.AllDirectories);
            Console.WriteLine($"Found {daeFiles.Length} DAE files to convert");

            int converted = 0;
            int failed = 0;
            int skipped = 0;

            using var ctx = new AssimpContext();

            foreach (var daeFile in daeFiles)
            {
                string fbxFile = Path.ChangeExtension(daeFile, ".fbx");

                // Skip if FBX already exists
                if (File.Exists(fbxFile))
                {
                    skipped++;
                    continue;
                }

                try
                {
                    var scene = ctx.ImportFile(daeFile,
                        PostProcessSteps.None); // Keep original data, no post-processing

                    if (scene == null)
                    {
                        if (failed < 5)
                            Console.Error.WriteLine($"  Failed to load: {Path.GetFileName(daeFile)}");
                        failed++;
                        continue;
                    }

                    bool success = false;
                    foreach (var format in FbxFormats)
                    {
                        try
                        {
                            success = ctx.ExportFile(scene, fbxFile, format);
                            if (success) break;
                        }
                        catch { }
                    }

                    if (success)
                    {
                        converted++;
                        if (deleteOriginals)
                            File.Delete(daeFile);
                    }
                    else
                    {
                        if (failed < 5)
                            Console.Error.WriteLine($"  FBX export failed: {Path.GetFileName(daeFile)}");
                        failed++;
                    }
                }
                catch (Exception ex)
                {
                    if (failed < 5)
                        Console.Error.WriteLine($"  Error: {Path.GetFileName(daeFile)}: {ex.Message}");
                    failed++;
                }
            }

            Console.WriteLine($"Conversion: {converted} converted, {failed} failed, {skipped} skipped (FBX exists)");
            return converted;
        }
    }
}

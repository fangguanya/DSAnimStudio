using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using DSAnimStudio.Export;
using SoulsFormats;
using SoulsAssetPipeline.Animation;

namespace SekiroExporter
{
    /// <summary>
    /// CLI batch export tool for Sekiro game assets.
    /// Exports models (DAE/FBX), animations (FBX), textures (PNG/DDS),
    /// skill configs (JSON), and material manifests (JSON).
    ///
    /// Sekiro file structure per character:
    ///   chr/{chrId}.chrbnd.dcx  - Model (FLVER) + Skeleton (HKX)
    ///   chr/{chrId}.anibnd.dcx  - Animations (HKX) + TAE events
    ///   chr/{chrId}.texbnd.dcx  - Textures (TPF)
    ///   chr/{chrId}.behbnd.dcx  - Behavior (not exported)
    /// </summary>
    class Program
    {
        static int Main(string[] args)
        {
            Console.WriteLine("=== Sekiro Asset Exporter v1.1 ===");
            Console.WriteLine();

            if (args.Length == 0)
            {
                PrintUsage();
                return 1;
            }

            string command = args[0].ToLower();
            var parsedArgs = ParseArgs(args.Skip(1).ToArray());

            try
            {
                switch (command)
                {
                    case "export-all":
                        return RunExportAll(parsedArgs);
                    case "export-model":
                        return RunExportModel(parsedArgs);
                    case "export-anims":
                        return RunExportAnims(parsedArgs);
                    case "export-textures":
                        return RunExportTextures(parsedArgs);
                    case "export-skills":
                        return RunExportSkills(parsedArgs);
                    case "convert-to-fbx":
                        return RunConvertToFbx(parsedArgs);
                    default:
                        Console.Error.WriteLine($"Unknown command: {command}");
                        PrintUsage();
                        return 1;
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Fatal error: {ex.Message}");
                Console.Error.WriteLine(ex.StackTrace);
                return 1;
            }
        }

        static void PrintUsage()
        {
            Console.WriteLine("Usage: SekiroExporter <command> [options]");
            Console.WriteLine();
            Console.WriteLine("Commands:");
            Console.WriteLine("  export-all       Export all assets (model, anims, textures, skills)");
            Console.WriteLine("  export-model     Export FLVER model to DAE/FBX");
            Console.WriteLine("  export-anims     Export animations to FBX");
            Console.WriteLine("  export-textures  Export textures to PNG/DDS");
            Console.WriteLine("  export-skills    Export TAE skill config to JSON");
            Console.WriteLine();
            Console.WriteLine("Options:");
            Console.WriteLine("  --game-dir <path>   Sekiro installation directory");
            Console.WriteLine("  --chr <id>          Character ID (e.g., c0000). If omitted, exports all.");
            Console.WriteLine("  --output <path>     Output directory");
            Console.WriteLine("  --format <png|dds>  Texture format (default: png)");
        }

        static Dictionary<string, string> ParseArgs(string[] args)
        {
            var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            for (int i = 0; i < args.Length - 1; i += 2)
            {
                if (args[i].StartsWith("--"))
                    dict[args[i].Substring(2)] = args[i + 1];
            }
            return dict;
        }

        static int RunExportAll(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrFilter = args.GetValueOrDefault("chr");

            var chrFiles = ScanCharacters(gameDir, chrFilter);
            Console.WriteLine($"Found {chrFiles.Count} character(s) to export.");
            Console.WriteLine();

            int completed = 0;
            int errors = 0;
            var sw = Stopwatch.StartNew();

            // Process characters sequentially for cleaner output
            // (Parallel caused interleaved console output)
            foreach (var chrPath in chrFiles)
            {
                string chrId = Path.GetFileNameWithoutExtension(chrPath).Split('.')[0];
                string chrOutputDir = Path.Combine(output, chrId);

                try
                {
                    int num = System.Threading.Interlocked.Increment(ref completed);
                    Console.WriteLine($"━━━ [{num}/{chrFiles.Count}] {chrId} ━━━");

                    ExportCharacter(gameDir, chrPath, chrId, chrOutputDir, args);
                    Console.WriteLine();
                }
                catch (Exception ex)
                {
                    System.Threading.Interlocked.Increment(ref errors);
                    Console.Error.WriteLine($"  ERROR exporting {chrId}: {ex.Message}");
                    Console.WriteLine();
                }
            }

            sw.Stop();
            Console.WriteLine($"═══════════════════════════════════════════");
            Console.WriteLine($"Export complete: {completed - errors} succeeded, {errors} failed in {sw.Elapsed.TotalSeconds:F1}s");
            return errors > 0 ? 1 : 0;
        }

        static void ExportCharacter(string gameDir, string chrBndPath, string chrId, string outputDir, Dictionary<string, string> args)
        {
            Directory.CreateDirectory(outputDir);
            string modelDir = Path.Combine(outputDir, "Model");
            string animDir = Path.Combine(outputDir, "Animations");
            string texDir = Path.Combine(outputDir, "Textures");
            string skillDir = Path.Combine(outputDir, "Skills");
            string chrDir = Path.GetDirectoryName(chrBndPath);

            // ═══════════════════════════════════════════
            // 1. Load CHRBND → FLVER model + HKX skeleton
            // ═══════════════════════════════════════════
            byte[] chrData = File.ReadAllBytes(chrBndPath);
            if (DCX.Is(chrData)) chrData = DCX.Decompress(chrData);
            var bnd = BND4.Read(chrData);

            FLVER2 flver = null;
            HKX skeletonHkx = null;
            List<byte[]> tpfDataList = new List<byte[]>();

            foreach (var file in bnd.Files)
            {
                string name = file.Name?.ToLower() ?? "";

                if (name.EndsWith(".flver") || name.EndsWith(".flver.dcx"))
                {
                    byte[] d = file.Bytes;
                    if (DCX.Is(d)) d = DCX.Decompress(d);
                    try { flver = FLVER2.Read(d); }
                    catch (Exception ex) { Console.Error.WriteLine($"  FLVER parse error: {ex.Message}"); }
                }
                else if (name.EndsWith(".hkx") && !name.Contains("_c.hkx") && !name.EndsWith(".hkxpwv"))
                {
                    // Sekiro skeleton HKX: named "{chrId}.HKX" (NOT "{chrId}_c.hkx" which is collision)
                    byte[] d = file.Bytes;
                    if (DCX.Is(d)) d = DCX.Decompress(d);
                    skeletonHkx = TryReadHkx(d, file.Name);
                }
                else if (name.EndsWith(".tpf") || name.EndsWith(".tpf.dcx"))
                {
                    byte[] d = file.Bytes;
                    if (DCX.Is(d)) d = DCX.Decompress(d);
                    tpfDataList.Add(d);
                }
            }

            Console.WriteLine($"  CHRBND: FLVER={flver != null} (nodes={flver?.Nodes?.Count}, meshes={flver?.Meshes?.Count}), Skeleton={skeletonHkx != null}, TPFs={tpfDataList.Count}");

            // ═══════════════════════════════════════════
            // 2. Load TEXBND → textures (separate file)
            // ═══════════════════════════════════════════
            string texbndPath = Path.Combine(chrDir, $"{chrId}.texbnd.dcx");
            if (File.Exists(texbndPath))
            {
                try
                {
                    byte[] texbndData = File.ReadAllBytes(texbndPath);
                    if (DCX.Is(texbndData)) texbndData = DCX.Decompress(texbndData);
                    var texBnd = BND4.Read(texbndData);

                    foreach (var file in texBnd.Files)
                    {
                        string name = file.Name?.ToLower() ?? "";
                        if (name.EndsWith(".tpf") || name.EndsWith(".tpf.dcx"))
                        {
                            byte[] d = file.Bytes;
                            if (DCX.Is(d)) d = DCX.Decompress(d);
                            tpfDataList.Add(d);
                        }
                    }
                    Console.WriteLine($"  TEXBND: found {texBnd.Files.Count} files, extracted {tpfDataList.Count} total TPFs");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  TEXBND error: {ex.Message}");
                }
            }

            // ═══════════════════════════════════════════
            // 3. Load ANIBND → TAE events + animations
            // ═══════════════════════════════════════════
            TAE tae = null;
            string anibndPath = Path.Combine(chrDir, $"{chrId}.anibnd.dcx");
            byte[] anibndRawBytes = null;
            byte[] compendiumBytes = null;

            if (File.Exists(anibndPath))
            {
                try
                {
                    byte[] anibndData = File.ReadAllBytes(anibndPath);
                    if (DCX.Is(anibndData)) anibndData = DCX.Decompress(anibndData);
                    anibndRawBytes = anibndData;
                    var aniBnd = BND4.Read(anibndData);

                    int hkxCount = 0;
                    foreach (var file in aniBnd.Files)
                    {
                        string name = file.Name?.ToLower() ?? "";

                        if (name.EndsWith(".tae") || name.EndsWith(".tae.dcx"))
                        {
                            byte[] d = file.Bytes;
                            if (DCX.Is(d)) d = DCX.Decompress(d);
                            try { tae = TAE.Read(d); }
                            catch (Exception ex) { Console.Error.WriteLine($"  TAE parse error: {ex.Message}"); }
                        }
                        else if (name.EndsWith(".hkx") || name.EndsWith(".hkx.dcx"))
                        {
                            hkxCount++;
                        }

                        // Compendium file (ID 7000000) for tagfile animation parsing
                        if (file.ID == 7000000)
                        {
                            compendiumBytes = file.Bytes;
                            if (DCX.Is(compendiumBytes)) compendiumBytes = DCX.Decompress(compendiumBytes);
                        }
                    }
                    Console.WriteLine($"  ANIBND: TAE={tae != null} (anims={tae?.Animations?.Count}), HKX anims={hkxCount}, Compendium={compendiumBytes != null}");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ANIBND error: {ex.Message}");
                }
            }

            // ═══════════════════════════════════════════
            // 4. Export model (DAE/Collada)
            // ═══════════════════════════════════════════
            if (flver != null && flver.Meshes.Count > 0)
            {
                Directory.CreateDirectory(modelDir);
                string modelPath = Path.Combine(modelDir, $"{chrId}.fbx");
                try
                {
                    var fbxExporter = new FlverToFbxExporter();
                    fbxExporter.Export(flver, modelPath);
                    // Check for the actual output file (format may have fallen back to .gltf/.glb/.dae)
                    string actualModelFile = FindExportedFile(modelDir, chrId, new[] { ".fbx", ".gltf", ".glb", ".dae" });
                    if (actualModelFile != null)
                        Console.WriteLine($"  ✓ Model: {Path.GetFileName(actualModelFile)} ({new FileInfo(actualModelFile).Length / 1024}KB)");
                    else
                        Console.Error.WriteLine($"  ✗ Model: export returned but file missing!");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ✗ Model export error: {ex.Message}");
                }

                // Export material manifest
                try
                {
                    var matExporter = new MaterialManifestExporter();
                    string matPath = Path.Combine(modelDir, "material_manifest.json");
                    matExporter.ExportToFile(flver, matPath);
                    Console.WriteLine($"  ✓ Material manifest exported");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ✗ Material manifest error: {ex.Message}");
                }
            }
            else if (flver != null)
            {
                Console.WriteLine($"  - Model: skipped (0 meshes, parts may be in separate files)");
            }

            // ═══════════════════════════════════════════
            // 5. Export textures
            // ═══════════════════════════════════════════
            if (tpfDataList.Count > 0)
            {
                Directory.CreateDirectory(texDir);
                var texExporter = new TextureExporter(new TextureExporter.ExportOptions
                {
                    Format = (args.GetValueOrDefault("format") ?? "png").ToLower() == "dds"
                        ? TextureExporter.ExportFormat.DDS
                        : TextureExporter.ExportFormat.PNG
                });

                int texCount = 0;
                int texErrors = 0;
                foreach (var tpfData in tpfDataList)
                {
                    try
                    {
                        var tpf = TPF.Read(tpfData);
                        texExporter.ExportTpf(tpf, texDir);
                        texCount += tpf.Textures.Count;
                    }
                    catch (Exception ex)
                    {
                        texErrors++;
                        Console.Error.WriteLine($"  Warning: TPF export error: {ex.Message}");
                    }
                }
                Console.WriteLine($"  ✓ Textures: {texCount} exported{(texErrors > 0 ? $", {texErrors} errors" : "")}");
            }

            // ═══════════════════════════════════════════
            // 6. Export animations from ANIBND
            // ═══════════════════════════════════════════
            if (anibndRawBytes != null && skeletonHkx != null)
            {
                Directory.CreateDirectory(animDir);
                var animExporter = new AnimationToFbxExporter();

                try
                {
                    animExporter.ExportAnibnd(anibndRawBytes, skeletonHkx, animDir,
                        (name, cur, total) =>
                        {
                            if (cur % 20 == 0 || cur == total)
                                Console.Write($"\r  Animations: {cur}/{total}...");
                        });
                    Console.WriteLine();

                    // Post-process: merge per-bone glTF animations into single clips
                    int mergedCount = GltfAnimationMerger.MergeAllInDirectory(animDir);
                    int animCount = Directory.GetFiles(animDir, "*.gltf").Length
                        + Directory.GetFiles(animDir, "*.fbx").Length
                        + Directory.GetFiles(animDir, "*.dae").Length;
                    Console.WriteLine($"  ✓ Animations: {animCount} clips exported ({mergedCount} glTF merged)");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ✗ Animation export error: {ex.Message}");
                }
            }
            else if (skeletonHkx == null && File.Exists(anibndPath))
            {
                Console.WriteLine($"  - Animations: skipped (skeleton not parsed)");
            }

            // ═══════════════════════════════════════════
            // 7. Export skill config (TAE events)
            // ═══════════════════════════════════════════
            if (tae != null)
            {
                Directory.CreateDirectory(skillDir);
                var skillExporter = new SkillConfigExporter();

                // Try to load template
                string templatePath = Path.Combine(
                    AppDomain.CurrentDomain.BaseDirectory, "Res", "TAE.Template.SDT.xml");
                if (File.Exists(templatePath))
                    skillExporter.LoadTemplate(templatePath);

                try
                {
                    skillExporter.ExportToFile(tae, Path.Combine(skillDir, "skill_config.json"), chrId);
                    Console.WriteLine($"  ✓ Skills: {tae.Animations.Count} animations, skill_config.json exported");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ✗ Skill config error: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// Try multiple HKX reading strategies for Sekiro's tagfile format.
        /// </summary>
        static HKX TryReadHkx(byte[] data, string fileName)
        {
            // Strategy 1: Sekiro uses Havok tagfile format
            try
            {
                var hkx = HKX.GenFakeFromTagFile(data);
                if (hkx != null)
                {
                    Console.WriteLine($"  HKX: parsed via GenFakeFromTagFile ({fileName})");
                    return hkx;
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"  HKX GenFakeFromTagFile failed: {ex.Message}");
            }

            // Strategy 2: Try as DS1R format (SDT fallback per zzz_GameRootIns.cs)
            try
            {
                var hkx = HKX.Read(data, HKX.HKXVariation.HKXDS1, isDS1RAnimHotfix: true);
                if (hkx != null)
                {
                    Console.WriteLine($"  HKX: parsed via HKX.Read HKXDS1 ({fileName})");
                    return hkx;
                }
            }
            catch { }

            // Strategy 3: Try DeS variation
            try
            {
                var hkx = HKX.Read(data, HKX.HKXVariation.HKXDeS, isDS1RAnimHotfix: true);
                if (hkx != null)
                {
                    Console.WriteLine($"  HKX: parsed via HKX.Read HKXDeS ({fileName})");
                    return hkx;
                }
            }
            catch { }

            // Strategy 4: Try DS3 variation
            try
            {
                var hkx = HKX.Read(data, HKX.HKXVariation.HKXDS3, isDS1RAnimHotfix: false);
                if (hkx != null)
                {
                    Console.WriteLine($"  HKX: parsed via HKX.Read HKXDS3 ({fileName})");
                    return hkx;
                }
            }
            catch { }

            Console.Error.WriteLine($"  HKX: ALL parsing strategies failed for {fileName} ({data.Length} bytes)");
            return null;
        }

        static int RunExportModel(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            ExportCharacter(gameDir, chrBndPath, chrId, output, args);
            return 0;
        }

        static int RunExportAnims(Dictionary<string, string> args)
        {
            return RunExportModel(args);
        }

        static int RunExportTextures(Dictionary<string, string> args)
        {
            return RunExportModel(args);
        }

        static int RunExportSkills(Dictionary<string, string> args)
        {
            return RunExportModel(args);
        }

        static List<string> ScanCharacters(string gameDir, string chrFilter)
        {
            string chrDir = Path.Combine(gameDir, "chr");
            if (!Directory.Exists(chrDir))
            {
                chrDir = gameDir;
            }

            var files = Directory.GetFiles(chrDir, "*.chrbnd.dcx", SearchOption.AllDirectories)
                .OrderBy(f => f)
                .ToList();

            if (!string.IsNullOrEmpty(chrFilter))
            {
                files = files.Where(f =>
                    Path.GetFileName(f).StartsWith(chrFilter, StringComparison.OrdinalIgnoreCase))
                    .ToList();
            }

            return files;
        }

        static string FindChrBnd(string gameDir, string chrId)
        {
            string chrDir = Path.Combine(gameDir, "chr");
            string path = Path.Combine(chrDir, $"{chrId}.chrbnd.dcx");
            if (!File.Exists(path))
                throw new FileNotFoundException($"Character file not found: {path}");
            return path;
        }

        static int RunConvertToFbx(Dictionary<string, string> args)
        {
            string dir = args.GetValueOrDefault("output") ?? args.GetValueOrDefault("dir") ?? "E:\\Sekiro\\Export";
            bool delete = args.ContainsKey("delete-originals");

            Console.WriteLine($"Converting DAE files to FBX in: {dir}");
            int converted = DaeToFbxConverter.ConvertDirectory(dir, delete);
            Console.WriteLine($"Done. {converted} files converted.");
            return 0;
        }

        /// <summary>Find the actual exported file, checking multiple possible extensions.</summary>
        static string FindExportedFile(string dir, string baseName, string[] extensions)
        {
            foreach (var ext in extensions)
            {
                string path = Path.Combine(dir, baseName + ext);
                if (File.Exists(path)) return path;
            }
            return null;
        }

        static string GetRequired(Dictionary<string, string> args, string key)
        {
            if (!args.TryGetValue(key, out string value) || string.IsNullOrEmpty(value))
                throw new ArgumentException($"Required argument --{key} not provided.");
            return value;
        }
    }
}

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using DSAnimStudio;
using DSAnimStudio.Export;
using DSAnimStudio.TaeEditor;
using Newtonsoft.Json.Linq;
using SoulsFormats;
using SoulsAssetPipeline.Animation;
using SoulsAssetPipeline;

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
        [Flags]
        private enum ExportSections
        {
            None = 0,
            Model = 1,
            Animations = 2,
            Textures = 4,
            Skills = 8,
            All = Model | Animations | Textures | Skills,
        }

        private sealed class FormalWeaponRow
        {
            public short EquipModelID { get; init; }
        }

        private sealed class FormalProtectorRow
        {
            public short EquipModelID { get; init; }
            public ParamData.EquipModelGenders EquipModelGender { get; init; }
            public bool HeadEquip { get; init; }
            public bool BodyEquip { get; init; }
            public bool ArmEquip { get; init; }
            public bool LegEquip { get; init; }
        }

        private sealed class FormalSekiroEquipTables
        {
            public Dictionary<int, FormalWeaponRow> Weapons { get; } = new Dictionary<int, FormalWeaponRow>();
            public Dictionary<int, FormalProtectorRow> Protectors { get; } = new Dictionary<int, FormalProtectorRow>();
        }

        private sealed class FormalSekiroSkillParams
        {
            public Dictionary<long, ParamData.BehaviorParam> BehaviorNpc { get; } = new Dictionary<long, ParamData.BehaviorParam>();
            public Dictionary<long, ParamData.BehaviorParam> BehaviorPc { get; } = new Dictionary<long, ParamData.BehaviorParam>();
            public Dictionary<long, ParamData.AtkParam> AtkNpc { get; } = new Dictionary<long, ParamData.AtkParam>();
            public Dictionary<long, ParamData.AtkParam> AtkPc { get; } = new Dictionary<long, ParamData.AtkParam>();
            public Dictionary<long, ParamData.SpEffectParam> SpEffect { get; } = new Dictionary<long, ParamData.SpEffectParam>();
            public Dictionary<long, ParamData.EquipParamWeapon> EquipParamWeapon { get; } = new Dictionary<long, ParamData.EquipParamWeapon>();
            public Dictionary<string, int[]> ProstheticOverrides { get; } = new Dictionary<string, int[]>(StringComparer.Ordinal);
        }

        private sealed class ExportCharacterReport
        {
            public string CharacterId { get; init; }
            public bool ModelSucceeded { get; set; }
            public string ModelFileName { get; set; }
            public bool MaterialManifestSucceeded { get; set; }
            public bool AnimationsSucceeded { get; set; }
            public int AnimationCount { get; set; }
            public bool TexturesSucceeded { get; set; }
            public int TextureCount { get; set; }
            public bool SkillsSucceeded { get; set; }
            public bool ParamsSucceeded { get; set; }
            public bool HasCanonicalSkillConfig { get; set; }
            public string SkillConfigFileName { get; set; }

            public bool IsSuccessfulFormalExport => ModelSucceeded && MaterialManifestSucceeded && AnimationsSucceeded && TexturesSucceeded && SkillsSucceeded && ParamsSucceeded && HasCanonicalSkillConfig;

            public JObject ToJson()
            {
                return new JObject
                {
                    ["characterId"] = CharacterId,
                    ["model"] = new JObject
                    {
                        ["success"] = ModelSucceeded,
                        ["fileName"] = ModelFileName ?? string.Empty,
                        ["materialManifest"] = MaterialManifestSucceeded,
                    },
                    ["animations"] = new JObject
                    {
                        ["success"] = AnimationsSucceeded,
                        ["count"] = AnimationCount,
                    },
                    ["textures"] = new JObject
                    {
                        ["success"] = TexturesSucceeded,
                        ["count"] = TextureCount,
                    },
                    ["skills"] = new JObject
                    {
                        ["success"] = SkillsSucceeded,
                        ["canonicalSkillConfig"] = HasCanonicalSkillConfig,
                        ["fileName"] = SkillConfigFileName ?? string.Empty,
                    },
                    ["params"] = new JObject
                    {
                        ["success"] = ParamsSucceeded,
                    },
                    ["formalSuccess"] = IsSuccessfulFormalExport,
                };
            }
        }

        private static readonly Dictionary<string, FormalSekiroEquipTables> FormalSekiroEquipTableCache
            = new Dictionary<string, FormalSekiroEquipTables>(StringComparer.OrdinalIgnoreCase);

        private static readonly Dictionary<string, FormalSekiroSkillParams> FormalSekiroSkillParamCache
            = new Dictionary<string, FormalSekiroSkillParams>(StringComparer.OrdinalIgnoreCase);

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
            Console.WriteLine("  --model-binder <path> Optional override binder for model export (absolute or relative to game dir)");
            Console.WriteLine("  --anibnd <path>       Optional override ANIBND for animation/skill export (absolute or relative to game dir)");
            Console.WriteLine("  --anim <id>           Optional animation clip filter (e.g., a000_200000)");
            Console.WriteLine("  --keep-intermediates  Reserved for debug exports; formal export keeps only canonical deliverables by default");
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
            var reports = new List<ExportCharacterReport>();

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

                    reports.Add(ExportCharacter(gameDir, chrPath, chrId, chrOutputDir, args, ExportSections.All));
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
            string summaryPath = Path.Combine(output, "export_summary.json");
            var summaryJson = new JObject
            {
                ["characters"] = new JArray(reports.Select(report => report.ToJson())),
                ["formalSuccessCount"] = reports.Count(report => report.IsSuccessfulFormalExport),
                ["formalFailureCount"] = reports.Count(report => !report.IsSuccessfulFormalExport),
                ["exceptionCount"] = errors,
                ["elapsedSeconds"] = sw.Elapsed.TotalSeconds,
            };
            File.WriteAllText(summaryPath, summaryJson.ToString(Newtonsoft.Json.Formatting.Indented));

            Console.WriteLine($"═══════════════════════════════════════════");
            Console.WriteLine($"Export complete: {reports.Count(report => report.IsSuccessfulFormalExport)} formal successes, {reports.Count(report => !report.IsSuccessfulFormalExport)} formal failures, {errors} exceptions in {sw.Elapsed.TotalSeconds:F1}s");
            Console.WriteLine($"Summary: {summaryPath}");
            return (errors > 0 || reports.Any(report => !report.IsSuccessfulFormalExport)) ? 1 : 0;
        }

        static ExportCharacterReport ExportCharacter(string gameDir, string chrBndPath, string chrId, string outputDir,
            Dictionary<string, string> args, ExportSections sections)
        {
            var report = new ExportCharacterReport { CharacterId = chrId };

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
            List<FLVER2> overrideFlvers = null;
            HKX skeletonHkx = null;
            List<byte[]> tpfDataList = new List<byte[]>();

            foreach (var file in bnd.Files)
            {
                string name = file.Name?.ToLower() ?? "";

                if ((sections.HasFlag(ExportSections.Model) || sections.HasFlag(ExportSections.Animations))
                    && (name.EndsWith(".flver") || name.EndsWith(".flver.dcx")))
                {
                    byte[] d = file.Bytes;
                    if (DCX.Is(d)) d = DCX.Decompress(d);
                    try { flver = FLVER2.Read(d); }
                    catch (Exception ex) { Console.Error.WriteLine($"  FLVER parse error: {ex.Message}"); }
                }
                else if ((sections.HasFlag(ExportSections.Model) || sections.HasFlag(ExportSections.Animations))
                    && name.EndsWith(".hkx") && !name.Contains("_c.hkx") && !name.EndsWith(".hkxpwv"))
                {
                    // Sekiro skeleton HKX: named "{chrId}.HKX" (NOT "{chrId}_c.hkx" which is collision)
                    byte[] d = file.Bytes;
                    if (DCX.Is(d)) d = DCX.Decompress(d);
                    skeletonHkx = TryReadHkx(d, file.Name);
                }
                else if (sections.HasFlag(ExportSections.Textures)
                    && (name.EndsWith(".tpf") || name.EndsWith(".tpf.dcx")))
                {
                    byte[] d = file.Bytes;
                    if (DCX.Is(d)) d = DCX.Decompress(d);
                    tpfDataList.Add(d);
                }
            }

            Console.WriteLine($"  CHRBND: FLVER={flver != null} (nodes={flver?.Nodes?.Count}, meshes={flver?.Meshes?.Count}), Skeleton={skeletonHkx != null}, TPFs={tpfDataList.Count}");
            var modelBinderPaths = (sections.HasFlag(ExportSections.Model) || sections.HasFlag(ExportSections.Animations))
                ? ResolveFormalModelBinderPaths(gameDir, chrId, args)
                : new List<string>();
            if (modelBinderPaths.Count > 0)
            {
                try
                {
                    overrideFlvers = modelBinderPaths
                        .Select(TryReadFirstFlverFromBinder)
                        .Where(f => f != null)
                        .ToList();

                    if (overrideFlvers.Count > 0)
                        flver = overrideFlvers[0];

                    Console.WriteLine($"  MODEL ASSEMBLY: {string.Join(", ", modelBinderPaths.Select(Path.GetFileName))} -> FLVERs={overrideFlvers.Count}, meshes={overrideFlvers.Sum(f => f.Meshes?.Count ?? 0)}");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  MODEL ASSEMBLY error: {ex.Message}");
                }
            }

            // ═══════════════════════════════════════════
            // 2. Load TEXBND → textures (separate file)
            // ═══════════════════════════════════════════
            string texbndPath = Path.Combine(chrDir, $"{chrId}.texbnd.dcx");
            if (sections.HasFlag(ExportSections.Textures) && File.Exists(texbndPath))
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
            string overrideAnibnd = args.GetValueOrDefault("anibnd");
            if (!string.IsNullOrWhiteSpace(overrideAnibnd))
                anibndPath = ResolveGamePath(gameDir, overrideAnibnd);
            byte[] anibndRawBytes = null;
            byte[] compendiumBytes = null;

            if ((sections.HasFlag(ExportSections.Animations) || sections.HasFlag(ExportSections.Skills))
                && File.Exists(anibndPath))
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
                    Console.WriteLine($"  ANIBND: {Path.GetFileName(anibndPath)} TAE={tae != null} (anims={tae?.Animations?.Count}), HKX anims={hkxCount}, Compendium={compendiumBytes != null}");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ANIBND error: {ex.Message}");
                }
            }

            // ═══════════════════════════════════════════
            // 4. Export model (DAE/Collada)
            // ═══════════════════════════════════════════
            var modelFlvers = overrideFlvers?.Count > 0 ? overrideFlvers : (flver != null ? new List<FLVER2> { flver } : null);
            if (sections.HasFlag(ExportSections.Model)
                && modelFlvers != null && modelFlvers.Any(f => f?.Meshes != null && f.Meshes.Count > 0))
            {
                Directory.CreateDirectory(modelDir);
                string modelPath = Path.Combine(modelDir, $"{chrId}.fbx");
                try
                {
                    var fbxExporter = new FlverToFbxExporter();
                    if (modelFlvers.Count > 1 || overrideFlvers?.Count > 0)
                        fbxExporter.Export(modelFlvers, skeletonHkx, modelPath);
                    else
                        fbxExporter.Export(modelFlvers[0], modelPath);
                    // Check for the actual output file (format may have fallen back to .gltf/.glb/.dae)
                    string actualModelFile = FindExportedFile(modelDir, chrId, new[] { ".fbx", ".gltf", ".glb" });
                    if (actualModelFile != null)
                    {
                        report.ModelSucceeded = true;
                        report.ModelFileName = Path.GetFileName(actualModelFile);
                        Console.WriteLine($"  ✓ Model: {Path.GetFileName(actualModelFile)} ({new FileInfo(actualModelFile).Length / 1024}KB)");
                    }
                    else
                        Console.Error.WriteLine($"  ✗ Model: no formal model deliverable produced (.fbx/.gltf/.glb)");
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
                    matExporter.ExportToFile(modelFlvers, matPath);
                    report.MaterialManifestSucceeded = true;
                    Console.WriteLine($"  ✓ Material manifest exported");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ✗ Material manifest error: {ex.Message}");
                }
            }
            else if (modelFlvers != null && modelFlvers.Count > 0)
            {
                Console.WriteLine($"  - Model: skipped (0 meshes, parts may be in separate files)");
            }

            // ═══════════════════════════════════════════
            // 5. Export textures
            // ═══════════════════════════════════════════
            if (sections.HasFlag(ExportSections.Textures) && tpfDataList.Count > 0)
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
                report.TextureCount = texCount;
                report.TexturesSucceeded = texCount > 0 && texErrors == 0;
                Console.WriteLine($"  ✓ Textures: {texCount} exported{(texErrors > 0 ? $", {texErrors} errors" : "")}");
            }

            // ═══════════════════════════════════════════
            // 6. Export animations from ANIBND
            // ═══════════════════════════════════════════
            if (sections.HasFlag(ExportSections.Animations) && anibndRawBytes != null && skeletonHkx != null)
            {
                Directory.CreateDirectory(animDir);
                var animExporter = new AnimationToFbxExporter();
                string animFilter = args.GetValueOrDefault("anim");

                try
                {
                    animExporter.ExportAnibnd(anibndRawBytes, skeletonHkx, animDir, modelFlvers, animFilter,
                        (name, cur, total) =>
                        {
                            if (cur % 20 == 0 || cur == total)
                                Console.Write($"\r  Animations: {cur}/{total}...");
                        });
                    Console.WriteLine();

                    int animCount = Directory.GetFiles(animDir, "*.gltf").Length
                        + Directory.GetFiles(animDir, "*.fbx").Length
                        + Directory.GetFiles(animDir, "*.glb").Length;
                    report.AnimationCount = animCount;
                    report.AnimationsSucceeded = animCount > 0;
                    Console.WriteLine($"  ✓ Animations: {animCount} formal clips exported");
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
            if (sections.HasFlag(ExportSections.Skills) && tae != null)
            {
                Directory.CreateDirectory(skillDir);
                var skillExporter = new SkillConfigExporter();
                var paramExporter = new ParamExporter();

                // Try to load template
                string templatePath = Path.Combine(
                    AppDomain.CurrentDomain.BaseDirectory, "Res", "TAE.Template.SDT.xml");
                if (File.Exists(templatePath))
                    skillExporter.LoadTemplate(templatePath);

                try
                {
                    FormalSekiroSkillParams formalParams = LoadFormalSekiroSkillParams(gameDir);
                    var exportedParams = BuildCanonicalParamPayload(paramExporter, formalParams);
                    var prosthetics = paramExporter.ExportProstheticOverrides(formalParams.ProstheticOverrides);
                    string skillConfigPath = Path.Combine(skillDir, "skill_config.json");
                    skillExporter.ExportToFile(tae, skillConfigPath, new SkillConfigExporter.SkillConfigExportContext
                    {
                        CharacterId = chrId,
                        ModelFlvers = modelFlvers,
                        Params = exportedParams,
                        Prosthetics = prosthetics,
                        ResolveAnimationFileName = animName => ResolveAnimationDeliverableFileName(animDir, animName),
                    });
                    report.ParamsSucceeded = exportedParams.Properties().Count() >= 4;
                    report.SkillsSucceeded = true;
                    report.HasCanonicalSkillConfig = true;
                    report.SkillConfigFileName = Path.GetFileName(skillConfigPath);
                    Console.WriteLine($"  ✓ Skills: {tae.Animations.Count} animations, skill_config.json exported");
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"  ✗ Skill config error: {ex.Message}");
                }
            }

            string reportPath = Path.Combine(outputDir, "export_report.json");
            File.WriteAllText(reportPath, report.ToJson().ToString(Newtonsoft.Json.Formatting.Indented));
            Console.WriteLine($"  Report: {Path.GetFileName(reportPath)} {(report.IsSuccessfulFormalExport ? "OK" : "FAILED")}");

            return report;
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
            ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Model);
            return 0;
        }

        static int RunExportAnims(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Animations);
            return 0;
        }

        static int RunExportTextures(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Textures);
            return 0;
        }

        static int RunExportSkills(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Skills);
            return 0;
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
        static FLVER2 TryReadFirstFlverFromBinder(string binderPath)
        {
            if (!File.Exists(binderPath))
                throw new FileNotFoundException($"Model binder not found: {binderPath}");

            byte[] binderData = File.ReadAllBytes(binderPath);
            if (DCX.Is(binderData)) binderData = DCX.Decompress(binderData);
            var bnd = BND4.Read(binderData);

            foreach (var file in bnd.Files)
            {
                string name = file.Name?.ToLower() ?? "";
                if (!name.EndsWith(".flver") && !name.EndsWith(".flver.dcx"))
                    continue;

                byte[] flverData = file.Bytes;
                if (DCX.Is(flverData)) flverData = DCX.Decompress(flverData);
                return FLVER2.Read(flverData);
            }

            return null;
        }

        static string ResolveGamePath(string gameDir, string path)
        {
            if (Path.IsPathRooted(path))
                return path;

            return Path.Combine(gameDir, path);
        }

        static IEnumerable<string> SplitOverridePaths(string rawPaths)
        {
            return (rawPaths ?? "")
                .Split(new[] { ';', ',' }, StringSplitOptions.RemoveEmptyEntries)
                .Select(path => path.Trim())
                .Where(path => path.Length > 0);
        }

        static List<string> ResolveFormalModelBinderPaths(string gameDir, string chrId, Dictionary<string, string> args)
        {
            string overrideModelBinder = args.GetValueOrDefault("model-binder");
            if (!string.IsNullOrWhiteSpace(overrideModelBinder))
            {
                return SplitOverridePaths(overrideModelBinder)
                    .Select(path => ResolveGamePath(gameDir, path))
                    .ToList();
            }

            if (chrId.Equals("c0000", StringComparison.OrdinalIgnoreCase))
                return ResolveFormalSekiroPlayerModelBinderPaths(gameDir, GetFormalSekiroExportAssemblyConfig(chrId), isFemale: false).ToList();

            return new List<string>();
        }

        static NewChrAsmCfgJson GetFormalSekiroExportAssemblyConfig(string chrId)
        {
            if (!chrId.Equals("c0000", StringComparison.OrdinalIgnoreCase))
                return null;

            return new NewChrAsmCfgJson
            {
                GameType = SoulsGames.SDT,
                IsFemale = false,
                EquipIDs = new Dictionary<NewChrAsm.EquipSlotTypes, int>
                {
                    { NewChrAsm.EquipSlotTypes.Head, 304000 },
                    { NewChrAsm.EquipSlotTypes.Body, 901000 },
                    { NewChrAsm.EquipSlotTypes.Arms, 102000 },
                    { NewChrAsm.EquipSlotTypes.Legs, 103000 },
                    { NewChrAsm.EquipSlotTypes.RightWeapon, 75000 },
                    { NewChrAsm.EquipSlotTypes.SekiroGrapplingHook, 9500000 },
                },
                DirectEquipInfos = new Dictionary<NewChrAsm.EquipSlotTypes, NewEquipSlot_Armor.DirectEquipInfo>
                {
                    {
                        NewChrAsm.EquipSlotTypes.Face,
                        new NewEquipSlot_Armor.DirectEquipInfo
                        {
                            PartPrefix = NewEquipSlot_Armor.DirectEquipPartPrefix.FC,
                            Gender = NewEquipSlot_Armor.DirectEquipGender.UnisexUseMForBoth,
                            ModelID = 200,
                        }
                    },
                },
            };
        }

        static IEnumerable<string> ResolveFormalSekiroPlayerModelBinderPaths(string gameDir, NewChrAsmCfgJson config, bool isFemale)
        {
            string partsDir = Path.Combine(gameDir, "parts");
            if (!Directory.Exists(partsDir))
                throw new DirectoryNotFoundException($"Sekiro parts directory not found: {partsDir}");

            if (config == null)
                throw new InvalidOperationException("Formal Sekiro export assembly configuration is missing.");

            var equipTables = LoadFormalSekiroEquipTables(gameDir);
            var requiredBinderPaths = new List<string>();

            foreach (var slot in new[]
            {
                NewChrAsm.EquipSlotTypes.Head,
                NewChrAsm.EquipSlotTypes.Face,
                NewChrAsm.EquipSlotTypes.Body,
                NewChrAsm.EquipSlotTypes.Arms,
                NewChrAsm.EquipSlotTypes.Legs,
                NewChrAsm.EquipSlotTypes.RightWeapon,
                NewChrAsm.EquipSlotTypes.LeftWeapon,
                NewChrAsm.EquipSlotTypes.SekiroMortalBlade,
                NewChrAsm.EquipSlotTypes.SekiroGrapplingHook,
            })
            {
                string binderFileName = ResolveFormalSekiroPartBinderFileName(partsDir, equipTables, config, slot, isFemale, config.CurrentPartSuffixType);
                if (string.IsNullOrWhiteSpace(binderFileName))
                    continue;

                requiredBinderPaths.Add(Path.Combine(partsDir, binderFileName));
            }

            return requiredBinderPaths
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToList();
        }

        static string ResolveFormalSekiroPartBinderFileName(
            string partsDir,
            FormalSekiroEquipTables equipTables,
            NewChrAsmCfgJson config,
            NewChrAsm.EquipSlotTypes slot,
            bool isFemale,
            ParamData.PartSuffixType suffixType)
        {
            if (config.DirectEquipInfos.TryGetValue(slot, out var directEquipInfo))
                return ResolveDirectEquipBinderFileName(partsDir, directEquipInfo, isFemale, suffixType, slot);

            if (!config.EquipIDs.TryGetValue(slot, out int equipId) || equipId < 0)
                return null;

            if (slot is NewChrAsm.EquipSlotTypes.RightWeapon
                or NewChrAsm.EquipSlotTypes.LeftWeapon
                or NewChrAsm.EquipSlotTypes.SekiroMortalBlade
                or NewChrAsm.EquipSlotTypes.SekiroGrapplingHook)
            {
                if (!equipTables.Weapons.TryGetValue(equipId, out var weaponRow))
                    throw new InvalidOperationException($"Formal EquipParamWeapon row {equipId} not found for slot {slot}.");

                string fileName = $"WP_A_{weaponRow.EquipModelID:D4}.partsbnd.dcx";
                EnsureFormalBinderExists(partsDir, fileName, slot, equipId);
                return fileName;
            }

            if (!equipTables.Protectors.TryGetValue(equipId, out var protectorRow))
                throw new InvalidOperationException($"Formal EquipParamProtector row {equipId} not found for slot {slot}.");

            string protectorFileName = ResolveProtectorBinderFileName(partsDir, protectorRow, isFemale, suffixType, slot, equipId);
            EnsureFormalBinderExists(partsDir, protectorFileName, slot, equipId);
            return protectorFileName;
        }

        static string ResolveDirectEquipBinderFileName(
            string partsDir,
            NewEquipSlot_Armor.DirectEquipInfo directEquipInfo,
            bool isFemale,
            ParamData.PartSuffixType suffixType,
            NewChrAsm.EquipSlotTypes slot)
        {
            string prefix = directEquipInfo.PartPrefix == NewEquipSlot_Armor.DirectEquipPartPrefix.None
                ? GetArmorSlotPrefix(slot)
                : directEquipInfo.PartPrefix.ToString();

            if (string.IsNullOrWhiteSpace(prefix) || directEquipInfo.ModelID < 0)
                throw new InvalidOperationException($"Formal direct-equip configuration is incomplete for slot {slot}.");

            string suffix = GetPartSuffixLiteral(suffixType);
            string genderSegment = directEquipInfo.Gender switch
            {
                NewEquipSlot_Armor.DirectEquipGender.UnisexUseA => "A",
                NewEquipSlot_Armor.DirectEquipGender.MaleOnlyUseM => "M",
                NewEquipSlot_Armor.DirectEquipGender.UnisexUseMForBoth => "M",
                NewEquipSlot_Armor.DirectEquipGender.FemaleOnlyUseF => "F",
                NewEquipSlot_Armor.DirectEquipGender.BothGendersUseMF => isFemale ? "F" : "M",
                _ => throw new InvalidOperationException($"Unsupported formal direct-equip gender '{directEquipInfo.Gender}' for slot {slot}."),
            };

            string candidate = $"{prefix}_{genderSegment}_{directEquipInfo.ModelID:D4}{suffix}.partsbnd.dcx";
            if (File.Exists(Path.Combine(partsDir, candidate)))
                return candidate;

            if (suffixType != ParamData.PartSuffixType.M)
            {
                string unsuffixed = $"{prefix}_{genderSegment}_{directEquipInfo.ModelID:D4}.partsbnd.dcx";
                if (File.Exists(Path.Combine(partsDir, unsuffixed)))
                    return unsuffixed;
            }

            throw new FileNotFoundException($"Formal direct-equip parts binder not found for slot {slot}: {candidate}");
        }

        static string ResolveProtectorBinderFileName(
            string partsDir,
            FormalProtectorRow protectorRow,
            bool isFemale,
            ParamData.PartSuffixType suffixType,
            NewChrAsm.EquipSlotTypes slot,
            int equipId)
        {
            string prefix = GetProtectorPrefix(protectorRow, slot, equipId);
            string suffix = GetPartSuffixLiteral(suffixType);
            string genderSegment = protectorRow.EquipModelGender switch
            {
                ParamData.EquipModelGenders.UnisexUseA => "A",
                ParamData.EquipModelGenders.MaleOnlyUseM => "M",
                ParamData.EquipModelGenders.UnisexUseMForBoth => "M",
                ParamData.EquipModelGenders.FemaleOnlyUseF => "F",
                ParamData.EquipModelGenders.BothGendersUseMF => isFemale ? "F" : "M",
                _ => throw new InvalidOperationException($"Unsupported formal protector gender '{protectorRow.EquipModelGender}' for equip ID {equipId} ({slot})."),
            };

            string candidate = $"{prefix}_{genderSegment}_{protectorRow.EquipModelID:D4}{suffix}.partsbnd.dcx";
            if (File.Exists(Path.Combine(partsDir, candidate)))
                return candidate;

            if (suffixType != ParamData.PartSuffixType.M)
            {
                string unsuffixed = $"{prefix}_{genderSegment}_{protectorRow.EquipModelID:D4}.partsbnd.dcx";
                if (File.Exists(Path.Combine(partsDir, unsuffixed)))
                    return unsuffixed;
            }

            throw new FileNotFoundException($"Formal protector parts binder not found for equip ID {equipId} ({slot}): {candidate}");
        }

        static FormalSekiroEquipTables LoadFormalSekiroEquipTables(string gameDir)
        {
            string normalizedGameDir = Path.GetFullPath(gameDir);
            if (FormalSekiroEquipTableCache.TryGetValue(normalizedGameDir, out var cached))
                return cached;

            string paramBinderPath = Path.Combine(normalizedGameDir, "param", "gameparam", "gameparam.parambnd.dcx");
            if (!File.Exists(paramBinderPath))
                throw new FileNotFoundException($"Formal Sekiro gameparam binder not found: {paramBinderPath}");

            byte[] paramBinderBytes = File.ReadAllBytes(paramBinderPath);
            if (DCX.Is(paramBinderBytes))
                paramBinderBytes = DCX.Decompress(paramBinderBytes);

            var paramBinder = BND4.Read(paramBinderBytes);
            var result = new FormalSekiroEquipTables();
            ReadFormalWeaponRows(paramBinder, result.Weapons);
            ReadFormalProtectorRows(paramBinder, result.Protectors);

            FormalSekiroEquipTableCache[normalizedGameDir] = result;
            return result;
        }

        static void ReadFormalWeaponRows(BND4 paramBinder, Dictionary<int, FormalWeaponRow> destination)
        {
            var param = ReadParamHackFromBinder(paramBinder, "EquipParamWeapon");
            try
            {
                foreach (var row in param.Rows)
                {
                    var reader = param.GetRowReader(row);
                    long start = reader.Position;
                    reader.ReadInt32();
                    reader.Position = start + 0xB8;
                    short equipModelId = reader.ReadInt16();
                    destination[row.ID] = new FormalWeaponRow { EquipModelID = equipModelId };
                }
            }
            finally
            {
                param.DisposeRowReader();
            }
        }

        static void ReadFormalProtectorRows(BND4 paramBinder, Dictionary<int, FormalProtectorRow> destination)
        {
            var param = ReadParamHackFromBinder(paramBinder, "EquipParamProtector");
            try
            {
                foreach (var row in param.Rows)
                {
                    var reader = param.GetRowReader(row);
                    long start = reader.Position;
                    reader.Position = start + 0xA0;
                    short equipModelId = reader.ReadInt16();

                    reader.Position = start + 0xD1;
                    var gender = (ParamData.EquipModelGenders)reader.ReadByte();

                    reader.Position = start + 0xD8;
                    var firstBitmask = ReadBitmask(reader, 5);

                    destination[row.ID] = new FormalProtectorRow
                    {
                        EquipModelID = equipModelId,
                        EquipModelGender = gender,
                        HeadEquip = firstBitmask[1],
                        BodyEquip = firstBitmask[2],
                        ArmEquip = firstBitmask[3],
                        LegEquip = firstBitmask[4],
                    };
                }
            }
            finally
            {
                param.DisposeRowReader();
            }
        }

        static PARAM_Hack ReadParamHackFromBinder(BND4 paramBinder, string paramName)
        {
            var binderFile = paramBinder.Files.FirstOrDefault(file =>
                file.Name != null && file.Name.IndexOf(paramName, StringComparison.OrdinalIgnoreCase) >= 0);
            if (binderFile == null)
                throw new InvalidOperationException($"Formal param '{paramName}' not found in Sekiro gameparam binder.");

            byte[] paramBytes = binderFile.Bytes;
            if (DCX.Is(paramBytes))
                paramBytes = DCX.Decompress(paramBytes);

            return PARAM_Hack.Read(paramBytes);
        }

        static List<bool> ReadBitmask(BinaryReaderEx reader, int bitCount)
        {
            var result = new List<bool>(bitCount);
            byte[] bytes = reader.ReadBytes((int)Math.Ceiling(bitCount / 8.0));
            for (int i = 0; i < bitCount; i++)
                result.Add((bytes[i / 8] & (1 << (i % 8))) != 0);
            return result;
        }

        static FormalSekiroSkillParams LoadFormalSekiroSkillParams(string gameDir)
        {
            string normalizedGameDir = Path.GetFullPath(gameDir);
            if (FormalSekiroSkillParamCache.TryGetValue(normalizedGameDir, out var cached))
                return cached;

            string paramBinderPath = Path.Combine(normalizedGameDir, "param", "gameparam", "gameparam.parambnd.dcx");
            if (!File.Exists(paramBinderPath))
                throw new FileNotFoundException($"Formal Sekiro gameparam binder not found: {paramBinderPath}");

            byte[] paramBinderBytes = File.ReadAllBytes(paramBinderPath);
            if (DCX.Is(paramBinderBytes))
                paramBinderBytes = DCX.Decompress(paramBinderBytes);

            var paramBinder = BND4.Read(paramBinderBytes);
            var result = new FormalSekiroSkillParams();

            RunWithSekiroDocumentContext(() =>
            {
                ReadTypedParamRows(paramBinder, "BehaviorParam", result.BehaviorNpc);
                ReadTypedParamRows(paramBinder, "BehaviorParam_PC", result.BehaviorPc);
                ReadTypedParamRows(paramBinder, "AtkParam_Npc", result.AtkNpc);
                ReadTypedParamRows(paramBinder, "AtkParam_Pc", result.AtkPc);
                ReadTypedParamRows(paramBinder, "SpEffectParam", result.SpEffect);
                ReadTypedParamRows(paramBinder, "EquipParamWeapon", result.EquipParamWeapon);
                return 0;
            });

            FormalSekiroSkillParamCache[normalizedGameDir] = result;
            return result;
        }

        static void ReadTypedParamRows<T>(BND4 paramBinder, string paramName, Dictionary<long, T> destination)
            where T : ParamData, new()
        {
            var param = ReadParamHackFromBinder(paramBinder, paramName);
            try
            {
                foreach (var row in param.Rows)
                {
                    var entry = new T
                    {
                        ID = row.ID,
                        Name = row.Name,
                    };
                    entry.Read(param.GetRowReader(row));
                    destination[row.ID] = entry;
                }
            }
            finally
            {
                param.DisposeRowReader();
            }
        }

        static JObject BuildCanonicalParamPayload(ParamExporter exporter, FormalSekiroSkillParams formalParams)
        {
            return exporter.ExportTypedParams(new Dictionary<string, JObject>(StringComparer.Ordinal)
            {
                ["AtkParam"] = new JObject
                {
                    ["player"] = exporter.ExportTypedParamTable(formalParams.AtkPc, "AtkParam_Pc"),
                    ["npc"] = exporter.ExportTypedParamTable(formalParams.AtkNpc, "AtkParam_Npc"),
                },
                ["BehaviorParam"] = new JObject
                {
                    ["player"] = exporter.ExportTypedParamTable(formalParams.BehaviorPc, "BehaviorParam_PC"),
                    ["npc"] = exporter.ExportTypedParamTable(formalParams.BehaviorNpc, "BehaviorParam"),
                },
                ["SpEffectParam"] = exporter.ExportTypedParamTable(formalParams.SpEffect, "SpEffectParam"),
                ["EquipParamWeapon"] = exporter.ExportTypedParamTable(formalParams.EquipParamWeapon, "EquipParamWeapon"),
            });
        }

        static string ResolveAnimationDeliverableFileName(string animDir, string animName)
        {
            return Path.GetFileName(FindExportedFile(animDir, animName, new[] { ".fbx", ".gltf", ".glb" })
                ?? $"{animName}.gltf");
        }

        static T RunWithSekiroDocumentContext<T>(Func<T> callback)
        {
            if (callback == null)
                throw new ArgumentNullException(nameof(callback));

            var managerType = typeof(zzz_DocumentManager);
            var currentDocumentField = managerType.GetField("_currentDocument", BindingFlags.Static | BindingFlags.NonPublic);
            var documentsField = managerType.GetField("Documents", BindingFlags.Static | BindingFlags.NonPublic);
            if (currentDocumentField == null || documentsField == null)
                throw new InvalidOperationException("Unable to establish a temporary Sekiro document context.");

            var previousCurrentDocument = currentDocumentField.GetValue(null);
            var documents = (System.Collections.IList)documentsField.GetValue(null);

            var fakeDoc = new zzz_DocumentIns((DSAProj)null)
            {
                GameRoot = null,
            };
            fakeDoc.GameRoot = new zzz_GameRootIns(fakeDoc)
            {
                GameType = SoulsGames.SDT,
            };

            documents?.Add(fakeDoc);
            currentDocumentField.SetValue(null, fakeDoc);

            try
            {
                return callback();
            }
            finally
            {
                currentDocumentField.SetValue(null, previousCurrentDocument);
                documents?.Remove(fakeDoc);
            }
        }

        static string GetProtectorPrefix(FormalProtectorRow protectorRow, NewChrAsm.EquipSlotTypes slot, int equipId)
        {
            int enabledSlotCount = (protectorRow.HeadEquip ? 1 : 0)
                + (protectorRow.BodyEquip ? 1 : 0)
                + (protectorRow.ArmEquip ? 1 : 0)
                + (protectorRow.LegEquip ? 1 : 0);

            if (enabledSlotCount != 1)
                throw new InvalidOperationException($"Formal protector row {equipId} for slot {slot} must map to exactly one armor prefix.");

            if (protectorRow.HeadEquip)
                return "HD";
            if (protectorRow.BodyEquip)
                return "BD";
            if (protectorRow.ArmEquip)
                return "AM";
            return "LG";
        }

        static string GetArmorSlotPrefix(NewChrAsm.EquipSlotTypes slot)
        {
            return slot switch
            {
                NewChrAsm.EquipSlotTypes.Head => "HD",
                NewChrAsm.EquipSlotTypes.Body => "BD",
                NewChrAsm.EquipSlotTypes.Arms => "AM",
                NewChrAsm.EquipSlotTypes.Legs => "LG",
                NewChrAsm.EquipSlotTypes.Face => "FC",
                NewChrAsm.EquipSlotTypes.Hair => "HR",
                _ => null,
            };
        }

        static string GetPartSuffixLiteral(ParamData.PartSuffixType suffixType)
        {
            return suffixType switch
            {
                ParamData.PartSuffixType.M => "_M",
                ParamData.PartSuffixType.L => "_L",
                ParamData.PartSuffixType.U => "_U",
                _ => string.Empty,
            };
        }

        static void EnsureFormalBinderExists(string partsDir, string fileName, NewChrAsm.EquipSlotTypes slot, int equipId)
        {
            string fullPath = Path.Combine(partsDir, fileName);
            if (!File.Exists(fullPath))
                throw new FileNotFoundException($"Formal parts binder not found for slot {slot}, equip ID {equipId}: {fullPath}");
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

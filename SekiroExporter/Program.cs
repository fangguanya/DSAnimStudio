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
    /// Exports formal model/animation glTF, formal texture PNG, skill config JSON,
    /// and structured acceptance reports.
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
            private sealed class FailureEntry
            {
                public string Code { get; init; }
                public string Message { get; init; }

                public JObject ToJson()
                {
                    return new JObject
                    {
                        ["code"] = Code ?? string.Empty,
                        ["message"] = Message ?? string.Empty,
                    };
                }
            }

            public string SchemaVersion { get; init; } = "2.2";
            public string CharacterId { get; init; }
            public bool ModelSucceeded { get; set; }
            public string ModelFileName { get; set; }
            public bool MaterialManifestSucceeded { get; set; }
            public bool AnimationsSucceeded { get; set; }
            public int AnimationCount { get; set; }
            public int ExpectedAnimationCount { get; set; }
            public bool TexturesSucceeded { get; set; }
            public int TextureCount { get; set; }
            public bool SkillsSucceeded { get; set; }
            public bool ParamsSucceeded { get; set; }
            public bool HasCanonicalSkillConfig { get; set; }
            public string SkillConfigFileName { get; set; }
            public string AssetPackageFileName { get; set; }
            public string AnimationSourceAnibnd { get; set; }
            public string SkillSourceAnibnd { get; set; }
            public string FormalSkeletonRoot { get; set; }
            public JToken AssemblyProfile { get; set; }
            public List<FormalAnimationResolution> AnimationResolutions { get; } = new List<FormalAnimationResolution>();
            public List<JObject> TextureEntries { get; } = new List<JObject>();
            public List<string> MissingAnimationFiles { get; } = new List<string>();
            public List<string> MissingAnimationRootMotionFiles { get; } = new List<string>();
            private List<FailureEntry> ModelErrors { get; } = new List<FailureEntry>();
            private List<FailureEntry> AnimationErrors { get; } = new List<FailureEntry>();
            private List<FailureEntry> TextureErrors { get; } = new List<FailureEntry>();
            private List<FailureEntry> SkillErrors { get; } = new List<FailureEntry>();
            private List<FailureEntry> ParamErrors { get; } = new List<FailureEntry>();

            public int TextureErrorCount => TextureErrors.Count;
            public int SkillErrorCount => SkillErrors.Count;
            public int ParamErrorCount => ParamErrors.Count;

            public void AddModelError(string code, string message) => ModelErrors.Add(new FailureEntry { Code = code, Message = message });
            public void AddAnimationError(string code, string message) => AnimationErrors.Add(new FailureEntry { Code = code, Message = message });
            public void AddTextureError(string code, string message) => TextureErrors.Add(new FailureEntry { Code = code, Message = message });
            public void AddSkillError(string code, string message) => SkillErrors.Add(new FailureEntry { Code = code, Message = message });
            public void AddParamError(string code, string message) => ParamErrors.Add(new FailureEntry { Code = code, Message = message });

            public bool IsSuccessfulFormalExport => ModelSucceeded && MaterialManifestSucceeded && AnimationsSucceeded && TexturesSucceeded && SkillsSucceeded && ParamsSucceeded && HasCanonicalSkillConfig;

            private static JArray SerializeErrors(IEnumerable<FailureEntry> errors)
            {
                return new JArray(errors.Select(error => error.ToJson()));
            }

            public JObject ToJson()
            {
                return new JObject
                {
                    ["schemaVersion"] = SchemaVersion,
                    ["characterId"] = CharacterId,
                    ["sources"] = new JObject
                    {
                        ["animationSourceAnibnd"] = AnimationSourceAnibnd ?? string.Empty,
                        ["skillSourceAnibnd"] = SkillSourceAnibnd ?? string.Empty,
                    },
                    ["assemblyProfile"] = AssemblyProfile ?? JValue.CreateNull(),
                    ["animationResolutions"] = new JArray(AnimationResolutions.Select(resolution => resolution.ToJson())),
                    ["model"] = new JObject
                    {
                        ["success"] = ModelSucceeded,
                        ["fileName"] = ModelFileName ?? string.Empty,
                        ["formalSkeletonRoot"] = FormalSkeletonRoot ?? string.Empty,
                        ["materialManifest"] = MaterialManifestSucceeded,
                        ["errors"] = SerializeErrors(ModelErrors),
                    },
                    ["animations"] = new JObject
                    {
                        ["success"] = AnimationsSucceeded,
                        ["expectedCount"] = ExpectedAnimationCount,
                        ["count"] = AnimationCount,
                        ["missingFiles"] = new JArray(MissingAnimationFiles.OrderBy(file => file, StringComparer.OrdinalIgnoreCase)),
                        ["missingRootMotionFiles"] = new JArray(MissingAnimationRootMotionFiles.OrderBy(file => file, StringComparer.OrdinalIgnoreCase)),
                        ["errors"] = SerializeErrors(AnimationErrors),
                    },
                    ["textures"] = new JObject
                    {
                        ["success"] = TexturesSucceeded,
                        ["count"] = TextureCount,
                        ["entries"] = new JArray(TextureEntries),
                        ["errors"] = SerializeErrors(TextureErrors),
                    },
                    ["skills"] = new JObject
                    {
                        ["success"] = SkillsSucceeded,
                        ["canonicalSkillConfig"] = HasCanonicalSkillConfig,
                        ["fileName"] = SkillConfigFileName ?? string.Empty,
                        ["errors"] = SerializeErrors(SkillErrors),
                    },
                    ["assetPackage"] = new JObject
                    {
                        ["fileName"] = AssetPackageFileName ?? string.Empty,
                    },
                    ["params"] = new JObject
                    {
                        ["success"] = ParamsSucceeded,
                        ["errors"] = SerializeErrors(ParamErrors),
                    },
                    ["formalSuccess"] = IsSuccessfulFormalExport,
                };
            }
        }

        private sealed class FormalAssemblyProfile
        {
            public string Id { get; init; }
            public string Description { get; init; }
            public bool IsFemale { get; init; }
            public NewChrAsmCfgJson Config { get; init; }
            public IReadOnlyList<string> BinderPaths { get; init; }

            public JObject ToJson()
            {
                var equipIds = new JObject();
                foreach (var kvp in Config?.EquipIDs ?? new Dictionary<NewChrAsm.EquipSlotTypes, int>())
                    equipIds[kvp.Key.ToString()] = kvp.Value;

                var directEquip = new JObject();
                foreach (var kvp in Config?.DirectEquipInfos ?? new Dictionary<NewChrAsm.EquipSlotTypes, NewEquipSlot_Armor.DirectEquipInfo>())
                {
                    directEquip[kvp.Key.ToString()] = new JObject
                    {
                        ["partPrefix"] = kvp.Value.PartPrefix.ToString(),
                        ["gender"] = kvp.Value.Gender.ToString(),
                        ["modelId"] = kvp.Value.ModelID,
                    };
                }

                return new JObject
                {
                    ["id"] = Id ?? string.Empty,
                    ["description"] = Description ?? string.Empty,
                    ["isFemale"] = IsFemale,
                    ["weaponStyle"] = Config?.WeaponStyle.ToString() ?? string.Empty,
                    ["partSuffixType"] = Config?.CurrentPartSuffixType.ToString() ?? string.Empty,
                    ["equipIds"] = equipIds,
                    ["directEquip"] = directEquip,
                    ["binderPaths"] = new JArray((BinderPaths ?? Array.Empty<string>()).Select(Path.GetFileName)),
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
            ValidateFormalArguments(command, parsedArgs);

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
            Console.WriteLine("  export-model     Export FLVER model to glTF 2.0");
            Console.WriteLine("  export-anims     Export animations to glTF 2.0");
            Console.WriteLine("  export-textures  Export textures to PNG");
            Console.WriteLine("  export-skills    Export TAE skill config to JSON");
            Console.WriteLine();
            Console.WriteLine("Options:");
            Console.WriteLine("  --game-dir <path>   Sekiro installation directory");
            Console.WriteLine("  --chr <id>          Character ID (e.g., c0000). If omitted, exports all.");
            Console.WriteLine("  --output <path>     Output directory");
            Console.WriteLine("  --format <png>      Texture format (formal export only supports png)");
            Console.WriteLine("  --model-binder <path> Optional override binder for model export (absolute or relative to game dir)");
            Console.WriteLine("  --anibnd <path>       Optional override ANIBND for animation export (absolute or relative to game dir)");
            Console.WriteLine("  --skill-anibnd <path> Optional override ANIBND for formal TAE/skill export (absolute or relative to game dir)");
            Console.WriteLine("  --anim <id>           Optional animation clip filter (e.g., a000_200000)");
            Console.WriteLine("  --anim-list <ids>     Optional comma-separated animation request list (e.g., a000_202010,a000_202011)");
            Console.WriteLine("  --keep-intermediates  Reserved for debug exports; formal export keeps only canonical deliverables by default");
        }

        static Dictionary<string, string> ParseArgs(string[] args)
        {
            var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            for (int i = 0; i < args.Length; i++)
            {
                if (!args[i].StartsWith("--"))
                    continue;

                string key = args[i].Substring(2);
                string value = "true";
                if (i + 1 < args.Length && !args[i + 1].StartsWith("--"))
                {
                    value = args[i + 1];
                    i++;
                }

                dict[key] = value;
            }
            return dict;
        }

        static void ValidateFormalArguments(string command, Dictionary<string, string> args)
        {
            if (string.Equals(command, "convert-to-fbx", StringComparison.OrdinalIgnoreCase))
                return;

            if (args.ContainsKey("keep-intermediates"))
                throw new ArgumentException("Formal export does not permit '--keep-intermediates'. Debug/compat outputs must stay outside the formal pipeline.");

            string requestedTextureFormat = (args.GetValueOrDefault("format") ?? "png").Trim().ToLowerInvariant();
            if (requestedTextureFormat != "png")
                throw new ArgumentException("Formal export only supports '--format png'. DDS is not a formal deliverable.");
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

                    reports.Add(RunWithIsolatedSekiroDocumentContext(gameDir, () => ExportCharacter(gameDir, chrPath, chrId, chrOutputDir, args, ExportSections.All)));
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
                ["schemaVersion"] = "1.0",
                ["deliveryMode"] = "formal-only",
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
            var assetPackage = new FormalAssetPackageSummary { CharacterId = chrId };

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
                    && IsFormalSkeletonHkxEntry(file.Name, chrId))
                {
                    byte[] d = file.Bytes;
                    if (DCX.Is(d)) d = DCX.Decompress(d);
                    skeletonHkx = TryReadHkx(d, file.Name);
                }
            }

            if (sections.HasFlag(ExportSections.Textures))
                CollectTpfBytesFromBinder(bnd, tpfDataList);

            Console.WriteLine($"  CHRBND: FLVER={flver != null} (nodes={flver?.Nodes?.Count}, meshes={flver?.Meshes?.Count}), Skeleton={skeletonHkx != null}, TPFs={tpfDataList.Count}");
            var formalAssemblyProfile = ResolveFormalAssemblyProfile(gameDir, chrId, args);
            report.AssemblyProfile = formalAssemblyProfile.ToJson();
            var modelBinderPaths = (sections.HasFlag(ExportSections.Model) || sections.HasFlag(ExportSections.Animations) || sections.HasFlag(ExportSections.Textures))
                ? formalAssemblyProfile.BinderPaths.ToList()
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

                    if (sections.HasFlag(ExportSections.Textures))
                    {
                        foreach (string binderPath in modelBinderPaths)
                            tpfDataList.AddRange(ReadTpfBytesFromBinder(binderPath));
                    }

                    Console.WriteLine($"  MODEL ASSEMBLY: {string.Join(", ", modelBinderPaths.Select(Path.GetFileName))} -> FLVERs={overrideFlvers.Count}, meshes={overrideFlvers.Sum(f => f.Meshes?.Count ?? 0)}");
                }
                catch (Exception ex)
                {
                    report.AddModelError("ASSEMBLY_PROFILE_BINDER_LOAD_FAILED", ex.Message);
                    Console.Error.WriteLine($"  MODEL ASSEMBLY error: {ex.Message}");
                }
            }

            // ═══════════════════════════════════════════
            // 2. Load TEXBND → textures (separate file)
            // ═══════════════════════════════════════════
            var formalTextureBinderPaths = sections.HasFlag(ExportSections.Textures)
                ? ResolveFormalTextureBinderPaths(chrDir, chrId)
                : new List<string>();
            if (sections.HasFlag(ExportSections.Textures) && formalTextureBinderPaths.Count > 0)
            {
                foreach (string texbndPath in formalTextureBinderPaths)
                {
                    try
                    {
                        byte[] texbndData = File.ReadAllBytes(texbndPath);
                        if (DCX.Is(texbndData)) texbndData = DCX.Decompress(texbndData);
                        var texBnd = BND4.Read(texbndData);

                        CollectTpfBytesFromBinder(texBnd, tpfDataList);

                        Console.WriteLine($"  TEXBND: {Path.GetFileName(texbndPath)} -> {texBnd.Files.Count} files, {tpfDataList.Count} total TPFs");
                    }
                    catch (Exception ex)
                    {
                        Console.Error.WriteLine($"  TEXBND error ({Path.GetFileName(texbndPath)}): {ex.Message}");
                    }
                }
            }

            if (sections.HasFlag(ExportSections.Textures))
            {
                foreach (string globalTexturePath in ResolveFormalGlobalTextureTpfPaths(gameDir))
                {
                    try
                    {
                        byte[] textureData = File.ReadAllBytes(globalTexturePath);
                        if (DCX.Is(textureData))
                            textureData = DCX.Decompress(textureData);

                        if (TPF.Is(textureData))
                            tpfDataList.Add(textureData);
                    }
                    catch (Exception ex)
                    {
                        Console.Error.WriteLine($"  GLOBAL TPF error ({Path.GetFileName(globalTexturePath)}): {ex.Message}");
                    }
                }
            }

            // ═══════════════════════════════════════════
            // 3. Load ANIBND → TAE events + animations
            // ═══════════════════════════════════════════
            TAE tae = null;
            var animationFilters = ParseAnimationFilters(args);
            string animationFilter = animationFilters.Count == 1 ? animationFilters[0] : null;
            string anibndPath = Path.Combine(chrDir, $"{chrId}.anibnd.dcx");
            string overrideAnibnd = args.GetValueOrDefault("anibnd");
            if (!string.IsNullOrWhiteSpace(overrideAnibnd))
                anibndPath = ResolveGamePath(gameDir, overrideAnibnd);
            var animationAnibndPaths = ResolveFormalAnimationSourceAnibndPaths(gameDir, chrDir, chrId, anibndPath, overrideAnibnd);
            string skillAnibndPath = ResolveSkillSourceAnibndPath(gameDir, chrDir, chrId, args);
            bool hasDistinctSkillSource = !string.Equals(
                Path.GetFullPath(anibndPath),
                Path.GetFullPath(skillAnibndPath),
                StringComparison.OrdinalIgnoreCase);
            var animationRawSources = new List<(string Path, byte[] RawBytes)>();
            AnibndSourceInfo animationSourceInfo = null;
            report.SkillSourceAnibnd = Path.GetFileName(skillAnibndPath);

            if ((sections.HasFlag(ExportSections.Animations) || sections.HasFlag(ExportSections.Skills))
                && animationAnibndPaths.Count > 0)
            {
                try
                {
                    var loadedAnimationSources = new List<(string Path, AnibndSourceInfo Source)>();
                    foreach (string animationSourcePath in animationAnibndPaths)
                    {
                        var source = LoadAnibndSource(
                            animationSourcePath,
                            includeRawBytes: true,
                            includeTae: sections.HasFlag(ExportSections.Animations)
                                || (sections.HasFlag(ExportSections.Skills) && !hasDistinctSkillSource));
                        loadedAnimationSources.Add((animationSourcePath, source));
                        if (source.RawBytes != null)
                            animationRawSources.Add((animationSourcePath, source.RawBytes));
                    }

                    animationSourceInfo = MergeAnibndSources(loadedAnimationSources.Select(entry => entry.Source));
                    report.AnimationSourceAnibnd = string.Join(";", animationAnibndPaths.Select(Path.GetFileName));
                    if (!hasDistinctSkillSource)
                        tae = animationSourceInfo.Tae;

                    Console.WriteLine($"  ANIBND SOURCES: {string.Join(", ", animationAnibndPaths.Select(Path.GetFileName))}");
                    Console.WriteLine($"  ANIBND MERGED: TAE={animationSourceInfo.Tae != null} (anims={animationSourceInfo.Tae?.Animations?.Count}), HKX anims={animationSourceInfo.HkxCount}, Compendium={animationSourceInfo.HasCompendium}");
                }
                catch (Exception ex)
                {
                    report.AddAnimationError("ANIMATION_SOURCE_LOAD_FAILED", ex.Message);
                    Console.Error.WriteLine($"  ANIBND error: {ex.Message}");
                }
            }

            if (sections.HasFlag(ExportSections.Skills) && hasDistinctSkillSource)
            {
                Console.WriteLine($"  SKILL SOURCE: {Path.GetFileName(skillAnibndPath)}");
                report.SkillSourceAnibnd = Path.GetFileName(skillAnibndPath);

                if (animationFilters.Count != 1)
                {
                    report.AddSkillError("SKILL_SOURCE_REQUIRES_SINGLE_ANIM_FILTER", "Formal skill export with an independent '--skill-anibnd' requires exactly one '--anim <id>' request.");
                }
                else if (!File.Exists(skillAnibndPath))
                {
                    report.AddSkillError("SKILL_SOURCE_NOT_FOUND", $"Formal skill ANIBND was not found: {skillAnibndPath}");
                }
                else
                {
                    try
                    {
                        var skillSource = LoadAnibndSource(skillAnibndPath, includeRawBytes: false, includeTae: true);
                        tae = FilterTaeForFormalSkillExport(skillSource.Tae, animationFilter, skillAnibndPath);
                        Console.WriteLine($"  SKILL ANIBND: TAE={tae != null} (anims={tae?.Animations?.Count}), HKX anims={skillSource.HkxCount}, Compendium={skillSource.HasCompendium}");
                    }
                    catch (Exception ex)
                    {
                        report.AddSkillError("SKILL_SOURCE_LOAD_FAILED", ex.Message);
                        Console.Error.WriteLine($"  ✗ Skill source error: {ex.Message}");
                    }
                }
            }
            else if (sections.HasFlag(ExportSections.Skills) && !string.IsNullOrWhiteSpace(animationFilter))
            {
                report.SkillSourceAnibnd = Path.GetFileName(skillAnibndPath);
                try
                {
                    tae = FilterTaeForFormalSkillExport(tae, animationFilter, anibndPath);
                }
                catch (Exception ex)
                {
                    report.AddSkillError("SKILL_FILTER_FAILED", ex.Message);
                    Console.Error.WriteLine($"  ✗ Skill filter error: {ex.Message}");
                }
            }

            var formalAnimationResolutions = BuildFormalAnimationResolutions(
                tae,
                animationFilters,
                animationSourceInfo,
                anibndPath,
                skillAnibndPath,
                report);
            report.AnimationResolutions.AddRange(formalAnimationResolutions);

            // ═══════════════════════════════════════════
            // 4. Export model (glTF 2.0)
            // ═══════════════════════════════════════════
            var modelFlvers = overrideFlvers?.Count > 0 ? overrideFlvers : (flver != null ? new List<FLVER2> { flver } : null);
            if (sections.HasFlag(ExportSections.Model)
                && modelFlvers != null && modelFlvers.Any(f => f?.Meshes != null && f.Meshes.Count > 0))
            {
                Directory.CreateDirectory(modelDir);
                string modelPath = Path.Combine(modelDir, $"{chrId}.gltf");
                try
                {
                    var fbxExporter = new FlverToFbxExporter(new FlverToFbxExporter.ExportOptions
                    {
                        ExportFormatId = "gltf2",
                    });
                    if (modelFlvers.Count > 1 || overrideFlvers?.Count > 0)
                        fbxExporter.Export(modelFlvers, skeletonHkx, modelPath);
                    else
                        fbxExporter.Export(modelFlvers[0], modelPath);
                    string actualModelFile = FindExportedFile(modelDir, chrId, new[] { ".gltf" });
                    if (actualModelFile != null)
                    {
                        report.ModelSucceeded = true;
                        report.ModelFileName = Path.GetFileName(actualModelFile);
                        report.FormalSkeletonRoot = ExtractFormalSkeletonRoot(actualModelFile);
                        var modelDeliverable = assetPackage.GetOrAdd("model");
                        modelDeliverable.Status = "ready";
                        modelDeliverable.Format = "gltf2";
                        modelDeliverable.RelativePath = Path.Combine("Model", Path.GetFileName(actualModelFile)).Replace('\\', '/');
                        modelDeliverable.FileCount = 1;
                        modelDeliverable.Files.Add(modelDeliverable.RelativePath);
                        Console.WriteLine($"  ✓ Model: {Path.GetFileName(actualModelFile)} ({new FileInfo(actualModelFile).Length / 1024}KB)");
                    }
                    else
                    {
                        string message = "No formal model deliverable produced (.gltf).";
                        report.AddModelError("MODEL_DELIVERABLE_MISSING", message);
                        assetPackage.GetOrAdd("model").FailureReasons.Add(message);
                        Console.Error.WriteLine($"  ✗ Model: {message}");
                    }
                }
                catch (Exception ex)
                {
                    report.AddModelError("MODEL_EXPORT_FAILED", ex.ToString());
                    assetPackage.GetOrAdd("model").FailureReasons.Add(ex.ToString());
                    Console.Error.WriteLine($"  ✗ Model export error: {ex}");
                }

                // Export material manifest
                try
                {
                    var matExporter = new MaterialManifestExporter();
                    string matPath = Path.Combine(modelDir, "material_manifest.json");
                    matExporter.ExportToFile(modelFlvers, matPath);
                    report.MaterialManifestSucceeded = true;
                    var materialDeliverable = assetPackage.GetOrAdd("materialManifest");
                    materialDeliverable.Status = "ready";
                    materialDeliverable.Format = "json";
                    materialDeliverable.RelativePath = Path.Combine("Model", Path.GetFileName(matPath)).Replace('\\', '/');
                    materialDeliverable.FileCount = 1;
                    materialDeliverable.Files.Add(materialDeliverable.RelativePath);
                    Console.WriteLine($"  ✓ Material manifest exported");
                }
                catch (Exception ex)
                {
                    report.AddModelError("MATERIAL_MANIFEST_EXPORT_FAILED", ex.ToString());
                    assetPackage.GetOrAdd("materialManifest").FailureReasons.Add(ex.ToString());
                    Console.Error.WriteLine($"  ✗ Material manifest error: {ex}");
                }
            }
            else if (modelFlvers != null && modelFlvers.Count > 0)
            {
                Console.WriteLine($"  - Model: skipped (0 meshes, parts may be in separate files)");
            }

            // ═══════════════════════════════════════════
            // 5. Export textures
            // ═══════════════════════════════════════════
            if (sections.HasFlag(ExportSections.Textures) && modelFlvers.Count > 0)
                AugmentTextureSourcesForFormalMaterialBindings(gameDir, modelFlvers, tpfDataList);

            if (sections.HasFlag(ExportSections.Textures) && tpfDataList.Count > 0)
            {
                Directory.CreateDirectory(texDir);
                var texExporter = new TextureExporter(new TextureExporter.ExportOptions
                {
                    Format = TextureExporter.ExportFormat.PNG,
                    SkipUnsupported = false,
                });

                int exportedTpfCount = 0;
                foreach (var tpfData in tpfDataList)
                {
                    try
                    {
                        var tpf = TPF.Read(tpfData);
                        texExporter.ExportTpf(tpf, texDir, recordCallback: record =>
                        {
                            report.TextureEntries.Add(new JObject
                            {
                                ["textureName"] = record.TextureName ?? string.Empty,
                                ["sourceContainer"] = record.SourceContainer ?? string.Empty,
                                ["sourceFormat"] = record.SourceFormat ?? string.Empty,
                                ["decodedPixelFormat"] = record.DecodedPixelFormat ?? string.Empty,
                                ["outputFileName"] = record.OutputFileName ?? string.Empty,
                                ["relativePath"] = record.RelativePath ?? string.Empty,
                                ["success"] = record.Success,
                                ["failureCode"] = record.FailureCode ?? string.Empty,
                                ["failureMessage"] = record.FailureMessage ?? string.Empty,
                            });
                        });
                        exportedTpfCount++;
                    }
                    catch (Exception ex)
                    {
                        report.AddTextureError("TEXTURE_EXPORT_FAILED", ex.Message);
                        Console.Error.WriteLine($"  ✗ TPF export error: {ex.Message}");
                        break;
                    }
                }

                int texCount = Directory.GetFiles(texDir, "*.png").Length;
                report.TextureCount = texCount;
                report.TexturesSucceeded = exportedTpfCount == tpfDataList.Count && texCount > 0 && report.TextureErrorCount == 0;
                var textureDeliverable = assetPackage.GetOrAdd("textures");
                textureDeliverable.Format = "png";
                textureDeliverable.FileCount = texCount;
                foreach (string texturePath in Directory.GetFiles(texDir, "*.png").OrderBy(path => path, StringComparer.OrdinalIgnoreCase))
                    textureDeliverable.Files.Add(Path.Combine("Textures", Path.GetFileName(texturePath)).Replace('\\', '/'));
                if (report.TexturesSucceeded)
                {
                    textureDeliverable.Status = "ready";
                    textureDeliverable.RelativePath = "Textures";
                }
                if (report.TexturesSucceeded)
                    Console.WriteLine($"  ✓ Textures: {texCount} PNG files exported");
            }
            else if (sections.HasFlag(ExportSections.Textures))
            {
                report.AddTextureError("TEXTURE_SOURCE_MISSING", "No formal texture sources were found for this character.");
                assetPackage.GetOrAdd("textures").FailureReasons.Add("No formal texture sources were found for this character.");
            }

            // ═══════════════════════════════════════════
            // 6. Export animations from ANIBND
            // ═══════════════════════════════════════════
            if (sections.HasFlag(ExportSections.Animations) && animationRawSources.Count > 0 && skeletonHkx != null)
            {
                Directory.CreateDirectory(animDir);
                var animExporter = new AnimationToFbxExporter(new AnimationToFbxExporter.ExportOptions
                {
                    ExportFormatId = "gltf2",
                });

                try
                {
                    string animationExportFilter = ResolveAnimationExportFilter(animationFilters, formalAnimationResolutions);
                    var animationRawBytesByPath = animationRawSources
                        .GroupBy(entry => entry.Path, StringComparer.OrdinalIgnoreCase)
                        .ToDictionary(group => group.Key, group => group.First().RawBytes, StringComparer.OrdinalIgnoreCase);
                    var animationResolutionsBySourceAndStem = formalAnimationResolutions
                        .Where(resolution => resolution != null
                            && !string.IsNullOrWhiteSpace(resolution.SourceAnimStem)
                            && !string.IsNullOrWhiteSpace(resolution.AnimationSourcePath))
                        .GroupBy(
                            resolution => $"{resolution.AnimationSourcePath}\n{resolution.SourceAnimStem}",
                            StringComparer.OrdinalIgnoreCase)
                        .ToDictionary(group => group.Key, group => group.ToList(), StringComparer.OrdinalIgnoreCase);
                    var exportPlan = formalAnimationResolutions
                        .Where(resolution => resolution != null
                            && !string.IsNullOrWhiteSpace(resolution.SourceAnimStem)
                            && !string.IsNullOrWhiteSpace(resolution.AnimationSourcePath))
                        .GroupBy(resolution => resolution.AnimationSourcePath, StringComparer.OrdinalIgnoreCase)
                        .Select(group => new
                        {
                            SourcePath = group.Key,
                            Stems = group.Select(resolution => resolution.SourceAnimStem)
                                .Distinct(StringComparer.OrdinalIgnoreCase)
                                .ToList(),
                        })
                        .ToList();

                    if (exportPlan.Count == 0 && !string.IsNullOrWhiteSpace(animationExportFilter) && animationRawSources.Count > 0)
                    {
                        exportPlan = animationRawSources
                            .Select(source => new
                            {
                                SourcePath = source.Path,
                                Stems = new List<string> { animationExportFilter },
                            })
                            .ToList();
                    }

                    foreach (var sourcePlan in exportPlan)
                    {
                        if (!animationRawBytesByPath.TryGetValue(sourcePlan.SourcePath, out byte[] sourceRawBytes))
                            throw new InvalidOperationException($"Formal animation export could not find raw bytes for source ANIBND '{Path.GetFileName(sourcePlan.SourcePath)}'.");

                        Console.WriteLine($"  EXPORT ANIBND: {Path.GetFileName(sourcePlan.SourcePath)} [{sourcePlan.Stems.Count} clips]");
                        animExporter.ExportAnibnd(sourceRawBytes, skeletonHkx, animDir, modelFlvers, animationFilter: null, allowedAnimationNames: sourcePlan.Stems,
                            (name, cur, total) =>
                            {
                                if (cur % 20 == 0 || cur == total)
                                    Console.Write($"\r  Animations ({Path.GetFileName(sourcePlan.SourcePath)}): {cur}/{total}...");
                            },
                            exportRecord =>
                            {
                                if (exportRecord == null || string.IsNullOrWhiteSpace(exportRecord.AnimationName))
                                    return;

                                string resolutionKey = $"{sourcePlan.SourcePath}\n{exportRecord.AnimationName}";
                                if (animationResolutionsBySourceAndStem.TryGetValue(resolutionKey, out var matchingResolutions))
                                {
                                    foreach (var resolution in matchingResolutions)
                                    {
                                        resolution.RootMotionSourcePresent = exportRecord.RootMotionSourcePresent;
                                        resolution.RootMotion = exportRecord.RootMotion;
                                    }
                                }
                            });
                        Console.WriteLine();
                    }

                    var exportedAnimationFiles = Directory.GetFiles(animDir, "*.gltf")
                        .Select(Path.GetFileName)
                        .Where(fileName => !string.IsNullOrWhiteSpace(fileName))
                        .Distinct(StringComparer.OrdinalIgnoreCase)
                        .OrderBy(fileName => fileName, StringComparer.OrdinalIgnoreCase)
                        .ToList();
                    var expectedAnimationFiles = formalAnimationResolutions
                        .Select(resolution => resolution?.DeliverableAnimFileName)
                        .Where(fileName => !string.IsNullOrWhiteSpace(fileName))
                        .Distinct(StringComparer.OrdinalIgnoreCase)
                        .OrderBy(fileName => fileName, StringComparer.OrdinalIgnoreCase)
                        .ToList();

                    if (expectedAnimationFiles.Count == 0 && exportPlan.Count > 0)
                    {
                        expectedAnimationFiles = exportPlan
                            .SelectMany(plan => plan.Stems)
                            .Where(stem => !string.IsNullOrWhiteSpace(stem))
                            .Select(stem => $"{stem}.gltf")
                            .Distinct(StringComparer.OrdinalIgnoreCase)
                            .OrderBy(fileName => fileName, StringComparer.OrdinalIgnoreCase)
                            .ToList();
                    }

                    report.ExpectedAnimationCount = expectedAnimationFiles.Count;
                    report.AnimationCount = exportedAnimationFiles.Count;
                    report.MissingAnimationFiles.Clear();
                    report.MissingAnimationFiles.AddRange(expectedAnimationFiles.Except(exportedAnimationFiles, StringComparer.OrdinalIgnoreCase));
                    report.MissingAnimationRootMotionFiles.Clear();
                    report.MissingAnimationRootMotionFiles.AddRange(formalAnimationResolutions
                        .Where(resolution => resolution != null
                            && !string.IsNullOrWhiteSpace(resolution.DeliverableAnimFileName)
                            && expectedAnimationFiles.Contains(resolution.DeliverableAnimFileName, StringComparer.OrdinalIgnoreCase)
                            && resolution.RootMotionSourcePresent
                            && resolution.RootMotion == null)
                        .Select(resolution => resolution.DeliverableAnimFileName)
                        .Distinct(StringComparer.OrdinalIgnoreCase)
                        .OrderBy(fileName => fileName, StringComparer.OrdinalIgnoreCase));

                    if (report.ExpectedAnimationCount == 0)
                    {
                        const string message = "Formal animation export did not produce an expected clip set.";
                        report.AddAnimationError("ANIMATION_EXPECTATION_EMPTY", message);
                        assetPackage.GetOrAdd("animations").FailureReasons.Add(message);
                    }

                    if (report.MissingAnimationFiles.Count > 0)
                    {
                        string message = $"Missing formal animation clips: {string.Join(", ", report.MissingAnimationFiles)}";
                        report.AddAnimationError("ANIMATION_DELIVERABLE_MISSING", message);
                        assetPackage.GetOrAdd("animations").FailureReasons.Add(message);
                    }

                    if (report.MissingAnimationRootMotionFiles.Count > 0)
                    {
                        string message = $"Missing formal root-motion tracks: {string.Join(", ", report.MissingAnimationRootMotionFiles)}";
                        report.AddAnimationError("ANIMATION_ROOT_MOTION_MISSING", message);
                        assetPackage.GetOrAdd("animations").FailureReasons.Add(message);
                    }

                    report.AnimationsSucceeded = report.ExpectedAnimationCount > 0
                        && report.MissingAnimationFiles.Count == 0
                        && report.MissingAnimationRootMotionFiles.Count == 0;
                    var animationDeliverable = assetPackage.GetOrAdd("animations");
                    animationDeliverable.Format = "gltf2";
                    animationDeliverable.FileCount = exportedAnimationFiles.Count;
                    animationDeliverable.RelativePath = "Animations";
                    animationDeliverable.Files.Clear();
                    foreach (string animationFileName in exportedAnimationFiles)
                        animationDeliverable.Files.Add(Path.Combine("Animations", animationFileName).Replace('\\', '/'));
                    if (report.AnimationsSucceeded)
                        animationDeliverable.Status = "ready";
                    else
                        animationDeliverable.Status = "failed";
                    Console.WriteLine($"  {(report.AnimationsSucceeded ? '✓' : '✗')} Animations: {exportedAnimationFiles.Count}/{report.ExpectedAnimationCount} formal clips exported");
                }
                catch (Exception ex)
                {
                    report.AddAnimationError("ANIMATION_EXPORT_FAILED", ex.Message);
                    assetPackage.GetOrAdd("animations").FailureReasons.Add(ex.Message);
                    Console.Error.WriteLine($"  ✗ Animation export error: {ex.Message}");
                }
            }
            else if (skeletonHkx == null && File.Exists(anibndPath))
            {
                report.AddAnimationError("SKELETON_PARSE_FAILED", "Skeleton HKX could not be parsed via the formal Sekiro parser.");
                assetPackage.GetOrAdd("animations").FailureReasons.Add("Skeleton HKX could not be parsed via the formal Sekiro parser.");
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

                try
                {
                    string templatePath = Path.Combine(
                        AppDomain.CurrentDomain.BaseDirectory, "Res", "TAE.Template.SDT.xml");
                    if (!File.Exists(templatePath))
                        throw new FileNotFoundException($"Formal TAE template not found: {templatePath}");

                    skillExporter.LoadTemplate(templatePath);

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
                        ResolveAnimation = anim => ResolveFormalAnimationForSkill(formalAnimationResolutions, anim),
                        ResolveEventSemanticLinks = (eventType, parameterValues) => BuildFormalEventSemanticLinks(eventType, parameterValues, formalParams),
                    });
                    report.ParamsSucceeded = exportedParams.Properties().Count() >= 4;
                    if (!report.ParamsSucceeded)
                        report.AddParamError("CANONICAL_PARAMS_INCOMPLETE", "Canonical param payload is incomplete.");
                    report.SkillsSucceeded = true;
                    report.HasCanonicalSkillConfig = true;
                    report.SkillConfigFileName = Path.GetFileName(skillConfigPath);
                    var skillDeliverable = assetPackage.GetOrAdd("skills");
                    skillDeliverable.Status = "ready";
                    skillDeliverable.Format = "json";
                    skillDeliverable.RelativePath = Path.Combine("Skills", Path.GetFileName(skillConfigPath)).Replace('\\', '/');
                    skillDeliverable.FileCount = 1;
                    skillDeliverable.Files.Add(skillDeliverable.RelativePath);
                    var paramDeliverable = assetPackage.GetOrAdd("params");
                    paramDeliverable.Status = report.ParamsSucceeded ? "ready" : "failed";
                    paramDeliverable.Format = "embedded-json";
                    if (!report.ParamsSucceeded)
                        paramDeliverable.FailureReasons.Add("Canonical param payload is incomplete.");
                    Console.WriteLine($"  ✓ Skills: {tae.Animations.Count} animations, skill_config.json exported");
                }
                catch (Exception ex)
                {
                    report.AddSkillError("SKILL_EXPORT_FAILED", ex.Message);
                    assetPackage.GetOrAdd("skills").FailureReasons.Add(ex.Message);
                    Console.Error.WriteLine($"  ✗ Skill config error: {ex.Message}");
                }
            }
            else if (sections.HasFlag(ExportSections.Skills) && report.SkillErrorCount == 0)
            {
                report.AddSkillError("SKILL_TAE_MISSING", "No TAE data was found in the selected ANIBND for formal skill export.");
                assetPackage.GetOrAdd("skills").FailureReasons.Add("No TAE data was found in the selected ANIBND for formal skill export.");
            }

            if (sections.HasFlag(ExportSections.Skills) && !report.ParamsSucceeded && report.ParamErrorCount == 0)
                report.AddParamError("PARAM_EXPORT_SKIPPED", "Canonical skill parameters were not exported because formal skill export did not complete.");

            ValidateFormalMaterialManifestAgainstTextures(outputDir, report, assetPackage);
            RecordUnexpectedArtifacts(outputDir, assetPackage);
            assetPackage.FormalSuccess = report.IsSuccessfulFormalExport;

            string assetPackagePath = Path.Combine(outputDir, "asset_package.json");
            var assetPackageDeliverable = assetPackage.GetOrAdd("assetPackage");
            assetPackageDeliverable.Status = "ready";
            assetPackageDeliverable.Format = "json";
            assetPackageDeliverable.RelativePath = "asset_package.json";
            assetPackageDeliverable.FileCount = 1;
            assetPackageDeliverable.Files.Add(assetPackageDeliverable.RelativePath);
            report.AssetPackageFileName = Path.GetFileName(assetPackagePath);

            string reportPath = Path.Combine(outputDir, "export_report.json");
            var reportDeliverable = assetPackage.GetOrAdd("report");
            reportDeliverable.Status = "ready";
            reportDeliverable.Format = "json";
            reportDeliverable.RelativePath = "export_report.json";
            reportDeliverable.FileCount = 1;
            reportDeliverable.Files.Clear();
            reportDeliverable.Files.Add(reportDeliverable.RelativePath);
            File.WriteAllText(assetPackagePath, assetPackage.ToJson().ToString(Newtonsoft.Json.Formatting.Indented));
            File.WriteAllText(reportPath, report.ToJson().ToString(Newtonsoft.Json.Formatting.Indented));
            Console.WriteLine($"  Report: {Path.GetFileName(reportPath)} {(report.IsSuccessfulFormalExport ? "OK" : "FAILED")}");

            return report;
        }

        /// <summary>
        /// Read Sekiro HKX via the formal tagfile parser.
        /// </summary>
        static HKX TryReadHkx(byte[] data, string fileName)
        {
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

            Console.Error.WriteLine($"  HKX: formal Sekiro tagfile parsing failed for {fileName} ({data.Length} bytes)");
            return null;
        }

        static bool IsFormalSkeletonHkxEntry(string fileName, string chrId)
        {
            if (string.IsNullOrWhiteSpace(fileName) || string.IsNullOrWhiteSpace(chrId))
                return false;

            string normalizedName = Path.GetFileName(fileName).ToLowerInvariant();
            if (!normalizedName.EndsWith(".hkx") && !normalizedName.EndsWith(".hkx.dcx"))
                return false;

            if (normalizedName.EndsWith(".hkxpwv") || normalizedName.EndsWith(".hkxpwv.dcx"))
                return false;

            string stem = Path.GetFileNameWithoutExtension(normalizedName);
            if (stem.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase))
                stem = Path.GetFileNameWithoutExtension(stem);

            return string.Equals(stem, chrId, StringComparison.OrdinalIgnoreCase);
        }

        static List<string> ResolveFormalTextureBinderPaths(string chrDir, string chrId)
        {
            var result = new List<string>();
            if (string.IsNullOrWhiteSpace(chrDir) || string.IsNullOrWhiteSpace(chrId))
                return result;

            void addIfExists(string path)
            {
                if (!string.IsNullOrWhiteSpace(path)
                    && File.Exists(path)
                    && !result.Contains(path, StringComparer.OrdinalIgnoreCase))
                {
                    result.Add(path);
                }
            }

            addIfExists(Path.Combine(chrDir, $"{chrId}.texbnd.dcx"));

            if (chrId.Length >= 4)
            {
                string sharedTextureChrId = $"{chrId.Substring(0, 4)}9";
                if (!string.Equals(sharedTextureChrId, chrId, StringComparison.OrdinalIgnoreCase))
                    addIfExists(Path.Combine(chrDir, $"{sharedTextureChrId}.texbnd.dcx"));
            }

            return result;
        }

        static IReadOnlyList<string> ResolveFormalGlobalTextureTpfPaths(string gameDir)
        {
            if (string.IsNullOrWhiteSpace(gameDir))
                return Array.Empty<string>();

            var result = new List<string>();

            void addIfExists(string relativePath)
            {
                string path = Path.Combine(gameDir, relativePath.Replace('/', Path.DirectorySeparatorChar));
                if (File.Exists(path) && !result.Contains(path, StringComparer.OrdinalIgnoreCase))
                    result.Add(path);
            }

            addIfExists("other/systex.tpf.dcx");
            addIfExists("other/maptex.tpf.dcx");
            addIfExists("other/decaltex.tpf.dcx");
            addIfExists("parts/common_body.tpf.dcx");

            return result;
        }

        static int RunExportModel(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            RunWithIsolatedSekiroDocumentContext(gameDir, () => ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Model));
            return 0;
        }

        static int RunExportAnims(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            RunWithIsolatedSekiroDocumentContext(gameDir, () => ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Animations));
            return 0;
        }

        static int RunExportTextures(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            RunWithIsolatedSekiroDocumentContext(gameDir, () => ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Textures));
            return 0;
        }

        static int RunExportSkills(Dictionary<string, string> args)
        {
            string gameDir = GetRequired(args, "game-dir");
            string output = GetRequired(args, "output");
            string chrId = GetRequired(args, "chr");

            string chrBndPath = FindChrBnd(gameDir, chrId);
            RunWithIsolatedSekiroDocumentContext(gameDir, () => ExportCharacter(gameDir, chrBndPath, chrId, output, args, ExportSections.Skills));
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

        static List<byte[]> ReadTpfBytesFromBinder(string binderPath)
        {
            if (!File.Exists(binderPath))
                throw new FileNotFoundException($"Model binder not found: {binderPath}");

            byte[] binderData = File.ReadAllBytes(binderPath);
            if (DCX.Is(binderData)) binderData = DCX.Decompress(binderData);
            var result = new List<byte[]>();

            IBinder bnd = null;
            if (BND4.Is(binderData))
                bnd = BND4.Read(binderData);
            else if (BND3.Is(binderData))
                bnd = BND3.Read(binderData);

            if (bnd != null)
                CollectTpfBytesFromBinder(bnd, result);

            return result;
        }

        static void CollectTpfBytesFromBinder(IBinder binder, List<byte[]> result)
        {
            if (binder == null || result == null)
                return;

            foreach (var file in binder.Files)
            {
                string name = file.Name?.ToLowerInvariant() ?? string.Empty;
                byte[] payload = file.Bytes;
                if (payload == null || payload.Length == 0)
                    continue;

                if (DCX.Is(payload))
                    payload = DCX.Decompress(payload);

                if (name.EndsWith(".tpf") || name.EndsWith(".tpf.dcx") || TPF.Is(payload))
                {
                    result.Add(payload);
                }
                else if (name.EndsWith(".tbnd") || name.EndsWith(".tbnd.dcx") || BND3.Is(payload) || BND4.Is(payload))
                {
                    IBinder tbnd = null;
                    if (BND3.Is(payload))
                        tbnd = BND3.Read(payload);
                    else if (BND4.Is(payload))
                        tbnd = BND4.Read(payload);

                    CollectTpfBytesFromBinder(tbnd, result);
                }
            }
        }

        static void AugmentTextureSourcesForFormalMaterialBindings(string gameDir, IReadOnlyList<FLVER2> modelFlvers, List<byte[]> tpfDataList)
        {
            if (string.IsNullOrWhiteSpace(gameDir) || modelFlvers == null || modelFlvers.Count == 0 || tpfDataList == null)
                return;

            var requiredTextureStems = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var flver in modelFlvers)
            {
                if (flver?.Materials == null)
                    continue;

                foreach (var material in flver.Materials)
                {
                    foreach (var binding in FormalMaterialTextureResolver.Resolve(material))
                    {
                        if (!string.IsNullOrWhiteSpace(binding?.TextureStem))
                            requiredTextureStems.Add(binding.TextureStem);
                    }
                }
            }

            if (requiredTextureStems.Count == 0)
                return;

            var availableTextureStems = CollectTextureStemsFromTpfs(tpfDataList);
            requiredTextureStems.ExceptWith(availableTextureStems);
            if (requiredTextureStems.Count == 0)
                return;

            string partsDir = Path.Combine(gameDir, "parts");
            if (!Directory.Exists(partsDir))
                return;

            var familyPrefixes = new HashSet<string>(
                requiredTextureStems
                    .Select(GetTextureFamilyPrefix)
                    .Where(prefix => !string.IsNullOrWhiteSpace(prefix)),
                StringComparer.OrdinalIgnoreCase);

            var candidateBinderPaths = new List<string>();
            foreach (string familyPrefix in familyPrefixes)
                candidateBinderPaths.AddRange(Directory.EnumerateFiles(partsDir, $"{familyPrefix}_*.partsbnd.dcx", SearchOption.TopDirectoryOnly));

            string chrDir = Path.Combine(gameDir, "chr");
            if (requiredTextureStems.Count > 0)
            {
                candidateBinderPaths.AddRange(Directory.EnumerateFiles(partsDir, "*.partsbnd.dcx", SearchOption.TopDirectoryOnly));
                if (Directory.Exists(chrDir))
                {
                    candidateBinderPaths.AddRange(Directory.EnumerateFiles(chrDir, "*.chrbnd.dcx", SearchOption.TopDirectoryOnly));
                    candidateBinderPaths.AddRange(Directory.EnumerateFiles(chrDir, "*.texbnd.dcx", SearchOption.TopDirectoryOnly));
                }
            }

            foreach (string binderPath in candidateBinderPaths.Distinct(StringComparer.OrdinalIgnoreCase))
            {
                if (requiredTextureStems.Count == 0)
                    break;

                foreach (byte[] tpfData in ReadTpfBytesFromBinder(binderPath))
                {
                    var tpfTextureStems = CollectTextureStemsFromTpf(tpfData);
                    if (tpfTextureStems.Count == 0 || !tpfTextureStems.Overlaps(requiredTextureStems))
                        continue;

                    tpfDataList.Add(tpfData);
                    requiredTextureStems.ExceptWith(tpfTextureStems);
                }
            }

            if (requiredTextureStems.Count > 0)
                Console.WriteLine($"  TEXTURE AUGMENT: unresolved formal texture stems remain: {requiredTextureStems.Count}");
            else
                Console.WriteLine("  TEXTURE AUGMENT: resolved all formal material texture stems");
        }

        static HashSet<string> CollectTextureStemsFromTpfs(IEnumerable<byte[]> tpfDataList)
        {
            var result = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (byte[] tpfData in tpfDataList ?? Enumerable.Empty<byte[]>())
                result.UnionWith(CollectTextureStemsFromTpf(tpfData));
            return result;
        }

        static HashSet<string> CollectTextureStemsFromTpf(byte[] tpfData)
        {
            var result = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            if (tpfData == null || tpfData.Length == 0)
                return result;

            try
            {
                var tpf = TPF.Read(tpfData);
                foreach (var texture in tpf.Textures)
                {
                    string stem = Utils.GetShortIngameFileName(texture?.Name);
                    if (!string.IsNullOrWhiteSpace(stem))
                        result.Add(stem.ToLowerInvariant());
                }
            }
            catch
            {
            }

            return result;
        }

        static string GetTextureFamilyPrefix(string textureStem)
        {
            if (string.IsNullOrWhiteSpace(textureStem))
                return null;

            string[] segments = textureStem.Split(new[] { '_' }, StringSplitOptions.RemoveEmptyEntries);
            if (segments.Length < 2)
                return null;

            return string.Join("_", segments.Take(2)).ToUpperInvariant();
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

        static FormalAssemblyProfile ResolveFormalAssemblyProfile(string gameDir, string chrId, Dictionary<string, string> args)
        {
            string overrideModelBinder = args.GetValueOrDefault("model-binder");
            if (!string.IsNullOrWhiteSpace(overrideModelBinder))
            {
                var overridePaths = SplitOverridePaths(overrideModelBinder)
                    .Select(path => ResolveGamePath(gameDir, path))
                    .ToList();

                return new FormalAssemblyProfile
                {
                    Id = $"{chrId}-override-binders-v1",
                    Description = "Explicit CLI override binders",
                    IsFemale = false,
                    Config = new NewChrAsmCfgJson { GameType = SoulsGames.SDT },
                    BinderPaths = overridePaths,
                };
            }

            if (chrId.Equals("c0000", StringComparison.OrdinalIgnoreCase))
            {
                var config = GetFormalSekiroExportAssemblyConfig(chrId);
                return new FormalAssemblyProfile
                {
                    Id = "sekiro-player-canonical-v1",
                    Description = "Canonical multipart Sekiro player assembly for formal export",
                    IsFemale = false,
                    Config = config,
                    BinderPaths = ResolveFormalSekiroPlayerModelBinderPaths(gameDir, config, isFemale: false).ToList(),
                };
            }

            return new FormalAssemblyProfile
            {
                Id = $"{chrId}-single-chrbnd-v1",
                Description = "Single CHRBND model without multipart override",
                IsFemale = false,
                Config = new NewChrAsmCfgJson { GameType = SoulsGames.SDT },
                BinderPaths = Array.Empty<string>(),
            };
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
            WithRawParamFromBinder(paramBinder, "EquipParamWeapon", param =>
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
            });
        }

        static void ReadFormalProtectorRows(BND4 paramBinder, Dictionary<int, FormalProtectorRow> destination)
        {
            WithRawParamFromBinder(paramBinder, "EquipParamProtector", param =>
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
            });
        }

        sealed class AnibndSourceInfo
        {
            public string SourcePath { get; init; }
            public byte[] RawBytes { get; init; }
            public TAE Tae { get; init; }
            public int HkxCount { get; init; }
            public bool HasCompendium { get; init; }
            public IReadOnlySet<string> AnimationFileStems { get; init; }
            public IReadOnlyDictionary<long, string> AnimationStemById { get; init; }
            public IReadOnlyDictionary<long, string> AnimationSourcePathById { get; init; }
        }

        private const int FormalLongAnimIdMod = 1_000000;
        private const int FormalAnimationBindIdMod = 1_000_000_000;

        static List<string> ResolveFormalAnimationSourceAnibndPaths(string gameDir, string chrDir, string chrId, string baseAnibndPath, string overrideAnibnd)
        {
            var resolvedPaths = new List<string>();

            if (string.IsNullOrWhiteSpace(baseAnibndPath) || !File.Exists(baseAnibndPath))
                return resolvedPaths;

            resolvedPaths.Add(baseAnibndPath);

            if (!string.IsNullOrWhiteSpace(overrideAnibnd))
                return resolvedPaths;

            foreach (string referencedAnibndPath in ResolveReferencedAdditionalAnibndPaths(gameDir, chrDir, baseAnibndPath))
            {
                if (!IsOriginalFormalAnimationAnibnd(chrId, referencedAnibndPath))
                    continue;

                if (!resolvedPaths.Contains(referencedAnibndPath, StringComparer.OrdinalIgnoreCase))
                    resolvedPaths.Add(referencedAnibndPath);
            }

            return resolvedPaths;
        }

        static bool IsOriginalFormalAnimationAnibnd(string chrId, string anibndPath)
        {
            if (string.IsNullOrWhiteSpace(chrId) || string.IsNullOrWhiteSpace(anibndPath))
                return false;

            string fileName = Path.GetFileName(anibndPath) ?? string.Empty;
            string stem = Path.GetFileNameWithoutExtension(fileName);
            if (stem.EndsWith(".anibnd", StringComparison.OrdinalIgnoreCase))
                stem = Path.GetFileNameWithoutExtension(stem);

            if (string.Equals(stem, chrId, StringComparison.OrdinalIgnoreCase)
                || string.Equals(stem, $"{chrId}_0000", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            return stem.StartsWith($"{chrId}_a", StringComparison.OrdinalIgnoreCase);
        }

        static List<string> ResolveReferencedAdditionalAnibndPaths(string gameDir, string chrDir, string baseAnibndPath)
        {
            byte[] anibndData = File.ReadAllBytes(baseAnibndPath);
            if (DCX.Is(anibndData))
                anibndData = DCX.Decompress(anibndData);

            var anibnd = BND4.Read(anibndData);
            bool ver0001 = anibnd.Files.Any(file => file.ID == 9999999);
            int additionalAnibndBindIdStart = ver0001 ? 6000000 : 4000000;
            var result = new List<string>();

            foreach (var file in anibnd.Files)
            {
                bool isAdditionalReference = (file.ID >= additionalAnibndBindIdStart && file.ID <= additionalAnibndBindIdStart + 99999)
                    || (ver0001 && file.ID >= 6200000 && file.ID < 6300000)
                    || (ver0001 && file.ID >= 6300000 && file.ID < 6400000);
                if (!isAdditionalReference)
                    continue;

                string shortName = Utils.GetShortIngameFileName(file.Name ?? string.Empty);
                if (string.IsNullOrWhiteSpace(shortName))
                    continue;

                if (shortName.StartsWith("delayload_", StringComparison.OrdinalIgnoreCase))
                    shortName = shortName.Substring("delayload_".Length);

                string candidatePath = Path.Combine(chrDir, $"{shortName}.anibnd.dcx");
                if (!File.Exists(candidatePath))
                {
                    candidatePath = ResolveGamePath(gameDir, Path.Combine("chr", $"{shortName}.anibnd.dcx"));
                }

                if (File.Exists(candidatePath))
                    result.Add(candidatePath);
            }

            return result
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
                .ToList();
        }

        static AnibndSourceInfo MergeAnibndSources(IEnumerable<AnibndSourceInfo> sources)
        {
            TAE mergedTae = null;
            int hkxCount = 0;
            bool hasCompendium = false;
            var animationFileStems = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var animationStemById = new Dictionary<long, string>();
            var animationSourcePathById = new Dictionary<long, string>();

            foreach (var source in sources ?? Enumerable.Empty<AnibndSourceInfo>())
            {
                if (source == null)
                    continue;

                mergedTae = MergeFormalTaeSources(mergedTae, source.Tae);
                hkxCount += source.HkxCount;
                hasCompendium |= source.HasCompendium;

                if (source.AnimationFileStems != null)
                {
                    foreach (string stem in source.AnimationFileStems)
                        animationFileStems.Add(stem);
                }

                if (source.AnimationStemById != null)
                {
                    foreach (var kvp in source.AnimationStemById)
                        animationStemById[kvp.Key] = kvp.Value;
                }

                if (source.AnimationSourcePathById != null)
                {
                    foreach (var kvp in source.AnimationSourcePathById)
                        animationSourcePathById[kvp.Key] = kvp.Value;
                }
            }

            return new AnibndSourceInfo
            {
                SourcePath = null,
                RawBytes = null,
                Tae = mergedTae,
                HkxCount = hkxCount,
                HasCompendium = hasCompendium,
                AnimationFileStems = animationFileStems,
                AnimationStemById = animationStemById,
                AnimationSourcePathById = animationSourcePathById,
            };
        }

        static string ResolveSkillSourceAnibndPath(string gameDir, string chrDir, string chrId, Dictionary<string, string> args)
        {
            string overrideSkillAnibnd = args.GetValueOrDefault("skill-anibnd");
            if (!string.IsNullOrWhiteSpace(overrideSkillAnibnd))
                return ResolveGamePath(gameDir, overrideSkillAnibnd);

            return Path.Combine(chrDir, $"{chrId}.anibnd.dcx");
        }

        static AnibndSourceInfo LoadAnibndSource(string anibndPath, bool includeRawBytes, bool includeTae)
        {
            byte[] anibndData = File.ReadAllBytes(anibndPath);
            if (DCX.Is(anibndData))
                anibndData = DCX.Decompress(anibndData);

            var aniBnd = BND4.Read(anibndData);
            TAE tae = null;
            int hkxCount = 0;
            bool hasCompendium = false;
            var animationFileStems = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var animationStemById = new Dictionary<long, string>();
            var animationSourcePathById = new Dictionary<long, string>();
            bool isPlayerAnibnd = IsFormalPlayerAnibnd(anibndPath);
            int taeRootBindId = ResolveFormalTaeRootBindId(aniBnd);
            int taeBindIdMax = taeRootBindId + 99999;

            foreach (var file in aniBnd.Files)
            {
                string name = file.Name?.ToLowerInvariant() ?? string.Empty;

                if (includeTae && IsFormalTaeBinderEntry(file, isPlayerAnibnd, taeRootBindId, taeBindIdMax))
                {
                    byte[] taeBytes = file.Bytes;
                    if (DCX.Is(taeBytes))
                        taeBytes = DCX.Decompress(taeBytes);
                    var taeFile = TAE.Read(taeBytes);
                    if (isPlayerAnibnd)
                    {
                        int taeBindIndex = file.ID % taeRootBindId;
                        NormalizePlayerTaeAnimationIds(taeFile, taeBindIndex);
                    }

                    tae = MergeFormalTaeSources(tae, taeFile);
                }
                else if (name.EndsWith(".hkx") || name.EndsWith(".hkx.dcx"))
                {
                    hkxCount++;
                    string stem = NormalizeAnimationReference(file.Name ?? string.Empty);
                    if (!string.IsNullOrWhiteSpace(stem))
                    {
                        animationFileStems.Add(stem);
                        long fullAnimationId = file.ID % FormalAnimationBindIdMod;
                        if (fullAnimationId >= 0)
                        {
                            animationStemById[fullAnimationId] = stem;
                            animationSourcePathById[fullAnimationId] = anibndPath;
                        }
                    }
                }

                if (file.ID == 7000000)
                    hasCompendium = true;
            }

            return new AnibndSourceInfo
            {
                SourcePath = anibndPath,
                RawBytes = includeRawBytes ? anibndData : null,
                Tae = tae,
                HkxCount = hkxCount,
                HasCompendium = hasCompendium,
                AnimationFileStems = animationFileStems,
                AnimationStemById = animationStemById,
                AnimationSourcePathById = animationSourcePathById,
            };
        }

        static bool IsFormalPlayerAnibnd(string anibndPath)
        {
            if (string.IsNullOrWhiteSpace(anibndPath))
                return false;

            string fileName = Path.GetFileName(anibndPath) ?? string.Empty;
            string stem = Path.GetFileNameWithoutExtension(fileName);
            if (stem.EndsWith(".anibnd", StringComparison.OrdinalIgnoreCase))
                stem = Path.GetFileNameWithoutExtension(stem);

            return string.Equals(stem, "c0000", StringComparison.OrdinalIgnoreCase)
                || string.Equals(stem, "c0000_0000", StringComparison.OrdinalIgnoreCase);
        }

        static int ResolveFormalTaeRootBindId(IBinder anibnd)
        {
            bool ver0001 = anibnd.Files.Any(file => file.ID == 9999999);
            return ver0001 ? 5000000 : 3000000;
        }

        static bool IsFormalTaeBinderEntry(BinderFile file, bool isPlayerAnibnd, int taeRootBindId, int taeBindIdMax)
        {
            if (file == null)
                return false;

            string name = file.Name?.ToLowerInvariant() ?? string.Empty;
            bool hasTaeExtension = name.EndsWith(".tae") || name.EndsWith(".tae.dcx");
            if (!hasTaeExtension)
                return false;

            if (!isPlayerAnibnd)
                return true;

            return file.ID >= taeRootBindId && file.ID <= taeBindIdMax;
        }

        static void NormalizePlayerTaeAnimationIds(TAE tae, int taeBindIndex)
        {
            if (tae?.Animations == null || tae.Animations.Count == 0)
                return;

            int upperId = taeBindIndex * FormalLongAnimIdMod;
            foreach (var animation in tae.Animations)
            {
                if (animation == null)
                    continue;

                animation.ID = upperId + (int)(animation.ID % FormalLongAnimIdMod);
            }
        }

        static List<FormalAnimationResolution> BuildFormalAnimationResolutions(
            TAE tae,
            IReadOnlyCollection<string> animationFilters,
            AnibndSourceInfo animationSource,
            string animationSourcePath,
            string skillSourcePath,
            ExportCharacterReport report)
        {
            var results = new List<FormalAnimationResolution>();
            var requestedAnimationNames = new HashSet<string>(ParseAnimationFilters(animationFilters), StringComparer.OrdinalIgnoreCase);
            var requestedAnimationIds = new HashSet<long>(requestedAnimationNames.Select(ParseAnimationKeyToId).Where(id => id >= 0));

            if (tae?.Animations != null && tae.Animations.Count > 0)
            {
                var animationIndex = BuildFormalAnimationIndex(tae.Animations, "formal animation resolution");
                foreach (var animation in tae.Animations)
                {
                    if (requestedAnimationNames.Count > 0)
                    {
                        string requestName = FormatAnimationKey(animation.ID);
                        if (!requestedAnimationNames.Contains(requestName) && !requestedAnimationIds.Contains(animation.ID))
                            continue;
                    }

                    try
                    {
                        results.Add(ResolveFormalAnimation(animation, animationIndex, animationSource, animationSourcePath, skillSourcePath));
                    }
                    catch (Exception ex)
                    {
                        report.AddAnimationError("ANIMATION_RESOLUTION_FAILED", ex.Message);
                    }
                }

                return FilterFormalAnimationResolutionsToOriginalContent(results);
            }

            if (requestedAnimationNames.Count > 0)
            {
                foreach (string normalizedFilter in requestedAnimationNames)
                {
                    long parsedId = ParseAnimationKeyToId(normalizedFilter);
                    results.Add(new FormalAnimationResolution
                    {
                        RequestTaeId = parsedId,
                        RequestTaeName = normalizedFilter,
                        ResolvedTaeId = parsedId,
                        ResolvedTaeName = normalizedFilter,
                        ResolvedHkxId = parsedId,
                        ResolvedHkxName = normalizedFilter,
                        SourceAnimFileName = normalizedFilter,
                        SourceAnimStem = normalizedFilter,
                        DeliverableAnimFileName = $"{normalizedFilter}.gltf",
                        AnimationSourcePath = animationSource?.SourcePath ?? animationSourcePath,
                        AnimationSourceAnibnd = Path.GetFileName(animationSource?.SourcePath ?? animationSourcePath),
                        SkillSourceAnibnd = Path.GetFileName(skillSourcePath),
                    });
                }
            }

            return FilterFormalAnimationResolutionsToOriginalContent(results);
        }

        static List<FormalAnimationResolution> FilterFormalAnimationResolutionsToOriginalContent(
            IEnumerable<FormalAnimationResolution> resolutions)
        {
            return (resolutions ?? Enumerable.Empty<FormalAnimationResolution>())
                .Where(resolution => resolution != null
                    && !string.IsNullOrWhiteSpace(resolution.SourceAnimStem)
                    && !string.IsNullOrWhiteSpace(resolution.AnimationSourcePath))
                .GroupBy(
                    resolution => $"{resolution.AnimationSourcePath}\n{resolution.SourceAnimStem}",
                    StringComparer.OrdinalIgnoreCase)
                .Select(group => group
                    .OrderByDescending(IsOriginalFormalAnimationResolution)
                    .ThenBy(resolution => resolution.RequestTaeId != resolution.ResolvedHkxId)
                    .ThenBy(resolution => resolution.RequestTaeId)
                    .First())
                .OrderBy(resolution => resolution.AnimationSourcePath, StringComparer.OrdinalIgnoreCase)
                .ThenBy(resolution => resolution.SourceAnimStem, StringComparer.OrdinalIgnoreCase)
                .ToList();
        }

        static bool IsOriginalFormalAnimationResolution(FormalAnimationResolution resolution)
        {
            if (resolution == null)
                return false;

            return resolution.RequestTaeId == resolution.ResolvedTaeId
                && resolution.RequestTaeId == resolution.ResolvedHkxId;
        }

        static IReadOnlyDictionary<long, TAE.Animation> BuildFormalAnimationIndex(
            IEnumerable<TAE.Animation> animations,
            string context)
        {
            var result = new Dictionary<long, TAE.Animation>();
            foreach (var group in (animations ?? Enumerable.Empty<TAE.Animation>()).GroupBy(animation => animation.ID))
            {
                var candidates = group.Where(animation => animation != null).ToList();
                if (candidates.Count == 0)
                    continue;

                var distinctSignatures = candidates
                    .Select(BuildFormalAnimationSignature)
                    .Distinct(StringComparer.Ordinal)
                    .ToList();

                if (distinctSignatures.Count > 1)
                    throw new InvalidOperationException($"Duplicate TAE animation ID '{FormatAnimationKey(group.Key)}' has multiple distinct payloads in {context}.");

                result[group.Key] = candidates[0];
            }

            return result;
        }

        static string BuildFormalAnimationSignature(TAE.Animation animation)
        {
            if (animation == null)
                return string.Empty;

            string sourceFile = NormalizeAnimationReference(GetSourceAnimationFileName(animation));
            string headerKind = animation.Header?.GetType().FullName ?? string.Empty;
            string actionSignature = string.Join("|", (animation.Actions ?? new List<TAE.Action>()).Select(action =>
                $"{action.Type}:{action.StartTime:F4}:{action.EndTime:F4}:{action.ParameterBytes?.Length ?? 0}"));

            return $"{FormatAnimationKey(animation.ID)}::{headerKind}::{sourceFile}::{actionSignature}";
        }

        static FormalAnimationResolution ResolveFormalAnimation(
            TAE.Animation requestAnimation,
            IReadOnlyDictionary<long, TAE.Animation> animationIndex,
            AnibndSourceInfo animationSource,
            string animationSourcePath,
            string skillSourcePath)
        {
            var visitedIds = new HashSet<long>();
            var resolvedAnimation = ResolveTaeReferenceChain(requestAnimation, animationIndex, visitedIds);
            var resolvedHkxAnimation = ResolveHkxReferenceChain(resolvedAnimation, animationIndex, visitedIds);
            long resolvedHkxId = ResolveHkxId(resolvedHkxAnimation);
            string resolvedHkxName = FormatAnimationKey(resolvedHkxId);
            string sourceAnimFileName = GetSourceAnimationFileName(resolvedHkxAnimation);
            string sourceAnimStem = NormalizeAnimationReference(sourceAnimFileName);
            string exportAnimStem = sourceAnimStem;
            string resolvedAnimationSourcePath = animationSourcePath;
            if (animationSource?.AnimationStemById != null
                && animationSource.AnimationStemById.TryGetValue(resolvedHkxId, out string mappedAnimStem)
                && !string.IsNullOrWhiteSpace(mappedAnimStem))
            {
                exportAnimStem = mappedAnimStem;
            }

            if (animationSource?.AnimationSourcePathById != null
                && animationSource.AnimationSourcePathById.TryGetValue(resolvedHkxId, out string mappedSourcePath)
                && !string.IsNullOrWhiteSpace(mappedSourcePath))
            {
                resolvedAnimationSourcePath = mappedSourcePath;
            }

            if (string.IsNullOrWhiteSpace(sourceAnimStem))
                sourceAnimStem = resolvedHkxName;
            if (string.IsNullOrWhiteSpace(exportAnimStem))
                exportAnimStem = sourceAnimStem;
            if (string.IsNullOrWhiteSpace(exportAnimStem))
                exportAnimStem = resolvedHkxName;

            if (animationSource?.AnimationFileStems != null && animationSource.AnimationFileStems.Count > 0
                && !animationSource.AnimationFileStems.Contains(exportAnimStem))
            {
                throw new InvalidOperationException($"Formal animation resolution for '{FormatAnimationKey(requestAnimation.ID)}' resolved to '{exportAnimStem}', but animation source '{Path.GetFileName(animationSourcePath)}' does not contain that HKX file.");
            }

            return new FormalAnimationResolution
            {
                RequestTaeId = requestAnimation.ID,
                RequestTaeName = FormatAnimationKey(requestAnimation.ID),
                ResolvedTaeId = resolvedAnimation.ID,
                ResolvedTaeName = FormatAnimationKey(resolvedAnimation.ID),
                ResolvedHkxId = resolvedHkxId,
                ResolvedHkxName = resolvedHkxName,
                SourceAnimFileName = sourceAnimFileName,
                SourceAnimStem = exportAnimStem,
                DeliverableAnimFileName = $"{exportAnimStem}.gltf",
                AnimationSourcePath = resolvedAnimationSourcePath,
                AnimationSourceAnibnd = Path.GetFileName(resolvedAnimationSourcePath),
                SkillSourceAnibnd = Path.GetFileName(skillSourcePath),
            };
        }

        static TAE.Animation ResolveTaeReferenceChain(
            TAE.Animation animation,
            IReadOnlyDictionary<long, TAE.Animation> animationIndex,
            HashSet<long> visitedIds)
        {
            if (animation == null)
                throw new InvalidOperationException("Formal animation resolution received a null TAE animation.");

            if (!visitedIds.Add(animation.ID))
                throw new InvalidOperationException($"Formal animation resolution detected a TAE reference loop at '{FormatAnimationKey(animation.ID)}'.");

            if (animation.Header is TAE.Animation.AnimFileHeader.ImportOtherAnim importOtherAnim)
            {
                if (importOtherAnim.ImportFromAnimID < 0)
                    throw new InvalidOperationException($"TAE animation '{FormatAnimationKey(animation.ID)}' imports another animation but does not declare a valid source anim ID.");

                if (!animationIndex.TryGetValue(importOtherAnim.ImportFromAnimID, out var nextAnimation))
                    throw new InvalidOperationException($"TAE animation '{FormatAnimationKey(animation.ID)}' references missing animation '{FormatAnimationKey(importOtherAnim.ImportFromAnimID)}'.");

                return ResolveTaeReferenceChain(nextAnimation, animationIndex, visitedIds);
            }

            return animation;
        }

        static TAE.Animation ResolveHkxReferenceChain(
            TAE.Animation animation,
            IReadOnlyDictionary<long, TAE.Animation> animationIndex,
            HashSet<long> visitedIds)
        {
            if (animation == null)
                throw new InvalidOperationException("Formal HKX resolution received a null TAE animation.");

            if (animation.Header is TAE.Animation.AnimFileHeader.Standard standardHeader
                && standardHeader.ImportsHKX
                && standardHeader.ImportHKXSourceAnimID >= 0
                && animationIndex != null
                && animationIndex.TryGetValue(standardHeader.ImportHKXSourceAnimID, out var nextAnimation)
                && nextAnimation != null)
            {
                if (!visitedIds.Add(nextAnimation.ID))
                    throw new InvalidOperationException($"Formal HKX resolution detected a reference loop at '{FormatAnimationKey(nextAnimation.ID)}'.");

                return ResolveHkxReferenceChain(nextAnimation, animationIndex, visitedIds);
            }

            return animation;
        }

        static long ResolveHkxId(TAE.Animation animation)
        {
            if (animation?.Header is TAE.Animation.AnimFileHeader.Standard standardHeader
                && standardHeader.ImportsHKX
                && standardHeader.ImportHKXSourceAnimID >= 0)
            {
                return standardHeader.ImportHKXSourceAnimID;
            }

            return animation?.ID ?? -1;
        }

        static FormalAnimationResolution ResolveFormalAnimationForSkill(
            IReadOnlyList<FormalAnimationResolution> resolutions,
            TAE.Animation animation)
        {
            if (animation == null)
                return null;

            string requestName = FormatAnimationKey(animation.ID);
            string sourceStem = NormalizeAnimationReference(GetSourceAnimationFileName(animation));

            return resolutions.FirstOrDefault(resolution =>
                resolution.RequestTaeId == animation.ID
                || string.Equals(resolution.RequestTaeName, requestName, StringComparison.OrdinalIgnoreCase)
                || (!string.IsNullOrWhiteSpace(sourceStem) && string.Equals(resolution.SourceAnimStem, sourceStem, StringComparison.OrdinalIgnoreCase)));
        }

        static string ResolveAnimationExportFilter(IReadOnlyCollection<string> animationFilters, IReadOnlyList<FormalAnimationResolution> resolutions)
        {
            var normalizedFilters = ParseAnimationFilters(animationFilters);
            if (normalizedFilters.Count == 0)
                return null;

            if (resolutions != null && resolutions.Count == 1)
                return resolutions[0].SourceAnimStem;

            return normalizedFilters.Count == 1 ? normalizedFilters[0] : null;
        }

        static List<string> ParseAnimationFilters(Dictionary<string, string> args)
        {
            var values = new List<string>();
            if (args == null)
                return values;

            if (args.TryGetValue("anim", out string single) && !string.IsNullOrWhiteSpace(single))
                values.Add(single);
            if (args.TryGetValue("anim-list", out string multi) && !string.IsNullOrWhiteSpace(multi))
                values.Add(multi);

            return ParseAnimationFilters(values);
        }

        static List<string> ParseAnimationFilters(IEnumerable<string> values)
        {
            var result = new List<string>();
            foreach (string value in values ?? Enumerable.Empty<string>())
            {
                if (string.IsNullOrWhiteSpace(value))
                    continue;

                var split = value
                    .Split(new[] { ',', ';', '\r', '\n', '\t', ' ' }, StringSplitOptions.RemoveEmptyEntries)
                    .Select(NormalizeAnimationReference)
                    .Where(entry => !string.IsNullOrWhiteSpace(entry));
                result.AddRange(split);
            }

            return result
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToList();
        }

        static long ParseAnimationKeyToId(string animationKey)
        {
            if (string.IsNullOrWhiteSpace(animationKey))
                return -1;

            string normalized = NormalizeAnimationReference(animationKey);
            if (normalized.Length != 11 || normalized[0] != 'a' || normalized[4] != '_')
                return -1;

            if (!int.TryParse(normalized.Substring(1, 3), out int prefix))
                return -1;
            if (!int.TryParse(normalized.Substring(5, 6), out int suffix))
                return -1;

            return (prefix * 1000000L) + suffix;
        }

        static TAE MergeFormalTaeSources(TAE existing, TAE next)
        {
            if (next == null)
                return existing;
            if (existing == null)
                return next;

            existing.Animations ??= new List<TAE.Animation>();
            if (next.Animations != null)
                existing.Animations.AddRange(next.Animations);
            return existing;
        }

        static TAE FilterTaeForFormalSkillExport(TAE sourceTae, string animationFilter, string sourcePath)
        {
            if (sourceTae == null)
                throw new InvalidOperationException($"No TAE data was found in the formal skill source '{sourcePath}'.");

            if (string.IsNullOrWhiteSpace(animationFilter))
                return sourceTae;

            string normalizedFilter = NormalizeAnimationKey(animationFilter);
            var matchingAnimations = (sourceTae.Animations ?? new List<TAE.Animation>())
                .Where(animation =>
                {
                    string animationKey = FormatAnimationKey(animation.ID);
                    string sourceAnimationName = NormalizeAnimationReference(GetSourceAnimationFileName(animation));
                    return string.Equals(animationKey, normalizedFilter, StringComparison.OrdinalIgnoreCase)
                        || string.Equals(sourceAnimationName, normalizedFilter, StringComparison.OrdinalIgnoreCase);
                })
                .ToList();

            if (matchingAnimations.Count == 0)
                throw new InvalidOperationException($"Formal skill source '{Path.GetFileName(sourcePath)}' does not contain TAE animation '{normalizedFilter}'.");

            var exactKeyMatches = matchingAnimations
                .Where(animation => string.Equals(FormatAnimationKey(animation.ID), normalizedFilter, StringComparison.OrdinalIgnoreCase))
                .ToList();
            if (exactKeyMatches.Count > 0)
                matchingAnimations = exactKeyMatches;

            var distinctSignatures = matchingAnimations
                .GroupBy(BuildFormalAnimationSignature, StringComparer.Ordinal)
                .ToList();

            if (distinctSignatures.Count > 1)
                throw new InvalidOperationException($"Formal skill source '{Path.GetFileName(sourcePath)}' contains multiple distinct TAE entries for '{normalizedFilter}'.");

            matchingAnimations = new List<TAE.Animation> { distinctSignatures[0].First() };

            return new TAE
            {
                Format = sourceTae.Format,
                ID = sourceTae.ID,
                Flags1 = sourceTae.Flags1,
                Flags2 = sourceTae.Flags2,
                Flags3 = sourceTae.Flags3,
                Flags4 = sourceTae.Flags4,
                Flags5 = sourceTae.Flags5,
                Flags6 = sourceTae.Flags6,
                Flags7 = sourceTae.Flags7,
                Flags8 = sourceTae.Flags8,
                SkeletonName = sourceTae.SkeletonName,
                SibName = sourceTae.SibName,
                ActionSetVersion = sourceTae.ActionSetVersion,
                Animations = matchingAnimations,
            };
        }

        static string NormalizeAnimationKey(string animationKey)
        {
            return (animationKey ?? string.Empty).Trim().ToLowerInvariant();
        }

        static string NormalizeAnimationReference(string animationReference)
        {
            if (string.IsNullOrWhiteSpace(animationReference))
                return string.Empty;

            string normalized = Path.GetFileNameWithoutExtension(animationReference.Trim());
            if (normalized.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase)
                || normalized.EndsWith(".hkt", StringComparison.OrdinalIgnoreCase))
            {
                normalized = Path.GetFileNameWithoutExtension(normalized);
            }

            return NormalizeAnimationKey(normalized);
        }

        static string GetSourceAnimationFileName(TAE.Animation animation)
        {
            if (animation?.Header is TAE.Animation.AnimFileHeader.Standard standardHeader)
                return standardHeader.AnimFileName ?? string.Empty;

            if (animation?.Header is TAE.Animation.AnimFileHeader.ImportOtherAnim importHeader)
                return importHeader.AnimFileName ?? string.Empty;

            return string.Empty;
        }

        static string FormatAnimationKey(long animationId)
        {
            int prefix = (int)(animationId / 1000000);
            int suffix = (int)(animationId % 1000000);
            return $"a{prefix:D3}_{suffix:D6}";
        }

        static PARAM ReadParamFromBinder(BND4 paramBinder, string paramName)
        {
            return PARAM.Read(ReadRequiredBinderEntryBytes(paramBinder, paramName));
        }

        static PARAM_Hack ReadRawParamFromBinder(BND4 paramBinder, string paramName)
        {
            return PARAM_Hack.Read(ReadRequiredBinderEntryBytes(paramBinder, paramName));
        }

        static void WithRawParamFromBinder(BND4 paramBinder, string paramName, Action<PARAM_Hack> action)
        {
            if (action == null)
                throw new ArgumentNullException(nameof(action));

            var param = ReadRawParamFromBinder(paramBinder, paramName);
            try
            {
                action(param);
            }
            finally
            {
                param.DisposeRowReader();
            }
        }

        static byte[] ReadRequiredBinderEntryBytes(BND4 paramBinder, string entryName)
        {
            var binderFile = paramBinder.Files.FirstOrDefault(file =>
                file.Name != null && file.Name.IndexOf(entryName, StringComparison.OrdinalIgnoreCase) >= 0);
            if (binderFile == null)
                throw new InvalidOperationException($"Formal param '{entryName}' not found in Sekiro gameparam binder.");

            byte[] entryBytes = binderFile.Bytes;
            if (DCX.Is(entryBytes))
                entryBytes = DCX.Decompress(entryBytes);

            return entryBytes;
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

            RunWithIsolatedSekiroDocumentContext(normalizedGameDir, () =>
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
            WithRawParamFromBinder(paramBinder, paramName, param =>
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
            });
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

        static JObject BuildFormalEventSemanticLinks(int eventType, IReadOnlyDictionary<string, object> parameterValues, FormalSekiroSkillParams formalParams)
        {
            if (parameterValues == null || formalParams == null)
                return null;

            var result = new JObject
            {
                ["eventType"] = eventType,
                ["eventCategory"] = SkillConfigExporter.ClassifyEventCategory(eventType),
            };

            if (TryGetSemanticInt(parameterValues, out int dummyPolyId, "DummyPolyID", "DummypolyID", "DummyPolyId", "DummyPolyID1", "DummyPolyBladeBaseID"))
            {
                result["dummyPoly"] = new JObject
                {
                    ["rawId"] = dummyPolyId,
                    ["resolvedId"] = dummyPolyId,
                    ["resolution"] = "event-param",
                };
            }

            if (TryGetSemanticInt(parameterValues, out int bladeTipDummyPolyId, "DummyPolyBladeTipID"))
            {
                result["bladeTipDummyPoly"] = new JObject
                {
                    ["rawId"] = bladeTipDummyPolyId,
                    ["resolvedId"] = bladeTipDummyPolyId,
                    ["resolution"] = "event-param",
                };
            }

            var behaviorMatches = new JObject();
            if (TryGetSemanticInt(parameterValues, out int behaviorJudgeId, "BehaviorJudgeID", "BehaviorJudgeId"))
            {
                result["behaviorJudgeId"] = behaviorJudgeId;
                AppendBehaviorMatch(behaviorMatches, "player", formalParams.BehaviorPc.Values.FirstOrDefault(row => row.BehaviorJudgeID == behaviorJudgeId));
                AppendBehaviorMatch(behaviorMatches, "npc", formalParams.BehaviorNpc.Values.FirstOrDefault(row => row.BehaviorJudgeID == behaviorJudgeId));
            }

            if (TryGetSemanticInt(parameterValues, out int behaviorVariationId, "BehaviorVariationID", "VariationID", "BehaviorVariationId"))
            {
                result["behaviorVariationId"] = behaviorVariationId;
                AppendBehaviorMatch(behaviorMatches, "playerByVariation", formalParams.BehaviorPc.Values.FirstOrDefault(row => row.VariationID == behaviorVariationId));
                AppendBehaviorMatch(behaviorMatches, "npcByVariation", formalParams.BehaviorNpc.Values.FirstOrDefault(row => row.VariationID == behaviorVariationId));
            }

            if (behaviorMatches.HasValues)
                result["behaviorMatches"] = behaviorMatches;

            if (TryGetSemanticInt(parameterValues, out int atkParamId, "AtkParamID", "AttackParamID", "AtkID"))
                result["atkParam"] = BuildAtkParamLink(formalParams, atkParamId);

            if (TryGetSemanticInt(parameterValues, out int spEffectId, "SpEffectID", "SpEffectId", "SpecialEffectID"))
                result["spEffect"] = BuildSpEffectLink(formalParams, spEffectId);

            if (TryGetSemanticInt(parameterValues, out int equipParamWeaponId, "EquipParamWeaponID", "WeaponID", "EquipWeaponID"))
                result["equipParamWeapon"] = BuildEquipParamWeaponLink(formalParams, equipParamWeaponId);

            if (result.Properties().Count() <= 2)
                return null;

            return result;
        }

        static bool TryGetSemanticInt(IReadOnlyDictionary<string, object> parameterValues, out int value, params string[] names)
        {
            foreach (string name in names ?? Array.Empty<string>())
            {
                if (!parameterValues.TryGetValue(name, out object rawValue) || rawValue == null)
                    continue;

                switch (rawValue)
                {
                    case int intValue:
                        value = intValue;
                        return true;
                    case long longValue when longValue >= int.MinValue && longValue <= int.MaxValue:
                        value = (int)longValue;
                        return true;
                    case short shortValue:
                        value = shortValue;
                        return true;
                    case byte byteValue:
                        value = byteValue;
                        return true;
                    case string stringValue when int.TryParse(stringValue, out int parsed):
                        value = parsed;
                        return true;
                }
            }

            value = 0;
            return false;
        }

        static void AppendBehaviorMatch(JObject destination, string key, ParamData.BehaviorParam behavior)
        {
            if (destination == null || string.IsNullOrWhiteSpace(key) || behavior == null)
                return;

            destination[key] = new JObject
            {
                ["rowId"] = behavior.ID,
                ["variationId"] = behavior.VariationID,
                ["behaviorJudgeId"] = behavior.BehaviorJudgeID,
                ["refType"] = behavior.RefType.ToString(),
                ["refId"] = behavior.RefID,
                ["stamina"] = behavior.Stamina,
                ["mp"] = behavior.MP,
                ["heroPoint"] = behavior.HeroPoint,
            };
        }

        static JObject BuildAtkParamLink(FormalSekiroSkillParams formalParams, int atkParamId)
        {
            var result = new JObject { ["rowId"] = atkParamId };
            if (formalParams.AtkPc.TryGetValue(atkParamId, out var playerAtk))
                result["player"] = BuildAtkParamSummary(playerAtk);
            if (formalParams.AtkNpc.TryGetValue(atkParamId, out var npcAtk))
                result["npc"] = BuildAtkParamSummary(npcAtk);
            return result;
        }

        static JObject BuildAtkParamSummary(ParamData.AtkParam atkParam)
        {
            return new JObject
            {
                ["id"] = atkParam.ID,
                ["name"] = atkParam.Name ?? string.Empty,
                ["hitSourceType"] = atkParam.HitSourceType.ToString(),
                ["hitCount"] = atkParam.Hits?.Length ?? 0,
            };
        }

        static JObject BuildSpEffectLink(FormalSekiroSkillParams formalParams, int spEffectId)
        {
            var result = new JObject { ["rowId"] = spEffectId };
            if (formalParams.SpEffect.TryGetValue(spEffectId, out var spEffect))
            {
                result["name"] = spEffect.Name ?? string.Empty;
                result["grabityRate"] = spEffect.GrabityRate;
            }

            return result;
        }

        static JObject BuildEquipParamWeaponLink(FormalSekiroSkillParams formalParams, int equipParamWeaponId)
        {
            var result = new JObject { ["rowId"] = equipParamWeaponId };
            if (formalParams.EquipParamWeapon.TryGetValue(equipParamWeaponId, out var weapon))
            {
                result["name"] = weapon.Name ?? string.Empty;
                result["behaviorVariationId"] = weapon.BehaviorVariationID;
                result["equipModelId"] = weapon.EquipModelID;
                result["weaponMotionCategory"] = weapon.WepMotionCategory;
            }

            return result;
        }

        static void ValidateFormalMaterialManifestAgainstTextures(string outputDir, ExportCharacterReport report, FormalAssetPackageSummary assetPackage)
        {
            string manifestPath = Path.Combine(outputDir, "Model", "material_manifest.json");
            if (!File.Exists(manifestPath))
                return;

            var manifest = JObject.Parse(File.ReadAllText(manifestPath));
            foreach (JObject material in (manifest["materials"] as JArray)?.OfType<JObject>() ?? Enumerable.Empty<JObject>())
            {
                foreach (JProperty binding in (material["textureBindings"] as JObject)?.Properties() ?? Enumerable.Empty<JProperty>())
                {
                    string relativePath = (string)binding.Value?["relativePath"] ?? string.Empty;
                    if (string.IsNullOrWhiteSpace(relativePath))
                        continue;

                    string absolutePath = Path.Combine(outputDir, relativePath.Replace('/', Path.DirectorySeparatorChar));
                    if (!File.Exists(absolutePath))
                    {
                        string message = $"Material '{(string)material["name"] ?? string.Empty}' references missing formal texture '{relativePath}'.";
                        report.AddTextureError("TEXTURE_BINDING_MISSING", message);
                        assetPackage.GetOrAdd("textures").FailureReasons.Add(message);
                    }
                }
            }

            report.TexturesSucceeded = report.TexturesSucceeded && report.TextureErrorCount == 0;
            if (assetPackage.Deliverables.TryGetValue("textures", out var textureDeliverable) && report.TextureErrorCount > 0)
                textureDeliverable.Status = "failed";
        }

        static void RecordUnexpectedArtifacts(string outputDir, FormalAssetPackageSummary assetPackage)
        {
            foreach (string filePath in Directory.GetFiles(outputDir, "*.*", SearchOption.AllDirectories))
            {
                string extension = Path.GetExtension(filePath).ToLowerInvariant();
                if (extension is ".fbx" or ".dae" or ".dds" or ".glb")
                {
                    string relativePath = Path.GetRelativePath(outputDir, filePath).Replace('\\', '/');
                    assetPackage.UnexpectedArtifacts.Add(relativePath);
                }
            }
        }

        static string ResolveAnimationDeliverableFileName(string animDir, string animName)
        {
            string deliverablePath = FindExportedFile(animDir, animName, new[] { ".gltf" });
            return deliverablePath != null ? Path.GetFileName(deliverablePath) : null;
        }

        static string ExtractFormalSkeletonRoot(string gltfPath)
        {
            string json = File.ReadAllText(gltfPath);
            var root = JObject.Parse(json);
            var skins = root["skins"] as JArray;
            var nodes = root["nodes"] as JArray;
            if (skins == null || skins.Count == 0 || nodes == null)
                return string.Empty;

            int skeletonIndex = (int?)skins[0]?["skeleton"] ?? -1;
            if (skeletonIndex < 0 || skeletonIndex >= nodes.Count)
                return string.Empty;

            return (string)nodes[skeletonIndex]?["name"] ?? string.Empty;
        }

        static T RunWithIsolatedSekiroDocumentContext<T>(string interrootPath, Func<T> callback)
        {
            if (callback == null)
                throw new ArgumentNullException(nameof(callback));

            var managerType = typeof(zzz_DocumentManager);
            var currentDocumentField = managerType.GetField("_currentDocument", BindingFlags.Static | BindingFlags.NonPublic);
            var documentsField = managerType.GetField("Documents", BindingFlags.Static | BindingFlags.NonPublic);
            if (currentDocumentField == null || documentsField == null)
                throw new InvalidOperationException("Unable to establish an isolated Sekiro document context.");

            var previousCurrentDocument = currentDocumentField.GetValue(null);
            var documents = (System.Collections.IList)documentsField.GetValue(null);

            var isolatedDocument = new zzz_DocumentIns((DSAProj)null)
            {
                GameRoot = null,
            };
            isolatedDocument.GameRoot = new zzz_GameRootIns(isolatedDocument)
            {
                GameType = SoulsGames.SDT,
                InterrootPath = interrootPath,
                DefaultGameDir = interrootPath,
            };

            documents?.Add(isolatedDocument);
            currentDocumentField.SetValue(null, isolatedDocument);

            try
            {
                isolatedDocument.GameRoot.LoadMTDBND();
                return callback();
            }
            finally
            {
                currentDocumentField.SetValue(null, previousCurrentDocument);
                documents?.Remove(isolatedDocument);
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

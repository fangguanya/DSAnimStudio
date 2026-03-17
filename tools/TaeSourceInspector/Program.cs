using SoulsAssetPipeline.Animation;
using SoulsFormats;

if (args.Length < 2)
{
    Console.Error.WriteLine("Usage: TaeSourceInspector <chr-dir> <anim-key>");
    return 1;
}

string chrDir = args[0];
string targetAnimKey = NormalizeAnimationKey(args[1]);

if (!Directory.Exists(chrDir))
{
    Console.Error.WriteLine($"Character directory was not found: {chrDir}");
    return 1;
}

foreach (string anibndPath in Directory.EnumerateFiles(chrDir, "c0000*.anibnd.dcx", SearchOption.TopDirectoryOnly).OrderBy(path => path, StringComparer.OrdinalIgnoreCase))
{
    try
    {
        byte[] anibndBytes = File.ReadAllBytes(anibndPath);
        if (DCX.Is(anibndBytes))
            anibndBytes = DCX.Decompress(anibndBytes);

        var binder = BND4.Read(anibndBytes);
        bool foundMatch = false;

        foreach (var file in binder.Files)
        {
            string name = file.Name?.ToLowerInvariant() ?? string.Empty;
            if (!name.EndsWith(".tae") && !name.EndsWith(".tae.dcx"))
                continue;

            byte[] taeBytes = file.Bytes;
            if (DCX.Is(taeBytes))
                taeBytes = DCX.Decompress(taeBytes);

            var tae = TAE.Read(taeBytes);
            foreach (var animation in tae.Animations ?? new List<TAE.Animation>())
            {
                string animationKey = FormatAnimationKey(animation.ID);
                string sourceAnimFile = animation.Header?.AnimFileName ?? string.Empty;
                bool keyMatch = string.Equals(animationKey, targetAnimKey, StringComparison.OrdinalIgnoreCase);
                bool sourceFileMatch = sourceAnimFile.IndexOf(targetAnimKey, StringComparison.OrdinalIgnoreCase) >= 0;
                if (!keyMatch && !sourceFileMatch)
                    continue;

                foundMatch = true;
                string matchKind = keyMatch && sourceFileMatch ? "KEY+SOURCE" : keyMatch ? "KEY" : "SOURCE";
                Console.WriteLine($"MATCH {Path.GetFileName(anibndPath)} | Kind={matchKind} | TAE={file.Name} | Key={animationKey} | ID={animation.ID} | SourceAnimFile={sourceAnimFile}");
            }
        }

        if (!foundMatch)
            Console.WriteLine($"MISS  {Path.GetFileName(anibndPath)}");
    }
    catch (Exception ex)
    {
        Console.WriteLine($"ERROR {Path.GetFileName(anibndPath)} | {ex.Message}");
    }
}

return 0;

static string NormalizeAnimationKey(string animationKey)
{
    return (animationKey ?? string.Empty).Trim().ToLowerInvariant();
}

static string FormatAnimationKey(long animationId)
{
    int prefix = (int)(animationId / 1000000);
    int suffix = (int)(animationId % 1000000);
    return $"a{prefix:D3}_{suffix:D6}";
}
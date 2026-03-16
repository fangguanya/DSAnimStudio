using System;
using System.Collections.Generic;
using System.IO;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace SekiroExporter
{
    /// <summary>
    /// Post-processes glTF animation files to merge per-bone animations into a single animation clip.
    /// Assimp's glTF2 exporter creates one animation per bone channel, but UE5 expects
    /// a single animation containing all bone channels.
    /// </summary>
    public static class GltfAnimationMerger
    {
        /// <summary>
        /// Merge all animations in a glTF file into a single animation.
        /// </summary>
        public static bool MergeAnimations(string gltfPath, string animName = null)
        {
            if (!File.Exists(gltfPath)) return false;

            try
            {
                string json = File.ReadAllText(gltfPath);
                var root = JObject.Parse(json);

                var animations = root["animations"] as JArray;
                if (animations == null || animations.Count <= 1)
                    return true; // Nothing to merge

                // Merge all animations into one
                var mergedChannels = new JArray();
                var mergedSamplers = new JArray();
                int samplerOffset = 0;

                foreach (var anim in animations)
                {
                    var channels = anim["channels"] as JArray;
                    var samplers = anim["samplers"] as JArray;
                    if (channels == null || samplers == null) continue;

                    foreach (var channel in channels)
                    {
                        var newChannel = channel.DeepClone() as JObject;
                        // Update sampler index to account for merged offset
                        int origSamplerIdx = newChannel["sampler"].Value<int>();
                        newChannel["sampler"] = origSamplerIdx + samplerOffset;
                        mergedChannels.Add(newChannel);
                    }

                    foreach (var sampler in samplers)
                    {
                        mergedSamplers.Add(sampler.DeepClone());
                    }

                    samplerOffset += samplers.Count;
                }

                // Replace with single merged animation
                string name = animName ?? Path.GetFileNameWithoutExtension(gltfPath);
                var mergedAnim = new JObject
                {
                    ["name"] = name,
                    ["channels"] = mergedChannels,
                    ["samplers"] = mergedSamplers
                };

                root["animations"] = new JArray { mergedAnim };

                File.WriteAllText(gltfPath, root.ToString(Formatting.None));
                return true;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"    glTF merge error for {Path.GetFileName(gltfPath)}: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Process all glTF animation files in a directory.
        /// </summary>
        public static int MergeAllInDirectory(string animDir)
        {
            if (!Directory.Exists(animDir)) return 0;

            var gltfFiles = Directory.GetFiles(animDir, "*.gltf");
            int merged = 0;

            foreach (var file in gltfFiles)
            {
                string animName = Path.GetFileNameWithoutExtension(file);
                if (MergeAnimations(file, animName))
                    merged++;
            }

            return merged;
        }
    }
}

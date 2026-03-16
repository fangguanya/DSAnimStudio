using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Assimp;
using SoulsFormats;
using SoulsAssetPipeline.Animation;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Vector3 = System.Numerics.Vector3;
using Quaternion = System.Numerics.Quaternion;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports HKX animations to FBX format.
    /// Supports SplineCompressed and InterleavedUncompressed animation types.
    /// Handles root motion baking from HKADefaultAnimatedReferenceFrame.
    /// </summary>
    public class AnimationToFbxExporter
    {
        public class ExportOptions
        {
            /// <summary>Scale factor for translation keyframes</summary>
            public float ScaleFactor { get; set; } = 1.0f;

            /// <summary>Frames per second for exported animation</summary>
            public float FrameRate { get; set; } = 30.0f;

            /// <summary>If true, bake root motion into root bone keyframes</summary>
            public bool BakeRootMotion { get; set; } = true;

            /// <summary>Export format ID for Assimp (default "fbx" for UE5 compatibility; collada fallback if FBX fails)</summary>
            public string ExportFormatId { get; set; } = "fbx";
        }

        private readonly ExportOptions _options;

        public AnimationToFbxExporter(ExportOptions options = null)
        {
            _options = options ?? new ExportOptions();
        }

        /// <summary>
        /// Export an animation with its skeleton to a standalone FBX file.
        /// </summary>
        /// <param name="skeletonHkx">The HKX skeleton data</param>
        /// <param name="animData">The parsed HavokAnimationData</param>
        /// <param name="animName">Name for the animation clip</param>
        /// <param name="outputPath">Output FBX file path</param>
        public void Export(HKX skeletonHkx, HavokAnimationData animData, string animName, string outputPath)
        {
            var scene = new Scene();
            scene.RootNode = new Node("RootNode");

            // Extract skeleton from HKX
            HKX.HKASkeleton skeleton = null;
            if (skeletonHkx?.DataSection?.Objects != null)
            {
                foreach (var obj in skeletonHkx.DataSection.Objects)
                {
                    if (obj is HKX.HKASkeleton asSkel)
                    {
                        skeleton = asSkel;
                        break;
                    }
                }
            }

            if (skeleton == null)
            {
                // Debug: what types ARE in the skeleton HKX?
                var types = new List<string>();
                if (skeletonHkx?.DataSection?.Objects != null)
                    foreach (var obj in skeletonHkx.DataSection.Objects)
                        types.Add(obj.GetType().Name);
                throw new InvalidOperationException($"No HKASkeleton found in skeleton HKX. Objects: [{string.Join(", ", types)}]");
            }

            // Build skeleton node tree
            var boneNodes = BuildSkeletonFromHkx(skeleton, scene.RootNode);

            // Extract root motion reference frame if available
            HKX.HKADefaultAnimatedReferenceFrame rootMotion = null;
            if (_options.BakeRootMotion)
            {
                foreach (var obj in skeletonHkx.DataSection.Objects)
                {
                    if (obj is HKX.HKADefaultAnimatedReferenceFrame asRefFrame)
                    {
                        rootMotion = asRefFrame;
                        break;
                    }
                }
            }

            // Build animation
            var anim = BuildAnimation(skeleton, animData, animName, boneNodes, rootMotion);
            if (anim != null)
            {
                scene.Animations.Add(anim);
            }

            // Need at least one material for valid FBX
            scene.Materials.Add(new Material { Name = "DefaultMaterial" });

            // Export
            using (var ctx = new AssimpContext())
            {
                var dir = Path.GetDirectoryName(outputPath);
                if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                    Directory.CreateDirectory(dir);

                // Try formats in order: FBX → glTF2 → Collada (fallback)
                string[] formatsToTry = { "fbx", "fbxa", "gltf2", "glb2", "collada" };
                string[] extensions = { ".fbx", ".fbx", ".gltf", ".glb", ".dae" };
                bool success = false;
                for (int i = 0; i < formatsToTry.Length; i++)
                {
                    string fmt = formatsToTry[i];
                    string tryPath = Path.ChangeExtension(outputPath, extensions[i]);
                    try
                    {
                        success = ctx.ExportFile(scene, tryPath, fmt);
                        if (success)
                        {
                            // Post-process: fix absolute buffer URIs in glTF files
                            if (tryPath.EndsWith(".gltf", StringComparison.OrdinalIgnoreCase))
                                FixGltfBufferUris(tryPath);
                            break;
                        }
                    }
                    catch { }
                }
            }
        }

        /// <summary>
        /// Fix Assimp's glTF2 exporter writing absolute paths for buffer URIs.
        /// </summary>
        private static void FixGltfBufferUris(string gltfPath)
        {
            try
            {
                string json = File.ReadAllText(gltfPath);
                bool modified = false;
                var lines = json.Split('\n');
                for (int i = 0; i < lines.Length; i++)
                {
                    string line = lines[i];
                    int uriIdx = line.IndexOf("\"uri\"", StringComparison.Ordinal);
                    if (uriIdx < 0) continue;
                    int firstQuote = line.IndexOf('"', uriIdx + 5);
                    if (firstQuote < 0) continue;
                    int secondQuote = line.IndexOf('"', firstQuote + 1);
                    if (secondQuote < 0) continue;
                    string uri = line.Substring(firstQuote + 1, secondQuote - firstQuote - 1);
                    if (uri.Length > 2 && (uri[1] == ':' || uri[0] == '/'))
                    {
                        string filename = Path.GetFileName(uri.Replace('\\', '/'));
                        lines[i] = line.Substring(0, firstQuote + 1) + filename + line.Substring(secondQuote);
                        modified = true;
                    }
                }
                if (modified)
                    File.WriteAllText(gltfPath, string.Join("\n", lines));
            }
            catch { }
        }

        /// <summary>
        /// Batch export all animations from an ANIBND archive.
        /// </summary>
        public void ExportAnibnd(byte[] anibndBytes, HKX skeletonHkx, string outputDir,
            Action<string, int, int> progressCallback = null)
        {
            var bnd = BND4.Read(anibndBytes);

            // Extract compendium (file ID 7000000) needed for Sekiro tagfile animation parsing
            byte[] compendium = null;
            foreach (var file in bnd.Files)
            {
                if (file.ID == 7000000)
                {
                    compendium = file.Bytes;
                    if (DCX.Is(compendium)) compendium = DCX.Decompress(compendium);
                    break;
                }
            }

            // Collect HKX animation files
            var animFiles = bnd.Files
                .Where(f => f.Name != null &&
                    (f.Name.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase) ||
                     f.Name.EndsWith(".hkx.dcx", StringComparison.OrdinalIgnoreCase)) &&
                    f.ID != 7000000)
                .ToList();

            int total = animFiles.Count;
            int current = 0;
            int exported = 0;

            foreach (var file in animFiles)
            {
                current++;
                var name = Path.GetFileNameWithoutExtension(file.Name ?? $"anim_{file.ID}");

                try
                {
                    progressCallback?.Invoke(name, current, total);

                    // Decompress if needed
                    byte[] hkxBytes = file.Bytes;
                    if (DCX.Is(hkxBytes)) hkxBytes = DCX.Decompress(hkxBytes);

                    // Parse the animation HKX (pass compendium for Sekiro tagfile format)
                    HavokAnimationData animData = ReadAnimData(hkxBytes, skeletonHkx, compendium);
                    if (animData == null) continue;

                    // Format output filename
                    string cleanName = Path.GetFileNameWithoutExtension(name);
                    // Remove .hkx from double extensions like "a000_003000.hkx"
                    if (cleanName.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase))
                        cleanName = cleanName.Substring(0, cleanName.Length - 4);
                    string outPath = Path.Combine(outputDir, $"{cleanName}.fbx");

                    Export(skeletonHkx, animData, cleanName, outPath);
                    exported++;
                }
                catch (Exception ex)
                {
                    if (_debugLogCount < 5)
                    {
                        Console.Error.WriteLine($"    [DEBUG] Export failed for '{name}': {ex.Message}");
                        if (ex.InnerException != null)
                            Console.Error.WriteLine($"    [DEBUG]   Inner: {ex.InnerException.Message}");
                        _debugLogCount++;
                    }
                    continue;
                }
            }

            progressCallback?.Invoke("done", exported, total);
        }

        /// <summary>
        /// Read HavokAnimationData from raw HKX bytes.
        /// </summary>
        // Debug counter for diagnostic logging (limited to avoid spam)
        private int _debugLogCount = 0;

        private HavokAnimationData ReadAnimData(byte[] hkxBytes, HKX skeletonHkx, byte[] compendium = null)
        {
            HKX hkx = null;
            string parseMethod = "none";

            try
            {
                hkx = HKX.GenFakeFromTagFile(hkxBytes, compendium);
                parseMethod = "tagfile+compendium";
            }
            catch
            {
                try
                {
                    hkx = HKX.GenFakeFromTagFile(hkxBytes);
                    parseMethod = "tagfile";
                }
                catch
                {
                    try
                    {
                        hkx = HKX.Read(hkxBytes, HKX.HKXVariation.HKXDS1, isDS1RAnimHotfix: true);
                        parseMethod = "HKXDS1";
                    }
                    catch
                    {
                        if (_debugLogCount < 3)
                        {
                            Console.Error.WriteLine($"    [DEBUG] All HKX parse methods failed for anim ({hkxBytes.Length} bytes)");
                            _debugLogCount++;
                        }
                        return null;
                    }
                }
            }

            if (hkx?.DataSection?.Objects == null)
            {
                if (_debugLogCount < 3)
                {
                    Console.Error.WriteLine($"    [DEBUG] HKX parsed via {parseMethod} but DataSection.Objects is null");
                    _debugLogCount++;
                }
                return null;
            }

            // Debug: log what object types are in the DataSection
            if (_debugLogCount < 3)
            {
                var types = new List<string>();
                foreach (var obj in hkx.DataSection.Objects)
                    types.Add(obj.GetType().Name);
                Console.Error.WriteLine($"    [DEBUG] HKX ({parseMethod}): {hkx.DataSection.Objects.Count} objects: [{string.Join(", ", types.Take(10))}]");
                _debugLogCount++;
            }

            foreach (var obj in hkx.DataSection.Objects)
            {
                if (obj is HKX.HKASplineCompressedAnimation spline)
                    return new HavokAnimationData_SplineCompressed(0, "", spline);
                if (obj is HKX.HKAInterleavedUncompressedAnimation interleaved)
                    return new HavokAnimationData_InterleavedUncompressed(0, "", interleaved);
            }

            return null;
        }

        /// <summary>
        /// Build skeleton node hierarchy from HKX skeleton data.
        /// </summary>
        private List<Node> BuildSkeletonFromHkx(HKX.HKASkeleton skeleton, Node rootNode)
        {
            var armatureNode = new Node("Armature", rootNode);
            rootNode.Children.Add(armatureNode);

            var boneNodes = new List<Node>();
            float scale = _options.ScaleFactor;

            for (int i = 0; i < skeleton.Bones.Size; i++)
            {
                var boneName = skeleton.Bones[i].Name.GetString();
                var boneNode = new Node(boneName ?? $"Bone_{i}");

                // Get local transform from skeleton
                var transform = skeleton.Transforms[i];
                var pos = transform.Position.Vector;
                var rot = transform.Rotation.Vector;
                var scl = transform.Scale.Vector;

                var translation = new Vector3(pos.X * scale, pos.Y * scale, pos.Z * scale);
                var rotation = new System.Numerics.Quaternion(rot.X, rot.Y, rot.Z, rot.W);
                var boneScale = new Vector3(scl.X, scl.Y, scl.Z);

                var localMatrix = Matrix4x4.CreateScale(boneScale)
                    * Matrix4x4.CreateFromQuaternion(rotation)
                    * Matrix4x4.CreateTranslation(translation);

                boneNode.Transform = ToAssimpMatrix(localMatrix);
                boneNodes.Add(boneNode);
            }

            // Build hierarchy
            for (int i = 0; i < skeleton.Bones.Size; i++)
            {
                short parentIdx = skeleton.ParentIndices[i].data;
                if (parentIdx >= 0 && parentIdx < boneNodes.Count)
                {
                    boneNodes[parentIdx].Children.Add(boneNodes[i]);
                }
                else
                {
                    armatureNode.Children.Add(boneNodes[i]);
                }
            }

            return boneNodes;
        }

        /// <summary>
        /// Build an Assimp animation from HavokAnimationData.
        /// Samples every frame for each bone track.
        /// </summary>
        private Animation BuildAnimation(HKX.HKASkeleton skeleton,
            HavokAnimationData animData, string animName,
            List<Node> boneNodes, HKX.HKADefaultAnimatedReferenceFrame rootMotion)
        {
            var anim = new Animation();
            anim.Name = animName;
            anim.TicksPerSecond = _options.FrameRate;

            int frameCount = animData.FrameCount;
            if (frameCount <= 0) frameCount = 1;

            anim.DurationInTicks = frameCount;

            float scale = _options.ScaleFactor;

            // Create a channel for each bone
            for (int boneIdx = 0; boneIdx < skeleton.Bones.Size; boneIdx++)
            {
                var boneName = skeleton.Bones[boneIdx].Name.GetString();
                if (string.IsNullOrEmpty(boneName)) boneName = $"Bone_{boneIdx}";

                var channel = new NodeAnimationChannel();
                channel.NodeName = boneName;

                // Sample each frame
                for (int frame = 0; frame <= frameCount; frame++)
                {
                    float time = frame;

                    try
                    {
                        var transform = animData.GetTransformOnFrameByBone(boneIdx, frame, false);

                        var translation = new Vector3(
                            transform.Translation.X * scale,
                            transform.Translation.Y * scale,
                            transform.Translation.Z * scale);

                        // Bake root motion into root bone (bone index 0)
                        if (boneIdx == 0 && rootMotion != null && _options.BakeRootMotion)
                        {
                            var rootMotionTransform = GetRootMotionAtFrame(rootMotion, frame, frameCount);
                            translation.X += rootMotionTransform.X * scale;
                            translation.Y += rootMotionTransform.Y * scale;
                            translation.Z += rootMotionTransform.Z * scale;
                        }

                        channel.PositionKeys.Add(new VectorKey(time, new Vector3D(
                            translation.X, translation.Y, translation.Z)));

                        channel.RotationKeys.Add(new QuaternionKey(time, new Assimp.Quaternion(
                            transform.Rotation.W, transform.Rotation.X,
                            transform.Rotation.Y, transform.Rotation.Z)));

                        channel.ScalingKeys.Add(new VectorKey(time, new Vector3D(
                            transform.Scale.X, transform.Scale.Y, transform.Scale.Z)));
                    }
                    catch
                    {
                        // If frame sampling fails, use identity
                        channel.PositionKeys.Add(new VectorKey(time, new Vector3D(0, 0, 0)));
                        channel.RotationKeys.Add(new QuaternionKey(time, new Assimp.Quaternion(1, 0, 0, 0)));
                        channel.ScalingKeys.Add(new VectorKey(time, new Vector3D(1, 1, 1)));
                    }
                }

                anim.NodeAnimationChannels.Add(channel);
            }

            return anim;
        }

        /// <summary>
        /// Get root motion translation at a specific frame from the reference frame data.
        /// </summary>
        private Vector3 GetRootMotionAtFrame(HKX.HKADefaultAnimatedReferenceFrame refFrame,
            int frame, int totalFrames)
        {
            if (refFrame.ReferenceFrameSamples == null || refFrame.ReferenceFrameSamples.Size == 0)
                return Vector3.Zero;

            int sampleCount = (int)refFrame.ReferenceFrameSamples.Size;
            if (sampleCount == 0) return Vector3.Zero;

            // Accumulate root motion up to the current frame
            float t = totalFrames > 0 ? (float)frame / totalFrames : 0;
            int sampleIdx = Math.Min((int)(t * (sampleCount - 1)), sampleCount - 1);

            var sample = refFrame.ReferenceFrameSamples[sampleIdx];
            return new Vector3(sample.Vector.X, sample.Vector.Y, sample.Vector.Z);
        }

        private static Assimp.Matrix4x4 ToAssimpMatrix(Matrix4x4 m)
        {
            return new Assimp.Matrix4x4(
                m.M11, m.M21, m.M31, m.M41,
                m.M12, m.M22, m.M32, m.M42,
                m.M13, m.M23, m.M33, m.M43,
                m.M14, m.M24, m.M34, m.M44);
        }
    }
}

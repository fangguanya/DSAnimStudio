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
    /// Exports HKX animations to glTF 2.0.
    /// This class is responsible for animation-specific work only: HKX parsing, root-motion handling,
    /// and Assimp animation channel generation. Shared scene construction is centralized in
    /// <see cref="FormalSceneExportShared"/> so model and animation export stay perfectly aligned.
    /// </summary>
    public class AnimationToFbxExporter
    {
        public sealed class AnimationExportRecord
        {
            public string AnimationName { get; init; }
            public string DeliverableFileName { get; init; }
            public bool RootMotionSourcePresent { get; init; }
            public FormalRootMotionTrack RootMotion { get; init; }
        }

        public class ExportOptions
        {
            /// <summary>Scale factor for translation keyframes.</summary>
            public float ScaleFactor { get; set; } = 1.0f;

            /// <summary>Frames per second for exported animation.</summary>
            public float FrameRate { get; set; } = 30.0f;

            /// <summary>If true, bake root motion into root bone keyframes.</summary>
            public bool BakeRootMotion { get; set; } = true;

            /// <summary>Retained for compatibility with older callers. Formal export always writes glTF 2.0 directly.</summary>
            public string ExportFormatId { get; set; } = "gltf2";
        }

        private readonly ExportOptions _options;

        // Debug counter for diagnostic logging (limited to avoid spam)
        private int _debugLogCount;

        public AnimationToFbxExporter(ExportOptions options = null)
        {
            _options = options ?? new ExportOptions();
        }

        private static Matrix4x4 ConvertSourceMatrixToGltf(Matrix4x4 value)
        {
            return FormalSceneExportShared.ConvertSourceMatrixToGltf(value);
        }

        private static Vector3 ConvertSourceVectorToGltf(Vector3 value)
        {
            return FormalSceneExportShared.ConvertSourceVectorToGltf(value);
        }

        /// <summary>
        /// Exports one animation clip together with its formal skeleton and visible scene meshes.
        /// </summary>
        /// <param name="skeletonHkx">The HKX skeleton data.</param>
        /// <param name="animData">The parsed Havok animation payload.</param>
        /// <param name="animName">Name for the exported clip.</param>
        /// <param name="outputPath">Destination glTF file path.</param>
        /// <param name="sceneFlvers">Real FLVER meshes to include in the animated preview/export scene.</param>
        public void Export(HKX skeletonHkx, HavokAnimationData animData, string animName, string outputPath,
            IReadOnlyList<FLVER2> sceneFlvers)
        {
            var scene = new Scene();
            scene.RootNode = new Node("RootNode");
            scene.RootNode.Transform = FormalSceneExportShared.ToAssimpMatrix(FormalSceneExportShared.RootNodeCorrectionGltf);

            HKX.HKASkeleton skeleton = FormalSceneExportShared.ExtractHkxSkeleton(skeletonHkx);
            if (skeleton == null)
            {
                var types = new List<string>();
                if (skeletonHkx?.DataSection?.Objects != null)
                {
                    foreach (var obj in skeletonHkx.DataSection.Objects)
                        types.Add(obj.GetType().Name);
                }

                throw new InvalidOperationException($"No HKASkeleton found in skeleton HKX. Objects: [{string.Join(", ", types)}]");
            }

            var boneNodes = FormalSceneExportShared.BuildSkeletonHierarchyFromHkx(skeleton, scene.RootNode, _options.ScaleFactor);
            FormalSceneExportShared.LogSkeletonSelfCheck("HKX", boneNodes);
            string formalSkeletonRootName = FormalSceneExportShared.ResolveFormalSkeletonRootName(boneNodes, scene.RootNode, "animation");

            if (sceneFlvers == null || !sceneFlvers.Any(f => f?.Meshes != null && f.Meshes.Count > 0))
                throw new InvalidOperationException("Formal animation export requires real FLVER meshes; synthetic carrier meshes are not allowed.");

            var sceneBoneNames = new HashSet<string>(
                boneNodes.Where(node => node != null).Select(node => node.Name),
                StringComparer.Ordinal);

            FormalSceneExportShared.AppendSceneMeshes(scene, sceneFlvers, sceneBoneNames, _options.ScaleFactor);

            var anim = BuildAnimation(skeleton, animData, animName, animData.RootMotion);
            if (anim != null)
                scene.Animations.Add(anim);

            if (scene.Materials.Count == 0)
                scene.Materials.Add(new Material { Name = "DefaultMaterial" });

            var dir = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);

            string exportedPath = Path.ChangeExtension(outputPath, ".gltf");
            GltfSceneWriter.Write(scene, exportedPath, formalSkeletonRootName, animName);
            Console.WriteLine($"    Animation export format 'gltf2': SUCCESS -> {exportedPath}");
            Console.WriteLine($"    Animation exported: {Path.GetFileName(exportedPath)}");
        }

        /// <summary>
        /// Batch exports all requested HKX clips from an ANIBND archive.
        /// </summary>
        public void ExportAnibnd(byte[] anibndBytes, HKX skeletonHkx, string outputDir,
            IReadOnlyList<FLVER2> sceneFlvers,
            string animationFilter = null,
            IReadOnlyCollection<string> allowedAnimationNames = null,
            Action<string, int, int> progressCallback = null,
            Action<AnimationExportRecord> exportRecordCallback = null)
        {
            var bnd = BND4.Read(anibndBytes);

            byte[] compendium = null;
            foreach (var file in bnd.Files)
            {
                if (file.ID == 7000000)
                {
                    compendium = file.Bytes;
                    if (DCX.Is(compendium))
                        compendium = DCX.Decompress(compendium);
                    break;
                }
            }

            var animFiles = bnd.Files
                .Where(IsFormalAnimationBinderEntry)
                .ToList();

            if (!string.IsNullOrWhiteSpace(animationFilter))
            {
                string normalizedFilter = NormalizeAnimationName(animationFilter);
                animFiles = animFiles
                    .Where(f => NormalizeAnimationName(Path.GetFileNameWithoutExtension(f.Name ?? string.Empty)) == normalizedFilter)
                    .ToList();

                if (animFiles.Count == 0)
                    throw new InvalidOperationException($"Animation '{animationFilter}' was not found in ANIBND.");
            }

            if (allowedAnimationNames != null && allowedAnimationNames.Count > 0)
            {
                var allowedNames = new HashSet<string>(
                    allowedAnimationNames
                        .Where(name => !string.IsNullOrWhiteSpace(name))
                        .Select(NormalizeAnimationName),
                    StringComparer.OrdinalIgnoreCase);

                animFiles = animFiles
                    .Where(f => allowedNames.Contains(NormalizeAnimationName(Path.GetFileNameWithoutExtension(f.Name ?? string.Empty))))
                    .ToList();

                if (animFiles.Count == 0)
                    throw new InvalidOperationException("Requested formal animation stems were not found in the selected ANIBND.");
            }

            int total = animFiles.Count;
            int current = 0;
            foreach (var file in animFiles)
            {
                current++;
                var name = Path.GetFileNameWithoutExtension(file.Name ?? $"anim_{file.ID}");

                progressCallback?.Invoke(name, current, total);

                byte[] hkxBytes = file.Bytes;
                if (DCX.Is(hkxBytes))
                    hkxBytes = DCX.Decompress(hkxBytes);

                HavokAnimationData animData = ReadAnimData(hkxBytes, skeletonHkx, compendium);
                if (animData == null)
                    throw new InvalidOperationException($"Formal animation parse returned no data for '{name}'.");

                string cleanName = Path.GetFileNameWithoutExtension(name);
                if (cleanName.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase))
                    cleanName = cleanName.Substring(0, cleanName.Length - 4);

                string outPath = Path.Combine(outputDir, $"{cleanName}.gltf");
                var rootMotion = BuildRootMotionTrack(animData.RootMotion, animData.FrameCount);

                Export(skeletonHkx, animData, cleanName, outPath, sceneFlvers);
                exportRecordCallback?.Invoke(new AnimationExportRecord
                {
                    AnimationName = cleanName,
                    DeliverableFileName = Path.GetFileName(outPath),
                    RootMotionSourcePresent = animData.RootMotion?.Frames != null && animData.RootMotion.Frames.Length > 0,
                    RootMotion = rootMotion,
                });
            }

            progressCallback?.Invoke("done", total, total);
        }

        private static string NormalizeAnimationName(string name)
        {
            string normalized = name?.Trim() ?? string.Empty;
            if (normalized.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase))
                normalized = normalized.Substring(0, normalized.Length - 4);
            return normalized;
        }

        private static bool IsFormalAnimationBinderEntry(BinderFile file)
        {
            if (file?.Name == null || file.ID == 7000000)
                return false;

            if (!file.Name.EndsWith(".hkx", StringComparison.OrdinalIgnoreCase)
                && !file.Name.EndsWith(".hkx.dcx", StringComparison.OrdinalIgnoreCase))
            {
                return false;
            }

            const int animBindIdMin = 1000000000;
            const int animBindIdMax = animBindIdMin + 999999999;
            return file.ID >= animBindIdMin && file.ID <= animBindIdMax;
        }

        /// <summary>
        /// Reads one raw animation HKX and converts it into a runtime-neutral Havok animation wrapper.
        /// Sekiro tagfile data may optionally require the ANIBND compendium sidecar.
        /// </summary>
        private HavokAnimationData ReadAnimData(byte[] hkxBytes, HKX skeletonHkx, byte[] compendium = null)
        {
            HKX hkx = null;
            string parseMethod = compendium != null ? "tagfile+compendium" : "tagfile";

            try
            {
                hkx = compendium != null
                    ? HKX.GenFakeFromTagFile(hkxBytes, compendium)
                    : HKX.GenFakeFromTagFile(hkxBytes);
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"Formal animation HKX parsing failed via {parseMethod}: {ex.Message}", ex);
            }

            if (hkx?.DataSection?.Objects == null)
                throw new InvalidOperationException($"Formal animation HKX parsing produced no DataSection objects via {parseMethod}.");

            HKX.HKASkeleton exportSkeleton = FormalSceneExportShared.ExtractHkxSkeleton(skeletonHkx);

            HKX.HKAAnimationBinding binding = null;
            HKX.HKADefaultAnimatedReferenceFrame referenceFrame = null;
            HKX.HKASplineCompressedAnimation splineAnimation = null;
            HKX.HKAInterleavedUncompressedAnimation interleavedAnimation = null;

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
                if (obj is HKX.HKAAnimationBinding asBinding)
                    binding = asBinding;
                else if (obj is HKX.HKADefaultAnimatedReferenceFrame asReferenceFrame)
                    referenceFrame = asReferenceFrame;
                else if (obj is HKX.HKASplineCompressedAnimation asSpline)
                    splineAnimation = asSpline;
                else if (obj is HKX.HKAInterleavedUncompressedAnimation asInterleaved)
                    interleavedAnimation = asInterleaved;
            }

            if (exportSkeleton != null && binding != null)
            {
                if (splineAnimation != null)
                    return new HavokAnimationData_SplineCompressed(0L, string.Empty, exportSkeleton, referenceFrame, binding, splineAnimation);

                if (interleavedAnimation != null)
                    return new HavokAnimationData_InterleavedUncompressed(0L, string.Empty, exportSkeleton, referenceFrame, binding, interleavedAnimation);
            }

            if (splineAnimation != null)
                return new HavokAnimationData_SplineCompressed(0L, string.Empty, splineAnimation);

            if (interleavedAnimation != null)
                return new HavokAnimationData_InterleavedUncompressed(0L, string.Empty, interleavedAnimation);

            return null;
        }

        /// <summary>
        /// Builds an Assimp animation by sampling every frame of every bone in the Havok clip.
        /// Root motion can be baked into the root bone translation track when requested.
        /// </summary>
        internal Animation BuildAnimation(HKX.HKASkeleton skeleton,
            HavokAnimationData animData, string animName,
            RootMotionData rootMotion)
        {
            var anim = new Animation
            {
                Name = animName,
                TicksPerSecond = _options.FrameRate,
            };

            int frameCount = animData.FrameCount;
            if (frameCount <= 0)
                frameCount = 1;

            anim.DurationInTicks = frameCount;

            float scale = _options.ScaleFactor;

            for (int boneIdx = 0; boneIdx < (int)skeleton.Bones.Size; boneIdx++)
            {
                var boneName = skeleton.Bones[boneIdx].Name.GetString();
                if (string.IsNullOrEmpty(boneName))
                    boneName = $"Bone_{boneIdx}";

                var channel = new NodeAnimationChannel
                {
                    NodeName = boneName,
                };

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

                        if (boneIdx == 0 && rootMotion != null && _options.BakeRootMotion)
                        {
                            var rootMotionTransform = GetRootMotionAtFrame(rootMotion, frame, frameCount);
                            translation.X += rootMotionTransform.X;
                            translation.Y += rootMotionTransform.Y;
                            translation.Z += rootMotionTransform.Z;
                        }

                        var sourceMatrix = Matrix4x4.CreateScale(
                                transform.Scale.X,
                                transform.Scale.Y,
                                transform.Scale.Z)
                            * Matrix4x4.CreateFromQuaternion(new Quaternion(
                                transform.Rotation.X,
                                transform.Rotation.Y,
                                transform.Rotation.Z,
                                transform.Rotation.W))
                            * Matrix4x4.CreateTranslation(translation);

                        var gltfMatrix = ConvertSourceMatrixToGltf(sourceMatrix);
                        Matrix4x4.Decompose(gltfMatrix, out var gltfScale, out var gltfRotation, out var gltfTranslation);
                        gltfRotation = Quaternion.Normalize(gltfRotation);

                        channel.PositionKeys.Add(new VectorKey(time, new Vector3D(
                            gltfTranslation.X, gltfTranslation.Y, gltfTranslation.Z)));

                        channel.RotationKeys.Add(new QuaternionKey(time, new Assimp.Quaternion(
                            gltfRotation.W, gltfRotation.X,
                            gltfRotation.Y, gltfRotation.Z)));

                        channel.ScalingKeys.Add(new VectorKey(time, new Vector3D(
                            gltfScale.X, gltfScale.Y, gltfScale.Z)));
                    }
                    catch (Exception ex)
                    {
                        throw new InvalidOperationException($"Failed to sample bone '{boneName}' at frame {frame} for animation '{animName}'.", ex);
                    }
                }

                anim.NodeAnimationChannels.Add(channel);
            }

            return anim;
        }

        /// <summary>
        /// Samples baked root motion translation for one output frame.
        /// </summary>
        internal Vector3 GetRootMotionAtFrame(RootMotionData rootMotionData,
            int frame, int totalFrames)
        {
            if (rootMotionData?.Frames == null || rootMotionData.Frames.Length == 0)
                return Vector3.Zero;

            int clampedFrameCount = Math.Max(totalFrames, 1);
            float timeSeconds = frame / _options.FrameRate;
            var sample = rootMotionData.GetSampleClamped(Math.Min(timeSeconds,
                rootMotionData.Duration > 0 ? rootMotionData.Duration : clampedFrameCount / _options.FrameRate));
            return new Vector3(
                sample.X * _options.ScaleFactor,
                sample.Y * _options.ScaleFactor,
                sample.Z * _options.ScaleFactor);
        }

        /// <summary>
        /// Builds the formal root-motion report track written alongside exported animation metadata.
        /// </summary>
        private FormalRootMotionTrack BuildRootMotionTrack(RootMotionData rootMotionData, int frameCount)
        {
            if (rootMotionData?.Frames == null || rootMotionData.Frames.Length == 0)
                return null;

            int sampleFrameCount = Math.Max(frameCount, 1);
            var track = new FormalRootMotionTrack
            {
                FrameRate = _options.FrameRate,
                DurationSeconds = sampleFrameCount / _options.FrameRate,
            };

            Vector3 initialTranslation = Vector3.Zero;
            float initialYaw = 0.0f;
            bool hasInitial = false;

            for (int frameIndex = 0; frameIndex <= sampleFrameCount; frameIndex++)
            {
                float timeSeconds = frameIndex / _options.FrameRate;
                var sample = rootMotionData.GetSampleClamped(timeSeconds);
                var convertedTranslation = ConvertSourceVectorToGltf(new Vector3(
                    sample.X * _options.ScaleFactor,
                    sample.Y * _options.ScaleFactor,
                    sample.Z * _options.ScaleFactor));

                if (!hasInitial)
                {
                    initialTranslation = convertedTranslation;
                    initialYaw = sample.W;
                    hasInitial = true;
                }

                track.Samples.Add(new FormalRootMotionSample
                {
                    FrameIndex = frameIndex,
                    TimeSeconds = timeSeconds,
                    X = convertedTranslation.X - initialTranslation.X,
                    Y = convertedTranslation.Y - initialTranslation.Y,
                    Z = convertedTranslation.Z - initialTranslation.Z,
                    YawRadians = sample.W - initialYaw,
                });
            }

            return track;
        }
    }
}

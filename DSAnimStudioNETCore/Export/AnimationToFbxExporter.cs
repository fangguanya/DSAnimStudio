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
    /// Assimp remains the in-memory scene builder, while glTF JSON/bin emission is handled by the custom writer.
    /// Supports SplineCompressed and InterleavedUncompressed animation types.
    /// Handles root motion baking from HKADefaultAnimatedReferenceFrame.
    /// </summary>
    public class AnimationToFbxExporter
    {
        public sealed class AnimationExportRecord
        {
            public string AnimationName { get; init; }
            public string DeliverableFileName { get; init; }
            public FormalRootMotionTrack RootMotion { get; init; }
        }

        public class ExportOptions
        {
            /// <summary>Scale factor for translation keyframes</summary>
            public float ScaleFactor { get; set; } = 1.0f;

            /// <summary>Frames per second for exported animation</summary>
            public float FrameRate { get; set; } = 30.0f;

            /// <summary>If true, bake root motion into root bone keyframes</summary>
            public bool BakeRootMotion { get; set; } = true;

            /// <summary>Retained for compatibility with older callers. Formal export always writes glTF 2.0 directly.</summary>
            public string ExportFormatId { get; set; } = "gltf2";
        }

        private readonly ExportOptions _options;

        public AnimationToFbxExporter(ExportOptions options = null)
        {
            _options = options ?? new ExportOptions();
        }

        private static Matrix4x4 ConvertSourceMatrixToGltf(Matrix4x4 value)
        {
            return AssimpExportTransformUtils.ConvertSourceMatrixToGltf(value);
        }

        private static Vector3 ConvertSourceVectorToGltf(Vector3 value)
        {
            return AssimpExportTransformUtils.ConvertSourceVectorToGltf(value);
        }

        /// <summary>
        /// Export an animation with its skeleton to a standalone glTF 2.0 file.
        /// </summary>
        /// <param name="skeletonHkx">The HKX skeleton data</param>
        /// <param name="animData">The parsed HavokAnimationData</param>
        /// <param name="animName">Name for the animation clip</param>
        /// <param name="outputPath">Output glTF file path</param>
        public void Export(HKX skeletonHkx, HavokAnimationData animData, string animName, string outputPath,
            IReadOnlyList<FLVER2> sceneFlvers)
        {
            var scene = new Scene();
            scene.RootNode = new Node("RootNode");
            scene.RootNode.Transform = AssimpExportTransformUtils.ToAssimpMatrix(AssimpExportTransformUtils.RootNodeCorrectionGltf);

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
            AssimpExportTransformUtils.LogSkeletonSelfCheck("HKX", boneNodes);
            string formalSkeletonRootName = ResolveFormalSkeletonRootName(boneNodes, scene.RootNode);

            if (sceneFlvers == null || !sceneFlvers.Any(f => f?.Meshes != null && f.Meshes.Count > 0))
                throw new InvalidOperationException("Formal animation export requires real FLVER meshes; synthetic carrier meshes are not allowed.");

            var sceneBoneNames = new HashSet<string>(
                boneNodes.Where(node => node != null).Select(node => node.Name),
                StringComparer.Ordinal);

            AppendSceneMeshes(scene, sceneFlvers, sceneBoneNames);

            // Build animation
            var anim = BuildAnimation(skeleton, animData, animName, animData.RootMotion);
            if (anim != null)
            {
                scene.Animations.Add(anim);
            }

            // Need at least one material for valid export scenes.
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

        private static string ResolveFormalSkeletonRootName(IReadOnlyList<Node> boneNodes, Node sceneRoot)
        {
            if (boneNodes == null || boneNodes.Count == 0)
                throw new InvalidOperationException("Formal animation export produced no skeleton nodes.");

            var rootCandidates = boneNodes
                .Where(node => node != null && node.Parent == sceneRoot)
                .ToList();

            var selected = rootCandidates
                .FirstOrDefault(node => !string.Equals(node.Name, "Armature", StringComparison.OrdinalIgnoreCase)
                    && !string.Equals(node.Name, "RootNode", StringComparison.OrdinalIgnoreCase))
                ?? rootCandidates.FirstOrDefault()
                ?? boneNodes.FirstOrDefault(node => node != null);

            if (selected == null || string.IsNullOrWhiteSpace(selected.Name))
                throw new InvalidOperationException("Formal animation export could not resolve a declared skeleton root name.");

            return selected.Name;
        }

        /// <summary>
        /// Batch export all animations from an ANIBND archive.
        /// </summary>
        public void ExportAnibnd(byte[] anibndBytes, HKX skeletonHkx, string outputDir,
            IReadOnlyList<FLVER2> sceneFlvers,
            string animationFilter = null,
            IReadOnlyCollection<string> allowedAnimationNames = null,
            Action<string, int, int> progressCallback = null,
            Action<AnimationExportRecord> exportRecordCallback = null)
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
                if (DCX.Is(hkxBytes)) hkxBytes = DCX.Decompress(hkxBytes);

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
        /// Read HavokAnimationData from raw HKX bytes.
        /// </summary>
        // Debug counter for diagnostic logging (limited to avoid spam)
        private int _debugLogCount = 0;

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

            HKX.HKASkeleton exportSkeleton = null;
            if (skeletonHkx?.DataSection?.Objects != null)
            {
                foreach (var obj in skeletonHkx.DataSection.Objects)
                {
                    if (obj is HKX.HKASkeleton asSkeleton)
                    {
                        exportSkeleton = asSkeleton;
                        break;
                    }
                }
            }

            HKX.HKAAnimationBinding binding = null;
            HKX.HKADefaultAnimatedReferenceFrame referenceFrame = null;
            HKX.HKASplineCompressedAnimation splineAnimation = null;
            HKX.HKAInterleavedUncompressedAnimation interleavedAnimation = null;

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
        /// Build skeleton node hierarchy from HKX skeleton data.
        /// </summary>
        private List<Node> BuildSkeletonFromHkx(HKX.HKASkeleton skeleton, Node rootNode)
        {
            float scale = _options.ScaleFactor;

            return AssimpExportTransformUtils.BuildSkeletonHierarchy(
                (int)skeleton.Bones.Size,
                i => skeleton.Bones[i].Name.GetString() ?? $"Bone_{i}",
                i => skeleton.ParentIndices[i].data,
                i =>
                {
                    var transform = skeleton.Transforms[i];
                    var pos = transform.Position.Vector;
                    var rot = transform.Rotation.Vector;
                    var scl = transform.Scale.Vector;

                    var translation = new Vector3(pos.X * scale, pos.Y * scale, pos.Z * scale);
                    var rotation = new Quaternion(rot.X, rot.Y, rot.Z, rot.W);
                    var boneScale = new Vector3(scl.X, scl.Y, scl.Z);

                    var localMatrix = AssimpExportTransformUtils.CreateLocalMatrix(translation, rotation, boneScale);
                    return ConvertSourceMatrixToGltf(localMatrix);
                },
                rootNode);
        }

        private void AppendSceneMeshes(Scene scene, IReadOnlyList<FLVER2> flvers, HashSet<string> sceneBoneNames)
        {
            int meshIdx = 0;

            foreach (var flver in flvers.Where(f => f != null))
            {
                for (int flverMeshIdx = 0; flverMeshIdx < flver.Meshes.Count; flverMeshIdx++)
                {
                    var flverMesh = flver.Meshes[flverMeshIdx];
                    if (flverMesh.Vertices == null || flverMesh.Vertices.Count == 0)
                        continue;

                    var material = CreateMaterial(flver, flverMesh.MaterialIndex, scene);
                    var assimpMesh = BuildMesh(flver, flverMesh, meshIdx, sceneBoneNames);
                    if (assimpMesh == null)
                    {
                        meshIdx++;
                        continue;
                    }

                    assimpMesh.MaterialIndex = scene.Materials.IndexOf(material);
                        assimpMesh.MaterialIndex = (int)scene.Materials.IndexOf(material);
                        if (assimpMesh.MaterialIndex < 0)
                    {
                        scene.Materials.Add(material);
                        assimpMesh.MaterialIndex = (int)scene.Materials.Count - 1;
                    }

                    scene.Meshes.Add(assimpMesh);

                    var meshNode = new Node($"Mesh_{meshIdx}", scene.RootNode);
                    meshNode.MeshIndices.Add((int)scene.Meshes.Count - 1);
                    scene.RootNode.Children.Add(meshNode);
                    meshIdx++;
                }
            }
        }

        private Mesh BuildMesh(FLVER2 flver, FLVER2.Mesh flverMesh, int meshIdx, HashSet<string> sceneBoneNames)
        {
            var mesh = new Mesh($"Mesh_{meshIdx}", PrimitiveType.Triangle);
            float scale = _options.ScaleFactor;
            bool useGlobalBoneIndices = flver.Header.Version > 0x2000D;

            foreach (var vert in flverMesh.Vertices)
            {
                var gltfPosition = ConvertSourceVectorToGltf(new Vector3(
                    vert.Position.X * scale,
                    vert.Position.Y * scale,
                    vert.Position.Z * scale));
                mesh.Vertices.Add(new Vector3D(gltfPosition.X, gltfPosition.Y, gltfPosition.Z));

                var gltfNormal = ConvertSourceVectorToGltf(new Vector3(vert.Normal.X, vert.Normal.Y, vert.Normal.Z));
                mesh.Normals.Add(new Vector3D(gltfNormal.X, gltfNormal.Y, gltfNormal.Z));

                if (vert.Tangents != null && vert.Tangents.Count > 0)
                {
                    var tang = vert.Tangents[0];
                    var gltfTangent = ConvertSourceVectorToGltf(new Vector3(tang.X, tang.Y, tang.Z));
                    var gltfBitangent = ConvertSourceVectorToGltf(new Vector3(vert.Bitangent.X, vert.Bitangent.Y, vert.Bitangent.Z));
                    mesh.Tangents.Add(new Vector3D(gltfTangent.X, gltfTangent.Y, gltfTangent.Z));
                    mesh.BiTangents.Add(new Vector3D(gltfBitangent.X, gltfBitangent.Y, gltfBitangent.Z));
                }

                if (vert.Colors != null && vert.Colors.Count > 0)
                {
                    if (mesh.VertexColorChannelCount == 0)
                        mesh.VertexColorChannels[0] = new List<Color4D>();

                    var c = vert.Colors[0];
                    mesh.VertexColorChannels[0].Add(new Color4D(c.R, c.G, c.B, c.A));
                }
            }

            if (flverMesh.Vertices.Count > 0)
            {
                int uvCount = flverMesh.Vertices[0].UVs?.Count ?? 0;
                for (int uvIdx = 0; uvIdx < Math.Min(uvCount, 8); uvIdx++)
                {
                    mesh.TextureCoordinateChannels[uvIdx] = new List<Vector3D>();
                    mesh.UVComponentCount[uvIdx] = 2;

                    foreach (var vert in flverMesh.Vertices)
                    {
                        if (vert.UVs != null && uvIdx < vert.UVs.Count)
                        {
                            var uv = vert.UVs[uvIdx];
                            mesh.TextureCoordinateChannels[uvIdx].Add(new Vector3D(uv.X, 1.0f - uv.Y, 0));
                        }
                        else
                        {
                            mesh.TextureCoordinateChannels[uvIdx].Add(new Vector3D(0, 0, 0));
                        }
                    }
                }
            }

            if (mesh.Tangents.Count > 0 && mesh.Tangents.Count != mesh.Vertices.Count)
            {
                mesh.Tangents.Clear();
                mesh.BiTangents.Clear();
            }

            if (mesh.BiTangents.Count > 0 && mesh.BiTangents.Count != mesh.Vertices.Count)
            {
                mesh.BiTangents.Clear();
                if (mesh.Tangents.Count != mesh.Vertices.Count)
                    mesh.Tangents.Clear();
            }

            if (mesh.VertexColorChannels[0] != null && mesh.VertexColorChannels[0].Count > 0
                && mesh.VertexColorChannels[0].Count != mesh.Vertices.Count)
            {
                mesh.VertexColorChannels[0].Clear();
            }

            var faceSet = flverMesh.FaceSets?.FirstOrDefault(fs =>
                (fs.Flags & FLVER2.FaceSet.FSFlags.LodLevel1) == 0 &&
                (fs.Flags & FLVER2.FaceSet.FSFlags.LodLevel2) == 0 &&
                (fs.Flags & FLVER2.FaceSet.FSFlags.LodLevelEx) == 0 &&
                (fs.Flags & FLVER2.FaceSet.FSFlags.MotionBlur) == 0);

            if (faceSet == null)
                faceSet = flverMesh.FaceSets?.FirstOrDefault();

            if (faceSet == null || faceSet.Indices == null || faceSet.Indices.Count < 3)
                return null;

            var indices = faceSet.Indices;
            if (faceSet.TriangleStrip)
            {
                for (int i = 0; i < indices.Count - 2; i++)
                {
                    int i0 = indices[i];
                    int i1 = indices[i + 1];
                    int i2 = indices[i + 2];
                    if (i0 == i1 || i1 == i2 || i0 == i2)
                        continue;

                    if (i % 2 == 0)
                        mesh.Faces.Add(new Face(new[] { i0, i1, i2 }));
                    else
                        mesh.Faces.Add(new Face(new[] { i0, i2, i1 }));
                }
            }
            else
            {
                for (int i = 0; i + 2 < indices.Count; i += 3)
                    mesh.Faces.Add(new Face(new[] { indices[i], indices[i + 1], indices[i + 2] }));
            }

            var boneBuckets = new Dictionary<int, List<VertexWeight>>();

            for (int vertIdx = 0; vertIdx < flverMesh.Vertices.Count; vertIdx++)
            {
                var vert = flverMesh.Vertices[vertIdx];

                if (flverMesh.UseBoneWeights)
                {
                    for (int w = 0; w < 4; w++)
                    {
                        float weight = vert.BoneWeights[w];
                        if (weight <= 0)
                            continue;

                        int localBoneIdx = vert.BoneIndices[w];
                        int globalBoneIdx;

                        if (useGlobalBoneIndices)
                        {
                            globalBoneIdx = localBoneIdx;
                        }
                        else
                        {
                            globalBoneIdx = localBoneIdx >= 0 && localBoneIdx < flverMesh.BoneIndices.Count
                                ? flverMesh.BoneIndices[localBoneIdx]
                                : localBoneIdx;
                        }

                        if (globalBoneIdx < 0 || globalBoneIdx >= flver.Nodes.Count)
                            continue;

                        if (!boneBuckets.ContainsKey(globalBoneIdx))
                            boneBuckets[globalBoneIdx] = new List<VertexWeight>();

                        boneBuckets[globalBoneIdx].Add(new VertexWeight(vertIdx, weight));
                    }
                }
                else
                {
                    int boneIdx = vert.NormalW;
                    if (boneIdx >= 0 && boneIdx < flverMesh.BoneIndices.Count)
                    {
                        int globalBoneIdx = flverMesh.BoneIndices[boneIdx];
                        if (globalBoneIdx >= 0 && globalBoneIdx < flver.Nodes.Count)
                        {
                            if (!boneBuckets.ContainsKey(globalBoneIdx))
                                boneBuckets[globalBoneIdx] = new List<VertexWeight>();

                            boneBuckets[globalBoneIdx].Add(new VertexWeight(vertIdx, 1.0f));
                        }
                    }
                }
            }

            foreach (var kvp in boneBuckets)
            {
                int globalBoneIdx = kvp.Key;
                var weights = kvp.Value;

                if (globalBoneIdx < 0 || globalBoneIdx >= flver.Nodes.Count)
                    continue;

                string boneName = flver.Nodes[globalBoneIdx].Name;
                if (string.IsNullOrWhiteSpace(boneName))
                    boneName = $"Bone_{globalBoneIdx}";

                if (!sceneBoneNames.Contains(boneName))
                    continue;

                var bone = new Bone { Name = boneName };
                var worldMatrix = ComputeWorldMatrix(flver.Nodes, globalBoneIdx);
                Matrix4x4.Invert(worldMatrix, out var inverseWorld);
                bone.OffsetMatrix = AssimpExportTransformUtils.ToAssimpMatrix(inverseWorld);
                bone.VertexWeights.AddRange(weights);
                mesh.Bones.Add(bone);
            }

            return mesh;
        }

        private Material CreateMaterial(FLVER2 flver, int materialIndex, Scene scene)
        {
            if (materialIndex < 0 || materialIndex >= flver.Materials.Count)
            {
                var defaultMat = new Material { Name = $"Material_{materialIndex}" };
                scene.Materials.Add(defaultMat);
                return defaultMat;
            }

            var flverMat = flver.Materials[materialIndex];
            var existing = scene.Materials.FirstOrDefault(m => m.Name == flverMat.Name);
            if (existing != null)
                return existing;

            var mat = new Material
            {
                Name = !string.IsNullOrEmpty(flverMat.Name) ? flverMat.Name : $"Material_{materialIndex}"
            };

            foreach (var tex in flverMat.Textures)
            {
                if (string.IsNullOrEmpty(tex.Path))
                    continue;

                string texFileName = FlverToFbxExporter.GetTextureFileName(tex.Path);
                var typeUpper = tex.Type?.ToUpper() ?? string.Empty;

                if (typeUpper.Contains("DIFFUSE") || typeUpper.Contains("ALBEDO"))
                {
                    mat.TextureDiffuse = new TextureSlot(texFileName,
                        TextureType.Diffuse, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (typeUpper.Contains("BUMPMAP") || typeUpper.Contains("NORMALMAP"))
                {
                    mat.TextureNormal = new TextureSlot(texFileName,
                        TextureType.Normals, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (typeUpper.Contains("SPECULAR") || typeUpper.Contains("REFLECTANCE"))
                {
                    mat.TextureSpecular = new TextureSlot(texFileName,
                        TextureType.Specular, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (typeUpper.Contains("EMISSIVE"))
                {
                    mat.TextureEmissive = new TextureSlot(texFileName,
                        TextureType.Emissive, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
            }

            scene.Materials.Add(mat);
            return mat;
        }

        /// <summary>
        /// Build an Assimp animation from HavokAnimationData.
        /// Samples every frame for each bone track.
        /// </summary>
        internal Animation BuildAnimation(HKX.HKASkeleton skeleton,
            HavokAnimationData animData, string animName,
            RootMotionData rootMotion)
        {
            var anim = new Animation();
            anim.Name = animName;
            anim.TicksPerSecond = _options.FrameRate;

            int frameCount = animData.FrameCount;
            if (frameCount <= 0) frameCount = 1;

            anim.DurationInTicks = frameCount;

            float scale = _options.ScaleFactor;

            // Create a channel for each bone
            for (int boneIdx = 0; boneIdx < (int)skeleton.Bones.Size; boneIdx++)
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
        /// Get root motion translation at a specific frame from the reference frame data.
        /// </summary>
        internal Vector3 GetRootMotionAtFrame(RootMotionData rootMotionData,
            int frame, int totalFrames)
        {
            if (rootMotionData?.Frames == null || rootMotionData.Frames.Length == 0)
                return Vector3.Zero;

            int clampedFrameCount = Math.Max(totalFrames, 1);
            float timeSeconds = frame / _options.FrameRate;
            var sample = rootMotionData.GetSampleClamped(Math.Min(timeSeconds, rootMotionData.Duration > 0 ? rootMotionData.Duration : clampedFrameCount / _options.FrameRate));
            return new Vector3(
                sample.X * _options.ScaleFactor,
                sample.Y * _options.ScaleFactor,
                sample.Z * _options.ScaleFactor);
        }

        private FormalRootMotionTrack BuildRootMotionTrack(RootMotionData rootMotionData, int frameCount)
        {
            if (rootMotionData == null)
                return null;

            if (rootMotionData.Frames == null || rootMotionData.Frames.Length == 0)
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

        private Matrix4x4 ComputeWorldMatrix(IList<FLVER.Node> nodes, int boneIndex)
        {
            float scale = _options.ScaleFactor;
            var result = Matrix4x4.Identity;
            int idx = boneIndex;

            while (idx >= 0 && idx < nodes.Count)
            {
                var bone = nodes[idx];
                var s = Matrix4x4.CreateScale(bone.Scale.X, bone.Scale.Y, bone.Scale.Z);
                var rx = Matrix4x4.CreateRotationX(bone.Rotation.X);
                var rz = Matrix4x4.CreateRotationZ(bone.Rotation.Z);
                var ry = Matrix4x4.CreateRotationY(bone.Rotation.Y);
                var t = Matrix4x4.CreateTranslation(
                    bone.Translation.X * scale,
                    bone.Translation.Y * scale,
                    bone.Translation.Z * scale);

                var localMatrix = s * rx * rz * ry * t;
                localMatrix = ConvertSourceMatrixToGltf(localMatrix);
                result *= localMatrix;
                idx = bone.ParentIndex;
            }

            return result;
        }

    }
}

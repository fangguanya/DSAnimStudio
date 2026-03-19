using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Assimp;
using Newtonsoft.Json.Linq;
using SoulsFormats;
using HKX = SoulsAssetPipeline.Animation.HKX;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports FLVER2 models to glTF 2.0.
    /// Assimp remains the in-memory scene builder, while glTF JSON/bin emission is handled by the custom writer.
    /// </summary>
    public class FlverToFbxExporter
    {
        private static readonly Matrix4x4 ModelRootNodeCorrectionGltf = Matrix4x4.Identity;

        public class ExportOptions
        {
            /// <summary>Scale factor applied to all positions (default 1.0)</summary>
            public float ScaleFactor { get; set; } = 1.0f;

            /// <summary>Retained for compatibility with older callers. Formal export always writes glTF 2.0 directly.</summary>
            public string ExportFormatId { get; set; } = "gltf2";
        }

        private readonly ExportOptions _options;

        public FlverToFbxExporter(ExportOptions options = null)
        {
            _options = options ?? new ExportOptions();
        }

        private static Vector3 ConvertSourceVectorToGltf(Vector3 value)
        {
            return AssimpExportTransformUtils.ConvertSourceVectorToGltf(value);
        }

        private static Matrix4x4 ConvertSourceMatrixToGltf(Matrix4x4 value)
        {
            return AssimpExportTransformUtils.ConvertSourceMatrixToGltf(value);
        }

        /// <summary>
        /// Export a FLVER2 model to glTF 2.0.
        /// </summary>
        public void Export(FLVER2 flver, string outputPath)
        {
            Export(new[] { flver }, skeletonHkx: null, outputPath);
        }

        public void Export(IReadOnlyList<FLVER2> flvers, HKX skeletonHkx, string outputPath)
        {
            if (flvers == null || flvers.Count == 0)
                throw new ArgumentException("At least one FLVER is required.", nameof(flvers));

            var validFlvers = flvers.Where(f => f != null).ToList();
            if (validFlvers.Count == 0)
                throw new ArgumentException("At least one non-null FLVER is required.", nameof(flvers));

            var scene = new Scene();
            scene.RootNode = new Node("RootNode");
            scene.RootNode.Transform = AssimpExportTransformUtils.ToAssimpMatrix(ModelRootNodeCorrectionGltf);

            var sceneSkeleton = ExtractHkxSkeleton(skeletonHkx);
            var boneNodes = sceneSkeleton != null
                ? BuildSkeletonHierarchy(sceneSkeleton, scene.RootNode)
                : BuildSkeletonHierarchy(validFlvers[0], scene.RootNode);

            AssimpExportTransformUtils.LogSkeletonSelfCheck(sceneSkeleton != null ? "HKX->FLVER" : "FLVER", boneNodes);

            var sceneBoneNames = new HashSet<string>(
                boneNodes.Where(node => node != null).Select(node => node.Name),
                StringComparer.Ordinal);
            string formalSkeletonRootName = ResolveFormalSkeletonRootName(boneNodes, scene.RootNode);

            int meshIdx = 0;
            foreach (var flver in validFlvers)
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

            if (scene.Materials.Count == 0)
            {
                scene.Materials.Add(new Material { Name = "DefaultMaterial" });
            }

            var dir = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);

            Console.WriteLine($"    Export scene: {scene.Meshes.Count} meshes, {scene.Materials.Count} materials, {scene.RootNode?.Children?.Count} children");
            foreach (var mesh in scene.Meshes)
            {
                Console.WriteLine($"      {mesh.Name}: {mesh.VertexCount} verts, {mesh.FaceCount} faces, {mesh.BoneCount} bones, hasNormals={mesh.HasNormals}, hasTangents={mesh.HasTangentBasis}");
                if (mesh.VertexCount > 0)
                {
                    bool hasNaN = mesh.Vertices.Any(v => float.IsNaN(v.X) || float.IsNaN(v.Y) || float.IsNaN(v.Z));
                    if (hasNaN)
                        Console.WriteLine("        WARNING: has NaN vertices!");

                    if (mesh.HasNormals)
                    {
                        bool normalsHaveNaN = mesh.Normals.Any(v => float.IsNaN(v.X) || float.IsNaN(v.Y) || float.IsNaN(v.Z));
                        if (normalsHaveNaN)
                            Console.WriteLine("        WARNING: has NaN normals!");
                    }
                }

                bool faceOOR = mesh.Faces.Any(f => f.Indices.Any(idx => idx < 0 || idx >= mesh.VertexCount));
                if (faceOOR)
                    Console.WriteLine("        WARNING: face indices out of range!");
            }

            string exportedPath = Path.ChangeExtension(outputPath, ".gltf");
            GltfSceneWriter.Write(scene, exportedPath, formalSkeletonRootName);
            Console.WriteLine($"    Export format 'gltf2': SUCCESS -> {exportedPath}");
        }

        private static string ResolveFormalSkeletonRootName(IReadOnlyList<Node> boneNodes, Node sceneRoot)
        {
            if (boneNodes == null || boneNodes.Count == 0)
                throw new InvalidOperationException("Formal model export produced no skeleton nodes.");

            var rootCandidates = boneNodes
                .Where(node => node != null && node.Parent == sceneRoot)
                .ToList();

            var selected = rootCandidates
                .FirstOrDefault(node => !string.Equals(node.Name, "Armature", StringComparison.OrdinalIgnoreCase)
                    && !string.Equals(node.Name, "RootNode", StringComparison.OrdinalIgnoreCase))
                ?? rootCandidates.FirstOrDefault()
                ?? boneNodes.FirstOrDefault(node => node != null);

            if (selected == null || string.IsNullOrWhiteSpace(selected.Name))
                throw new InvalidOperationException("Formal model export could not resolve a declared skeleton root name.");

            return selected.Name;
        }

        private static HashSet<int> CollectJointAccessorIndices(JObject root)
        {
            var result = new HashSet<int>();
            var meshes = root["meshes"] as JArray;
            if (meshes == null)
                return result;

            foreach (var meshObj in meshes.OfType<JObject>())
            {
                var primitives = meshObj["primitives"] as JArray;
                if (primitives == null)
                    continue;

                foreach (var primitiveObj in primitives.OfType<JObject>())
                {
                    var attrs = primitiveObj["attributes"] as JObject;
                    int? accessorIndex = (int?)attrs?["JOINTS_0"];
                    if (accessorIndex.HasValue)
                        result.Add(accessorIndex.Value);
                }
            }

            return result;
        }

        private static int? FindSkinRootJoint(JArray nodes, JArray joints)
        {
            var jointList = joints.Values<int>().ToList();
            var parents = new Dictionary<int, int>();
            var depths = new Dictionary<int, int>();

            for (int nodeIndex = 0; nodeIndex < nodes.Count; nodeIndex++)
            {
                var node = nodes[nodeIndex] as JObject;
                var children = node?["children"] as JArray;
                if (children == null)
                    continue;

                foreach (int child in children.Values<int>())
                    parents[child] = nodeIndex;
            }

            foreach (int nodeIndex in Enumerable.Range(0, nodes.Count))
            {
                int depth = 0;
                int current = nodeIndex;
                while (parents.TryGetValue(current, out int parent))
                {
                    depth++;
                    current = parent;
                }

                depths[nodeIndex] = depth;
            }

            HashSet<int> commonAncestors = null;
            foreach (int joint in jointList)
            {
                var ancestors = new HashSet<int>();
                int current = joint;
                ancestors.Add(current);
                while (parents.TryGetValue(current, out int parent))
                {
                    current = parent;
                    ancestors.Add(current);
                }

                if (commonAncestors == null)
                {
                    commonAncestors = ancestors;
                }
                else
                {
                    commonAncestors.IntersectWith(ancestors);
                }
            }

            if (commonAncestors != null && commonAncestors.Count > 0)
            {
                return commonAncestors
                    .OrderByDescending(idx => depths.TryGetValue(idx, out int depth) ? depth : -1)
                    .First();
            }

            return jointList.Count > 0 ? jointList[0] : (int?)null;
        }

        private static int AlignTo4(int value)
        {
            int remainder = value % 4;
            return remainder == 0 ? value : value + (4 - remainder);
        }

        private static HashSet<int> CollectMeshNodeIndices(JArray nodes)
        {
            var result = new HashSet<int>();
            for (int i = 0; i < nodes.Count; i++)
            {
                var node = nodes[i] as JObject;
                if (node?["mesh"] != null)
                    result.Add(i);
            }

            return result;
        }

        /// <summary>
        /// Build the bone hierarchy as Assimp nodes attached to the root.
        /// </summary>
        private List<Node> BuildSkeletonHierarchy(FLVER2 flver, Node rootNode)
        {
            float scale = _options.ScaleFactor;

            return AssimpExportTransformUtils.BuildSkeletonHierarchy(
                flver.Nodes.Count,
                i => !string.IsNullOrEmpty(flver.Nodes[i].Name) ? flver.Nodes[i].Name : $"Bone_{i}",
                i => (short)flver.Nodes[i].ParentIndex,
                i =>
                {
                    var flverBone = flver.Nodes[i];
                    var s = Matrix4x4.CreateScale(flverBone.Scale.X, flverBone.Scale.Y, flverBone.Scale.Z);
                    var rx = Matrix4x4.CreateRotationX(flverBone.Rotation.X);
                    var rz = Matrix4x4.CreateRotationZ(flverBone.Rotation.Z);
                    var ry = Matrix4x4.CreateRotationY(flverBone.Rotation.Y);
                    var t = Matrix4x4.CreateTranslation(
                        flverBone.Translation.X * scale,
                        flverBone.Translation.Y * scale,
                        flverBone.Translation.Z * scale);

                    var localMatrix = s * rx * rz * ry * t;
                    return ConvertSourceMatrixToGltf(localMatrix);
                },
                rootNode);
        }

        private List<Node> BuildSkeletonHierarchy(HKX.HKASkeleton skeleton, Node rootNode)
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
                    var rotation = new System.Numerics.Quaternion(rot.X, rot.Y, rot.Z, rot.W);
                    var boneScale = new Vector3(scl.X, scl.Y, scl.Z);

                    var localMatrix = AssimpExportTransformUtils.CreateLocalMatrix(translation, rotation, boneScale);
                    return ConvertSourceMatrixToGltf(localMatrix);
                },
                rootNode);
        }

        /// <summary>
        /// Build an Assimp mesh from a FLVER mesh, including vertices, faces, UVs, and bone weights.
        /// </summary>
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
                var worldMatrix = ComputeWorldMatrix(flver.Nodes, globalBoneIdx, scale);
                Matrix4x4.Invert(worldMatrix, out var inverseWorld);
                bone.OffsetMatrix = AssimpExportTransformUtils.ToAssimpMatrix(inverseWorld);
                bone.VertexWeights.AddRange(weights);
                mesh.Bones.Add(bone);
            }

            return mesh;
        }

        /// <summary>
        /// Create an Assimp material from a FLVER material.
        /// </summary>
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

            foreach (var binding in FormalMaterialTextureResolver.Resolve(flverMat))
            {
                if (string.IsNullOrWhiteSpace(binding.ModelRelativePath))
                    continue;

                if (string.Equals(binding.SlotType, "BaseColor", StringComparison.OrdinalIgnoreCase)
                    && binding.SlotIndex == 1)
                {
                    mat.TextureDiffuse = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Diffuse, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (string.Equals(binding.SlotType, "Normal", StringComparison.OrdinalIgnoreCase)
                    && binding.SlotIndex == 1)
                {
                    mat.TextureNormal = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Normals, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (string.Equals(binding.SlotType, "Specular", StringComparison.OrdinalIgnoreCase)
                    && binding.SlotIndex == 1)
                {
                    mat.TextureSpecular = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Specular, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (string.Equals(binding.SlotType, "Emissive", StringComparison.OrdinalIgnoreCase)
                    && binding.SlotIndex == 1)
                {
                    mat.TextureEmissive = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Emissive, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
            }

            scene.Materials.Add(mat);
            return mat;
        }

        private static HKX.HKASkeleton ExtractHkxSkeleton(HKX skeletonHkx)
        {
            if (skeletonHkx?.DataSection?.Objects == null)
                return null;

            foreach (var obj in skeletonHkx.DataSection.Objects)
            {
                if (obj is HKX.HKASkeleton skeleton)
                    return skeleton;
            }

            return null;
        }

        /// <summary>
        /// Compute the world (FK) matrix of a bone by walking up the hierarchy.
        /// Matches the pattern in NewBone constructor: S * RX * RZ * RY * T.
        /// </summary>
        private Matrix4x4 ComputeWorldMatrix(IList<FLVER.Node> nodes, int boneIndex, float scale)
        {
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
                result = result * localMatrix;
                idx = bone.ParentIndex;
            }

            return result;
        }

        /// <summary>
        /// Extract the texture file name from a FLVER internal path.
        /// e.g. "N:\SPRJ\data\Model\chr\c0000\c0000_a.tga" -> "c0000_a"
        /// </summary>
        public static string GetTextureFileName(string internalPath)
        {
            if (string.IsNullOrEmpty(internalPath))
                return string.Empty;

            var name = Path.GetFileNameWithoutExtension(internalPath);
            return name ?? string.Empty;
        }
    }
}

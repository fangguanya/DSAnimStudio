using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Assimp;
using SoulsFormats;
using HKX = SoulsAssetPipeline.Animation.HKX;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Quaternion = System.Numerics.Quaternion;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Shared scene-building helpers for formal glTF model and animation export.
    /// This is the single common place for coordinate conversion, matrix conversion,
    /// skeleton hierarchy construction, mesh/material creation, and export diagnostics.
    /// </summary>
    public static class FormalSceneExportShared
    {
        /// <summary>
        /// Source-to-glTF basis used by the formal exporter.
        /// Source +X becomes glTF -Z, source +Y remains +Y, source +Z becomes glTF -X.
        /// </summary>
        public static readonly Matrix4x4 SourceToGltfBasis = Matrix4x4.CreateScale(-1,1,1) * Matrix4x4.CreateRotationY(MathF.PI);

        /// <summary>
        /// Inverse basis for glTF-to-source conversion.
        /// </summary>
        public static readonly Matrix4x4 GltfToSourceBasis = Matrix4x4.Transpose(SourceToGltfBasis);

        /// <summary>
        /// Additional correction applied after the basis transform.
        /// Kept as identity for the current formal export convention.
        /// </summary>
        public static readonly Matrix4x4 PostTransformCorrectionGltf = Matrix4x4.Identity;

        /// <summary>
        /// Full source-to-glTF transform used for positions and directions.
        /// </summary>
        public static readonly Matrix4x4 SourceToGltfTransform = SourceToGltfBasis * PostTransformCorrectionGltf;

        /// <summary>
        /// Full glTF-to-source transform corresponding to <see cref="SourceToGltfTransform"/>.
        /// </summary>
        public static readonly Matrix4x4 GltfToSourceTransform = Matrix4x4.Transpose(SourceToGltfTransform);

        /// <summary>
        /// Root scene correction matrix. Identity because the scene is serialized with already-converted TRS values.
        /// </summary>
        public static readonly Matrix4x4 RootNodeCorrectionGltf = Matrix4x4.Identity;

        /// <summary>
        /// Converts a source-space vector into the formal glTF basis.
        /// </summary>
        public static Vector3 ConvertSourceVectorToGltf(Vector3 value)
        {
            return Vector3.Transform(value, SourceToGltfTransform);
        }

        /// <summary>
        /// Converts a source-space local matrix into glTF basis space via basis conjugation.
        /// </summary>
        public static Matrix4x4 ConvertSourceMatrixToGltf(Matrix4x4 value)
        {
            return GltfToSourceTransform * value * SourceToGltfTransform;
        }

        /// <summary>
        /// Builds a local scale-rotation-translation matrix.
        /// </summary>
        public static Matrix4x4 CreateLocalMatrix(Vector3 translation, Quaternion rotation, Vector3 scale)
        {
            return Matrix4x4.CreateScale(scale)
                * Matrix4x4.CreateFromQuaternion(rotation)
                * Matrix4x4.CreateTranslation(translation);
        }

        /// <summary>
        /// Builds an Assimp skeleton hierarchy under an `Armature` wrapper.
        /// </summary>
        public static List<Node> BuildSkeletonHierarchy(
            int boneCount,
            Func<int, string> getBoneName,
            Func<int, short> getParentIndex,
            Func<int, Matrix4x4> getLocalMatrix,
            Node rootNode)
        {
            var armatureNode = new Node("Armature", rootNode);
            rootNode.Children.Add(armatureNode);

            var boneNodes = new List<Node>(new Node[boneCount]);
            for (int i = 0; i < boneCount; i++)
            {
                var boneNode = new Node(getBoneName(i) ?? $"Bone_{i}");
                boneNode.Transform = ToAssimpMatrix(getLocalMatrix(i));
                boneNodes[i] = boneNode;
            }

            for (int i = 0; i < boneCount; i++)
            {
                short parentIdx = getParentIndex(i);
                if (parentIdx >= 0 && parentIdx < boneNodes.Count)
                    boneNodes[parentIdx].Children.Add(boneNodes[i]);
                else
                    armatureNode.Children.Add(boneNodes[i]);
            }

            return boneNodes;
        }

        /// <summary>
        /// Converts a numerics matrix to Assimp matrix layout.
        /// </summary>
        public static Assimp.Matrix4x4 ToAssimpMatrix(Matrix4x4 value)
        {
            return new Assimp.Matrix4x4(
                value.M11, value.M21, value.M31, value.M41,
                value.M12, value.M22, value.M32, value.M42,
                value.M13, value.M23, value.M33, value.M43,
                value.M14, value.M24, value.M34, value.M44);
        }

        /// <summary>
        /// Converts an Assimp matrix back to numerics layout.
        /// </summary>
        public static Matrix4x4 ToNumericsMatrix(Assimp.Matrix4x4 value)
        {
            return new Matrix4x4(
                value.A1, value.B1, value.C1, value.D1,
                value.A2, value.B2, value.C2, value.D2,
                value.A3, value.B3, value.C3, value.D3,
                value.A4, value.B4, value.C4, value.D4);
        }

        /// <summary>
        /// Logs a lightweight skeleton sanity check to aid coordinate-system debugging.
        /// </summary>
        public static void LogSkeletonSelfCheck(string label, IReadOnlyList<Node> boneNodes)
        {
            if (boneNodes == null || boneNodes.Count == 0)
            {
                Console.WriteLine($"    Skeleton self-check [{label}]: no bones");
                return;
            }

            var childSet = new HashSet<Node>(boneNodes.SelectMany(node => node.Children).Where(node => node != null));
            var rootBone = boneNodes.FirstOrDefault(node => node != null && !childSet.Contains(node)) ?? boneNodes.FirstOrDefault(node => node != null);
            if (rootBone == null)
            {
                Console.WriteLine($"    Skeleton self-check [{label}]: no valid root bone");
                return;
            }

            var rootLocal = ToNumericsMatrix(rootBone.Transform);
            var rootWorld = ComputeSkeletonWorldMatrix(rootBone, boneNodes);
            var rootForward = Vector3.Normalize(Vector3.TransformNormal(Vector3.UnitZ, rootWorld));

            Console.WriteLine($"    Skeleton self-check [{label}]: root='{rootBone.Name}', forward(+Z)={FormatVector(rootForward)}");
            Console.WriteLine($"      local={FormatMatrix(rootLocal)}");
            Console.WriteLine($"      world={FormatMatrix(rootWorld)}");
        }

        private static Matrix4x4 ComputeSkeletonWorldMatrix(Node node, IReadOnlyList<Node> boneNodes)
        {
            var lineage = new Stack<Node>();
            var current = node;
            var parentMap = BuildParentMap(boneNodes);

            while (current != null)
            {
                lineage.Push(current);
                if (!parentMap.TryGetValue(current, out current))
                    break;
            }

            var result = Matrix4x4.Identity;
            while (lineage.Count > 0)
                result *= ToNumericsMatrix(lineage.Pop().Transform);

            return result;
        }

        private static Dictionary<Node, Node> BuildParentMap(IReadOnlyList<Node> boneNodes)
        {
            var parentMap = new Dictionary<Node, Node>();
            foreach (var node in boneNodes)
            {
                if (node == null)
                    continue;

                foreach (var child in node.Children)
                {
                    if (child != null && !parentMap.ContainsKey(child))
                        parentMap.Add(child, node);
                }
            }

            return parentMap;
        }

        private static string FormatMatrix(Matrix4x4 value)
        {
            return string.Create(CultureInfo.InvariantCulture, $"[{value.M11:F3}, {value.M12:F3}, {value.M13:F3}, {value.M14:F3}; {value.M21:F3}, {value.M22:F3}, {value.M23:F3}, {value.M24:F3}; {value.M31:F3}, {value.M32:F3}, {value.M33:F3}, {value.M34:F3}; {value.M41:F3}, {value.M42:F3}, {value.M43:F3}, {value.M44:F3}]");
        }

        private static string FormatVector(Vector3 value)
        {
            return string.Create(CultureInfo.InvariantCulture, $"({value.X:F3}, {value.Y:F3}, {value.Z:F3})");
        }

        /// <summary>
        /// Extracts the first <see cref="HKX.HKASkeleton"/> object from a parsed HKX container.
        /// </summary>
        public static HKX.HKASkeleton ExtractHkxSkeleton(HKX skeletonHkx)
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
        /// Resolves the formal skeleton root node name from a built Assimp bone hierarchy.
        /// Wrapper nodes such as `Armature` and `RootNode` are ignored when possible.
        /// </summary>
        public static string ResolveFormalSkeletonRootName(IReadOnlyList<Node> boneNodes, Node sceneRoot, string exportKind)
        {
            if (boneNodes == null || boneNodes.Count == 0)
                throw new InvalidOperationException($"Formal {exportKind} export produced no skeleton nodes.");

            var rootCandidates = boneNodes
                .Where(node => node != null && node.Parent == sceneRoot)
                .ToList();

            var selected = rootCandidates
                .FirstOrDefault(node => !string.Equals(node.Name, "Armature", StringComparison.OrdinalIgnoreCase)
                    && !string.Equals(node.Name, "RootNode", StringComparison.OrdinalIgnoreCase))
                ?? rootCandidates.FirstOrDefault()
                ?? boneNodes.FirstOrDefault(node => node != null);

            if (selected == null || string.IsNullOrWhiteSpace(selected.Name))
                throw new InvalidOperationException($"Formal {exportKind} export could not resolve a declared skeleton root name.");

            return selected.Name;
        }

        /// <summary>
        /// Builds skeleton nodes from FLVER bind-pose bones and converts them into the formal glTF basis.
        /// </summary>
        public static List<Node> BuildSkeletonHierarchyFromFlver(FLVER2 flver, Node rootNode, float scaleFactor)
        {
            return BuildSkeletonHierarchy(
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
                        flverBone.Translation.X * scaleFactor,
                        flverBone.Translation.Y * scaleFactor,
                        flverBone.Translation.Z * scaleFactor);

                    var localMatrix = s * rx * rz * ry * t;
                    return ConvertSourceMatrixToGltf(localMatrix);
                },
                rootNode);
        }

        /// <summary>
        /// Builds skeleton nodes from HKX bind-pose transforms and converts them into the formal glTF basis.
        /// </summary>
        public static List<Node> BuildSkeletonHierarchyFromHkx(HKX.HKASkeleton skeleton, Node rootNode, float scaleFactor)
        {
            return BuildSkeletonHierarchy(
                (int)skeleton.Bones.Size,
                i => skeleton.Bones[i].Name.GetString() ?? $"Bone_{i}",
                i => skeleton.ParentIndices[i].data,
                i =>
                {
                    var transform = skeleton.Transforms[i];
                    var pos = transform.Position.Vector;
                    var rot = transform.Rotation.Vector;
                    var scl = transform.Scale.Vector;

                    var translation = new Vector3(pos.X * scaleFactor, pos.Y * scaleFactor, pos.Z * scaleFactor);
                    var rotation = new Quaternion(rot.X, rot.Y, rot.Z, rot.W);
                    var boneScale = new Vector3(scl.X, scl.Y, scl.Z);

                    var localMatrix = CreateLocalMatrix(translation, rotation, boneScale);
                    return ConvertSourceMatrixToGltf(localMatrix);
                },
                rootNode);
        }

        /// <summary>
        /// Creates or reuses an Assimp material for a FLVER material using the formal material texture contract.
        /// </summary>
        public static Material CreateFormalMaterial(FLVER2 flver, int materialIndex, Scene scene)
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

                if (string.Equals(binding.SlotType, "BaseColor", StringComparison.OrdinalIgnoreCase) && binding.SlotIndex == 1)
                {
                    mat.TextureDiffuse = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Diffuse, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (string.Equals(binding.SlotType, "Normal", StringComparison.OrdinalIgnoreCase) && binding.SlotIndex == 1)
                {
                    mat.TextureNormal = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Normals, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (string.Equals(binding.SlotType, "Specular", StringComparison.OrdinalIgnoreCase) && binding.SlotIndex == 1)
                {
                    mat.TextureSpecular = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Specular, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
                else if (string.Equals(binding.SlotType, "Emissive", StringComparison.OrdinalIgnoreCase) && binding.SlotIndex == 1)
                {
                    mat.TextureEmissive = new TextureSlot(binding.ModelRelativePath,
                        TextureType.Emissive, 0, TextureMapping.FromUV,
                        0, 1.0f, TextureOperation.Multiply, TextureWrapMode.Wrap, TextureWrapMode.Wrap, 0);
                }
            }

            scene.Materials.Add(mat);
            return mat;
        }

        /// <summary>
        /// Builds a skinned Assimp mesh from FLVER geometry.
        /// </summary>
        public static Mesh BuildFormalMesh(FLVER2 flver, FLVER2.Mesh flverMesh, int meshIdx, HashSet<string> sceneBoneNames, float scaleFactor)
        {
            var mesh = new Mesh($"Mesh_{meshIdx}", PrimitiveType.Triangle);
            bool useGlobalBoneIndices = flver.Header.Version > 0x2000D;

            foreach (var vert in flverMesh.Vertices)
            {
                var gltfPosition = ConvertSourceVectorToGltf(new Vector3(
                    vert.Position.X * scaleFactor,
                    vert.Position.Y * scaleFactor,
                    vert.Position.Z * scaleFactor));
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
                        mesh.Faces.Add(new Face(new[] { i0, i2, i1 }));
                    else
                        mesh.Faces.Add(new Face(new[] { i0, i1, i2 }));
                }
            }
            else
            {
                for (int i = 0; i + 2 < indices.Count; i += 3)
                    mesh.Faces.Add(new Face(new[] { indices[i], indices[i + 2], indices[i + 1] }));
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
                        int globalBoneIdx = useGlobalBoneIndices
                            ? localBoneIdx
                            : localBoneIdx >= 0 && localBoneIdx < flverMesh.BoneIndices.Count
                                ? flverMesh.BoneIndices[localBoneIdx]
                                : localBoneIdx;

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
                var worldMatrix = ComputeWorldMatrix(flver.Nodes, globalBoneIdx, scaleFactor);
                Matrix4x4.Invert(worldMatrix, out var inverseWorld);
                bone.OffsetMatrix = ToAssimpMatrix(inverseWorld);
                bone.VertexWeights.AddRange(weights);
                mesh.Bones.Add(bone);
            }

            return mesh;
        }

        /// <summary>
        /// Appends all valid FLVER meshes to the scene using the shared formal material and mesh pipeline.
        /// </summary>
        public static void AppendSceneMeshes(Scene scene, IReadOnlyList<FLVER2> flvers, HashSet<string> sceneBoneNames, float scaleFactor)
        {
            int meshIdx = 0;

            foreach (var flver in flvers.Where(f => f != null))
            {
                for (int flverMeshIdx = 0; flverMeshIdx < flver.Meshes.Count; flverMeshIdx++)
                {
                    var flverMesh = flver.Meshes[flverMeshIdx];
                    if (flverMesh.Vertices == null || flverMesh.Vertices.Count == 0)
                        continue;

                    var material = CreateFormalMaterial(flver, flverMesh.MaterialIndex, scene);
                    var assimpMesh = BuildFormalMesh(flver, flverMesh, meshIdx, sceneBoneNames, scaleFactor);
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
        }

        /// <summary>
        /// Computes the exported world matrix for a FLVER bone by walking its parent chain.
        /// </summary>
        public static Matrix4x4 ComputeWorldMatrix(IList<FLVER.Node> nodes, int boneIndex, float scaleFactor)
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
                    bone.Translation.X * scaleFactor,
                    bone.Translation.Y * scaleFactor,
                    bone.Translation.Z * scaleFactor);

                var localMatrix = s * rx * rz * ry * t;
                localMatrix = ConvertSourceMatrixToGltf(localMatrix);
                result *= localMatrix;
                idx = bone.ParentIndex;
            }

            return result;
        }

        /// <summary>
        /// Extracts a texture stem from a FLVER internal texture path.
        /// </summary>
        public static string GetTextureFileName(string internalPath)
        {
            if (string.IsNullOrEmpty(internalPath))
                return string.Empty;

            var name = System.IO.Path.GetFileNameWithoutExtension(internalPath);
            return name ?? string.Empty;
        }
    }
}

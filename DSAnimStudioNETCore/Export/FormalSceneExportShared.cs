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

        // ─── Bone reparent / rename configuration ───

        private static readonly (string Bone, string NewParent)[] BoneReparentRules =
        {
            ("RootRotY", "Pelvis"),
        };

        internal static readonly Dictionary<string, string> BoneRenameRules = new(StringComparer.Ordinal)
        {
            ["RootRotY"]  = "spine_01",
            ["RootRotXZ"] = "spine_02",
            ["Spine"]     = "spine_03",
            ["Spine1"]    = "spine_04",
            ["Spine2"]    = "spine_05",
        };

        /// <summary>
        /// Returns the renamed bone name if a rule exists, otherwise returns the original.
        /// </summary>
        internal static string GetRenamedBoneName(string originalName)
        {
            if (originalName != null && BoneRenameRules.TryGetValue(originalName, out string newName))
                return newName;
            return originalName;
        }

        /// <summary>
        /// Deep-copies FLVER.Nodes and applies reparent + rename + local transform recalculation in source space.
        /// The returned list can be used everywhere in place of the original flver.Nodes.
        /// </summary>
        public static List<FLVER.Node> BuildModifiedFlverNodes(IList<FLVER.Node> originalNodes, float scaleFactor)
        {
            var nodes = new List<FLVER.Node>(originalNodes.Count);
            for (int i = 0; i < originalNodes.Count; i++)
                nodes.Add(new FLVER.Node(originalNodes[i]));

            foreach (var (boneName, newParentName) in BoneReparentRules)
            {
                int boneIdx = nodes.FindIndex(n => string.Equals(n.Name, boneName, StringComparison.Ordinal));
                if (boneIdx < 0) { Console.WriteLine($"    [BoneTransform] reparent source '{boneName}' not found, skipping."); continue; }

                int newParentIdx = nodes.FindIndex(n => string.Equals(n.Name, newParentName, StringComparison.Ordinal));
                if (newParentIdx < 0) { Console.WriteLine($"    [BoneTransform] reparent target '{newParentName}' not found, skipping."); continue; }

                Console.WriteLine($"    [BoneTransform] Reparent '{boneName}' from parent={nodes[boneIdx].ParentIndex} to '{newParentName}' (idx={newParentIdx})");

                // Recalculate local transform: newLocal = oldLocal * inv(newParentLocal) [row-vector convention]
                var oldLocal = nodes[boneIdx].ComputeLocalTransform()
                    * Matrix4x4.CreateScale(1f / scaleFactor == 0 ? 1 : 1, 1, 1); // identity guard
                // Recompute with scale factor applied to translations
                oldLocal = ComputeSourceLocalMatrix(nodes[boneIdx], scaleFactor);
                var parentLocal = ComputeSourceLocalMatrix(nodes[newParentIdx], scaleFactor);

                if (Matrix4x4.Invert(parentLocal, out var invParentLocal))
                {
                    var newLocal = oldLocal * invParentLocal;
                    DecomposeToFlverNode(newLocal, scaleFactor, nodes[boneIdx]);
                }
                else
                {
                    Console.WriteLine($"    [BoneTransform] WARNING: cannot invert parent local for '{boneName}', keeping original transform.");
                }

                nodes[boneIdx].ParentIndex = (short)newParentIdx;
            }

            // Apply rename
            for (int i = 0; i < nodes.Count; i++)
            {
                if (BoneRenameRules.TryGetValue(nodes[i].Name, out string newName))
                {
                    Console.WriteLine($"    [BoneTransform] Rename '{nodes[i].Name}' -> '{newName}'");
                    nodes[i].Name = newName;
                }
            }

            return nodes;
        }

        /// <summary>
        /// Computes the source-space local matrix for a FLVER.Node with a scale factor applied to translation.
        /// </summary>
        private static Matrix4x4 ComputeSourceLocalMatrix(FLVER.Node bone, float scaleFactor)
        {
            var s = Matrix4x4.CreateScale(bone.Scale.X, bone.Scale.Y, bone.Scale.Z);
            var rx = Matrix4x4.CreateRotationX(bone.Rotation.X);
            var rz = Matrix4x4.CreateRotationZ(bone.Rotation.Z);
            var ry = Matrix4x4.CreateRotationY(bone.Rotation.Y);
            var t = Matrix4x4.CreateTranslation(
                bone.Translation.X * scaleFactor,
                bone.Translation.Y * scaleFactor,
                bone.Translation.Z * scaleFactor);
            return s * rx * rz * ry * t;
        }

        /// <summary>
        /// Decomposes a source-space local matrix back into FLVER.Node Translation/Rotation/Scale fields.
        /// Uses XZY Euler extraction matching the FLVER convention: localMatrix = S * Rx * Rz * Ry * T.
        /// </summary>
        private static void DecomposeToFlverNode(Matrix4x4 localMatrix, float scaleFactor, FLVER.Node target)
        {
            // Decompose using System.Numerics (gives quaternion)
            if (!Matrix4x4.Decompose(localMatrix, out var scale, out var quat, out var translation))
            {
                Console.WriteLine($"    [BoneTransform] WARNING: matrix decomposition failed for '{target.Name}'.");
                return;
            }

            // Convert quaternion to rotation matrix, then extract XZY Euler angles
            var rotMatrix = Matrix4x4.CreateFromQuaternion(quat);

            // From the derivation: R = Rx(a) * Rz(b) * Ry(c)
            // R[0][1] = sin(b)  => b = asin(M12)
            // R[1][1] = cos(a)*cos(b), R[2][1] = -sin(a)*cos(b) => a = atan2(-M32, M22)
            // R[0][0] = cos(b)*cos(c), R[0][2] = -cos(b)*sin(c) => c = atan2(-M13, M11)
            float sinB = Math.Clamp(rotMatrix.M12, -1f, 1f);
            float b = MathF.Asin(sinB);
            float cosB = MathF.Cos(b);

            float a, c;
            if (MathF.Abs(cosB) > 1e-6f)
            {
                a = MathF.Atan2(-rotMatrix.M32, rotMatrix.M22);
                c = MathF.Atan2(-rotMatrix.M13, rotMatrix.M11);
            }
            else
            {
                // Gimbal lock fallback
                a = MathF.Atan2(rotMatrix.M31, rotMatrix.M33);
                c = 0;
            }

            target.Translation = scaleFactor != 0
                ? new Vector3(translation.X / scaleFactor, translation.Y / scaleFactor, translation.Z / scaleFactor)
                : new Vector3(translation.X, translation.Y, translation.Z);
            target.Rotation = new Vector3(a, c, b); // X=a, Y=c, Z=b
            target.Scale = scale;
        }

        /// <summary>
        /// Builds skeleton nodes from FLVER bind-pose bones and converts them into the formal glTF basis.
        /// Deep-copies and modifies FLVER.Nodes (reparent + rename) before building the hierarchy.
        /// </summary>
        public static List<Node> BuildSkeletonHierarchyFromFlver(FLVER2 flver, Node rootNode, float scaleFactor)
        {
            return BuildSkeletonHierarchyFromFlver(flver, rootNode, scaleFactor, out _);
        }

        public static List<Node> BuildSkeletonHierarchyFromFlver(FLVER2 flver, Node rootNode, float scaleFactor, out List<FLVER.Node> modifiedNodes)
        {
            modifiedNodes = BuildModifiedFlverNodes(flver.Nodes, scaleFactor);
            var nodes = modifiedNodes;

            return BuildSkeletonHierarchy(
                nodes.Count,
                i => !string.IsNullOrEmpty(nodes[i].Name) ? nodes[i].Name : $"Bone_{i}",
                i => nodes[i].ParentIndex,
                i =>
                {
                    var localMatrix = ComputeSourceLocalMatrix(nodes[i], scaleFactor);
                    return ConvertSourceMatrixToGltf(localMatrix);
                },
                rootNode);
        }

        /// <summary>
        /// Builds modified parallel arrays from HKX skeleton data (reparent + rename + transform recalc).
        /// Returns (names[], parentIndices[], sourceLocalMatrices[]).
        /// </summary>
        public static (string[] Names, short[] ParentIndices, Matrix4x4[] SourceLocals) BuildModifiedHkxBoneArrays(
            HKX.HKASkeleton skeleton, float scaleFactor)
        {
            int boneCount = (int)skeleton.Bones.Size;
            var names = new string[boneCount];
            var parentIndices = new short[boneCount];
            var sourceLocals = new Matrix4x4[boneCount];

            for (int i = 0; i < boneCount; i++)
            {
                names[i] = skeleton.Bones[i].Name.GetString() ?? $"Bone_{i}";
                parentIndices[i] = skeleton.ParentIndices[i].data;

                var transform = skeleton.Transforms[i];
                var pos = transform.Position.Vector;
                var rot = transform.Rotation.Vector;
                var scl = transform.Scale.Vector;
                sourceLocals[i] = CreateLocalMatrix(
                    new Vector3(pos.X * scaleFactor, pos.Y * scaleFactor, pos.Z * scaleFactor),
                    new Quaternion(rot.X, rot.Y, rot.Z, rot.W),
                    new Vector3(scl.X, scl.Y, scl.Z));
            }

            // Apply reparent
            foreach (var (boneName, newParentName) in BoneReparentRules)
            {
                int boneIdx = Array.FindIndex(names, n => string.Equals(n, boneName, StringComparison.Ordinal));
                if (boneIdx < 0) { Console.WriteLine($"    [BoneTransform/HKX] reparent source '{boneName}' not found, skipping."); continue; }

                int newParentIdx = Array.FindIndex(names, n => string.Equals(n, newParentName, StringComparison.Ordinal));
                if (newParentIdx < 0) { Console.WriteLine($"    [BoneTransform/HKX] reparent target '{newParentName}' not found, skipping."); continue; }

                Console.WriteLine($"    [BoneTransform/HKX] Reparent '{boneName}' from parent={parentIndices[boneIdx]} to '{newParentName}' (idx={newParentIdx})");

                if (Matrix4x4.Invert(sourceLocals[newParentIdx], out var invParentLocal))
                    sourceLocals[boneIdx] = sourceLocals[boneIdx] * invParentLocal;
                else
                    Console.WriteLine($"    [BoneTransform/HKX] WARNING: cannot invert parent local for '{boneName}', keeping original.");

                parentIndices[boneIdx] = (short)newParentIdx;
            }

            // Apply rename
            for (int i = 0; i < boneCount; i++)
            {
                if (BoneRenameRules.TryGetValue(names[i], out string newName))
                {
                    Console.WriteLine($"    [BoneTransform/HKX] Rename '{names[i]}' -> '{newName}'");
                    names[i] = newName;
                }
            }

            return (names, parentIndices, sourceLocals);
        }

        /// <summary>
        /// Builds skeleton nodes from HKX bind-pose transforms and converts them into the formal glTF basis.
        /// </summary>
        public static List<Node> BuildSkeletonHierarchyFromHkx(HKX.HKASkeleton skeleton, Node rootNode, float scaleFactor)
        {
            return BuildSkeletonHierarchyFromHkx(skeleton, rootNode, scaleFactor,
                out _, out _, out _);
        }

        public static List<Node> BuildSkeletonHierarchyFromHkx(HKX.HKASkeleton skeleton, Node rootNode, float scaleFactor,
            out string[] modifiedNames, out short[] modifiedParentIndices, out Matrix4x4[] modifiedSourceLocals)
        {
            var (names, parentIndices, sourceLocals) = BuildModifiedHkxBoneArrays(skeleton, scaleFactor);
            modifiedNames = names;
            modifiedParentIndices = parentIndices;
            modifiedSourceLocals = sourceLocals;

            return BuildSkeletonHierarchy(
                names.Length,
                i => names[i],
                i => parentIndices[i],
                i => ConvertSourceMatrixToGltf(sourceLocals[i]),
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

                string boneName = GetRenamedBoneName(flver.Nodes[globalBoneIdx].Name);
                if (string.IsNullOrWhiteSpace(boneName))
                    boneName = $"Bone_{globalBoneIdx}";

                if (!sceneBoneNames.Contains(boneName))
                    continue;

                var bone = new Bone { Name = boneName };

                // Prefer skeleton node tree for world matrix (ensures IBM matches skeleton hierarchy).
                // Falls back to FLVER parent chain if node not found.
                // Note: the OffsetMatrix here is a hint for Assimp. The final IBM written to glTF
                // is always recomputed from the skeleton node graph in GltfSceneWriter.BuildSkinContext
                // to ensure consistency with the skeleton hierarchy (avoiding FLVER/HKX bind pose mismatches).
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
        /// Computes the world matrix for an Assimp node by walking up the node tree.
        /// Returns the matrix in Numerics format (same space as the node transforms, i.e. glTF space).
        /// </summary>
        public static Matrix4x4 ComputeAssimpNodeWorldMatrix(Node node)
        {
            var result = Matrix4x4.Identity;
            var current = node;
            while (current != null)
            {
                result *= ToNumericsMatrix(current.Transform);
                current = current.Parent;
            }
            return result;
        }

        /// <summary>
        /// Builds a name-to-node lookup from the skeleton bone node list.
        /// </summary>
        public static Dictionary<string, Node> BuildBoneNodeMap(IReadOnlyList<Node> boneNodes)
        {
            var map = new Dictionary<string, Node>(StringComparer.Ordinal);
            foreach (var node in boneNodes)
            {
                if (node != null && !string.IsNullOrEmpty(node.Name))
                    map[node.Name] = node;
            }
            return map;
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

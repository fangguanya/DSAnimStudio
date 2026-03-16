using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Assimp;
using SoulsFormats;
using Matrix4x4 = System.Numerics.Matrix4x4;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports FLVER2 models to FBX format using AssimpNet.
    /// Handles mesh, skeleton, bone weights, materials, and UV data.
    /// </summary>
    public class FlverToFbxExporter
    {
        public class ExportOptions
        {
            /// <summary>Scale factor applied to all positions (default 1.0)</summary>
            public float ScaleFactor { get; set; } = 1.0f;

            /// <summary>If true, flip Z axis for right-hand to left-hand conversion</summary>
            public bool ConvertCoordinateSystem { get; set; } = false;

            /// <summary>Export format ID for Assimp (default "collada" as FBX has issues with large bone counts)</summary>
            public string ExportFormatId { get; set; } = "collada";
        }

        private readonly ExportOptions _options;

        public FlverToFbxExporter(ExportOptions options = null)
        {
            _options = options ?? new ExportOptions();
        }

        /// <summary>
        /// Export a FLVER2 model to FBX file.
        /// </summary>
        public void Export(FLVER2 flver, string outputPath)
        {
            var scene = new Scene();
            scene.RootNode = new Node("RootNode");

            // 1. Build skeleton hierarchy
            var boneNodes = BuildSkeletonHierarchy(flver, scene.RootNode);

            // 2. Build meshes with bone weights
            var meshParentNode = new Node("Meshes", scene.RootNode);
            scene.RootNode.Children.Add(meshParentNode);

            for (int meshIdx = 0; meshIdx < flver.Meshes.Count; meshIdx++)
            {
                var flverMesh = flver.Meshes[meshIdx];

                // Skip meshes with no vertices
                if (flverMesh.Vertices == null || flverMesh.Vertices.Count == 0)
                    continue;

                // Get material
                var material = CreateMaterial(flver, flverMesh.MaterialIndex, scene);

                // Build Assimp mesh
                var assimpMesh = BuildMesh(flver, flverMesh, meshIdx, boneNodes);
                if (assimpMesh == null) continue;

                assimpMesh.MaterialIndex = scene.Materials.IndexOf(material);
                if (assimpMesh.MaterialIndex < 0)
                {
                    scene.Materials.Add(material);
                    assimpMesh.MaterialIndex = scene.Materials.Count - 1;
                }

                scene.Meshes.Add(assimpMesh);

                // Create mesh node
                var meshNode = new Node($"Mesh_{meshIdx}", meshParentNode);
                meshNode.MeshIndices.Add(scene.Meshes.Count - 1);
                meshParentNode.Children.Add(meshNode);
            }

            // Ensure at least one material
            if (scene.Materials.Count == 0)
            {
                scene.Materials.Add(new Material { Name = "DefaultMaterial" });
            }

            // 3. Export
            using (var ctx = new AssimpContext())
            {
                var dir = Path.GetDirectoryName(outputPath);
                if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                    Directory.CreateDirectory(dir);

                Console.WriteLine($"    Assimp scene: {scene.Meshes.Count} meshes, {scene.Materials.Count} materials, {scene.RootNode?.Children?.Count} children");
                foreach (var m in scene.Meshes)
                {
                    Console.WriteLine($"      {m.Name}: {m.VertexCount} verts, {m.FaceCount} faces, {m.BoneCount} bones, hasNormals={m.HasNormals}, hasTangents={m.HasTangentBasis}");
                    if (m.VertexCount > 0)
                    {
                        // Check for NaN
                        bool hasNaN = m.Vertices.Any(v => float.IsNaN(v.X) || float.IsNaN(v.Y) || float.IsNaN(v.Z));
                        if (hasNaN) Console.WriteLine($"        WARNING: has NaN vertices!");
                        if (m.HasNormals)
                        {
                            bool normNaN = m.Normals.Any(v => float.IsNaN(v.X) || float.IsNaN(v.Y) || float.IsNaN(v.Z));
                            if (normNaN) Console.WriteLine($"        WARNING: has NaN normals!");
                        }
                    }
                    // Check face indices in range
                    bool faceOOR = m.Faces.Any(f => f.Indices.Any(idx => idx < 0 || idx >= m.VertexCount));
                    if (faceOOR) Console.WriteLine($"        WARNING: face indices out of range!");
                }

                // Try different formats
                string[] formatsToTry = { _options.ExportFormatId, "collada", "obj" };
                bool exported = false;
                foreach (var fmt in formatsToTry)
                {
                    string tryPath = fmt == _options.ExportFormatId
                        ? outputPath
                        : Path.ChangeExtension(outputPath, fmt == "collada" ? ".dae" : ".obj");
                    bool result = ctx.ExportFile(scene, tryPath, fmt);
                    Console.WriteLine($"    Export format '{fmt}': {(result ? "SUCCESS" : "FAILED")} -> {tryPath}");
                    if (result) { exported = true; break; }
                }
                if (!exported)
                {
                    // Try fbx without bones
                    foreach (var m in scene.Meshes) m.Bones.Clear();
                    bool noBoneResult = ctx.ExportFile(scene, outputPath, _options.ExportFormatId);
                    Console.WriteLine($"    Export fbx without bones: {(noBoneResult ? "SUCCESS" : "FAILED")}");
                    if (!noBoneResult)
                        throw new Exception("All export formats failed");
                }
            }
        }

        /// <summary>
        /// Build the bone hierarchy as Assimp nodes attached to the root.
        /// </summary>
        private List<Node> BuildSkeletonHierarchy(FLVER2 flver, Node rootNode)
        {
            var armatureNode = new Node("Armature", rootNode);
            rootNode.Children.Add(armatureNode);

            var boneNodes = new List<Node>(new Node[flver.Nodes.Count]);
            float scale = _options.ScaleFactor;

            // Create all bone nodes
            for (int i = 0; i < flver.Nodes.Count; i++)
            {
                var flverBone = flver.Nodes[i];
                var boneName = !string.IsNullOrEmpty(flverBone.Name) ? flverBone.Name : $"Bone_{i}";
                var boneNode = new Node(boneName);

                // Build local transform matrix from FLVER bone:
                // Scale * RotX * RotZ * RotY * Translation
                var s = Matrix4x4.CreateScale(
                    flverBone.Scale.X, flverBone.Scale.Y, flverBone.Scale.Z);
                var rx = Matrix4x4.CreateRotationX(flverBone.Rotation.X);
                var rz = Matrix4x4.CreateRotationZ(flverBone.Rotation.Z);
                var ry = Matrix4x4.CreateRotationY(flverBone.Rotation.Y);
                var t = Matrix4x4.CreateTranslation(
                    flverBone.Translation.X * scale,
                    flverBone.Translation.Y * scale,
                    flverBone.Translation.Z * scale);

                var localMatrix = s * rx * rz * ry * t;

                if (_options.ConvertCoordinateSystem)
                {
                    // Flip Z for coordinate system conversion
                    localMatrix.M31 = -localMatrix.M31;
                    localMatrix.M32 = -localMatrix.M32;
                    localMatrix.M13 = -localMatrix.M13;
                    localMatrix.M23 = -localMatrix.M23;
                    localMatrix.M43 = -localMatrix.M43;
                }

                boneNode.Transform = ToAssimpMatrix(localMatrix);
                boneNodes[i] = boneNode;
            }

            // Build parent-child hierarchy
            for (int i = 0; i < flver.Nodes.Count; i++)
            {
                var parentIdx = flver.Nodes[i].ParentIndex;
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
        /// Build an Assimp mesh from a FLVER mesh, including vertices, faces, UVs, and bone weights.
        /// </summary>
        private Mesh BuildMesh(FLVER2 flver, FLVER2.Mesh flverMesh, int meshIdx, List<Node> boneNodes)
        {
            var mesh = new Mesh($"Mesh_{meshIdx}", PrimitiveType.Triangle);
            float scale = _options.ScaleFactor;
            bool useGlobalBoneIndices = flver.Header.Version > 0x2000D;

            // --- Vertices ---
            foreach (var vert in flverMesh.Vertices)
            {
                // Position
                var pos = new Vector3D(
                    vert.Position.X * scale,
                    vert.Position.Y * scale,
                    vert.Position.Z * scale);
                mesh.Vertices.Add(pos);

                // Normal
                var norm = new Vector3D(vert.Normal.X, vert.Normal.Y, vert.Normal.Z);
                mesh.Normals.Add(norm);

                // Tangent (first tangent if available)
                if (vert.Tangents != null && vert.Tangents.Count > 0)
                {
                    var tang = vert.Tangents[0];
                    mesh.Tangents.Add(new Vector3D(tang.X, tang.Y, tang.Z));
                    mesh.BiTangents.Add(new Vector3D(
                        vert.Bitangent.X, vert.Bitangent.Y, vert.Bitangent.Z));
                }

                // Vertex colors
                if (vert.Colors != null && vert.Colors.Count > 0)
                {
                    if (mesh.VertexColorChannelCount == 0)
                        mesh.VertexColorChannels[0] = new List<Color4D>();
                    var c = vert.Colors[0];
                    mesh.VertexColorChannels[0].Add(new Color4D(c.R, c.G, c.B, c.A));
                }
            }

            // --- UVs (up to 8 channels) ---
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
                            // FLVER UV: X=U, Y=V (V may need flipping for some apps)
                            mesh.TextureCoordinateChannels[uvIdx].Add(
                                new Vector3D(uv.X, 1.0f - uv.Y, 0));
                        }
                        else
                        {
                            mesh.TextureCoordinateChannels[uvIdx].Add(new Vector3D(0, 0, 0));
                        }
                    }
                }
            }

            // --- Validate tangent/bitangent counts must match vertex count ---
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

            // Validate vertex color count
            if (mesh.VertexColorChannels[0] != null && mesh.VertexColorChannels[0].Count > 0
                && mesh.VertexColorChannels[0].Count != mesh.Vertices.Count)
            {
                mesh.VertexColorChannels[0].Clear();
            }

            // --- Faces (from FaceSet LOD 0) ---
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
                // Convert triangle strip to triangle list
                for (int i = 0; i < indices.Count - 2; i++)
                {
                    int i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
                    if (i0 == i1 || i1 == i2 || i0 == i2) continue; // Degenerate
                    if (i % 2 == 0)
                        mesh.Faces.Add(new Face(new int[] { i0, i1, i2 }));
                    else
                        mesh.Faces.Add(new Face(new int[] { i0, i2, i1 }));
                }
            }
            else
            {
                // Triangle list
                for (int i = 0; i + 2 < indices.Count; i += 3)
                {
                    mesh.Faces.Add(new Face(new int[] { indices[i], indices[i + 1], indices[i + 2] }));
                }
            }

            // --- Bone Weights ---
            var boneBuckets = new Dictionary<int, List<VertexWeight>>();

            for (int vertIdx = 0; vertIdx < flverMesh.Vertices.Count; vertIdx++)
            {
                var vert = flverMesh.Vertices[vertIdx];

                if (flverMesh.UseBoneWeights)
                {
                    // Multi-bone skinning
                    for (int w = 0; w < 4; w++)
                    {
                        float weight = vert.BoneWeights[w];
                        if (weight <= 0) continue;

                        int localBoneIdx = vert.BoneIndices[w];
                        int globalBoneIdx;

                        if (useGlobalBoneIndices)
                        {
                            globalBoneIdx = localBoneIdx;
                        }
                        else
                        {
                            // Map per-mesh bone index to global bone index
                            if (localBoneIdx >= 0 && localBoneIdx < flverMesh.BoneIndices.Count)
                                globalBoneIdx = flverMesh.BoneIndices[localBoneIdx];
                            else
                                globalBoneIdx = localBoneIdx;
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
                    // Single bone binding via NormalW
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

            // Convert bone buckets to Assimp Bones
            foreach (var kvp in boneBuckets)
            {
                int globalBoneIdx = kvp.Key;
                var weights = kvp.Value;

                if (globalBoneIdx >= boneNodes.Count || boneNodes[globalBoneIdx] == null)
                    continue;

                var bone = new Bone();
                bone.Name = boneNodes[globalBoneIdx].Name;

                // Compute offset matrix = inverse of bone's world transform
                var worldMatrix = ComputeWorldMatrix(flver.Nodes, globalBoneIdx, scale);
                Matrix4x4.Invert(worldMatrix, out var inverseWorld);
                bone.OffsetMatrix = ToAssimpMatrix(inverseWorld);

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

            // Check if already added
            var existing = scene.Materials.FirstOrDefault(m => m.Name == flverMat.Name);
            if (existing != null) return existing;

            var mat = new Material();
            mat.Name = !string.IsNullOrEmpty(flverMat.Name) ? flverMat.Name : $"Material_{materialIndex}";

            // Map textures
            foreach (var tex in flverMat.Textures)
            {
                if (string.IsNullOrEmpty(tex.Path)) continue;

                string texFileName = GetTextureFileName(tex.Path);
                var typeUpper = tex.Type?.ToUpper() ?? "";

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
                result = result * localMatrix;

                idx = bone.ParentIndex;
            }

            return result;
        }

        /// <summary>
        /// Extract the texture file name from a FLVER internal path.
        /// e.g. "N:\\SPRJ\\data\\Model\\chr\\c0000\\c0000_a.tga" -> "c0000_a"
        /// </summary>
        public static string GetTextureFileName(string internalPath)
        {
            if (string.IsNullOrEmpty(internalPath)) return "";
            var name = Path.GetFileNameWithoutExtension(internalPath);
            return name ?? "";
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

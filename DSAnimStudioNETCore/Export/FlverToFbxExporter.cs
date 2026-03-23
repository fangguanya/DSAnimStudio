using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Assimp;
using SoulsFormats;
using HKX = SoulsAssetPipeline.Animation.HKX;
using Matrix4x4 = System.Numerics.Matrix4x4;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports FLVER2 models to glTF 2.0.
    /// This class now only coordinates model export flow; all shared scene construction
    /// details live in <see cref="FormalSceneExportShared"/> so model and animation export
    /// always use the exact same skeleton, mesh, material, and bind-pose logic.
    /// </summary>
    public class FlverToFbxExporter
    {
        public class ExportOptions
        {
            /// <summary>Scale factor applied to all positions (default 1.0).</summary>
            public float ScaleFactor { get; set; } = 1.0f;

            /// <summary>Retained for compatibility with older callers. Formal export always writes glTF 2.0 directly.</summary>
            public string ExportFormatId { get; set; } = "gltf2";
        }

        private readonly ExportOptions _options;

        public FlverToFbxExporter(ExportOptions options = null)
        {
            _options = options ?? new ExportOptions();
        }

        /// <summary>
        /// Exports a single FLVER model to glTF 2.0.
        /// </summary>
        public void Export(FLVER2 flver, string outputPath)
        {
            Export(new[] { flver }, skeletonHkx: null, outputPath);
        }

        /// <summary>
        /// Exports one or more FLVERs as a single formal glTF scene.
        /// When HKX skeleton data is available it is preferred as the authoritative skeleton source;
        /// otherwise the first FLVER node hierarchy is used as the skeleton basis.
        /// </summary>
        public void Export(IReadOnlyList<FLVER2> flvers, HKX skeletonHkx, string outputPath)
        {
            if (flvers == null || flvers.Count == 0)
                throw new ArgumentException("At least one FLVER is required.", nameof(flvers));

            var validFlvers = flvers.Where(f => f != null).ToList();
            if (validFlvers.Count == 0)
                throw new ArgumentException("At least one non-null FLVER is required.", nameof(flvers));

            var scene = new Scene();
            scene.RootNode = new Node("RootNode");
            scene.RootNode.Transform = FormalSceneExportShared.ToAssimpMatrix(FormalSceneExportShared.RootNodeCorrectionGltf);

            var sceneSkeleton = FormalSceneExportShared.ExtractHkxSkeleton(skeletonHkx);
            var boneNodes = sceneSkeleton != null
                ? FormalSceneExportShared.BuildSkeletonHierarchyFromHkx(sceneSkeleton, scene.RootNode, _options.ScaleFactor)
                : FormalSceneExportShared.BuildSkeletonHierarchyFromFlver(validFlvers[0], scene.RootNode, _options.ScaleFactor);

            FormalSceneExportShared.LogSkeletonSelfCheck(sceneSkeleton != null ? "HKX->FLVER" : "FLVER", boneNodes);

            var sceneBoneNames = new HashSet<string>(
                boneNodes.Where(node => node != null).Select(node => node.Name),
                StringComparer.Ordinal);
            string formalSkeletonRootName = FormalSceneExportShared.ResolveFormalSkeletonRootName(boneNodes, scene.RootNode, "model");

            FormalSceneExportShared.AppendSceneMeshes(scene, validFlvers, sceneBoneNames, _options.ScaleFactor);

            if (scene.Materials.Count == 0)
                scene.Materials.Add(new Material { Name = "DefaultMaterial" });

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
    }
}

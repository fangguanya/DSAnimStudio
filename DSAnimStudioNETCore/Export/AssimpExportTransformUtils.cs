using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Assimp;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Quaternion = System.Numerics.Quaternion;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudio.Export
{
    internal static class AssimpExportTransformUtils
    {
        public static readonly Matrix4x4 SourceToGltfBasis = new Matrix4x4(
            0, 1, 0, 0,
            -1, 0, 0, 0,
            0, 0, -1, 0,
            0, 0, 0, 1);

        public static readonly Matrix4x4 GltfToSourceBasis = Matrix4x4.Transpose(SourceToGltfBasis);

        public static readonly Matrix4x4 PostTransformCorrectionGltf =
            Matrix4x4.CreateRotationX(MathF.PI / 2)
            * Matrix4x4.CreateRotationY(MathF.PI / 2);

        public static readonly Matrix4x4 SourceToGltfTransform = SourceToGltfBasis * PostTransformCorrectionGltf;

        public static readonly Matrix4x4 GltfToSourceTransform = Matrix4x4.Transpose(SourceToGltfTransform);

        public static readonly Matrix4x4 RootNodeCorrectionGltf = Matrix4x4.Identity;

        public static Vector3 ConvertSourceVectorToGltf(Vector3 value)
        {
            return Vector3.Transform(value, SourceToGltfTransform);
        }

        public static Matrix4x4 ConvertSourceMatrixToGltf(Matrix4x4 value)
        {
            return GltfToSourceTransform * value * SourceToGltfTransform;
        }

        public static Matrix4x4 CreateLocalMatrix(Vector3 translation, Quaternion rotation, Vector3 scale)
        {
            return Matrix4x4.CreateScale(scale)
                * Matrix4x4.CreateFromQuaternion(rotation)
                * Matrix4x4.CreateTranslation(translation);
        }

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

        public static Assimp.Matrix4x4 ToAssimpMatrix(Matrix4x4 value)
        {
            return new Assimp.Matrix4x4(
                value.M11, value.M21, value.M31, value.M41,
                value.M12, value.M22, value.M32, value.M42,
                value.M13, value.M23, value.M33, value.M43,
                value.M14, value.M24, value.M34, value.M44);
        }

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
            var rootWorld = ComputeWorldMatrix(rootBone, boneNodes);
            var rootForward = Vector3.Normalize(Vector3.TransformNormal(Vector3.UnitZ, rootWorld));

            Console.WriteLine($"    Skeleton self-check [{label}]: root='{rootBone.Name}', forward(+Z)={FormatVector(rootForward)}");
            Console.WriteLine($"      local={FormatMatrix(rootLocal)}");
            Console.WriteLine($"      world={FormatMatrix(rootWorld)}");
        }

        private static Matrix4x4 ComputeWorldMatrix(Node node, IReadOnlyList<Node> boneNodes)
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
            {
                result *= ToNumericsMatrix(lineage.Pop().Transform);
            }

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

        public static Matrix4x4 ToNumericsMatrix(Assimp.Matrix4x4 value)
        {
            return new Matrix4x4(
                value.A1, value.B1, value.C1, value.D1,
                value.A2, value.B2, value.C2, value.D2,
                value.A3, value.B3, value.C3, value.D3,
                value.A4, value.B4, value.C4, value.D4);
        }

        private static string FormatMatrix(Matrix4x4 value)
        {
            return string.Create(CultureInfo.InvariantCulture, $"[{value.M11:F3}, {value.M12:F3}, {value.M13:F3}, {value.M14:F3}; {value.M21:F3}, {value.M22:F3}, {value.M23:F3}, {value.M24:F3}; {value.M31:F3}, {value.M32:F3}, {value.M33:F3}, {value.M34:F3}; {value.M41:F3}, {value.M42:F3}, {value.M43:F3}, {value.M44:F3}]");
        }

        private static string FormatVector(Vector3 value)
        {
            return string.Create(CultureInfo.InvariantCulture, $"({value.X:F3}, {value.Y:F3}, {value.Z:F3})");
        }
    }
}
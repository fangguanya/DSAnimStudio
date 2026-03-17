using System;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json.Linq;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Quaternion = System.Numerics.Quaternion;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudio.Export
{
    internal static class GltfFormalHumanoidSkeletonValidator
    {
        private const float DirectionThreshold = 0.75f;

        private static readonly BoneRequirement[] RequiredBones =
        {
            new BoneRequirement("Pelvis", "Pelvis"),
            new BoneRequirement("Spine", "Spine"),
            new BoneRequirement("Spine1", "Spine1"),
            new BoneRequirement("UpperChest", "Spine2", "Chest"),
            new BoneRequirement("Neck", "Neck"),
            new BoneRequirement("Head", "Head"),
            new BoneRequirement("L_Shoulder", "L_Shoulder"),
            new BoneRequirement("R_Shoulder", "R_Shoulder"),
            new BoneRequirement("L_Hand", "L_Hand"),
            new BoneRequirement("R_Hand", "R_Hand"),
        };

        internal static bool ShouldValidate(JObject root, string expectedSkeletonRootName = null)
        {
            try
            {
                var context = CreateContext(root, expectedSkeletonRootName);
                var nodeNames = context.DescendantNodeIndices
                    .Select(index => GetNodeName(context.Nodes, index))
                    .Where(name => !string.IsNullOrWhiteSpace(name))
                    .ToHashSet(StringComparer.Ordinal);

                int markerCount = 0;
                foreach (var requirement in RequiredBones)
                {
                    if (requirement.Aliases.Any(nodeNames.Contains))
                        markerCount++;
                }

                return markerCount >= 4;
            }
            catch
            {
                return false;
            }
        }

        internal static FormalHumanoidSkeletonMetrics Analyze(JObject root, string expectedSkeletonRootName = null)
        {
            var context = CreateContext(root, expectedSkeletonRootName);
            var resolvedBones = ResolveRequiredBones(context);

            Vector3 pelvisPosition = GetWorldPosition(context, resolvedBones["Pelvis"]);
            Vector3 upperChestPosition = GetWorldPosition(context, resolvedBones["UpperChest"]);
            Vector3 headPosition = GetWorldPosition(context, resolvedBones["Head"]);
            Vector3 leftShoulderPosition = GetWorldPosition(context, resolvedBones["L_Shoulder"]);
            Vector3 rightShoulderPosition = GetWorldPosition(context, resolvedBones["R_Shoulder"]);
            Vector3 rightHandPosition = GetWorldPosition(context, resolvedBones["R_Hand"]);

            Vector3 upDirection = NormalizeOrThrow(headPosition - pelvisPosition,
                "Humanoid up direction is degenerate because Pelvis and Head overlap.");
            Vector3 shoulderDirection = NormalizeOrThrow(rightShoulderPosition - leftShoulderPosition,
                "Humanoid shoulder direction is degenerate because L_Shoulder and R_Shoulder overlap.");
            Vector3 faceDirection = NormalizeOrThrow(Vector3.Cross(shoulderDirection, upDirection),
                "Humanoid face direction is degenerate because shoulder and up directions are parallel.");
            Vector3 rightHandDirection = NormalizeOrThrow(rightHandPosition - upperChestPosition,
                "Humanoid right-hand direction is degenerate because UpperChest and R_Hand overlap.");

            return new FormalHumanoidSkeletonMetrics(
                context.SkeletonRootIndex,
                resolvedBones,
                pelvisPosition,
                upperChestPosition,
                headPosition,
                shoulderDirection,
                upDirection,
                faceDirection,
                rightHandDirection);
        }

        internal static void Validate(JObject root, string gltfPath, string expectedSkeletonRootName = null)
        {
            var metrics = Analyze(root, expectedSkeletonRootName);
            var failures = new List<string>();

            float faceScore = Vector3.Dot(metrics.FaceDirection, Vector3.UnitY);
            if (faceScore < DirectionThreshold)
            {
                failures.Add($"Expected face +Y, got {FormatVector(metrics.FaceDirection)} (score {faceScore:F3})");
            }

            float rightHandScore = Vector3.Dot(metrics.RightHandDirection, -Vector3.UnitX);
            if (rightHandScore < DirectionThreshold)
            {
                failures.Add($"Expected R_Hand toward -X, got {FormatVector(metrics.RightHandDirection)} (score {rightHandScore:F3})");
            }

            if (failures.Count > 0)
            {
                throw new InvalidOperationException(
                    $"Formal humanoid skeleton orientation is invalid in '{gltfPath}'. {string.Join("; ", failures)}.");
            }
        }

        private static SkeletonContext CreateContext(JObject root, string expectedSkeletonRootName)
        {
            var nodes = root["nodes"] as JArray
                ?? throw new InvalidOperationException("glTF nodes are missing for humanoid skeleton validation.");
            var skins = root["skins"] as JArray;
            var parents = BuildParentMap(nodes);
            var depths = BuildDepthMap(nodes, parents);

            int skeletonRootIndex = -1;
            if (skins != null)
            {
                foreach (var skinObj in skins.OfType<JObject>())
                {
                    var joints = skinObj["joints"] as JArray;
                    if (joints == null || joints.Count == 0)
                        continue;

                    skeletonRootIndex = (int?)skinObj["skeleton"] ?? -1;
                    if (skeletonRootIndex < 0)
                        skeletonRootIndex = DetermineSkinRoot(nodes, joints, parents, depths, expectedSkeletonRootName);

                    break;
                }
            }

            if (skeletonRootIndex < 0)
            {
                if (string.IsNullOrWhiteSpace(expectedSkeletonRootName))
                    throw new InvalidOperationException("Formal humanoid skeleton validation requires a glTF skin or declared skeleton root.");

                skeletonRootIndex = ResolveDeclaredSkeletonRootIndex(nodes, expectedSkeletonRootName);
            }

            var descendantNodeIndices = new HashSet<int>();
            for (int nodeIndex = 0; nodeIndex < nodes.Count; nodeIndex++)
            {
                if (IsNodeDescendantOf(nodeIndex, skeletonRootIndex, parents))
                    descendantNodeIndices.Add(nodeIndex);
            }

            return new SkeletonContext(nodes, parents, descendantNodeIndices, skeletonRootIndex);
        }

        private static Dictionary<string, int> ResolveRequiredBones(SkeletonContext context)
        {
            var resolved = new Dictionary<string, int>(StringComparer.Ordinal);
            var missing = new List<string>();

            foreach (var requirement in RequiredBones)
            {
                int? nodeIndex = ResolveNamedBone(context, requirement.Aliases);
                if (nodeIndex.HasValue)
                    resolved.Add(requirement.CanonicalName, nodeIndex.Value);
                else
                    missing.Add(requirement.DisplayName);
            }

            if (missing.Count > 0)
            {
                throw new InvalidOperationException(
                    "Formal humanoid skeleton is missing required head/chest/armature bones: " +
                    string.Join(", ", missing) + ".");
            }

            return resolved;
        }

        private static int? ResolveNamedBone(SkeletonContext context, IEnumerable<string> aliases)
        {
            foreach (int nodeIndex in context.DescendantNodeIndices)
            {
                string nodeName = GetNodeName(context.Nodes, nodeIndex);
                if (aliases.Any(alias => string.Equals(alias, nodeName, StringComparison.Ordinal)))
                    return nodeIndex;
            }

            return null;
        }

        private static Vector3 GetWorldPosition(SkeletonContext context, int nodeIndex)
        {
            Matrix4x4 world = ComputeWorldMatrix(context.Nodes, context.Parents, nodeIndex);
            return new Vector3(world.M41, world.M42, world.M43);
        }

        private static string FormatVector(Vector3 value)
        {
            return $"({value.X:F3}, {value.Y:F3}, {value.Z:F3})";
        }

        private static Vector3 NormalizeOrThrow(Vector3 value, string message)
        {
            if (value.LengthSquared() <= 1e-8f)
                throw new InvalidOperationException(message);

            return Vector3.Normalize(value);
        }

        private static Dictionary<int, int> BuildParentMap(JArray nodes)
        {
            var parents = new Dictionary<int, int>();
            for (int nodeIndex = 0; nodeIndex < nodes.Count; nodeIndex++)
            {
                var node = nodes[nodeIndex] as JObject;
                var children = node?["children"] as JArray;
                if (children == null)
                    continue;

                foreach (int child in children.Values<int>())
                {
                    if (!parents.ContainsKey(child))
                        parents.Add(child, nodeIndex);
                }
            }

            return parents;
        }

        private static Dictionary<int, int> BuildDepthMap(JArray nodes, Dictionary<int, int> parents)
        {
            var depths = new Dictionary<int, int>(nodes.Count);
            for (int nodeIndex = 0; nodeIndex < nodes.Count; nodeIndex++)
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

            return depths;
        }

        private static int DetermineSkinRoot(JArray nodes, JArray joints, Dictionary<int, int> parents,
            Dictionary<int, int> depths, string expectedSkeletonRootName = null)
        {
            if (!string.IsNullOrWhiteSpace(expectedSkeletonRootName))
                return ResolveDeclaredSkeletonRootIndex(nodes, expectedSkeletonRootName);

            var jointList = joints.Values<int>().ToList();
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
                    commonAncestors = ancestors;
                else
                    commonAncestors.IntersectWith(ancestors);
            }

            if (commonAncestors == null || commonAncestors.Count == 0)
                return jointList.Count > 0 ? jointList[0] : 0;

            int? preferred = commonAncestors
                .Where(idx => IsPreferredSkeletonRoot(nodes[idx] as JObject))
                .OrderByDescending(idx => depths.TryGetValue(idx, out int depth) ? depth : int.MinValue)
                .Cast<int?>()
                .FirstOrDefault();
            if (preferred.HasValue)
                return preferred.Value;

            return commonAncestors
                .OrderByDescending(idx => depths.TryGetValue(idx, out int depth) ? depth : int.MinValue)
                .First();
        }

        private static int ResolveDeclaredSkeletonRootIndex(JArray nodes, string expectedSkeletonRootName)
        {
            var matches = new List<int>();
            for (int nodeIndex = 0; nodeIndex < nodes.Count; nodeIndex++)
            {
                var node = nodes[nodeIndex] as JObject;
                if (node == null || node["mesh"] != null)
                    continue;

                string nodeName = (string)node["name"];
                if (string.Equals(nodeName, expectedSkeletonRootName, StringComparison.Ordinal))
                    matches.Add(nodeIndex);
            }

            if (matches.Count == 1)
                return matches[0];
            if (matches.Count == 0)
                throw new InvalidOperationException($"Declared formal skeleton root '{expectedSkeletonRootName}' was not found in glTF nodes.");

            throw new InvalidOperationException($"Declared formal skeleton root '{expectedSkeletonRootName}' matched multiple glTF nodes.");
        }

        private static bool IsNodeDescendantOf(int nodeIndex, int rootIndex, Dictionary<int, int> parents)
        {
            int current = nodeIndex;
            while (current >= 0)
            {
                if (current == rootIndex)
                    return true;

                if (!parents.TryGetValue(current, out current))
                    break;
            }

            return false;
        }

        private static string GetNodeName(JArray nodes, int nodeIndex)
        {
            if (nodeIndex < 0 || nodeIndex >= nodes.Count)
                return string.Empty;

            return (string)(nodes[nodeIndex] as JObject)?["name"] ?? string.Empty;
        }

        private static bool IsPreferredSkeletonRoot(JObject node)
        {
            if (node == null || node["mesh"] != null)
                return false;

            string name = (string)node["name"];
            if (string.IsNullOrWhiteSpace(name))
                return false;

            return !string.Equals(name, "Armature", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(name, "RootNode", StringComparison.OrdinalIgnoreCase);
        }

        private static Matrix4x4 ComputeWorldMatrix(JArray nodes, Dictionary<int, int> parents, int nodeIndex)
        {
            var result = Matrix4x4.Identity;
            int current = nodeIndex;
            while (current >= 0 && current < nodes.Count)
            {
                var node = nodes[current] as JObject;
                if (node == null)
                    break;

                result *= GetLocalMatrix(node);
                if (!parents.TryGetValue(current, out current))
                    break;
            }

            return result;
        }

        private static Matrix4x4 GetLocalMatrix(JObject node)
        {
            var matrixArray = node["matrix"] as JArray;
            if (matrixArray != null && matrixArray.Count == 16)
                return GltfMatrixSerialization.ParseMatrix(matrixArray);

            Vector3 translation = ReadVector3(node["translation"] as JArray, Vector3.Zero);
            Quaternion rotation = ReadQuaternion(node["rotation"] as JArray, Quaternion.Identity);
            Vector3 scale = ReadVector3(node["scale"] as JArray, Vector3.One);
            return Matrix4x4.CreateScale(scale) * Matrix4x4.CreateFromQuaternion(rotation) * Matrix4x4.CreateTranslation(translation);
        }

        private static Vector3 ReadVector3(JArray array, Vector3 fallback)
        {
            if (array == null || array.Count < 3)
                return fallback;

            return new Vector3((float)array[0], (float)array[1], (float)array[2]);
        }

        private static Quaternion ReadQuaternion(JArray array, Quaternion fallback)
        {
            if (array == null || array.Count < 4)
                return fallback;

            return Quaternion.Normalize(new Quaternion((float)array[0], (float)array[1], (float)array[2], (float)array[3]));
        }

        internal sealed class FormalHumanoidSkeletonMetrics
        {
            internal FormalHumanoidSkeletonMetrics(
                int skeletonRootIndex,
                IReadOnlyDictionary<string, int> resolvedBones,
                Vector3 pelvisPosition,
                Vector3 upperChestPosition,
                Vector3 headPosition,
                Vector3 shoulderDirection,
                Vector3 upDirection,
                Vector3 faceDirection,
                Vector3 rightHandDirection)
            {
                SkeletonRootIndex = skeletonRootIndex;
                ResolvedBones = resolvedBones;
                PelvisPosition = pelvisPosition;
                UpperChestPosition = upperChestPosition;
                HeadPosition = headPosition;
                ShoulderDirection = shoulderDirection;
                UpDirection = upDirection;
                FaceDirection = faceDirection;
                RightHandDirection = rightHandDirection;
            }

            internal int SkeletonRootIndex { get; }
            internal IReadOnlyDictionary<string, int> ResolvedBones { get; }
            internal Vector3 PelvisPosition { get; }
            internal Vector3 UpperChestPosition { get; }
            internal Vector3 HeadPosition { get; }
            internal Vector3 ShoulderDirection { get; }
            internal Vector3 UpDirection { get; }
            internal Vector3 FaceDirection { get; }
            internal Vector3 RightHandDirection { get; }
        }

        private sealed class SkeletonContext
        {
            internal SkeletonContext(JArray nodes, Dictionary<int, int> parents, HashSet<int> descendantNodeIndices, int skeletonRootIndex)
            {
                Nodes = nodes;
                Parents = parents;
                DescendantNodeIndices = descendantNodeIndices;
                SkeletonRootIndex = skeletonRootIndex;
            }

            internal JArray Nodes { get; }
            internal Dictionary<int, int> Parents { get; }
            internal HashSet<int> DescendantNodeIndices { get; }
            internal int SkeletonRootIndex { get; }
        }

        private sealed class BoneRequirement
        {
            internal BoneRequirement(string canonicalName, params string[] aliases)
            {
                CanonicalName = canonicalName;
                Aliases = aliases;
            }

            internal string CanonicalName { get; }
            internal IReadOnlyList<string> Aliases { get; }
            internal string DisplayName => CanonicalName == "UpperChest"
                ? "UpperChest(Spine2/Chest)"
                : CanonicalName;
        }
    }
}
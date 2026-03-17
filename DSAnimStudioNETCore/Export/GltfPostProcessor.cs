using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Newtonsoft.Json.Linq;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Quaternion = System.Numerics.Quaternion;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudio.Export
{
    internal static class GltfPostProcessor
    {
        public static void PostProcess(string gltfPath, string animationName = null)
        {
            string json = File.ReadAllText(gltfPath);
            var root = JObject.Parse(json);
            bool modified = false;
            bool binModified = false;

            var buffers = root["buffers"] as JArray;
            if (buffers != null)
            {
                foreach (var bufferToken in buffers.OfType<JObject>())
                {
                    var uri = (string)bufferToken["uri"];
                    if (!string.IsNullOrEmpty(uri) && Path.IsPathRooted(uri))
                    {
                        bufferToken["uri"] = Path.GetFileName(uri.Replace('\\', '/'));
                        modified = true;
                    }
                }
            }

            byte[] binBytes = null;
            List<byte> binData = null;
            string binPath = null;
            if (buffers != null && buffers.Count > 0)
            {
                string binUri = (string)buffers[0]?["uri"];
                if (!string.IsNullOrEmpty(binUri))
                {
                    binPath = Path.Combine(Path.GetDirectoryName(gltfPath) ?? string.Empty, binUri);
                    if (File.Exists(binPath))
                    {
                        binBytes = File.ReadAllBytes(binPath);
                        binData = binBytes.ToList();
                    }
                }
            }

            var accessors = root["accessors"] as JArray;
            var bufferViews = root["bufferViews"] as JArray;
            var nodes = root["nodes"] as JArray;
            var skins = root["skins"] as JArray;
            var scenes = root["scenes"] as JArray;
            var animations = root["animations"] as JArray;

            if (binData != null)
            {
                foreach (int accessorIndex in CollectJointAccessorIndices(root))
                {
                    if (ConvertJointAccessorToUnsignedShort(accessorIndex, accessors, bufferViews, binBytes, binData))
                    {
                        modified = true;
                        binModified = true;
                    }
                }
            }

            if (nodes != null && skins != null && accessors != null && bufferViews != null && binData != null)
            {
                if (NormalizeSkinHierarchy(nodes, skins, accessors, bufferViews, binBytes, binData))
                {
                    modified = true;
                    binModified = true;
                }
            }

            if (animations != null && animations.Count > 0)
            {
                if (ConvertAnimatedNodesToTrs(nodes, animations))
                    modified = true;

                if (MergeAnimations(root, animationName))
                    modified = true;
            }

            if (nodes != null && scenes != null && scenes.Count > 0)
            {
                if (NormalizeSceneMeshRoots(nodes, scenes))
                    modified = true;
            }

            if (binModified && buffers != null && buffers.Count > 0 && binPath != null)
            {
                buffers[0]["byteLength"] = binData.Count;
                File.WriteAllBytes(binPath, binData.ToArray());
            }

            ValidateProcessedGltf(root, gltfPath);

            if (modified)
                File.WriteAllText(gltfPath, root.ToString());
        }

        private static void ValidateProcessedGltf(JObject root, string gltfPath)
        {
            var buffers = root["buffers"] as JArray;
            if (buffers != null)
            {
                foreach (var bufferToken in buffers.OfType<JObject>())
                {
                    var uri = (string)bufferToken["uri"];
                    if (!string.IsNullOrEmpty(uri) && Path.IsPathRooted(uri))
                        throw new InvalidOperationException($"glTF buffer URI remained absolute after post-process: {gltfPath}");
                }
            }

            var accessors = root["accessors"] as JArray;
            var skins = root["skins"] as JArray;
            if (skins != null)
            {
                foreach (var skinObj in skins.OfType<JObject>())
                {
                    int skeletonIndex = (int?)skinObj["skeleton"] ?? -1;
                    if (skeletonIndex < 0)
                        throw new InvalidOperationException("glTF skin is missing a formal skeleton root.");

                    var joints = skinObj["joints"] as JArray;
                    if (joints == null || joints.Count == 0)
                        throw new InvalidOperationException("glTF skin has no joints after post-process.");

                    int accessorIndex = (int?)skinObj["inverseBindMatrices"] ?? -1;
                    if (accessors == null || accessorIndex < 0 || accessorIndex >= accessors.Count)
                        throw new InvalidOperationException("glTF skin is missing inverse bind matrices.");

                    var accessor = accessors[accessorIndex] as JObject;
                    int accessorCount = (int?)accessor?["count"] ?? 0;
                    if (accessorCount != joints.Count)
                        throw new InvalidOperationException($"glTF inverse bind matrix count {accessorCount} did not match joint count {joints.Count}.");
                }
            }

            var animations = root["animations"] as JArray;
            if (animations != null && animations.Count > 0)
            {
                if (animations.Count != 1)
                    throw new InvalidOperationException($"Formal animation glTF must contain exactly one animation entry, found {animations.Count}.");

                var nodes = root["nodes"] as JArray;
                var channels = animations[0]?["channels"] as JArray;
                if (channels == null || channels.Count == 0)
                    throw new InvalidOperationException("Formal animation glTF contains no channels.");

                foreach (var channelObj in channels.OfType<JObject>())
                {
                    int nodeIndex = (int?)channelObj["target"]?["node"] ?? -1;
                    if (nodes == null || nodeIndex < 0 || nodeIndex >= nodes.Count)
                        throw new InvalidOperationException("Formal animation glTF references an invalid node.");
                    if (nodes[nodeIndex]?["matrix"] != null)
                        throw new InvalidOperationException($"Animated node {nodeIndex} still contains a matrix transform after post-process.");
                }
            }
        }

        private static bool ConvertJointAccessorToUnsignedShort(int accessorIndex, JArray accessors, JArray bufferViews,
            byte[] binBytes, List<byte> binData)
        {
            if (accessorIndex < 0 || accessorIndex >= accessors.Count)
                return false;

            var accessor = accessors[accessorIndex] as JObject;
            if (accessor == null || (int?)accessor["componentType"] != 5126)
                return false;

            string type = (string)accessor["type"];
            int components = type == "VEC4" ? 4 : 0;
            int count = (int?)accessor["count"] ?? 0;
            int bufferViewIndex = (int?)accessor["bufferView"] ?? -1;
            if (components == 0 || count <= 0 || bufferViewIndex < 0 || bufferViewIndex >= bufferViews.Count)
                return false;

            var bufferView = bufferViews[bufferViewIndex] as JObject;
            if (bufferView == null)
                return false;

            int viewOffset = (int?)bufferView["byteOffset"] ?? 0;
            int accessorOffset = (int?)accessor["byteOffset"] ?? 0;
            int srcOffset = viewOffset + accessorOffset;
            int srcLength = count * components * sizeof(float);
            if (srcOffset < 0 || srcOffset + srcLength > binBytes.Length)
                return false;

            int dstOffset = AlignTo4(binData.Count);
            while (binData.Count < dstOffset)
                binData.Add(0);

            for (int i = 0; i < count * components; i++)
            {
                float value = BitConverter.ToSingle(binBytes, srcOffset + (i * sizeof(float)));
                ushort jointIndex = value < 0 ? (ushort)0 : (ushort)Math.Clamp((int)Math.Round(value), 0, ushort.MaxValue);
                binData.Add((byte)(jointIndex & 0xFF));
                binData.Add((byte)((jointIndex >> 8) & 0xFF));
            }

            var newBufferView = new JObject
            {
                ["buffer"] = 0,
                ["byteOffset"] = dstOffset,
                ["byteLength"] = count * components * sizeof(ushort)
            };
            bufferViews.Add(newBufferView);

            accessor["bufferView"] = bufferViews.Count - 1;
            accessor["byteOffset"] = 0;
            accessor["componentType"] = 5123;
            accessor.Remove("min");
            accessor.Remove("max");
            return true;
        }

        private static bool NormalizeSkinHierarchy(JArray nodes, JArray skins, JArray accessors, JArray bufferViews,
            byte[] binBytes, List<byte> binData)
        {
            bool modified = false;
            var parents = BuildParentMap(nodes);
            var depths = BuildDepthMap(nodes, parents);

            foreach (var skinObj in skins.OfType<JObject>())
            {
                var joints = skinObj["joints"] as JArray;
                if (joints == null || joints.Count == 0)
                    continue;

                int skeletonRoot = DetermineSkinRoot(nodes, joints, parents, depths);
                if ((int?)skinObj["skeleton"] != skeletonRoot)
                {
                    skinObj["skeleton"] = skeletonRoot;
                    modified = true;
                }

                var fullJointList = CollectSkeletonJointNodes(nodes, skeletonRoot);
                if (fullJointList.Count == 0)
                    continue;

                int? existingAccessorIndex = (int?)skinObj["inverseBindMatrices"];
                var matricesByJoint = ReadExistingInverseBindMatrices(joints, existingAccessorIndex, accessors, bufferViews, binBytes);
                var finalMatrices = new List<Matrix4x4>(fullJointList.Count);
                foreach (int jointIndex in fullJointList)
                {
                    if (!matricesByJoint.TryGetValue(jointIndex, out Matrix4x4 inverseBind))
                    {
                        Matrix4x4 world = ComputeWorldMatrix(nodes, parents, jointIndex);
                        Matrix4x4.Invert(world, out inverseBind);
                    }

                    finalMatrices.Add(inverseBind);
                }

                int newAccessorIndex = AppendMatrixAccessor(accessors, bufferViews, binData, finalMatrices);
                skinObj["inverseBindMatrices"] = newAccessorIndex;

                var finalJoints = new JArray(fullJointList);
                if (!JToken.DeepEquals(joints, finalJoints))
                {
                    skinObj["joints"] = finalJoints;
                    modified = true;
                }

                modified = true;
            }

            return modified;
        }

        private static Dictionary<int, Matrix4x4> ReadExistingInverseBindMatrices(JArray joints, int? accessorIndex,
            JArray accessors, JArray bufferViews, byte[] binBytes)
        {
            var result = new Dictionary<int, Matrix4x4>();
            if (!accessorIndex.HasValue || accessorIndex.Value < 0 || accessorIndex.Value >= accessors.Count)
                return result;

            var accessor = accessors[accessorIndex.Value] as JObject;
            if (accessor == null || (int?)accessor["componentType"] != 5126 || (string)accessor["type"] != "MAT4")
                return result;

            int bufferViewIndex = (int?)accessor["bufferView"] ?? -1;
            if (bufferViewIndex < 0 || bufferViewIndex >= bufferViews.Count)
                return result;

            var bufferView = bufferViews[bufferViewIndex] as JObject;
            if (bufferView == null)
                return result;

            int viewOffset = (int?)bufferView["byteOffset"] ?? 0;
            int accessorOffset = (int?)accessor["byteOffset"] ?? 0;
            int count = Math.Min((int?)accessor["count"] ?? 0, joints.Count);
            int baseOffset = viewOffset + accessorOffset;

            for (int i = 0; i < count; i++)
            {
                int matrixOffset = baseOffset + (i * 16 * sizeof(float));
                if (matrixOffset < 0 || matrixOffset + (16 * sizeof(float)) > binBytes.Length)
                    break;

                result[(int)joints[i]] = ReadMatrix(binBytes, matrixOffset);
            }

            return result;
        }

        private static int AppendMatrixAccessor(JArray accessors, JArray bufferViews, List<byte> binData,
            IReadOnlyList<Matrix4x4> matrices)
        {
            int dstOffset = AlignTo4(binData.Count);
            while (binData.Count < dstOffset)
                binData.Add(0);

            foreach (var matrix in matrices)
            {
                WriteFloat(binData, matrix.M11);
                WriteFloat(binData, matrix.M12);
                WriteFloat(binData, matrix.M13);
                WriteFloat(binData, matrix.M14);
                WriteFloat(binData, matrix.M21);
                WriteFloat(binData, matrix.M22);
                WriteFloat(binData, matrix.M23);
                WriteFloat(binData, matrix.M24);
                WriteFloat(binData, matrix.M31);
                WriteFloat(binData, matrix.M32);
                WriteFloat(binData, matrix.M33);
                WriteFloat(binData, matrix.M34);
                WriteFloat(binData, matrix.M41);
                WriteFloat(binData, matrix.M42);
                WriteFloat(binData, matrix.M43);
                WriteFloat(binData, matrix.M44);
            }

            var bufferView = new JObject
            {
                ["buffer"] = 0,
                ["byteOffset"] = dstOffset,
                ["byteLength"] = matrices.Count * 16 * sizeof(float)
            };
            bufferViews.Add(bufferView);

            var accessor = new JObject
            {
                ["bufferView"] = bufferViews.Count - 1,
                ["byteOffset"] = 0,
                ["componentType"] = 5126,
                ["count"] = matrices.Count,
                ["type"] = "MAT4"
            };
            accessors.Add(accessor);
            return accessors.Count - 1;
        }

        private static bool ConvertAnimatedNodesToTrs(JArray nodes, JArray animations)
        {
            bool modified = false;
            var animatedNodeIndices = new HashSet<int>();
            foreach (var animation in animations.OfType<JObject>())
            {
                var channels = animation["channels"] as JArray;
                if (channels == null)
                    continue;

                foreach (var channel in channels.OfType<JObject>())
                {
                    int? nodeIndex = (int?)channel["target"]?["node"];
                    if (nodeIndex.HasValue)
                        animatedNodeIndices.Add(nodeIndex.Value);
                }
            }

            foreach (int nodeIndex in animatedNodeIndices)
            {
                if (nodeIndex < 0 || nodeIndex >= nodes.Count)
                    continue;

                var nodeObj = nodes[nodeIndex] as JObject;
                var matrixArray = nodeObj?["matrix"] as JArray;
                if (nodeObj == null || matrixArray == null || matrixArray.Count != 16)
                    continue;

                Matrix4x4 matrix = ParseMatrix(matrixArray);
                if (!Matrix4x4.Decompose(matrix, out var scale, out var rotation, out var translation))
                    continue;

                rotation = Quaternion.Normalize(rotation);
                nodeObj.Remove("matrix");
                nodeObj["translation"] = new JArray(translation.X, translation.Y, translation.Z);
                nodeObj["rotation"] = new JArray(rotation.X, rotation.Y, rotation.Z, rotation.W);
                nodeObj["scale"] = new JArray(scale.X, scale.Y, scale.Z);
                modified = true;
            }

            return modified;
        }

        private static bool MergeAnimations(JObject root, string animationName)
        {
            var animations = root["animations"] as JArray;
            if (animations == null || animations.Count <= 1)
                return false;

            var mergedChannels = new JArray();
            var mergedSamplers = new JArray();
            int samplerOffset = 0;

            foreach (var animation in animations.OfType<JObject>())
            {
                var samplers = animation["samplers"] as JArray ?? new JArray();
                var channels = animation["channels"] as JArray ?? new JArray();

                foreach (var channel in channels.OfType<JObject>())
                {
                    var mergedChannel = (JObject)channel.DeepClone();
                    int samplerIndex = (int?)mergedChannel["sampler"] ?? 0;
                    mergedChannel["sampler"] = samplerIndex + samplerOffset;
                    mergedChannels.Add(mergedChannel);
                }

                foreach (var sampler in samplers)
                    mergedSamplers.Add(sampler.DeepClone());

                samplerOffset += samplers.Count;
            }

            var mergedAnimation = new JObject
            {
                ["name"] = !string.IsNullOrWhiteSpace(animationName) ? animationName : "Animation",
                ["channels"] = mergedChannels,
                ["samplers"] = mergedSamplers
            };

            root["animations"] = new JArray(mergedAnimation);
            return true;
        }

        private static bool NormalizeSceneMeshRoots(JArray nodes, JArray scenes)
        {
            var meshNodeIndices = CollectMeshNodeIndices(nodes);
            if (meshNodeIndices.Count == 0)
                return false;

            bool modified = false;
            foreach (var nodeObj in nodes.OfType<JObject>())
            {
                var children = nodeObj["children"] as JArray;
                if (children == null)
                    continue;

                var filtered = new JArray(children.Values<int>().Where(idx => !meshNodeIndices.Contains(idx)));
                if (!JToken.DeepEquals(children, filtered))
                {
                    nodeObj["children"] = filtered;
                    modified = true;
                }
            }

            var scene0 = scenes[0] as JObject;
            if (scene0 == null)
                return modified;

            var sceneRoots = scene0["nodes"] as JArray ?? new JArray();
            var mergedRoots = new JArray(sceneRoots.Values<int>().Union(meshNodeIndices.OrderBy(idx => idx)));
            if (!JToken.DeepEquals(sceneRoots, mergedRoots))
            {
                scene0["nodes"] = mergedRoots;
                modified = true;
            }

            return modified;
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
            Dictionary<int, int> depths)
        {
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

            int preferred = commonAncestors
                .OrderBy(idx => depths.TryGetValue(idx, out int depth) ? depth : int.MaxValue)
                .FirstOrDefault(idx => IsPreferredSkeletonRoot(nodes[idx] as JObject));
            if (preferred != 0 || IsPreferredSkeletonRoot(nodes[0] as JObject))
                return preferred;

            return commonAncestors
                .OrderBy(idx => depths.TryGetValue(idx, out int depth) ? depth : int.MaxValue)
                .First();
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

        private static List<int> CollectSkeletonJointNodes(JArray nodes, int skeletonRoot)
        {
            var result = new List<int>();

            void Traverse(int nodeIndex)
            {
                if (nodeIndex < 0 || nodeIndex >= nodes.Count)
                    return;

                var node = nodes[nodeIndex] as JObject;
                if (node == null)
                    return;

                if (node["mesh"] == null)
                    result.Add(nodeIndex);

                var children = node["children"] as JArray;
                if (children == null)
                    return;

                foreach (int child in children.Values<int>())
                    Traverse(child);
            }

            Traverse(skeletonRoot);
            return result;
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
                return ParseMatrix(matrixArray);

            Vector3 translation = ReadVector3(node["translation"] as JArray, Vector3.Zero);
            Quaternion rotation = ReadQuaternion(node["rotation"] as JArray, Quaternion.Identity);
            Vector3 scale = ReadVector3(node["scale"] as JArray, Vector3.One);
            return Matrix4x4.CreateScale(scale) * Matrix4x4.CreateFromQuaternion(rotation) * Matrix4x4.CreateTranslation(translation);
        }

        private static Matrix4x4 ParseMatrix(JArray matrixArray)
        {
            return new Matrix4x4(
                (float)matrixArray[0], (float)matrixArray[1], (float)matrixArray[2], (float)matrixArray[3],
                (float)matrixArray[4], (float)matrixArray[5], (float)matrixArray[6], (float)matrixArray[7],
                (float)matrixArray[8], (float)matrixArray[9], (float)matrixArray[10], (float)matrixArray[11],
                (float)matrixArray[12], (float)matrixArray[13], (float)matrixArray[14], (float)matrixArray[15]);
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

        private static Matrix4x4 ReadMatrix(byte[] bytes, int offset)
        {
            return new Matrix4x4(
                BitConverter.ToSingle(bytes, offset + 0),
                BitConverter.ToSingle(bytes, offset + 4),
                BitConverter.ToSingle(bytes, offset + 8),
                BitConverter.ToSingle(bytes, offset + 12),
                BitConverter.ToSingle(bytes, offset + 16),
                BitConverter.ToSingle(bytes, offset + 20),
                BitConverter.ToSingle(bytes, offset + 24),
                BitConverter.ToSingle(bytes, offset + 28),
                BitConverter.ToSingle(bytes, offset + 32),
                BitConverter.ToSingle(bytes, offset + 36),
                BitConverter.ToSingle(bytes, offset + 40),
                BitConverter.ToSingle(bytes, offset + 44),
                BitConverter.ToSingle(bytes, offset + 48),
                BitConverter.ToSingle(bytes, offset + 52),
                BitConverter.ToSingle(bytes, offset + 56),
                BitConverter.ToSingle(bytes, offset + 60));
        }

        private static void WriteFloat(List<byte> bytes, float value)
        {
            bytes.AddRange(BitConverter.GetBytes(value));
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
    }
}
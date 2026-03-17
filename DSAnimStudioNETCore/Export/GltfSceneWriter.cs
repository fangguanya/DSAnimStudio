using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using Assimp;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Quaternion = System.Numerics.Quaternion;
using Vector2 = System.Numerics.Vector2;
using Vector3 = System.Numerics.Vector3;
using Vector4 = System.Numerics.Vector4;

namespace DSAnimStudio.Export
{
    internal static class GltfSceneWriter
    {
        private const int ArrayBufferTarget = 34962;
        private const int ElementArrayBufferTarget = 34963;
        private const int ComponentTypeFloat = 5126;
        private const int ComponentTypeUnsignedShort = 5123;
        private const int ComponentTypeUnsignedInt = 5125;

        public static void Write(Scene scene, string outputPath, string expectedSkeletonRootName = null, string animationName = null)
        {
            if (scene == null)
                throw new ArgumentNullException(nameof(scene));

            string gltfPath = Path.ChangeExtension(outputPath, ".gltf");
            string outputDir = Path.GetDirectoryName(gltfPath) ?? string.Empty;
            if (!Directory.Exists(outputDir))
                Directory.CreateDirectory(outputDir);

            var graph = NormalizedNodeGraph.Create(scene);
            string skeletonRootName = !string.IsNullOrWhiteSpace(expectedSkeletonRootName)
                ? expectedSkeletonRootName
                : graph.ResolveDefaultSkeletonRootName();

            int? skeletonRootIndex = !string.IsNullOrWhiteSpace(skeletonRootName)
                ? graph.TryGetNodeIndexByName(skeletonRootName)
                : null;

            var root = new JObject
            {
                ["asset"] = new JObject
                {
                    ["version"] = "2.0",
                    ["generator"] = "DSAnimStudio glTF Writer"
                },
                ["scene"] = 0,
            };

            var bufferBuilder = new BufferBuilder();
            var materials = new JArray();
            var textures = new JArray();
            var images = new JArray();
            var samplers = new JArray();
            BuildMaterials(scene, materials, textures, images, samplers);

            SkinContext skinContext = skeletonRootIndex.HasValue
                ? BuildSkinContext(scene, graph, skeletonRootIndex.Value)
                : null;

            var meshes = new JArray();
            var meshMappings = BuildMeshes(scene, graph, skinContext, materials, bufferBuilder, meshes);

            var nodes = BuildNodes(graph, meshMappings, skinContext);
            var scenes = new JArray
            {
                new JObject
                {
                    ["nodes"] = new JArray(graph.SceneRootIndices)
                }
            };

            root["nodes"] = nodes;
            root["scenes"] = scenes;

            if (meshes.Count > 0)
                root["meshes"] = meshes;

            if (skinContext != null)
            {
                var skins = new JArray
                {
                    CreateSkinObject(skinContext, bufferBuilder)
                };
                root["skins"] = skins;
            }

            if (materials.Count > 0)
                root["materials"] = materials;
            if (textures.Count > 0)
                root["textures"] = textures;
            if (images.Count > 0)
                root["images"] = images;
            if (samplers.Count > 0)
                root["samplers"] = samplers;

            var animations = BuildAnimations(scene, graph, animationName, bufferBuilder);
            if (animations.Count > 0)
                root["animations"] = animations;

            root["bufferViews"] = bufferBuilder.BufferViews;
            root["accessors"] = bufferBuilder.Accessors;

            string binFileName = Path.GetFileNameWithoutExtension(gltfPath) + ".bin";
            root["buffers"] = new JArray
            {
                new JObject
                {
                    ["uri"] = binFileName,
                    ["byteLength"] = bufferBuilder.Length,
                }
            };

            ValidateGltf(root, gltfPath, skeletonRootName);

            File.WriteAllBytes(Path.Combine(outputDir, binFileName), bufferBuilder.ToArray());
            File.WriteAllText(gltfPath, root.ToString(Formatting.Indented));
        }

        private static void BuildMaterials(Scene scene, JArray materials, JArray textures, JArray images, JArray samplers)
        {
            var imageIndices = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            var textureIndices = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            int defaultSamplerIndex = -1;

            int GetTextureIndex(string path)
            {
                if (string.IsNullOrWhiteSpace(path))
                    return -1;

                string normalizedPath = path.Replace('\\', '/');
                if (textureIndices.TryGetValue(normalizedPath, out int existing))
                    return existing;

                if (defaultSamplerIndex < 0)
                {
                    samplers.Add(new JObject
                    {
                        ["magFilter"] = 9729,
                        ["minFilter"] = 9987,
                        ["wrapS"] = 10497,
                        ["wrapT"] = 10497,
                    });
                    defaultSamplerIndex = samplers.Count - 1;
                }

                if (!imageIndices.TryGetValue(normalizedPath, out int imageIndex))
                {
                    images.Add(new JObject { ["uri"] = Path.GetFileName(normalizedPath) });
                    imageIndex = images.Count - 1;
                    imageIndices.Add(normalizedPath, imageIndex);
                }

                textures.Add(new JObject
                {
                    ["sampler"] = defaultSamplerIndex,
                    ["source"] = imageIndex,
                });
                int textureIndex = textures.Count - 1;
                textureIndices.Add(normalizedPath, textureIndex);
                return textureIndex;
            }

            foreach (var material in scene.Materials)
            {
                var materialObj = new JObject
                {
                    ["name"] = string.IsNullOrWhiteSpace(material?.Name) ? "Material" : material.Name
                };

                var pbr = new JObject
                {
                    ["metallicFactor"] = 0.0f,
                    ["roughnessFactor"] = 1.0f,
                };

                if (material != null)
                {
                    if (material.HasTextureDiffuse)
                    {
                        int baseColorTexture = GetTextureIndex(material.TextureDiffuse.FilePath);
                        if (baseColorTexture >= 0)
                            pbr["baseColorTexture"] = new JObject { ["index"] = baseColorTexture };
                    }

                    if (material.HasTextureNormal)
                    {
                        int normalTexture = GetTextureIndex(material.TextureNormal.FilePath);
                        if (normalTexture >= 0)
                            materialObj["normalTexture"] = new JObject { ["index"] = normalTexture };
                    }

                    if (material.HasTextureEmissive)
                    {
                        int emissiveTexture = GetTextureIndex(material.TextureEmissive.FilePath);
                        if (emissiveTexture >= 0)
                        {
                            materialObj["emissiveTexture"] = new JObject { ["index"] = emissiveTexture };
                            materialObj["emissiveFactor"] = new JArray(1.0f, 1.0f, 1.0f);
                        }
                    }
                }

                materialObj["pbrMetallicRoughness"] = pbr;
                materials.Add(materialObj);
            }
        }

        private static Dictionary<int, int> BuildMeshes(Scene scene, NormalizedNodeGraph graph, SkinContext skinContext,
            JArray materials, BufferBuilder bufferBuilder, JArray meshes)
        {
            var meshMappings = new Dictionary<int, int>();

            for (int nodeIndex = 0; nodeIndex < graph.Nodes.Count; nodeIndex++)
            {
                var nodeInfo = graph.Nodes[nodeIndex];
                if (nodeInfo.MeshIndices.Count == 0)
                    continue;

                var primitives = new JArray();
                foreach (int sceneMeshIndex in nodeInfo.MeshIndices)
                {
                    if (sceneMeshIndex < 0 || sceneMeshIndex >= scene.Meshes.Count)
                        throw new InvalidOperationException($"Normalized scene node '{nodeInfo.Name}' referenced invalid mesh index {sceneMeshIndex}.");

                    primitives.Add(BuildPrimitive(scene.Meshes[sceneMeshIndex], skinContext, bufferBuilder, materials.Count));
                }

                meshes.Add(new JObject
                {
                    ["name"] = nodeInfo.Name,
                    ["primitives"] = primitives,
                });
                meshMappings.Add(nodeIndex, meshes.Count - 1);
            }

            return meshMappings;
        }

        private static JObject BuildPrimitive(Assimp.Mesh mesh, SkinContext skinContext, BufferBuilder bufferBuilder, int materialCount)
        {
            if (mesh == null)
                throw new ArgumentNullException(nameof(mesh));

            int vertexCount = mesh.VertexCount;
            if (vertexCount <= 0)
                throw new InvalidOperationException($"Mesh '{mesh.Name}' contains no vertices.");

            var positions = new List<Vector3>(vertexCount);
            for (int i = 0; i < vertexCount; i++)
                positions.Add(ToVector3(mesh.Vertices[i]));

            var attributes = new JObject
            {
                ["POSITION"] = bufferBuilder.AppendVec3Accessor(positions, ArrayBufferTarget, includeMinMax: true)
            };

            if (mesh.HasNormals && mesh.Normals.Count == vertexCount)
            {
                var normals = new List<Vector3>(vertexCount);
                for (int i = 0; i < vertexCount; i++)
                    normals.Add(ToVector3(mesh.Normals[i]));
                attributes["NORMAL"] = bufferBuilder.AppendVec3Accessor(normals, ArrayBufferTarget);

                if (mesh.HasTangentBasis && mesh.Tangents.Count == vertexCount && mesh.BiTangents.Count == vertexCount)
                {
                    var tangents = new List<Vector4>(vertexCount);
                    for (int i = 0; i < vertexCount; i++)
                    {
                        Vector3 normal = normals[i];
                        Vector3 tangent = ToVector3(mesh.Tangents[i]);
                        Vector3 bitangent = ToVector3(mesh.BiTangents[i]);
                        float handedness = Vector3.Dot(Vector3.Cross(normal, tangent), bitangent) >= 0 ? 1.0f : -1.0f;
                        tangents.Add(new Vector4(tangent, handedness));
                    }

                    attributes["TANGENT"] = bufferBuilder.AppendVec4Accessor(tangents, ArrayBufferTarget);
                }
            }

            if (mesh.VertexColorChannels[0] != null && mesh.VertexColorChannels[0].Count == vertexCount)
            {
                var colors = new List<Vector4>(vertexCount);
                for (int i = 0; i < vertexCount; i++)
                {
                    var color = mesh.VertexColorChannels[0][i];
                    colors.Add(new Vector4(color.R, color.G, color.B, color.A));
                }

                attributes["COLOR_0"] = bufferBuilder.AppendVec4Accessor(colors, ArrayBufferTarget);
            }

            for (int uvChannel = 0; uvChannel < mesh.TextureCoordinateChannels.Length; uvChannel++)
            {
                var channel = mesh.TextureCoordinateChannels[uvChannel];
                if (channel == null || channel.Count != vertexCount)
                    continue;

                var uvs = new List<Vector2>(vertexCount);
                for (int i = 0; i < vertexCount; i++)
                    uvs.Add(new Vector2(channel[i].X, channel[i].Y));

                attributes[$"TEXCOORD_{uvChannel}"] = bufferBuilder.AppendVec2Accessor(uvs, ArrayBufferTarget);
            }

            if (mesh.HasBones)
            {
                if (skinContext == null)
                    throw new InvalidOperationException($"Mesh '{mesh.Name}' contains bones but no skin context was created.");

                BuildJointAttributes(mesh, skinContext, vertexCount, out var joints, out var weights);
                attributes["JOINTS_0"] = bufferBuilder.AppendUShort4Accessor(joints, ArrayBufferTarget);
                attributes["WEIGHTS_0"] = bufferBuilder.AppendVec4Accessor(weights, ArrayBufferTarget);
            }

            var indices = new List<int>(mesh.FaceCount * 3);
            foreach (var face in mesh.Faces)
            {
                if (face.IndexCount != 3)
                    throw new InvalidOperationException($"Mesh '{mesh.Name}' contained a non-triangle face after FLVER triangulation.");

                indices.Add(face.Indices[0]);
                indices.Add(face.Indices[1]);
                indices.Add(face.Indices[2]);
            }

            int maxIndex = indices.Count > 0 ? indices.Max() : 0;
            bool useUnsignedInt = maxIndex > ushort.MaxValue;

            var primitive = new JObject
            {
                ["attributes"] = attributes,
                ["indices"] = bufferBuilder.AppendIndexAccessor(indices, useUnsignedInt),
            };

            if (mesh.MaterialIndex >= 0 && mesh.MaterialIndex < materialCount)
                primitive["material"] = mesh.MaterialIndex;

            return primitive;
        }

        private static void BuildJointAttributes(Assimp.Mesh mesh, SkinContext skinContext, int vertexCount,
            out List<UShort4> joints, out List<Vector4> weights)
        {
            var vertexInfluences = new List<(ushort Joint, float Weight)>[vertexCount];
            for (int i = 0; i < vertexCount; i++)
                vertexInfluences[i] = new List<(ushort Joint, float Weight)>(4);

            foreach (var bone in mesh.Bones)
            {
                if (bone == null)
                    continue;

                if (!skinContext.JointLookup.TryGetValue(bone.Name, out ushort jointIndex))
                    throw new InvalidOperationException($"Mesh '{mesh.Name}' referenced bone '{bone.Name}' that was not serialized into skin.joints.");

                foreach (var vertexWeight in bone.VertexWeights)
                {
                    if (vertexWeight.VertexID < 0 || vertexWeight.VertexID >= vertexCount || vertexWeight.Weight <= 0)
                        continue;

                    vertexInfluences[vertexWeight.VertexID].Add((jointIndex, vertexWeight.Weight));
                }
            }

            joints = new List<UShort4>(vertexCount);
            weights = new List<Vector4>(vertexCount);

            for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
            {
                var ordered = vertexInfluences[vertexIndex]
                    .OrderByDescending(influence => influence.Weight)
                    .Take(4)
                    .ToList();

                while (ordered.Count < 4)
                    ordered.Add((0, 0));

                float weightSum = ordered.Sum(entry => entry.Weight);
                if (weightSum > 0)
                {
                    for (int i = 0; i < ordered.Count; i++)
                        ordered[i] = (ordered[i].Joint, ordered[i].Weight / weightSum);
                }

                joints.Add(new UShort4(ordered[0].Joint, ordered[1].Joint, ordered[2].Joint, ordered[3].Joint));
                weights.Add(new Vector4(ordered[0].Weight, ordered[1].Weight, ordered[2].Weight, ordered[3].Weight));
            }
        }

        private static SkinContext BuildSkinContext(Scene scene, NormalizedNodeGraph graph, int skeletonRootIndex)
        {
            var weightedJointNames = new List<string>();
            var seenWeightedJoints = new HashSet<string>(StringComparer.Ordinal);
            var exactInverseBindMatrices = new Dictionary<string, Matrix4x4>(StringComparer.Ordinal);

            foreach (var mesh in scene.Meshes)
            {
                if (mesh == null || !mesh.HasBones)
                    continue;

                foreach (var bone in mesh.Bones)
                {
                    if (bone == null || string.IsNullOrWhiteSpace(bone.Name))
                        continue;

                    if (seenWeightedJoints.Add(bone.Name))
                        weightedJointNames.Add(bone.Name);

                    if (!exactInverseBindMatrices.ContainsKey(bone.Name))
                        exactInverseBindMatrices.Add(bone.Name, AssimpExportTransformUtils.ToNumericsMatrix(bone.OffsetMatrix));
                }
            }

            if (weightedJointNames.Count == 0)
                return null;

            var finalJointIndices = new List<int>();
            var finalJointNames = new List<string>();
            var finalJointSet = new HashSet<int>();

            void AddJoint(int index)
            {
                if (!finalJointSet.Add(index))
                    return;

                finalJointIndices.Add(index);
                finalJointNames.Add(graph.Nodes[index].Name);
            }

            foreach (string jointName in weightedJointNames)
            {
                int jointIndex = graph.RequireNodeIndexByName(jointName);
                AddJoint(jointIndex);

                int current = jointIndex;
                while (graph.ParentByIndex.TryGetValue(current, out int parentIndex))
                {
                    current = parentIndex;
                    AddJoint(current);
                    if (current == skeletonRootIndex)
                        break;
                }
            }

            AddJoint(skeletonRootIndex);

            var worldMatrices = graph.ComputeWorldMatrices();
            var inverseBindMatrices = new List<Matrix4x4>(finalJointIndices.Count);
            foreach (int jointIndex in finalJointIndices)
            {
                string jointName = graph.Nodes[jointIndex].Name;
                if (exactInverseBindMatrices.TryGetValue(jointName, out Matrix4x4 exactInverseBind))
                {
                    inverseBindMatrices.Add(exactInverseBind);
                    continue;
                }

                if (!Matrix4x4.Invert(worldMatrices[jointIndex], out Matrix4x4 computedInverseBind))
                    throw new InvalidOperationException($"Failed to compute inverse bind matrix for joint '{jointName}'.");

                inverseBindMatrices.Add(computedInverseBind);
            }

            var jointLookup = new Dictionary<string, ushort>(StringComparer.Ordinal);
            for (int i = 0; i < finalJointNames.Count; i++)
                jointLookup.Add(finalJointNames[i], checked((ushort)i));

            return new SkinContext(skeletonRootIndex, finalJointIndices, finalJointNames, inverseBindMatrices, jointLookup);
        }

        private static JObject CreateSkinObject(SkinContext skinContext, BufferBuilder bufferBuilder)
        {
            int inverseBindMatricesAccessor = bufferBuilder.AppendMat4Accessor(skinContext.InverseBindMatrices);

            return new JObject
            {
                ["skeleton"] = skinContext.SkeletonRootIndex,
                ["joints"] = new JArray(skinContext.JointIndices),
                ["inverseBindMatrices"] = inverseBindMatricesAccessor,
            };
        }

        private static JArray BuildNodes(NormalizedNodeGraph graph, IReadOnlyDictionary<int, int> meshMappings, SkinContext skinContext)
        {
            var nodes = new JArray();
            var meshNodeIndices = new HashSet<int>(meshMappings.Keys);

            for (int nodeIndex = 0; nodeIndex < graph.Nodes.Count; nodeIndex++)
            {
                var nodeInfo = graph.Nodes[nodeIndex];
                var nodeObj = new JObject
                {
                    ["name"] = nodeInfo.Name,
                };

                WriteTrs(nodeObj, nodeInfo.LocalTransform);

                if (nodeInfo.Children.Count > 0)
                    nodeObj["children"] = new JArray(nodeInfo.Children);

                if (meshMappings.TryGetValue(nodeIndex, out int meshIndex))
                {
                    nodeObj["mesh"] = meshIndex;
                    bool hasSkin = skinContext != null && nodeInfo.MeshIndices.Any(index => index >= 0 && index < graph.SceneMeshHasBones.Length && graph.SceneMeshHasBones[index]);
                    if (hasSkin)
                    {
                        nodeObj["skin"] = 0;
                        nodeObj["skeletons"] = new JArray(skinContext.SkeletonRootIndex);
                    }
                }

                nodes.Add(nodeObj);
            }

            return nodes;
        }

        private static JArray BuildAnimations(Scene scene, NormalizedNodeGraph graph, string animationName, BufferBuilder bufferBuilder)
        {
            var animations = new JArray();
            foreach (var sceneAnimation in scene.Animations)
            {
                if (sceneAnimation == null)
                    continue;

                var channels = new JArray();
                var samplers = new JArray();
                double ticksPerSecond = Math.Abs(sceneAnimation.TicksPerSecond) > double.Epsilon ? sceneAnimation.TicksPerSecond : 1.0;

                foreach (var nodeChannel in sceneAnimation.NodeAnimationChannels)
                {
                    if (nodeChannel == null)
                        continue;

                    int targetNodeIndex = graph.RequireNodeIndexByName(nodeChannel.NodeName);

                    AppendAnimationPath(nodeChannel.PositionKeys, targetNodeIndex, "translation", ticksPerSecond, channels, samplers, bufferBuilder,
                        key => new Vector3(key.Value.X, key.Value.Y, key.Value.Z));

                    AppendAnimationPath(nodeChannel.RotationKeys, targetNodeIndex, "rotation", ticksPerSecond, channels, samplers, bufferBuilder,
                        key =>
                        {
                            var rotation = new Quaternion(key.Value.X, key.Value.Y, key.Value.Z, key.Value.W);
                            rotation = Quaternion.Normalize(rotation);
                            return new Vector4(rotation.X, rotation.Y, rotation.Z, rotation.W);
                        });

                    AppendAnimationPath(nodeChannel.ScalingKeys, targetNodeIndex, "scale", ticksPerSecond, channels, samplers, bufferBuilder,
                        key => new Vector3(key.Value.X, key.Value.Y, key.Value.Z));
                }

                if (channels.Count == 0)
                    continue;

                animations.Add(new JObject
                {
                    ["name"] = !string.IsNullOrWhiteSpace(animationName) ? animationName : (!string.IsNullOrWhiteSpace(sceneAnimation.Name) ? sceneAnimation.Name : "Animation"),
                    ["channels"] = channels,
                    ["samplers"] = samplers,
                });
            }

            return animations;
        }

        private static void AppendAnimationPath<TAssimpKey, TValue>(IList<TAssimpKey> keys, int targetNodeIndex, string path,
            double ticksPerSecond, JArray channels, JArray samplers, BufferBuilder bufferBuilder, Func<TAssimpKey, TValue> getValue)
        {
            if (keys == null || keys.Count == 0)
                return;

            var times = new float[keys.Count];
            for (int i = 0; i < keys.Count; i++)
                times[i] = ConvertKeyTime(keys[i], ticksPerSecond);

            int inputAccessor = bufferBuilder.AppendScalarFloatAccessor(times, includeMinMax: true);
            int outputAccessor;

            if (typeof(TValue) == typeof(Vector3))
            {
                var values = keys.Select(key => (Vector3)(object)getValue(key)).ToList();
                outputAccessor = bufferBuilder.AppendVec3Accessor(values, null);
            }
            else if (typeof(TValue) == typeof(Vector4))
            {
                var values = keys.Select(key => (Vector4)(object)getValue(key)).ToList();
                outputAccessor = bufferBuilder.AppendVec4Accessor(values, null);
            }
            else
            {
                throw new NotSupportedException($"Unsupported animation output type '{typeof(TValue).Name}'.");
            }

            samplers.Add(new JObject
            {
                ["input"] = inputAccessor,
                ["output"] = outputAccessor,
                ["interpolation"] = "LINEAR",
            });

            channels.Add(new JObject
            {
                ["sampler"] = samplers.Count - 1,
                ["target"] = new JObject
                {
                    ["node"] = targetNodeIndex,
                    ["path"] = path,
                }
            });
        }

        private static float ConvertKeyTime<TAssimpKey>(TAssimpKey key, double ticksPerSecond)
        {
            double time;
            switch (key)
            {
                case VectorKey vectorKey:
                    time = vectorKey.Time;
                    break;
                case QuaternionKey quaternionKey:
                    time = quaternionKey.Time;
                    break;
                default:
                    throw new NotSupportedException($"Unsupported animation key type '{typeof(TAssimpKey).Name}'.");
            }

            return (float)(time / ticksPerSecond);
        }

        private static void WriteTrs(JObject nodeObj, Matrix4x4 matrix)
        {
            if (!Matrix4x4.Decompose(matrix, out Vector3 scale, out Quaternion rotation, out Vector3 translation))
                throw new InvalidOperationException($"Failed to decompose node transform matrix for glTF TRS output: {FormatMatrix(matrix)}");

            rotation = Quaternion.Normalize(rotation);

            if (!NearlyEquals(translation, Vector3.Zero))
                nodeObj["translation"] = new JArray(translation.X, translation.Y, translation.Z);

            if (!NearlyEquals(rotation, Quaternion.Identity))
                nodeObj["rotation"] = new JArray(rotation.X, rotation.Y, rotation.Z, rotation.W);

            if (!NearlyEquals(scale, Vector3.One))
                nodeObj["scale"] = new JArray(scale.X, scale.Y, scale.Z);
        }

        private static void ValidateGltf(JObject root, string gltfPath, string expectedSkeletonRootName)
        {
            var nodes = root["nodes"] as JArray;
            var skins = root["skins"] as JArray;
            var scenes = root["scenes"] as JArray;
            var accessors = root["accessors"] as JArray;
            var animations = root["animations"] as JArray;

            if (nodes == null || scenes == null || scenes.Count == 0)
                throw new InvalidOperationException($"glTF writer produced no nodes/scenes for '{gltfPath}'.");

            var parents = BuildParentMap(nodes);
            var sceneRoots = scenes[0]?["nodes"] as JArray ?? new JArray();

            int? uniqueSkeletonRoot = null;
            var formalJointSet = new HashSet<int>();
            if (skins != null)
            {
                foreach (var skinObj in skins.OfType<JObject>())
                {
                    var joints = skinObj["joints"] as JArray;
                    int skeletonIndex = (int?)skinObj["skeleton"] ?? -1;
                    if (skeletonIndex < 0)
                        throw new InvalidOperationException("glTF skin is missing a formal skeleton root.");

                    if (!string.IsNullOrWhiteSpace(expectedSkeletonRootName))
                    {
                        int declaredSkeletonIndex = ResolveDeclaredSkeletonRootIndex(nodes, expectedSkeletonRootName);
                        if (declaredSkeletonIndex != skeletonIndex)
                            throw new InvalidOperationException($"glTF skin skeleton root '{GetNodeName(nodes, skeletonIndex)}' did not match declared formal skeleton root '{expectedSkeletonRootName}'.");
                    }

                    if (joints == null || joints.Count == 0)
                        throw new InvalidOperationException("glTF skin has no joints.");

                    foreach (int jointIndex in joints.Values<int>())
                        formalJointSet.Add(jointIndex);

                    formalJointSet.Add(skeletonIndex);
                    uniqueSkeletonRoot ??= skeletonIndex;
                    if (uniqueSkeletonRoot.Value != skeletonIndex)
                        throw new InvalidOperationException("Formal glTF contains multiple distinct skeleton roots.");

                    int accessorIndex = (int?)skinObj["inverseBindMatrices"] ?? -1;
                    if (accessors == null || accessorIndex < 0 || accessorIndex >= accessors.Count)
                        throw new InvalidOperationException("glTF skin is missing inverse bind matrices.");

                    int accessorCount = (int?)accessors[accessorIndex]?["count"] ?? 0;
                    if (accessorCount != joints.Count)
                        throw new InvalidOperationException($"glTF inverse bind matrix count {accessorCount} did not match joint count {joints.Count}.");
                }
            }

            if (uniqueSkeletonRoot.HasValue)
            {
                if (parents.ContainsKey(uniqueSkeletonRoot.Value))
                    throw new InvalidOperationException($"Formal skeleton root '{GetNodeName(nodes, uniqueSkeletonRoot.Value)}' must be a direct scene root.");

                if (!sceneRoots.Values<int>().Contains(uniqueSkeletonRoot.Value))
                    throw new InvalidOperationException($"Formal skeleton root '{GetNodeName(nodes, uniqueSkeletonRoot.Value)}' was not exposed as a scene root.");

                foreach (int sceneRootIndex in sceneRoots.Values<int>())
                {
                    if (IsWrapperRootNode(nodes[sceneRootIndex] as JObject))
                        throw new InvalidOperationException($"Scene root '{GetNodeName(nodes, sceneRootIndex)}' remained as a wrapper node.");
                }

                foreach (var nodeObj in nodes.OfType<JObject>())
                {
                    if (nodeObj["mesh"] == null || nodeObj["skin"] == null)
                        continue;

                    var meshSkeletons = nodeObj["skeletons"] as JArray;
                    if (meshSkeletons == null || meshSkeletons.Count == 0)
                        throw new InvalidOperationException($"Skinned mesh node '{(string)nodeObj["name"] ?? string.Empty}' is missing skeletons metadata.");

                    var distinctSkeletonRefs = meshSkeletons.Values<int>().Distinct().ToArray();
                    if (distinctSkeletonRefs.Length != 1 || distinctSkeletonRefs[0] != uniqueSkeletonRoot.Value)
                    {
                        throw new InvalidOperationException(
                            $"Skinned mesh node '{(string)nodeObj["name"] ?? string.Empty}' referenced skeleton roots [{string.Join(",", distinctSkeletonRefs)}] instead of the formal skeleton root '{GetNodeName(nodes, uniqueSkeletonRoot.Value)}'.");
                    }
                }

                foreach (int jointIndex in formalJointSet.ToArray())
                {
                    int current = jointIndex;
                    while (parents.TryGetValue(current, out int parentIndex))
                    {
                        current = parentIndex;
                        if (current == uniqueSkeletonRoot.Value)
                        {
                            formalJointSet.Add(current);
                            break;
                        }

                        if (!formalJointSet.Contains(current))
                        {
                            throw new InvalidOperationException(
                                $"Formal skin hierarchy is disconnected because ancestor node '{GetNodeName(nodes, current)}' is missing from skin.joints.");
                        }
                    }
                }
            }

            if (animations != null && animations.Count > 0)
            {
                if (animations.Count != 1)
                    throw new InvalidOperationException($"Formal animation glTF must contain exactly one animation entry, found {animations.Count}.");

                var channels = animations[0]?["channels"] as JArray;
                if (channels == null || channels.Count == 0)
                    throw new InvalidOperationException("Formal animation glTF contains no channels.");

                foreach (var channelObj in channels.OfType<JObject>())
                {
                    int nodeIndex = (int?)channelObj["target"]?["node"] ?? -1;
                    if (nodeIndex < 0 || nodeIndex >= nodes.Count)
                        throw new InvalidOperationException("Formal animation glTF references an invalid node.");
                    if (nodes[nodeIndex]?["matrix"] != null)
                        throw new InvalidOperationException($"Animated node {nodeIndex} still contains a matrix transform.");
                    if (formalJointSet.Count > 0 && !formalJointSet.Contains(nodeIndex) && (!uniqueSkeletonRoot.HasValue || !IsNodeDescendantOf(nodeIndex, uniqueSkeletonRoot.Value, parents)))
                        throw new InvalidOperationException($"Formal animation channel targets node '{GetNodeName(nodes, nodeIndex)}' ({nodeIndex}) outside the declared formal skeleton hierarchy.");
                }
            }

            if (GltfFormalHumanoidSkeletonValidator.ShouldValidate(root, expectedSkeletonRootName))
                GltfFormalHumanoidSkeletonValidator.Validate(root, gltfPath, expectedSkeletonRootName);
        }

        private static Dictionary<int, int> BuildParentMap(JArray nodes)
        {
            var parents = new Dictionary<int, int>();
            for (int nodeIndex = 0; nodeIndex < nodes.Count; nodeIndex++)
            {
                var children = nodes[nodeIndex]?["children"] as JArray;
                if (children == null)
                    continue;

                foreach (int childIndex in children.Values<int>())
                {
                    if (!parents.ContainsKey(childIndex))
                        parents.Add(childIndex, nodeIndex);
                }
            }

            return parents;
        }

        private static int ResolveDeclaredSkeletonRootIndex(JArray nodes, string expectedSkeletonRootName)
        {
            var matches = new List<int>();
            for (int i = 0; i < nodes.Count; i++)
            {
                if (string.Equals((string)nodes[i]?["name"], expectedSkeletonRootName, StringComparison.Ordinal))
                    matches.Add(i);
            }

            if (matches.Count == 1)
                return matches[0];
            if (matches.Count == 0)
                throw new InvalidOperationException($"Declared formal skeleton root '{expectedSkeletonRootName}' was not found in glTF nodes.");
            throw new InvalidOperationException($"Declared formal skeleton root '{expectedSkeletonRootName}' matched multiple glTF nodes.");
        }

        private static bool IsNodeDescendantOf(int nodeIndex, int rootIndex, IReadOnlyDictionary<int, int> parents)
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

        private static bool IsWrapperRootNode(JObject nodeObj)
        {
            string name = (string)nodeObj?["name"] ?? string.Empty;
            return string.IsNullOrWhiteSpace(name)
                || string.Equals(name, "Armature", StringComparison.OrdinalIgnoreCase)
                || string.Equals(name, "RootNode", StringComparison.OrdinalIgnoreCase);
        }

        private static string GetNodeName(JArray nodes, int index)
        {
            if (index < 0 || index >= nodes.Count)
                return string.Empty;
            return (string)nodes[index]?["name"] ?? string.Empty;
        }

        private static Vector3 ToVector3(Vector3D value) => new Vector3(value.X, value.Y, value.Z);

        private static bool NearlyEquals(Vector3 a, Vector3 b)
        {
            return MathF.Abs(a.X - b.X) <= 1e-5f
                && MathF.Abs(a.Y - b.Y) <= 1e-5f
                && MathF.Abs(a.Z - b.Z) <= 1e-5f;
        }

        private static bool NearlyEquals(Quaternion a, Quaternion b)
        {
            return MathF.Abs(a.X - b.X) <= 1e-5f
                && MathF.Abs(a.Y - b.Y) <= 1e-5f
                && MathF.Abs(a.Z - b.Z) <= 1e-5f
                && MathF.Abs(a.W - b.W) <= 1e-5f;
        }

        private static string FormatMatrix(Matrix4x4 value)
        {
            return string.Create(CultureInfo.InvariantCulture, $"[{value.M11:F3}, {value.M12:F3}, {value.M13:F3}, {value.M14:F3}; {value.M21:F3}, {value.M22:F3}, {value.M23:F3}, {value.M24:F3}; {value.M31:F3}, {value.M32:F3}, {value.M33:F3}, {value.M34:F3}; {value.M41:F3}, {value.M42:F3}, {value.M43:F3}, {value.M44:F3}]");
        }

        private sealed class SkinContext
        {
            public SkinContext(int skeletonRootIndex, List<int> jointIndices, List<string> jointNames,
                List<Matrix4x4> inverseBindMatrices, Dictionary<string, ushort> jointLookup)
            {
                SkeletonRootIndex = skeletonRootIndex;
                JointIndices = jointIndices;
                JointNames = jointNames;
                InverseBindMatrices = inverseBindMatrices;
                JointLookup = jointLookup;
            }

            public int SkeletonRootIndex { get; }
            public List<int> JointIndices { get; }
            public List<string> JointNames { get; }
            public List<Matrix4x4> InverseBindMatrices { get; }
            public Dictionary<string, ushort> JointLookup { get; }
        }

        private sealed class NormalizedNodeGraph
        {
            private readonly Dictionary<string, int> _nodeIndicesByName;

            private NormalizedNodeGraph(List<NodeInfo> nodes, List<int> sceneRootIndices, Dictionary<int, int> parentByIndex,
                bool[] sceneMeshHasBones, Dictionary<string, int> nodeIndicesByName)
            {
                Nodes = nodes;
                SceneRootIndices = sceneRootIndices;
                ParentByIndex = parentByIndex;
                SceneMeshHasBones = sceneMeshHasBones;
                _nodeIndicesByName = nodeIndicesByName;
            }

            public List<NodeInfo> Nodes { get; }
            public List<int> SceneRootIndices { get; }
            public Dictionary<int, int> ParentByIndex { get; }
            public bool[] SceneMeshHasBones { get; }

            public static NormalizedNodeGraph Create(Scene scene)
            {
                var nodes = new List<NodeInfo>();
                var sceneRoots = new List<int>();
                var parents = new Dictionary<int, int>();

                void AddNodeRecursive(Node sourceNode, Matrix4x4 inheritedWrapperTransform, int? parentIndex)
                {
                    if (sourceNode == null)
                        return;

                    Matrix4x4 sourceLocal = AssimpExportTransformUtils.ToNumericsMatrix(sourceNode.Transform);
                    Matrix4x4 effectiveLocal = inheritedWrapperTransform * sourceLocal;
                    bool isWrapper = sourceNode.MeshIndices.Count == 0
                        && (string.IsNullOrWhiteSpace(sourceNode.Name)
                            || string.Equals(sourceNode.Name, "Armature", StringComparison.OrdinalIgnoreCase)
                            || string.Equals(sourceNode.Name, "RootNode", StringComparison.OrdinalIgnoreCase));

                    if (isWrapper)
                    {
                        foreach (var child in sourceNode.Children)
                            AddNodeRecursive(child, effectiveLocal, parentIndex);
                        return;
                    }

                    string nodeName = string.IsNullOrWhiteSpace(sourceNode.Name) ? $"Node_{nodes.Count}" : sourceNode.Name;
                    var nodeInfo = new NodeInfo(nodeName, effectiveLocal, sourceNode.MeshIndices.ToList());
                    int nodeIndex = nodes.Count;
                    nodes.Add(nodeInfo);

                    if (parentIndex.HasValue)
                    {
                        nodes[parentIndex.Value].Children.Add(nodeIndex);
                        parents.Add(nodeIndex, parentIndex.Value);
                    }
                    else
                    {
                        sceneRoots.Add(nodeIndex);
                    }

                    foreach (var child in sourceNode.Children)
                        AddNodeRecursive(child, Matrix4x4.Identity, nodeIndex);
                }

                if (scene.RootNode == null)
                    throw new InvalidOperationException("glTF writer requires a scene root node.");

                AddNodeRecursive(scene.RootNode, Matrix4x4.Identity, null);

                var nodeIndicesByName = new Dictionary<string, int>(StringComparer.Ordinal);
                for (int i = 0; i < nodes.Count; i++)
                {
                    if (nodeIndicesByName.ContainsKey(nodes[i].Name))
                        throw new InvalidOperationException($"glTF writer encountered duplicate node name '{nodes[i].Name}'. Unique node names are required.");

                    nodeIndicesByName.Add(nodes[i].Name, i);
                }

                var sceneMeshHasBones = new bool[scene.Meshes.Count];
                for (int i = 0; i < scene.Meshes.Count; i++)
                    sceneMeshHasBones[i] = scene.Meshes[i] != null && scene.Meshes[i].HasBones;

                return new NormalizedNodeGraph(nodes, sceneRoots, parents, sceneMeshHasBones, nodeIndicesByName);
            }

            public int? TryGetNodeIndexByName(string nodeName)
            {
                if (string.IsNullOrWhiteSpace(nodeName))
                    return null;

                return _nodeIndicesByName.TryGetValue(nodeName, out int index) ? index : (int?)null;
            }

            public int RequireNodeIndexByName(string nodeName)
            {
                if (!_nodeIndicesByName.TryGetValue(nodeName, out int index))
                    throw new InvalidOperationException($"glTF writer could not resolve node '{nodeName}' in the normalized scene graph.");
                return index;
            }

            public string ResolveDefaultSkeletonRootName()
            {
                foreach (int sceneRootIndex in SceneRootIndices)
                {
                    if (!Nodes[sceneRootIndex].Name.StartsWith("Mesh_", StringComparison.Ordinal))
                        return Nodes[sceneRootIndex].Name;
                }

                return SceneRootIndices.Count > 0 ? Nodes[SceneRootIndices[0]].Name : null;
            }

            public Matrix4x4[] ComputeWorldMatrices()
            {
                var worldMatrices = new Matrix4x4[Nodes.Count];
                for (int i = 0; i < SceneRootIndices.Count; i++)
                {
                    int rootIndex = SceneRootIndices[i];
                    ComputeWorldRecursive(rootIndex, Matrix4x4.Identity, worldMatrices);
                }

                return worldMatrices;
            }

            private void ComputeWorldRecursive(int nodeIndex, Matrix4x4 parentWorld, Matrix4x4[] worldMatrices)
            {
                Matrix4x4 world = parentWorld * Nodes[nodeIndex].LocalTransform;
                worldMatrices[nodeIndex] = world;
                foreach (int childIndex in Nodes[nodeIndex].Children)
                    ComputeWorldRecursive(childIndex, world, worldMatrices);
            }
        }

        private sealed class NodeInfo
        {
            public NodeInfo(string name, Matrix4x4 localTransform, List<int> meshIndices)
            {
                Name = name;
                LocalTransform = localTransform;
                MeshIndices = meshIndices;
            }

            public string Name { get; }
            public Matrix4x4 LocalTransform { get; }
            public List<int> MeshIndices { get; }
            public List<int> Children { get; } = new List<int>();
        }

        private readonly struct UShort4
        {
            public UShort4(ushort x, ushort y, ushort z, ushort w)
            {
                X = x;
                Y = y;
                Z = z;
                W = w;
            }

            public ushort X { get; }
            public ushort Y { get; }
            public ushort Z { get; }
            public ushort W { get; }
        }

        private sealed class BufferBuilder
        {
            private readonly List<byte> _bytes = new List<byte>();

            public JArray BufferViews { get; } = new JArray();
            public JArray Accessors { get; } = new JArray();
            public int Length => _bytes.Count;

            public byte[] ToArray() => _bytes.ToArray();

            public int AppendScalarFloatAccessor(IReadOnlyList<float> values, bool includeMinMax = false)
            {
                return AppendFloatAccessor(values.Count, 1, null, includeMinMax ? new JArray(values.Min()) : null, includeMinMax ? new JArray(values.Max()) : null, writer =>
                {
                    foreach (float value in values)
                        WriteFloat(value);
                }, "SCALAR");
            }

            public int AppendVec2Accessor(IReadOnlyList<Vector2> values, int? target)
            {
                return AppendFloatAccessor(values.Count, 2, target, null, null, writer =>
                {
                    foreach (Vector2 value in values)
                    {
                        WriteFloat(value.X);
                        WriteFloat(value.Y);
                    }
                }, "VEC2");
            }

            public int AppendVec3Accessor(IReadOnlyList<Vector3> values, int? target, bool includeMinMax = false)
            {
                JArray minArray = null;
                JArray maxArray = null;
                if (includeMinMax && values.Count > 0)
                {
                    Vector3 min = values.Aggregate(new Vector3(float.MaxValue), (current, value) => Vector3.Min(current, value));
                    Vector3 max = values.Aggregate(new Vector3(float.MinValue), (current, value) => Vector3.Max(current, value));
                    minArray = new JArray(min.X, min.Y, min.Z);
                    maxArray = new JArray(max.X, max.Y, max.Z);
                }

                return AppendFloatAccessor(values.Count, 3, target, minArray, maxArray, writer =>
                {
                    foreach (Vector3 value in values)
                    {
                        WriteFloat(value.X);
                        WriteFloat(value.Y);
                        WriteFloat(value.Z);
                    }
                }, "VEC3");
            }

            public int AppendVec4Accessor(IReadOnlyList<Vector4> values, int? target)
            {
                return AppendFloatAccessor(values.Count, 4, target, null, null, writer =>
                {
                    foreach (Vector4 value in values)
                    {
                        WriteFloat(value.X);
                        WriteFloat(value.Y);
                        WriteFloat(value.Z);
                        WriteFloat(value.W);
                    }
                }, "VEC4");
            }

            public int AppendUShort4Accessor(IReadOnlyList<UShort4> values, int? target)
            {
                return AppendAccessor(values.Count, 8, target, "VEC4", ComponentTypeUnsignedShort, null, null, writer =>
                {
                    foreach (UShort4 value in values)
                    {
                        WriteUShort(value.X);
                        WriteUShort(value.Y);
                        WriteUShort(value.Z);
                        WriteUShort(value.W);
                    }
                });
            }

            public int AppendIndexAccessor(IReadOnlyList<int> indices, bool useUnsignedInt)
            {
                int componentType = useUnsignedInt ? ComponentTypeUnsignedInt : ComponentTypeUnsignedShort;
                int componentSize = useUnsignedInt ? 4 : 2;
                JArray minArray = indices.Count > 0 ? new JArray(indices.Min()) : null;
                JArray maxArray = indices.Count > 0 ? new JArray(indices.Max()) : null;
                return AppendAccessor(indices.Count, componentSize, ElementArrayBufferTarget, "SCALAR", componentType, minArray, maxArray, writer =>
                {
                    foreach (int index in indices)
                    {
                        if (useUnsignedInt)
                            WriteUInt((uint)index);
                        else
                            WriteUShort((ushort)index);
                    }
                });
            }

            public int AppendMat4Accessor(IReadOnlyList<Matrix4x4> matrices)
            {
                return AppendAccessor(matrices.Count, 64, null, "MAT4", ComponentTypeFloat, null, null, writer =>
                {
                    foreach (Matrix4x4 matrix in matrices)
                        GltfMatrixSerialization.WriteMatrix(_bytes, matrix);
                });
            }

            private int AppendFloatAccessor(int count, int components, int? target, JArray minArray, JArray maxArray, Action<BufferBuilder> writeAction, string accessorType)
            {
                return AppendAccessor(count, components * sizeof(float), target, accessorType, ComponentTypeFloat, minArray, maxArray, writeAction);
            }

            private int AppendAccessor(int count, int byteStride, int? target, string type, int componentType, JArray minArray, JArray maxArray, Action<BufferBuilder> writeAction)
            {
                AlignTo4();
                int offset = _bytes.Count;
                writeAction(this);
                int byteLength = _bytes.Count - offset;

                var bufferView = new JObject
                {
                    ["buffer"] = 0,
                    ["byteOffset"] = offset,
                    ["byteLength"] = byteLength,
                };

                if (target.HasValue)
                    bufferView["target"] = target.Value;
                if (byteStride > 0 && target == ArrayBufferTarget && type != "MAT4")
                    bufferView["byteStride"] = byteStride;

                BufferViews.Add(bufferView);

                var accessor = new JObject
                {
                    ["bufferView"] = BufferViews.Count - 1,
                    ["componentType"] = componentType,
                    ["count"] = count,
                    ["type"] = type,
                };

                if (minArray != null)
                    accessor["min"] = minArray;
                if (maxArray != null)
                    accessor["max"] = maxArray;

                Accessors.Add(accessor);
                return Accessors.Count - 1;
            }

            private void AlignTo4()
            {
                while ((_bytes.Count & 3) != 0)
                    _bytes.Add(0);
            }

            private void WriteFloat(float value)
            {
                _bytes.AddRange(BitConverter.GetBytes(value));
            }

            private void WriteUShort(ushort value)
            {
                _bytes.AddRange(BitConverter.GetBytes(value));
            }

            private void WriteUInt(uint value)
            {
                _bytes.AddRange(BitConverter.GetBytes(value));
            }
        }
    }
}
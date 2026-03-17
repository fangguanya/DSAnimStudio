using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using DSAnimStudio.Export;
using Newtonsoft.Json.Linq;
using NUnit.Framework;

namespace DSAnimStudioNETCore.Tests.Export
{
    [TestFixture]
    public class ExportedFormalSampleTests
    {
        private static readonly string[] RequiredUpperBodyBones =
        {
            "Pelvis",
            "Spine",
            "Spine1",
            "Spine2",
            "Neck",
            "Head",
            "L_Shoulder",
            "R_Shoulder",
            "L_Hand",
            "R_Hand",
        };

        private static string RepoRoot => FindRepoRoot();
        private static string SampleRoot => Path.Combine(RepoRoot, "_ExportCheck", "UserVerifyRun", "FreshUnifiedSingleSkill_A010_SceneRootFix2", "c0000");
        private static string ReportPath => Path.Combine(SampleRoot, "export_report.json");
        private static string ModelPath => Path.Combine(SampleRoot, "Model", "c0000.gltf");
        private static string AnimationPath => Path.Combine(SampleRoot, "Animations", "a010_000100.gltf");

        [Test]
        public void ExportReport_DeclaresSuccessfulFormalSample()
        {
            JObject report = ReadJson(ReportPath);

            Assert.That((string)report["characterId"], Is.EqualTo("c0000"));
            Assert.That((string)report["model"]?["formalSkeletonRoot"], Is.EqualTo("Master"));
            Assert.That((bool?)report["model"]?["success"], Is.True);
            Assert.That((bool?)report["animations"]?["success"], Is.True);
            Assert.That((int?)report["animations"]?["count"], Is.EqualTo(1));
            Assert.That((bool?)report["textures"]?["success"], Is.True);
            Assert.That((int?)report["textures"]?["count"], Is.EqualTo(62));
            Assert.That((bool?)report["skills"]?["success"], Is.True);
            Assert.That((bool?)report["formalSuccess"], Is.True);
        }

        [Test]
        public void ModelExport_ExposesMasterSceneRootAndContainsUpperBodySkinJoints()
        {
            JObject model = ReadJson(ModelPath);
            var nodes = (JArray)model["nodes"]!;
            var sceneRoots = ((JArray)model["scenes"]![0]!["nodes"]!).Values<int>().ToArray();
            var skin = (JObject)model["skins"]![0]!;
            int skeletonRootIndex = (int)skin["skeleton"]!;
            var jointSet = skin["joints"]!.Values<int>().ToHashSet();

            Assert.That((string)nodes[skeletonRootIndex]?["name"], Is.EqualTo("Master"));
            Assert.That(sceneRoots, Does.Contain(skeletonRootIndex));
            Assert.That(BuildParentMap(nodes).ContainsKey(skeletonRootIndex), Is.False);

            foreach (int sceneRootIndex in sceneRoots)
            {
                string sceneRootName = (string)nodes[sceneRootIndex]?["name"] ?? string.Empty;
                Assert.That(string.IsNullOrWhiteSpace(sceneRootName), Is.False, $"Scene root {sceneRootIndex} remained unnamed.");
                Assert.That(sceneRootName, Is.Not.EqualTo("Armature"));
                Assert.That(sceneRootName, Is.Not.EqualTo("RootNode"));
            }

            foreach (string boneName in RequiredUpperBodyBones)
            {
                int boneIndex = FindNodeIndexByName(nodes, boneName);
                Assert.That(boneIndex, Is.GreaterThanOrEqualTo(0), $"Missing node '{boneName}'.");
                Assert.That(jointSet.Contains(boneIndex), Is.True, $"Bone '{boneName}' was not present in skin.joints.");
            }
        }

        [Test]
        public void ModelExport_SkinnedMeshNodesReferenceFormalSkeletonRoot()
        {
            JObject model = ReadJson(ModelPath);
            var nodes = (JArray)model["nodes"]!;
            var skin = (JObject)model["skins"]![0]!;
            int skeletonRootIndex = (int)skin["skeleton"]!;

            foreach (var nodeObj in nodes.OfType<JObject>())
            {
                if (nodeObj["mesh"] == null || nodeObj["skin"] == null)
                    continue;

                var meshSkeletons = nodeObj["skeletons"] as JArray;
                if (meshSkeletons == null)
                    continue;

                Assert.That(meshSkeletons.Values<int>().Distinct().ToArray(), Is.EqualTo(new[] { skeletonRootIndex }),
                    $"Skinned mesh node '{(string)nodeObj["name"] ?? string.Empty}' did not reference the formal skeleton root.");
            }
        }

        [Test]
        public void ModelExport_IncludesConnectorAncestorsForSpineBranchInSkinJoints()
        {
            JObject model = ReadJson(ModelPath);
            var nodes = (JArray)model["nodes"]!;
            var skin = (JObject)model["skins"]![0]!;
            var jointSet = skin["joints"]!.Values<int>().ToHashSet();

            foreach (string nodeName in new[] { "Master", "RootPos", "RootRotY", "RootRotXZ", "Pelvis", "Spine" })
            {
                int nodeIndex = FindNodeIndexByName(nodes, nodeName);
                Assert.That(nodeIndex, Is.GreaterThanOrEqualTo(0), $"Missing node '{nodeName}'.");
                Assert.That(jointSet.Contains(nodeIndex), Is.True, $"Connector node '{nodeName}' was missing from skin.joints.");
            }
        }

        [Test]
        public void ModelExport_PassesHumanoidOrientationValidation()
        {
            JObject model = ReadJson(ModelPath);

            Assert.DoesNotThrow(() => GltfFormalHumanoidSkeletonValidator.Validate(model, ModelPath, "Master"));
        }

        [Test]
        public void AnimationExport_HasSingleClipTargetsSkeletonHierarchyAndPassesValidation()
        {
            JObject animation = ReadJson(AnimationPath);
            var nodes = (JArray)animation["nodes"]!;
            var skin = (JObject)animation["skins"]![0]!;
            int skeletonRootIndex = (int)skin["skeleton"]!;
            var jointSet = skin["joints"]!.Values<int>().ToHashSet();
            var parents = BuildParentMap(nodes);

            Assert.That((int?)animation["animations"]?.Count(), Is.EqualTo(1));
            Assert.That((string)animation["animations"]?[0]?["name"], Is.EqualTo("a010_000100"));

            foreach (var channel in animation["animations"]![0]!["channels"]!.OfType<JObject>())
            {
                int targetNodeIndex = (int)channel["target"]!["node"]!;
                Assert.That(nodes[targetNodeIndex]?["matrix"], Is.Null, $"Animated node {targetNodeIndex} still has a matrix transform.");
                Assert.That(
                    jointSet.Contains(targetNodeIndex) || IsDescendantOf(targetNodeIndex, skeletonRootIndex, parents),
                    Is.True,
                    $"Animated node {targetNodeIndex} was outside the formal skeleton hierarchy.");
            }

            Assert.DoesNotThrow(() => GltfFormalHumanoidSkeletonValidator.Validate(animation, AnimationPath, "Master"));
        }

        private static JObject ReadJson(string path)
        {
            Assert.That(File.Exists(path), Is.True, $"Expected exported sample file was missing: {path}");
            return JObject.Parse(File.ReadAllText(path));
        }

        private static int FindNodeIndexByName(JArray nodes, string nodeName)
        {
            for (int i = 0; i < nodes.Count; i++)
            {
                if (string.Equals((string)nodes[i]?["name"], nodeName, StringComparison.Ordinal))
                    return i;
            }

            return -1;
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

        private static bool IsDescendantOf(int nodeIndex, int rootIndex, IReadOnlyDictionary<int, int> parents)
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

        private static string FindRepoRoot()
        {
            string current = TestContext.CurrentContext.TestDirectory;
            while (!string.IsNullOrWhiteSpace(current))
            {
                if (File.Exists(Path.Combine(current, "DSAnimStudioNETCore.sln")))
                    return current;

                current = Directory.GetParent(current)?.FullName;
            }

            throw new InvalidOperationException("Could not locate repository root from the test output directory.");
        }
    }
}
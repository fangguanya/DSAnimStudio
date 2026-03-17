using System;
using System.Collections.Generic;
using DSAnimStudio.Export;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudioNETCore.Tests.Export
{
    [TestFixture]
    public class GltfFormalHumanoidSkeletonValidatorTests
    {
        [Test]
        public void Analyze_ComputesExpectedHumanoidDirections()
        {
            JObject gltf = CreateHumanoidGltf(facePositiveY: true, rightHandNegativeX: true, includeHead: true, includeUpperChest: true);

            var metrics = GltfFormalHumanoidSkeletonValidator.Analyze(gltf, "Master");

            Assert.That(Vector3.Dot(metrics.FaceDirection, Vector3.UnitY), Is.GreaterThan(0.99f));
            Assert.That(Vector3.Dot(metrics.RightHandDirection, -Vector3.UnitX), Is.GreaterThan(0.99f));
            Assert.That(metrics.ResolvedBones.ContainsKey("Head"), Is.True);
            Assert.That(metrics.ResolvedBones.ContainsKey("UpperChest"), Is.True);
        }

        [Test]
        public void Validate_ThrowsWhenHumanoidFacesWrongDirection()
        {
            JObject gltf = CreateHumanoidGltf(facePositiveY: false, rightHandNegativeX: true, includeHead: true, includeUpperChest: true);

            var exception = Assert.Throws<InvalidOperationException>(() =>
                GltfFormalHumanoidSkeletonValidator.Validate(gltf, "wrong_orientation.gltf", "Master"));

            StringAssert.Contains("Expected face +Y", exception!.Message);
        }

        [Test]
        public void Validate_ThrowsWhenRightHandDoesNotPointNegativeX()
        {
            JObject gltf = CreateHumanoidGltf(facePositiveY: true, rightHandNegativeX: false, includeHead: true, includeUpperChest: true);

            var exception = Assert.Throws<InvalidOperationException>(() =>
                GltfFormalHumanoidSkeletonValidator.Validate(gltf, "wrong_right_hand.gltf", "Master"));

            StringAssert.Contains("Expected R_Hand toward -X", exception!.Message);
        }

        [Test]
        public void Validate_ThrowsWhenHeadOrChestProxyBoneIsMissing()
        {
            JObject gltf = CreateHumanoidGltf(facePositiveY: true, rightHandNegativeX: true, includeHead: false, includeUpperChest: false);

            var exception = Assert.Throws<InvalidOperationException>(() =>
                GltfFormalHumanoidSkeletonValidator.Validate(gltf, "missing_head_chest.gltf", "Master"));

            StringAssert.Contains("Head", exception!.Message);
            StringAssert.Contains("UpperChest(Spine2/Chest)", exception.Message);
        }

        private static JObject CreateHumanoidGltf(bool facePositiveY, bool rightHandNegativeX, bool includeHead, bool includeUpperChest)
        {
            var nodes = new JArray();
            var joints = new List<int>();

            int AddNode(string name, int? parentIndex, Vector3 translation)
            {
                var node = new JObject
                {
                    ["name"] = name,
                    ["translation"] = new JArray(translation.X, translation.Y, translation.Z),
                    ["children"] = new JArray(),
                };

                int nodeIndex = nodes.Count;
                nodes.Add(node);
                if (parentIndex.HasValue)
                    ((JArray)nodes[parentIndex.Value]!["children"]!).Add(nodeIndex);

                joints.Add(nodeIndex);
                return nodeIndex;
            }

            int armature = AddNode("Armature", null, Vector3.Zero);
            int master = AddNode("Master", armature, Vector3.Zero);
            int pelvis = AddNode("Pelvis", master, Vector3.Zero);
            int spine = AddNode("Spine", pelvis, new Vector3(0, 0, 0.4f));
            int spine1 = AddNode("Spine1", spine, new Vector3(0, 0, 0.4f));
            int chestParent = spine1;

            if (includeUpperChest)
                chestParent = AddNode("Spine2", spine1, new Vector3(0, 0, 0.3f));

            int neck = AddNode("Neck", chestParent, new Vector3(0, 0, 0.25f));
            if (includeHead)
                AddNode("Head", neck, new Vector3(0, 0, 0.3f));

            float leftX = facePositiveY ? 0.6f : -0.6f;
            float rightX = -leftX;

            int leftShoulder = AddNode("L_Shoulder", chestParent, new Vector3(leftX, 0, 0.05f));
            AddNode("L_Hand", leftShoulder, new Vector3(leftX, 0, -0.05f));
            int rightShoulder = AddNode("R_Shoulder", chestParent, new Vector3(rightX, 0, 0.05f));
            float rightHandX = rightHandNegativeX ? -0.8f : 0.8f;
            AddNode("R_Hand", rightShoulder, new Vector3(rightHandX, 0, -0.05f));

            return new JObject
            {
                ["asset"] = new JObject
                {
                    ["version"] = "2.0",
                },
                ["scenes"] = new JArray(
                    new JObject
                    {
                        ["nodes"] = new JArray(armature),
                    }),
                ["scene"] = 0,
                ["nodes"] = nodes,
                ["skins"] = new JArray(
                    new JObject
                    {
                        ["skeleton"] = master,
                        ["joints"] = new JArray(joints),
                    }),
            };
        }
    }
}
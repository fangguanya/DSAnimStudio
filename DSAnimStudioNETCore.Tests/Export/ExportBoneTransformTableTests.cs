using System;
using System.Collections.Generic;
using System.Numerics;
using DSAnimStudio.Export;
using SoulsFormats;
using NUnit.Framework;

namespace DSAnimStudioNETCore.Tests.Export
{
    [TestFixture]
    public class ExportBoneTransformTests
    {
        // Build a minimal FLVER.Node list simulating the Sekiro skeleton:
        //  0: Master         (parent=-1)
        //  1: RootPos        (parent=0)
        //  2: Pelvis         (parent=1)
        //  3: RootRotY       (parent=1)  <-- should be reparented to Pelvis (2)
        //  4: RootRotXZ      (parent=3)
        //  5: Spine          (parent=4)
        //  6: Spine1         (parent=5)
        //  7: Spine2         (parent=6)
        //  8: L_Hip          (parent=2)
        private static List<FLVER.Node> BuildTestNodes()
        {
            var names = new[] { "Master", "RootPos", "Pelvis", "RootRotY", "RootRotXZ", "Spine", "Spine1", "Spine2", "L_Hip" };
            var parents = new short[] { -1, 0, 1, 1, 3, 4, 5, 6, 2 };
            var nodes = new List<FLVER.Node>();
            for (int i = 0; i < names.Length; i++)
            {
                var n = new FLVER.Node { Name = names[i], ParentIndex = parents[i] };
                // Give Pelvis a non-trivial transform for testing
                if (names[i] == "Pelvis")
                    n.Translation = new Vector3(0, 0, 5);
                if (names[i] == "RootRotY")
                    n.Translation = new Vector3(10, 0, 0);
                nodes.Add(n);
            }
            return nodes;
        }

        [Test]
        public void BuildModified_ReparentsRootRotYToPelvis()
        {
            var original = BuildTestNodes();
            var modified = FormalSceneExportShared.BuildModifiedFlverNodes(original, 1.0f);

            Assert.AreEqual(2, modified[3].ParentIndex, "RootRotY parent should be Pelvis (idx 2)");
        }

        [Test]
        public void BuildModified_OtherParentsUnchanged()
        {
            var original = BuildTestNodes();
            var modified = FormalSceneExportShared.BuildModifiedFlverNodes(original, 1.0f);

            Assert.AreEqual(3, modified[4].ParentIndex, "RootRotXZ parent should remain 3");
            Assert.AreEqual(1, modified[2].ParentIndex, "Pelvis parent should remain 1");
            Assert.AreEqual(2, modified[8].ParentIndex, "L_Hip parent should remain 2");
        }

        [Test]
        public void BuildModified_RenamesSpineBones()
        {
            var original = BuildTestNodes();
            var modified = FormalSceneExportShared.BuildModifiedFlverNodes(original, 1.0f);

            Assert.AreEqual("spine_01", modified[3].Name);
            Assert.AreEqual("spine_02", modified[4].Name);
            Assert.AreEqual("spine_03", modified[5].Name);
            Assert.AreEqual("spine_04", modified[6].Name);
            Assert.AreEqual("spine_05", modified[7].Name);
        }

        [Test]
        public void BuildModified_NonRenamedBonesKeepNames()
        {
            var original = BuildTestNodes();
            var modified = FormalSceneExportShared.BuildModifiedFlverNodes(original, 1.0f);

            Assert.AreEqual("Master", modified[0].Name);
            Assert.AreEqual("RootPos", modified[1].Name);
            Assert.AreEqual("Pelvis", modified[2].Name);
            Assert.AreEqual("L_Hip", modified[8].Name);
        }

        [Test]
        public void BuildModified_DoesNotMutateOriginal()
        {
            var original = BuildTestNodes();
            var modified = FormalSceneExportShared.BuildModifiedFlverNodes(original, 1.0f);

            Assert.AreEqual("RootRotY", original[3].Name, "Original should be unchanged");
            Assert.AreEqual(1, original[3].ParentIndex, "Original parent should be unchanged");
        }

        [Test]
        public void BuildModified_PreservesWorldPosition()
        {
            var original = BuildTestNodes();
            float scaleFactor = 1.0f;

            // Compute old world for RootRotY: walk up old parent chain
            var oldWorld = ComputeWorldMatrix(original, 3, scaleFactor);

            var modified = FormalSceneExportShared.BuildModifiedFlverNodes(original, scaleFactor);

            // Compute new world for RootRotY: walk up new parent chain
            var newWorld = ComputeWorldMatrix(modified, 3, scaleFactor);

            AssertMatricesEqual(oldWorld, newWorld, 1e-4f, "World matrix should be preserved after reparenting");
        }

        [Test]
        public void BuildModified_SkipsReparentWhenBoneNotFound()
        {
            // Skeleton without Pelvis
            var nodes = new List<FLVER.Node>
            {
                new FLVER.Node { Name = "Master", ParentIndex = -1 },
                new FLVER.Node { Name = "RootPos", ParentIndex = 0 },
                new FLVER.Node { Name = "RootRotY", ParentIndex = 1 },
            };

            var modified = FormalSceneExportShared.BuildModifiedFlverNodes(nodes, 1.0f);
            Assert.AreEqual(1, modified[2].ParentIndex, "Should keep original parent when target not found");
        }

        [Test]
        public void GetRenamedBoneName_MapsCorrectly()
        {
            Assert.AreEqual("spine_01", FormalSceneExportShared.GetRenamedBoneName("RootRotY"));
            Assert.AreEqual("spine_02", FormalSceneExportShared.GetRenamedBoneName("RootRotXZ"));
            Assert.AreEqual("spine_03", FormalSceneExportShared.GetRenamedBoneName("Spine"));
            Assert.AreEqual("spine_04", FormalSceneExportShared.GetRenamedBoneName("Spine1"));
            Assert.AreEqual("spine_05", FormalSceneExportShared.GetRenamedBoneName("Spine2"));
            Assert.AreEqual("Pelvis", FormalSceneExportShared.GetRenamedBoneName("Pelvis"));
        }

        // Helper: compute world matrix by walking parent chain (same logic as FormalSceneExportShared.ComputeWorldMatrix)
        private static Matrix4x4 ComputeWorldMatrix(IList<FLVER.Node> nodes, int boneIndex, float scaleFactor)
        {
            return FormalSceneExportShared.ComputeWorldMatrix(nodes, boneIndex, scaleFactor);
        }

        private static void AssertMatricesEqual(Matrix4x4 expected, Matrix4x4 actual, float tolerance, string message)
        {
            Assert.AreEqual(expected.M11, actual.M11, tolerance, $"{message} [M11]");
            Assert.AreEqual(expected.M12, actual.M12, tolerance, $"{message} [M12]");
            Assert.AreEqual(expected.M13, actual.M13, tolerance, $"{message} [M13]");
            Assert.AreEqual(expected.M14, actual.M14, tolerance, $"{message} [M14]");
            Assert.AreEqual(expected.M21, actual.M21, tolerance, $"{message} [M21]");
            Assert.AreEqual(expected.M22, actual.M22, tolerance, $"{message} [M22]");
            Assert.AreEqual(expected.M23, actual.M23, tolerance, $"{message} [M23]");
            Assert.AreEqual(expected.M24, actual.M24, tolerance, $"{message} [M24]");
            Assert.AreEqual(expected.M31, actual.M31, tolerance, $"{message} [M31]");
            Assert.AreEqual(expected.M32, actual.M32, tolerance, $"{message} [M32]");
            Assert.AreEqual(expected.M33, actual.M33, tolerance, $"{message} [M33]");
            Assert.AreEqual(expected.M34, actual.M34, tolerance, $"{message} [M34]");
            Assert.AreEqual(expected.M41, actual.M41, tolerance, $"{message} [M41]");
            Assert.AreEqual(expected.M42, actual.M42, tolerance, $"{message} [M42]");
            Assert.AreEqual(expected.M43, actual.M43, tolerance, $"{message} [M43]");
            Assert.AreEqual(expected.M44, actual.M44, tolerance, $"{message} [M44]");
        }
    }
}

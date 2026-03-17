using System;
using System.IO;
using System.Linq;
using Assimp;
using DSAnimStudio.Export;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
using Matrix4x4 = System.Numerics.Matrix4x4;

namespace DSAnimStudioNETCore.Tests.Export
{
    [TestFixture]
    public class GltfSceneWriterTests
    {
        [Test]
        public void Write_NormalizesWrappersAndClosesSkinAncestors()
        {
            string tempDir = Path.Combine(Path.GetTempPath(), "DSAnimStudioTests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(tempDir);

            try
            {
                string gltfPath = Path.Combine(tempDir, "sample.gltf");
                var scene = BuildTestScene();

                GltfSceneWriter.Write(scene, gltfPath, "Master", "sample_anim");

                JObject root = JObject.Parse(File.ReadAllText(gltfPath));
                var nodes = (JArray)root["nodes"]!;
                var sceneRoots = ((JArray)root["scenes"]![0]!["nodes"]!).Values<int>().ToArray();
                var skin = (JObject)root["skins"]![0]!;
                int skeletonRootIndex = (int)skin["skeleton"]!;
                var joints = skin["joints"]!.Values<int>().ToHashSet();

                Assert.That((string)nodes[skeletonRootIndex]?["name"], Is.EqualTo("Master"));
                Assert.That(sceneRoots.Select(index => (string)nodes[index]?["name"]).ToArray(), Does.Not.Contain("Armature"));
                Assert.That(sceneRoots.Select(index => (string)nodes[index]?["name"]).ToArray(), Does.Contain("Master"));
                Assert.That(joints.Contains(FindNodeIndexByName(nodes, "Master")), Is.True);
                Assert.That(joints.Contains(FindNodeIndexByName(nodes, "RootPos")), Is.True);
                Assert.That(joints.Contains(FindNodeIndexByName(nodes, "Spine")), Is.True);

                var meshNode = nodes.OfType<JObject>().First(node => (string)node["name"] == "Mesh_0");
                Assert.That(meshNode["skin"]?.Value<int>(), Is.EqualTo(0));
                Assert.That(meshNode["skeletons"]!.Values<int>().Single(), Is.EqualTo(skeletonRootIndex));

                Assert.That((int?)root["animations"]?.Count(), Is.EqualTo(1));
                Assert.That((string)root["animations"]?[0]?["name"], Is.EqualTo("sample_anim"));
            }
            finally
            {
                if (Directory.Exists(tempDir))
                    Directory.Delete(tempDir, recursive: true);
            }
        }

        private static int FindNodeIndexByName(JArray nodes, string name)
        {
            for (int i = 0; i < nodes.Count; i++)
            {
                if (string.Equals((string)nodes[i]?["name"], name, StringComparison.Ordinal))
                    return i;
            }

            return -1;
        }

        private static Scene BuildTestScene()
        {
            var scene = new Scene();
            scene.RootNode = new Node("RootNode")
            {
                Transform = AssimpExportTransformUtils.ToAssimpMatrix(Matrix4x4.Identity)
            };

            var armature = new Node("Armature", scene.RootNode)
            {
                Transform = AssimpExportTransformUtils.ToAssimpMatrix(Matrix4x4.Identity)
            };
            scene.RootNode.Children.Add(armature);

            var master = new Node("Master", armature)
            {
                Transform = AssimpExportTransformUtils.ToAssimpMatrix(Matrix4x4.Identity)
            };
            armature.Children.Add(master);

            var rootPos = new Node("RootPos", master)
            {
                Transform = AssimpExportTransformUtils.ToAssimpMatrix(Matrix4x4.CreateTranslation(0, 1, 0))
            };
            master.Children.Add(rootPos);

            var spine = new Node("Spine", rootPos)
            {
                Transform = AssimpExportTransformUtils.ToAssimpMatrix(Matrix4x4.CreateTranslation(0, 0, 1))
            };
            rootPos.Children.Add(spine);

            var material = new Material { Name = "TestMaterial" };
            scene.Materials.Add(material);

            var mesh = new Assimp.Mesh("Mesh_0", PrimitiveType.Triangle)
            {
                MaterialIndex = 0
            };
            mesh.Vertices.AddRange(new[]
            {
                new Vector3D(0, 0, 0),
                new Vector3D(1, 0, 0),
                new Vector3D(0, 1, 0),
            });
            mesh.Normals.AddRange(new[]
            {
                new Vector3D(0, 0, 1),
                new Vector3D(0, 0, 1),
                new Vector3D(0, 0, 1),
            });
            mesh.Faces.Add(new Face(new[] { 0, 1, 2 }));

            var bone = new Bone
            {
                Name = "Spine",
                OffsetMatrix = AssimpExportTransformUtils.ToAssimpMatrix(Matrix4x4.Identity)
            };
            bone.VertexWeights.Add(new VertexWeight(0, 1.0f));
            bone.VertexWeights.Add(new VertexWeight(1, 1.0f));
            bone.VertexWeights.Add(new VertexWeight(2, 1.0f));
            mesh.Bones.Add(bone);
            scene.Meshes.Add(mesh);

            var meshNode = new Node("Mesh_0", scene.RootNode)
            {
                Transform = AssimpExportTransformUtils.ToAssimpMatrix(Matrix4x4.Identity)
            };
            meshNode.MeshIndices.Add(0);
            scene.RootNode.Children.Add(meshNode);

            var animation = new Animation
            {
                Name = "sample_anim",
                DurationInTicks = 1,
                TicksPerSecond = 30,
            };
            var channel = new NodeAnimationChannel { NodeName = "Spine" };
            channel.PositionKeys.Add(new VectorKey(0, new Vector3D(0, 0, 1)));
            channel.PositionKeys.Add(new VectorKey(1, new Vector3D(0, 0, 2)));
            channel.RotationKeys.Add(new QuaternionKey(0, new Assimp.Quaternion(1, 0, 0, 0)));
            channel.RotationKeys.Add(new QuaternionKey(1, new Assimp.Quaternion(1, 0, 0, 0)));
            channel.ScalingKeys.Add(new VectorKey(0, new Vector3D(1, 1, 1)));
            channel.ScalingKeys.Add(new VectorKey(1, new Vector3D(1, 1, 1)));
            animation.NodeAnimationChannels.Add(channel);
            scene.Animations.Add(animation);

            return scene;
        }
    }
}
using DSAnimStudio.Export;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
using SoulsAssetPipeline.Animation;
using System.Collections.Generic;
using System.Numerics;

namespace DSAnimStudioNETCore.Tests.Export
{
    [TestFixture]
    public class FormalExportContractTests
    {
        private sealed class TestHavokAnimationData : HavokAnimationData
        {
            public TestHavokAnimationData(HKX.HKASkeleton skeleton, HKX.HKADefaultAnimatedReferenceFrame refFrame, int frameCount)
                : base(0, "test", skeleton, refFrame, null)
            {
                FrameCount = frameCount;
                HkxBoneIndexToTransformTrackMap = new[] { -1 };
                TransformTrackIndexToHkxBoneMap = new int[0];
            }

            public override NewBlendableTransform GetTransformOnFrame(int transformIndex, float frame, bool enableLooping)
            {
                return NewBlendableTransform.Identity;
            }
        }

        [Test]
        public void FormalTextureContract_NormalizesFormatsAndBuildsStableRelativePath()
        {
            Assert.That(FormalTextureContract.NormalizeSourceFormat("Pfim.ImageFormat.Dxt5"), Is.EqualTo("DXT5"));
            Assert.That(FormalTextureContract.IsFormalSourceFormat("bc7"), Is.True);
            Assert.That(FormalTextureContract.BuildFormalTextureFileName("m1234_a"), Is.EqualTo("m1234_a.png"));
            Assert.That(FormalTextureContract.BuildRelativeTexturePath("m1234_a.png"), Is.EqualTo("Textures/m1234_a.png"));
            Assert.That(FormalTextureContract.GetColorSpace("BaseColor"), Is.EqualTo("sRGB"));
            Assert.That(FormalTextureContract.GetColorSpace("Normal"), Is.EqualTo("Linear"));
        }

        [Test]
        public void FormalAssetPackageSummary_SerializesDeliverablesAsStableObjectMap()
        {
            var summary = new FormalAssetPackageSummary
            {
                CharacterId = "c1020",
                FormalSuccess = true,
            };

            var model = summary.GetOrAdd("model");
            model.Status = "ok";
            model.Format = "gltf";
            model.RelativePath = "Model";
            model.FileCount = 1;
            model.Files.Add("Model/c1020.gltf");

            var textures = summary.GetOrAdd("textures");
            textures.Status = "ok";
            textures.Format = "png";
            textures.RelativePath = "Textures";
            textures.FileCount = 2;
            textures.Files.Add("Textures/a.png");
            textures.Files.Add("Textures/b.png");

            summary.UnexpectedArtifacts.Add("Debug/legacy.txt");

            JObject json = summary.ToJson();

            Assert.That((string)json["characterId"], Is.EqualTo("c1020"));
            Assert.That((bool?)json["formalSuccess"], Is.True);
            Assert.That(json["deliverables"], Is.TypeOf<JObject>());
            Assert.That((string)json["deliverables"]?["model"]?["status"], Is.EqualTo("ok"));
            Assert.That((int?)json["deliverables"]?["textures"]?["fileCount"], Is.EqualTo(2));
            Assert.That((string?)json["unexpectedArtifacts"]?[0], Is.EqualTo("Debug/legacy.txt"));
        }

        [Test]
        public void FormalAnimationResolution_SerializesRootMotionWhenPresent()
        {
            var resolution = new FormalAnimationResolution
            {
                RequestTaeId = 100,
                RequestTaeName = "a000_000100",
                ResolvedTaeId = 100,
                ResolvedTaeName = "a000_000100",
                ResolvedHkxId = 100,
                ResolvedHkxName = "a000_000100",
                DeliverableAnimFileName = "a000_000100.gltf",
                RootMotion = new FormalRootMotionTrack
                {
                    FrameRate = 30.0f,
                    DurationSeconds = 1.0f,
                    Samples =
                    {
                        new FormalRootMotionSample { FrameIndex = 0, TimeSeconds = 0.0f, X = 0.0f, Y = 0.0f, Z = 0.0f, YawRadians = 0.0f },
                        new FormalRootMotionSample { FrameIndex = 30, TimeSeconds = 1.0f, X = 100.0f, Y = 0.0f, Z = 0.0f, YawRadians = 1.57f },
                    },
                },
            };

            JObject json = resolution.ToJson();

            Assert.That((int?)json["rootMotion"]?["sampleCount"], Is.EqualTo(2));
            Assert.That((float?)json["rootMotion"]?["totalTranslation"]?["x"], Is.EqualTo(100.0f));
            Assert.That((float?)json["rootMotion"]?["totalYawRadians"], Is.EqualTo(1.57f));
        }

        [Test]
        public void AnimationToFbxExporter_BakesAnimationRootMotionIntoRootBoneChannel()
        {
            var exporter = new AnimationToFbxExporter(new AnimationToFbxExporter.ExportOptions
            {
                ScaleFactor = 1.0f,
                FrameRate = 30.0f,
                BakeRootMotion = true,
            });

            var skeleton = new HKX.HKASkeleton
            {
                Bones = new HKX.HKArray<HKX.Bone>(new HKX.HKArrayData<HKX.Bone>
                {
                    Elements = new List<HKX.Bone>
                    {
                        new HKX.Bone { Name = new HKX.HKCString("Master") },
                    },
                }),
                ParentIndices = new HKX.HKArray<HKX.HKShort>(new HKX.HKArrayData<HKX.HKShort>
                {
                    Elements = new List<HKX.HKShort>
                    {
                        new HKX.HKShort(-1),
                    },
                }),
                Transforms = new HKX.HKArray<HKX.Transform>(new HKX.HKArrayData<HKX.Transform>
                {
                    Elements = new List<HKX.Transform>
                    {
                        new HKX.Transform
                        {
                            Position = new HKX.HKVector4(new Vector4(0, 0, 0, 0)),
                            Rotation = new HKX.HKVector4(new Vector4(0, 0, 0, 1)),
                            Scale = new HKX.HKVector4(new Vector4(1, 1, 1, 0)),
                        },
                    },
                }),
            };

            var refFrame = new HKX.HKADefaultAnimatedReferenceFrame
            {
                Up = new Vector4(0, 1, 0, 0),
                Forward = new Vector4(1, 0, 0, 0),
                Duration = 1.0f / 30.0f,
                ReferenceFrameSamples = new HKX.HKArray<HKX.HKVector4>(new HKX.HKArrayData<HKX.HKVector4>
                {
                    Elements = new List<HKX.HKVector4>
                    {
                        new HKX.HKVector4(new Vector4(0, 0, 0, 0)),
                        new HKX.HKVector4(new Vector4(10, 0, 0, 0)),
                    },
                }),
            };

            var animData = new TestHavokAnimationData(skeleton, refFrame, 1);

            var animation = exporter.BuildAnimation(skeleton, animData, "test", animData.RootMotion);

            Assert.That(animation.NodeAnimationChannels.Count, Is.EqualTo(1));
            Assert.That(animation.NodeAnimationChannels[0].PositionKeys.Count, Is.EqualTo(2));

            var firstKey = animation.NodeAnimationChannels[0].PositionKeys[0].Value;
            var lastKey = animation.NodeAnimationChannels[0].PositionKeys[1].Value;

            Assert.That(firstKey.X, Is.EqualTo(0).Within(0.0001f));
            Assert.That(firstKey.Y, Is.EqualTo(0).Within(0.0001f));
            Assert.That(firstKey.Z, Is.EqualTo(0).Within(0.0001f));
            Assert.That(lastKey.X, Is.EqualTo(-10).Within(0.0001f));
            Assert.That(lastKey.Y, Is.EqualTo(0).Within(0.0001f));
            Assert.That(lastKey.Z, Is.EqualTo(0).Within(0.0001f));
        }
    }
}

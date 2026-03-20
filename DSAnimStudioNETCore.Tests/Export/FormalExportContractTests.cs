using DSAnimStudio.Export;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
using SoulsAssetPipeline.Animation;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Reflection;

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

        private static HKX.HKASkeleton CreateSingleBoneSkeleton()
        {
            return new HKX.HKASkeleton
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
        }

        private static HKX.HKADefaultAnimatedReferenceFrame CreateReferenceFrame(params Vector4[] frames)
        {
            return new HKX.HKADefaultAnimatedReferenceFrame
            {
                Up = new Vector4(0, 1, 0, 0),
                Forward = new Vector4(1, 0, 0, 0),
                Duration = 1.0f / 30.0f,
                ReferenceFrameSamples = new HKX.HKArray<HKX.HKVector4>(new HKX.HKArrayData<HKX.HKVector4>
                {
                    Elements = new List<HKX.HKVector4>(frames.Select(frame => new HKX.HKVector4(frame))),
                }),
            };
        }

        [Test]
        public void FormalTextureContract_NormalizesFormatsAndBuildsStableRelativePath()
        {
            Assert.That(FormalTextureContract.NormalizeSourceFormat("Pfim.ImageFormat.Dxt5"), Is.EqualTo("DXT5"));
            Assert.That(FormalTextureContract.IsFormalSourceFormat("bc7"), Is.True);
            Assert.That(FormalTextureContract.BuildFormalTextureFileName("m1234_a"), Is.EqualTo("m1234_a.png"));
            Assert.That(FormalTextureContract.BuildRelativeTexturePath("m1234_a.png"), Is.EqualTo("Textures/m1234_a.png"));
            Assert.That(FormalTextureContract.BuildRelativeModelTexturePath("m1234_a.png"), Is.EqualTo("../Textures/m1234_a.png"));
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

            var skeleton = CreateSingleBoneSkeleton();
            var refFrame = CreateReferenceFrame(
                new Vector4(0, 0, 0, 0),
                new Vector4(10, 0, 0, 0));

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

        [Test]
        public void AnimationToFbxExporter_KeepsRootMotionSamplingInSourceSpaceUntilMatrixConversion()
        {
            var exporter = new AnimationToFbxExporter(new AnimationToFbxExporter.ExportOptions
            {
                ScaleFactor = 1.0f,
                FrameRate = 30.0f,
                BakeRootMotion = true,
            });

            var animData = new TestHavokAnimationData(
                CreateSingleBoneSkeleton(),
                CreateReferenceFrame(
                    new Vector4(0, 0, 0, 0),
                    new Vector4(10, 0, 5, 0)),
                1);

            var sampledRootMotion = exporter.GetRootMotionAtFrame(animData.RootMotion, 1, 1);

            Assert.That(sampledRootMotion.X, Is.EqualTo(10).Within(0.0001f));
            Assert.That(sampledRootMotion.Y, Is.EqualTo(0).Within(0.0001f));
            Assert.That(sampledRootMotion.Z, Is.EqualTo(5).Within(0.0001f));
        }

        [Test]
        public void AnimationToFbxExporter_FormalRootMotionTrackMatchesExportedGltfBasis()
        {
            var exporter = new AnimationToFbxExporter(new AnimationToFbxExporter.ExportOptions
            {
                ScaleFactor = 1.0f,
                FrameRate = 30.0f,
                BakeRootMotion = true,
            });

            var animData = new TestHavokAnimationData(
                CreateSingleBoneSkeleton(),
                CreateReferenceFrame(
                    new Vector4(0, 0, 0, 0),
                    new Vector4(10, 0, 5, 0.25f)),
                1);

            var buildRootMotionTrack = typeof(AnimationToFbxExporter).GetMethod("BuildRootMotionTrack", BindingFlags.Instance | BindingFlags.NonPublic);
            Assert.That(buildRootMotionTrack, Is.Not.Null);

            var track = (FormalRootMotionTrack)buildRootMotionTrack.Invoke(exporter, new object[] { animData.RootMotion, animData.FrameCount });

            Assert.That(track, Is.Not.Null);
            Assert.That(track.Samples.Count, Is.EqualTo(2));
            Assert.That(track.Samples[1].X, Is.EqualTo(-10).Within(0.0001f));
            Assert.That(track.Samples[1].Y, Is.EqualTo(0).Within(0.0001f));
            Assert.That(track.Samples[1].Z, Is.EqualTo(-5).Within(0.0001f));
            Assert.That(track.Samples[1].YawRadians, Is.EqualTo(0.25f).Within(0.0001f));
        }
    }

    [TestFixture]
    public class FormalFailClosedTests
    {
        [Test]
        public void FormalAssetPackageSummary_MissingRequiredDeliverable_MarksFormalSuccessFalse()
        {
            var summary = new FormalAssetPackageSummary
            {
                CharacterId = "c0000",
                FormalSuccess = true,
            };

            var model = summary.GetOrAdd("model", required: true);
            model.Status = "ready";

            var textures = summary.GetOrAdd("textures", required: true);
            textures.Status = "missing";

            summary.FormalSuccess = summary.Deliverables
                .Where(kvp => kvp.Value.Required)
                .All(kvp => kvp.Value.Status == "ready");

            Assert.That(summary.FormalSuccess, Is.False, "FormalSuccess must be false when a required deliverable is missing");
        }

        [Test]
        public void FormalAssetPackageSummary_MissingOptionalDeliverable_DoesNotAffectFormalSuccess()
        {
            var summary = new FormalAssetPackageSummary
            {
                CharacterId = "c0000",
            };

            var model = summary.GetOrAdd("model", required: true);
            model.Status = "ready";

            var debug = summary.GetOrAdd("debugArtifacts", required: false);
            debug.Status = "missing";

            summary.FormalSuccess = summary.Deliverables
                .Where(kvp => kvp.Value.Required)
                .All(kvp => kvp.Value.Status == "ready");

            Assert.That(summary.FormalSuccess, Is.True, "FormalSuccess must be true when only optional deliverables are missing");
        }

        [Test]
        public void FormalAssetPackageSummary_MaterialManifestFailure_RecordsFailureReason()
        {
            var summary = new FormalAssetPackageSummary { CharacterId = "c0000" };
            var materialManifest = summary.GetOrAdd("materialManifest", required: true);
            materialManifest.Status = "failed";
            materialManifest.FailureReasons.Add("Referenced texture 'c0000_a' not exported");

            Assert.That(materialManifest.FailureReasons.Count, Is.EqualTo(1));
            Assert.That(materialManifest.Status, Is.EqualTo("failed"));
        }

        [Test]
        public void FormalAnimationResolution_MissingRootMotion_MarksResolutionFailed()
        {
            var resolution = new FormalAnimationResolution
            {
                RequestTaeId = 100,
                RequestTaeName = "a000_000100",
                RootMotionSourcePresent = true,
                RootMotion = null,
                DeliverableAnimFileName = "a000_000100.gltf",
            };

            Assert.That(resolution.RootMotionSourcePresent, Is.True);
            Assert.That(resolution.RootMotion, Is.Null, "RootMotion must be null when extraction fails");
        }

        [Test]
        public void FormalRootMotionTrack_ZeroSamples_IndicatesMissingRootMotion()
        {
            var track = new FormalRootMotionTrack
            {
                FrameRate = 30.0f,
                DurationSeconds = 1.0f,
            };

            Assert.That(track.Samples.Count, Is.EqualTo(0), "Empty samples indicate missing root-motion");
        }

        [Test]
        public void FormalTextureContract_UnsupportedFormat_MarksAsFailure()
        {
            var supportedFormats = new[] { "BC7", "DXT1", "DXT3", "DXT5", "BC4", "BC5", "BC6H", "ATI1", "ATI2", "A8", "R8G8B8A8" };
            var unsupportedFormat = "BC2";

            var isFormal = FormalTextureContract.IsFormalSourceFormat(unsupportedFormat);
            Assert.That(isFormal, Is.False, "BC2 is not a formal texture format");
        }
    }

    [TestFixture]
    public class MaterialManifestExporterTests
    {
        [Test]
        public void MaterialManifestExporter_TextureBindingsIsJObjectNotArray()
        {
            var flver = CreateTestFlver();
            var exporter = new MaterialManifestExporter();
            var manifest = exporter.GenerateManifest(flver, ".png");
            var materials = manifest["materials"] as JArray;
            Assert.That(materials, Is.Not.Null);
            Assert.That(materials.Count, Is.GreaterThan(0));
            var firstMat = materials[0] as JObject;
            Assert.That(firstMat, Is.Not.Null);
            var textureBindings = firstMat["textureBindings"];
            Assert.That(textureBindings, Is.TypeOf<JObject>(), "textureBindings should be JObject (dictionary), not JArray");
        }

        [Test]
        public void MaterialManifestExporter_TextureBindingsKeysAreParameterNames()
        {
            var flver = CreateTestFlver();
            var exporter = new MaterialManifestExporter();
            var manifest = exporter.GenerateManifest(flver, ".png");
            var materials = manifest["materials"] as JArray;
            var firstMat = materials[0] as JObject;
            var textureBindings = firstMat["textureBindings"] as JObject;
            Assert.That(textureBindings, Is.Not.Null);
            foreach (var binding in textureBindings.Properties())
            {
                var paramNames = new[] { "BaseColor", "BaseColor2", "Normal", "Normal2", "Specular", "Specular2", "Emissive", "Emissive2", "Roughness", "Roughness2", "BlendMask", "BlendMask3", "" };
                Assert.That(paramNames, Contains.Item(binding.Name), 
                    $"Key '{binding.Name}' should be a valid parameter name");
            }
        }

        [Test]
        public void MaterialManifestExporter_TextureBindingsContainsRequiredFields()
        {
            var flver = CreateTestFlver();
            var exporter = new MaterialManifestExporter();
            var manifest = exporter.GenerateManifest(flver, ".png");
            var materials = manifest["materials"] as JArray;
            var firstMat = materials[0] as JObject;
            var textureBindings = firstMat["textureBindings"] as JObject;
            Assert.That(textureBindings, Is.Not.Null);
            foreach (var binding in textureBindings.Properties())
            {
                var texInfo = binding.Value as JObject;
                Assert.That(texInfo, Is.Not.Null);
                Assert.That(texInfo["slotType"], Is.Not.Null, "slotType is required");
                Assert.That(texInfo["slotIndex"], Is.Not.Null, "slotIndex is required");
                Assert.That(texInfo["exportedFileName"], Is.Not.Null, "exportedFileName is required");
                Assert.That(texInfo["relativePath"], Is.Not.Null, "relativePath is required");
                Assert.That(texInfo["colorSpace"], Is.Not.Null, "colorSpace is required");
            }
        }

        [Test]
        public void FormalMaterialTextureResolver_PreservesIndexedTextureParameters()
        {
            var flver = new SoulsFormats.FLVER2();
            flver.Materials.Add(new SoulsFormats.FLVER2.Material
            {
                Name = "IndexedMaterial",
                MTD = "Test.mtd",
                Textures = new List<SoulsFormats.FLVER2.Texture>
                {
                    new SoulsFormats.FLVER2.Texture { Type = "g_Diffuse", Path = @"N:\SPRJ\data\Model\chr\c0000\body_d.tga" },
                    new SoulsFormats.FLVER2.Texture { Type = "g_Diffuse_2", Path = @"N:\SPRJ\data\Model\chr\c0000\body_d2.tga" },
                    new SoulsFormats.FLVER2.Texture { Type = "g_BlendMask", Path = @"N:\SPRJ\data\Model\chr\c0000\body_m.tga" },
                    new SoulsFormats.FLVER2.Texture { Type = "g_Bumpmap", Path = @"N:\SPRJ\data\Model\chr\c0000\body_n.tga" },
                    new SoulsFormats.FLVER2.Texture { Type = "g_Bumpmap_2", Path = @"N:\SPRJ\data\Model\chr\c0000\body_n2.tga" },
                }
            });

            var bindings = FormalMaterialTextureResolver.Resolve(flver.Materials[0]).ToList();

            Assert.That(bindings.Any(binding => binding.ParameterName == "BaseColor"), Is.True);
            Assert.That(bindings.Any(binding => binding.ParameterName == "BaseColor2"), Is.True);
            Assert.That(bindings.Any(binding => binding.ParameterName == "BlendMask"), Is.True);
            Assert.That(bindings.Any(binding => binding.ParameterName == "Normal"), Is.True);
            Assert.That(bindings.Any(binding => binding.ParameterName == "Normal2"), Is.True);
        }

        [Test]
        public void MaterialManifestExporter_MaterialInstanceKeyFormat()
        {
            var flver = CreateTestFlver();
            var exporter = new MaterialManifestExporter();
            var manifest = exporter.GenerateManifest(flver, ".png");
            var materials = manifest["materials"] as JArray;
            var firstMat = materials[0] as JObject;
            var materialInstanceKey = (string)firstMat["materialInstanceKey"];
            Assert.That(materialInstanceKey, Is.Not.Null);
            Assert.That(materialInstanceKey, Does.StartWith("MI_"), "materialInstanceKey should start with MI_");
        }

        [Test]
        public void MaterialManifestExporter_MultipleTexturesPerMaterial()
        {
            var flver = CreateMultiTextureFlver();
            var exporter = new MaterialManifestExporter();
            var manifest = exporter.GenerateManifest(flver, ".png");
            var materials = manifest["materials"] as JArray;
            var firstMat = materials[0] as JObject;
            var textureBindings = firstMat["textureBindings"] as JObject;
            Assert.That(textureBindings, Is.Not.Null);
            Assert.That(textureBindings.Count, Is.GreaterThanOrEqualTo(2), "Should have multiple texture bindings");
        }

        private static SoulsFormats.FLVER2 CreateTestFlver()
        {
            var flver = new SoulsFormats.FLVER2();
            flver.Materials.Add(new SoulsFormats.FLVER2.Material
            {
                Name = "TestMaterial",
                MTD = "Test.mtd",
                Textures = new List<SoulsFormats.FLVER2.Texture>
                {
                    new SoulsFormats.FLVER2.Texture
                    {
                        Type = "g_Diffuse",
                        Path = @"N:\SPRJ\data\Model\chr\c0000\c0000_a.tga"
                    }
                }
            });
            flver.Meshes.Add(new SoulsFormats.FLVER2.Mesh());
            flver.Meshes[0].MaterialIndex = 0;
            return flver;
        }

        private static SoulsFormats.FLVER2 CreateMultiTextureFlver()
        {
            var flver = new SoulsFormats.FLVER2();
            flver.Materials.Add(new SoulsFormats.FLVER2.Material
            {
                Name = "TestMaterial",
                MTD = "Test.mtd",
                Textures = new List<SoulsFormats.FLVER2.Texture>
                {
                    new SoulsFormats.FLVER2.Texture
                    {
                        Type = "g_Diffuse",
                        Path = @"N:\SPRJ\data\Model\chr\c0000\c0000_a.tga"
                    },
                    new SoulsFormats.FLVER2.Texture
                    {
                        Type = "g_Bumpmap",
                        Path = @"N:\SPRJ\data\Model\chr\c0000\c0000_n.tga"
                    },
                    new SoulsFormats.FLVER2.Texture
                    {
                        Type = "g_Specular",
                        Path = @"N:\SPRJ\data\Model\chr\c0000\c0000_s.tga"
                    }
                }
            });
            flver.Meshes.Add(new SoulsFormats.FLVER2.Mesh());
            flver.Meshes[0].MaterialIndex = 0;
            return flver;
        }
    }
}

using DSAnimStudio.Export;
using NUnit.Framework;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudioNETCore.Tests.Export
{
    [TestFixture]
    public class FormalSceneExportSharedTests
    {
        [Test]
        public void ConvertSourceVectorToGltf_MapsSourceForwardToNegativeX()
        {
            var result = FormalSceneExportShared.ConvertSourceVectorToGltf(Vector3.UnitZ);

            Assert.That(result.X, Is.EqualTo(-1).Within(1e-5));
            Assert.That(result.Y, Is.EqualTo(0).Within(1e-5));
            Assert.That(result.Z, Is.EqualTo(0).Within(1e-5));
        }

        [Test]
        public void ConvertSourceVectorToGltf_MapsSourceUpToPositiveY()
        {
            var result = FormalSceneExportShared.ConvertSourceVectorToGltf(Vector3.UnitY);

            Assert.That(result.X, Is.EqualTo(0).Within(1e-5));
            Assert.That(result.Y, Is.EqualTo(1).Within(1e-5));
            Assert.That(result.Z, Is.EqualTo(0).Within(1e-5));
        }

        [Test]
        public void ConvertSourceVectorToGltf_MapsSourcePositiveXToNegativeZ()
        {
            var result = FormalSceneExportShared.ConvertSourceVectorToGltf(Vector3.UnitX);

            Assert.That(result.X, Is.EqualTo(0).Within(1e-5));
            Assert.That(result.Y, Is.EqualTo(0).Within(1e-5));
            Assert.That(result.Z, Is.EqualTo(-1).Within(1e-5));
        }

        [Test]
        public void ConvertSourceVectorToGltf_PreservesUnitLength()
        {
            var source = Vector3.Normalize(new Vector3(1, 2, 3));
            var result = FormalSceneExportShared.ConvertSourceVectorToGltf(source);

            Assert.That(result.Length(), Is.EqualTo(1).Within(1e-5));
        }
    }
}
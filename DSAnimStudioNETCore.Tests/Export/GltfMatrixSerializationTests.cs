using System;
using DSAnimStudio.Export;
using Newtonsoft.Json.Linq;
using NUnit.Framework;
using Matrix4x4 = System.Numerics.Matrix4x4;
using Vector3 = System.Numerics.Vector3;

namespace DSAnimStudioNETCore.Tests.Export
{
    [TestFixture]
    public class GltfMatrixSerializationTests
    {
        [Test]
        public void ParseMatrix_UsesGltfColumnMajorOrder()
        {
            var gltfMatrix = new JArray(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                3, 4, 5, 1);

            Matrix4x4 matrix = GltfMatrixSerialization.ParseMatrix(gltfMatrix);
            Vector3 transformed = Vector3.Transform(Vector3.Zero, matrix);

            Assert.That(transformed.X, Is.EqualTo(3).Within(1e-5));
            Assert.That(transformed.Y, Is.EqualTo(4).Within(1e-5));
            Assert.That(transformed.Z, Is.EqualTo(5).Within(1e-5));
        }

        [Test]
        public void WriteMatrix_WritesColumnMajorOrder()
        {
            var bytes = new System.Collections.Generic.List<byte>();
            var matrix = new Matrix4x4(
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16);

            GltfMatrixSerialization.WriteMatrix(bytes, matrix);

            Assert.That(BitConverter.ToSingle(bytes.ToArray(), 0), Is.EqualTo(1));
            Assert.That(BitConverter.ToSingle(bytes.ToArray(), 4), Is.EqualTo(2));
            Assert.That(BitConverter.ToSingle(bytes.ToArray(), 8), Is.EqualTo(3));
            Assert.That(BitConverter.ToSingle(bytes.ToArray(), 12), Is.EqualTo(4));
            Assert.That(BitConverter.ToSingle(bytes.ToArray(), 48), Is.EqualTo(13));
        }
    }
}
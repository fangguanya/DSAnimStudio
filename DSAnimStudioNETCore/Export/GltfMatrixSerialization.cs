using System;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;
using Matrix4x4 = System.Numerics.Matrix4x4;

namespace DSAnimStudio.Export
{
    internal static class GltfMatrixSerialization
    {
        public static Matrix4x4 ParseMatrix(JArray matrixArray)
        {
            if (matrixArray == null || matrixArray.Count != 16)
                throw new InvalidOperationException("glTF matrix must contain exactly 16 elements.");

            return new Matrix4x4(
                (float)matrixArray[0], (float)matrixArray[1], (float)matrixArray[2], (float)matrixArray[3],
                (float)matrixArray[4], (float)matrixArray[5], (float)matrixArray[6], (float)matrixArray[7],
                (float)matrixArray[8], (float)matrixArray[9], (float)matrixArray[10], (float)matrixArray[11],
                (float)matrixArray[12], (float)matrixArray[13], (float)matrixArray[14], (float)matrixArray[15]);
        }

        public static Matrix4x4 ReadMatrix(byte[] bytes, int offset)
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

        public static void WriteMatrix(List<byte> bytes, Matrix4x4 matrix)
        {
            WriteFloat(bytes, matrix.M11);
            WriteFloat(bytes, matrix.M12);
            WriteFloat(bytes, matrix.M13);
            WriteFloat(bytes, matrix.M14);
            WriteFloat(bytes, matrix.M21);
            WriteFloat(bytes, matrix.M22);
            WriteFloat(bytes, matrix.M23);
            WriteFloat(bytes, matrix.M24);
            WriteFloat(bytes, matrix.M31);
            WriteFloat(bytes, matrix.M32);
            WriteFloat(bytes, matrix.M33);
            WriteFloat(bytes, matrix.M34);
            WriteFloat(bytes, matrix.M41);
            WriteFloat(bytes, matrix.M42);
            WriteFloat(bytes, matrix.M43);
            WriteFloat(bytes, matrix.M44);
        }

        private static void WriteFloat(List<byte> bytes, float value)
        {
            bytes.AddRange(BitConverter.GetBytes(value));
        }
    }
}
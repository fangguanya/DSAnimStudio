using System;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json.Linq;

namespace DSAnimStudio.Export
{
    public sealed class FormalRootMotionSample
    {
        public int FrameIndex { get; init; }
        public float TimeSeconds { get; init; }
        public float X { get; init; }
        public float Y { get; init; }
        public float Z { get; init; }
        public float YawRadians { get; init; }

        public JObject ToJson()
        {
            return new JObject
            {
                ["frameIndex"] = FrameIndex,
                ["timeSeconds"] = TimeSeconds,
                ["translation"] = new JObject
                {
                    ["x"] = X,
                    ["y"] = Y,
                    ["z"] = Z,
                },
                ["yawRadians"] = YawRadians,
            };
        }
    }

    public sealed class FormalRootMotionTrack
    {
        public float FrameRate { get; init; }
        public float DurationSeconds { get; init; }
        public List<FormalRootMotionSample> Samples { get; } = new List<FormalRootMotionSample>();

        public JObject ToJson()
        {
            var samples = Samples ?? new List<FormalRootMotionSample>();
            var first = samples.Count > 0 ? samples[0] : null;
            var last = samples.Count > 0 ? samples[samples.Count - 1] : null;

            return new JObject
            {
                ["frameRate"] = FrameRate,
                ["durationSeconds"] = DurationSeconds,
                ["sampleCount"] = samples.Count,
                ["totalTranslation"] = new JObject
                {
                    ["x"] = last?.X ?? 0.0f,
                    ["y"] = last?.Y ?? 0.0f,
                    ["z"] = last?.Z ?? 0.0f,
                },
                ["totalYawRadians"] = last?.YawRadians ?? 0.0f,
                ["samples"] = new JArray(samples.Select(sample => sample.ToJson())),
            };
        }
    }
}
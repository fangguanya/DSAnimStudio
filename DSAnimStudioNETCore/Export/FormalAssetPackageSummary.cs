using System;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json.Linq;

namespace DSAnimStudio.Export
{
    public sealed class FormalAssetPackageSummary
    {
        public sealed class Deliverable
        {
            public string Id { get; init; }
            public bool Required { get; init; } = true;
            public string Status { get; set; } = "missing";
            public string Format { get; set; } = string.Empty;
            public string RelativePath { get; set; } = string.Empty;
            public int FileCount { get; set; }
            public IList<string> Files { get; } = new List<string>();
            public IList<string> FailureReasons { get; } = new List<string>();

            public JObject ToJson()
            {
                return new JObject
                {
                    ["required"] = Required,
                    ["status"] = Status ?? string.Empty,
                    ["format"] = Format ?? string.Empty,
                    ["relativePath"] = RelativePath ?? string.Empty,
                    ["fileCount"] = FileCount,
                    ["files"] = new JArray(Files.OrderBy(file => file, StringComparer.Ordinal)),
                    ["failureReasons"] = new JArray(FailureReasons),
                };
            }
        }

        public string SchemaVersion { get; init; } = "1.0";
        public string CharacterId { get; init; }
        public string DeliveryMode { get; init; } = "formal-only";
        public bool FormalSuccess { get; set; }
        public IList<string> UnexpectedArtifacts { get; } = new List<string>();
        public IDictionary<string, Deliverable> Deliverables { get; } = new Dictionary<string, Deliverable>(StringComparer.Ordinal);

        public Deliverable GetOrAdd(string id, bool required = true)
        {
            if (!Deliverables.TryGetValue(id, out var deliverable))
            {
                deliverable = new Deliverable { Id = id, Required = required };
                Deliverables.Add(id, deliverable);
            }

            return deliverable;
        }

        public JObject ToJson()
        {
            var deliverables = new JObject();
            foreach (var kvp in Deliverables.OrderBy(entry => entry.Key, StringComparer.Ordinal))
                deliverables[kvp.Key] = kvp.Value.ToJson();

            return new JObject
            {
                ["schemaVersion"] = SchemaVersion,
                ["characterId"] = CharacterId ?? string.Empty,
                ["deliveryMode"] = DeliveryMode,
                ["formalSuccess"] = FormalSuccess,
                ["deliverables"] = deliverables,
                ["unexpectedArtifacts"] = new JArray(UnexpectedArtifacts.OrderBy(item => item, StringComparer.Ordinal)),
            };
        }
    }
}
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

            public static Deliverable FromJson(string id, JObject json)
            {
                if (string.IsNullOrWhiteSpace(id))
                    throw new ArgumentException("Deliverable id is required.", nameof(id));

                if (json == null)
                    throw new ArgumentNullException(nameof(json));

                var deliverable = new Deliverable
                {
                    Id = id,
                    Required = (bool?)json["required"] ?? true,
                    Status = (string)json["status"] ?? "missing",
                    Format = (string)json["format"] ?? string.Empty,
                    RelativePath = (string)json["relativePath"] ?? string.Empty,
                    FileCount = (int?)json["fileCount"] ?? 0,
                };

                foreach (string file in (json["files"] as JArray)?.Values<string>() ?? Enumerable.Empty<string>())
                    deliverable.Files.Add(file);

                foreach (string reason in (json["failureReasons"] as JArray)?.Values<string>() ?? Enumerable.Empty<string>())
                    deliverable.FailureReasons.Add(reason);

                return deliverable;
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

        public static FormalAssetPackageSummary FromJson(JObject json)
        {
            if (json == null)
                throw new ArgumentNullException(nameof(json));

            var summary = new FormalAssetPackageSummary
            {
                SchemaVersion = (string)json["schemaVersion"] ?? "1.0",
                CharacterId = (string)json["characterId"],
                DeliveryMode = (string)json["deliveryMode"] ?? "formal-only",
                FormalSuccess = (bool?)json["formalSuccess"] ?? false,
            };

            foreach (string artifact in (json["unexpectedArtifacts"] as JArray)?.Values<string>() ?? Enumerable.Empty<string>())
                summary.UnexpectedArtifacts.Add(artifact);

            if (json["deliverables"] is JObject deliverables)
            {
                foreach (var property in deliverables.Properties().OrderBy(property => property.Name, StringComparer.Ordinal))
                {
                    if (property.Value is JObject deliverableJson)
                        summary.Deliverables[property.Name] = Deliverable.FromJson(property.Name, deliverableJson);
                }
            }

            return summary;
        }
    }
}
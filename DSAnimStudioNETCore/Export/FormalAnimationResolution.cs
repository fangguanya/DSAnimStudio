using Newtonsoft.Json.Linq;

namespace DSAnimStudio.Export
{
    public sealed class FormalAnimationResolution
    {
        public long RequestTaeId { get; init; }
        public string RequestTaeName { get; init; }
        public long ResolvedTaeId { get; init; }
        public string ResolvedTaeName { get; init; }
        public long ResolvedHkxId { get; init; }
        public string ResolvedHkxName { get; init; }
        public string SourceAnimFileName { get; init; }
        public string SourceAnimStem { get; init; }
        public string DeliverableAnimFileName { get; init; }
        public string AnimationSourceAnibnd { get; init; }
        public string SkillSourceAnibnd { get; init; }

        public JObject ToJson()
        {
            return new JObject
            {
                ["requestTaeId"] = RequestTaeId,
                ["requestTaeName"] = RequestTaeName ?? string.Empty,
                ["resolvedTaeId"] = ResolvedTaeId,
                ["resolvedTaeName"] = ResolvedTaeName ?? string.Empty,
                ["resolvedHkxId"] = ResolvedHkxId,
                ["resolvedHkxName"] = ResolvedHkxName ?? string.Empty,
                ["sourceAnimFileName"] = SourceAnimFileName ?? string.Empty,
                ["sourceAnimStem"] = SourceAnimStem ?? string.Empty,
                ["deliverableAnimFileName"] = DeliverableAnimFileName ?? string.Empty,
                ["animationSourceAnibnd"] = AnimationSourceAnibnd ?? string.Empty,
                ["skillSourceAnibnd"] = SkillSourceAnibnd ?? string.Empty,
            };
        }
    }
}

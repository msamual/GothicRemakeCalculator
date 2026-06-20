using System.Text;
using GothicCalculatorApi.Models;

namespace GothicCalculatorApi.Services;

public static class LockTextSerializer
{
    public const int MinPlates = 2;
    public const int MaxPlates = 8;
    public const int MinPosition = 1;
    public const int MaxPosition = 7;

    public static string? Validate(LockDefinitionRequest request)
    {
        if (request.Rules.Count != request.Start.Count)
        {
            return "plate count mismatch between rules and start positions";
        }

        if (request.Rules.Count < MinPlates || request.Rules.Count > MaxPlates)
        {
            return $"plate count must be between {MinPlates} and {MaxPlates}";
        }

        foreach (var position in request.Start)
        {
            if (position < MinPosition || position > MaxPosition)
            {
                return "invalid start positions (must be 1-7)";
            }
        }

        foreach (var rule in request.Rules)
        {
            if (string.IsNullOrWhiteSpace(rule))
            {
                return "invalid rules line";
            }
        }

        return null;
    }

    public static string Serialize(LockDefinitionRequest request)
    {
        var builder = new StringBuilder();
        builder.Append("Name: ");
        builder.AppendLine(string.IsNullOrWhiteSpace(request.Name) ? "my lock" : request.Name.Trim());
        builder.AppendLine("Rules:");

        for (var i = 0; i < request.Rules.Count; i++)
        {
            builder.Append(i + 1);
            builder.Append(": ");
            builder.AppendLine(request.Rules[i].Trim());
        }

        builder.AppendLine("Start:");
        builder.Append('[');
        builder.Append(string.Join(", ", request.Start));
        builder.AppendLine("]");

        return builder.ToString();
    }
}

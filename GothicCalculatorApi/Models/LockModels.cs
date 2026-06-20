using System.ComponentModel.DataAnnotations;
using System.Text.Json.Serialization;

namespace GothicCalculatorApi.Models;

public sealed class LockDefinitionRequest
{
    public string Name { get; set; } = string.Empty;

    [MinLength(2)]
    [MaxLength(8)]
    public List<string> Rules { get; set; } = [];

    [MinLength(2)]
    [MaxLength(8)]
    public List<int> Start { get; set; } = [];
}

public sealed class LockInstruction
{
    public int Plate { get; set; }
    public int Count { get; set; }
    public string Direction { get; set; } = string.Empty;
}

public sealed class LockSolveResponse
{
    public bool Ok { get; set; }

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? Name { get; set; }

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? Status { get; set; }

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public int? Lines { get; set; }

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public int? Steps { get; set; }

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public List<LockInstruction>? Instructions { get; set; }

    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? Error { get; set; }
}

public sealed class HealthResponse
{
    public string Status { get; set; } = "ok";
    public bool BinaryFound { get; set; }
    public string BinaryPath { get; set; } = string.Empty;
}

using GothicCalculatorApi.Models;
using GothicCalculatorApi.Services;

namespace GothicCalculatorApi.Tests;

public class LockTextSerializerTests
{
    [Fact]
    public void Serialize_ProducesNamelessLocksmithFormat()
    {
        var request = new LockDefinitionRequest
        {
            Name = "Second chest in the tower",
            Rules = ["3r, 6l", "-", "1r, 4l, 6r", "2r, 5r, 6l", "-", "3l"],
            Start = [5, 3, 6, 7, 2, 7]
        };

        var text = LockTextSerializer.Serialize(request);

        Assert.Contains("Name: Second chest in the tower", text);
        Assert.Contains("1: 3r, 6l", text);
        Assert.Contains("6: 3l", text);
        Assert.Contains("[5, 3, 6, 7, 2, 7]", text);
    }

    [Fact]
    public void Validate_RejectsMismatchedCounts()
    {
        var request = new LockDefinitionRequest
        {
            Rules = ["-", "-"],
            Start = [3]
        };

        var error = LockTextSerializer.Validate(request);

        Assert.Equal("plate count mismatch between rules and start positions", error);
    }

    [Fact]
    public void Validate_RejectsInvalidPositions()
    {
        var request = new LockDefinitionRequest
        {
            Rules = ["-", "-"],
            Start = [3, 8]
        };

        var error = LockTextSerializer.Validate(request);

        Assert.Equal("invalid start positions (must be 1-7)", error);
    }
}

using GothicCalculatorApi.Models;
using GothicCalculatorApi.Services;
using Microsoft.Extensions.FileProviders;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.Extensions.Options;

namespace GothicCalculatorApi.Tests;

public class LockSolverServiceTests
{
    [Fact]
    public async Task SolveAsync_ReturnsSolution_ForTowerChest()
    {
        var repoRoot = FindRepoRoot();
        var binaryPath = Path.Combine(repoRoot, "gothic-lock");
        Assert.True(File.Exists(binaryPath), $"Expected binary at {binaryPath}");

        Environment.SetEnvironmentVariable("GOTHIC_LOCK_BIN", binaryPath);

        var service = new LockSolverService(
            Options.Create(new GothicLockOptions()),
            NullLogger<LockSolverService>.Instance,
            new TestHostEnvironment(repoRoot));

        var request = new LockDefinitionRequest
        {
            Name = "Second chest in the tower",
            Rules = ["3r, 6l", "-", "1r, 4l, 6r", "2r, 5r, 6l", "-", "3l"],
            Start = [5, 3, 6, 7, 2, 7]
        };

        var (response, exitCode) = await service.SolveAsync(request);

        Assert.Equal(0, exitCode);
        Assert.True(response.Ok);
        Assert.Equal("Second chest in the tower", response.Name);
        Assert.Equal("solved", response.Status);
        Assert.Equal(18, response.Lines);
        Assert.Equal(52, response.Steps);
        Assert.Equal(18, response.Instructions?.Count);
    }

    private static string FindRepoRoot()
    {
        var dir = new DirectoryInfo(AppContext.BaseDirectory);
        while (dir is not null)
        {
            if (File.Exists(Path.Combine(dir.FullName, "Makefile")))
            {
                return dir.FullName;
            }

            dir = dir.Parent;
        }

        throw new InvalidOperationException("Could not locate repository root");
    }

    private sealed class TestHostEnvironment : IHostEnvironment
    {
        public TestHostEnvironment(string contentRootPath)
        {
            ContentRootPath = contentRootPath;
        }

        public string EnvironmentName { get; set; } = "Test";
        public string ApplicationName { get; set; } = "GothicCalculatorApi.Tests";
        public string ContentRootPath { get; set; }
        public IFileProvider ContentRootFileProvider { get; set; } = null!;
    }
}

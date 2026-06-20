using System.Diagnostics;
using System.Text;
using System.Text.Json;
using GothicCalculatorApi.Models;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Options;

namespace GothicCalculatorApi.Services;

public sealed class GothicLockOptions
{
    public string BinaryPath { get; set; } = "./gothic-lock";
}

public sealed class LockSolverService : ILockSolverService
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true
    };

    private readonly GothicLockOptions _options;
    private readonly ILogger<LockSolverService> _logger;
    private readonly IHostEnvironment _environment;

    public LockSolverService(
        IOptions<GothicLockOptions> options,
        ILogger<LockSolverService> logger,
        IHostEnvironment environment)
    {
        _options = options.Value;
        _logger = logger;
        _environment = environment;
    }

    public (bool Found, string Path) CheckBinary()
    {
        var path = ResolveBinaryPath();
        return (File.Exists(path), path);
    }

    public async Task<(LockSolveResponse Response, int ExitCode)> SolveAsync(
        LockDefinitionRequest request,
        CancellationToken cancellationToken = default)
    {
        var validationError = LockTextSerializer.Validate(request);
        if (validationError is not null)
        {
            return (new LockSolveResponse { Ok = false, Error = validationError }, 1);
        }

        var binaryPath = ResolveBinaryPath();
        if (!File.Exists(binaryPath))
        {
            return (new LockSolveResponse
            {
                Ok = false,
                Error = $"solver binary not found at {binaryPath}"
            }, 1);
        }

        var input = LockTextSerializer.Serialize(request);

        var startInfo = new ProcessStartInfo
        {
            FileName = binaryPath,
            Arguments = "--json -",
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
            StandardInputEncoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false),
            StandardOutputEncoding = new UTF8Encoding(false),
            StandardErrorEncoding = new UTF8Encoding(false)
        };

        using var process = new Process { StartInfo = startInfo };

        if (!process.Start())
        {
            return (new LockSolveResponse { Ok = false, Error = "failed to start solver process" }, 1);
        }

        var stdoutTask = process.StandardOutput.ReadToEndAsync(cancellationToken);
        var stderrTask = process.StandardError.ReadToEndAsync(cancellationToken);

        await process.StandardInput.WriteAsync(input.AsMemory(), cancellationToken);
        await process.StandardInput.FlushAsync(cancellationToken);
        process.StandardInput.Close();

        var stdout = (await stdoutTask).Trim();
        var stderr = (await stderrTask).Trim();
        await process.WaitForExitAsync(cancellationToken);

        if (!string.IsNullOrWhiteSpace(stderr))
        {
            _logger.LogWarning("Solver stderr: {Stderr}", stderr);
        }

        if (string.IsNullOrWhiteSpace(stdout))
        {
            return (new LockSolveResponse
            {
                Ok = false,
                Error = string.IsNullOrWhiteSpace(stderr) ? "empty solver output" : stderr
            }, process.ExitCode);
        }

        try
        {
            var response = JsonSerializer.Deserialize<LockSolveResponse>(stdout, JsonOptions)
                ?? new LockSolveResponse { Ok = false, Error = "invalid solver JSON" };
            return (response, process.ExitCode);
        }
        catch (JsonException ex)
        {
            _logger.LogError(ex, "Failed to parse solver output: {Stdout}", stdout);
            return (new LockSolveResponse { Ok = false, Error = "invalid solver JSON" }, process.ExitCode);
        }
    }

    private string ResolveBinaryPath()
    {
        var configured = Environment.GetEnvironmentVariable("GOTHIC_LOCK_BIN");
        if (!string.IsNullOrWhiteSpace(configured))
        {
            return Path.GetFullPath(configured);
        }

        if (Path.IsPathRooted(_options.BinaryPath))
        {
            return _options.BinaryPath;
        }

        return Path.GetFullPath(Path.Combine(_environment.ContentRootPath, _options.BinaryPath));
    }
}

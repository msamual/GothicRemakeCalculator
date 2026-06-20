using GothicCalculatorApi.Models;

namespace GothicCalculatorApi.Services;

public interface ILockSolverService
{
    Task<(LockSolveResponse Response, int ExitCode)> SolveAsync(
        LockDefinitionRequest request,
        CancellationToken cancellationToken = default);

    (bool Found, string Path) CheckBinary();
}

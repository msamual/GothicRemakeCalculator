using GothicCalculatorApi.Models;
using GothicCalculatorApi.Services;
using Microsoft.AspNetCore.Mvc;

namespace GothicCalculatorApi.Controllers;

[ApiController]
[Route("api/lock")]
public sealed class LockController : ControllerBase
{
    private readonly ILockSolverService _solverService;

    public LockController(ILockSolverService solverService)
    {
        _solverService = solverService;
    }

    [HttpGet("health")]
    public ActionResult<HealthResponse> Health()
    {
        var (found, path) = _solverService.CheckBinary();
        return Ok(new HealthResponse
        {
            Status = found ? "ok" : "degraded",
            BinaryFound = found,
            BinaryPath = path
        });
    }

    [HttpPost("solve")]
    public async Task<ActionResult<LockSolveResponse>> Solve(
        [FromBody] LockDefinitionRequest request,
        CancellationToken cancellationToken)
    {
        var (response, exitCode) = await _solverService.SolveAsync(request, cancellationToken);

        if (!response.Ok)
        {
            return exitCode == 0
                ? Ok(response)
                : BadRequest(response);
        }

        return Ok(response);
    }
}

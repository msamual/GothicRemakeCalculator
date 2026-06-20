using GothicCalculatorApi.Services;

var builder = WebApplication.CreateBuilder(args);

builder.Services.Configure<GothicLockOptions>(
    builder.Configuration.GetSection("GothicLock"));
builder.Services.AddSingleton<ILockSolverService, LockSolverService>();
builder.Services.AddControllers();
builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowFrontend", policy =>
    {
        policy.WithOrigins(
                "http://localhost:4200",
                "https://localhost:4200",
                "http://localhost:8080",
                "http://localhost")
              .AllowAnyHeader()
              .AllowAnyMethod();
    });
});

var app = builder.Build();

app.UseCors("AllowFrontend");
app.MapControllers();

app.Run();

public partial class Program;

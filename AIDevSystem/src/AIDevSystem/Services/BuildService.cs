using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AIDevSystem.Infrastructure;

namespace AIDevSystem.Services;

/// <summary>
/// 自动检测项目类型并编译。给 ReasonX(Dev) 和 Claude(PM) 都提供编译验证能力。
/// </summary>
public class BuildService
{
    private readonly CliRunner _runner;

    public BuildService() => _runner = new CliRunner();

    /// <summary>
    /// 在指定目录中自动检测项目类型并编译。
    /// 返回 (success, output)。
    /// </summary>
    public async Task<(bool success, string output)> BuildAsync(string directory)
    {
        if (!Directory.Exists(directory))
            return (false, $"目录不存在: {directory}");

        var files = Directory.GetFiles(directory, "*", SearchOption.AllDirectories)
                             .Select(f => Path.GetFileName(f))
                             .ToList();

        // Rust
        if (files.Contains("Cargo.toml"))
        {
            var result = await _runner.RunAsync("cargo", "build 2>&1", directory, 120000);
            return (result.Success, result.FullOutput);
        }

        // C/C++ with Makefile
        if (files.Contains("Makefile"))
        {
            var result = await _runner.RunAsync("make", "", directory, 120000);
            return (result.Success, result.FullOutput);
        }

        // C/C++ with CMake
        if (files.Contains("CMakeLists.txt"))
        {
            var buildDir = Path.Combine(directory, "build");
            if (!Directory.Exists(buildDir)) Directory.CreateDirectory(buildDir);
            await _runner.RunAsync("cmake", "..", buildDir, 30000);
            var result = await _runner.RunAsync("cmake", "--build .", buildDir, 120000);
            return (result.Success, result.FullOutput);
        }

        // .NET
        if (files.Any(f => f.EndsWith(".csproj") || f.EndsWith(".sln")))
        {
            var projFile = files.FirstOrDefault(f => f.EndsWith(".csproj"))
                        ?? files.FirstOrDefault(f => f.EndsWith(".sln"));
            var projDir = FindFileDir(directory, projFile!);
            var result = await _runner.RunAsync("dotnet", "build", projDir ?? directory, 120000);
            return (result.Success, result.FullOutput);
        }

        // C/C++ 单文件
        var cppFiles = Directory.GetFiles(directory, "*.cpp").Concat(
                       Directory.GetFiles(directory, "*.c")).ToList();
        if (cppFiles.Count > 0)
        {
            var exeName = Path.Combine(directory,
                Path.GetFileNameWithoutExtension(cppFiles[0]) + ".exe");
            var cmd = $"g++ -o \"{exeName}\" {string.Join(" ", cppFiles.Select(f => $"\"{f}\""))} -lSDL2 -lSDL2main -lSDL2_ttf -mwindows";
            var result = await _runner.RunAsync("bash", $"-c \"{cmd}\"", directory, 120000);
            return (result.Success, result.FullOutput);
        }

        // JS/HTML
        if (files.Contains("index.html"))
            return (true, "Web 项目，无需编译。直接用浏览器打开 index.html。");

        return (true, "未检测到已知项目类型，跳过编译。");
    }

    /// <summary>
    /// 运行已编译的程序（用于 PM 验证）
    /// </summary>
    public async Task<(bool success, string output)> RunAsync(string directory)
    {
        var files = Directory.GetFiles(directory, "*", SearchOption.TopDirectoryOnly)
                             .Select(f => Path.GetFileName(f))
                             .ToList();

        // 找 exe
        var exeFiles = Directory.GetFiles(directory, "*.exe");
        if (exeFiles.Length == 0)
            return (false, "未找到可执行文件。");

        var exe = exeFiles.First(f => !f.EndsWith(".vshost.exe"));
        var result = await _runner.RunAsync(exe, "", directory, 10000);
        return (result.Success || result.ExitCode != -1, result.FullOutput);
    }

    private static string? FindFileDir(string root, string fileName)
    {
        foreach (var dir in Directory.GetDirectories(root, "*", SearchOption.AllDirectories))
            if (File.Exists(Path.Combine(dir, fileName)))
                return dir;
        return null;
    }
}

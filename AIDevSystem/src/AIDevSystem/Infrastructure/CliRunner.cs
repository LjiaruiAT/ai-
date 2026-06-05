using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.Threading.Tasks;

namespace AIDevSystem.Infrastructure;

public class CliRunner
{
    private readonly List<Process> _runningProcesses = new();

    public async Task<CliResult> RunAsync(
        string command,
        string arguments,
        string? workingDir = null,
        int timeoutMs = 300000)
    {
        return await RunInternalAsync(command, arguments, null, workingDir, timeoutMs, null, null);
    }

    /// <summary>
    /// 执行命令，通过 stdin 传递输入文本（用于大段 prompt）
    /// </summary>
    public async Task<CliResult> RunWithStdinAsync(
        string command,
        string arguments,
        string stdinText,
        string? workingDir = null,
        int timeoutMs = 300000)
    {
        return await RunInternalAsync(command, arguments, stdinText, workingDir, timeoutMs, null, null);
    }

    public async Task<CliResult> RunWithProgressAsync(
        string command,
        string arguments,
        Action<string>? onOutputLine,
        Action<string>? onErrorLine,
        string? workingDir = null,
        int timeoutMs = 300000)
    {
        return await RunInternalAsync(command, arguments, null, workingDir, timeoutMs, onOutputLine, onErrorLine);
    }

    /// <summary>
    /// 执行命令，通过 stdin 传递输入文本 + 实时回调
    /// </summary>
    public async Task<CliResult> RunWithStdinAndProgressAsync(
        string command,
        string arguments,
        string stdinText,
        Action<string>? onOutputLine,
        Action<string>? onErrorLine,
        string? workingDir = null,
        int timeoutMs = 300000)
    {
        return await RunInternalAsync(command, arguments, stdinText, workingDir, timeoutMs, onOutputLine, onErrorLine);
    }

    private async Task<CliResult> RunInternalAsync(
        string command,
        string arguments,
        string? stdinText,
        string? workingDir,
        int timeoutMs,
        Action<string>? onOutputLine,
        Action<string>? onErrorLine)
    {
        var sw = Stopwatch.StartNew();
        var stdout = new StringBuilder();
        var stderr = new StringBuilder();

        var psi = new ProcessStartInfo
        {
            FileName = command,
            Arguments = arguments,
            WorkingDirectory = string.IsNullOrEmpty(workingDir) ? Environment.CurrentDirectory : workingDir,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            RedirectStandardInput = stdinText != null,
            CreateNoWindow = true,
            StandardOutputEncoding = Encoding.UTF8,
            StandardErrorEncoding = Encoding.UTF8
        };

        using var process = new Process { StartInfo = psi };

        process.OutputDataReceived += (_, e) =>
        {
            if (e.Data != null)
            {
                stdout.AppendLine(e.Data);
                onOutputLine?.Invoke(e.Data);
            }
        };
        process.ErrorDataReceived += (_, e) =>
        {
            if (e.Data != null)
            {
                stderr.AppendLine(e.Data);
                onErrorLine?.Invoke(e.Data);
            }
        };

        lock (_runningProcesses) { _runningProcesses.Add(process); }

        try
        {
            process.Start();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();

            // 写入 stdin 并关闭，让子进程可以开始读取
            if (stdinText != null)
            {
                process.StandardInput.Write(stdinText);
                process.StandardInput.Close();
            }

            var completed = await Task.Run(() => process.WaitForExit(timeoutMs));

            if (!completed)
            {
                process.Kill(entireProcessTree: true);
                return new CliResult
                {
                    ExitCode = -1,
                    Stdout = stdout.ToString(),
                    Stderr = stderr.ToString(),
                    DurationMs = sw.ElapsedMilliseconds,
                    TimedOut = true
                };
            }

            await Task.Delay(200);

            return new CliResult
            {
                ExitCode = process.ExitCode,
                Stdout = stdout.ToString(),
                Stderr = stderr.ToString(),
                DurationMs = sw.ElapsedMilliseconds,
                TimedOut = false
            };
        }
        finally
        {
            lock (_runningProcesses) { _runningProcesses.Remove(process); }
        }
    }

    public void KillAll()
    {
        lock (_runningProcesses)
        {
            foreach (var p in _runningProcesses)
            {
                try { p.Kill(entireProcessTree: true); } catch { }
            }
            _runningProcesses.Clear();
        }
    }
}

using System;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace AIDevSystem.Infrastructure;

/// <summary>
/// Dev 适配器 — 给 ReasonX 完整的 Shell 权限，多轮对话循环执行命令。
/// ReasonX 输出 shell 命令 → C# 执行 → 输出返回给 ReasonX → 继续直到完成。
/// </summary>
public class ReasonXAdapter
{
    private readonly DeepSeekApiClient _api;
    private readonly CliRunner _cli;
    private const int MaxTurns = 15;

    public ReasonXAdapter(DeepSeekApiClient api)
    {
        _api = api;
        _cli = new CliRunner();
    }

    public async Task<CliResult> ExecuteTaskAsync(
        string taskDescription, Action<string>? onProgress = null)
    {
        var result = new CliResult();
        var sw = System.Diagnostics.Stopwatch.StartNew();
        var log = new StringBuilder();

        try
        {
            var systemPrompt = $"""
                你是公司开发员工，拥有完整的 Shell 终端权限。
                用户: {Environment.UserName}
                桌面: C:\Users\{Environment.UserName}\Desktop
                当前目录: C:\Users\{Environment.UserName}

                你有 Bash/PowerShell 命令权限。直接做事，不要问问题。
                每个操作输出一个代码块：
                ```bash
                g++ -o game.exe main.cpp -lSDL2 -lSDL2main -lSDL2_ttf -mwindows
                ```
                或输出文件：
                ```C:\Users\{Environment.UserName}\Desktop\project\main.cpp
                代码内容
                ```
                当你确认任务完成，输出 DONE。
                如果某个命令失败，仔细看错误输出，修正后重试。
                """;

            var history = new System.Collections.Generic.List<(string role, string text)>();
            var userMsg = taskDescription;
            bool done = false;

            for (int turn = 0; turn < MaxTurns && !done; turn++)
            {
                onProgress?.Invoke($"🔄 轮次 {turn + 1}/{MaxTurns}...");

                history.Add(("user", userMsg));
                var response = await _api.ChatWithHistoryAsync(systemPrompt, history.ToArray());
                history.Add(("assistant", response));

                userMsg = ""; // 下一轮从 DeepSeek 的回复开始

                // 解析并执行响应中的命令/文件
                bool didSomething = false;
                var output = new StringBuilder();

                // 1. 写文件
                var fileRegex = new Regex(
                    @"```([A-Za-z]:[^\r\n]*?\.[a-zA-Z0-9]+)\r?\n(.*?)```",
                    RegexOptions.Singleline);
                foreach (Match m in fileRegex.Matches(response))
                {
                    var path = m.Groups[1].Value.Trim();
                    var content = m.Groups[2].Value.Trim();
                    if (path.Length < 3 || !char.IsLetter(path[0]) || path[1] != ':') continue;

                    try
                    {
                        var dir = System.IO.Path.GetDirectoryName(path);
                        if (!string.IsNullOrEmpty(dir) && !System.IO.Directory.Exists(dir))
                            System.IO.Directory.CreateDirectory(dir);
                        System.IO.File.WriteAllText(path, content, System.Text.Encoding.UTF8);
                        output.AppendLine($"✅ 已写入: {path}");
                        didSomething = true;
                    }
                    catch (Exception ex) { output.AppendLine($"❌ 写入失败: {ex.Message}"); }
                }

                // 2. 执行 Shell 命令
                var cmdRegex = new Regex(
                    @"```(?:bash|cmd|sh|shell|powershell)\r?\n(.*?)```",
                    RegexOptions.Singleline);
                foreach (Match m in cmdRegex.Matches(response))
                {
                    var cmd = m.Groups[1].Value.Trim();
                    if (cmd.StartsWith("cd ")) continue; // bash 环境跨调用不保留 cwd

                    onProgress?.Invoke($"⚡ {cmd[..Math.Min(80, cmd.Length)]}...");
                    var cmdResult = await _cli.RunAsync("bash", $"-c \"{cmd.Replace("\"", "\\\"")}\"",
                        timeoutMs: 120000);
                    output.AppendLine(cmdResult.Success ? $"✅ {cmd}" : $"❌ {cmd}");
                    if (!string.IsNullOrEmpty(cmdResult.Stdout)) output.AppendLine(cmdResult.Stdout);
                    if (!string.IsNullOrEmpty(cmdResult.Stderr)) output.AppendLine(cmdResult.Stderr);
                    didSomething = true;
                }

                // 3. 把执行结果喂给 AI
                var feedback = output.ToString();
                log.AppendLine(feedback);

                if (response.Contains("DONE"))
                {
                    done = true;
                    break;
                }

                if (didSomething)
                {
                    userMsg = $"命令执行结果：\n{feedback}\n\n请继续。";
                }
                else
                {
                    // AI 没输出命令也没输出文件，问它
                    userMsg = "你没有输出任何可执行的命令或文件。请用 ```bash 或 ```C:\\path\\file 格式输出你的操作。";
                }
            }

            result.Stdout = log.ToString();
            result.ExitCode = done ? 0 : 1;
            onProgress?.Invoke(result.Stdout);
        }
        catch (Exception ex)
        {
            result.ExitCode = -1;
            result.Stderr = ex.Message;
        }

        sw.Stop(); result.DurationMs = sw.ElapsedMilliseconds;
        return result;
    }
}

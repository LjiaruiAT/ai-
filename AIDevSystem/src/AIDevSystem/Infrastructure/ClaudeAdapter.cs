using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using AIDevSystem.Models;
using Newtonsoft.Json;

namespace AIDevSystem.Infrastructure;

public record ChatHistoryEntry(string Role, string Content);

public class ReviewResult
{
    public string? Summary { get; set; }
    public double Score { get; set; }
    public List<string> Issues { get; set; } = new();
}

/// <summary>
/// PM 适配器 — DeepSeek API + 完整 Shell 权限。
/// </summary>
public class ClaudeAdapter
{
    private readonly DeepSeekApiClient _api;
    private readonly CliRunner _cli;

    public ClaudeAdapter(DeepSeekApiClient api)
    {
        _api = api;
        _cli = new CliRunner();
    }

    // ===== 任务拆解 =====

    public async Task<List<TaskItem>> DecomposeTaskAsync(string requirement, string projectContext)
    {
        var systemPrompt = "你是一个项目经理。将用户需求拆解为子任务。只返回 JSON。";
        var userMessage = $"需求：{requirement}\n项目背景：{projectContext}\n\n返回 JSON 数组：[{{\"Title\":\"任务标题\",\"Description\":\"详细描述\",\"Difficulty\":\"low|medium|high\",\"ModelSelected\":\"reasonix|claude\"}}]";

        var json = await _api.ChatAsync(systemPrompt, userMessage);
        return ParseJson<List<TaskItem>>(json) ?? new List<TaskItem>();
    }

    // ===== 代码审查 — 带编译验证 =====

    public async Task<ReviewResult> ReviewCodeAsync(string codePath, string taskDescription)
    {
        // 先自动编译
        var buildInfo = "";
        if (!string.IsNullOrEmpty(codePath))
        {
            var cmdResult = await _cli.RunAsync("bash", $"-c \"cd '{codePath}' 2>/dev/null && (cargo build 2>&1 || make 2>&1 || dotnet build 2>&1 || (ls *.cpp *.c 2>/dev/null && g++ -o /tmp/test *.cpp *.c 2>&1) || echo 'no build system detected')\"", timeoutMs: 60000);
            if (!string.IsNullOrEmpty(cmdResult.FullOutput))
                buildInfo = $"\n编译结果:\n{cmdResult.FullOutput}";
        }

        var systemPrompt = "你是资深代码审查专家。返回 JSON 格式审查结果。Score: 0-3严重缺陷 4-6需返工 7-8良好 9-10优秀。如果编译不通过，Score 最多给 5。";
        var userMessage = $"任务：{taskDescription}\n路径：{codePath}{buildInfo}\n\n返回 JSON：{{\"Summary\":\"摘要\",\"Score\":8,\"Issues\":[\"问题1\"]}}";

        var json = await _api.ChatAsync(systemPrompt, userMessage);
        return ParseJson<ReviewResult>(json) ?? new ReviewResult { Score = 5, Summary = "审查解析失败" };
    }

    // ===== PM 对话 =====

    private static string MapRole(string dbRole) => dbRole switch { "pm" => "assistant", _ => dbRole };
    private const int MaxHistoryMessages = 20;

    public async Task<string> ChatAsync(string userMessage, List<ChatHistoryEntry> history)
    {
        var systemPrompt = "你是专业的软件项目经理。帮用户澄清需求，直到需求足够清晰。用简洁中文回复。";

        var recentHistory = history.Skip(history.Count > MaxHistoryMessages ? history.Count - MaxHistoryMessages : 0).ToList();
        var apiHistory = recentHistory.Select(h => (MapRole(h.Role), h.Content)).ToList();
        apiHistory.Add(("user", userMessage));

        return await _api.ChatWithHistoryAsync(systemPrompt, apiHistory.ToArray());
    }

    public async Task<string> SynthesizeSpecAsync(List<ChatHistoryEntry> history)
    {
        var systemPrompt = "将对话历史综合为最终技术规格说明书。包含：项目目标、功能需求、技术约束、验收标准。用中文。";

        var recentHistory = history.Skip(history.Count > MaxHistoryMessages ? history.Count - MaxHistoryMessages : 0).ToList();
        var apiHistory = recentHistory.Select(h => (MapRole(h.Role), h.Content)).ToList();
        apiHistory.Add(("user", "请综合以上对话，输出最终技术规格说明书。"));

        return await _api.ChatWithHistoryAsync(systemPrompt, apiHistory.ToArray());
    }

    // ===== JSON =====

    private static T ParseJson<T>(string raw) where T : class
    {
        var cleaned = Regex.Replace(raw, @"```\w*\n?|```", "").Trim();

        int start = cleaned.IndexOf('[');
        int end = cleaned.LastIndexOf(']');
        if (start >= 0 && end > start) cleaned = cleaned[start..(end + 1)];
        else
        {
            start = cleaned.IndexOf('{'); end = cleaned.LastIndexOf('}');
            if (start >= 0 && end > start)
            {
                var obj = cleaned[start..(end + 1)];
                if (typeof(T).IsGenericType && typeof(T).GetGenericTypeDefinition() == typeof(List<>))
                    cleaned = $"[{obj}]";
                else cleaned = obj;
            }
        }

        return JsonConvert.DeserializeObject<T>(cleaned)
            ?? throw new Exception($"JSON 解析失败: {cleaned[..Math.Min(200, cleaned.Length)]}");
    }
}

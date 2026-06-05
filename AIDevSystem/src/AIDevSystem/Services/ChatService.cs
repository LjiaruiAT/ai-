using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using AIDevSystem.Infrastructure;
using AIDevSystem.Models;

namespace AIDevSystem.Services;

public class ChatService
{
    private readonly ClaudeAdapter _claude;
    private readonly TaskManager _taskManager;

    public ChatService(ClaudeAdapter claude, TaskManager taskManager)
    {
        _claude = claude;
        _taskManager = taskManager;
    }

    public async Task<ChatMessage> SendMessageAsync(int projectId, string userMessage, List<ChatMessage> history)
    {
        // 保存用户消息
        var userMsg = new ChatMessage { ProjectId = projectId, Role = "user", Content = userMessage };
        _taskManager.SaveChatMessage(userMsg);

        // 构建历史
        var entries = history.Select(m => new ChatHistoryEntry(m.Role, m.Content)).ToList();

        // 获取 PM 回复
        var pmResponse = await _claude.ChatAsync(userMessage, entries);

        // 保存 PM 回复
        var pmMsg = new ChatMessage { ProjectId = projectId, Role = "pm", Content = pmResponse };
        _taskManager.SaveChatMessage(pmMsg);

        return pmMsg;
    }

    public async Task<string> SynthesizeFinalSpecAsync(int projectId, List<ChatMessage> history)
    {
        var entries = history.Select(m => new ChatHistoryEntry(m.Role, m.Content)).ToList();
        var spec = await _claude.SynthesizeSpecAsync(entries);

        // 保存为系统消息
        var sysMsg = new ChatMessage { ProjectId = projectId, Role = "system", Content = "✅ 最终规格已确认：\n\n" + spec };
        _taskManager.SaveChatMessage(sysMsg);

        return spec;
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using AIDevSystem.Infrastructure;
using AIDevSystem.Models;

namespace AIDevSystem.Services;

public class TaskDecomposer
{
    private readonly ClaudeAdapter _claude;
    private readonly Database _db;
    private readonly TaskManager _taskManager;

    public TaskDecomposer(ClaudeAdapter claude, Database db, TaskManager taskManager)
    {
        _claude = claude;
        _db = db;
        _taskManager = taskManager;
    }

    /// <summary>
    /// 将用户需求拆解为子任务列表，保存到数据库
    /// </summary>
    public async Task<List<TaskItem>> DecomposeAsync(int projectId, string requirement)
    {
        var project = _taskManager.GetProject(projectId);
        var context = project?.Description ?? "无";
        var tasks = await _claude.DecomposeTaskAsync(requirement, context);

        foreach (var task in tasks)
        {
            task.ProjectId = projectId;
            task.Status = "pending";
            _taskManager.CreateTask(task);
        }

        return tasks;
    }

    /// <summary>
    /// 对单个任务进一步细化拆解
    /// </summary>
    public async Task<List<TaskItem>> DecomposeSubAsync(int parentTaskId)
    {
        var parent = _taskManager.GetTask(parentTaskId);
        if (parent == null) return new List<TaskItem>();

        var prompt = $"请将以下任务进一步拆解为更小的子任务：\n标题：{parent.Title}\n描述：{parent.Description}";
        var tasks = await _claude.DecomposeTaskAsync(prompt, "");

        foreach (var task in tasks)
        {
            task.ProjectId = parent.ProjectId;
            task.ParentTaskId = parentTaskId;
            task.Status = "pending";
            _taskManager.CreateTask(task);
        }

        return tasks;
    }

    /// <summary>
    /// 审查任务执行结果
    /// </summary>
    public async Task<ReviewResult> ReviewAsync(string codePath, string taskDescription)
    {
        return await _claude.ReviewCodeAsync(codePath, taskDescription);
    }
}

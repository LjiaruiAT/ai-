using System;
using System.Diagnostics;
using System.Threading.Tasks;
using AIDevSystem.Infrastructure;
using AIDevSystem.Models;

namespace AIDevSystem.Services;

public class TaskExecutor
{
    private readonly ReasonXAdapter _reasonix;
    private readonly Database _db;
    private readonly TaskManager _taskManager;
    private readonly MemoryService _memory;

    public TaskExecutor(ReasonXAdapter reasonix, Database db, TaskManager taskManager, MemoryService memory)
    {
        _reasonix = reasonix;
        _db = db;
        _taskManager = taskManager;
        _memory = memory;
    }

    public async Task<TaskExecution> ExecuteTaskAsync(
        TaskItem task,
        Action<string>? onProgress = null,
        ProgressTracker? progressTracker = null)
    {
        var sw = Stopwatch.StartNew();
        progressTracker?.StartTask(task.Title);

        _taskManager.UpdateTaskStatus(task.Id, "running");

        // 不再绑死项目路径 — reasonix 根据任务描述自己决定在哪干活
        var prompt = $"你是公司的开发员工。直接完成任务不要询问。\n任务: {task.Title}\n{task.Description}";

        var execution = new TaskExecution
        {
            TaskId = task.Id,
            Model = task.ModelSelected ?? "reasonix",
            Role = "executor",
            Prompt = prompt
        };

        try
        {
            var result = await _reasonix.ExecuteTaskAsync(prompt, onProgress);

            execution.DurationMs = sw.ElapsedMilliseconds;
            execution.Output = result.Stdout;
            execution.Success = result.Success ? 1 : 0;

            if (!result.Success)
            {
                execution.ErrorMsg = result.Stderr;
                _taskManager.UpdateTaskStatus(task.Id, "failed");
                progressTracker?.FailTask();
            }
            else
            {
                _taskManager.UpdateTaskStatus(task.Id, "completed");
                progressTracker?.CompleteTask();
                await _memory.RecordExecutionAsync(execution, task);
            }
        }
        catch (Exception ex)
        {
            execution.DurationMs = sw.ElapsedMilliseconds;
            execution.Success = 0;
            execution.ErrorMsg = ex.Message;
            _taskManager.UpdateTaskStatus(task.Id, "failed");
            progressTracker?.FailTask();
        }

        _taskManager.SaveExecution(execution);
        return execution;
    }

    public async Task<int> ExecuteProjectTasksAsync(
        int projectId,
        Action<string>? onProgress = null,
        ProgressTracker? progressTracker = null)
    {
        var tasks = _taskManager.GetTasksForProject(projectId);
        var pending = tasks.FindAll(t => t.Status == "pending");
        progressTracker?.StartBatch(pending.Count);

        var executed = 0;
        foreach (var task in pending)
        {
            onProgress?.Invoke($"执行: {task.Title}");
            await ExecuteTaskAsync(task, onProgress, progressTracker);
            executed++;
        }

        onProgress?.Invoke($"完成: {executed} 个任务");
        return executed;
    }
}

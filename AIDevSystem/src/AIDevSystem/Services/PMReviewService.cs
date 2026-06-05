using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using AIDevSystem.Infrastructure;
using AIDevSystem.Models;

namespace AIDevSystem.Services;

public class PMReviewService
{
    private readonly ClaudeAdapter _claude;
    private readonly TaskExecutor _executor;
    private readonly TaskManager _taskManager;
    private readonly ScoreService _scoreService;

    public double QualityThreshold { get; set; } = 6.0;
    public int MaxRetries { get; set; } = 3;

    public PMReviewService(ClaudeAdapter claude, TaskExecutor executor,
        TaskManager taskManager, ScoreService scoreService)
    {
        _claude = claude;
        _executor = executor;
        _taskManager = taskManager;
        _scoreService = scoreService;
    }

    public async Task<TaskExecution> ExecuteWithReviewAsync(
        TaskItem task, ProgressTracker? progress = null)
    {
        int attempt = 0;
        TaskExecution? lastExecution = null;

        while (attempt < MaxRetries)
        {
            attempt++;
            progress?.StartTask($"#{attempt}: {task.Title}");

            var execution = await _executor.ExecuteTaskAsync(task, null, progress);
            execution.AttemptNumber = attempt;
            lastExecution = execution;

            if (execution.Success != 1)
            {
                task.Status = "failed";
                task.ReviewStatus = "failed_execution";
                progress?.FailTask();
                break;
            }

            // Claude 审查（内部自带编译验证 + 代码审查）
            var codePath = task.Description ?? "";
            var review = await _claude.ReviewCodeAsync(codePath, task.Description ?? task.Title);
            execution.ReviewScore = review.Score;
            execution.ReviewNotes = string.Join("\n", review.Issues);

            if (review.Score >= QualityThreshold)
            {
                task.Status = "completed";
                task.ReviewStatus = "passed_review";
                task.ReworkCount = attempt - 1;
                progress?.CompleteTask();
                break;
            }

            task.ReviewStatus = "in_rework";
            task.ReworkCount = attempt;
            progress?.ReportTaskProgress(0);

            if (attempt >= MaxRetries)
            {
                task.Status = "failed";
                task.ReviewStatus = "max_retries_exceeded";
                progress?.FailTask();
                break;
            }

            task.Description = AppendReviewFeedback(task.Description ?? task.Title, review, attempt);
            _taskManager.UpdateTask(task);
        }

        if (lastExecution != null)
        {
            _taskManager.SaveExecution(lastExecution);
            _scoreService.CalculateScore(lastExecution, task);
            _taskManager.UpdateTask(task);
        }

        return lastExecution ?? new TaskExecution { Success = 0, ErrorMsg = "No execution attempted" };
    }

    private static string AppendReviewFeedback(string original, ReviewResult review, int attempt)
    {
        var issues = string.Join("\n", review.Issues!);
        return $"{original}\n\n=== PM 审查 #{attempt} ===\n评分: {review.Score}/10 (阈值: 6.0)\n需修复:\n{issues}\n\n请修复以上全部问题。";
    }
}

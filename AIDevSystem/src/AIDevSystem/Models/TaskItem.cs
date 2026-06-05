namespace AIDevSystem.Models;

public class TaskItem
{
    public int Id { get; set; }
    public int ProjectId { get; set; }
    public int? ParentTaskId { get; set; }
    public string Title { get; set; } = "";
    public string? Description { get; set; }
    public string? Difficulty { get; set; }
    public string? ModelSelected { get; set; }
    public string Status { get; set; } = "pending";
    public string? PromptFile { get; set; }
    public string? ResultFile { get; set; }
    public string? CreatedAt { get; set; }
    public string? CompletedAt { get; set; }

    // Phase 4: PM 审查循环
    public int ReworkCount { get; set; }
    public string ReviewStatus { get; set; } = "pending_review";
}

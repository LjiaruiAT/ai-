namespace AIDevSystem.Models;

public class TaskExecution
{
    public int Id { get; set; }
    public int TaskId { get; set; }
    public string Model { get; set; } = "";
    public string Role { get; set; } = "";
    public string? Prompt { get; set; }
    public string? Output { get; set; }
    public int? CostTokens { get; set; }
    public long? DurationMs { get; set; }
    public int Success { get; set; }
    public string? ErrorMsg { get; set; }
    public string? ExecutedAt { get; set; }

    // Phase 4: PM 审查循环
    public int AttemptNumber { get; set; } = 1;
    public double? ReviewScore { get; set; }
    public string? ReviewNotes { get; set; }
}

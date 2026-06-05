namespace AIDevSystem.Models;

public class ModelScore
{
    public int Id { get; set; }
    public int TaskExecutionId { get; set; }
    public string Model { get; set; } = "";
    public string? Difficulty { get; set; }
    public double? ScoreCost { get; set; }
    public double? ScoreSpeed { get; set; }
    public double? ScorePassRate { get; set; }
    public double? ScoreRework { get; set; }
    public double? ScoreQuality { get; set; }
    public double? CompositeScore { get; set; }
    public string? CreatedAt { get; set; }
}

/// <summary>
/// P2: 模型评分摘要 — 用于统计查询
/// </summary>
public class ModelScoreSummary
{
    public string Model { get; set; } = "";
    public string? Difficulty { get; set; }
    public int TotalTasks { get; set; }
    public double AvgCost { get; set; }
    public double AvgSpeed { get; set; }
    public double AvgPassRate { get; set; }
    public double AvgRework { get; set; }
    public double AvgQuality { get; set; }
    public double AvgComposite { get; set; }
}

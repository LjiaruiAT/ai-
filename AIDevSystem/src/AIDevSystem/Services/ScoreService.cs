using System;
using System.Collections.Generic;
using AIDevSystem.Infrastructure;
using AIDevSystem.Models;

namespace AIDevSystem.Services;

public class ScoreService
{
    private readonly Database _db;

    public ScoreService(Database db) => _db = db;

    /// <summary>
    /// 计算并保存一次执行的模型评分
    /// </summary>
    public ModelScore CalculateScore(TaskExecution execution, TaskItem task)
    {
        var score = new ModelScore
        {
            TaskExecutionId = execution.Id,
            Model = execution.Model,
            Difficulty = task.Difficulty
        };

        // 成本评分：tokens 越少越好（0-10）
        score.ScoreCost = execution.CostTokens switch
        {
            null => 5,
            < 500 => 9,
            < 1000 => 7,
            < 3000 => 5,
            < 10000 => 3,
            _ => 1
        };

        // 速度评分：越快越好（0-10）
        score.ScoreSpeed = execution.DurationMs switch
        {
            null => 5,
            < 5000 => 9,
            < 15000 => 7,
            < 60000 => 5,
            < 120000 => 3,
            _ => 1
        };

        // 通过率：成功=10，失败=0
        score.ScorePassRate = execution.Success == 1 ? 10 : 0;

        // 返工评分（这里简化，实际应根据代码审查结果）
        score.ScoreRework = 7;

        // 质量评分复合
        score.ScoreQuality = (score.ScorePassRate * 0.4 + score.ScoreRework * 0.6);

        // 综合评分
        score.CompositeScore = Math.Round(
            (score.ScoreCost ?? 5) * 0.2 +
            (score.ScoreSpeed ?? 5) * 0.2 +
            (score.ScorePassRate ?? 5) * 0.35 +
            (score.ScoreQuality ?? 5) * 0.25, 2);

        // 持久化
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"INSERT INTO model_scores (task_execution_id, model, difficulty, score_cost, score_speed, score_pass_rate, score_rework, score_quality, composite_score) 
                            VALUES (@teid, @model, @diff, @sc, @ss, @sp, @sr, @sq, @cs)";
        cmd.Parameters.AddWithValue("@teid", score.TaskExecutionId);
        cmd.Parameters.AddWithValue("@model", score.Model);
        cmd.Parameters.AddWithValue("@diff", (object?)score.Difficulty ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@sc", (object?)score.ScoreCost ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@ss", (object?)score.ScoreSpeed ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@sp", (object?)score.ScorePassRate ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@sr", (object?)score.ScoreRework ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@sq", (object?)score.ScoreQuality ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@cs", (object?)score.CompositeScore ?? DBNull.Value);
        cmd.ExecuteNonQuery();

        return score;
    }

    /// <summary>
    /// 获取所有模型评分统计
    /// </summary>
    public List<ModelScoreSummary> GetModelScoreSummaries()
    {
        var list = new List<ModelScoreSummary>();
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"SELECT 
            model,
            difficulty,
            COUNT(*) as total,
            AVG(COALESCE(score_cost, 0)),
            AVG(COALESCE(score_speed, 0)),
            AVG(COALESCE(score_pass_rate, 0)),
            AVG(COALESCE(score_rework, 0)),
            AVG(COALESCE(score_quality, 0)),
            AVG(COALESCE(composite_score, 0))
        FROM model_scores
        GROUP BY model, difficulty
        ORDER BY AVG(COALESCE(composite_score, 0)) DESC";
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
        {
            list.Add(new ModelScoreSummary
            {
                Model = reader.GetString(0),
                Difficulty = reader.IsDBNull(1) ? null : reader.GetString(1),
                TotalTasks = reader.GetInt32(2),
                AvgCost = reader.GetDouble(3),
                AvgSpeed = reader.GetDouble(4),
                AvgPassRate = reader.GetDouble(5),
                AvgRework = reader.GetDouble(6),
                AvgQuality = reader.GetDouble(7),
                AvgComposite = reader.GetDouble(8)
            });
        }
        return list;
    }
}

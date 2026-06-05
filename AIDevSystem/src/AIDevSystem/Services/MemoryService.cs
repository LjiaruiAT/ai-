using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using AIDevSystem.Infrastructure;
using AIDevSystem.Models;
using Microsoft.Data.Sqlite;

namespace AIDevSystem.Services;

/// <summary>
/// B1-B3: AI 员工记忆系统
/// 记录每次任务执行的表现，用于后续模型选择和评分优化
/// </summary>
public class MemoryService
{
    private readonly Database _db;

    public MemoryService(Database db) => _db = db;

    /// <summary>
    /// 记录一次任务执行到记忆库
    /// </summary>
    public async Task RecordExecutionAsync(TaskExecution execution, TaskItem task)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"INSERT INTO agent_memories (agent, role, task_execution_id, file_path, good_points, mistakes, composite_score) 
                            VALUES (@agent, @role, @teid, @fp, @good, @mistakes, @score)";
        cmd.Parameters.AddWithValue("@agent", execution.Model);
        cmd.Parameters.AddWithValue("@role", "executor");
        cmd.Parameters.AddWithValue("@teid", execution.Id);
        cmd.Parameters.AddWithValue("@fp", (object?)task.ResultFile ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@good", execution.Success == 1 ? "任务执行成功" : DBNull.Value);
        cmd.Parameters.AddWithValue("@mistakes", execution.Success == 0 ? execution.ErrorMsg : DBNull.Value);
        cmd.Parameters.AddWithValue("@score", execution.Success == 1 ? 8.0 : 3.0);
        cmd.ExecuteNonQuery();
    }

    /// <summary>
    /// 获取某模型的历史表现摘要
    /// </summary>
    public List<ModelScoreSummary> GetModelPerformanceSummaries()
    {
        var list = new List<ModelScoreSummary>();
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"SELECT 
            m.model,
            t.difficulty,
            COUNT(*) as cnt,
            AVG(CASE WHEN m.composite_score IS NOT NULL THEN m.composite_score ELSE 0 END) as avg_score
        FROM agent_memories m
        JOIN task_executions te ON m.task_execution_id = te.id
        JOIN tasks t ON te.task_id = t.id
        GROUP BY m.model, t.difficulty
        ORDER BY m.model, t.difficulty";
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
        {
            list.Add(new ModelScoreSummary
            {
                Model = reader.GetString(0),
                Difficulty = reader.IsDBNull(1) ? null : reader.GetString(1),
                TotalTasks = reader.GetInt32(2),
                AvgComposite = reader.GetDouble(3)
            });
        }
        return list;
    }
}

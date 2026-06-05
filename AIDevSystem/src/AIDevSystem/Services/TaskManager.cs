using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using AIDevSystem.Infrastructure;
using AIDevSystem.Models;
using Microsoft.Data.Sqlite;

namespace AIDevSystem.Services;

public class TaskManager
{
    private readonly Database _db;

    public TaskManager(Database db) => _db = db;

    // ========== 项目 CRUD ==========

    public List<Project> GetAllProjects()
    {
        var list = new List<Project>();
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "SELECT id, name, description, root_path, created_at, updated_at FROM projects ORDER BY updated_at DESC";
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
        {
            list.Add(new Project
            {
                Id = reader.GetInt32(0),
                Name = reader.GetString(1),
                Description = reader.IsDBNull(2) ? null : reader.GetString(2),
                RootPath = reader.IsDBNull(3) ? null : reader.GetString(3),
                CreatedAt = reader.IsDBNull(4) ? null : reader.GetString(4),
                UpdatedAt = reader.IsDBNull(5) ? null : reader.GetString(5)
            });
        }
        return list;
    }

    public Project? GetProject(int id)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "SELECT id, name, description, root_path, created_at, updated_at FROM projects WHERE id=@id";
        cmd.Parameters.AddWithValue("@id", id);
        using var reader = cmd.ExecuteReader();
        if (!reader.Read()) return null;
        return new Project
        {
            Id = reader.GetInt32(0),
            Name = reader.GetString(1),
            Description = reader.IsDBNull(2) ? null : reader.GetString(2),
            RootPath = reader.IsDBNull(3) ? null : reader.GetString(3),
            CreatedAt = reader.IsDBNull(4) ? null : reader.GetString(4),
            UpdatedAt = reader.IsDBNull(5) ? null : reader.GetString(5)
        };
    }

    public Project CreateProject(string name, string? description, string? rootPath)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"INSERT INTO projects (name, description, root_path) VALUES (@name, @desc, @path);
                            SELECT last_insert_rowid();";
        cmd.Parameters.AddWithValue("@name", name);
        cmd.Parameters.AddWithValue("@desc", (object?)description ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@path", (object?)rootPath ?? DBNull.Value);
        var id = (long)cmd.ExecuteScalar()!;
        return new Project { Id = (int)id, Name = name, Description = description, RootPath = rootPath };
    }

    public void UpdateProject(int id, string name, string? description, string? rootPath)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"UPDATE projects SET name=@name, description=@desc, root_path=@path, updated_at=datetime('now') WHERE id=@id";
        cmd.Parameters.AddWithValue("@name", name);
        cmd.Parameters.AddWithValue("@desc", (object?)description ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@path", (object?)rootPath ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@id", id);
        cmd.ExecuteNonQuery();
    }

    public void DeleteProject(int id)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "DELETE FROM tasks WHERE project_id=@id; DELETE FROM projects WHERE id=@id";
        cmd.Parameters.AddWithValue("@id", id);
        cmd.ExecuteNonQuery();
    }

    // ========== 任务 CRUD ==========

    public List<TaskItem> GetTasksForProject(int projectId)
    {
        var list = new List<TaskItem>();
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"SELECT id, project_id, parent_task_id, title, description, difficulty, model_selected, status, prompt_file, result_file, created_at, completed_at, COALESCE(rework_count,0), COALESCE(review_status,'pending_review')
                            FROM tasks WHERE project_id=@pid ORDER BY id";
        cmd.Parameters.AddWithValue("@pid", projectId);
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
        {
            list.Add(ReadTaskItem(reader));
        }
        return list;
    }

    public TaskItem? GetTask(int id)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "SELECT id, project_id, parent_task_id, title, description, difficulty, model_selected, status, prompt_file, result_file, created_at, completed_at, COALESCE(rework_count,0), COALESCE(review_status,'pending_review') FROM tasks WHERE id=@id";
        cmd.Parameters.AddWithValue("@id", id);
        using var reader = cmd.ExecuteReader();
        return reader.Read() ? ReadTaskItem(reader) : null;
    }

    public TaskItem CreateTask(TaskItem task)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"INSERT INTO tasks (project_id, parent_task_id, title, description, difficulty, model_selected, status, prompt_file, result_file) 
                            VALUES (@pid, @ptid, @title, @desc, @diff, @model, @status, @pf, @rf);
                            SELECT last_insert_rowid();";
        cmd.Parameters.AddWithValue("@pid", task.ProjectId);
        cmd.Parameters.AddWithValue("@ptid", (object?)task.ParentTaskId ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@title", task.Title);
        cmd.Parameters.AddWithValue("@desc", (object?)task.Description ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@diff", (object?)task.Difficulty ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@model", (object?)task.ModelSelected ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@status", task.Status);
        cmd.Parameters.AddWithValue("@pf", (object?)task.PromptFile ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@rf", (object?)task.ResultFile ?? DBNull.Value);
        var id = (long)cmd.ExecuteScalar()!;
        task.Id = (int)id;
        return task;
    }

    public void UpdateTaskStatus(int taskId, string status, string? resultFile = null)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"UPDATE tasks SET status=@status, result_file=COALESCE(@rf, result_file), 
                            completed_at=CASE WHEN @status IN ('completed','failed') THEN datetime('now') ELSE completed_at END 
                            WHERE id=@id";
        cmd.Parameters.AddWithValue("@status", status);
        cmd.Parameters.AddWithValue("@rf", (object?)resultFile ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@id", taskId);
        cmd.ExecuteNonQuery();
    }

    public void UpdateTask(TaskItem task)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"UPDATE tasks SET title=@title, description=@desc, difficulty=@diff, model_selected=@model, status=@status, rework_count=@rework, review_status=@review WHERE id=@id";
        cmd.Parameters.AddWithValue("@title", task.Title);
        cmd.Parameters.AddWithValue("@desc", (object?)task.Description ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@diff", (object?)task.Difficulty ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@model", (object?)task.ModelSelected ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@status", task.Status);
        cmd.Parameters.AddWithValue("@rework", task.ReworkCount);
        cmd.Parameters.AddWithValue("@review", (object?)task.ReviewStatus ?? "pending_review");
        cmd.Parameters.AddWithValue("@id", task.Id);
        cmd.ExecuteNonQuery();
    }

    public void DeleteTask(int taskId)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "DELETE FROM task_executions WHERE task_id=@id; DELETE FROM tasks WHERE id=@id";
        cmd.Parameters.AddWithValue("@id", taskId);
        cmd.ExecuteNonQuery();
    }

    // ========== 执行记录 ==========

    public void SaveExecution(TaskExecution execution)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"INSERT INTO task_executions (task_id, model, role, prompt, output, cost_tokens, duration_ms, success, error_msg, attempt_number, review_score, review_notes)
                            VALUES (@tid, @model, @role, @prompt, @output, @tokens, @dur, @success, @err, @attempt, @rscore, @rnotes);
                            SELECT last_insert_rowid();";
        cmd.Parameters.AddWithValue("@tid", execution.TaskId);
        cmd.Parameters.AddWithValue("@model", execution.Model);
        cmd.Parameters.AddWithValue("@role", execution.Role);
        cmd.Parameters.AddWithValue("@prompt", (object?)execution.Prompt ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@output", (object?)execution.Output ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@tokens", (object?)execution.CostTokens ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@dur", (object?)execution.DurationMs ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@success", execution.Success);
        cmd.Parameters.AddWithValue("@err", (object?)execution.ErrorMsg ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@attempt", execution.AttemptNumber);
        cmd.Parameters.AddWithValue("@rscore", (object?)execution.ReviewScore ?? DBNull.Value);
        cmd.Parameters.AddWithValue("@rnotes", (object?)execution.ReviewNotes ?? DBNull.Value);
        var id = (long)cmd.ExecuteScalar()!;
        execution.Id = (int)id;
    }

    public List<TaskExecution> GetExecutionsForTask(int taskId)
    {
        var list = new List<TaskExecution>();
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = @"SELECT id, task_id, model, role, prompt, output, cost_tokens, duration_ms, success, error_msg, executed_at, COALESCE(attempt_number,1), review_score, review_notes
                            FROM task_executions WHERE task_id=@tid ORDER BY id DESC";
        cmd.Parameters.AddWithValue("@tid", taskId);
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
        {
            list.Add(new TaskExecution
            {
                Id = reader.GetInt32(0),
                TaskId = reader.GetInt32(1),
                Model = reader.GetString(2),
                Role = reader.GetString(3),
                Prompt = reader.IsDBNull(4) ? null : reader.GetString(4),
                Output = reader.IsDBNull(5) ? null : reader.GetString(5),
                CostTokens = reader.IsDBNull(6) ? null : reader.GetInt32(6),
                DurationMs = reader.IsDBNull(7) ? null : reader.GetInt64(7),
                Success = reader.GetInt32(8),
                ErrorMsg = reader.IsDBNull(9) ? null : reader.GetString(9),
                ExecutedAt = reader.IsDBNull(10) ? null : reader.GetString(10),
                AttemptNumber = reader.IsDBNull(11) ? 1 : reader.GetInt32(11),
                ReviewScore = reader.IsDBNull(12) ? null : reader.GetDouble(12),
                ReviewNotes = reader.IsDBNull(13) ? null : reader.GetString(13)
            });
        }
        return list;
    }

    private static TaskItem ReadTaskItem(SqliteDataReader reader)
    {
        return new TaskItem
        {
            Id = reader.GetInt32(0),
            ProjectId = reader.GetInt32(1),
            ParentTaskId = reader.IsDBNull(2) ? null : reader.GetInt32(2),
            Title = reader.GetString(3),
            Description = reader.IsDBNull(4) ? null : reader.GetString(4),
            Difficulty = reader.IsDBNull(5) ? null : reader.GetString(5),
            ModelSelected = reader.IsDBNull(6) ? null : reader.GetString(6),
            Status = reader.GetString(7),
            PromptFile = reader.IsDBNull(8) ? null : reader.GetString(8),
            ResultFile = reader.IsDBNull(9) ? null : reader.GetString(9),
            CreatedAt = reader.IsDBNull(10) ? null : reader.GetString(10),
            CompletedAt = reader.IsDBNull(11) ? null : reader.GetString(11),
            ReworkCount = reader.GetInt32(12),
            ReviewStatus = reader.IsDBNull(13) ? "pending_review" : reader.GetString(13)
        };
    }

    // ========== 聊天记录 (Phase 2) ==========

    public void SaveChatMessage(ChatMessage msg)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "INSERT INTO chat_messages (project_id, role, content) VALUES (@pid, @role, @content)";
        cmd.Parameters.AddWithValue("@pid", msg.ProjectId);
        cmd.Parameters.AddWithValue("@role", msg.Role);
        cmd.Parameters.AddWithValue("@content", msg.Content);
        cmd.ExecuteNonQuery();
    }

    public List<ChatMessage> GetChatMessagesForProject(int projectId)
    {
        var list = new List<ChatMessage>();
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "SELECT id, project_id, role, content, created_at FROM chat_messages WHERE project_id=@pid ORDER BY id";
        cmd.Parameters.AddWithValue("@pid", projectId);
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
            list.Add(new ChatMessage
            {
                Id = reader.GetInt32(0),
                ProjectId = reader.GetInt32(1),
                Role = reader.GetString(2),
                Content = reader.GetString(3),
                CreatedAt = reader.IsDBNull(4) ? null : reader.GetString(4)
            });
        return list;
    }

    public void DeleteChatHistory(int projectId)
    {
        using var conn = _db.GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "DELETE FROM chat_messages WHERE project_id=@pid";
        cmd.Parameters.AddWithValue("@pid", projectId);
        cmd.ExecuteNonQuery();
    }
}

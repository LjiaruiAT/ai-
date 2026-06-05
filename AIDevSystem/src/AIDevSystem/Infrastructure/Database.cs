using System;
using System.IO;
using Microsoft.Data.Sqlite;

namespace AIDevSystem.Infrastructure;

public class Database
{
    private readonly string _connectionString;

    public Database(string dbPath)
    {
        // 确保目录存在
        string? dir = Path.GetDirectoryName(dbPath);
        if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
        {
            Directory.CreateDirectory(dir);
        }
        _connectionString = $"Data Source={dbPath}";
    }

    public SqliteConnection GetConnection()
    {
        var conn = new SqliteConnection(_connectionString);
        conn.Open();
        return conn;
    }

    public void Initialize()
    {
        using var conn = GetConnection();
        using var cmd = conn.CreateCommand();

        cmd.CommandText = @"
            CREATE TABLE IF NOT EXISTS projects (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                description TEXT,
                root_path TEXT,
                created_at TEXT DEFAULT (datetime('now')),
                updated_at TEXT DEFAULT (datetime('now'))
            );

            CREATE TABLE IF NOT EXISTS tasks (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                project_id INTEGER REFERENCES projects(id),
                parent_task_id INTEGER REFERENCES tasks(id),
                title TEXT NOT NULL,
                description TEXT,
                difficulty TEXT CHECK(difficulty IN ('low','medium','high')),
                model_selected TEXT,
                status TEXT DEFAULT 'pending',
                prompt_file TEXT,
                result_file TEXT,
                created_at TEXT DEFAULT (datetime('now')),
                completed_at TEXT
            );

            CREATE TABLE IF NOT EXISTS task_executions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                task_id INTEGER REFERENCES tasks(id),
                model TEXT NOT NULL,
                role TEXT NOT NULL,
                prompt TEXT,
                output TEXT,
                cost_tokens INTEGER,
                duration_ms INTEGER,
                success INTEGER DEFAULT 0,
                error_msg TEXT,
                executed_at TEXT DEFAULT (datetime('now'))
            );

            CREATE TABLE IF NOT EXISTS model_scores (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                task_execution_id INTEGER REFERENCES task_executions(id),
                model TEXT NOT NULL,
                difficulty TEXT,
                score_cost REAL,
                score_speed REAL,
                score_pass_rate REAL,
                score_rework REAL,
                score_quality REAL,
                composite_score REAL,
                created_at TEXT DEFAULT (datetime('now'))
            );

            CREATE TABLE IF NOT EXISTS settings (
                key TEXT PRIMARY KEY,
                value TEXT
            );

            -- B1: AI 员工记忆系统
            CREATE TABLE IF NOT EXISTS agent_memories (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                agent TEXT NOT NULL,
                role TEXT NOT NULL,
                task_execution_id INTEGER REFERENCES task_executions(id),
                file_path TEXT,
                good_points TEXT,
                mistakes TEXT,
                composite_score REAL,
                created_at TEXT DEFAULT (datetime('now'))
            );

            -- Phase 2: 对话历史
            CREATE TABLE IF NOT EXISTS chat_messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                project_id INTEGER NOT NULL REFERENCES projects(id),
                role TEXT NOT NULL CHECK(role IN ('user','pm','system')),
                content TEXT NOT NULL,
                created_at TEXT DEFAULT (datetime('now'))
            );
        ";
        cmd.ExecuteNonQuery();
        EnsureDefaults();
        RunMigrations(conn);
    }

    private static void RunMigrations(SqliteConnection conn)
    {
        // Phase 4: tasks 表新增列
        TryAddColumn(conn, "tasks", "rework_count", "INTEGER DEFAULT 0");
        TryAddColumn(conn, "tasks", "review_status", "TEXT DEFAULT 'pending_review'");

        // Phase 4: task_executions 表新增列
        TryAddColumn(conn, "task_executions", "attempt_number", "INTEGER DEFAULT 1");
        TryAddColumn(conn, "task_executions", "review_score", "REAL");
        TryAddColumn(conn, "task_executions", "review_notes", "TEXT");
    }

    private static void TryAddColumn(SqliteConnection conn, string table, string column, string typeDef)
    {
        try
        {
            using var cmd = conn.CreateCommand();
            cmd.CommandText = $"ALTER TABLE {table} ADD COLUMN {column} {typeDef}";
            cmd.ExecuteNonQuery();
        }
        catch (SqliteException)
        {
            // 列已存在，忽略
        }
    }

    public void EnsureDefaults()
    {
        using var conn = GetConnection();
        InsertSettingIfNotExists(conn, "deepseek_api_key", "");
        InsertSettingIfNotExists(conn, "default_pm_model", "claude");
        InsertSettingIfNotExists(conn, "auto_mode", "true");
        InsertSettingIfNotExists(conn, "project_root", @"C:\Users\lenovo\Desktop\开发记录\AIProjects");
        InsertSettingIfNotExists(conn, "db_path", @"C:\Users\lenovo\Desktop\开发记录\AIProjects\aidev.db");
    }

    private static void InsertSettingIfNotExists(SqliteConnection conn, string key, string value)
    {
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "INSERT OR IGNORE INTO settings (key, value) VALUES (@key, @value)";
        cmd.Parameters.AddWithValue("@key", key);
        cmd.Parameters.AddWithValue("@value", value);
        cmd.ExecuteNonQuery();
    }

    // ---- Settings CRUD ----

    public string? GetSetting(string key)
    {
        using var conn = GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "SELECT value FROM settings WHERE key = @key";
        cmd.Parameters.AddWithValue("@key", key);
        var result = cmd.ExecuteScalar();
        return result?.ToString();
    }

    public void SetSetting(string key, string value)
    {
        using var conn = GetConnection();
        using var cmd = conn.CreateCommand();
        cmd.CommandText = "INSERT OR REPLACE INTO settings (key, value) VALUES (@key, @value)";
        cmd.Parameters.AddWithValue("@key", key);
        cmd.Parameters.AddWithValue("@value", value);
        cmd.ExecuteNonQuery();
    }
}

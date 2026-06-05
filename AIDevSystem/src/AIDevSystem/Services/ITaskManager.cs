using System.Collections.Generic;
using System.Threading.Tasks;
using AIDevSystem.Models;

namespace AIDevSystem.Services;

public interface ITaskManager
{
    List<Project> GetProjects();
    Project CreateProject(string name, string? description = null, string? rootPath = null);
    List<TaskItem> GetTasks(int projectId);
    void UpdateTaskStatus(int taskId, string status, string? completedAt = null);
    long SaveExecution(TaskExecution execution);
}

public interface IModelRouter
{
    string SelectModel(string difficulty, string category);
}

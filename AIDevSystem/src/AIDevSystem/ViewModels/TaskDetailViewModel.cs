using System.Collections.ObjectModel;
using System.Windows.Input;
using AIDevSystem.Models;
using AIDevSystem.Services;

namespace AIDevSystem.ViewModels;

public class TaskDetailViewModel : BaseViewModel
{
    private readonly TaskManager _taskManager;

    public TaskItem Task { get; }
    public ObservableCollection<TaskExecution> Executions { get; } = new();

    private string _outputText = "";
    public string OutputText
    {
        get => _outputText;
        set => Set(ref _outputText, value);
    }

    private TaskExecution? _selectedExecution;
    public TaskExecution? SelectedExecution
    {
        get => _selectedExecution;
        set
        {
            if (Set(ref _selectedExecution, value))
                OutputText = value?.Output ?? value?.ErrorMsg ?? "";
        }
    }

    public ICommand GoBackCommand { get; }

    public TaskDetailViewModel(TaskItem task, System.Collections.Generic.List<TaskExecution> executions, TaskManager taskManager)
    {
        _taskManager = taskManager;
        Task = task;

        foreach (var e in executions)
            Executions.Add(e);

        if (executions.Count > 0)
            SelectedExecution = executions[0];

        GoBackCommand = new RelayCommand(_ =>
        {
            // 返回项目列表
            if (System.Windows.Application.Current.MainWindow?.DataContext is MainViewModel mainVM)
                mainVM.NavigateTo("projects");
        });
    }
}

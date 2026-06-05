using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using AIDevSystem.Models;
using AIDevSystem.Services;

namespace AIDevSystem.ViewModels;

public class ProjectListViewModel : BaseViewModel
{
    private readonly TaskManager _taskManager;
    private readonly TaskDecomposer _decomposer;
    private readonly TaskExecutor _executor;
    private readonly ScoreService _scoreService;
    private readonly MainViewModel _mainVM;

    public ObservableCollection<Project> Projects { get; } = new();
    public ObservableCollection<TaskItem> Tasks { get; } = new();

    private Project? _selectedProject;
    public Project? SelectedProject
    {
        get => _selectedProject;
        set
        {
            if (SetProperty(ref _selectedProject, value) && value != null)
                LoadTasks(value.Id);
        }
    }

    private TaskItem? _selectedTask;
    public TaskItem? SelectedTask { get => _selectedTask; set => SetProperty(ref _selectedTask, value); }

    private string _newProjectName = "";
    public string NewProjectName { get => _newProjectName; set => SetProperty(ref _newProjectName, value); }

    private string _requirementText = "";
    public string RequirementText { get => _requirementText; set => SetProperty(ref _requirementText, value); }

    // Phase 4: PM 审查开关
    private bool _enablePMReview = true;
    public bool EnablePMReview { get => _enablePMReview; set => SetProperty(ref _enablePMReview, value); }

    // Phase 3: 进度跟踪
    public ProgressTracker Progress { get; } = new();

    public ICommand CreateProjectCommand { get; }
    public ICommand DecomposeCommand { get; }
    public ICommand ExecuteTaskCommand { get; }
    public ICommand ExecuteAllCommand { get; }
    public ICommand ViewTaskCommand { get; }
    public ICommand DiscussCommand { get; }
    public ICommand RefreshCommand { get; }

    public ProjectListViewModel(TaskManager taskManager, TaskDecomposer decomposer,
        TaskExecutor executor, ScoreService scoreService, MainViewModel mainVM)
    {
        _taskManager = taskManager;
        _decomposer = decomposer;
        _executor = executor;
        _scoreService = scoreService;
        _mainVM = mainVM;

        CreateProjectCommand = new RelayCommand(_ => CreateProject());
        DecomposeCommand = new RelayCommand(async _ => await DecomposeAsync());
        ExecuteTaskCommand = new RelayCommand(async t => { if (t is TaskItem task) { SelectedTask = task; await ExecuteSelectedTaskAsync(); } });
        ExecuteAllCommand = new RelayCommand(async _ => await ExecuteAllAsync());
        ViewTaskCommand = new RelayCommand(t => { if (t is TaskItem task) _mainVM.ShowTaskDetail(task.Id); });
        DiscussCommand = new RelayCommand(_ => OpenDiscussion());
        RefreshCommand = new RelayCommand(_ => Refresh());
    }

    public void Refresh()
    {
        Projects.Clear();
        foreach (var p in _taskManager.GetAllProjects())
            Projects.Add(p);
    }

    private void LoadTasks(int projectId)
    {
        Tasks.Clear();
        foreach (var t in _taskManager.GetTasksForProject(projectId))
            Tasks.Add(t);
    }

    private void CreateProject()
    {
        if (string.IsNullOrWhiteSpace(NewProjectName)) return;
        // 项目不再绑定物理路径，只是一个逻辑分组
        var project = _taskManager.CreateProject(NewProjectName, null, null);
        NewProjectName = "";
        Refresh();
        SelectedProject = project;
    }

    // Phase 2: PM 讨论
    private void OpenDiscussion()
    {
        if (SelectedProject == null) return;
        _mainVM.CurrentProjectId = SelectedProject.Id;
        _mainVM.NavigateTo("chat");
    }

    private async Task DecomposeAsync()
    {
        if (SelectedProject == null || string.IsNullOrWhiteSpace(RequirementText)) return;
        IsBusy = true;
        _mainVM.SetProgress("正在拆解需求...");
        try
        {
            var tasks = await _decomposer.DecomposeAsync(SelectedProject.Id, RequirementText);
            RequirementText = "";
            LoadTasks(SelectedProject.Id);
            _mainVM.SetProgress($"✅ 拆解完成，生成 {tasks.Count} 个子任务");
        }
        catch (Exception ex)
        {
            MessageBox.Show($"拆解失败: {ex.Message}", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
        }
        finally { IsBusy = false; }
    }

    private async Task ExecuteSelectedTaskAsync()
    {
        if (SelectedTask == null || SelectedProject == null) return;
        IsBusy = true;
        try
        {
            TaskExecution execution;
            if (EnablePMReview)
                execution = await App.PMReviewService!.ExecuteWithReviewAsync(SelectedTask, Progress);
            else
                execution = await _executor.ExecuteTaskAsync(SelectedTask, null, Progress);

            _scoreService.CalculateScore(execution, SelectedTask);
            LoadTasks(SelectedProject.Id);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"执行失败: {ex.Message}", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
        }
        finally { IsBusy = false; }
    }

    private async Task ExecuteAllAsync()
    {
        if (SelectedProject == null) return;
        IsBusy = true;
        var pending = Tasks.Where(t => t.Status == "pending").ToList();
        Progress.StartBatch(pending.Count);
        _mainVM.ShowProgressBar = true;

        try
        {
            foreach (var task in pending)
            {
                _mainVM.SetProgress($"执行: {task.Title}");
                if (EnablePMReview)
                    await App.PMReviewService!.ExecuteWithReviewAsync(task, Progress);
                else
                    await _executor.ExecuteTaskAsync(task, line => _mainVM.SetProgress(line), Progress);
            }
            LoadTasks(SelectedProject.Id);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"执行失败: {ex.Message}", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
        }
        finally
        {
            Progress.Reset();
            _mainVM.ShowProgressBar = false;
            IsBusy = false;
        }
    }
}

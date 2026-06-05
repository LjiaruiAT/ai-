using System;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using AIDevSystem.Services;

namespace AIDevSystem.ViewModels;

public class MainViewModel : BaseViewModel
{
    private readonly TaskManager _taskManager;
    private readonly TaskDecomposer _decomposer;
    private readonly TaskExecutor _executor;
    private readonly ScoreService _scoreService;

    private BaseViewModel? _currentView;
    public BaseViewModel? CurrentView
    {
        get => _currentView;
        set => Set(ref _currentView, value);
    }

    private string _statusText = "就绪";
    public string StatusText { get => _statusText; set => Set(ref _statusText, value); }

    private string _taskProgressText = "";
    public string TaskProgressText { get => _taskProgressText; set => Set(ref _taskProgressText, value); }

    // Phase 3: 进度条
    private double _overallProgress;
    public double OverallProgress { get => _overallProgress; set => Set(ref _overallProgress, value); }

    private bool _showProgressBar;
    public bool ShowProgressBar { get => _showProgressBar; set => Set(ref _showProgressBar, value); }

    // Phase 2: 导航上下文
    public int CurrentProjectId { get; set; }

    // VM 缓存
    private ProjectListViewModel? _projectListVM;
    private SettingsViewModel? _settingsVM;
    private DashboardViewModel? _dashboardVM;
    private ChatViewModel? _chatVM;
    private HelpViewModel? _helpVM;

    public ICommand NavigateCommand { get; }
    public ICommand HelpCommand { get; }

    public MainViewModel(TaskManager taskManager, TaskDecomposer decomposer, TaskExecutor executor, ScoreService scoreService)
    {
        _taskManager = taskManager;
        _decomposer = decomposer;
        _executor = executor;
        _scoreService = scoreService;

        NavigateCommand = new RelayCommand(Navigate);
        HelpCommand = new RelayCommand(_ => NavigateTo("help"));
        NavigateTo("projects");
    }

    public void Navigate(object? parameter)
    {
        var page = parameter?.ToString() ?? "projects";
        NavigateTo(page);
    }

    public void NavigateTo(string page)
    {
        switch (page)
        {
            case "projects":
                _projectListVM ??= new ProjectListViewModel(_taskManager, _decomposer, _executor, _scoreService, this);
                _projectListVM.Refresh();
                CurrentView = _projectListVM;
                StatusText = "📁 项目列表";
                break;
            case "settings":
                _settingsVM ??= new SettingsViewModel(this);
                CurrentView = _settingsVM;
                StatusText = "⚙ 系统设置";
                break;
            case "dashboard":
                _dashboardVM ??= new DashboardViewModel(_scoreService, _taskManager);
                _dashboardVM.Refresh();
                CurrentView = _dashboardVM;
                StatusText = "📊 仪表盘";
                break;
            case "chat":
                _chatVM ??= new ChatViewModel(App.ChatService!, _taskManager, _decomposer, this);
                _chatVM.Initialize(CurrentProjectId);
                CurrentView = _chatVM;
                StatusText = "💬 需求讨论";
                break;
            case "help":
                _helpVM ??= new HelpViewModel();
                CurrentView = _helpVM;
                StatusText = "❓ 用户指南";
                break;
        }
    }

    public void ShowTaskDetail(int taskId)
    {
        var task = _taskManager.GetTask(taskId);
        if (task == null) return;

        var executions = _taskManager.GetExecutionsForTask(taskId);
        var detailVM = new TaskDetailViewModel(task, executions, _taskManager);
        CurrentView = detailVM;
        StatusText = $"📋 任务: {task.Title}";
    }

    public void SetProgress(string text) => TaskProgressText = text;
}

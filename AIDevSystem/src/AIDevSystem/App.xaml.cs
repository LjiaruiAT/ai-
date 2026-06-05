using System;
using System.Windows;
using System.Windows.Threading;
using AIDevSystem.Infrastructure;
using AIDevSystem.Services;
using AIDevSystem.ViewModels;

namespace AIDevSystem;

public partial class App : Application
{
    // 全局服务实例
    public static Database Database { get; private set; } = null!;
    public static DeepSeekApiClient ApiClient { get; private set; } = null!;
    public static ClaudeAdapter ClaudeAdapter { get; private set; } = null!;
    public static ReasonXAdapter ReasonXAdapter { get; private set; } = null!;
    public static TaskManager TaskManager { get; private set; } = null!;
    public static TaskDecomposer TaskDecomposer { get; private set; } = null!;
    public static TaskExecutor TaskExecutor { get; private set; } = null!;
    public static ScoreService ScoreService { get; private set; } = null!;
    public static MemoryService MemoryService { get; private set; } = null!;
    public static ChatService ChatService { get; private set; } = null!;
    public static PMReviewService PMReviewService { get; private set; } = null!;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        // 全局异常处理
        DispatcherUnhandledException += (s, args) =>
        {
            MessageBox.Show($"未处理的错误:\n{args.Exception.Message}", "错误", 
                MessageBoxButton.OK, MessageBoxImage.Error);
            args.Handled = true;
        };

        // 初始化基础设施
        string dbPath = @"C:\Users\lenovo\Desktop\开发记录\AIProjects\aidev.db";
        Database = new Database(dbPath);
        Database.Initialize();

        // API 直连（替代 CLI 进程调用）
        ApiClient = new DeepSeekApiClient();
        var apiKey = Database.GetSetting("deepseek_api_key") ?? "";
        if (!string.IsNullOrEmpty(apiKey)) ApiClient.SetApiKey(apiKey);

        ClaudeAdapter = new ClaudeAdapter(ApiClient);
        ReasonXAdapter = new ReasonXAdapter(ApiClient);

        // 初始化服务层
        TaskManager = new TaskManager(Database);
        TaskDecomposer = new TaskDecomposer(ClaudeAdapter, Database, TaskManager);
        MemoryService = new MemoryService(Database);
        TaskExecutor = new TaskExecutor(ReasonXAdapter, Database, TaskManager, MemoryService);
        ScoreService = new ScoreService(Database);
        ChatService = new ChatService(ClaudeAdapter, TaskManager);
        PMReviewService = new PMReviewService(ClaudeAdapter, TaskExecutor, TaskManager, ScoreService);

        // 启动主窗口
        var mainVM = new MainViewModel(TaskManager, TaskDecomposer, TaskExecutor, ScoreService);
        var mainWindow = new MainWindow { DataContext = mainVM };
        mainWindow.Show();

        // 首次运行引导
        if (string.IsNullOrEmpty(apiKey))
            mainVM.NavigateTo("settings");
    }

    protected override void OnExit(ExitEventArgs e)
    {
        base.OnExit(e);
    }
}

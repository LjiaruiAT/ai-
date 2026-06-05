using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using AIDevSystem.Models;
using AIDevSystem.Services;

namespace AIDevSystem.ViewModels;

public class ChatViewModel : BaseViewModel
{
    private readonly ChatService _chatService;
    private readonly TaskManager _taskManager;
    private readonly TaskDecomposer _decomposer;
    private readonly MainViewModel _mainVM;

    public ObservableCollection<ChatMessage> Messages { get; } = new();

    private string _userInput = "";
    public string UserInput { get => _userInput; set => SetProperty(ref _userInput, value); }

    private bool _isPmThinking;
    public bool IsPmThinking { get => _isPmThinking; set => SetProperty(ref _isPmThinking, value); }

    private bool _canApprove;
    public bool CanApprove { get => _canApprove; set => SetProperty(ref _canApprove, value); }

    private Project? _project;
    public Project? Project { get => _project; set => SetProperty(ref _project, value); }

    public ICommand SendMessageCommand { get; }
    public ICommand ApproveAndDecomposeCommand { get; }
    public ICommand ClearChatCommand { get; }
    public ICommand BackToProjectCommand { get; }

    public ChatViewModel(ChatService chatService, TaskManager taskManager,
        TaskDecomposer decomposer, MainViewModel mainVM)
    {
        _chatService = chatService;
        _taskManager = taskManager;
        _decomposer = decomposer;
        _mainVM = mainVM;

        SendMessageCommand = new RelayCommand(async _ => await SendMessageAsync());
        ApproveAndDecomposeCommand = new RelayCommand(async _ => await ApproveAndDecomposeAsync());
        ClearChatCommand = new RelayCommand(() => ClearChat());
        BackToProjectCommand = new RelayCommand(_ => _mainVM.NavigateTo("projects"));
    }

    public void Initialize(int projectId)
    {
        Project = _taskManager.GetProject(projectId);
        if (Project == null) return;

        Messages.Clear();
        var history = _taskManager.GetChatMessagesForProject(projectId);
        foreach (var msg in history)
            Messages.Add(msg);

        CanApprove = Messages.Count >= 4;

        if (Messages.Count == 0)
        {
            Messages.Add(new ChatMessage
            {
                ProjectId = projectId,
                Role = "system",
                Content = $"👋 欢迎讨论项目「{Project.Name}」。请描述你的需求，我会帮你澄清和完善。"
            });
        }
    }

    private async Task SendMessageAsync()
    {
        if (string.IsNullOrWhiteSpace(UserInput) || IsPmThinking || Project == null) return;

        var msg = UserInput.Trim();
        UserInput = "";
        IsPmThinking = true;

        try
        {
            // 只发送最近的消息，避免请求体过大
            var recentMessages = Messages.Skip(Messages.Count > 20 ? Messages.Count - 20 : 0).ToList();
            var pmMsg = await _chatService.SendMessageAsync(Project.Id, msg, recentMessages);
            Messages.Add(pmMsg);
            CanApprove = Messages.Count >= 4;
        }
        catch (Exception ex)
        {
            Messages.Add(new ChatMessage
            {
                ProjectId = Project.Id,
                Role = "system",
                Content = $"❌ 错误: {ex.Message}"
            });
        }
        finally
        {
            IsPmThinking = false;
        }
    }

    private async Task ApproveAndDecomposeAsync()
    {
        if (Project == null) return;
        IsPmThinking = true;
        try
        {
            Messages.Add(new ChatMessage
            {
                ProjectId = Project.Id,
                Role = "system",
                Content = "📝 正在综合需求规格..."
            });

            var spec = await _chatService.SynthesizeFinalSpecAsync(Project.Id, Messages.ToList());

            Messages.Add(new ChatMessage
            {
                ProjectId = Project.Id,
                Role = "system",
                Content = "🔍 正在拆解为子任务..."
            });

            var tasks = await _decomposer.DecomposeAsync(Project.Id, spec);

            Messages.Add(new ChatMessage
            {
                ProjectId = Project.Id,
                Role = "system",
                Content = $"✅ 已生成 {tasks.Count} 个子任务，返回项目视图查看。"
            });

            await Task.Delay(1000);
            _mainVM.NavigateTo("projects");
        }
        catch (Exception ex)
        {
            Messages.Add(new ChatMessage
            {
                ProjectId = Project.Id,
                Role = "system",
                Content = $"❌ 拆解失败: {ex.Message}"
            });
        }
        finally
        {
            IsPmThinking = false;
        }
    }

    private void ClearChat()
    {
        if (Project == null) return;
        _taskManager.DeleteChatHistory(Project.Id);
        Messages.Clear();
        CanApprove = false;
        Messages.Add(new ChatMessage
        {
            ProjectId = Project.Id,
            Role = "system",
            Content = "对话已重置。请描述你的新需求。"
        });
    }
}

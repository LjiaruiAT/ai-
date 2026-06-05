using System.Collections.ObjectModel;
using System.Linq;
using AIDevSystem.Models;
using AIDevSystem.Services;

namespace AIDevSystem.ViewModels;

public class DashboardViewModel : BaseViewModel
{
    private readonly ScoreService _scoreService;
    private readonly TaskManager _taskManager;

    public ObservableCollection<ModelScoreSummary> Scores { get; } = new();

    private string _summaryText = "";
    public string SummaryText
    {
        get => _summaryText;
        set => Set(ref _summaryText, value);
    }

    public DashboardViewModel(ScoreService scoreService, TaskManager taskManager)
    {
        _scoreService = scoreService;
        _taskManager = taskManager;
    }

    public void Refresh()
    {
        Scores.Clear();
        var summaries = _scoreService.GetModelScoreSummaries();
        foreach (var s in summaries)
            Scores.Add(s);

        if (Scores.Count > 0)
        {
            var best = Scores.OrderByDescending(s => s.AvgComposite).First();
            SummaryText = $"🏆 最佳模型: {best.Model} (综合评分: {best.AvgComposite:F1}) | 共 {Scores.Sum(s => s.TotalTasks)} 次评测";
        }
        else
        {
            SummaryText = "暂无评分数据，请先执行一些任务";
        }
    }
}

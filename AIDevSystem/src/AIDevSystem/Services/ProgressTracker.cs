using System;
using System.ComponentModel;

namespace AIDevSystem.Services;

public class ProgressTracker : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    private int _totalTasks;
    public int TotalTasks { get => _totalTasks; set { _totalTasks = value; Notify(nameof(TotalTasks)); Recalc(); } }

    private int _completedTasks;
    public int CompletedTasks { get => _completedTasks; set { _completedTasks = value; Notify(nameof(CompletedTasks)); Recalc(); } }

    private int _failedTasks;
    public int FailedTasks { get => _failedTasks; set { _failedTasks = value; Notify(nameof(FailedTasks)); Recalc(); } }

    private double _currentTaskProgress;
    public double CurrentTaskProgress { get => _currentTaskProgress; set { _currentTaskProgress = value; Notify(nameof(CurrentTaskProgress)); Recalc(); } }

    private string _currentTaskTitle = "";
    public string CurrentTaskTitle { get => _currentTaskTitle; set { _currentTaskTitle = value; Notify(nameof(CurrentTaskTitle)); } }

    private double _overallPercentage;
    public double OverallPercentage { get => _overallPercentage; set { _overallPercentage = value; Notify(nameof(OverallPercentage)); } }

    private string _statusText = "";
    public string StatusText { get => _statusText; set { _statusText = value; Notify(nameof(StatusText)); } }

    private bool _isActive;
    public bool IsActive { get => _isActive; set { _isActive = value; Notify(nameof(IsActive)); } }

    public void StartBatch(int totalTasks)
    {
        TotalTasks = totalTasks;
        CompletedTasks = 0;
        FailedTasks = 0;
        CurrentTaskProgress = 0;
        CurrentTaskTitle = "";
        IsActive = true;
        Recalc();
    }

    public void StartTask(string title) { CurrentTaskTitle = title; CurrentTaskProgress = 0; }
    public void ReportTaskProgress(double pct) => CurrentTaskProgress = Math.Clamp(pct, 0, 100);
    public void CompleteTask() { CompletedTasks++; CurrentTaskProgress = 100; CurrentTaskTitle = ""; Recalc(); }
    public void FailTask() { FailedTasks++; CurrentTaskProgress = 0; CurrentTaskTitle = ""; Recalc(); }

    public void Reset()
    {
        IsActive = false;
        TotalTasks = 0; CompletedTasks = 0; FailedTasks = 0;
        CurrentTaskProgress = 0; CurrentTaskTitle = "";
        OverallPercentage = 0; StatusText = "";
    }

    private void Recalc()
    {
        if (TotalTasks == 0) { OverallPercentage = 0; return; }
        double baseProgress = (CompletedTasks + FailedTasks) * 100.0 / TotalTasks;
        double currentContribution = CurrentTaskProgress / TotalTasks;
        OverallPercentage = Math.Min(100, baseProgress + currentContribution);
        StatusText = $"{CompletedTasks}/{TotalTasks} tasks ({OverallPercentage:F0}%)";
    }

    private void Notify(string name) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

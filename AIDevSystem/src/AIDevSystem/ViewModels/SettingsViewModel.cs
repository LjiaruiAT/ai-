using System;
using System.Windows.Input;
using AIDevSystem.Infrastructure;

namespace AIDevSystem.ViewModels;

public class SettingsViewModel : BaseViewModel
{
    private readonly MainViewModel _mainVM;
    private static readonly Database _db = App.Database;

    private string _apiKey = "";
    public string ApiKey
    {
        get => _apiKey;
        set => Set(ref _apiKey, value);
    }

    private string _defaultPmModel = "claude";
    public string DefaultPmModel
    {
        get => _defaultPmModel;
        set => Set(ref _defaultPmModel, value);
    }

    private bool _autoMode = true;
    public bool AutoMode
    {
        get => _autoMode;
        set => Set(ref _autoMode, value);
    }

    private string _projectRoot = "";
    public string ProjectRoot
    {
        get => _projectRoot;
        set => Set(ref _projectRoot, value);
    }

    public ICommand SaveCommand { get; }

    public SettingsViewModel(MainViewModel mainVM)
    {
        _mainVM = mainVM;
        SaveCommand = new RelayCommand(_ => Save());

        // 加载当前设置
        ApiKey = _db.GetSetting("deepseek_api_key") ?? "";
        DefaultPmModel = _db.GetSetting("default_pm_model") ?? "claude";
        AutoMode = _db.GetSetting("auto_mode") != "false";
        ProjectRoot = _db.GetSetting("project_root") ?? "";
    }

    private void Save()
    {
        _db.SetSetting("deepseek_api_key", ApiKey);
        _db.SetSetting("default_pm_model", DefaultPmModel);
        _db.SetSetting("auto_mode", AutoMode ? "true" : "false");
        _db.SetSetting("project_root", ProjectRoot);
        _mainVM.SetProgress("设置已保存");
    }
}

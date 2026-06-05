using System.Windows.Controls;

namespace AIDevSystem.Views;

public partial class HelpView : UserControl
{
    public HelpView()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, System.Windows.RoutedEventArgs e)
    {
        if (DataContext is ViewModels.HelpViewModel vm)
            GuideBrowser.NavigateToString(vm.GuideHtml);
    }
}

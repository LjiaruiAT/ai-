using System.Windows;
using System.Windows.Controls;

namespace AIDevSystem.Controls;

public partial class ActivityIndicator : UserControl
{
    public static readonly DependencyProperty IsActiveProperty =
        DependencyProperty.Register(nameof(IsActive), typeof(bool), typeof(ActivityIndicator),
            new PropertyMetadata(true, OnIsActiveChanged));

    public static readonly DependencyProperty StatusTextProperty =
        DependencyProperty.Register(nameof(StatusText), typeof(string), typeof(ActivityIndicator),
            new PropertyMetadata("处理中..."));

    public bool IsActive
    {
        get => (bool)GetValue(IsActiveProperty);
        set => SetValue(IsActiveProperty, value);
    }

    public string StatusText
    {
        get => (string)GetValue(StatusTextProperty);
        set => SetValue(StatusTextProperty, value);
    }

    public ActivityIndicator()
    {
        InitializeComponent();
    }

    private static void OnIsActiveChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var control = (ActivityIndicator)d;
        control.Visibility = (bool)e.NewValue ? Visibility.Visible : Visibility.Collapsed;
    }
}

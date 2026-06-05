using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;

namespace AIDevSystem.Converters;

public class InvertBoolConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is bool b && !b;

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => value is bool b && !b;
}

public class BoolToVisibilityConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is bool b && b ? Visibility.Visible : Visibility.Collapsed;

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => value is Visibility v && v == Visibility.Visible;
}

public class ZeroToVisibilityConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is int i && i == 0) return Visibility.Visible;
        return Visibility.Collapsed;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

public class SuccessToEmojiConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is int i && i == 1 ? "✅" : "❌";

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

public class BoolToColorConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is int i && i == 1 ? new SolidColorBrush(Color.FromRgb(0x10, 0xB9, 0x81))
                                     : new SolidColorBrush(Color.FromRgb(0xEF, 0x44, 0x44));

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

// Phase 1: 任务状态 → 颜色
public class StatusToColorConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        var s = value?.ToString() ?? "";
        return s switch
        {
            "completed" or "passed_review" => new SolidColorBrush(Color.FromRgb(0x10, 0xB9, 0x81)),
            "failed" or "max_retries_exceeded" => new SolidColorBrush(Color.FromRgb(0xEF, 0x44, 0x44)),
            "in_progress" or "running" => new SolidColorBrush(Color.FromRgb(0xFB, 0xBF, 0x24)),
            "in_rework" => new SolidColorBrush(Color.FromRgb(0xF9, 0x73, 0x16)),
            _ => new SolidColorBrush(Color.FromRgb(0x88, 0x88, 0x88))
        };
    }
    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

// Phase 1: 聊天角色 → 水平对齐
public class RoleToAlignmentConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        var role = value?.ToString() ?? "";
        return role switch
        {
            "user" => HorizontalAlignment.Right,
            "pm" => HorizontalAlignment.Left,
            _ => HorizontalAlignment.Center
        };
    }
    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

// Phase 1: 聊天角色 → 可见性（多个实例，用 Role 属性区分）
public class RoleToVisibilityConverter : IValueConverter
{
    public string Role { get; set; } = "";
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        var role = value?.ToString() ?? "";
        return role == Role ? Visibility.Visible : Visibility.Collapsed;
    }
    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotImplementedException();
}

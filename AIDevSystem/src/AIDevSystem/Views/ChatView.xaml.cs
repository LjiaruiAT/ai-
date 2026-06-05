using System.Collections.Specialized;
using System.Windows.Controls;

namespace AIDevSystem.Views;

public partial class ChatView : UserControl
{
    public ChatView()
    {
        InitializeComponent();
        Loaded += (_, _) =>
        {
            if (DataContext is ViewModels.ChatViewModel vm)
                vm.Messages.CollectionChanged += OnMessagesChanged;
        };
    }

    private void OnMessagesChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        Dispatcher.InvokeAsync(() => ChatScroll.ScrollToEnd(), System.Windows.Threading.DispatcherPriority.Background);
    }
}

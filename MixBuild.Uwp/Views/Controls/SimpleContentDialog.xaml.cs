using Windows.UI.Xaml.Controls;

namespace MixBuild.Uwp.Views.Controls
{
    public sealed partial class SimpleContentDialog : ContentDialog
    {
        public new string Title { get; set; }
        public new string Content { get; set; }
        public new string CloseButtonText { get; set; } = "Close";

        public SimpleContentDialog()
        {
            this.InitializeComponent();
        }
    }
}

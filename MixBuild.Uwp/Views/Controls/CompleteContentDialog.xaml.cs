using Windows.UI.Xaml.Controls;

namespace MixBuild.Uwp.Views.Controls
{
    public sealed partial class CompleteContentDialog : ContentDialog
    {
        public bool CanDismiss { get; set; } = false;

        public CompleteContentDialog()
        {
            this.InitializeComponent();
        }

        private void ContentDialog_Closing(ContentDialog sender, ContentDialogClosingEventArgs args)
        {
            if (!CanDismiss) args.Cancel = true;
        }

        private void ContentDialog_CloseButtonClick(ContentDialog sender, ContentDialogButtonClickEventArgs args) => CanDismiss = true;
    }
}

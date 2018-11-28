using System;
using Windows.Storage.Pickers;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MixBuild.Uwp.Views.Controls
{
    public sealed partial class CompleteContentDialog : ContentDialog
    {
        public bool CanDismiss { get; set; } = false;
        public string OutputFilePath { get; set; }

        public CompleteContentDialog(string outputFilePath)
        {
            this.InitializeComponent();
            OutputFilePath = outputFilePath;
        }

        private void ContentDialog_Closing(ContentDialog sender, ContentDialogClosingEventArgs args)
        {
            if (!CanDismiss) args.Cancel = true;
            else
            {
                var frame = Window.Current.Content as Frame;
                if (frame.CanGoBack) frame.GoBack();
            }
        }

        private void ContentDialog_CloseButtonClick(ContentDialog sender, ContentDialogButtonClickEventArgs args) => CanDismiss = true;

        private async void ContentDialog_PrimaryButtonClick(ContentDialog sender, ContentDialogButtonClickEventArgs args)
        {
            var picker = new FolderPicker();
            picker.FileTypeFilter.Add(".jpg");
            var folder = await picker.PickSingleFolderAsync();

            if (folder != null)
            {
                // TODO: copy the output file

                CanDismiss = true;
                sender.Hide();
            }
        }
    }
}

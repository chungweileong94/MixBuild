using Windows.UI.Xaml.Controls;

namespace MixBuild.Uwp.Views.Controls
{
    public sealed partial class ConfirmationContentDialog : ContentDialog
    {
        public new string Title { get; set; }
        public new string Content { get; set; }
        public new string PrimaryButtonText { get; set; } = "Yes";
        public new string SecondaryButtonText { get; set; } = "No";

        public ConfirmationContentDialog()
        {
            this.InitializeComponent();
        }
    }
}

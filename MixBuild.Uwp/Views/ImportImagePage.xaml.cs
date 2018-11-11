using Windows.ApplicationModel.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MixBuild.Uwp.Views
{
    public sealed partial class ImportImagePage : Page
    {
        public ImportImagePage()
        {
            this.InitializeComponent();
            BackButton.Click += (s, e) =>
            {
                Frame rootFrame = Window.Current.Content as Frame;
                if (rootFrame.CanGoBack) rootFrame.GoBack();
            };

            //setup title bar draggable region
            var coreTitleBar = CoreApplication.GetCurrentView().TitleBar;
            AppTitleBar.Height = coreTitleBar.Height;
            coreTitleBar.LayoutMetricsChanged += (s, args) => AppTitleBar.Height = coreTitleBar.Height;

            Window.Current.SetTitleBar(AppTitleBar);
        }
    }
}

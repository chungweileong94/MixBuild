using MixBuild.Uwp.ViewModels;
using MixBuild.Uwp.Views.Controls;
using System;
using Windows.ApplicationModel.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

namespace MixBuild.Uwp.Views
{
    public sealed partial class ImportImagePage : Page
    {
        public ImportImageViewModel ViewModel => new ImportImageViewModel();

        public ImportImagePage()
        {
            this.InitializeComponent();

            BackButton.Click += async (s, e) =>
            {
                var dialog = new ConfirmationContentDialog
                {
                    Title = "Go Back",
                    Content = "Are you sure to cancel the process?"
                };

                var dialogResult = await dialog.ShowAsync();

                if (dialogResult == ContentDialogResult.Primary)
                {
                    Frame rootFrame = Window.Current.Content as Frame;
                    if (rootFrame.CanGoBack) rootFrame.GoBack();
                }
            };

            //setup title bar draggable region
            var coreTitleBar = CoreApplication.GetCurrentView().TitleBar;
            AppTitleBar.Height = coreTitleBar.Height;
            coreTitleBar.LayoutMetricsChanged += (s, args) => AppTitleBar.Height = coreTitleBar.Height;

            Window.Current.SetTitleBar(AppTitleBar);
        }
    }

    #region Coverters
    public class InvertBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language) => !(bool)value;

        public object ConvertBack(object value, Type targetType, object parameter, string language) => throw new NotImplementedException();
    }
    #endregion
}

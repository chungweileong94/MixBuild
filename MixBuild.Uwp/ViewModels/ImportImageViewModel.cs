using MixBuild.Uwp.Helpers;
using MixBuild.Uwp.Models;
using MixBuild.Uwp.Views.Controls;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Core;
using Windows.Storage;
using Windows.Storage.Search;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using static MixBuild.Uwp.Models.ImageData;

namespace MixBuild.Uwp.ViewModels
{
    public class ImportImageViewModel : BaseViewModel
    {
        public ObservableCollection<ImageData> ImageCollection { get; } = new ObservableCollection<ImageData>();
        private bool _IsReady;
        public bool IsReady
        {
            get => _IsReady;
            set => Set(ref _IsReady, value);
        }
        private bool _IsLoading;
        public bool IsLoading
        {
            get => _IsLoading;
            set => Set(ref _IsLoading, value);
        }

        public ICommand ProceedReconstructionCommand => new RelayCommand<RoutedEventArgs>(async (e) => await ProceedReconstructionAsync());

        public ImportImageViewModel()
        {
            IsReady = false;
            IsLoading = false;
            ImageCollection = new ObservableCollection<ImageData>
            {
                new ImageData(Faces.Front, BitmapChanged_Callback),
                new ImageData(Faces.Back, BitmapChanged_Callback),
                new ImageData(Faces.Left, BitmapChanged_Callback),
                new ImageData(Faces.Right, BitmapChanged_Callback),
                new ImageData(Faces.Top, BitmapChanged_Callback)
            };
        }

        private void BitmapChanged_Callback()
        {
            foreach (var imageData in ImageCollection)
            {
                if (imageData.File == null)
                {
                    IsReady = false;
                    return;
                }
            }

            IsReady = true;
        }

        private async Task ProceedReconstructionAsync()
        {
            IsLoading = true;
            var outputFolder = await KnownFolders.PicturesLibrary.CreateFolderAsync("MixBuild", CreationCollisionOption.OpenIfExists);

            // copy image to the output folder
            foreach (var imageData in ImageCollection)
            {
                var fileName = $"{((int)imageData.Face).ToString()}{imageData.File.FileType}";
                await imageData.File.CopyAsync(outputFolder, fileName, NameCollisionOption.ReplaceExisting);
            }

            // subscribe to file change
            var query = outputFolder.CreateFileQueryWithOptions(new QueryOptions { FileTypeFilter = { ".txt" } });
            query.ContentsChanged += Query_ContentsChanged;
            await query.GetFilesAsync();

            await FullTrustProcessLauncher.LaunchFullTrustProcessForCurrentAppAsync();
        }

        private async void Query_ContentsChanged(IStorageQueryResultBase sender, object args)
        {
            if (await sender.GetItemCountAsync() > 0)
            {
                await CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                async () =>
                {
                    IsLoading = false;

                    var dialog = new CompleteContentDialog();
                    var dialogResult = await dialog.ShowAsync();

                    switch (dialogResult)
                    {
                        case ContentDialogResult.Primary:
                            // Export
                            break;
                        default:
                            // Close
                            var frame = Window.Current.Content as Frame;
                            if (frame.CanGoBack) frame.GoBack();
                            break;
                    }
                });
            }
        }
    }
}

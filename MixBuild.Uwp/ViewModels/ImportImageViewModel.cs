﻿using MixBuild.Uwp.Helpers;
using MixBuild.Uwp.Models;
using MixBuild.Uwp.Views.Controls;
using Newtonsoft.Json;
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

        private StorageFileQueryResult fileQueryResult;

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
            bool hasError = false;
            var outputFolder = await KnownFolders.PicturesLibrary.CreateFolderAsync("MixBuild", CreationCollisionOption.OpenIfExists);

            int? imageHeight = null;
            int? imageWidth = null;

            // copy image to the output folder
            foreach (var imageData in ImageCollection)
            {
                if (imageHeight == null || imageWidth == null)
                {
                    imageHeight = imageData.Bitmap.PixelHeight;
                    imageWidth = imageData.Bitmap.PixelWidth;
                }
                else
                {
                    if (imageHeight != imageData.Bitmap.PixelHeight || imageWidth != imageData.Bitmap.PixelWidth)
                    {
                        hasError = true;
                    }
                }

                var fileName = $"{((int)imageData.Face).ToString()}{imageData.File.FileType}";
                await imageData.File.CopyAsync(outputFolder, fileName, NameCollisionOption.ReplaceExisting);
            }

            if (hasError)
            {
                IsLoading = false;

                var dialog = new SimpleContentDialog
                {
                    Title = "Warning",
                    Content = "All images should use the same resolution size",
                    CloseButtonText = "Okay"
                };
                await dialog.ShowAsync();
            }
            else
            {
                // create a status file
                var statusFile = await outputFolder.CreateFileAsync("status.json", CreationCollisionOption.OpenIfExists);
                await FileIO.WriteTextAsync(statusFile, JsonConvert.SerializeObject(new Status { status = false, path = "" }));

                // subscribe to file change
                if (fileQueryResult == null)
                {
                    fileQueryResult = outputFolder.CreateFileQueryWithOptions(new QueryOptions { FileTypeFilter = { ".json" } });
                }
                fileQueryResult.ContentsChanged += Query_ContentsChanged;
                await fileQueryResult.GetFilesAsync();

                await FullTrustProcessLauncher.LaunchFullTrustProcessForCurrentAppAsync();
            }
        }

        private async void Query_ContentsChanged(IStorageQueryResultBase sender, object args)
        {
            if (await sender.GetItemCountAsync() > 0)
            {
                await CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(
                    CoreDispatcherPriority.Normal,
                    async () =>
                    {
                        var outputFolder = await KnownFolders.PicturesLibrary.GetFolderAsync("MixBuild");
                        var statusFile = await outputFolder.GetFileAsync("status.json");
                        var jsonString = await FileIO.ReadTextAsync(statusFile);
                        var status = JsonConvert.DeserializeObject<Status>(jsonString);

                        if (status.status)
                        {
                            // if the dialog already opened, this will catch the error
                            try
                            {
                                IsLoading = false;

                                var dialog = new CompleteContentDialog(status.path);
                                var dialogResult = await dialog.ShowAsync();

                                if (dialogResult == Windows.UI.Xaml.Controls.ContentDialogResult.None)
                                {
                                    // unsubscribe the file change event
                                    fileQueryResult.ContentsChanged -= Query_ContentsChanged;
                                }
                            }
                            catch { }
                        }
                    }
                );
            }
        }
    }
}

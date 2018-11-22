using MixBuild.Uwp.Helpers;
using MixBuild.Uwp.Models;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.ApplicationModel;
using Windows.Storage;
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

        public ICommand ProceedReconstructionCommand => new RelayCommand<RoutedEventArgs>(async (e) => await ProceedReconstructionAsync());

        public ImportImageViewModel()
        {
            IsReady = false;
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
            var tempFolder = KnownFolders.PicturesLibrary.;

            // copy image to the temp folder
            foreach (var imageData in ImageCollection)
            {
                var fileName = $"{((int)imageData.Face).ToString()}{imageData.File.FileType}";
                await imageData.File.CopyAsync(tempFolder, fileName, NameCollisionOption.ReplaceExisting);
            }

            await FullTrustProcessLauncher.LaunchFullTrustProcessForAppAsync();
        }
    }
}

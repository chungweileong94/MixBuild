using MixBuild.Uwp.Helpers;
using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media.Imaging;

namespace MixBuild.Uwp.Models
{

    public class ImageData : INotifyPropertyChanged
    {
        public enum Faces
        {
            Front = 0,
            Right = 90,
            Back = 180,
            Left = 270,
            Top = -1
        }

        #region properties
        public Faces Face { get; }

        private StorageFile _File;
        public StorageFile File
        {
            get => _File;
            set => Set(ref _File, value);
        }

        private BitmapImage _Bitmap;
        public BitmapImage Bitmap
        {
            get => _Bitmap;
            set => Set(ref _Bitmap, value);
        }

        private bool _IsLoading;
        public bool IsLoading
        {
            get => _IsLoading;
            set => Set(ref _IsLoading, value);
        }

        public Action BitmapChangedCallback { get; set; }
        #endregion

        public ImageData(Faces face, Action bitmapChangedCallback)
        {
            Face = face;
            Bitmap = new BitmapImage();
            BitmapChangedCallback = bitmapChangedCallback;
        }

        public ICommand AddPathCommand => new RelayCommand<RoutedEventArgs>(async (e) => await AddPathAsync());

        private async Task AddPathAsync()
        {
            IsLoading = true;
            var picker = new FileOpenPicker
            {
                ViewMode = PickerViewMode.Thumbnail,
                SuggestedStartLocation = PickerLocationId.PicturesLibrary
            };
            picker.FileTypeFilter.Add(".jpg");
            picker.FileTypeFilter.Add(".jpeg");
            picker.FileTypeFilter.Add(".png");

            File = await picker.PickSingleFileAsync();

            if (File != null)
            {
                await Bitmap.SetSourceAsync(await File.OpenReadAsync());
                BitmapChangedCallback?.Invoke();
            }
            IsLoading = false;
        }

        #region INotifyProperty
        public event PropertyChangedEventHandler PropertyChanged;

        public void RaisePropertyChanged([CallerMemberName]string propertyName = null)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        public bool Set<T>(ref T storage, T value, [CallerMemberName]string propertyName = null)
        {
            if (Equals(storage, value))
                return false;
            storage = value;
            RaisePropertyChanged(propertyName);
            return true;
        }
        #endregion
    }
}

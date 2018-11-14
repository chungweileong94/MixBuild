using MixBuild.Uwp.Models;
using System.Collections.ObjectModel;
using static MixBuild.Uwp.Models.ImageData;

namespace MixBuild.Uwp.ViewModels
{
    public class ImportImageViewModel : BaseViewModel
    {
        public ObservableCollection<ImageData> ImageCollection { get; } = new ObservableCollection<ImageData>();

        public ImportImageViewModel()
        {
            ImageCollection = new ObservableCollection<ImageData>
            {
                new ImageData(Faces.Front),
                new ImageData(Faces.Back),
                new ImageData(Faces.Left),
                new ImageData(Faces.Right),
                new ImageData(Faces.Top)
            };
        }
    }
}

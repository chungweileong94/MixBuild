using MixBuild.Uwp.Models;
using System.Collections.ObjectModel;

namespace MixBuild.Uwp.ViewModels
{
    public class ImportImageViewModel : BaseViewModel
    {
        public ObservableCollection<ImageData> ImageCollection { get; } = new ObservableCollection<ImageData>();

        public ImportImageViewModel()
        {
            ImageCollection = new ObservableCollection<ImageData>
            {
                new ImageData { Label = "Front" },
                new ImageData { Label = "Back" },
                new ImageData { Label = "Left" },
                new ImageData { Label = "Right" },
                new ImageData { Label = "Top" }
            };
        }
    }
}

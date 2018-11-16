using MixBuild.Uwp.Models;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

namespace MixBuild.Uwp.Views.Controls
{
    public sealed partial class ImageImportControl : UserControl
    {
        public ImageData ImageData => DataContext as ImageData;

        public ImageImportControl()
        {
            this.InitializeComponent();
            DataContextChanged += (s, e) => Bindings.Update();
        }
    }

    #region Converters
    public class InvertBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language) => !(bool)value;

        public object ConvertBack(object value, Type targetType, object parameter, string language) => throw new NotImplementedException();
    }

    public class ObjectVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language) =>
            value != null ?
            (parameter == null ? Visibility.Visible : Visibility.Collapsed) :
            (parameter == null ? Visibility.Collapsed : Visibility.Visible);

        public object ConvertBack(object value, Type targetType, object parameter, string language) => throw new NotImplementedException();
    }

    public class ObjectOpacityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language) =>
            value != null ?
            (parameter == null ? 1.0 : 0) :
            (parameter == null ? 0 : 1.0);

        public object ConvertBack(object value, Type targetType, object parameter, string language) => throw new NotImplementedException();
    }

    public class ImportLabelConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language) =>
            value != null ?
            (parameter == null ? "Change" : "Import") :
            (parameter == null ? "Import" : "Change");

        public object ConvertBack(object value, Type targetType, object parameter, string language) => throw new NotImplementedException();
    }
    #endregion
}

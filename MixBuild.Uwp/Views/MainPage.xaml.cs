using System.Numerics;
using Windows.ApplicationModel.Core;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MixBuild.Uwp.Views
{
    public sealed partial class MainPage : Page
    {
        Compositor _compositor;
        SpringVector3NaturalMotionAnimation _springAnimation;

        public MainPage()
        {
            this.InitializeComponent();
            _compositor = Window.Current.Compositor;

            //remove titlebar
            var coreTitleBar = CoreApplication.GetCurrentView().TitleBar;
            coreTitleBar.ExtendViewIntoTitleBar = true;

            //setup title bar draggable region
            AppTitleBar.Height = coreTitleBar.Height;
            coreTitleBar.LayoutMetricsChanged += (s, args) => AppTitleBar.Height = coreTitleBar.Height;
            Window.Current.SetTitleBar(AppTitleBar);
        }

        private void LogoPanel_Loaded(object sender, RoutedEventArgs e)
        {
            var expressionAnim = _compositor.CreateExpressionAnimation();
            expressionAnim.Expression = "Lerp(0, -50, (scaleElement.Scale.Y - 1) * 2)";
            expressionAnim.Target = "Translation.Y";

            expressionAnim.SetExpressionReferenceParameter("scaleElement", GetStartedButton);
            (sender as UIElement).StartAnimation(expressionAnim);
        }

        private void GetStartedButton_Loaded(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            button.CenterPoint = new Vector3((float)button.ActualWidth / 2, (float)button.ActualHeight / 2, 0);
        }

        private void GetStartedButton_Click(object sender, RoutedEventArgs e) => Frame.Navigate(typeof(ImportImagePage));

        private void GetStartedButton_PointerEntered(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            CreateOrUpdateSpringAnimation(1.5f);
            (sender as UIElement).StartAnimation(_springAnimation);
        }

        private void GetStartedButton_PointerExited(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            CreateOrUpdateSpringAnimation(1.0f);
            (sender as UIElement).StartAnimation(_springAnimation);
        }

        private void CreateOrUpdateSpringAnimation(float finalValue)
        {
            if (_springAnimation == null)
            {
                _springAnimation = _compositor.CreateSpringVector3Animation();
                _springAnimation.Target = "Scale";
            }
            _springAnimation.FinalValue = new Vector3(finalValue);
        }
    }
}

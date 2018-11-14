using System.Numerics;
using Windows.ApplicationModel.Core;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MixBuild.Uwp.Views
{
    public sealed partial class MainPage : Page
    {
        Compositor _Compositor;
        SpringVector3NaturalMotionAnimation _SpringAnimation;

        public MainPage()
        {
            this.InitializeComponent();
            _Compositor = Window.Current.Compositor;

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
            var expressionAnim = _Compositor.CreateExpressionAnimation();
            expressionAnim.Expression = "Lerp(0, -50, (scaleElement.Scale.Y - 1) * 2)";
            expressionAnim.Target = "Translation.Y";

            expressionAnim.SetExpressionReferenceParameter("scaleElement", GetStartedButton);
            (sender as UIElement).StartAnimation(expressionAnim);
        }

        private void GetStartedButton_Loaded(object sender, RoutedEventArgs e)
        {
            var button = sender as Button;
            button.CenterPoint = new Vector3((float)button.ActualWidth / 2, (float)button.ActualHeight / 2, 0);
        }

        private void GetStartedButton_Click(object sender, RoutedEventArgs e) => Frame.Navigate(typeof(ImportImagePage));

        private void GetStartedButton_PointerEntered(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            CreateOrUpdateSpringAnimation(1.5f);
            (sender as UIElement).StartAnimation(_SpringAnimation);
        }

        private void GetStartedButton_PointerExited(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            CreateOrUpdateSpringAnimation(1.0f);
            (sender as UIElement).StartAnimation(_SpringAnimation);
        }

        private void CreateOrUpdateSpringAnimation(float finalValue)
        {
            if (_SpringAnimation == null)
            {
                _SpringAnimation = _Compositor.CreateSpringVector3Animation();
                _SpringAnimation.Target = "Scale";
            }
            _SpringAnimation.FinalValue = new Vector3(finalValue);
        }
    }
}

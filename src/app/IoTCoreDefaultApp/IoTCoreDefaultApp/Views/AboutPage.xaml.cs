using IoTCoreDefaultApp.Utils;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace IoTCoreDefaultApp.Views
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AboutPage : Page
    {
        DispatcherTimer timer;
        public AboutPage()
        {
            this.InitializeComponent();


            this.NavigationCacheMode = NavigationCacheMode.Enabled;

            this.DataContext = LanguageManager.GetInstance();

            this.Loaded += (sender, e) =>
            {
                UpdateDateTime();

                timer = new DispatcherTimer();
                timer.Tick += timer_Tick;
                timer.Interval = TimeSpan.FromSeconds(30);
                timer.Start();
            };
            this.Unloaded += (sender, e) =>
            {
                timer.Stop();
                timer = null;
            };
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            if (NetworkHelper.CheckNetworkAvailability())
            {
                webView.NavigationFailed += WebView_NavigationFailed;
                webView.NavigationCompleted += WebView_NavigationCompleted;
                webView.NavigationStarting += WebView_NavigationStarting;
                NavigateToHome();
            }
            else
            {
                webView.Visibility = Visibility.Collapsed;
                commandBar.Visibility = Visibility.Collapsed;
                offlineIntr.Visibility = Visibility.Visible;
            }
        }

        private void NavigateToHome()
        {
            var info = CultureInfo.CurrentCulture;
            Uri uri;
            if (info.TwoLetterISOLanguageName == "zh")
            {
                uri = new Uri(@"http://www.allwinnertech.com/index.html#zh");
            }
            else
            {
                uri = new Uri(@"http://www.allwinnertech.com/en/");
            }
            webView.Navigate(uri);

        }

        private void Refresh()
        {
            webView.Refresh();
        }

        private void Back()
        {
            if (webView.CanGoBack)
            {
                webView.GoBack();
            }
        }

        private void WebView_NavigationStarting(WebView sender, WebViewNavigationStartingEventArgs args)
        {
            refreshButton.IsEnabled = false;
            backButton.IsEnabled = false;
            homeButton.IsEnabled = false;
        }

        private void WebView_NavigationCompleted(WebView sender, WebViewNavigationCompletedEventArgs args)
        {
            refreshButton.IsEnabled = true;
            backButton.IsEnabled = true;
            homeButton.IsEnabled = true;
        }

        private void WebView_NavigationFailed(object sender, WebViewNavigationFailedEventArgs e)
        {
            refreshButton.IsEnabled = true;
            backButton.IsEnabled = true;
            homeButton.IsEnabled = true;
        }

        private void DeviceInfo_Clicked(object sender, RoutedEventArgs e)
        {
            NavigationUtils.NavigateToScreen(typeof(MainPage));
        }

        private void Tutorials_Clicked(object sender, RoutedEventArgs e)
        {
            NavigationUtils.NavigateToScreen(typeof(TutorialMainPage));
        }

        private void UpdateDateTime()
        {
            var t = DateTime.Now;
            this.CurrentTime.Text = t.ToString("t", CultureInfo.CurrentCulture) + Environment.NewLine + t.ToString("d", CultureInfo.CurrentCulture);
        }

        private void timer_Tick(object sender, object e)
        {
            UpdateDateTime();
        }

        private void SettingsButton_Clicked(object sender, RoutedEventArgs e)
        {
            NavigationUtils.NavigateToScreen(typeof(Settings));
        }

        private void ShutdownButton_Clicked(object sender, RoutedEventArgs e)
        {
            ShutdownDropdown.IsOpen = true;
        }

        private void ShutdownDropdown_Opened(object sender, object e)
        {
            var w = ShutdownListView.ActualWidth;
            if (w == 0)
            {
                // trick to recalculate the size of the dropdown
                ShutdownDropdown.IsOpen = false;
                ShutdownDropdown.IsOpen = true;
            }
            var offset = -(ShutdownListView.ActualWidth - ShutdownButton.ActualWidth);
            ShutdownDropdown.HorizontalOffset = offset;
        }

        private void ShutdownHelper(ShutdownKind kind)
        {
            ShutdownManager.BeginShutdown(kind, TimeSpan.FromSeconds(0.5));
        }

        private void ShutdownListView_ItemClick(object sender, ItemClickEventArgs e)
        {
            var item = e.ClickedItem as FrameworkElement;
            if (item == null)
            {
                return;
            }
            switch (item.Name)
            {
                case "ShutdownOption":
                    ShutdownHelper(ShutdownKind.Shutdown);
                    break;
                case "RestartOption":
                    ShutdownHelper(ShutdownKind.Restart);
                    break;
            }
        }
    }
}

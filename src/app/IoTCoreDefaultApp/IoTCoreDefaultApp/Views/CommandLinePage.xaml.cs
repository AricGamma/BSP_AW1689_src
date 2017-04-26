using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading.Tasks;
using Windows.ApplicationModel.Resources;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Security.Cryptography;
using Windows.Storage.Streams;
using Windows.System;
using Windows.UI.Core;
using Windows.UI.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Web.Http;
using Windows.Web.Http.Headers;

// “空白页”项模板在 http://go.microsoft.com/fwlink/?LinkId=234238 上有介绍

namespace IoTCoreDefaultApp.Views
{
    enum CdResult
    {
        Succeed, AlreadyRoot, NotCdCommand,
        DirNotFound,
        InvalidPath
    }
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class CommandLinePage : Page
    {

        private const string CommandLineProcesserExe = "c:\\windows\\system32\\cmd.exe";
        private const string EnableCommandLineProcesserRegCommand = "reg ADD \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\EmbeddedMode\\ProcessLauncher\" /v AllowedExecutableFilesList /t REG_MULTI_SZ /d \"c:\\windows\\system32\\cmd.exe\\0\"";
        private const uint BufSize = 8192;

        private string currentDirectory = "C:\\";
        private List<string> commandLineHistory = new List<string>();
        private int currentCommandLine = -1;
        private ResourceLoader resourceLoader = new Windows.ApplicationModel.Resources.ResourceLoader();

        public CommandLinePage()
        {
            this.InitializeComponent();
            this.DataContext = LanguageManager.GetInstance();
            CommandLine.PlaceholderText = String.Format(resourceLoader.GetString("CommandLinePlaceholderText"), currentDirectory);
        }

        protected async override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            await Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                () => CommandLine.Focus(FocusState.Keyboard));
        }

        private void BackButton_Clicked(object sender, RoutedEventArgs e)
        {
            NavigationUtils.GoBack();
        }

        private async Task RunProcess()
        {
            if (string.IsNullOrWhiteSpace(CommandLine.Text))
            {
                return;
            }

            commandLineHistory.Add(CommandLine.Text);
            currentCommandLine = commandLineHistory.Count;

            var curDir = currentDirectory.Length > 3 ? currentDirectory.Substring(0, currentDirectory.Length - 1) : currentDirectory;

            bool isCmdAuthorized = true;
            Run cmdLineRun = new Run();
            cmdLineRun.Foreground = new SolidColorBrush(Windows.UI.Colors.LightGray);
            cmdLineRun.FontWeight = FontWeights.Bold;
            cmdLineRun.Text = curDir + "> " + CommandLine.Text + "\n";

            Run stdOutRun = new Run();
            Run stdErrRun = new Run();
            stdErrRun.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);

            var commandLineText = CommandLine.Text.Trim();

            if (commandLineText.Equals("cls", StringComparison.CurrentCultureIgnoreCase))
            {
                StdOutputText.Blocks.Clear();
                return;
            }
            else if (commandLineText.StartsWith("cd ", StringComparison.CurrentCultureIgnoreCase) || commandLineText.Equals("cd", StringComparison.CurrentCultureIgnoreCase))
            {
                var cdRes = await ExecuteCdCmd(commandLineText);
                switch (cdRes)
                {
                    case CdResult.Succeed:
                        curDir = currentDirectory.Length > 3 ? currentDirectory.Substring(0, currentDirectory.Length - 1) : currentDirectory;
                        CommandLine.PlaceholderText = String.Format(resourceLoader.GetString("CommandLinePlaceholderText") + "\n", curDir);
                        stdOutRun.Text = String.Format(resourceLoader.GetString("CurrentDirectory") + "\n", curDir);
                        break;
                    case CdResult.InvalidPath:
                        stdErrRun.Text = resourceLoader.GetString("InvalidPath");
                        break;
                    case CdResult.NotCdCommand:
                        break;
                    case CdResult.DirNotFound:
                        stdErrRun.Text = resourceLoader.GetString("CdDirNotFound");
                        break;
                }
                //stdErrRun.Text = resourceLoader.GetString("CdNotSupported");
            }
            else if (commandLineText.Equals("exit", StringComparison.CurrentCultureIgnoreCase))
            {
                NavigationUtils.GoBack();
            }
            else
            {
                var standardOutput = new InMemoryRandomAccessStream();
                var standardError = new InMemoryRandomAccessStream();
                var options = new ProcessLauncherOptions
                {
                    StandardOutput = standardOutput,
                    StandardError = standardError,
                    WorkingDirectory = currentDirectory
                };

                try
                {
                    //var args = "/C \"cd \"" + currentDirectory + "\" & " + commandLineText + "\"";
                    var args = "/C \"" + commandLineText + "\"";
                    var result = await ProcessLauncher.RunToCompletionAsync(CommandLineProcesserExe, args, options);

                    // First write std out
                    using (var outStreamRedirect = standardOutput.GetInputStreamAt(0))
                    {
                        using (var dataReader = new DataReader(outStreamRedirect))
                        {
                            await ReadText(dataReader, stdOutRun);
                        }
                    }

                    // Then write std err
                    using (var errStreamRedirect = standardError.GetInputStreamAt(0))
                    {
                        using (var dataReader = new DataReader(errStreamRedirect))
                        {
                            await ReadText(dataReader, stdErrRun);
                        }
                    }
                }
                catch (UnauthorizedAccessException uex)
                {
                    isCmdAuthorized = false;
                    var errorMessage = uex.Message + "\n\n" + resourceLoader.GetString("CmdNotEnabled");
                    stdErrRun.Text = errorMessage;
                }
                catch (Exception ex)
                {
                    var errorMessage = ex.Message + "\n" + ex.StackTrace + "\n";
                    stdErrRun.Text = errorMessage;
                }
            }

            await CoreWindow.GetForCurrentThread().Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                Paragraph paragraph = new Paragraph();

                paragraph.Inlines.Add(cmdLineRun);
                paragraph.Inlines.Add(stdOutRun);
                paragraph.Inlines.Add(stdErrRun);

                if (!isCmdAuthorized)
                {
                    InlineUIContainer uiContainer = new InlineUIContainer();
                    Button cmdEnableButton = new Button();
                    cmdEnableButton.Content = resourceLoader.GetString("EnableCmdText");
                    cmdEnableButton.Click += AccessButtonClicked;
                    uiContainer.Child = cmdEnableButton;
                    paragraph.Inlines.Add(uiContainer);
                }

                StdOutputText.Blocks.Add(paragraph);
            });
        }

        private void AccessButtonClicked(object sender, RoutedEventArgs e)
        {
            CoreWindow currentWindow = Window.Current.CoreWindow;
            EnableCmdPopup.VerticalOffset = (currentWindow.Bounds.Height / 2) - (EnableCmdStackPanel.Height / 2);
            EnableCmdPopup.HorizontalOffset = (currentWindow.Bounds.Width / 2) - (EnableCmdStackPanel.Width / 2);
            EnableCmdPopup.IsOpen = true;
            Password.Focus(FocusState.Keyboard);
        }

        private async Task ReadText(DataReader dataReader, Run run)
        {
            StringBuilder textOutput = new StringBuilder((int)BufSize);
            uint bytesLoaded = 0;
            while ((bytesLoaded = await dataReader.LoadAsync(BufSize)) > 0)
            {
                textOutput.Append(dataReader.ReadString(bytesLoaded));
            }

            new System.Threading.ManualResetEvent(false).WaitOne(10);
            if ((bytesLoaded = await dataReader.LoadAsync(BufSize)) > 0)
            {
                textOutput.Append(dataReader.ReadString(bytesLoaded));
            }

            run.Text = textOutput.ToString();
        }

        private async void CommandLine_KeyUp(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == VirtualKey.Enter)
            {
                await DoRunCommand(true);
            }
            else if (e.Key == VirtualKey.Up)
            {
                currentCommandLine = Math.Max(0, currentCommandLine - 1);
                if (currentCommandLine < commandLineHistory.Count)
                {
                    UpdateCommandLineFromHistory();
                }
            }
            else if (e.Key == VirtualKey.Down)
            {
                currentCommandLine = Math.Min(commandLineHistory.Count, currentCommandLine + 1);
                if (currentCommandLine < commandLineHistory.Count && currentCommandLine >= 0)
                {
                    UpdateCommandLineFromHistory();
                }
                else
                {
                    CommandLine.Text = string.Empty;
                }
            }
        }

        private void UpdateCommandLineFromHistory()
        {
            CommandLine.Text = commandLineHistory[currentCommandLine];
            if (CommandLine.Text.Length > 0)
            {
                CommandLine.SelectionStart = CommandLine.Text.Length;
                CommandLine.SelectionLength = 0;
            }
        }

        private async void RunButton_Click(object sender, RoutedEventArgs e)
        {
            await DoRunCommand(false);
        }

        private async Task DoRunCommand(bool isFocus)
        {
            await Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                async () =>
                {
                    RunButton.IsEnabled = false;
                    CommandLine.IsEnabled = false;
                    ClearButton.IsEnabled = false;

                    await RunProcess();
                    CommandLine.Text = string.Empty;

                    RunButton.IsEnabled = true;
                    CommandLine.IsEnabled = true;
                    ClearButton.IsEnabled = true;
                    if (isFocus)
                    {
                        CommandLine.Focus(FocusState.Keyboard);
                    }
                });
        }

        private void StdOutputText_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            StdOutputScroller.ChangeView(null, StdOutputScroller.ScrollableHeight, null);
        }

        private void ClearButton_Click(object sender, RoutedEventArgs e)
        {
            StdOutputText.Blocks.Clear();
        }

        private async void EnableCmdLineButton_Click(object sender, RoutedEventArgs e)
        {
            if (Password.Password.Trim().Equals(string.Empty))
            {
                // Empty password not accepted
                return;
            }

            try
            {
                var response = await EnableCmdExe("127.0.0.1", Username.Text, Password.Password, EnableCommandLineProcesserRegCommand);
                if (response.IsSuccessStatusCode)
                {
                    CmdEnabledStatus.Text = resourceLoader.GetString("CmdTextEnabledSuccess");
                }
                else
                {
                    CmdEnabledStatus.Text = string.Format(resourceLoader.GetString("CmdTextEnabledFailure"), response.StatusCode);
                }
            }
            catch (Exception cmdEnabledException)
            {
                CmdEnabledStatus.Text = string.Format(resourceLoader.GetString("CmdTextEnabledFailure"), cmdEnabledException.HResult);
            }

            EnableCmdPopup.IsOpen = false;

            CoreWindow currentWindow = Window.Current.CoreWindow;
            CmdEnabledStatusPopup.VerticalOffset = (currentWindow.Bounds.Height / 2) - (StatusStackPanel.Height / 2);
            CmdEnabledStatusPopup.HorizontalOffset = (currentWindow.Bounds.Width / 2) - (StatusStackPanel.Width / 2);

            CmdEnabledStatusPopup.IsOpen = true;
        }

        private static async Task<HttpResponseMessage> EnableCmdExe(string ipAddress, string username, string password, string runCommand)
        {
            HttpClient client = new HttpClient();
            var command = CryptographicBuffer.ConvertStringToBinary(runCommand, BinaryStringEncoding.Utf8);
            var runAsdefault = CryptographicBuffer.ConvertStringToBinary("false", BinaryStringEncoding.Utf8);

            var urlContent = new HttpFormUrlEncodedContent(new[]
            {
                new KeyValuePair<string,string>("command", CryptographicBuffer.EncodeToBase64String(command)),
                new KeyValuePair<string,string>("runasdefaultaccount", CryptographicBuffer.EncodeToBase64String(runAsdefault)),
            });

            Uri uri = new Uri("http://" + ipAddress + ":8080/api/iot/processmanagement/runcommand?" + await urlContent.ReadAsStringAsync());

            var authBuffer = CryptographicBuffer.ConvertStringToBinary(username + ":" + password, BinaryStringEncoding.Utf8);
            client.DefaultRequestHeaders.Authorization = new HttpCredentialsHeaderValue("Basic", CryptographicBuffer.EncodeToBase64String(authBuffer));

            HttpResponseMessage response = await client.PostAsync(uri, null);
            return response;
        }

        private void CloseStatusButton_Click(object sender, RoutedEventArgs e)
        {
            CmdEnabledStatusPopup.IsOpen = false;
            CommandLine.Focus(FocusState.Keyboard);
        }

        private async Task<CdResult> ExecuteCdCmd(string strCmd)
        {

            var curDir = currentDirectory;
            if (strCmd.StartsWith("cd ", StringComparison.CurrentCultureIgnoreCase))
            {
                try
                {
                    var args = strCmd.Substring(3).Trim();

                    if (string.IsNullOrEmpty(args) || string.IsNullOrEmpty(args.Replace('\\', ' ').Replace('/', ' ').Trim()))
                    {
                        currentDirectory = "C:\\";
                        return CdResult.Succeed;
                    }

                    if (args.Contains("*"))
                    {
                        return CdResult.InvalidPath;
                    }

                    var cmdout = new InMemoryRandomAccessStream();
                    var cmderr = new InMemoryRandomAccessStream();

                    if (args.StartsWith("C:\\", StringComparison.CurrentCultureIgnoreCase)
                        || args.StartsWith("C:/", StringComparison.CurrentCultureIgnoreCase))
                    {
                        curDir = "C:\\";
                        args = args.Substring(3);
                    }

                    ProcessLauncherOptions options = new ProcessLauncherOptions()
                    {
                        StandardOutput = cmdout,
                        StandardError = cmderr,
                        WorkingDirectory = curDir
                    };

                    foreach (var item in args.Split('\\', '/'))
                    {
                        var dirStr = item.Trim();
                        if (dirStr.Equals(".."))
                        {
                            if (!curDir.Equals("C:\\", StringComparison.CurrentCultureIgnoreCase))
                            {
                                curDir = curDir.Substring(0, curDir.LastIndexOf('\\'));
                                curDir = curDir.Substring(0, curDir.LastIndexOf('\\') + 1);
                                options.WorkingDirectory = curDir;
                            }
                        }
                        else if (string.IsNullOrEmpty(dirStr) || dirStr.Equals("."))
                        {
                            continue;
                        }
                        else
                        {
                            string dir = "/C \"cd \"" + curDir + "\" & " + "cd \"" + dirStr + "\"\"";
                            var result = await ProcessLauncher.RunToCompletionAsync(CommandLineProcesserExe, dir, options);
                            if (result.ExitCode == 0)
                            {
                                curDir += dirStr + "\\";
                                options.WorkingDirectory = curDir;
                            }
                            else
                                return CdResult.DirNotFound;
                        }
                    }
                    currentDirectory = curDir;
                    return CdResult.Succeed;

                }
                catch (Exception ex)
                {
                    throw ex;
                }
            }
            else if (strCmd.Equals("cd", StringComparison.CurrentCultureIgnoreCase))
            {
                currentDirectory = "C:\\";
                return CdResult.Succeed;
            }

            return CdResult.NotCdCommand;
        }
    }
}

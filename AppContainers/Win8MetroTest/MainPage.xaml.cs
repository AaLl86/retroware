/*
 *  Copyright (C) 2013 Saferbytes Srl - Andrea Allievi
 *	Filename: MainPage.xaml.cs
 *	Implements a new Metro Style stupid test app
 *	Last revision: 05/30/2013
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Networking;
using Windows.Storage.Streams;
using Windows.Networking.Sockets;
using Windows.Storage;
using Windows.UI.Popups;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace MetroTest1
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
        }


        private async void Button_Click_1(object sender, RoutedEventArgs e)
        {
            Windows.Networking.Sockets.StreamSocket streamSocket = new Windows.Networking.Sockets.StreamSocket();
            HostName localHost = new HostName("localhost");
            HostName remoteHost = new HostName("www.andrea-allievi.com");
            EndpointPair ep = new EndpointPair(localHost, "", remoteHost, "80");
            MessageDialog dlg = new Windows.UI.Popups.MessageDialog("");

            // Save the socket, so subsequent steps can use it. 
            Windows.ApplicationModel.Core.CoreApplication.Properties.Add("clientSocket", streamSocket); 

            try
            {
                await streamSocket.ConnectAsync(remoteHost, "80");
            }
            catch (Exception exc)
            {
                // If this is an unknown status it means that the error is fatal and retry will likely fail. 
                if (SocketError.GetStatus(exc.HResult) == SocketErrorStatus.Unknown)
                {
                    throw;
                }

                dlg.Title = "Socket Error!";
                dlg.Content = "Connect failed with error: " + exc.Message;
                dlg.ShowAsync();
                return;
            }

            DataWriter writer = new DataWriter(streamSocket.OutputStream);
            writer.WriteString("GET /index.html\r\n");
            await writer.StoreAsync();

            DataReader reader = new DataReader(streamSocket.InputStream);
            uint len = 2048;
            reader.InputStreamOptions = InputStreamOptions.Partial;
            uint numStrBytes = await reader.LoadAsync(len);
            string data = reader.ReadString(numStrBytes);

            dlg.Title = "Received data";
            dlg.Content = data;
            await dlg.ShowAsync();
        }

        private void cmdExit_Click(object sender, RoutedEventArgs e)
        {
            Application.Current.Exit();
        }

        private async void BtnFile_Click(object sender, RoutedEventArgs e)
        {
            StorageFolder sf = KnownFolders.DocumentsLibrary;
            StorageFile file = await sf.CreateFileAsync("WiFiTemp.txt", CreationCollisionOption.OpenIfExists);
            string text = await Windows.Storage.FileIO.ReadTextAsync(file);

            MessageDialog dlg = new Windows.UI.Popups.MessageDialog("");
            dlg.Title = "Received data";
            dlg.Content = text;
            await dlg.ShowAsync();



        }
    }
}

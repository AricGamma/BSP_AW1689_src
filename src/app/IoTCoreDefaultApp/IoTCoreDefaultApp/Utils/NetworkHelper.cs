using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Networking.Connectivity;

namespace IoTCoreDefaultApp.Utils
{
    public class NetworkHelper
    {
        public static bool CheckNetworkAvailability()
        {
            var profile = NetworkInformation.GetInternetConnectionProfile();
            if (profile != null)
            {
                var level = profile.GetNetworkConnectivityLevel();
                switch (level)
                {
                    case NetworkConnectivityLevel.InternetAccess:
                        return true;
                    default:
                        return false;
                }
            }
            return false;
        }
    }
}

#include <lwip/inet.h>

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x "

typedef enum HALWifiStatus {
	WIFI_UNDEFINED,
	WIFI_STA_CONNECTING,
	WIFI_STA_DISCONNECTED,
	WIFI_STA_AUTH_FAILED,
	WIFI_STA_CONNECTED,
	WIFI_AP_CONNECTED,
	WIFI_AP_FAILED,
} HALWifiStatus_t;


int HAL_SetupWiFiOpenAccessPoint(const char* ssid);
void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, char dhcp_mode, in_addr_t ip, in_addr_t netmask, in_addr_t gw, in_addr_t dns);
void HAL_DisconnectFromWifi();
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code));
// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString();
const char* HAL_GetMACStr(char* macstr);
void WiFI_GetMacAddress(char* mac);
int WiFI_SetMacAddress(char* mac);
void HAL_PrintNetworkInfo();
int HAL_GetWifiStrength();


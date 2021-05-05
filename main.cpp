#include "../pinc.h"
#include "whitelist.h"

#include <string>
#include <vector>
#include <chrono>

cvar_t *vpnEmail;
cvar_t *vpnFlags;
cvar_t *vpnMsg;
cvar_t *vpnThreshold;

const int cmdPower = 100;
const std::string apiUrl("http://check.getipintel.net/check.php?ip=");
std::string apiUrlParams;

PCL int OnInit(){ //Function executed after the plugin is loaded on the server.
	vpnEmail = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_email", "", 0, "Email address to be used with IP Intel API (https://getipintel.net/)I");
	vpnFlags = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_flag", "m", 0, "Flag to be used with IP Intel API (https://getipintel.net/)");
	vpnMsg = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_kick_msg", "Usage of a VPN or Proxy is not allowed on this server!", 0, "The message to be shown to the user when they are denied access to the server");
	vpnThreshold = (cvar_t*)Plugin_Cvar_RegisterFloat("vpn_blocker_threshold", 0.99f, 0.0f, 1.0f, 0, "Threshold value of when to kick a player based on the probability of using a VPN or Proxy (0.99+ is recommended)");
	Whitelist::SetEnabled(Plugin_Cvar_RegisterBool("vpn_blocker_whitelist", qtrue, 0, "Enable or disable the use of the whitelist"));
	Whitelist::SetFile(Plugin_Cvar_RegisterString("vpn_blocker_whitelist_file", "vpn_whitelist.dat", CVAR_INIT, "Name of file which holds the whitelist"));

	Plugin_AddCommand("vpn_whitelist_add", Whitelist::CommandHandler, cmdPower);
	Plugin_AddCommand("vpn_whitelist_remove", Whitelist::CommandHandler, cmdPower);

	vpnThreshold->fmin = 0.0f;
	vpnThreshold->fmax = 1.0f;

	if (!vpnEmail->string[0]) {
		Plugin_PrintError("Init failure. Cvar vpn_blocker_email is not set\n");
		return -1;
	}

	if (!vpnFlags->string[0]) {
		Plugin_PrintError("Init failure. Cvar vpn_blocker_flag is not set\n");
		return -1;
	} else if (vpnFlags->string[1] || (vpnFlags->string[0] != 'm' && vpnFlags->string[0] != 'b')) {
		Plugin_PrintError("Init failure. Cvar vpn_blocker_flag is invalid (use m or b)\n");
		return -1;
	}

	if (!vpnMsg->string[0]) {
		Plugin_PrintError("Init failure. Cvar vpn_blocker_kick_msg is not set\n");
		return -1;
	}

	if (vpnThreshold->value < vpnThreshold->fmin || vpnThreshold->value > vpnThreshold->fmax) {
		Plugin_PrintError("Init failure. Cvar vpn_blocker_threshold must be between %f and %f\n", vpnThreshold->fmin, vpnThreshold->fmax);
		return -1;
	}

	if (!Whitelist::IsFileSet()) {
		Plugin_PrintError("Init failure. Cvar vpn_blocker_whitelist_file is not set\n");
		return -1;
	}

	std::string vpnEmailStr(vpnEmail->string);
	std::string vpnFlagStr(vpnFlags->string);

	apiUrlParams = "&contact=" + vpnEmailStr + "&flags=" + vpnFlagStr;

	if (Whitelist::IsEnabled())
		Whitelist::Load();

	return 0;
}

PCL void OnPlayerGetBanStatus(baninfo_t* baninfo, char* message, int len) {
	if (message[0])
		return;

	if (baninfo->adr.type == NA_BOT)
		return;
	else if (baninfo->adr.type == NA_BAD) {
		std::string badAddressMsg = "Got a bad address when connecting, please try again!";
		strncpy(message, badAddressMsg.c_str(), badAddressMsg.length() + 1);
		return;
	}

	if (Whitelist::IsEnabled()) {
		if (Whitelist::IsWhitelisted(baninfo->playerid))
			return;
	}

	char address[128];

	Plugin_NET_AdrToStringShortMT(&baninfo->adr, address, sizeof(address));

	std::string ip(address);
	std::string url(apiUrl + ip + apiUrlParams);

	//Make GET Request to API
	char data[8192];
	int contentLength;
	int resCode;

	ftRequest_t* request = Plugin_HTTP_GET(url.c_str());

	if (request == NULL) {
		Plugin_Printf("[VPN BLOCKER] Request was NULL\n");
		return;
	}

	if (request->code != 200 && request->contentLength <= 0) {
		Plugin_Printf("[VPN BLOCKER] Request failed with code: %d\n", request->code);
		return;
	}

	//Get response
	contentLength = request->contentLength;
	resCode = request->code;
	memcpy(data, request->extrecvmsg->data + request->headerLength, contentLength);
	data[contentLength] = 0;

	//Free request
	Plugin_HTTP_FreeObj(request);

	//Get string response
	std::string result(data);
	float probability = 2;

	try {
		probability = std::stof(result);
	} catch(...) {
		Plugin_Printf("[VPN BLOCKER] Failed to convert request result\n");
	}

	if (probability == 2)
		return;

	if (probability < 0) {
		Plugin_PrintWarning("[VPN BLOCKER] Got error code: %f, check https://getipintel.net/\n", probability);
		return;
	}

	if (probability >= vpnThreshold->value) {
		int percentage = (int)(probability * 100);
		char format[2048];
		char tempId[128];

		Plugin_SteamIDToString(baninfo->playerid, tempId, sizeof(tempId));
		snprintf(format, strlen(vpnMsg->string) + strlen(tempId) + 24, "%s [%i/100] [ID: %s]", vpnMsg->string, percentage, tempId);
		strncpy(message, format, strlen(format) + 1);
		Plugin_Printf("[VPN BLOCKER] Removed %s [%llu] %i%c\n", baninfo->playername, baninfo->playerid, percentage, '%');
	}

	return;
}

PCL void OnInfoRequest(pluginInfo_t *info) { //Function used to obtain information about the plugin
	info->handlerVersion.major = PLUGIN_HANDLER_VERSION_MAJOR;
	info->handlerVersion.minor = PLUGIN_HANDLER_VERSION_MINOR;

	info->pluginVersion.major = 0;
	info->pluginVersion.minor = 2; //Plugin version

	strncpy(info->fullName, "VPN Blocker", sizeof(info->fullName)); //Full plugin name
	strncpy(info->shortDescription, "Prevent VPN usage by player", sizeof(info->shortDescription)); //Short plugin description
	strncpy(info->longDescription, "Prevent players joining that are using a VPN by using the IP Intelligence API", sizeof(info->longDescription)); //Long plugin description
}
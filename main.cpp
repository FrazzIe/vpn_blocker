#include "../pinc.h"

#include <string>
#include <vector>
#include <chrono>
#include <unordered_set>

cvar_t *vpnEmail;
cvar_t *vpnFlags;
cvar_t *vpnMsg;
cvar_t *vpnThreshold;
cvar_t *vpnWhitelist;
cvar_t *vpnWhitelistFile;

const std::string apiUrl("http://check.getipintel.net/check.php?ip=");
std::string apiUrlParams;
bool apiLimitReached = false;
std::int64_t apiCooldown = 3600000; //1 hour
std::int64_t apiCooldownEnd;

std::unordered_set<std::uint64_t> whitelistSet;

std::unordered_set<std::uint64_t> LoadWhitelist() {
	std::unordered_set<std::uint64_t> idSet = {};
	fileHandle_t fileHandle;
	long fileLength;

	fileLength = Plugin_FS_SV_FOpenFileRead(vpnWhitelistFile->string, &fileHandle);

	if (fileHandle == 0) {
		Plugin_Printf("[VPN BLOCKER] Couldn't open %s, does it exist?\n", vpnWhitelistFile->string);
		Plugin_Cvar_SetBool(vpnWhitelist, qfalse);
		Plugin_FS_FCloseFile(fileHandle);
		return idSet;
	}

	if (fileLength < 1) {
		Plugin_Printf("[VPN BLOCKER] Couldn't open %s because it's empty!\n", vpnWhitelistFile->string);
		Plugin_Cvar_SetBool(vpnWhitelist, qfalse);
		Plugin_FS_FCloseFile(fileHandle);
		return idSet;
	}

	char buf[256];
	int line;
	int count = 0;
	buf[0] = 0;

	while (true) {
		line = Plugin_FS_ReadLine(buf, sizeof(buf), fileHandle);

		if (line == 0) {
			Plugin_Printf("[VPN BLOCKER] %i lines parsed from %s\n", count, vpnWhitelistFile->string);
			break;
		} else if(line == -1) {
			Plugin_Printf("[VPN BLOCKER] Could not read from %s\n", vpnWhitelistFile->string);
			break;
		}

		//Plugin_Printf("[VPN BLOCKER] line %i contains %s\n", count, buf);

		std::uint64_t id = Plugin_StringToSteamID(buf);

		//Plugin_Printf("[VPN BLOCKER] line %i contains %llu\n", count, id);

		if (id !== 0) {
			idSet.insert(id);
			count++;
		}
	}

	Plugin_FS_FCloseFile(fileHandle);
	return idSet;
}

bool IsWhitelisted(std::int64_t id) {
	return whitelistSet.find(id) != whitelistSet.end();
}

PCL int OnInit(){ //Function executed after the plugin is loaded on the server.
	vpnEmail = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_email", "", 0, "Email address to be used with IP Intel API (https://getipintel.net/)I");
	vpnFlags = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_flag", "m", 0, "Flag to be used with IP Intel API (https://getipintel.net/)");
	vpnMsg = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_kick_msg", "Usage of a VPN or Proxy is not allowed on this server!", 0, "The message to be shown to the user when they are denied access to the server");
	vpnThreshold = (cvar_t*)Plugin_Cvar_RegisterFloat("vpn_blocker_threshold", 0.99f, 0.0f, 1.0f, 0, "Threshold value of when to kick a player based on the probability of using a VPN or Proxy (0.99+ is recommended)");
	vpnWhitelist = (cvar_t*)Plugin_Cvar_RegisterBool("vpn_blocker_whitelist", qtrue, 0, "Enable or disable the use of the whitelist");
	vpnWhitelistFile = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_whitelist_file", "vpn_whitelist.dat", CVAR_INIT, "Name of file which holds the whitelist");

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

	std::string vpnEmailStr(vpnEmail->string);
	std::string vpnFlagStr(vpnFlags->string);

	apiUrlParams = "&contact=" + vpnEmailStr + "&flags=" + vpnFlagStr;

	qboolean whitelistEnabled = Plugin_Cvar_GetBoolean(vpnWhitelist);

	if (whitelistEnabled == qtrue)
		whitelistSet = LoadWhitelist();

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

	qboolean whitelistEnabled = Plugin_Cvar_GetBoolean(vpnWhitelist);

	if (whitelistEnabled == qtrue) {
		if (IsWhitelisted(baninfo->playerid))
			return;
	}

	if (apiLimitReached) {
		std::int64_t epochTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (epochTime > apiCooldownEnd)
			apiLimitReached = false;
		else
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
		Plugin_Printf("[VPN BLOCKER] Got error code: %f, check https://getipintel.net/\n", probability);

		if (resCode == 429) {
			apiLimitReached = true;
			std::int64_t epochTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			apiCooldownEnd = epochTime + apiCooldown;
		}

		return;
	}

	if (probability >= vpnThreshold->value) {
		int percentage = (int)(probability * 100);
		char format[2048];

		snprintf(format, strlen(vpnMsg->string) + 12, "%s [%i/100]", vpnMsg->string, percentage);
		strncpy(message, format, strlen(format) + 1);
	}

	return;
}

PCL void OnInfoRequest(pluginInfo_t *info) { //Function used to obtain information about the plugin
	info->handlerVersion.major = PLUGIN_HANDLER_VERSION_MAJOR;
	info->handlerVersion.minor = PLUGIN_HANDLER_VERSION_MINOR;

	info->pluginVersion.major = 0;
	info->pluginVersion.minor = 1; //Plugin version

	strncpy(info->fullName, "VPN Blocker", sizeof(info->fullName)); //Full plugin name
	strncpy(info->shortDescription, "Prevent VPN usage by player", sizeof(info->shortDescription)); //Short plugin description
	strncpy(info->longDescription, "Prevent players joining that are using a VPN by using the IP Intelligence API", sizeof(info->longDescription)); //Long plugin description
}
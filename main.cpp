#include "../pinc.h"

#include <string>
#include <vector>

cvar_t *vpnEmail;
cvar_t *vpnFlags;
const std::string apiUrl("http://check.getipintel.net/check.php?ip=");
std::string apiUrlParams;

//Function to split string `str` using a given delimiter
std::vector<std::string> Split(const std::string &str, char delim) {
	int i = 0;
	std::vector<std::string> list;

	size_t pos = str.find(delim);

	while (pos != std::string::npos)
	{
		list.push_back(str.substr(i, pos - i));
		i = ++pos;
		pos = str.find(delim, pos);
	}

	list.push_back(str.substr(i, str.length()));

	return list;
}

PCL int OnInit(){ //Function executed after the plugin is loaded on the server.
	vpnEmail = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_email", "", 0, "");
	vpnFlags = (cvar_t*)Plugin_Cvar_RegisterString("vpn_blocker_flag", "m", 0, "");

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

	std::string vpnEmailStr(vpnEmail->string);
	std::string vpnFlagStr(vpnFlags->string);

	apiUrlParams = "&contact=" + vpnEmailStr + "&flags=" + vpnFlagStr;

	return 0;
}

PCL void OnPlayerConnect(int clientnum, netadr_t* netaddress, char* pbguid, char* userinfo, int authstatus, char* deniedmsg,  int deniedmsgbufmaxlen) {
	char address[128];

	Plugin_NET_AdrToStringMT(netaddress, address, sizeof(address));

	std::string ip(address);
	std::vector<std::string> ipData = Split(ip, ':');

	if (ipData.empty())
		return;

	std::string url(apiUrl + ipData.front() + apiUrlParams);

	//Make GET Request to API
	char data[8192];
	int contentLength;
	int resCode;

	ftRequest_t* request = Plugin_HTTP_GET(url.c_str());

	if (request == NULL) {
		Plugin_Printf("[VPN BLOCKER] Request was NULL\n");
		return;
	}

	if (request->code != 200  && request->contentLength <= 0) {
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
	float probability = NULL;

	try {
		probability = std::stof(result);
	} catch(std::invalid_argument& e) {
		Plugin_Printf("[VPN BLOCKER] Failed to convert request result");
	}

	if (probability == NULL)
		return;
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
#include "../pinc.h"
#include "whitelist.h"
#include "ipintel.h"

#include <string>
#include <stdexcept>

PCL int OnInit(){ //Function executed after the plugin is loaded on the server.
	try {
		IPIntel::SetEmail(Plugin_Cvar_RegisterString("vpn_blocker_email", "", 0, "Email address to be used with IP Intel API (https://getipintel.net/)"));
		IPIntel::SetFlag(Plugin_Cvar_RegisterString("vpn_blocker_flag", "m", 0, "Flag to be used with IP Intel API (https://getipintel.net/)"));
		IPIntel::SetKickMessage(Plugin_Cvar_RegisterString("vpn_blocker_kick_msg", "Usage of a VPN or Proxy is not allowed on this server!", 0, "The message to be shown to the user when they are denied access to the server"));
		IPIntel::SetThreshold(Plugin_Cvar_RegisterFloat("vpn_blocker_threshold", 0.99f, 0.0f, 1.0f, 0, "Threshold value of when to kick a player based on the probability of using a VPN or Proxy (0.99+ is recommended)"));
		Whitelist::SetEnabled(Plugin_Cvar_RegisterBool("vpn_blocker_whitelist", qtrue, 0, "Enable or disable the use of the whitelist"));
		Whitelist::SetFile(Plugin_Cvar_RegisterString("vpn_blocker_whitelist_file", "vpn_whitelist.dat", CVAR_INIT, "Name of file which holds the whitelist"));
	} catch(const std::invalid_argument& e) {
		Plugin_PrintError(e.what());
		return -1;
	}

	Plugin_AddCommand("vpn_whitelist_add", Whitelist::CommandHandler, Whitelist::cmdPower);
	Plugin_AddCommand("vpn_whitelist_remove", Whitelist::CommandHandler, Whitelist::cmdPower);

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

	IPResult result = IPIntel::Check(baninfo->adr);

	if (result.probability == 2)
		return;

	if (result.probability < 0) {
		Plugin_PrintWarning("[VPN BLOCKER] Got error code: %f, check https://getipintel.net/\n", result.probability);
		return;
	}

	if (IPIntel::ShouldKick(result.probability)) {
		int percentage = (int)(result.probability * 100);
		char* kickMsg = IPIntel::GetKickMessage();
		char format[2048];
		char tempId[128];

		Plugin_SteamIDToString(baninfo->playerid, tempId, sizeof(tempId));
		snprintf(format, strlen(kickMsg) + strlen(tempId) + 24, "%s [%i/100] [ID: %s]", kickMsg, percentage, tempId);
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
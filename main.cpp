#include "../pinc.h"
#include "whitelist.h"
#include "ipintel.h"
#include "ipcache.h"
#include "iplimit.h"

#include <string>
#include <stdexcept>

#define PLUGIN_NAME "VPN Blocker"
#define PLUGIN_DESC "Prevent VPN usage by player"
#define PLUGIN_DESC_LONG "Prevent players joining that are using a VPN by using the IP Intelligence API"
#define PLUGIN_VER_MAJ 0
#define PLUGIN_VER_MIN 4

PCL int OnInit(){ //Function executed after the plugin is loaded on the server.
	try {
		IPIntel::SetEmail(Plugin_Cvar_RegisterString("vpn_blocker_email", "", 0, "Email address to be used with IP Intel API (https://getipintel.net/)"));
		IPIntel::SetFlag(Plugin_Cvar_RegisterString("vpn_blocker_flag", "m", 0, "Flag to be used with IP Intel API (https://getipintel.net/)"));
		IPIntel::SetKickMessage(Plugin_Cvar_RegisterString("vpn_blocker_kick_msg", "Usage of a VPN or Proxy is not allowed on this server!", 0, "The message to be shown to the user when they are denied access to the server"));
		IPIntel::SetThreshold(Plugin_Cvar_RegisterFloat("vpn_blocker_threshold", 0.99f, 0.0f, 1.0f, 0, "Threshold value of when to kick a player based on the probability of using a VPN or Proxy (0.99+ is recommended)"));
		Whitelist::SetEnabled(Plugin_Cvar_RegisterBool("vpn_blocker_whitelist", qtrue, 0, "Enable or disable the use of the whitelist"));
		Whitelist::SetFile(Plugin_Cvar_RegisterString("vpn_blocker_whitelist_file", "vpn_whitelist.dat", CVAR_INIT, "Name of file which holds the whitelist"));
		IPCache::SetFile(Plugin_Cvar_RegisterString("vpn_blocker_cache_file", "vpn_cache.bin", CVAR_INIT, "Name of file which holds the cache"));
		IPLimit::SetFile(Plugin_Cvar_RegisterString("vpn_blocker_limit_file", "vpn_limit.bin", CVAR_INIT, "Name of file which holds the api limit info"));
	} catch(const std::invalid_argument& e) {
		Plugin_PrintError(e.what());
		return -1;
	}

	Plugin_AddCommand("vpn_whitelist_add", Whitelist::CommandHandler, Whitelist::cmdPower);
	Plugin_AddCommand("vpn_whitelist_remove", Whitelist::CommandHandler, Whitelist::cmdPower);
	Plugin_AddCommand("vpn_limit_info", IPLimit::CommandHandler, IPLimit::cmdPower);
	Plugin_AddCommand("vpn_cache_info", IPCache::CommandHandler, IPCache::cmdPower);

	if (Whitelist::IsEnabled())
		Whitelist::Load();

	IPCache::Load();
	IPLimit::Load();
	return 0;
}

PCL void OnPlayerGetBanStatus(baninfo_t* baninfo, char* message, int len) {
	if (message[0])
		return;

	if (baninfo->adr.type == NA_BOT)
		return;
	else if (baninfo->adr.type == NA_BAD) {
		std::string badAddressMsg = "Got a bad address when connecting, please try again!";
		strncpy(message, badAddressMsg.c_str(), len);
		return;
	}

	if (Whitelist::IsEnabled()) {
		if (Whitelist::IsWhitelisted(baninfo->playerid))
			return;
	}

	std::string addr = IPIntel::GetAddress(baninfo->adr);
	uint64_t addrNum = 	IPIntel::GetAddress(addr.c_str());
	bool isCached = IPCache::IsCached(addrNum);
	bool shouldUpdate = true;
	IPInfo cacheEntry;
	IPResult result;

	if (isCached) {
		cacheEntry = IPCache::Fetch(addrNum);
		shouldUpdate = IPCache::ShouldUpdate(cacheEntry.lastChecked);
		Plugin_Printf("Found %s cache entry containing (p->%f, t->%llu, u->%i)", addr.c_str(), cacheEntry.probability, cacheEntry.lastChecked, shouldUpdate);
	}

	if (shouldUpdate) {
		IPLimit::Increase();
		if (IPLimit::ReachedLimit()) {
			if (!IPLimit::TryReset()) {
				Plugin_Printf("[VPN BLOCKER] Limit reached, skipping...");
				return;
			}
		}

		result = IPIntel::Check(addr);
	} else
		result = IPResult(cacheEntry.probability, 200);

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
		snprintf(format, sizeof(format), "%s [%i/100] [ID: %s]", kickMsg, percentage, tempId);
		strncpy(message, format, len);
		Plugin_Printf("[VPN BLOCKER] Removed %s [%llu] %i%c\n", baninfo->playername, baninfo->playerid, percentage, '%');
	}

	if (shouldUpdate) {
		if (isCached)
			IPCache::Update(cacheEntry, result.probability);
		else
			IPCache::Insert(addrNum, result.probability);
	}

	return;
}

PCL void OnInfoRequest(pluginInfo_t *info) { //Function used to obtain information about the plugin
	info->handlerVersion.major = PLUGIN_HANDLER_VERSION_MAJOR;
	info->handlerVersion.minor = PLUGIN_HANDLER_VERSION_MINOR;

	info->pluginVersion.major = PLUGIN_VER_MAJ;
	info->pluginVersion.minor = PLUGIN_VER_MIN; //Plugin version

	strncpy(info->fullName, PLUGIN_NAME, sizeof(info->fullName)); //Full plugin name
	strncpy(info->shortDescription, PLUGIN_DESC, sizeof(info->shortDescription)); //Short plugin description
	strncpy(info->longDescription, PLUGIN_DESC_LONG, sizeof(info->longDescription)); //Long plugin description
}

PCL void OnExitLevel() {
	IPCache::Save();
	IPLimit::Save();
}

PCL void OnTerminate() {
	IPCache::Save();
	IPLimit::Save();
}

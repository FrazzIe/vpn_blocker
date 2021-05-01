#include "../pinc.h"

#include <string>

PCL int OnInit(){ // Function executed after the plugin is loaded on the server.
	return 0;
}

PCL void OnInfoRequest(pluginInfo_t *info){	// Function used to obtain information about the plugin
	info->handlerVersion.major = PLUGIN_HANDLER_VERSION_MAJOR;
	info->handlerVersion.minor = PLUGIN_HANDLER_VERSION_MINOR;

	info->pluginVersion.major = 0;
	info->pluginVersion.minor = 1; // Plugin version

	strncpy(info->fullName, "VPN Blocker", sizeof(info->fullName)); //Full plugin name
	strncpy(info->shortDescription, "Prevent VPN usage by player", sizeof(info->shortDescription)); // Short plugin description
	strncpy(info->longDescription, "Prevent players joining that are using a VPN by using the IP Intelligence API", sizeof(info->longDescription)); // Long plugin description
}
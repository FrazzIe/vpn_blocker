#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "whitelist.h"
#include <unordered_set>
#include <stdexcept>

std::unordered_set<uint64_t> Whitelist::ids = {};
cvar_t *Whitelist::enabled = NULL;
cvar_t *Whitelist::file = NULL;

void Whitelist::Load() {
	std::unordered_set<uint64_t> idSet = {};
	fileHandle_t fileHandle;
	long fileLength;

	fileLength = Plugin_FS_SV_FOpenFileRead(file->string, &fileHandle);

	if (fileHandle == 0) {
		Plugin_PrintWarning("[VPN BLOCKER] Couldn't open %s, does it exist?\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		ids = idSet;
		return;
	}

	if (fileLength < 1) {
		Plugin_Printf("[VPN BLOCKER] Couldn't open %s because it's empty!\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		ids = idSet;
		return;
	}

	char buf[256];
	int line;
	int count = 0;
	buf[0] = 0;

	while (true) {
		line = Plugin_FS_ReadLine(buf, sizeof(buf), fileHandle);

		if (line == 0) {
			Plugin_Printf("[VPN BLOCKER] %i lines parsed from %s\n", count, file->string);
			break;
		} else if(line == -1) {
			Plugin_PrintWarning("[VPN BLOCKER] Could not read from %s\n", file->string);
			break;
		}

		//Plugin_Printf("[VPN BLOCKER] line %i contains %s\n", count, buf);

		uint64_t id = Plugin_StringToSteamID(buf);

		//Plugin_Printf("[VPN BLOCKER] line %i contains %llu\n", count, id);

		if (id != 0) {
			idSet.insert(id);
			count++;
		}
	}

	Plugin_FS_FCloseFile(fileHandle);
	ids = idSet;
	return;
}

void Whitelist::Save() {
	fileHandle_t fileHandle;
	std::string tmpFile(file->string);
	char buf[128];

	tmpFile += ".tmp";
	fileHandle = Plugin_FS_SV_FOpenFileWrite(tmpFile.c_str());

	if (fileHandle == 0) {
		Plugin_PrintError("[VPN BLOCKER] Couldn't open %s\n", tmpFile.c_str());
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	for (std::unordered_set<uint64_t>::iterator iter = ids.begin(); iter != ids.end(); ++iter) {
		Plugin_SteamIDToString(*iter, buf, sizeof(buf));
		std::string line(buf);
		line += "\n";

		Plugin_FS_Write(line.c_str(), line.length(), fileHandle);
	}

	Plugin_FS_FCloseFile(fileHandle);
	Plugin_FS_SV_HomeCopyFile(strdup(tmpFile.c_str()), file->string);
}

void Whitelist::CommandHandler() {
	int numArgs;
	char *cmdName;
	char *playerArg;
	client_t *targetPlayer;
	uint64_t targetPlayerId;

	numArgs = Plugin_Cmd_Argc();

	if (numArgs == 0)
		return;

	cmdName = Plugin_Cmd_Argv(0);

	if (numArgs <= 1) {
		Plugin_Printf("[VPN BLOCKER] Usage: %s <player>\n", cmdName);
		return;
	}

	playerArg = Plugin_Cmd_Argv(1);
	targetPlayerId = Plugin_StringToSteamID(playerArg);
	targetPlayer = Plugin_SV_Cmd_GetPlayerClByHandle(playerArg);

	//Plugin_Printf("[VPN BLOCKER] Player ID: %llu\n", targetPlayerId);

	if (targetPlayerId == 0) {
		if (targetPlayer == NULL) {
			Plugin_Printf("[VPN BLOCKER] %s: Can't find player\n", cmdName);
			return;
		} else if (targetPlayer->state < CS_ACTIVE) {
			Plugin_Printf("[VPN BLOCKER] %s: Player isn't ready\n", cmdName);
			return;
		}

		targetPlayerId = targetPlayer->playerid;
	}

	std::string cmd(cmdName);

	if (cmd == "vpn_whitelist_add") {
		if (IsWhitelisted(targetPlayerId)) {
			Plugin_Printf("[VPN BLOCKER] %s: Player is already whitelisted!\n", cmdName);
			return;
		}

		ids.insert(targetPlayerId);

		Plugin_Printf("[VPN BLOCKER] %s: Player added to whitelist\n", cmdName);
	}

	if (cmd == "vpn_whitelist_remove") {
		if (!IsWhitelisted(targetPlayerId)) {
			Plugin_Printf("[VPN BLOCKER] %s: Player is not whitelisted!\n", cmdName);
			return;
		}

		ids.erase(targetPlayerId);

		Plugin_Printf("[VPN BLOCKER] %s: Player removed from whitelist\n", cmdName);
	}

	Save();
}

void Whitelist::SetEnabled(CONVAR_T* var) {
	enabled = (cvar_t*)var;
}
void Whitelist::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_whitelist_file is not set\n");
}

bool Whitelist::IsWhitelisted(uint64_t id) {
	return ids.find(id) != ids.end();
}

bool Whitelist::IsEnabled() {
	return Plugin_Cvar_GetBoolean(enabled) == qtrue;
}

bool Whitelist::IsFileSet() {
	return file->string[0];
}
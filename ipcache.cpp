#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "ipcache.h"
#include <string>
#include <math.h>

time_t GetSystemEpoch() {
	return time(NULL);
}

std::unordered_map<uint64_t, IPInfo> IPCache::ipMap = {};
cvar_t *IPCache::file = NULL;

void IPCache::Load() {
	fileHandle_t fileHandle;
	long fileLength;

	fileLength = Plugin_FS_SV_FOpenFileRead(file->string, &fileHandle);

	if (fileHandle == 0) {
		Plugin_PrintWarning("[VPN BLOCKER] Couldn't open %s, does it exist?\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	if (fileLength < 1) {
		Plugin_Printf("[VPN BLOCKER] Couldn't open %s because it's empty!\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	IPCacheFileHeader header;
	int numBytes;

	numBytes = Plugin_FS_Read(&header, sizeof(IPCacheFileHeader), fileHandle);

	if (numBytes == 0)  {
		Plugin_Printf("[VPN BLOCKER] Couldn't read %s header, is it corrupt?\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	if (header.ver != IPCACHE_FILE_HEADER_VER) {
		Plugin_Printf("[VPN BLOCKER] Couldn't read %s, unsupported format (%i != %i)\n", file->string, header.ver, IPCACHE_FILE_HEADER_VER);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	time_t curTime = GetSystemEpoch();

	for (uint64_t i = 0; i < header.size; i++) {
		uint64_t readAddress;
		IPInfo readEntry;

		Plugin_FS_Read(&readAddress, sizeof(uint64_t), fileHandle);
		Plugin_FS_Read(&readEntry.probability, sizeof(float), fileHandle);
		Plugin_FS_Read(&readEntry.lastChecked, sizeof(time_t), fileHandle);

		//Plugin_Printf("Got %llu with %f and %lld from entry %llu\n", readAddress, readEntry.probability, readEntry.lastChecked, i);

		if (curTime < readEntry.lastChecked)
			ipMap.insert(std::make_pair(readAddress, readEntry));
	}

	Plugin_FS_FCloseFile(fileHandle);
	return;
}

void IPCache::Save() {
	fileHandle_t fileHandle;
	std::string tmpFile(file->string);

	tmpFile += ".tmp";
	fileHandle = Plugin_FS_SV_FOpenFileWrite(tmpFile.c_str());

	if (fileHandle == 0) {
		Plugin_PrintError("[VPN BLOCKER] Couldn't open %s\n", tmpFile.c_str());
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	IPCacheFileHeader header;
	header.size = ipMap.size();

	Plugin_FS_Write(&header, sizeof(IPCacheFileHeader), fileHandle);

	for (std::pair<uint64_t, IPInfo> entry : ipMap) {
		Plugin_FS_Write(&entry.first, sizeof(uint64_t), fileHandle);
		Plugin_FS_Write(&entry.second.probability, sizeof(float), fileHandle);
		Plugin_FS_Write(&entry.second.lastChecked, sizeof(time_t), fileHandle);

		//Plugin_Printf("Stored %llu with %f and %lld from entry %llu\n", entry.first, entry.second.probability, entry.second.lastChecked);
	}

	Plugin_FS_FCloseFile(fileHandle);
	Plugin_FS_SV_HomeCopyFile(strdup(tmpFile.c_str()), file->string);
}

void IPCache::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_cache_file is not set\n");
}

void IPCache::Insert(uint64_t addr, float probability) {
	ipMap.insert(std::make_pair(addr, IPInfo(probability, GetSystemEpoch() + cacheLength)));
}

void IPCache::Update(IPInfo &entry, float probability) {
	entry.probability = probability;
	entry.lastChecked = GetSystemEpoch() + cacheLength;
}

void IPCache::CommandHandler() {
	int numArgs;
	char *cmdName;

	numArgs = Plugin_Cmd_Argc();

	if (numArgs == 0)
		return;

	cmdName = Plugin_Cmd_Argv(0);

	std::string cmd(cmdName);

	if (cmd == "vpn_cache_info") {
		time_t curTime = GetSystemEpoch();
		uint64_t lastChecked = 0;
		uint64_t count = 0;
		uint32_t hour, minute;

		for (std::pair<uint64_t, IPInfo> entry : ipMap) {
			lastChecked = (curTime - (entry.second.lastChecked - cacheLength)) / 60;
			minute = floor(lastChecked % 60);
			lastChecked /= 60;
			hour = floor(lastChecked % 24);

			Plugin_Printf("[VPN BLOCKER] %llu - Probability: %f, Last checked: %u %s, %u %s ago\n", entry.first, entry.second.probability, hour, (hour > 1) ? "hrs" : "hr", minute, (minute > 1) ? "mins" : "min");
			count++;
		}

		Plugin_Printf("[VPN BLOCKER] %llu cache entries\n", count);
	}
}

IPInfo& IPCache::Fetch(uint64_t addr) {
	return ipMap.at(addr);
}

bool IPCache::IsCached(uint64_t addr) {
	return ipMap.find(addr) != ipMap.end();
}

bool IPCache::ShouldUpdate(int64_t lastChecked) {
	return GetSystemEpoch() > lastChecked;
}
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "ipcache.h"
#include <string>
#include <chrono>

int64_t GetSystemEpoch() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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

	IPFileHeader header;
	int numBytes;

	numBytes = Plugin_FS_Read(&header, sizeof(IPFileHeader), fileHandle);

	if (numBytes == 0)  {
		Plugin_Printf("[VPN BLOCKER] Couldn't read %s header, is it corrupt?\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	if (header.ver != IP_FILE_HEADER_VER) {
		Plugin_Printf("[VPN BLOCKER] Couldn't read %s, unsupported format\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	int64_t curTime = GetSystemEpoch();

	for (uint64_t i = 0; i < header.size; i++) {
		uint64_t readAddress;
		IPInfo readEntry;

		Plugin_FS_Read(&readAddress, sizeof(uint64_t), fileHandle);
		Plugin_FS_Read(&readEntry.probability, sizeof(float), fileHandle);
		Plugin_FS_Read(&readEntry.lastChecked, sizeof(int64_t), fileHandle);

		//Plugin_Printf("Got %llu with %f and %lld from entry %llu\n", readAddress, readEntry.probability, readEntry.lastChecked, i);

		if (curTime < readEntry.lastChecked)
			ipMap.insert(std::make_pair(readAddress, readEntry));
	}

	Plugin_FS_FCloseFile(fileHandle);
	return;
}

void IPCache::Save() {

}

void IPCache::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_cache_file is not set\n");
}

void IPCache::Insert(uint64_t addr, float probability) {
	ipMap.insert(std::make_pair(addr, IPInfo(probability, GetSystemEpoch() + cacheLength)));
	Save();
}

void IPCache::Update(IPInfo &entry, float probability) {
	entry.probability = probability;
	entry.lastChecked = GetSystemEpoch() + cacheLength;
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
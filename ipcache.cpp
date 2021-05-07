#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "ipcache.h"
#include <string>
#include <chrono>

int64_t GetSystemEpoch() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::unordered_map<std::string, IPInfo> IPCache::ipMap = {};
cvar_t *IPCache::file = NULL;

void IPCache::Load() {

}

void IPCache::Save() {

}

void IPCache::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_cache_file is not set\n");
}

void IPCache::Insert(std::string addr, float probability) {
	ipMap.insert(std::make_pair(addr, IPInfo(probability, GetSystemEpoch() + cacheLength)));
}

void IPCache::Update(IPInfo &entry, float probability) {
	entry.probability = probability;
	entry.lastChecked = GetSystemEpoch() + cacheLength;
}

IPInfo& IPCache::Fetch(std::string addr) {
	return ipMap.at(addr);
}

bool IPCache::IsCached(std::string addr) {
	return ipMap.find(addr) != ipMap.end();
}

bool IPCache::ShouldUpdate(int64_t lastChecked) {
	return GetSystemEpoch() > lastChecked;
}
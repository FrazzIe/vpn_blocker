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

void IPCache::Insert(std::string addr, float probability) {
	ipMap.insert(addr, IPInfo(probability, GetSystemEpoch() + cacheLength));
}

IPInfo IPCache::Fetch(std::string addr) {
	return ipMap.at(addr);
}

bool IPCache::Cached(std::string addr) {
	return ipMap.find(addr) != ipMap.end();
}

#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "ipcache.h"
#include <string>

std::unordered_map<std::string, IPInfo> IPCache::ipMap = {};

void IPCache::Insert(std::string addr, float probability) {
	ipMap.insert(addr, IPInfo(probability, 0));
}

IPInfo IPCache::Fetch(std::string addr) {
	return ipMap.at(addr);
}

bool IPCache::Cached(std::string addr) {
	return ipMap.find(addr) != ipMap.end();
}

#ifndef IPCACHE_H
#define IPCACHE_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include <string>
#include <unordered_map>

struct IPInfo {
	float probability;
	int64_t lastChecked;

	IPInfo(float _probability, int64_t _lastChecked) {
		probability = _probability;
		lastChecked = _lastChecked;
	}
};

class IPCache {
private:
	static std::unordered_map<std::string, IPInfo> ipMap;
	static const int64_t cacheLength = 21600000; //6 Hours
	static const int queryLengthMax = 500;
	static int queryLength;
public:
	static void Insert(std::string addr, float probability);
	static void Update(IPInfo &entry);
	static IPInfo& Fetch(std::string addr);
	static bool IsCached(std::string addr);
	static bool ShouldUpdate(int64_t lastChecked);
};

#endif // IPCACHE_H
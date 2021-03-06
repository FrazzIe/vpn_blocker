#ifndef IPCACHE_H
#define IPCACHE_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include <string>
#include <unordered_map>

#define IPCACHE_FILE_HEADER_VER 1

struct IPInfo {
	float probability;
	time_t lastChecked;

	IPInfo() : probability(2), lastChecked(0) {

	}
	IPInfo(float _probability, int64_t _lastChecked) {
		probability = _probability;
		lastChecked = _lastChecked;
	}
};

struct IPCacheFileHeader {
	uint64_t size;
	int ver;

	IPCacheFileHeader() : size(0), ver(IPCACHE_FILE_HEADER_VER) {

	}
};

class IPCache {
private:
	static std::unordered_map<uint64_t, IPInfo> ipMap;
	static const time_t cacheLength = 43200; //6 Hours
	static cvar_t *file;
public:
	static const int cmdPower = 100;
public:
	static void Load();
	static void Save();
	static void SetFile(CONVAR_T* var);
	static void Insert(uint64_t addr, float probability);
	static void Update(IPInfo &entry, float probability);
	static void CommandHandler();
	static IPInfo& Fetch(uint64_t addr);
	static bool IsCached(uint64_t addr);
	static bool ShouldUpdate(int64_t lastChecked);
};

#endif // IPCACHE_H
#ifndef IPLIMIT_H
#define IPLIMIT_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif

#define IPLIMIT_FILE_HEADER_VER 1

struct IPLimitFileHeader {
	int ver;

	IPLimitFileHeader() : ver(IPLIMIT_FILE_HEADER_VER) {

	}
};


class IPLimit {
private:
	static cvar_t *file;
	static const uint32_t limit = 500;
	static uint32_t count;
	static time_t reset;
public:
	static void Load();
	static void Save();
	static void Create();
	static void SetFile(CONVAR_T* var);
	static void Increase();
	static bool ReachedLimit();
	static bool TryReset();
};

#endif // IPLIMIT_H
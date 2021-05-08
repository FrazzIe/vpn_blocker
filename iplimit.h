#ifndef IPLIMIT_H
#define IPLIMIT_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif

class IPLimit {
private:
	static cvar_t *file;
	static const uint32_t limit = 500;
	static uint32_t count;
	static time_t reset;
public:
	static void Load();
	static void Save();
	static void SetFile(CONVAR_T* var);
};

#endif // IPLIMIT_H
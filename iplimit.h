#ifndef IPLIMIT_H
#define IPLIMIT_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif

class IPLimit {
private:
	static cvar_t *file;
	static const int limit = 500;
	static int count;
	static time_t reset;
public:
	static void Load();
	static void Save();
	static void SetFile(CONVAR_T* var);
};

#endif // IPLIMIT_H
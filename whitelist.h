#ifndef WHITELIST_H
#define WHITELIST_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include <unordered_set>

class Whitelist {
private:
	static std::unordered_set<uint64_t> ids;
	static cvar_t *enabled;
	static cvar_t *file;
public:
	static const int cmdPower = 100;
public:
	static void Load();
	static void Save();
	static void CommandHandler();
	static void SetEnabled(CONVAR_T* var);
	static void SetFile(CONVAR_T* var);
	static bool IsWhitelisted(uint64_t id);
	static bool IsEnabled();
	static bool IsFileSet();
};

#endif // WHITELIST_H
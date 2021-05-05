#ifndef WHITELIST_H
#define WHITELIST_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include <unordered_set>

class Whitelist {
private:
	static std::unordered_set<std::uint64_t> ids;
	static cvar_t *enabled;
	static cvar_t *file;
public:
	static void Load();
	static void Save();
	static void CommandHandler();
	static void SetEnabled(CONVAR_T* var);
	static void SetFile(CONVAR_T* var);
	static bool IsWhitelisted(std::int64_t id);
	static bool IsEnabled();
	static bool IsFileSet();
};

#endif
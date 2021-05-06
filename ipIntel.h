#ifndef IPINTEL_H
#define IPINTEL_H
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include <string>

struct IPResult {
	float probability;
	int code;

	IPResult(float _probability, int _code) {
		probability = _probability;
		code = _code;
	}
};

class IPIntel {
private:
	static cvar_t *email;
	static cvar_t *flag;
	static cvar_t *kickMsg;
	static cvar_t *threshold;
	struct API
	{
		static std::string url;
		static std::string params;
	};
public:
	static void SetEmail(CONVAR_T* var);
	static void SetFlag(CONVAR_T* var);
	static void SetKickMessage(CONVAR_T* var);
	static void SetThreshold(CONVAR_T* var);
	static void SetAPIParams();
	static IPResult Check(std::string addr);
	static std::string GetAddress(netadr_t addr);
	static std::string GetURL(std::string addr);
	static char* GetKickMessage();
	static bool ShouldKick(float probability);
};

#endif // IPINTEL_H
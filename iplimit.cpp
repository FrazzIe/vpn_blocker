#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "iplimit.h"
#include <stdexcept>
#include <time.h>

cvar_t *IPLimit::file = NULL;
void IPLimit::Load() {

}

void IPLimit::Save() {

}

void IPLimit::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_limit_file is not set\n");
}
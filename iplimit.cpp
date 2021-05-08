#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "iplimit.h"
#include <stdexcept>
#include <time.h>

cvar_t *IPLimit::file = NULL;
int IPLimit::count = 0;
time_t IPLimit::reset = 0;
void IPLimit::Load() {

}

void IPLimit::Save() {

}

void IPLimit::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_limit_file is not set\n");
}
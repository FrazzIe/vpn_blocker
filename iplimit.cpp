#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "iplimit.h"
#include <stdexcept>
#include <time.h>

cvar_t *IPLimit::file = NULL;
int IPLimit::count = 0;
time_t IPLimit::reset = 0;

//https://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date
bool isDST(tm tm) {
	//January, february, and december are out.
	if (tm->tm_mon < 2 || tm->tm_mon > 10) { return false; }
	//April to October are in
	if (tm->tm_mon > 2 && tm->tm_mon < 10) { return true; }
	int previousSunday = tm->tm_mday - tm->tm_wday;
	//In march, we are DST if our previous sunday was on or after the 8th.
	if (tm->tm_mon == 2)
		return previousSunday >= 8;
	//In november we must be before the first sunday to be dst.
	//That means the previous sunday must be before the 1st.
	return previousSunday <= 0;
}

time_t GetUTCEpoch() {
	time_t t = time(NULL);
	gmtime(&t);
	return t;
}

time_t GetResetEpoch() {
	time_t t = time(NULL);
	struct tm *tInfo = gmtime(&t);
	int hour = isDST(tInfo) ? 4 : 5
	if (tInfo->tm_hour >= hour)
		tInfo->tm_mday++;
	tInfo->tm_hour = hour;
	tInfo->tm_min = 0;
	tInfo->tm_sec = 0;
	tInfo->tm_isdst = -1;

	return mktime(tInfo);
}

void IPLimit::Load() {

}

void IPLimit::Save() {

}

void IPLimit::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_limit_file is not set\n");
}
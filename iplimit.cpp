#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "iplimit.h"
#include <stdexcept>
#include <time.h>

cvar_t *IPLimit::file = NULL;
uint32_t IPLimit::count = 0;
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
	fileHandle_t fileHandle;
	long fileLength;

	fileLength = Plugin_FS_SV_FOpenFileRead(file->string, &fileHandle);

	if (fileHandle == 0) {
		Plugin_PrintWarning("[VPN BLOCKER] Couldn't open %s, does it exist?\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	if (fileLength < 1) {
		Plugin_Printf("[VPN BLOCKER] Couldn't open %s because it's empty!\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	IPLimitFileHeader header;
	int numBytes;

	numBytes = Plugin_FS_Read(&header, sizeof(IPLimitFileHeader), fileHandle);

	if (numBytes == 0)  {
		Plugin_Printf("[VPN BLOCKER] Couldn't read %s header, is it corrupt?\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	if (header.ver != IPLIMIT_FILE_HEADER_VER) {
		Plugin_Printf("[VPN BLOCKER] Couldn't read %s, unsupported format\n", file->string);
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	Plugin_FS_Read(&count, sizeof(uint32_t), fileHandle);
	Plugin_FS_Read(&reset, sizeof(time_t), fileHandle);

	Plugin_FS_FCloseFile(fileHandle);
	return;
}

void IPLimit::Save() {
	fileHandle_t fileHandle;
	std::string tmpFile(file->string);

	tmpFile += ".tmp";
	fileHandle = Plugin_FS_SV_FOpenFileWrite(tmpFile.c_str());

	if (fileHandle == 0) {
		Plugin_PrintError("[VPN BLOCKER] Couldn't open %s\n", tmpFile.c_str());
		Plugin_FS_FCloseFile(fileHandle);
		return;
	}

	IPLimitFileHeader header;
	header.size = ipMap.size();

	Plugin_FS_Write(&header, sizeof(IPLimitFileHeader), fileHandle);
	Plugin_FS_Write(&count, sizeof(uint32_t), fileHandle);
	Plugin_FS_Write(&reset, sizeof(time_t), fileHandle);

	Plugin_FS_FCloseFile(fileHandle);
	Plugin_FS_SV_HomeCopyFile(strdup(tmpFile.c_str()), file->string);
}

void IPLimit::SetFile(CONVAR_T* var) {
	file = (cvar_t*)var;

	if (!file->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_limit_file is not set\n");
}

void Increase() {
	if (count != limit)
		count++;
}

bool IPLimit::ReachedLimit() {
	return count >= limit;
}

bool IPLimit::TryReset() {
	time_t curTime = GetUTCEpoch();

	if (curTime > reset) {
		reset = GetResetEpoch();
		count = 0;
		Save();
		return true;
	}

	return false;
}
#ifndef PLUGIN_INCLUDES
#include "../pinc.h"
#endif
#include "ipintel.h"
#include <string>
#include <stdexcept>

cvar_t *IPIntel::email = NULL;
cvar_t *IPIntel::flag = NULL;
cvar_t *IPIntel::kickMsg = NULL;
cvar_t *IPIntel::threshold = NULL;
std::string IPIntel::API::url = "http://check.getipintel.net/check.php?ip=";
std::string IPIntel::API::params = "";

void IPIntel::SetEmail(CONVAR_T* var) {
	email = (cvar_t*)var;

	if (!email->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_email is not set\n");

	if (flag != NULL)
		SetAPIParams();
}

void IPIntel::SetFlag(CONVAR_T* var) {
	flag = (cvar_t*)var;

	if (!flag->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_flag is not set\n");
	else if (flag->string[1] || (flag->string[0] != 'm' && flag->string[0] != 'b'))
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_flag is invalid (use m or b)\n");

	if (email != NULL)
		SetAPIParams();
}

void IPIntel::SetKickMessage(CONVAR_T* var) {
	kickMsg = (cvar_t*)var;

	if (!kickMsg->string[0])
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_kick_msg is not set\n");
}

void IPIntel::SetThreshold(CONVAR_T* var) {
	threshold = (cvar_t*)var;

	threshold->fmin = 0.0f;
	threshold->fmax = 1.0f;

	if (threshold->value < threshold->fmin || threshold->value > threshold->fmax)
		throw std::invalid_argument("Init failure. Cvar vpn_blocker_threshold must be between 0.0 and 1.0\n");
}

void IPIntel::SetAPIParams() {
	std::string _email(email->string);
	std::string _flag(flag->string);
	API::params = "&contact=" + _email + "&flags=" + _flag;
}

IPResult IPIntel::Check(std::string addr) {
	std::string url = GetURL(addr);

	//Make GET Request to API
	char data[8192];
	int contentLength;
	int resCode;

	ftRequest_t* request = Plugin_HTTP_GET(url.c_str());

	if (request == NULL) {
		Plugin_Printf("[VPN BLOCKER] Request was NULL\n");
		return IPResult(2, 400);
	}

	if (request->code != 200 && request->contentLength <= 0) {
		Plugin_Printf("[VPN BLOCKER] Request failed with code: %d\n", request->code);
		return IPResult(2, request->code);
	}

	//Get response
	contentLength = request->contentLength;
	resCode = request->code;
	memcpy(data, request->extrecvmsg->data + request->headerLength, contentLength);
	data[contentLength] = 0;

	//Free request
	Plugin_HTTP_FreeObj(request);

	//Get string response
	std::string result(data);
	float probability = 2;

	try {
		probability = std::stof(result);
	} catch(...) {
		Plugin_Printf("[VPN BLOCKER] Failed to convert request result\n");
	}

	return IPResult(probability, resCode);
}

std::string IPIntel::GetAddress(netadr_t addr) {
	char address[128];

	Plugin_NET_AdrToStringShortMT(&addr, address, sizeof(address));

	return std::string(address);
}

std::string IPIntel::GetURL(std::string addr) {
	return API::url + addr + API::params;
}

char* IPIntel::GetKickMessage() {
	return kickMsg->string;
}

bool IPIntel::ShouldKick(float probability) {
	return probability >= threshold->value;
}
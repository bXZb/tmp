#pragma once

#ifdef _WIN64
#define DoSyscall DoIndirectSyscallx64 // Change this to DoRecycledGateSyscallx64 (x64 only) if desired.
#elif _WIN32
#define DoSyscall DoIndirectSyscallx86
#endif

void PrepareSyscall(DWORD sysNum, PVOID addr);
NTSTATUS DoRecycledGateSyscallx64(...);
NTSTATUS DoIndirectSyscallx64(...);
NTSTATUS DoIndirectSyscallx86(...);

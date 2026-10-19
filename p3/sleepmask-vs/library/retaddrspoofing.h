#pragma once
#include "beacon_gate.h"

/**
* A structure to encapsulate the paramaters for return address spoofing.
*
* RopGadget - A pointer to the ROP gadget
* TargetFunction - A pointer to the target function
* RestoreRegister - The original value of the register used to JMP
* OriginalRetAddress - The original return address pre-spoof
*/
#ifdef _WIN64
typedef struct {
    const void* RopGadget;          //0
    void* TargetFunction;           //8
    void* RestoreRegister;          //16
    void* OriginalRetAddress;       //24
} RET_SPOOF_INFO, * PRET_SPOOF_INFO;
#elif _WIN32
typedef struct {
    void* OriginalEbx;              //0
    const void* RopGadget;          //4
    void* TargetFunction;           //8
    void* OriginalReturnAddress;    //12
    void* Fixup;                    //16
} RET_SPOOF_INFO, * PRET_SPOOF_INFO;
#endif

/**
* Structure to hold ret addres spoofing rop gadgets.
*/
typedef struct _GADGETS {
    void* WinInet;
    void* Kernel32;
} GADGETS, * PGADGETS;

/**
* Function definitions for return address spoofing.
*/
#ifdef _WIN64
ULONG_PTR SpoofReturnAddressx64(...);
#elif _WIN32
ULONG_PTR SpoofReturnAddressx86(...);
#endif

PVOID FindGadget(char* moduleHandle, const char* gadget, size_t gadgetLength);
BOOL FindGadgets(PGADGETS gadgets);
void SetupFunctionCall(PFUNCTION_CALL functionCall, PGADGETS gadgets, PRET_SPOOF_INFO spoofInfo);

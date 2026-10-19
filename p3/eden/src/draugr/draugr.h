#pragma once
#include <windows.h>
#include "../eden.h"

/*
* Call stack spoofing defines.
*/
typedef ULONG(NTAPI* RtlRandomExPtr)(PULONG seed);
typedef PRUNTIME_FUNCTION (NTAPI* RtlLookupFunctionEntryPtr)(DWORD64 ControlPc,PDWORD64 ImageBase, PUNWIND_HISTORY_TABLE HistoryTable);

#define draugrArg(i) (ULONG_PTR)draugrCall->FunctionCall->args[i]
#define RBP_OP_INFO 0x5
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)

#define maxNumberOfGadgets 15
#define maxNumberOfTries 15

typedef struct _DRAUGR_FUNCTION_CALL {
    PFUNCTION_CALL FunctionCall;
    PVOID          StackFrame;
    PVOID          SpoofCall;
} DRAUGR_FUNCTION_CALL, *PDRAUGR_FUNCTION_CALL;

typedef struct _FRAME_INFO {
    PVOID   ModuleAddress;
    PVOID   FunctionAddress;
    DWORD   Offset;
} FRAME_INFO, * PFRAME_INFO;

typedef struct _SYNTHETIC_STACK_FRAME {
    FRAME_INFO  Frame1;
    FRAME_INFO  Frame2;
    PVOID       pGadget;
} SYNTHETIC_STACK_FRAME, * PSYNTHETIC_STACK_FRAME;

typedef struct _DRAUGR_PARAMETERS {
    PVOID       Fixup;                                 // 0
    PVOID       OriginalReturnAddress;                 // 8
    PVOID       Rbx;                                   // 16
    PVOID       Rdi;                                   // 24
    PVOID       BaseThreadInitThunkStackSize;          // 32
    PVOID       BaseThreadInitThunkReturnAddress;      // 40
    PVOID       TrampolineStackSize;                   // 48
    PVOID       RtlUserThreadStartStackSize;           // 56
    PVOID       RtlUserThreadStartReturnAddress;       // 64
    PVOID       Ssn;                                   // 72
    PVOID       Trampoline;                            // 80
    PVOID       Rsi;                                   // 88
    PVOID       R12;                                   // 96
    PVOID       R13;                                   // 104
    PVOID       R14;                                   // 112
    PVOID       R15;                                   // 120
} DRAUGR_PARAMETERS, * PDRAUGR_PARAMETERS;

typedef struct _STACK_FRAME {
    LPCWSTR    DllPath;
    ULONG      Offset;
    ULONGLONG  TotalStackSize;
    BOOL       RequiresLoadLibrary;
    BOOL       SetsFramePointer;
    PVOID      ReturnAddress;
    BOOL       PushRbp;
    ULONG      CountOfCodes;
    BOOL       PushRbpIndex;
} STACK_FRAME, * PSTACK_FRAME;

typedef enum _UNWIND_OP_CODES {
    UWOP_PUSH_NONVOL = 0, // info == register number
    UWOP_ALLOC_LARGE,     // no info, alloc size in next 2 slots
    UWOP_ALLOC_SMALL,     // info == size of allocation / 8 - 1
    UWOP_SET_FPREG,       // no info, FP = RSP + UNWIND_INFO.FPRegOffset*16
    UWOP_SAVE_NONVOL,     // info == register number, offset in next slot
    UWOP_SAVE_NONVOL_FAR, // info == register number, offset in next 2 slots
    UWOP_SAVE_XMM128 = 8, // info == XMM reg number, offset in next slot
    UWOP_SAVE_XMM128_FAR, // info == XMM reg number, offset in next 2 slots
    UWOP_PUSH_MACHFRAME   // info == 0: no error-code, 1: error-code
} UNWIND_CODE_OPS, * PUNWIND_CODE_OPS;

typedef union _UNWIND_CODE {
    struct {
        BYTE CodeOffset;
        BYTE UnwindOp : 4;
        BYTE OpInfo : 4;
    };
    USHORT FrameOffset;
} UNWIND_CODE, * PUNWIND_CODE;

typedef struct _UNWIND_INFO {
    BYTE Version : 3;
    BYTE Flags : 5;
    BYTE SizeOfProlog;
    BYTE CountOfCodes;
    BYTE FrameRegister : 4;
    BYTE FrameOffset : 4;
    UNWIND_CODE UnwindCode[1];
} UNWIND_INFO, * PUNWIND_INFO;

/* Draugr function definitions */
void* DraugrSpoofStub(PVOID pArg1, PVOID pArg2, PVOID pArg3, PVOID pArg4, PVOID draugrParameters, PVOID pFunctionAddr, int size, PVOID pArg5, PVOID pArg6, PVOID pArg7, PVOID pArg8, PVOID pArg9, PVOID pArg10, PVOID pArg11, PVOID pArg12);
PVOID SpoofCall(IMPORTFUNCS * funcs, PSYNTHETIC_STACK_FRAME stackFrame, PVOID pFunctionAddr, PVOID pArg1, PVOID pArg2, PVOID pArg3, PVOID pArg4, PVOID pArg5, PVOID pArg6, PVOID pArg7, PVOID pArg8, PVOID pArg9, PVOID pArg10, PVOID pArg11, PVOID pArg12);
typedef PVOID(*SpoofCallPtr)( IMPORTFUNCS * funcs, PSYNTHETIC_STACK_FRAME stackFrame, PVOID pFunctionAddr, PVOID pArg1, PVOID pArg2, PVOID pArg3, PVOID pArg4, PVOID pArg5, PVOID pArg6, PVOID pArg7, PVOID pArg8, PVOID pArg9, PVOID pArg10, PVOID pArg11, PVOID pArg12);

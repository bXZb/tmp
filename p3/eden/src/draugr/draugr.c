#include "../eden.h"

/*
* The Draugr PIC stub expects an IMPORTFUNCS struct (i.e. vtable) containing LoadLibraryA, GetProcAddress, and GetModuleHandleA to resolve all it needs.
* NB The idea behind this was to decouple it from its caller (i.e. so the caller doesnt need to resolve a bunch of WinAPIs and pass them in).
*/
void Callgate(IMPORTFUNCS * funcs, PFUNCTION_CALL functionCall);

/* This is the REAL entry point to this whole mess and it needs to go first! */
__declspec(noinline) void go(IMPORTFUNCS * funcs, PFUNCTION_CALL functionCall) {
    Callgate(funcs, functionCall);
}

/* NB these are after so that the entry point^ is first code in PIC blob. */
#include <windows.h>
#include "../tcglib/loader.h"
#include "../tcglib/picdebug.h"
#include "draugr.h"
#include "stdlib.c"

/*
* This is a PIC port of: https://github.com/NtDallas/Draugr & https://github.com/Cobalt-Strike/sleepmask-vs/blob/dfaf24816f2947660cdfa301ffd764e9edf12356/sleepmask-vs/draugr-sleepmask.cpp#L69.
* Only very minor changes to the original implementation have been made.
* NB Draugr is only supported on X64.
*/

/*
* Execute DraugrGate.
*
* @param funcs A pointer to an IMPORTFUNCS structure.
* @param draugrCall A pointer to a DRAUGR_FUNCTION_CALL structure.
*/
void DraugrGate(IMPORTFUNCS * funcs, PDRAUGR_FUNCTION_CALL draugrCall) {
    ULONG_PTR retValue = 0;

    /* sanity check incoming ptr */
    if (!draugrCall) {
        return;
    }

    /* call appropriate function pointer based on number of args */
    if (draugrCall->FunctionCall->numOfArgs == 0) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 1) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 2) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 3) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 4) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0),(PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 5) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 6) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), (PVOID)draugrArg(5), NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 7) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), (PVOID)draugrArg(5), (PVOID)draugrArg(6), NULL, NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 8) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), (PVOID)draugrArg(5), (PVOID)draugrArg(6), (PVOID)draugrArg(7), NULL, NULL, NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 9) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), (PVOID)draugrArg(5), (PVOID)draugrArg(6), (PVOID)draugrArg(7), (PVOID)draugrArg(8), NULL, NULL, NULL);
  }
    else if (draugrCall->FunctionCall->numOfArgs == 10) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), (PVOID)draugrArg(5), (PVOID)draugrArg(6), (PVOID)draugrArg(7), (PVOID)draugrArg(8), (PVOID)draugrArg(9), NULL, NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 11) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), (PVOID)draugrArg(5), (PVOID)draugrArg(6), (PVOID)draugrArg(7), (PVOID)draugrArg(8), (PVOID)draugrArg(9), (PVOID)draugrArg(10), NULL);
    }
    else if (draugrCall->FunctionCall->numOfArgs == 12) {
        retValue = (ULONG_PTR)((SpoofCallPtr)draugrCall->SpoofCall)(funcs, (PSYNTHETIC_STACK_FRAME)draugrCall->StackFrame, draugrCall->FunctionCall->functionPtr, (PVOID)draugrArg(0), (PVOID)draugrArg(1), (PVOID)draugrArg(2), (PVOID)draugrArg(3), (PVOID)draugrArg(4), (PVOID)draugrArg(5), (PVOID)draugrArg(6), (PVOID)draugrArg(7), (PVOID)draugrArg(8), (PVOID)draugrArg(9), (PVOID)draugrArg(10), (PVOID)draugrArg(11));
    }

    draugrCall->FunctionCall->retValue = retValue;

    return;
}

/*
* Retrieve size of text section.
*
* @param funcs A pointer to an IMPORTFUNCS structure.
* @param pModule A pointer to target module.
* @param pdwVirtualAddress [out] A pointer to start of target module text section.
* @param pdwSize [out] A pointer to size of target module text section.
* @return A bool indicating success.
*/
BOOL GetTextSectionSize(IMPORTFUNCS * funcs, PVOID pModule, PDWORD pdwVirtualAddress, PDWORD pdwSize) {
    PIMAGE_DOS_HEADER pImgDosHeader = (PIMAGE_DOS_HEADER)(pModule);
    if (pImgDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
      return FALSE;
    }

    PIMAGE_NT_HEADERS pImgNtHeaders = (PIMAGE_NT_HEADERS)((UINT_PTR)pModule + pImgDosHeader->e_lfanew);
    if (pImgNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
      return FALSE;
    }

    PIMAGE_SECTION_HEADER   pImgSectionHeader = IMAGE_FIRST_SECTION(pImgNtHeaders);
    for (int i = 0; i < pImgNtHeaders->FileHeader.NumberOfSections; i++) {
      if (_strcmp((char*)pImgSectionHeader[i].Name, (char*)".text") == 0) {
        *pdwVirtualAddress = pImgSectionHeader[i].VirtualAddress;
        *pdwSize = pImgSectionHeader[i].SizeOfRawData;
        return TRUE;
      }
    }

    return FALSE;
}

/*
* Find a gadget to use for call stack spoofing.
*
* @param funcs A pointer to an IMPORTFUNCS structure.
* @param pModuleAddr A pointer to target module base address.
* @return A pointer to a valid gadget.
*/
PVOID FindGadget(IMPORTFUNCS * funcs, PVOID  pModuleAddr) {
    DWORD dwTextSectionSize = 0;
    DWORD dwTextSectionVa = 0;
    PVOID pGadgetList[maxNumberOfGadgets];
    DWORD dwCounter = 0;
    ULONG seed = 0;
    ULONG randomNbr = 0;
    PVOID pModTextSection = NULL;
    RtlRandomExPtr RtlRandomEx = (RtlRandomExPtr)funcs->GetProcAddress(funcs->GetModuleHandleA("ntdll"), "RtlRandomEx");

    if (!GetTextSectionSize(funcs, pModuleAddr, &dwTextSectionVa, &dwTextSectionSize)) {
        return NULL;
    }

    pModTextSection = (PBYTE)((UINT_PTR)pModuleAddr + dwTextSectionVa);
    for (int i = 0; i < (dwTextSectionSize - 2); i++) {
        /* searching for jmp rbx gadget */
        if (((PBYTE)pModTextSection)[i] == 0xFF && ((PBYTE)pModTextSection)[i + 1] == 0x23) {
            pGadgetList[dwCounter] = (void*)((UINT_PTR)pModTextSection + i);
            dwCounter++;
            if (dwCounter == maxNumberOfGadgets) {
                break;
            }
        }
    }

    seed = 0x1337;
    randomNbr = RtlRandomEx(&seed);
    randomNbr %= dwCounter;
    return pGadgetList[randomNbr];
}

/*
* Calculates the stack size for a given function.
*
* @param pRuntimeFunction A pointer to a RUNTIME_FUNCTION structure.
* @param imageBase A dword holding the image base.
* @return A PVOID containing function stack size.
*/
void* CalculateFunctionStackSize(PRUNTIME_FUNCTION pRuntimeFunction, const DWORD64 imageBase) {
    PUNWIND_INFO pUnwindInfo = NULL;
    ULONG unwindOperation = 0;
    ULONG operationInfo = 0;
    ULONG index = 0;
    ULONG frameOffset = 0;
    STACK_FRAME stackFrame;
    _memset(&stackFrame, 0, sizeof(stackFrame));

    if (!pRuntimeFunction) {
        goto Cleanup;
    }

    pUnwindInfo = (PUNWIND_INFO)(pRuntimeFunction->UnwindData + imageBase);
    while (index < pUnwindInfo->CountOfCodes) {
        unwindOperation = pUnwindInfo->UnwindCode[index].UnwindOp;
        operationInfo = pUnwindInfo->UnwindCode[index].OpInfo;
        switch (unwindOperation) {
            case UWOP_PUSH_NONVOL:
                stackFrame.TotalStackSize += 8;
                if (RBP_OP_INFO == operationInfo) {
                    stackFrame.PushRbp = TRUE;
                    stackFrame.CountOfCodes = pUnwindInfo->CountOfCodes;
                    stackFrame.PushRbpIndex = index + 1;
                }
                break;
            case UWOP_SAVE_NONVOL:
                index += 1;
                break;
            case UWOP_ALLOC_SMALL:
                stackFrame.TotalStackSize += ((operationInfo * 8) + 8);
                break;
            case UWOP_ALLOC_LARGE:
                index += 1;
                frameOffset = pUnwindInfo->UnwindCode[index].FrameOffset;
                if (operationInfo == 0) {
                    frameOffset *= 8;
                }
                else {
                    index += 1;
                    frameOffset += (pUnwindInfo->UnwindCode[index].FrameOffset << 16);
                }
                stackFrame.TotalStackSize += frameOffset;
                break;
            case UWOP_SET_FPREG:
                stackFrame.SetsFramePointer = TRUE;
                break;
            case UWOP_SAVE_XMM128:
                /* this can cause crashes so ignore */
                goto Cleanup;
            default:
                break;
        }

        index += 1;
    }

    if (0 != (pUnwindInfo->Flags & UNW_FLAG_CHAININFO)) {
        index = pUnwindInfo->CountOfCodes;
        if (0 != (index & 1)) {
            index += 1;
        }
        pRuntimeFunction = (PRUNTIME_FUNCTION)(&pUnwindInfo->UnwindCode[index]);
        return CalculateFunctionStackSize(pRuntimeFunction, imageBase);
    }

    stackFrame.TotalStackSize += 8;

    return (void*)stackFrame.TotalStackSize;

Cleanup:
    return NULL;
}

/*
* Wrapper for calculating the stack size for a given function.
*
* @param funcs A pointer to an IMPORTFUNCS structure.
* @param returnAddress The target return address to calculate the stack size for.
* @return A PVOID containing function stack size.
*/
void* CalculateFunctionStackSizeWrapper(IMPORTFUNCS * funcs, PVOID returnAddress) {
    RtlLookupFunctionEntryPtr RtlLookupFunctionEntry = (RtlLookupFunctionEntryPtr)funcs->GetProcAddress(funcs->GetModuleHandleA("ntdll"), "RtlLookupFunctionEntry");
    PRUNTIME_FUNCTION pRuntimeFunction = NULL;
    DWORD64 ImageBase = 0;
    PUNWIND_HISTORY_TABLE pHistoryTable = NULL;

    if (!returnAddress) {
        goto Cleanup;
    }

    pRuntimeFunction = RtlLookupFunctionEntry((DWORD64)returnAddress, &ImageBase, pHistoryTable);
    if (NULL == pRuntimeFunction) {
        goto Cleanup;
    }
    return CalculateFunctionStackSize(pRuntimeFunction, ImageBase);

Cleanup:
    return NULL;
}

/*
* Initialise the fake stack frame.
*
* @param funcs A pointer to an IMPORTFUNCS structure.
* @param stackFrame A pointer to a SYNTHETIC_STACK_FRAME structure.
* @return A BOOL indicating success.
*/
BOOL InitFrameInfo(IMPORTFUNCS * funcs, PSYNTHETIC_STACK_FRAME stackFrame) {
    void* pModuleFrame1 = (PVOID)funcs->GetModuleHandleA("Kernel32.dll");
    void* pModuleFrame2 = (PVOID)funcs->GetModuleHandleA("Ntdll.dll");
    void* pModuleGadget = (PVOID)funcs->GetModuleHandleA("Kernelbase.dll");

    if (!pModuleFrame1 || !pModuleFrame2 || !pModuleGadget) {
        //dprintf(funcs, "InitFrameFailed!\n");
        return FALSE;
    }

    stackFrame->Frame1.ModuleAddress = pModuleFrame1;
    stackFrame->Frame1.FunctionAddress = (PVOID)funcs->GetProcAddress((HMODULE)pModuleFrame1, "BaseThreadInitThunk");
    stackFrame->Frame1.Offset = 0x14;

    stackFrame->Frame2.ModuleAddress = pModuleFrame2;
    stackFrame->Frame2.FunctionAddress = (PVOID)funcs->GetProcAddress((HMODULE)pModuleFrame2, "RtlUserThreadStart");
    stackFrame->Frame2.Offset = 0x21;

    if (!stackFrame->Frame1.FunctionAddress || !stackFrame->Frame2.FunctionAddress) {
        return FALSE;
    }

    stackFrame->pGadget = pModuleGadget;  // jmp [rbx]
    if (!stackFrame->pGadget) {
        return FALSE;
    }

    return TRUE;
}

/*
* Spoofs the call stack for a target function.
*
* @param funcs A pointer to an IMPORTFUNCS structure.
* @param stackFrame A pointer to a SYNTHETIC_STACK_FRAME structure.
* @param pFunctionAddr A pointer to target function to call.
* @param pArg* The arguments for call (max 10).
* @return A PVOID containing return value.
*/
PVOID SpoofCall(IMPORTFUNCS * funcs, PSYNTHETIC_STACK_FRAME stackFrame, PVOID pFunctionAddr, PVOID pArg1, PVOID pArg2, PVOID pArg3, PVOID pArg4, PVOID pArg5, PVOID pArg6, PVOID pArg7, PVOID pArg8, PVOID pArg9, PVOID pArg10, PVOID pArg11, PVOID pArg12) {
    int attempts = 0;
    void* returnAddress = NULL;
    DRAUGR_PARAMETERS draugrParameters;
    _memset(&draugrParameters, 0, sizeof(draugrParameters));

    /* configure BaseThreadInitThunk frame */
    returnAddress = (void*)((UINT_PTR)stackFrame->Frame1.FunctionAddress + stackFrame->Frame1.Offset);
    draugrParameters.BaseThreadInitThunkStackSize = CalculateFunctionStackSizeWrapper(funcs, returnAddress);
    draugrParameters.BaseThreadInitThunkReturnAddress = returnAddress;
    if (!draugrParameters.BaseThreadInitThunkStackSize || !draugrParameters.BaseThreadInitThunkReturnAddress) {
        //dprintf(funcs, "DRAUGR: Failed to configure spoofed frame\n");
        return NULL;
    }

    /* configure RtlUserThreadStart frame */
    returnAddress = (void*)((UINT_PTR)stackFrame->Frame2.FunctionAddress + stackFrame->Frame2.Offset);
    draugrParameters.RtlUserThreadStartStackSize = CalculateFunctionStackSizeWrapper(funcs, returnAddress);
    draugrParameters.RtlUserThreadStartReturnAddress = returnAddress;
    if (!draugrParameters.RtlUserThreadStartStackSize || !draugrParameters.RtlUserThreadStartReturnAddress) {
        //dprintf(funcs, "DRAUGR: Failed to configure spoofed frame\n");
        return NULL;
    }

    /* Configure trampoline frame */
    /**
    * Ensure that the gadget stack size is bigger than 0x80, which is min
    * required to hold 10 arguments, otherwise it will crash sporadically.
    *
    * Note: The better fix is to record how many args are passed and determine
    * if the identified gadget is big enough for the specified function call.
    */

    //dprintf(funcs, "DRAUGR: Finding suitable draugr trampoline gadget...\n");
    do {
        draugrParameters.Trampoline = FindGadget(funcs, stackFrame->pGadget);
        draugrParameters.TrampolineStackSize = CalculateFunctionStackSizeWrapper(funcs, draugrParameters.Trampoline);
        /* Quick sanity check for infinite loop */
        attempts++;
        if (attempts > maxNumberOfTries) {
            //dprintf(funcs, "DRAUGR: Failed to find gadget\n");
            return NULL;
        }

    } while (draugrParameters.TrampolineStackSize == NULL || ((__int64)draugrParameters.TrampolineStackSize < 0x80));
    if (!draugrParameters.Trampoline || !draugrParameters.TrampolineStackSize) {
        return NULL;
    }
    //dprintf(funcs, "DRAUGR: Trampoline: 0x%p\n", draugrParameters.Trampoline);
    //dprintf(funcs, "DRAUGR: Trampoline func stack size: %d\n", draugrParameters.TrampolineStackSize);

    /* make the call! */
    void* retVal = DraugrSpoofStub(pArg1, pArg2, pArg3, pArg4, &draugrParameters, pFunctionAddr, 8, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11, pArg12);

    return retVal;
}

/*
* Asm stub for performing draugr call stack spoofing.
* NB This has been modified from the Sleepmask-VS version to compile with mingw.
* For a fully annotated version see: https://github.com/Cobalt-Strike/sleepmask-vs/blob/dfaf24816f2947660cdfa301ffd764e9edf12356/sleepmask-vs/library/stackspoofing.cpp#L404
*
* @param pArg1, pArg2, pArg3. pArg4, draugrParameters, pFunctionAddr, size, pArg5, pArg6. pArg7, pArg8, pArg9, pArg10, pArg11, pArg12
* @return A PVOID containing return value.
*/
__attribute__((naked))
void* DraugrSpoofStub(PVOID pArg1, PVOID pArg2, PVOID pArg3, PVOID pArg4, PVOID draugrParameters, PVOID pFunctionAddr, int size, PVOID pArg5, PVOID pArg6, PVOID pArg7, PVOID pArg8, PVOID pArg9, PVOID pArg10, PVOID pArg11, PVOID pArg12) {
  asm("pop rax \n  "
      "mov r10, rdi \n  "
      "mov r11, rsi \n  "
      "mov rdi, [rsp + 32]  \n  "
      "mov rsi, [rsp + 40] \n  "
      "mov [rdi + 24], r10 \n  "
      "mov [rdi + 88], r11 \n  "
      "mov [rdi + 96], r12 \n  "
      "mov [rdi + 104], r13 \n  "
      "mov [rdi + 112], r14  \n  "
      "mov [rdi + 120], r15 \n  "
      "mov r12, rax \n  "
      "xor r11, r11 \n  "
      "mov r13, [rsp + 0x30] \n  "
      "mov r14, 0x200 \n  "
      "add r14, 8 \n  "
      "add r14, [rdi + 56] \n  "
      "add r14, [rdi + 48] \n  "
      "add r14, [rdi + 32] \n  "
      "sub r14, 0x20 \n  "
      "mov r10, rsp \n  "
      "add r10, 0x30 \n  "
      "looping: \n  "
      "xor r15, r15  \n  "
      "cmp r11d, r13d \n  "
      "je finish \n  "
      "sub r14, 8 \n  "
      "mov r15, rsp \n  "
      "sub r15, r14 \n  "
      "add r10, 8 \n  "
      "push qword ptr [r10] \n  "
      "pop qword ptr [r15] \n  "
      "add r11, 1 \n  "
      "jmp looping \n  "
      "finish: \n  "
      "sub rsp, 0x200 \n  "
      "push 0 \n  "
      "sub rsp, [rdi + 56] \n  "
      "mov r11, [rdi + 64] \n  "
      "mov [rsp], r11 \n  "
      "sub rsp, [rdi + 32] \n  "
      "mov r11, [rdi + 40] \n  "
      "mov [rsp], r11 \n  "
      "sub rsp, [rdi + 48] \n  "
      "mov r11, [rdi + 80] \n  "
      "mov [rsp], r11 \n  "
      "mov r11, rsi     \n  "
      "mov [rdi + 8], r12 \n  "
      "mov [rdi + 16], rbx \n  "
      "lea rbx, [rip + fixup] \n  "
      "mov [rdi], rbx  \n  "
      "mov rbx, rdi \n  "
      "mov r10, rcx \n  "
      "mov rax, [rdi + 72] \n  "
      "jmp r11 \n  "
      "fixup: \n  "
      "mov rcx, rbx \n  "
      "add rsp, 0x200 \n  "
      "add rsp, [rbx + 48]  \n  "
      "add rsp, [rbx + 32] \n  "
      "add rsp, [rbx + 56] \n  "
      "mov rbx, [rcx + 16]  \n  "
      "mov rdi, [rcx + 24] \n  "
      "mov rsi, [rcx + 88] \n  "
      "mov r12, [rcx + 96] \n  "
      "mov r13, [rcx + 104] \n  "
      "mov r14, [rcx + 112]  \n  "
      "mov r15, [rcx + 120]  \n  "
      "push rax \n  "
      "xor rax, rax\n  "
      "pop rax \n  "
      "jmp qword ptr [rcx + 8] \n  "
);
}

/*
* Entry point for Draugr PIC callgate.
* NB The entry point is slightly different to Sleepmask-VS as we do not need BEACON_INFO for this use case.
*
* @param funcs A pointer to an IMPORTFUNCS structure.
* @param functionCall A pointer to a FUNCTION_CALL structure.
*/
void Callgate(IMPORTFUNCS * funcs, PFUNCTION_CALL functionCall) {
    /* Set up required structs */
    DRAUGR_FUNCTION_CALL draugrCall = { 0 };
    SYNTHETIC_STACK_FRAME stackFrame = { 0 };
    _memset(&draugrCall, 0, sizeof(DRAUGR_FUNCTION_CALL));
    _memset(&stackFrame, 0, sizeof(stackFrame));

    /* Initialize Draugr call stack */
    if (!InitFrameInfo(funcs, &stackFrame)) {
        goto Cleanup;
    }

    /* Set target stack frame and call gate entry point */
    draugrCall.StackFrame = &stackFrame;
    draugrCall.SpoofCall = (PVOID)SpoofCall;
    draugrCall.FunctionCall = functionCall;

    /* Debug output if required... */
    dprintf(funcs, "DRAUGR: Target func ptr: %p\n", draugrCall.FunctionCall->functionPtr);
    dprintf(funcs, "DRAUGR: No of args: %d\n", draugrCall.FunctionCall->numOfArgs);
    for (int i = 0; i < draugrCall.FunctionCall->numOfArgs; i++){
            dprintf(funcs, "DRAUGR: Arg %d -> %p\n", i, draugrCall.FunctionCall->args[i]);
    }

    /* Make the call! */
    DraugrGate(funcs, &draugrCall);
    dprintf(funcs, "DRAUGR: Ret Value: %p\n", draugrCall.FunctionCall->retValue);

Cleanup:
    return;
}

/*
* Debug build - you can use this to debug in WinDbg.
*/
#if DEBUG_EXE
/*
* Debug entry point. This is a basic harness which will mock a LoadLibrary call.
*/
void dbg_go(){
    /* Local vars to mock call */
    FUNCTION_CALL dbgFunctionCall = { 0 };
    IMPORTFUNCS dbgFuncs = { 0 };
    char dllPath[] = "C:\\Windows\\System32\\ResourceMapper.dll";

    /* Configure IMPORTFUNCS */
    dbgFuncs.LoadLibrary = LoadLibraryA;
    dbgFuncs.GetProcAddress = GetProcAddress;
    dbgFuncs.GetModuleHandleA = GetModuleHandleA;

    /* Configure FUNCTION_CALL */
    dbgFunctionCall.functionPtr = LoadLibraryA;
    dbgFunctionCall.bMask = FALSE;
    dbgFunctionCall.numOfArgs = 1;
    dbgFunctionCall.args[0] = (ULONG_PTR)dllPath;

    /* Invoke PIC draugr stub */
    go(&dbgFuncs, &dbgFunctionCall);
   }
#endif

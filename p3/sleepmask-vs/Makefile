CPP="$(VSAPPIDDIR)..\..\VC\Tools\Llvm\bin\clang-cl.exe"

!IF !EXISTS($(CPP))
!ERROR Unable to locate clang-cl.exe. Please correct the CPP variable in the Makefile.
!ENDIF

CFLAGS=/c /GS- /GF- /std:c++20 /clang:-fno-jump-tables
DEBUGCFLAGS=/Zi /MTd /D_DEBUG /EHsc /std:c++20

!IF "$(PROCESSOR_ARCHITECTURE)" != "x86" && "$(PROCESSOR_ARCHITECTURE)" != "AMD64"
!ERROR Only x86 and AMD64 architectures are supported or the PROCESSOR_ARCHITECTURE environment variable is not set.
!ELSEIF "$(PROCESSOR_ARCHITECTURE)" == "AMD64"
CFLAGS=$(CFLAGS) -m64
DEBUGCFLAGS=$(DEBUGCFLAGS) -m64
OUTDIR=..\x64\Release\ 
IMDIR=x64\Release\ 
DOUTDIR=..\x64\Debug\ 
DIMDIR=x64\Debug\ 
OUTEXT=.x64.o
!ELSE
CFLAGS=$(CFLAGS) -m32
DEBUGCFLAGS=$(DEBUGCFLAGS) -m32
OUTDIR=..\Release\ 
IMDIR=Release\ 
DIMDIR=Debug\ 
DOUTDIR=..\Debug\ 
OUTEXT=.x86.o
!ENDIF

all: *.cpp
    @$(MAKE) /A $(patsubst %.c,%.obj, $(patsubst %.cpp, %.obj, $(patsubsti %, $(IMDIR)\%, $**)))
    @if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"
    copy "$(IMDIR)\*.obj" "$(OUTDIR)"
    del /F "$(OUTDIR)\*$(OUTEXT)"
    ren "$(OUTDIR)*.obj" "*$(OUTEXT)"

all-debug: *.cpp
    @$(MAKE) /A $(patsubst %.c,%.exe, $(patsubst %.cpp, %.exe, $(patsubsti %, $(DOUTDIR)\%, $**)))

.cpp{$(IMDIR)}.obj:
    @if not exist "$(IMDIR)" mkdir "$(IMDIR)"
    $(CPP) $(CFLAGS) /Fo"$@" $<

.cpp{$(DOUTDIR)}.exe:
    @if not exist "$(DIMDIR)" mkdir "$(DIMDIR)"
    @if not exist "$(DOUTDIR)" mkdir "$(DOUTDIR)"
    $(CPP) $(DEBUGCFLAGS) /Fo$(DIMDIR) /Fd$(DIMDIR) /Fe"$@" $< ../bof-vs/BOF-Template/base/mock.cpp

clean:
     @if exist "$(DIMDIR)" rmdir /Q /S "$(DIMDIR)"
     @if exist "$(IMDIR)" rmdir /Q /S "$(IMDIR)"
     @if exist "$(OUTDIR)" rmdir /Q /S "$(OUTDIR)"
     @if exist "$(DOUTDIR)" rmdir /Q /S "$(DOUTDIR)"

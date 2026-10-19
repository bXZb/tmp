CC_64=x86_64-w64-mingw32-gcc

all: bin/loader.x64.o

bin:
	mkdir bin

#
# x64 targets
#
bin/loader.x64.o: bin
	$(CC_64) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -c src/loader.c -o bin/loader.x64.o

	$(CC_64) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -c src/guardexec.c -o bin/guardexec.x64.o
	$(CC_64) -DWIN_X64 -DDEBUG_EXE=1 -g -shared -masm=intel -Wall -Wno-pointer-arith -lwininet -c src/guardexec.c -o bin/guardexec_dbg.x64.o
	$(CC_64) -DWIN_X64 -nostartfiles -g -Wl,-e,dbg_go bin/guardexec_dbg.x64.o -lwininet -o bin/guardexec.x64.exe

	$(CC_64) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -fno-jump-tables -c src/draugr/draugr.c -o bin/draugr.x64.o
	$(CC_64) -DWIN_X64 -DDEBUG_EXE=1 -g -shared -masm=intel -Wall -Wno-pointer-arith -fno-jump-tables -c src/draugr/draugr.c -o bin/draugr_dbg.x64.o
	$(CC_64) -DWIN_X64 -nostartfiles -g -Wl,-e,dbg_go bin/draugr_dbg.x64.o -o bin/draugr.x64.exe

#
# Other targets
#
clean:
	rm -f bin/*

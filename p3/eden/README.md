# Eden

<img src="eden_baby.png" alt="drawing" width="600"/>

Eden loader is a PoC UDRL for Cobalt Strike built with [Crystal Palace](https://tradecraftgarden.org/crystalpalace.html) that combines Raphael Mudge's [page streaming technique](https://tradecraftgarden.org/pagestream.html) with a modular call gate (currently a PIC version of the [Sleepmask-VS Draugr callgate BOF](https://github.com/Cobalt-Strike/sleepmask-vs/blob/dfaf24816f2947660cdfa301ffd764e9edf12356/sleepmask-vs/draugr-sleepmask.cpp)).

The goal of Eden loader is to:
* Demonstrate the **power of using Crystal Palace to combine (and reuse) 'capabilities' to rapidly develop custom loaders/tooling**
* Serve as an example resource for others to build on
* Help security practitioners understand how UDRLs work (it is fully debuggable)
* Inform/stimulate the [security conversation](https://aff-wg.org/2025/03/13/the-security-conversation/)

For more information on Eden loader see the accompanying [blog](https://www.cobaltstrike.com/blog/playing-in-the-tradecraft-garden-of-beacon) for more information.

**Note:** The purpose of Eden is to demonstrate the idea of using Crystal Palace to combine different 'units of execution' (i.e. capabilities) to create custom loaders. It is *not* intended to be a fully fledged 'evasive' loader and hence it lacks a number of basic OPSEC features by design. For example, it uses RWX memory and does not [track/mask](https://github.com/klezVirus/titanldr-ng/blob/master/hooks/HeapAlloc.c#L39) Beacon's heap memory (and hence it is vulnerable to YARA sigs such as [this](https://github.com/roadwy/DefenderYara/blob/95d7a68c353d805e68276acd6cf75ec5db4703b4/HackTool/Win64/CobaltStrike/HackTool_Win64_CobaltStrike_G_.yar)).

## Quick Start Guide

**Note:** This quick start guide assumes you have [downloaded/built](https://tradecraftgarden.org/crystalpalace.html) Crystal Palace and completed the following [steps](https://tradecraftgarden.org/wslsetup.html) to configure your development environment.

1. You will first need to set the following Malleable C2 setting:
```
stage {
    set sleep_mask "false";
}
```
2. Open a `WSL` terminal in the repository root and build Eden loader: `make clean; make`.
2. Copy `crystalpalace.jar` to your Cobalt Strike client directory.
3. Load `eden.cna` into the Cobalt Strike client.
4. When you next export a payload, Eden will automatically be applied. You can verify this by checking the output in the script console:
```
[14:55:55] [*] Generating Payload: HTTP -- Type: HTTP -- Arch: x64 -- Exit Function: Thread -- System Call: None -- HTTP Library: wininet
[14:55:56] [EDEN] Parsing C:\Users\wb\Desktop\eden\eden.spec...
[14:55:56] [EDEN] Applying eden ldr spec...
[14:55:56] [EDEN] Payload Size: 387060 bytes
[14:55:56] [*] Using user modified reflective DLL! DLLName=resources/beacon.x64.rl0k.dll Arch=x64
```
**Note:** Eden supports HTTP(S), DNS, and Pivot Beacons.

## Debug Guide

One limitation with Crystal Palace at the time of release is that it currently only supports object files built with `mingw`. If you try and use a COFF built with MSVC or Clang you will typically get relocation errors. This can be frustrating as `mingw` does not directly support `pdb` files and hence it can be difficult to debug when writing complex Windows tradecraft. However, you can add the `-g` option to your `Makefile` to embed debug information in executable builds. This makes it possible to step through your code in `WinDbg`. For more information on this process is it recommended to read the following [blog](https://rastamouse.me/debugging-the-tradecraft-garden/) by Rastamouse.

This repository uses the approach above to build a debug build of Draugr (`draugr.x64.exe`) and the page streaming code (`guardexec.x64.exe`) by default. The Draugr debug build has no depedencies and so will build out the box, however if you wish to step through/debug the page streaming/IAT hook code you will need to follow the steps below:

**1. Export Beacon without a loader**:
* Load `debug/export_beacon_with_no_ldr.cna` into your CS client and export a stageless raw x64 (HTTP) Beacon into the `/eden/debug/` directory. This will export a Beacon DLL *without* a reflective loader which we can use to mock the `guardexec` entry point.
* (In WSL) `$ xxd -i ./beacon_x64.bin > debug_beacon.h`

**2. Export Draugr PIC stub from Crystal Palace**:
* To do this you will need to run Crystal Palace from WSL via `$ ./piclink /<path>/eden/debug/draugr.spec x64 /<path>/eden/debug/draugr.bin`. This will use Crystal Palace to spit out *only* the Draugr PIC stub which we can use for mocking the guardexec entry point.
* (In WSL) `$ xxd -i ./draugr.bin > debug_druagr.h`

**3. Start Debugging in WinDbg**
* Rebuild Eden after performing the steps above: `make clean;make`
* Open `WinDbg` and select launch executable (`/eden/bin/draugr.exe` or `/eden/bin/guardexec.exe`)
* Select `Open source file` and select the relevant `.c` file (i.e. `guardexec.c` if debugging the page streaming code).
* You can now step through / set breakpoints for either the stand alone call stack spoofing Draugr binary or a live Beacon with IAT hooks. As a warning, the masking will not work correctly in debug mode as it requires Crystal Palace to patch in a key during payload generation.

**Note:** There is no debug executable for the loader as there is no obvious way to get Crystal Palace to export debug payloads for things like encrypted DLLs and their keys. Hence, it becomes non-trivial to pass in mocked 'encrypted' PIC buffers.

## Design Considerations

1. Eden loader is primarily intended to demonstrate the power of Crystal Palace by combining different **'capabilties'** to create a novel loader. This idea could be taken much further than it is in this repo (i.e. a 'static' PIC loader which is fully customisable via COFF modules (guardrails, callgates, sleep obfuscation etc.)).

2. Eden loader uses a PIC version of Draugr explicitly **so it can spoof every call of Beacon's lifecycle** (i.e. the `VirtualAlloc` / `LoadLibrary` calls used during the reflective loading process). In some cases, this may be overkill (i.e. an EDR doesn't care about unbacked calls to `LoadLibrary` etc.) in which case this could be modified to use a PICO (=='BOF') equivalent of Draugr which is much simpler.

2. Eden loader deliberately attempts to keep the 'Callgate' de-coupled from the loader. This is by design for **modularity**, as the idea is you could swap in and out BeaconGate/callgate BOFs. Hence, the Draugr callgate code is all self contained in its own object file. By *replacing* this with another 'capability' you could drastically change Eden's TTPs.

3. Eden loader does not use more recent features of Crystal Palace by choice. As an example, the `mergelib` can be used with Crystal Palace's shared library, `LibTCG`. However, note that this means you lose the ability to debug your code. 

## Troubleshooting

The page stream technique can impact Beacon performance for certain commands (i.e. by default, process injection will take **~1min(!)** with 4 visible pages configured). You can increase `#define MAXVISIBLE` in `guardexec.h` to resolve most issues (the default value for Eden is 6). As a general FYI, page streaming can cause issues with process injection techniques (especially if they rely on timing).

## Why Release This?

https://aff-wg.org/2025/03/13/the-security-conversation/

## Related Work/Credit

Eden loader is built with Crystal Palace (https://tradecraftgarden.org/crystalpalace.html) and makes use of the following projects:

* Raphael Mudge's page streaming technique: https://tradecraftgarden.org/pagestream.html.
* NtDallas' Draugr call stack spoofing technique: https://github.com/NtDallas/Draugr.
* [Sleepmask-VS](https://github.com/Cobalt-Strike/sleepmask-vs) which contains example BOF callgates. These 'capabilities' were repurposed and modified to compile to PIC for use with Crystal Palace.
* Rastamouse's [Crystal-Kit](https://github.com/rasta-mouse/Crystal-Kit) is a similar project that also utilises a Draugr BeaconGate BOF but with a different implementation.

Lastly, h/t to [@rastamouse](https://x.com/_RastaMouse) who's [blogging](https://rastamouse.me/debugging-the-tradecraft-garden/) on Crystal Palace helped during development.


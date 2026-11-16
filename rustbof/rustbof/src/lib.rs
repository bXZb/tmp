#![no_std]

extern crate alloc;
use alloc::format;
use bofentry::bof;
use bofhelper::{beacon_print, import_function, BofData, CALLBACK_OUTPUT};

import_function!(KERNEL32!OutputDebugStringA(s: *const u8));

// you can specify the export name in the proc macro or just use it bare
// to have it use the function name!
#[bof(entrypoint)]
fn entry(mut data: BofData) {
    unsafe { OutputDebugStringA("Hello world!\n\0".as_ptr()) };
    let s = data.get_str();
    let _int = data.get_int();
    let asdf = format!("my string = {}\0", s);
    beacon_print!("Hello world! {}", asdf);
    unsafe { OutputDebugStringA(asdf.as_ptr()) };
}

/* CRT shims: the BOF loader leaves undefined CRT symbols unresolved.
 * These provide the memcmp/memcpy/strlen that LLVM emits for the demo,
 * and a no-op _Unwind_Resume (panic=abort means it is never called). */
#[no_mangle]
pub extern "C" fn memcmp(s1: *const u8, s2: *const u8, n: usize) -> i32 {
    unsafe {
        let a = core::slice::from_raw_parts(s1, n);
        let b = core::slice::from_raw_parts(s2, n);
        for i in 0..n {
            if a[i] != b[i] {
                return (a[i] as i32) - (b[i] as i32);
            }
        }
    }
    0
}

#[no_mangle]
pub extern "C" fn memcpy(dst: *mut u8, src: *const u8, n: usize) -> *mut u8 {
    unsafe {
        let d = core::slice::from_raw_parts_mut(dst, n);
        let s = core::slice::from_raw_parts(src, n);
        for i in 0..n {
            d[i] = s[i];
        }
    }
    dst
}

#[no_mangle]
pub extern "C" fn memset(dst: *mut u8, val: i32, n: usize) -> *mut u8 {
    unsafe {
        let d = core::slice::from_raw_parts_mut(dst, n);
        for i in 0..n {
            d[i] = val as u8;
        }
    }
    dst
}

#[no_mangle]
pub extern "C" fn _Unwind_Resume() {
    loop {}
}

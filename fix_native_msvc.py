# -*- coding: utf-8 -*-
r"""fix_native_msvc.py - convert GCC-isms in AceLdr's native.h to MSVC."""
import io
import re

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\aceldr\src\native.h")
s = io.open(P, encoding="utf-8", errors="replace").read()

pat = re.compile(
    r"typedef struct __attribute__\(\(packed\)\)" + chr(10) +
    r"\{" + chr(10) +
    r"(.*?)" + chr(10) +
    r"\} (\w+), \*(\w+);", re.S)

count = 0
def repl(m):
    global count
    count += 1
    fields = m.group(1)
    name = m.group(2)
    pname = m.group(3)
    return ("#pragma pack(push, 1)" + chr(10) +
            "typedef struct {" + chr(10) +
            fields + chr(10) +
            "} " + name + ", *" + pname + ";" + chr(10) +
            "#pragma pack(pop)")

s = pat.sub(repl, s)
print("converted %d packed structs" % count)
print("remaining packed attr:", s.count("__attribute__((packed))"))

io.open(P, "w", encoding="utf-8", newline="").write(s)

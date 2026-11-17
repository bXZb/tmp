# -*- coding: utf-8 -*-
"""pack_iea.py - pack iea.x64.o with the ziiiiizzzib contract via the XTF
bof_pack module API (blob arg = raw assembly bytes)."""
import importlib.util
import sys

spec = importlib.util.spec_from_file_location(
    "bofpack",
    r"C:\Users\Administrator\Desktop\cobaltstrike\_vendor\Cobaltstrike_BOFLoader\bof_pack.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

coff = m.COFFParser(open(r"C:\Users\Public\iea.x64.o", "rb").read())
bb = m.BlobBuilder(coff)

asm = open(r"C:\Users\Public\TestAssembly.exe", "rb").read()
args = ["", 0, 1, 1, 0, 1, "obslot", "obpipe", "IEA-TEST-OK", len(asm), asm]
packed = bb.pack_args("ziiiiizzzib", args)
blob = bb.build("go", packed)
open(r"C:\Users\Public\iea_packed.bin", "wb").write(blob)
print("packed:", len(blob), "bytes")

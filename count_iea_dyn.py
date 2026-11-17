# -*- coding: utf-8 -*-
"""count_iea_dyn.py - count IEA dynamic API (Module$Proc) references."""
import re

d = open(r"C:\Users\Public\iea.x64.o", "rb").read()
pat = re.compile(rb"([A-Za-z0-9_]+)" + b"\x24" + rb"([A-Za-z0-9_]+)")
names = set()
for mod, proc in pat.findall(d):
    s = mod.decode() + "$" + proc.decode()
    if s.startswith("__") or "Beacon" in s or "imp_" in s:
        continue
    names.add(s)

dyn = {}
for s in names:
    mod, proc = s.split("$", 1)
    if mod in ("MSVCRT", "KERNEL32", "ADVAPI32", "SHELL32", "OLEAUT32",
               "OLE32", "MSCOREE", "USER32"):
        dyn.setdefault(mod, set()).add(proc)

total = sum(len(v) for v in dyn.values())
print("unique dynamic API calls:", total)
for k in sorted(dyn):
    print(" ", k, len(dyn[k]))

# -*- coding: utf-8 -*-
"""count_iea_dyn2.py - count __imp_Module$Proc dynamic imports in iea.o."""
import re

d = open(r"C:\Users\Public\iea.x64.o", "rb").read()
pat = re.compile(rb"__imp_([A-Za-z0-9_]+)\x24([A-Za-z0-9_]+)")
dyn = {}
for mod, proc in pat.findall(d):
    s = mod.decode() + "$" + proc.decode()
    dyn.setdefault(mod, set()).add(proc)

total = sum(len(v) for v in dyn.values())
print("unique dynamic API imports:", total)
for k in sorted(dyn):
    print(" ", k, len(dyn[k]))

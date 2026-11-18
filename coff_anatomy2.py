# -*- coding: utf-8 -*-
"""coff_anatomy2.py - correct COFF parse of iea.x64.o: section-header field
order Name[8] VS VA RS PR RelocPtr LinePtr RelocN LineN Characteristics."""
import struct

d = open(r"C:\Users\Public\iea.x64.o", "rb").read()
nsec = struct.unpack_from("<H", d, 2)[0]
symoff = struct.unpack_from("<I", d, 8)[0]
nsym = struct.unpack_from("<I", d, 12)[0]
strs = d[symoff + nsym * 18:]

secs = []
so = 20
for i in range(nsec):
    raw = d[so:so+8].rstrip(b"\x00").decode(errors="replace")
    vs, va, rs, pr, rp, lp, rn, ln = struct.unpack_from("<IIIIIIHH", d, so + 8)
    name = raw
    if raw.startswith("/"):
        off = int(raw[1:])
        name = strs[off:strs.find(b"\x00", off)].decode(errors="replace")
    secs.append((name, pr, rs, rp, rn))
    so += 40

merged = {}
for name, pr, rs, rp, rn in secs:
    merged.setdefault(name.split("$")[0], [0, 0])
    merged[name.split("$")[0]][0] += rs
    merged[name.split("$")[0]][1] += rn
print("merged section families (by $-root):")
for k, (rs, rn) in merged.items():
    print("  %-10s rawsize=%-7d relocs=%d" % (k, rs, rn))

def sym_name(off):
    end = strs.find(b"\x00", off)
    return strs[off:end].decode(errors="replace")

# symbols with aux skipping (correct)
syms = []
i = 0
while i < nsym:
    o = symoff + i * 18
    nm = d[o:o+8]
    if nm[0:1] == b"\x00":
        noff = struct.unpack_from("<I", d, o + 4)[0]
        name = sym_name(noff)
    else:
        name = nm.rstrip(b"\x00").decode(errors="replace")
    value, secnum, typ, sclass = struct.unpack_from("<IhHB", d, o + 8)
    naux = d[o + 17]
    syms.append((name, value, secnum, sclass))
    i += 1 + naux

print("\ntotal symbols (aux-skipped):", len(syms))
cnt = 0
for name, value, secnum, sclass in syms:
    if name.startswith("??_C@") and cnt < 4:
        secname = secs[secnum - 1][0] if 0 < secnum <= len(secs) else "?"
        print("  ??_C@ sec=%d(%s) class=%d value=%#x" % (secnum, secname, sclass, value))
        cnt += 1

# relocations targeting ??_C@ / __xmm@ symbols
print("\nrelocs targeting ??_C@ or __xmm@ (host section -> symbol):")
hits = {}
for si, (name, pr, rs, rp, rn) in enumerate(secs):
    if rn == 0 or rp == 0:
        continue
    for j in range(rn):
        rva, symidx, rtype = struct.unpack_from("<IIH", d, rp + j * 10)
        if symidx < len(syms):
            sname = syms[symidx][0]
            if sname.startswith("??_C@") or sname.startswith("__xmm@"):
                hits.setdefault((name, sname.startswith("__xmm@")), 0)
                hits[(name, sname.startswith("__xmm@"))] += 1
for (host, isxmm), n in sorted(hits.items()):
    print("  host=%-12s %s x%d" % (host, "XMM" if isxmm else "STRLIT", n))

# -*- coding: utf-8 -*-
"""coff_anatomy.py - dump iea.x64.o sections, symbols, and relocations to see
which section the ??_C@ string-literal relocs resolve to (TS OBJExecutable
classifies by relocation-section name and '??_C@...' fell to Unknown)."""
import struct

d = open(r"C:\Users\Public\iea.x64.o", "rb").read()
nsec = struct.unpack_from("<H", d, 2)[0]
symoff = struct.unpack_from("<I", d, 8)[0]
nsym = struct.unpack_from("<I", d, 12)[0]
strs = d[symoff + nsym * 18:]

secs = []
so = 20
for i in range(nsec):
    name = d[so:so+8].rstrip(b"\x00").decode(errors="replace")
    vs, va, rs, pr, reloc_p, reloc_n = struct.unpack_from("<IIIIHH", d, so + 8)
    if name.startswith("/"):
        off = int(name[1:])
        end = strs.find(b"\x00", off)
        name = strs[off:end].decode(errors="replace")
    secs.append((name, pr, rs, reloc_p, reloc_n))
    so += 40
print("sections:")
for i, (n, pr, rs, rp, rn) in enumerate(secs, 1):
    print("  %2d %-12s praw=%-7d rsize=%-7d relocs@%d x%d" % (i, n, pr, rs, rp, rn))

def sym_name(off):
    if off == 0:
        return ""
    if off < 4:
        return "?err?"
    end = strs.find(b"\x00", off - 4)
    return strs[off - 4:end].decode(errors="replace")

syms = []
for i in range(nsym):
    o = symoff + i * 18
    nm = d[o:o+8]
    if nm[0:1] == b"\x00":
        noff = struct.unpack_from("<I", d, o + 4)[0]
        name = sym_name(noff)
    else:
        name = nm.rstrip(b"\x00").decode(errors="replace")
    value, secnum, typ, sclass = struct.unpack_from("<IhHB", d, o + 8)
    syms.append((name, value, secnum, sclass))
    # skip aux symbols
    naux = d[o + 17]
    for a in range(naux):
        i += 1

# failing symbol sample
print("\n??_C@ symbols (first 5):")
cnt = 0
for name, value, secnum, sclass in syms:
    if name.startswith("??_C@") and cnt < 5:
        secname = secs[secnum - 1][0] if 0 < secnum <= len(secs) else "?"
        print("  sec=%d(%s) class=%d value=%#x %s" % (secnum, secname, sclass, value, name[:60]))
        cnt += 1

# relocations that reference ??_C@ symbols
print("\nrelocs targeting ??_C@ (per section):")
for si, (n, pr, rs, rp, rn) in enumerate(secs, 1):
    if rn == 0 or rp == 0:
        continue
    for j in range(rn):
        rva, symidx, rtype = struct.unpack_from("<IIH", d, rp + j * 10)
        if symidx < len(syms) and syms[symidx][0].startswith("??_C@"):
            print("  in section %s: reloc@%#x type=%d -> %s" % (n, rva, rtype, syms[symidx][0][:50]))

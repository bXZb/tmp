# -*- coding: utf-8 -*-
r"""fix_iea_bridge.py - C-50: iea command must use &beacon_inline_execute
(BeaconBridge, UserSpecifiedFull passes bofBytes+entry+argsBytes). The old
call went through &binline_execute (file variant) which reads only
(file, entry) and DROPS the 4th bofArgs blob - args arrived as "go\0"."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8").read()

L = chr(10)
TAB = chr(9)

old = (TAB + TAB + 'binline_execute($bid, $bof, "go", $bofArgs);')
assert old in s, "old iea dispatch not found"
new = (TAB + TAB + '# C-50: beacon_inline_execute carries the args blob;' + L +
       TAB + TAB + '# binline_execute (file variant) dropped arg 4.' + L +
       TAB + TAB + 'beacon_inline_execute($bid, $bof, "go", $bofArgs);')
s = s.replace(old, new, 1)

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("iea dispatch fixed (no BOM)")

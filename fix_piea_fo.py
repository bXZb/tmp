# -*- coding: utf-8 -*-
r"""fix_piea_fo.py - rewrite the piea build step to use quoted /Fo path."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\.github\workflows\r2-round2.yml")
s = io.open(P, encoding="utf-8").read()

BS = chr(92)
L = chr(10)

old = [l for l in s.split(L) if "cl /nologo" in l and "piea" in l][0]
new = ('          if not exist piea' + BS + ' mkdir piea' + BS + L +
       '          cl /nologo /W0 /c /O2 /GS- /Fo"piea' + BS +
       'PatchlessinlineExecute-Assembly.obj" patchless-iea' + BS + 'src' +
       BS + 'PatchlessinlineExecute-Assembly.c || exit /b 1' + L +
       '          dir piea')
s = s.replace(old, new)
io.open(P, "w", encoding="utf-8", newline="").write(s)
print("piea cl line rewritten")

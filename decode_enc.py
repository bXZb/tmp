# -*- coding: utf-8 -*-
"""decode_enc.py - replicate SleeveSecurity.decrypt: AES-CBC(NoPadding,
IV=abcdefghijklmnop, key=SHA256(key16)[:16]) then strip [4B rand][4B len]."""
import hashlib
import struct
import sys

try:
    from Crypto.Cipher import AES
except ImportError:
    from Cryptodome.Cipher import AES

KEY16 = bytes([255, 12, 250, 65, 7, 209, 91, 48, 17, 61, 29, 43, 157, 233, 21, 109])
IV = b"abcdefghijklmnop"
digest = hashlib.sha256(KEY16).digest()
aes_key = digest[:16]
hmac_key = digest[16:32]

path = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\Public\official_ctrl\beacon.x64.dll.enc"
blob = open(path, "rb").read()
ct, tag = blob[:-16], blob[-16:]
pt = AES.new(aes_key, AES.MODE_CBC, IV).decrypt(ct)
rnd, ln = struct.unpack_from(">II", pt, 0)
data = pt[8:8 + ln]
print("enc:", len(blob), "| plaintext:", len(pt), "| declared len:", ln,
      "| head:", data[:8])
open(path + ".dec", "wb").write(data)

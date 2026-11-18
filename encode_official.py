# -*- coding: utf-8 -*-
"""encode_official.py - C-49: produce CORRECT sleeve .enc files for the
DECODED official 4.9.1 beacon DLLs (SleeveSecurity format:
AES-CBC/NoPadding IV=abcdefghijklmnop key=SHA256(key16)[:16],
plaintext=[4B rand][4B len][data]['A' pad to 16], + HmacSHA256(key16[16:]) tag)."""
import hashlib
import os
import random
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
OUT = r"C:\Users\Public\official_ctrl2"
os.makedirs(OUT, exist_ok=True)


def encode(data: bytes) -> bytes:
    body = struct.pack(">II", random.randrange(2 ** 31), len(data)) + data
    if len(body) % 16:
        body += b"A" * (16 - len(body) % 16)
    ct = AES.new(aes_key, AES.MODE_CBC, IV).encrypt(body)
    import hmac as H
    tag = H.new(hmac_key, ct, hashlib.sha256).digest()[:16]
    return ct + tag


DECODE = (r"C:\Users\Administrator\Desktop\cobaltstrike"
          r"\cobaltstrike-official\4.9.1\CrackSleeve4.9\Resource\Decode\sleeve")
for name in ("beacon.x64.dll", "beacon.x64.rl0k.dll"):
    p = os.path.join(DECODE, name)
    if not os.path.exists(p):
        print("MISSING:", name)
        continue
    d = open(p, "rb").read()
    enc = encode(d)
    open(os.path.join(OUT, name + ".enc"), "wb").write(enc)
    print(name, "raw", len(d), d[:4], "-> enc", len(enc))

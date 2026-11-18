# -*- coding: utf-8 -*-
r"""patch_objexec.py - C-50: teach OBJExecutable about MSVC COMDAT objects.

Root cause chain (IEA = MSVC cl /c):
 1. OBJParser.parseSection renames DUPLICATE section names to name+"$"+index
    (2nd ".rdata" becomes ".rdata$44"). MSVC emits 37 .rdata / 8 .text$mn /
    16 .xdata / 16 .pdata COMDAT sections, so nearly every section carries a
    parser-synthesized "$" suffix that collides with real "$"-family syntax.
 2. Relocation.getSection() then returns ".rdata$44" for the ??_C@ string
    literals living there -> OBJExecutable's ".rdata".equals(...) misses ->
    "Unknown symbol '??_C@...'".
 3. Even with classification fixed, getCode/getRData/... only carve ONE
    section, entry/r_vaddr/e_value are section-relative -> wrong in the
    merged stream.

Fix (all inside OBJExecutable, single-section mingw objects keep byte-identical
behavior because every family base is 0):
 - build famData/famBase tables in parse(): merge every section whose "$" root
   is one of .text/.rdata/.data/.xdata/.pdata/.bss (file order), recording
   each section's base offset inside its family stream;
 - getCode/getRData/getData/getXData/getPData/getBssSize read the merged
   family streams (bss keeps using sectionSize semantics);
 - getRelocations classifies by SYMBOL-section ROOT, and emits
   r_vaddr = familyBase(host) + in-section offset, e_value = familyBase(sym
   section) + in-section value;
 - getRelocSection compares by ROOT so relocations hosted in renamed
   ".text$mn$5" sections still resolve to 1026;
 - getEntryPoint adds the entry symbol's family base."""
import io

P = (r"C:\Users\Administrator\Desktop\cobaltstrike\cobaltstrike-ts"
     r"\p3-llvm-bench\src\main\java\pe\OBJExecutable.java")
s = io.open(P, encoding="utf-8").read()

# ---- 1. imports + fields ----
old = """import common.CommonUtils;
import common.MudgeSanity;
import common.Packer;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

public class OBJExecutable {
   protected OBJParser info = null;
   protected byte[] data;
   protected List errors = new LinkedList();
   protected String epfunc = "";
   protected String exesect = "";
"""
new = """import common.CommonUtils;
import common.MudgeSanity;
import common.Packer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class OBJExecutable {
   protected OBJParser info = null;
   protected byte[] data;
   protected List errors = new LinkedList();
   protected String epfunc = "";
   protected String exesect = "";
   protected Map famData = new HashMap();
   protected Map famBase = new HashMap();
   protected Map famSize = new HashMap();
   protected static final Set FAMILY_ROOTS = new HashSet(Arrays.asList(
      new String[]{".text", ".rdata", ".data", ".xdata", ".pdata", ".bss"}));

   protected static String familyRoot(String var0) {
      int var1 = var0.indexOf(36);
      return var1 > 0 ? var0.substring(0, var1) : var0;
   }

   protected void buildFamilies() {
      for (java.util.Iterator _itF = this.info.getSectionsTable().iterator(); _itF.hasNext(); ) {
         String var1 = (String)_itF.next();
         String var2 = familyRoot(var1);
         if (!FAMILY_ROOTS.contains(var2)) {
            continue;
         }
         int var3 = this.famSize.containsKey(var2) ? ((Integer)this.famSize.get(var2)).intValue() : 0;
         this.famBase.put(var1, new Integer(var3));
         byte[] var4 = this.A(var1);
         int var5 = this.info.sectionSize(var1);
         if (var4.length > 0) {
            byte[] var6 = (byte[])this.famData.get(var2);
            byte[] var7 = var6 == null ? new byte[0] : var6;
            byte[] var8 = new byte[var7.length + var4.length];
            System.arraycopy(var7, 0, var8, 0, var7.length);
            System.arraycopy(var4, 0, var8, var7.length, var4.length);
            this.famData.put(var2, var8);
         }
         this.famSize.put(var2, new Integer(var3 + var5));
      }
   }

   protected int famBaseOf(String var0) {
      Integer var1 = (Integer)this.famBase.get(var0);
      return var1 == null ? 0 : var1.intValue();
   }
"""
assert old in s, "header block not found"
s = s.replace(old, new, 1)

# ---- 2. parse(): build families ----
old = """         this.info = OBJParser.load(this.data);
         if (this.info.hasSection(".text")) {
            this.exesect = ".text";
         } else {
            this.exesect = this.info.getExeSect();
         }
"""
new = """         this.info = OBJParser.load(this.data);
         if (this.info.hasSection(".text")) {
            this.exesect = ".text";
         } else {
            this.exesect = this.info.getExeSect();
         }
         this.buildFamilies();
"""
assert old in s, "parse body not found"
s = s.replace(old, new, 1)

# ---- 3. entry point: add family base of the symbol's own section ----
old = """      for (int var2 = 0; var2 < var1; var2++) {
         String var3 = this.info.getString("Symbol." + var2 + ".e");
         if (("_" + this.epfunc).equals(var3) || this.epfunc.equals(var3)) {
            return this.info.get("Symbol." + var2 + ".e_value");
         }
      }
"""
new = """      for (int var2 = 0; var2 < var1; var2++) {
         String var3 = this.info.getString("Symbol." + var2 + ".e");
         if (("_" + this.epfunc).equals(var3) || this.epfunc.equals(var3)) {
            int var4 = this.info.get("Symbol." + var2 + ".e_value");
            int var5 = this.info.get("Symbol." + var2 + ".e_scnum");
            if (var5 > 0) {
               java.util.Iterator _itE = this.info.getSectionsTable().iterator();
               int var6 = 1;
               while (_itE.hasNext()) {
                  String var7 = (String)_itE.next();
                  if (var6 == var5) {
                     return var4 + this.famBaseOf(var7);
                  }
                  ++var6;
               }
            }
            return var4;
         }
      }
"""
assert old in s, "entry block not found"
s = s.replace(old, new, 1)

# ---- 4. section accessors -> merged families ----
old = """   public byte[] getCode() {
      byte[] var1 = this.A(this.exesect);
      if (var1.length == 0) {
         this.error("No .text section in object file");
      }

      return var1;
   }

   public byte[] getRData() {
      return this.A(".rdata");
   }

   public byte[] getData() {
      return this.A(".data");
   }

   public byte[] getXData() {
      return this.A(".xdata");
   }

   public byte[] getPData() {
      return this.A(".pdata");
   }

   public int getBssSize() {
      return this.info.sectionSize(".bss");
   }
"""
new = """   public byte[] getCode() {
      byte[] var1 = (byte[])this.famData.get(familyRoot(this.exesect));
      if (var1 == null) {
         var1 = this.A(this.exesect);
      }
      if (var1.length == 0) {
         this.error("No .text section in object file");
      }

      return var1;
   }

   public byte[] getRData() {
      byte[] var1 = (byte[])this.famData.get(".rdata");
      return var1 == null ? this.A(".rdata") : var1;
   }

   public byte[] getData() {
      byte[] var1 = (byte[])this.famData.get(".data");
      return var1 == null ? this.A(".data") : var1;
   }

   public byte[] getXData() {
      byte[] var1 = (byte[])this.famData.get(".xdata");
      return var1 == null ? this.A(".xdata") : var1;
   }

   public byte[] getPData() {
      byte[] var1 = (byte[])this.famData.get(".pdata");
      return var1 == null ? this.A(".pdata") : var1;
   }

   public int getBssSize() {
      Integer var1 = (Integer)this.famSize.get(".bss");
      return var1 == null ? this.info.sectionSize(".bss") : var1.intValue();
   }
"""
assert old in s, "accessor block not found"
s = s.replace(old, new, 1)

# ---- 5. getRelocSection by ROOT ----
old = """   public short getRelocSection(String var1) {
      if (this.exesect.equals(var1)) {
         return 1026;
      } else if (".data".equals(var1)) {
         return 1025;
      } else {
         return (short)(".pdata".equals(var1) ? 1030 : 0);
      }
   }
"""
new = """   public short getRelocSection(String var1) {
      String var2 = familyRoot(var1);
      if (familyRoot(this.exesect).equals(var2)) {
         return 1026;
      } else if (".data".equals(var2)) {
         return 1025;
      } else if (".rdata".equals(var2)) {
         return 1024;
      } else if (".xdata".equals(var2)) {
         return 1029;
      } else {
         return (short)(".pdata".equals(var2) ? 1030 : 0);
      }
   }
"""
assert old in s, "getRelocSection not found"
s = s.replace(old, new, 1)

# ---- 6. relocation loop: root classification + family-base offsets ----
old = """            if (".rdata".equals(var6.getSection())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1024);
               var1.addInt(var6.getOffset());
               var1.addInt(var6.getOffsetInSection());
            } else if (".data".equals(var6.getSection())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1025);
               var1.addInt(var6.getOffset());
               var1.addInt(var6.getOffsetInSection());
            } else if (".xdata".equals(var6.getSection())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1029);
               var1.addInt(var6.getOffset());
               var1.addInt(var6.getOffsetInSection());
            } else if (this.exesect.equals(var6.getSection())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1026);
               var1.addInt(var6.getOffset());
               var1.addInt(var6.getOffsetInSection());
            } else if (".bss".equals(var6.getSection())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1031);
               var1.addInt(var6.getOffset());
               var1.addInt(var6.getOffsetInSection());
            } else if (BOFUtils.isInternalFunction(var6.getSymbol())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(BOFUtils.findInternalFunction(var6.getSymbol()));
               var1.addInt(var6.getOffset());
               var1.addInt(0);
            } else if (BOFUtils.isDynamicFunction(var6.getSymbol())) {
"""
new = """            String var7 = familyRoot(var6.getSection());
            int var8 = this.famBaseOf(var6.getSection());
            int var9 = this.famBaseOf(var3) + var6.getOffset();
            if (".rdata".equals(var7)) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1024);
               var1.addInt(var9);
               var1.addInt(var8 + var6.getOffsetInSection());
            } else if (".data".equals(var7)) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1025);
               var1.addInt(var9);
               var1.addInt(var8 + var6.getOffsetInSection());
            } else if (".xdata".equals(var7)) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1029);
               var1.addInt(var9);
               var1.addInt(var8 + var6.getOffsetInSection());
            } else if (familyRoot(this.exesect).equals(var7)) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1026);
               var1.addInt(var9);
               var1.addInt(var8 + var6.getOffsetInSection());
            } else if (".bss".equals(var7)) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1031);
               var1.addInt(var9);
               var1.addInt(var8 + var6.getOffsetInSection());
            } else if (BOFUtils.isInternalFunction(var6.getSymbol())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(BOFUtils.findInternalFunction(var6.getSymbol()));
               var1.addInt(var9);
               var1.addInt(0);
            } else if (BOFUtils.isDynamicFunction(var6.getSymbol())) {
"""
assert old in s, "reloc loop block not found"
s = s.replace(old, new, 1)

# dynamic-function branch r_vaddr also needs the host base
old = """            } else if (BOFUtils.isDynamicFunction(var6.getSymbol())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1027);
               var1.addInt(var6.getOffset());
               var1.addInt(0);
"""
new = """            } else if (BOFUtils.isDynamicFunction(var6.getSymbol())) {
               var1.addInt(var4);
               var1.addShort(var6.getType());
               var1.addShort(1027);
               var1.addInt(var9);
               var1.addInt(0);
"""
assert old in s, "dynamic branch not found"
s = s.replace(old, new, 1)

io.open(P, "w", encoding="utf-8", newline="").write(s)
print("OBJExecutable MSVC COMDAT patch applied (6 edits)")

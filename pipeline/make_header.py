import re, sys
from pathlib import Path

def build_header(c_path):
    src = Path(c_path).read_text()
    m = re.search(r'#include\s+"([^"]+\.h)"', src)
    hname = m.group(1) if m else Path(c_path).stem + ".h"
    guard = re.sub(r"\W", "_", hname).upper()

    enums = []
    for tname in sorted(set(re.findall(r"\b(\w+_(?:Error|State)_t)\b", src))):
        prefix, kind = re.match(r"(\w+)_(Error|State)_t", tname).groups()
        names = sorted(set(re.findall(rf"\b{prefix}_{kind.upper()}_[A-Z0-9_]+\b", src)),
                       key=lambda n: (not n.endswith(("_NONE", "_IDLE")), n))
        if names:
            body = ",\n".join(f"    {n}" for n in names)
            enums.append(f"typedef enum {{\n{body}\n}} {tname};")

    protos = []
    for ret, name, args in re.findall(
            r"^(?!static\b)([A-Za-z_][\w \*]*?[\w\*])\s+(\w+)\s*\(([^;{)]*)\)\s*\{",
            src, re.M):
        if name != "main":
            protos.append(f"{ret} {name}({' '.join(args.split()) or 'void'});")

    externs = [f"extern {t} {n};" for t, n in re.findall(r"^(\w+_t)\s+(\w+)\s*;", src, re.M)]
    if "SystemCoreClock" in src:
        externs.append("extern uint32_t SystemCoreClock;")

    parts = [f"#ifndef {guard}", f"#define {guard}", "",
             "#include <stdint.h>", "#include <stdbool.h>", "#include <stddef.h>", ""]
    parts += enums + [""] + externs + [""] + protos + ["", "#endif"]
    return hname, "\n".join(parts) + "\n"

def ensure_header(c_path):
    """Create the header next to the .c file if missing. Returns its path or None."""
    hname, text = build_header(c_path)
    target = Path(c_path).parent / hname
    if target.exists():
        return None
    target.write_text(text)
    return str(target)

if __name__ == "__main__":
    print(ensure_header(sys.argv[1]) or "header already exists")

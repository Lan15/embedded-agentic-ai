import re, json, sys
from pathlib import Path

REQ_RE  = re.compile(r'/\* (REQ-\w+-\d+): (.+?) \*/')
ASIL_RE = re.compile(r'/\* (ASIL-\w+, ISO\d+-PL-\w+) \*/')
TEST_RE = re.compile(r'void (test_\w+)\(')

def build_traceability(src_dir="src", test_dir="tests/unit") -> dict:
    matrix = {}
    for f in Path(src_dir).rglob("*.c"):
        text = f.read_text()
        lines = text.splitlines()
        for i, line in enumerate(lines):
            m = REQ_RE.search(line)
            if m:
                rid, desc = m.group(1), m.group(2)
                asil = ""
                if i+1 < len(lines):
                    am = ASIL_RE.search(lines[i+1])
                    if am: asil = am.group(1)
                matrix.setdefault(rid, {"description": desc,
                    "safety_tag": asil, "files": [], "tests": []})
                matrix[rid]["files"].append(str(f))
    for f in Path(test_dir).rglob("test_*.c"):
        text = f.read_text()
        for rid in matrix:
            slug = rid.lower().replace("-", "_")
            matrix[rid]["tests"] += [
                t for t in TEST_RE.findall(text) if slug in t]
    return matrix

if __name__ == "__main__":
    m = build_traceability()
    print(json.dumps(m, indent=2))
    missing = [r for r, v in m.items() if not v["tests"]]
    if missing:
        print(f"FAIL: no tests for {missing}", file=sys.stderr)
        sys.exit(1)

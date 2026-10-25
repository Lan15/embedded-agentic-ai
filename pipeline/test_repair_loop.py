import sys, re, os, subprocess
from pathlib import Path
import ollama

MODEL = os.getenv("LLM_MODEL", "qwen2.5-coder:14b")
MAX_ROUNDS = 4
UNITY = "tests/unity"

def build_and_run(d, test):
    exe = "/tmp/module_test"
    Path(exe).unlink(missing_ok=True)
    cc = subprocess.run(["gcc", "-std=c11", "-Wall", f"-I{d}", f"-I{UNITY}",
                         str(test), f"{UNITY}/unity.c", "-o", exe],
                        capture_output=True, text=True)
    if cc.returncode != 0:
        return False, "COMPILE ERRORS:\n" + cc.stderr[:3000]
    run = subprocess.run([exe], capture_output=True, text=True)
    return run.returncode == 0, "TEST RUN OUTPUT:\n" + run.stdout[:3000]

def repair(out_dir):
    d = Path(out_dir)
    c = next(p for p in d.glob("*.c") if not p.name.startswith("test_"))
    h = c.with_suffix(".h")
    test = d / f"test_{c.name}"
    for rnd in range(1, MAX_ROUNDS + 1):
        ok, log = build_and_run(d, test)
        if ok:
            print(f"TESTS PASSED after {rnd - 1} repair round(s)")
            return True
        print(f"Round {rnd}: fixing tests...")
        prompt = (
            "Rewrite this Unity test file so it compiles and passes against the REAL module below. "
            "Do NOT change the module. Rules: use only identifiers that exist in the header or source; "
            "at the top of the test define 'static uint32_t mock_regs[16];' and "
            "'#define UART_BASE ((uintptr_t)mock_regs)' (use the module's actual base macro name) "
            "and 'uint32_t SystemCoreClock = 16000000u;' BEFORE '#include \"" + c.name + "\"' "
            "(include the .c file directly so the fake registers are used). Do not busy-wait on "
            "status bits that the fake registers never set: pre-set the status register bits in "
            "setUp. Keep setUp, tearDown and a main() with UNITY_BEGIN/RUN_TEST/UNITY_END. "
            "Return exactly one fenced code block with the full test file.\n\n"
            f"{log}\n\nHEADER:\n{h.read_text()}\n\nSOURCE:\n{c.read_text()}\n\nCURRENT TEST:\n{test.read_text()}")
        reply = ollama.chat(model=MODEL, messages=[{"role": "user", "content": prompt}],
                            options={"temperature": 0.1})["message"]["content"]
        blocks = re.findall(r"```(?:c)?\n(.*?)```", reply, re.S)
        if not blocks:
            print("No code block in reply, retrying")
            continue
        test.write_text(blocks[0].strip() + "\n")
    ok, log = build_and_run(d, test)
    print("TESTS PASSED" if ok else "STILL FAILING, review manually:\n" + log)
    return ok

if __name__ == "__main__":
    sys.exit(0 if repair(sys.argv[1]) else 1)

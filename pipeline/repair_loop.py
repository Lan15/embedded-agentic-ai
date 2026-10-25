import sys, re, glob, subprocess
from pathlib import Path
import ollama, os
sys.path.insert(0, "pipeline")
from deploy_to_jetson import deploy_to_jetson

MODEL = os.getenv("LLM_MODEL", "qwen2.5-coder:14b")
MAX_ROUNDS = 3

def capture_errors(d):
    """Run the deploy check and return (ok, compiler_output)."""
    import io, contextlib
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        ok = deploy_to_jetson(d, "mcu")
    return ok, buf.getvalue()

def blocks(text):
    return re.findall(r"```(?:c|h)?\n(.*?)```", text, re.S)

def repair(out_dir):
    d = Path(out_dir)
    c_file = next(p for p in d.glob("*.c") if not p.name.startswith("test_"))
    h_file = c_file.with_suffix(".h")
    for rnd in range(1, MAX_ROUNDS + 1):
        ok, log = capture_errors(str(d))
        if ok:
            print(f"PASSED after {rnd - 1} repair round(s)")
            return True
        print(f"Round {rnd}: fixing compiler errors...")
        prompt = (
            "Fix this embedded C module so it compiles with gcc -std=c11 -Wall -Wextra. "
            "Keep behaviour and REQ-ID comments. Every identifier used in the .c must be "
            "declared in the .h or the .c. Add extern uint32_t SystemCoreClock; to the header "
            "if used. Return exactly two fenced blocks: the full .h, then the full .c.\n\n"
            f"COMPILER OUTPUT:\n{log}\n\nHEADER:\n{h_file.read_text()}\n\nSOURCE:\n{c_file.read_text()}")
        reply = ollama.chat(model=MODEL, messages=[{"role": "user", "content": prompt}],
                            options={"temperature": 0.1})["message"]["content"]
        b = blocks(reply)
        if len(b) < 2:
            print("Model reply had no two code blocks, retrying")
            continue
        h_file.write_text(b[0].strip() + "\n")
        c_file.write_text(b[1].strip() + "\n")
    ok, _ = capture_errors(str(d))
    print("PASSED" if ok else f"STILL FAILING after {MAX_ROUNDS} rounds: review manually")
    return ok

if __name__ == "__main__":
    sys.exit(0 if repair(sys.argv[1]) else 1)

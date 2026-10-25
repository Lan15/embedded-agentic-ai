import subprocess, os, glob, sys
from pathlib import Path
from make_header import ensure_header

JETSON_HOST  = os.getenv("JETSON_HOST", "192.168.0.169").split("@")[-1]
JETSON_USER  = os.getenv("JETSON_USER", "agilanvs")
JETSON_KEY   = os.getenv("JETSON_KEY", "/home/agilanvs/.ssh/jetson_key")
FIRMWARE_DIR = f"/home/{JETSON_USER}/embedded-agentic-ai/firmware"
SSH_OPTS     = ["-i", JETSON_KEY, "-o", "StrictHostKeyChecking=no"]

def ssh(cmd):
    return subprocess.run(["ssh", *SSH_OPTS, f"{JETSON_USER}@{JETSON_HOST}", cmd],
                          capture_output=True, text=True)

def deploy_to_jetson(codegen_dir, mode="mcu"):
    c_files = [f for f in glob.glob(f"{codegen_dir}/*.c")
               if not Path(f).name.startswith("test_")]
    if not c_files:
        raise FileNotFoundError(f"No .c files found in {codegen_dir}")

    for c in c_files:
        made = ensure_header(c)
        if made:
            print(f"Generated header: {Path(made).name}")

    files = c_files + glob.glob(f"{codegen_dir}/*.h")
    ssh(f"mkdir -p {FIRMWARE_DIR} && rm -f {FIRMWARE_DIR}/*.c {FIRMWARE_DIR}/*.h")
    for f in files:
        subprocess.run(["scp", *SSH_OPTS, f,
                        f"{JETSON_USER}@{JETSON_HOST}:{FIRMWARE_DIR}/"], check=True)
        print(f"Deployed: {Path(f).name}")

    names = [Path(f).name for f in c_files]
    if mode == "mcu":
        # Register-level firmware cannot run on the Jetson: syntax/type check only
        ok = True
        for n in names:
            r = ssh(f"cd {FIRMWARE_DIR} && gcc -std=c11 -Wall -Wextra -fsyntax-only -I. {n}")
            print(r.stderr or f"{n}: no warnings")
            ok = ok and r.returncode == 0
        print("MCU syntax check:", "PASSED" if ok else "FAILED")
        return ok

    r = ssh(f"cd {FIRMWARE_DIR} && gcc -Wall -std=c11 -o firmware_out {' '.join(names)} && echo Compiled OK")
    print(r.stdout, r.stderr)
    if r.returncode != 0:
        return False
    r = ssh(f"cd {FIRMWARE_DIR} && ./firmware_out; echo exit_code=$?")
    print(r.stdout)
    return "exit_code=0" in r.stdout

if __name__ == "__main__":
    directory = sys.argv[1] if len(sys.argv) > 1 else "codegen_output"
    mode = sys.argv[2] if len(sys.argv) > 2 else "mcu"
    sys.exit(0 if deploy_to_jetson(directory, mode) else 1)

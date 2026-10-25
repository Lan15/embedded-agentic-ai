from crewai import Agent, Task, Crew, Process, LLM
import os
import re
from datetime import datetime

OLLAMA_BASE = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
LLM_MODEL = os.getenv("LLM_MODEL", "qwen2.5-coder:14b")
llm = LLM(model=f"ollama/{LLM_MODEL}", base_url=OLLAMA_BASE)

# Per-target instructions. "mcu" is the main path, "linux" the alternative.
TARGETS = {
    "mcu": {
        "coder": (
            "Write register-level bare-metal C for a Cortex-M MCU using volatile peripheral access. "
            "Wrap every peripheral base address in #ifndef <NAME>_BASE ... #endif so host tests can override it. "
            "Declare the CMSIS clock as extern uint32_t SystemCoreClock in the header."),
        "tester": (
            "Tests run on the host. Define the peripheral base macro to point at a static uint32_t array "
            "BEFORE including the module source, so no real hardware is touched."),
    },
    "linux": {
        "coder": (
            "Write Linux userspace C (POSIX, termios, /dev/ttyTHS* devices). No raw register addresses. "
            "Include a small main() that exercises the module and returns 0 on success."),
        "tester": (
            "Write plain Unity tests that call the public functions directly. Do not touch real devices."),
    },
}

def make_agents(target):
    t = TARGETS[target]
    architect = Agent(
        role="Firmware Architect",
        goal="Decompose requirements into detailed C module specs with function signatures, types, and REQ-IDs.",
        backstory="10yr embedded architect, AUTOSAR and MISRA C:2012 expert. Always assigns REQ-MCU-XXX IDs.",
        llm=llm, verbose=True)
    coder = Agent(
        role="Firmware Coder",
        goal="Write compilable MISRA C:2012 compliant C: one header block and one source block. No JSON, no prose.",
        backstory=("Expert embedded C developer for safety-critical systems. " + t["coder"] +
                   " Only include <stdint.h>, <stdbool.h>, <stddef.h>"
                   + (", POSIX headers" if target == "linux" else "") +
                   " and the module's own header. Never invent other driver headers. "
                   "This is a research and educational project, so always write the full code."),
        llm=llm, verbose=True)
    test_writer = Agent(
        role="Test Engineer",
        goal="Write complete Unity unit test files for all public functions.",
        backstory=("Experienced in ThrowTheSwitch Unity for embedded C. " + t["tester"] +
                   " This is a research and educational project. Always write complete test files."),
        llm=llm, verbose=True)
    safety_officer = Agent(
        role="Safety Officer",
        goal="Review the generated C code and confirm every function has a REQ-MCU-XXX comment and ASIL tag. List any gaps.",
        backstory="Functional safety expert, ISO 13849 / IEC 62061 background. Reviews code for traceability compliance.",
        llm=llm, verbose=True)
    return architect, coder, test_writer, safety_officer

def extract_blocks(text):
    """Return all fenced code blocks (```c, ```cpp or bare) as a list of strings."""
    blocks = re.findall(r"```(?:c|cpp|h)?\n(.*?)```", text, re.DOTALL)
    return [b.strip() for b in blocks] or [text.strip()]

def split_header_source(text, module_name):
    """First block containing a FILE marker or include guard is the header."""
    header, source = None, None
    for b in extract_blocks(text):
        if header is None and re.search(r"FILE:\s*\S+\.h|^#ifndef\s+\w+_H", b, re.M):
            header = b
        elif source is None:
            source = b
    return header, source or text.strip()

def save_outputs(module_name, target, c_text, test_text, safety_report):
    out_dir = f"codegen_output/{module_name}_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    os.makedirs(out_dir, exist_ok=True)
    header, source = split_header_source(c_text, module_name)
    source = re.sub(r"^#define (\w+_BASE)\s+(.*)$", r"#ifndef \1\n#define \1 \2\n#endif", source, flags=re.M)
    open(f"{out_dir}/{module_name}.c", "w").write(source + "\n")
    if header:
        open(f"{out_dir}/{module_name}.h", "w").write(header + "\n")
    open(f"{out_dir}/test_{module_name}.c", "w").write(extract_blocks(test_text)[0] + "\n")
    open(f"{out_dir}/safety_report.txt", "w").write(safety_report)
    open(f"{out_dir}/target.txt", "w").write(target + "\n")
    print(f"\n✅ Outputs saved to: {out_dir}/  (target={target}, header={'yes' if header else 'no, generator will create it'})")
    return out_dir

def run_firmware_crew(requirement, module_name="pwm_motor", target="mcu"):
    architect, coder, test_writer, safety_officer = make_agents(target)

    t1 = Task(
        description=(f"Decompose this requirement into a C module spec (target: {target}):\n{requirement}\n"
                     f"Module name: {module_name}. List function signatures with REQ-MCU-XXX IDs, typedefs, macros."),
        expected_output="Complete C module specification with function signatures and REQ-IDs.",
        agent=architect)
    t2 = Task(
        description=(f"Using the spec, write the module as TWO fenced code blocks and nothing else.\n"
                     f"Block 1: first line /* FILE: {module_name}.h */ then the header (include guard, enums, prototypes, externs).\n"
                     f"Block 2: first line /* FILE: {module_name}.c */ then the source, which does #include \"{module_name}.h\".\n"
                     "Put /* REQ-MCU-XXX [ASIL-x]: description */ above every function."),
        expected_output="Two code blocks: the .h file and the .c file.",
        agent=coder)
    t3 = Task(
        description=("Using the C code from the previous task, write one complete Unity test file for every public "
                     "function with TEST_GROUP-free plain Unity style: setUp, tearDown, 2+ tests per function, "
                     "and a main() using UNITY_BEGIN/RUN_TEST/UNITY_END."),
        expected_output="One complete Unity test .c file.",
        agent=test_writer)
    t4 = Task(
        description=("Review the code from task 2. Check that every function has a /* REQ-MCU-XXX */ comment, "
                     "an ASIL tag, and that REQ-IDs are unique. List gaps, or reply APPROVED."),
        expected_output="Safety sign-off: list of gaps or APPROVED.",
        agent=safety_officer)

    crew = Crew(agents=[architect, coder, test_writer, safety_officer],
                tasks=[t1, t2, t3, t4], process=Process.sequential,
                verbose=True, tracing=True)
    result = crew.kickoff()
    save_outputs(module_name, target, str(t2.output.raw), str(t3.output.raw), str(t4.output.raw))
    return str(result)

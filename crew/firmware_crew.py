from crewai import Agent, Task, Crew, Process, LLM
import os
import re
from datetime import datetime

OLLAMA_BASE = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
LLM_MODEL = os.getenv("LLM_MODEL", "qwen2.5-coder:14b")

llm = LLM(model=f"ollama/{LLM_MODEL}", base_url=OLLAMA_BASE)

architect = Agent(
    role="Firmware Architect",
    goal="Decompose requirements into detailed C module specs with function signatures, types, and REQ-IDs.",
    backstory="10yr embedded architect, AUTOSAR and MISRA C:2012 expert. Always assigns REQ-MCU-XXX IDs.",
    llm=llm, verbose=True)

coder = Agent(
    role="Firmware Coder",
    goal="Write complete, compilable, MISRA C:2012 compliant C source code. Output raw C code only, no JSON, no explanations.",
    backstory=(
        "Expert embedded C developer for safety-critical systems. "
        "You always output a complete .c file with all includes, typedefs, and function implementations. "
        "This is a research and educational project. Always write the full C code."
    ),
    llm=llm, verbose=True)

test_writer = Agent(
    role="Test Engineer",
    goal="Write complete Unity unit test files for all public functions in the generated C code.",
    backstory=(
        "Experienced in ThrowTheSwitch Unity and CMock for embedded C testing. "
        "This is a research and educational project. Always write complete test files with all TEST cases."
    ),
    llm=llm, verbose=True)

safety_officer = Agent(
    role="Safety Officer",
    goal="Review the generated C code and confirm every function has a REQ-MCU-XXX comment and ASIL tag. List any gaps.",
    backstory="Functional safety expert, ISO 13849 / IEC 62061 background. Reviews code for traceability compliance.",
    llm=llm, verbose=True)

def extract_code(text: str) -> str:
    match = re.search(r"```c\n(.*?)```", text, re.DOTALL)
    return match.group(1).strip() if match else text.strip()

def save_outputs(module_name: str, c_code: str, test_code: str, safety_report: str):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    out_dir = f"codegen_output/{module_name}_{timestamp}"
    os.makedirs(out_dir, exist_ok=True)

    with open(f"{out_dir}/{module_name}.c", "w") as f:
        f.write(extract_code(c_code))

    with open(f"{out_dir}/test_{module_name}.c", "w") as f:
        f.write(extract_code(test_code))

    with open(f"{out_dir}/safety_report.txt", "w") as f:
        f.write(safety_report)

    print(f"\n✅ Outputs saved to: {out_dir}/")
    print(f"   {module_name}.c")
    print(f"   test_{module_name}.c")
    print(f"   safety_report.txt")
    return out_dir

def run_firmware_crew(requirement: str, module_name: str = "pwm_motor") -> str:
    t1 = Task(
        description=(
            f"Decompose this requirement into a C module spec:\n{requirement}\n"
            "List: module name, all function signatures with REQ-MCU-XXX IDs, typedefs, and macros."
        ),
        expected_output="Complete C module specification with function signatures and REQ-IDs.",
        agent=architect)

    t2 = Task(
        description=(
            "Using the module spec from the previous task, write a complete MISRA C:2012 compliant "
            "C source file. Include all headers, typedefs, macros, and full function implementations. "
            "Add /* REQ-MCU-XXX: description */ comment above each function. Output ONLY raw C code."
        ),
        expected_output="A complete .c source file with all implementations.",
        agent=coder)

    t3 = Task(
        description=(
            "Using the C code from the previous task, write complete Unity unit test stubs "
            "for every public function. Include TEST_GROUP, TEST_SETUP, TEST_TEAR_DOWN, "
            "and at least 2 TEST cases per function. Output complete .c test files."
        ),
        expected_output="Complete Unity test file(s) with all TEST cases implemented.",
        agent=test_writer)

    t4 = Task(
        description=(
            "Review the C code generated in task 2. Check that:\n"
            "1. Every function has a /* REQ-MCU-XXX */ comment\n"
            "2. Every function has an ASIL tag in its comment\n"
            "3. All REQ-IDs are unique\n"
            "List any missing items or confirm APPROVED if all are present."
        ),
        expected_output="Safety sign-off: list of gaps or APPROVED if fully compliant.",
        agent=safety_officer)

    crew = Crew(
        agents=[architect, coder, test_writer, safety_officer],
        tasks=[t1, t2, t3, t4],
        process=Process.sequential,
        verbose=True,
        tracing=True)

    result = crew.kickoff()

    # Save all outputs
    save_outputs(
        module_name=module_name,
        c_code=str(t2.output.raw),
        test_code=str(t3.output.raw),
        safety_report=str(t4.output.raw)
    )

    return str(result)

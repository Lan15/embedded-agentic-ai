from autogen_agentchat.agents import AssistantAgent
from autogen_agentchat.teams import RoundRobinGroupChat
from autogen_agentchat.conditions import TextMentionTermination, MaxMessageTermination
from autogen_ext.models.ollama import OllamaChatCompletionClient
from rag.retrieve import retrieve_context
import os

OLLAMA_BASE = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
LLM_MODEL = os.getenv("LLM_MODEL", "qwen2.5-coder:14b")

model_client = OllamaChatCompletionClient(
    model=LLM_MODEL,
    host=OLLAMA_BASE,
)

coder = AssistantAgent(
    name="EmbeddedCoder",
    model_client=model_client,
    system_message=(
        "You are an expert embedded C developer. Write ISO C11, MISRA "
        "C:2012 compliant firmware. Use libgpiod for GPIO. Add REQ-ID "
        "comments: /* REQ-MCU-042: description */. Output only C code."
    ),
)

reviewer = AssistantAgent(
    name="MISRAReviewer",
    model_client=model_client,
    system_message=(
        "Review embedded C for MISRA C:2012 compliance. List each "
        "violation with its rule number. If none remain, reply: APPROVED"
    ),
)

async def run_codegen_loop(requirement: str) -> str:
    context = retrieve_context(requirement)
    prompt = (
        f"REQUIREMENT: {requirement}\n\n"
        f"CONTEXT:\n{context}\n\nWrite the C implementation."
    )
    termination = TextMentionTermination("APPROVED") | MaxMessageTermination(10)
    team = RoundRobinGroupChat([coder, reviewer], termination_condition=termination)
    result = await team.run(task=prompt)
    for msg in reversed(result.messages):
        if msg.source == "EmbeddedCoder":
            return msg.content
    return result.messages[-1].content

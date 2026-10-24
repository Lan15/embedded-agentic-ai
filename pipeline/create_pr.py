import os, json
from github import Github
from safety.trace_matrix import build_traceability

def create_firmware_pr(branch: str, requirement: str) -> str:
    g = Github(os.environ["GITHUB_TOKEN"])
    repo = g.get_repo(os.environ["GITHUB_REPO"])
    matrix = build_traceability()
    rows = "\n".join(
        f"| {rid} | {v['description']} | "
        f"{', '.join(v['files'])} | "
        f"{', '.join(v['tests']) or 'TBD'} |"
        for rid, v in matrix.items())
    body = (
        f"## Automated Firmware PR\n\n"
        f"**Requirement**: {requirement}\n\n"
        f"| REQ-ID | Description | Files | Tests |\n"
        f"|--------|-------------|-------|-------|\n"
        f"{rows}\n\n*Auto-generated*"
    )
    pr = repo.create_pull(
        title=f"feat: {requirement[:60]}",
        body=body, head=branch, base="main")
    return pr.html_url

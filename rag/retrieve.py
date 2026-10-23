from langchain_chroma import Chroma
from langchain_ollama import OllamaEmbeddings
import os

OLLAMA_BASE = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
EMBED_MODEL = os.getenv("EMBED_MODEL", "nomic-embed-text")

_db = None

def _get_db():
    global _db
    if _db is None:
        emb = OllamaEmbeddings(
            model=EMBED_MODEL, base_url=OLLAMA_BASE)
        _db = Chroma(persist_directory="rag_db",
                     embedding_function=emb)
    return _db

def retrieve_context(query: str, k: int = 5) -> str:
    docs = _get_db().similarity_search(query, k=k)
    return "\n---\n".join(d.page_content for d in docs)

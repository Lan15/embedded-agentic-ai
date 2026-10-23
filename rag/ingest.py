from langchain_community.document_loaders import DirectoryLoader, PyPDFLoader
from langchain_text_splitters import RecursiveCharacterTextSplitter
from langchain_chroma import Chroma
from langchain_ollama import OllamaEmbeddings
import os

OLLAMA_BASE = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
EMBED_MODEL = os.getenv("EMBED_MODEL", "nomic-embed-text")

def build_knowledge_base(docs_dir="knowledge_base", persist_dir="rag_db"):
    raw_docs = []

    pdf_loader = DirectoryLoader(docs_dir, glob="**/*.pdf",
                                  loader_cls=PyPDFLoader, show_progress=True)
    raw_docs.extend(pdf_loader.load())

    text_loader = DirectoryLoader(docs_dir,
                                   glob="**/*.{md,h,hpp,txt}", show_progress=True)
    raw_docs.extend(text_loader.load())

    print(f"Loaded {len(raw_docs)} documents")

    splitter = RecursiveCharacterTextSplitter(
        chunk_size=512, chunk_overlap=64,
        separators=["\n\n", "\n", " ", ""])
    chunks = splitter.split_documents(raw_docs)

    print(f"Split into {len(chunks)} chunks")

    embeddings = OllamaEmbeddings(
        model=EMBED_MODEL, base_url=OLLAMA_BASE)
    vectordb = Chroma.from_documents(
        documents=chunks, embedding=embeddings,
        persist_directory=persist_dir)
    vectordb.persist()
    print(f"Ingested {len(chunks)} chunks")
    return vectordb

if __name__ == "__main__":
    build_knowledge_base()

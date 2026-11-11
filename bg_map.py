"""Project map: finds the repo's own files and links each to the part of BankGuard it implements."""
import re
from pathlib import Path

SKIP = {".git", "__pycache__", "node_modules", ".venv", "venv", "build", ".streamlit"}
EXT = {".py", ".cpp", ".cc", ".hpp", ".h", ".c", ".csv", ".md", ".txt", ".toml"}
LAYERS = [("Graph analysis", ("graph", "network", "cluster")), ("Isolation Forest (ML)", ("isolation", "forest", "iforest", "outlier")),
          ("Rules engine", ("rule", "threshold", "structur")), ("Hybrid risk scoring", ("hybrid", "scor", "risk")),
          ("Data", (".csv",)), ("Web app", ("app.py", "bg_"))]


def repo_url(root="."):
    try:
        text = (Path(root) / ".git" / "config").read_text(errors="ignore")
        m = re.search(r"url\s*=\s*(\S+)", text)
        if not m:
            return ""
        url = m.group(1).removesuffix(".git")
        return re.sub(r"^git@github\.com:", "https://github.com/", url)
    except OSError:
        return ""


def branch(root="."):
    try:
        head = (Path(root) / ".git" / "HEAD").read_text().strip()
        return head.rsplit("/", 1)[-1] if head.startswith("ref:") else "main"
    except OSError:
        return "main"


def note(path):
    try:
        for line in path.read_text(errors="ignore")[:3000].splitlines():
            s = line.strip().strip('"').strip("'")
            if s.startswith(("#", "//", "/*")) and len(s) > 6 and not s.startswith(("#include", "#!")):
                return s.lstrip("#/* ").rstrip("*/ ")[:90]
            if line.strip().startswith('"""'):
                return line.strip().strip('"')[:90]
    except OSError:
        pass
    return ""


def layer_of(p):
    low = str(p).lower()
    for name, keys in LAYERS:
        if any(k in low for k in keys):
            return name
    return "C++ backend" if p.suffix in {".cpp", ".cc", ".hpp", ".h", ".c"} else "Other"


def scan(root="."):
    out = []
    for p in sorted(Path(root).rglob("*")):
        if p.is_file() and p.suffix in EXT and not any(part in SKIP for part in p.parts):
            out.append(dict(path=p.as_posix().lstrip("./"), layer=layer_of(p), lang=p.suffix.lstrip("."), note=note(p),
                            lines=sum(1 for _ in open(p, errors="ignore")) if p.suffix != ".csv" else 0))
    return out


def table_html(files, base):
    import html
    rows = ""
    for f in sorted(files, key=lambda x: (x["layer"], x["path"])):
        name = html.escape(f["path"])
        link = f'<a href="{html.escape(base)}/{name}" target="_blank">{name}</a>' if base else name
        rows += f'<tr><td>{html.escape(f["layer"])}</td><td>{link}</td><td>{f["lines"] or ""}</td><td>{html.escape(f["note"])}</td></tr>'
    return f'<table class="map"><tr><th>Part of BankGuard</th><th>File</th><th>Lines</th><th>What it says it does</th></tr>{rows}</table>'

"""Bridge between the Streamlit website (app.py) and the C++ detector (the backend).

The website does not repeat any detection logic. It only
  1. makes sure the C++ program is built,
  2. runs it,
  3. reads the scored CSV that the program writes.
"""
import glob
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import pandas as pd

CPP_DIR = Path(__file__).resolve().parent / "cpp"
BINARY = CPP_DIR / ("detector.exe" if os.name == "nt" else "detector")
SCORED_FILE = CPP_DIR / "data" / "scored_transactions.csv"

# The C++ loader needs these three columns; receiver_id and the others are optional.
REQUIRED_RAW_COLUMNS = {"account_id", "timestamp", "amount"}


def build():
    """Compile the detector if it is not built yet. Returns (ok, message)."""
    if BINARY.exists():
        return True, "detector is already built"

    if shutil.which("make"):
        command = ["make"]
    elif shutil.which("g++"):
        sources = sorted(glob.glob(str(CPP_DIR / "src" / "*.cpp")))
        sources = [os.path.relpath(s, CPP_DIR) for s in sources]
        command = ["g++", "-std=c++17", "-O2", "-Iinclude", "-pthread", *sources, "-o", BINARY.name]
    else:
        return False, "Neither 'make' nor 'g++' was found. Install MinGW-w64 (Windows) or build-essential (Linux)."

    result = subprocess.run(command, cwd=CPP_DIR, capture_output=True, text=True)
    if result.returncode != 0 or not BINARY.exists():
        return False, "Build failed:\n" + (result.stdout + result.stderr)[-1500:]
    return True, "detector built"


def _run(args, timeout=120):
    """Run the detector with the given arguments. Returns (ok, text_output)."""
    result = subprocess.run([str(BINARY), *args], cwd=CPP_DIR, capture_output=True, text=True, timeout=timeout)
    return result.returncode == 0, (result.stdout + result.stderr).strip()


def run_demo():
    """Generate demo data, score it and return (ok, message, DataFrame or None)."""
    ok, message = build()
    if not ok:
        return False, message, None
    ok, text = _run([])                      # no arguments = generate data + detect
    if not ok or not SCORED_FILE.exists():
        return False, text or "the detector produced no output", None
    return True, text, pd.read_csv(SCORED_FILE)


def needs_scoring(df):
    """True when the table has no risk column yet, i.e. it is a raw transaction file."""
    columns = {str(c).strip().lower() for c in df.columns}
    return not ({"risk", "risk_score"} & columns)


def score_dataframe(raw):
    """Score raw transactions with the C++ detector. Returns (ok, message, DataFrame or None)."""
    raw = raw.rename(columns=lambda c: str(c).strip().lower())
    missing = REQUIRED_RAW_COLUMNS - set(raw.columns)
    if missing:
        return False, "Missing columns: " + ", ".join(sorted(missing)), None
    if raw.empty:
        return False, "The file has no transactions.", None

    ok, message = build()
    if not ok:
        return False, message, None

    with tempfile.TemporaryDirectory() as folder:
        source = Path(folder) / "raw.csv"
        target = Path(folder) / "scored.csv"
        raw.to_csv(source, index=False)
        ok, text = _run(["detect", str(source), str(target)])
        if not ok or not target.exists():
            return False, text or "the detector produced no output", None
        return True, text, pd.read_csv(target)

"""BankGuard data loading and cleaning."""
import io
import re

import pandas as pd
import streamlit as st

ALIASES = {
    "id": ["transaction_id", "tx_id", "txn_id", "tx", "id"],
    "sender": ["account_id", "sender_id", "sender", "from_account", "source"],
    "receiver": ["receiver_id", "receiver", "to_account", "destination", "beneficiary"],
    "amount": ["amount", "amount_inr", "value"],
    "time": ["timestamp", "time", "datetime", "date"],
    "score": ["risk_score", "hybrid_risk_score", "hybrid_score", "final_score", "risk", "score"],
    "level": ["risk_level", "level", "severity", "risk_band"],
    "reasons": ["reasons", "detection_reasons", "reason", "flags", "detections"],
}


def run_demo_detector():
    """PLUG IN: call your C++ detector on the demo data and return a scored DataFrame."""
    raise NotImplementedError


def score_with_detector(raw):
    """PLUG IN: score a raw transactions DataFrame with your C++ detector."""
    raise NotImplementedError


def clean_reason(r):
    r = r.strip().replace("_", " ")
    if "linked cluster" in r.lower():
        return "Graph: Linked cluster"
    parts = [p.strip() for p in r.split(":")]
    r = ": ".join(p[:1].upper() + p[1:].lower() for p in parts)
    return " ".join("ML" if w.lower() == "ml" else w for w in r.split())


def prepare(raw):
    lower = {c.lower().strip(): c for c in raw.columns}
    cols = {k: next((lower[n] for n in names if n in lower), None) for k, names in ALIASES.items()}
    if cols["score"] is None or cols["amount"] is None:
        return None
    d = pd.DataFrame(index=raw.index)
    d["id"] = raw[cols["id"]].astype(str) if cols["id"] else [f"TX{i + 1:06d}" for i in range(len(raw))]
    d["sender"] = raw[cols["sender"]].astype(str) if cols["sender"] else ""
    d["receiver"] = raw[cols["receiver"]].astype(str) if cols["receiver"] else ""
    d["amount"] = pd.to_numeric(raw[cols["amount"]], errors="coerce")
    d["time"] = pd.to_datetime(raw[cols["time"]], errors="coerce") if cols["time"] else pd.NaT
    score = pd.to_numeric(raw[cols["score"]], errors="coerce").fillna(0)
    d["score"] = score / 100 if score.max() > 1 else score
    if cols["level"]:
        d["level"] = raw[cols["level"]].astype(str).str.upper().str.strip()
    else:
        d["level"] = pd.cut(d["score"], [-1, .2, .5, .8, 2], labels=["NONE", "LOW", "MEDIUM", "HIGH"]).astype(str)
    reasons = raw[cols["reasons"]].fillna("").astype(str) if cols["reasons"] else pd.Series("", index=raw.index)
    split = reasons.map(lambda s: [x for x in re.split(r"[;|,]", s) if x.strip() and x.strip().lower() != "nan"])
    d["reason_list"] = split.map(lambda xs: sorted({clean_reason(x) for x in xs}))
    d["reasons"] = d["reason_list"].map(", ".join)
    return d


@st.cache_data(show_spinner=False)
def read_csv(data):
    return pd.read_csv(io.BytesIO(data))

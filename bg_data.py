"""BankGuard data loading, validation, and C++ detector integration."""
import io
import re
import pandas as pd
import streamlit as st
import engine

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

def read_csv(data):
    return pd.read_csv(io.BytesIO(data))

def run_demo_detector():
    """Run the repository's existing C++ demo detector."""
    return engine.run_demo()

def score_with_detector(raw):
    """Score raw transactions through the repository's existing C++ engine."""
    return engine.score_dataframe(raw)

def clean_reason(value):
    value = str(value).strip().replace("_", " ")
    if "linked cluster" in value.lower():
        return "Graph: Linked cluster"
    parts = [p.strip() for p in value.split(":")]
    value = ": ".join(p[:1].upper() + p[1:].lower() for p in parts)
    return " ".join("ML" if word.lower() == "ml" else word for word in value.split())

def _parse_time(series):
    # The C++ output may contain epoch seconds or readable date strings.
    numeric = pd.to_numeric(series, errors="coerce")
    if numeric.notna().all() and len(numeric):
        median = numeric.abs().median()
        unit = "ms" if median > 10**11 else "s"
        return pd.to_datetime(numeric, unit=unit, errors="coerce")
    return pd.to_datetime(series, errors="coerce")

def prepare(raw):
    if raw is None or not isinstance(raw, pd.DataFrame) or raw.empty:
        return None
    lower = {str(c).lower().strip(): c for c in raw.columns}
    cols = {key: next((lower[name] for name in names if name in lower), None)
            for key, names in ALIASES.items()}
    if cols["amount"] is None:
        raise ValueError("Missing amount column. Accepted names: amount, amount_inr, value.")
    if cols["score"] is None:
        return None

    data = pd.DataFrame(index=raw.index)
    data["id"] = raw[cols["id"]].astype(str) if cols["id"] else [f"TX{i+1:06d}" for i in range(len(raw))]
    data["sender"] = raw[cols["sender"]].fillna("").astype(str).str.strip() if cols["sender"] else ""
    data["receiver"] = raw[cols["receiver"]].fillna("").astype(str).str.strip() if cols["receiver"] else ""
    data["amount"] = pd.to_numeric(
        raw[cols["amount"]].astype(str).str.replace(r"[^0-9.\-]", "", regex=True),
        errors="coerce"
    )
    data["time"] = _parse_time(raw[cols["time"]]) if cols["time"] else pd.NaT
    score = pd.to_numeric(raw[cols["score"]], errors="coerce").fillna(0)
    # Scores from C++ may be either 0-1 or 0-100.
    data["score"] = (score / 100 if score.max() > 1 else score).clip(0, 1)
    if cols["level"]:
        data["level"] = raw[cols["level"]].fillna("NONE").astype(str).str.upper().str.strip()
    else:
        data["level"] = pd.cut(
            data["score"], [-0.001, 0.2, 0.5, 0.8, 1.001],
            labels=["NONE", "LOW", "MEDIUM", "HIGH"]
        ).astype(str).replace("nan", "NONE")
    reasons = raw[cols["reasons"]].fillna("").astype(str) if cols["reasons"] else pd.Series("", index=raw.index)
    split = reasons.map(lambda s: [clean_reason(x) for x in re.split(r"[;|,]", s)
                                   if x.strip() and x.strip().lower() != "nan"])
    data["reason_list"] = split.map(lambda items: list(dict.fromkeys(items)))
    data["reasons"] = data["reason_list"].map(", ".join)
    data = data.dropna(subset=["amount"]).copy()
    return data.reset_index(drop=True) if not data.empty else None

@st.cache_data(show_spinner=False)
def load_scored_file(path, modified_time):
    # modified_time is included in the cache key so reruns see fresh detector output.
    return pd.read_csv(path)

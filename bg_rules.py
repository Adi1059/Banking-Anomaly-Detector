"""BankGuard quick check: a simple rule and graph scorer in pure Python."""
import numpy as np
import pandas as pd

from bg_data import ALIASES

WEIGHTS = {"Amount spike": .5, "Large transfer to new payee": .35, "Structuring": .4,
           "Repeated transaction": .3, "Graph: Fan out": .35, "Graph: Fan in": .35}


def standardize(raw):
    lower = {c.lower().strip(): c for c in raw.columns}
    col = {k: next((lower[n] for n in ALIASES[k] if n in lower), None) for k in ("id", "sender", "receiver", "amount", "time")}
    if col["sender"] is None or col["amount"] is None:
        raise ValueError("The file needs at least an account_id column and an amount column.")
    d = pd.DataFrame(index=raw.index)
    d["id"] = raw[col["id"]].astype(str) if col["id"] else [f"TX{i + 1:06d}" for i in range(len(raw))]
    d["sender"] = raw[col["sender"]].astype(str).str.strip()
    d["receiver"] = raw[col["receiver"]].astype(str).str.strip() if col["receiver"] else ""
    d["amount"] = pd.to_numeric(raw[col["amount"]].astype(str).str.replace(r"[^0-9.\-]", "", regex=True), errors="coerce")
    d["time"] = pd.to_datetime(raw[col["time"]], errors="coerce") if col["time"] else pd.NaT
    return d.dropna(subset=["amount"]).reset_index(drop=True)


def score_transactions(d):
    n = len(d)
    hits = {k: np.zeros(n, bool) for k in WEIGHTS}
    amt = d["amount"].to_numpy(float)
    med = np.median(amt)
    mad = np.median(np.abs(amt - med)) or max(float(amt.std()), 1.0)
    smed = d.groupby("sender")["amount"].transform("median").to_numpy(float)
    scnt = d.groupby("sender")["amount"].transform("count").to_numpy()
    hits["Amount spike"] = (0.6745 * (amt - med) / mad > 3.5) | ((scnt >= 4) & (amt > 5 * smed))
    has_rx = ((d["receiver"] != "") & (d["receiver"].str.lower() != "nan")).to_numpy()
    order = d.sort_values("time", kind="stable").index
    seen = d.loc[order].duplicated(["sender", "receiver"]).reindex(d.index)
    hits["Large transfer to new payee"] = (~seen).to_numpy() & has_rx & (amt >= np.quantile(amt, 0.95))
    for t in (10_000, 50_000, 100_000, 1_000_000):
        near = (amt >= 0.9 * t) & (amt < t)
        cnt = pd.Series(near).groupby(d["sender"].to_numpy()).transform("sum").to_numpy()
        hits["Structuring"] |= near & (cnt >= 3)
    day = d["time"].dt.strftime("%Y-%m-%d").fillna("all")
    x = d.assign(_d=day)
    hits["Repeated transaction"] = x.duplicated(["sender", "receiver", "amount", "_d"], keep=False).to_numpy() & has_rx
    hits["Graph: Fan out"] = (x.groupby(["sender", "_d"])["receiver"].transform("nunique") >= 5).to_numpy() & has_rx
    hits["Graph: Fan in"] = (x.groupby(["receiver", "_d"])["sender"].transform("nunique") >= 5).to_numpy() & has_rx
    score = np.minimum(1.0, sum(w * hits[k] for k, w in WEIGHTS.items()))
    out = d.copy()
    out["score"] = score
    out["level"] = np.select([score >= .9, score >= .6, score >= .35, score > 0], ["CRITICAL", "HIGH", "MEDIUM", "LOW"], "NONE")
    out["reason_list"] = [[k for k in WEIGHTS if hits[k][i]] for i in range(n)]
    out["reasons"] = out["reason_list"].map(", ".join)
    return out


def sample_df():
    rng = np.random.default_rng(7)
    rows = []
    t0 = pd.Timestamp("2026-11-02 09:00")
    for i in range(60):
        rows.append((f"ACC{rng.integers(1, 15):03d}", f"ACC{rng.integers(100, 140):03d}", round(float(rng.gamma(2, 2500)), 2),
                     t0 + pd.Timedelta(minutes=int(rng.integers(0, 6000)))))
    for k in range(6):
        rows.append((f"ACC{20 + k:03d}", "ACC900", 18_000.0 + k * 500, t0 + pd.Timedelta(hours=30, minutes=k * 9)))
    for k in range(3):
        rows.append(("ACC031", f"ACC10{k}", 49_500.0, t0 + pd.Timedelta(hours=5, minutes=k * 20)))
    rows += [("ACC032", "ACC777", 950_000.0, t0 + pd.Timedelta(hours=40)),
             ("ACC033", "ACC555", 12_000.0, t0 + pd.Timedelta(hours=8)), ("ACC033", "ACC555", 12_000.0, t0 + pd.Timedelta(hours=8, minutes=4))]
    df = pd.DataFrame(rows, columns=["account_id", "receiver_id", "amount", "timestamp"])
    df.insert(0, "transaction_id", [f"TX{i + 1:04d}" for i in range(len(df))])
    df["timestamp"] = df["timestamp"].dt.strftime("%Y-%m-%d %H:%M")
    return df

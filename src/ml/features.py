"""Feature engineering for the ML detector (all features are per-transaction, no labels used)."""
from __future__ import annotations

import numpy as np
import pandas as pd

FEATURE_COLUMNS = [
    "log_amount", "amount_ratio", "hour_sin", "hour_cos",
    "velocity_1h", "log_gap_prev", "new_receiver", "distinct_receivers_1h",
]


def _window_counts(times: np.ndarray, window_ns: int) -> np.ndarray:
    """Number of events in (t - window, t] for sorted event times."""
    left = np.searchsorted(times, times - window_ns, side="right")
    return np.arange(1, len(times) + 1) - left


def build_features(df: pd.DataFrame) -> pd.DataFrame:
    """Return a feature frame aligned to df.index."""
    d = df[["account_id", "receiver_id", "timestamp", "amount"]].copy()
    d["timestamp"] = pd.to_datetime(d["timestamp"])
    d = d.sort_values(["account_id", "timestamp"])

    med = d.groupby("account_id")["amount"].transform("median").clip(lower=1e-6)
    feats = pd.DataFrame(index=d.index)
    feats["log_amount"] = np.log1p(d["amount"])
    feats["amount_ratio"] = (d["amount"] / med).clip(upper=100)

    hour = d["timestamp"].dt.hour + d["timestamp"].dt.minute / 60.0
    feats["hour_sin"] = np.sin(2 * np.pi * hour / 24)
    feats["hour_cos"] = np.cos(2 * np.pi * hour / 24)

    gap = d.groupby("account_id")["timestamp"].diff().dt.total_seconds().fillna(86400 * 7)
    feats["log_gap_prev"] = np.log1p(gap.clip(lower=0))

    feats["new_receiver"] = (~d.duplicated(["account_id", "receiver_id"])).astype(int)

    hour_ns = np.int64(3600 * 1e9)
    vel = pd.Series(0, index=d.index, dtype=float)
    dist = pd.Series(0, index=d.index, dtype=float)
    for _, g in d.groupby("account_id"):
        t = g["timestamp"].to_numpy().astype("datetime64[ns]").astype("int64")
        vel.loc[g.index] = _window_counts(t, hour_ns)
        left = np.searchsorted(t, t - hour_ns, side="right")
        recv = g["receiver_id"].to_numpy()
        dist.loc[g.index] = [len(set(recv[l:i + 1])) for i, l in enumerate(left)]
    feats["velocity_1h"] = vel
    feats["distinct_receivers_1h"] = dist

    return feats.loc[df.index, FEATURE_COLUMNS]

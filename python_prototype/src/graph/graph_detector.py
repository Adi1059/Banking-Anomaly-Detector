"""Graph-based detection on the account-to-account transfer network (pure Python, no extra deps).

Patterns
--------
* cycle   : money returns to its origin within a short time (A->B->C->A), similar amounts
* fan_out : one account sends to many distinct accounts in a short window
* fan_in  : many distinct accounts send to one account in a short window
Merchant receivers (ids starting with `merchant_prefix`) are excluded: they legitimately
receive from many senders.
"""
from __future__ import annotations

from collections import defaultdict, Counter

import pandas as pd


class GraphDetector:
    def __init__(self, window_hours: float = 2.0, max_cycle_len: int = 4, amount_tolerance: float = 0.25,
                 fan_threshold: int = 6, merchant_prefix: str = "MRC"):
        self.window = pd.Timedelta(hours=window_hours)
        self.max_cycle_len = max_cycle_len
        self.amount_tolerance = amount_tolerance
        self.fan_threshold = fan_threshold
        self.merchant_prefix = merchant_prefix

    # ------------------------------------------------------------------
    def detect(self, df: pd.DataFrame) -> pd.DataFrame:
        """Return DataFrame (index=df.index) with bool columns graph_cycle / graph_fan_out / graph_fan_in."""
        d = df[["account_id", "receiver_id", "timestamp", "amount"]].copy()
        d["timestamp"] = pd.to_datetime(d["timestamp"])
        d = d[~d["receiver_id"].astype(str).str.startswith(self.merchant_prefix)]
        d = d.sort_values("timestamp")

        out = pd.DataFrame(False, index=df.index, columns=["graph_cycle", "graph_fan_out", "graph_fan_in"])
        for idx in self._cycles(d):
            out.loc[idx, "graph_cycle"] = True
        for idx in self._fan(d, "account_id", "receiver_id"):
            out.loc[idx, "graph_fan_out"] = True
        for idx in self._fan(d, "receiver_id", "account_id"):
            out.loc[idx, "graph_fan_in"] = True
        return out

    # ------------------------------------------------------------------
    def _cycles(self, d: pd.DataFrame) -> set:
        """DFS for time-ordered paths that return to the start account within the window."""
        adj: dict[str, list] = defaultdict(list)  # src -> [(dst, ts, amount, idx)]
        for idx, s, r, t, a in zip(d.index, d["account_id"], d["receiver_id"], d["timestamp"], d["amount"]):
            if s != r:
                adj[s].append((r, t, a, idx))

        flagged: set = set()
        tol = self.amount_tolerance
        for idx0, s, r, t0, a0 in zip(d.index, d["account_id"], d["receiver_id"], d["timestamp"], d["amount"]):
            if s == r:
                continue
            # path = list of tx indices; walk forward in time
            stack = [(r, t0, [idx0], 1)]
            while stack:
                node, last_t, path, depth = stack.pop()
                if depth >= self.max_cycle_len:
                    continue
                for dst, t, a, idx in adj.get(node, ()):
                    if t < last_t or t - t0 > self.window or idx in path:
                        continue
                    if abs(a - a0) > tol * a0:  # similar amounts only
                        continue
                    if dst == s:
                        flagged.update(path + [idx])
                    else:
                        stack.append((dst, t, path + [idx], depth + 1))
        return flagged

    def _fan(self, d: pd.DataFrame, key: str, other: str) -> set:
        """Sliding-window count of distinct counterparties per key."""
        flagged: set = set()
        for _, g in d.groupby(key):
            if g[other].nunique() < self.fan_threshold:
                continue
            g = g.sort_values("timestamp")
            times = g["timestamp"].tolist()
            idxs = g.index.tolist()
            others = g[other].tolist()
            counts: Counter = Counter()
            left = 0
            for right in range(len(times)):
                counts[others[right]] += 1
                while times[right] - times[left] > self.window:
                    counts[others[left]] -= 1
                    if counts[others[left]] == 0:
                        del counts[others[left]]
                    left += 1
                if len(counts) >= self.fan_threshold:
                    flagged.update(idxs[left:right + 1])
        return flagged

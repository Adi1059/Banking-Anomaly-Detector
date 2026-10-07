"""Flag bursts of identical-amount transactions on one account in a short window."""
from __future__ import annotations

import pandas as pd

from .base_rule import BaseRule


class RepeatedTransactionRule(BaseRule):
    name = "repeated_transaction"

    def __init__(self, window_minutes: int = 10, min_repeats: int = 3):
        self.window = pd.Timedelta(minutes=window_minutes)
        self.min_repeats = min_repeats

    def apply(self, df: pd.DataFrame) -> pd.Series:
        self._validate(df, ["account_id", "timestamp", "amount"])
        flags = pd.Series(False, index=df.index)
        ts = pd.to_datetime(df["timestamp"])
        for _, grp in df.assign(_ts=ts).groupby(["account_id", "amount"]):
            if len(grp) < self.min_repeats:
                continue
            grp = grp.sort_values("_ts")
            times = grp["_ts"].to_numpy()
            idx = grp.index.to_numpy()
            left = 0
            for right in range(len(times)):  # sliding window
                while times[right] - times[left] > self.window.to_timedelta64():
                    left += 1
                if right - left + 1 >= self.min_repeats:
                    flags.loc[idx[left:right + 1]] = True
        return flags

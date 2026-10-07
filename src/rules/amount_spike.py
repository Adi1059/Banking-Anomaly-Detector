"""Flag amounts far above an account's typical behaviour (robust z-score via MAD)."""
from __future__ import annotations

import pandas as pd

from .base_rule import BaseRule


class AmountSpikeRule(BaseRule):
    name = "amount_spike"

    def __init__(self, threshold: float = 3.5, min_history: int = 5):
        self.threshold = threshold
        self.min_history = min_history

    def apply(self, df: pd.DataFrame) -> pd.Series:
        self._validate(df, ["account_id", "amount"])
        g = df.groupby("account_id")["amount"]
        median = g.transform("median")
        mad = (df["amount"] - median).abs().groupby(df["account_id"]).transform("median")
        # Fallback scale avoids divide-by-zero for constant-spend accounts
        scale = (1.4826 * mad).where(mad > 0, median * 0.1).clip(lower=1e-9)
        score = (df["amount"] - median) / scale
        enough = g.transform("count") >= self.min_history
        return (score > self.threshold) & enough

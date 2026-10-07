"""Flag transactions during unusual hours (default 00:00-05:00)."""
from __future__ import annotations

import pandas as pd

from .base_rule import BaseRule


class OddHourRule(BaseRule):
    name = "odd_hour"

    def __init__(self, start_hour: int = 0, end_hour: int = 5):
        self.start_hour = start_hour
        self.end_hour = end_hour  # exclusive

    def apply(self, df: pd.DataFrame) -> pd.Series:
        self._validate(df, ["timestamp"])
        hour = pd.to_datetime(df["timestamp"]).dt.hour
        if self.start_hour <= self.end_hour:
            return (hour >= self.start_hour) & (hour < self.end_hour)
        return (hour >= self.start_hour) | (hour < self.end_hour)  # wraps midnight

"""Base class every detection rule extends."""
from __future__ import annotations

from abc import ABC, abstractmethod

import pandas as pd


class BaseRule(ABC):
    name: str = "base_rule"

    @abstractmethod
    def apply(self, df: pd.DataFrame) -> pd.Series:
        """Return a boolean Series (same index as df): True = flagged."""

    @staticmethod
    def _validate(df: pd.DataFrame, cols: list[str]) -> None:
        missing = [c for c in cols if c not in df.columns]
        if missing:
            raise ValueError(f"Missing required columns: {missing}")

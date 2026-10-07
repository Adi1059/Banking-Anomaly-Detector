"""Unsupervised ML detector: Isolation Forest on behavioural features."""
from __future__ import annotations

import numpy as np
import pandas as pd
from sklearn.ensemble import IsolationForest
from sklearn.preprocessing import StandardScaler

from .features import build_features


class MLDetector:
    """Returns ml_score in [0, 1]; the bottom `baseline_pct` of transactions score 0."""

    def __init__(self, n_estimators: int = 300, baseline_pct: float = 90.0, top_pct: float = 99.5, seed: int = 42):
        self.model = IsolationForest(n_estimators=n_estimators, contamination="auto", random_state=seed, n_jobs=-1)
        self.scaler = StandardScaler()
        self.baseline_pct = baseline_pct
        self.top_pct = top_pct

    def score(self, df: pd.DataFrame) -> pd.Series:
        X = build_features(df)
        Z = self.scaler.fit_transform(X)
        self.model.fit(Z)
        raw = -self.model.score_samples(Z)  # higher = more anomalous
        lo, hi = np.percentile(raw, [self.baseline_pct, self.top_pct])
        scaled = np.clip((raw - lo) / max(hi - lo, 1e-9), 0, 1)
        return pd.Series(scaled, index=df.index, name="ml_score")

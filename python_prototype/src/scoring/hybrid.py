"""Hybrid risk scoring: fuses rule, graph and ML signals into one explainable 0-1 risk score.

Each signal first becomes a component score in [0, 1]:
    rule_score  = noisy-OR of the weights of the fired rules
    graph_score = noisy-OR of the weights of the fired graph patterns
    ml_score    = scaled Isolation Forest score (0 for normal-looking behaviour)
Components are then fused with a weighted noisy-OR:
    risk = 1 - (1 - a*rule_score) * (1 - b*graph_score) * (1 - c*ml_score)
A single strong signal gives a medium/high risk; agreement between independent
detectors pushes risk towards 1. Every alert also carries a human-readable reason.
"""
from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np
import pandas as pd

DEFAULT_RULE_WEIGHTS = {"amount_spike": 0.50, "repeated_transaction": 0.60, "odd_hour": 0.30}
DEFAULT_GRAPH_WEIGHTS = {"graph_cycle": 0.90, "graph_fan_out": 0.70, "graph_fan_in": 0.70}


@dataclass
class HybridConfig:
    rule_weights: dict = field(default_factory=lambda: dict(DEFAULT_RULE_WEIGHTS))
    graph_weights: dict = field(default_factory=lambda: dict(DEFAULT_GRAPH_WEIGHTS))
    w_rule: float = 0.90
    w_graph: float = 0.90
    w_ml: float = 0.80
    high: float = 0.70
    medium: float = 0.50
    low: float = 0.20
    alert_threshold: float = 0.50  # risk >= this counts as an alert (MEDIUM and above)


def _noisy_or(flags: pd.DataFrame, weights: dict, prefix: str = "") -> pd.Series:
    prod = pd.Series(1.0, index=flags.index)
    for name, w in weights.items():
        col = f"{prefix}{name}"
        if col in flags:
            prod *= np.where(flags[col].astype(bool), 1.0 - w, 1.0)
    return 1.0 - prod


class HybridScorer:
    def __init__(self, config: HybridConfig | None = None):
        self.cfg = config or HybridConfig()

    def score(self, flagged: pd.DataFrame, graph_flags: pd.DataFrame, ml_score: pd.Series) -> pd.DataFrame:
        c = self.cfg
        out = flagged.copy()
        for col in graph_flags.columns:
            out[col] = graph_flags[col].values
        out["ml_score"] = ml_score.values

        out["rule_score"] = _noisy_or(out, c.rule_weights, prefix="flag_")
        out["graph_score"] = _noisy_or(out, c.graph_weights)
        out["risk_score"] = 1.0 - (
            (1.0 - c.w_rule * out["rule_score"])
            * (1.0 - c.w_graph * out["graph_score"])
            * (1.0 - c.w_ml * out["ml_score"])
        )
        out["risk_level"] = pd.cut(
            out["risk_score"], bins=[-1, c.low, c.medium, c.high, 2],
            labels=["NONE", "LOW", "MEDIUM", "HIGH"], right=False,
        ).astype(str)
        out["is_alert"] = out["risk_score"] >= c.alert_threshold
        out["reasons"] = out.apply(self._explain, axis=1)
        return out

    def _explain(self, row: pd.Series) -> str:
        parts = [name.replace("_", " ") for name in self.cfg.rule_weights if row.get(f"flag_{name}", False)]
        parts += [name.replace("graph_", "graph: ").replace("_", " ") for name in self.cfg.graph_weights if row.get(name, False)]
        if row["ml_score"] >= 0.5:
            parts.append(f"ML outlier ({row['ml_score']:.2f})")
        return "; ".join(parts)

"""End-to-end hybrid anomaly detection pipeline.

    transactions -> RuleEngine ─┐
                 -> GraphDetector ─┼─> HybridScorer -> risk score, level, reasons
                 -> MLDetector ──┘
"""
from __future__ import annotations

import argparse
import time

import pandas as pd

from .graph.graph_detector import GraphDetector
from .ml.isolation_forest import MLDetector
from .rules.engine import RuleEngine
from .scoring.hybrid import HybridConfig, HybridScorer


def run_pipeline(df: pd.DataFrame, config: HybridConfig | None = None) -> pd.DataFrame:
    df = df.reset_index(drop=True)
    rules_out = RuleEngine().run(df)
    graph_flags = GraphDetector().detect(df)
    ml_score = MLDetector().score(df)
    return HybridScorer(config).score(rules_out, graph_flags, ml_score)


def _metrics(pred: pd.Series, truth: pd.Series) -> tuple[float, float, float]:
    tp = int((pred & truth).sum())
    p = tp / max(int(pred.sum()), 1)
    r = tp / max(int(truth.sum()), 1)
    f1 = 2 * p * r / max(p + r, 1e-9)
    return p, r, f1


def evaluate(res: pd.DataFrame, config: HybridConfig) -> None:
    if "is_injected_anomaly" not in res:
        return
    truth = res["is_injected_anomaly"] == 1
    graph_cols = [c for c in res.columns if c.startswith("graph_") and c != "graph_score"]
    methods = {
        "Rules only": res[[c for c in res.columns if c.startswith("flag_")]].any(axis=1),
        "Graph only": res[graph_cols].any(axis=1),
        "ML only": res["ml_score"] >= 0.5,
        "Hybrid (risk >= %.2f)" % config.alert_threshold: res["is_alert"],
    }
    print("\nMethod comparison (against injected labels)")
    print(f"{'method':<26}{'alerts':>8}{'precision':>11}{'recall':>9}{'F1':>7}")
    for name, pred in methods.items():
        p, r, f1 = _metrics(pred, truth)
        print(f"{name:<26}{int(pred.sum()):>8}{p:>11.2f}{r:>9.2f}{f1:>7.2f}")

    if "anomaly_type" in res:
        print("\nHybrid recall by anomaly type")
        for kind, g in res[truth].groupby("anomaly_type"):
            print(f"  {kind:<14}{int(g.is_alert.sum()):>4}/{len(g):<4} ({g.is_alert.mean():.0%})")


def main() -> None:
    p = argparse.ArgumentParser(description="Run the hybrid anomaly detection pipeline")
    p.add_argument("--input", default="data/demo_transactions.csv")
    p.add_argument("--output", default="data/scored_transactions.csv")
    p.add_argument("--threshold", type=float, default=0.50, help="risk score needed to raise an alert")
    a = p.parse_args()

    cfg = HybridConfig(alert_threshold=a.threshold)
    df = pd.read_csv(a.input, parse_dates=["timestamp"])
    t0 = time.time()
    res = run_pipeline(df, cfg)
    res.to_csv(a.output, index=False)

    print(f"Transactions: {len(res)} | alerts: {int(res.is_alert.sum())} | time: {time.time() - t0:.1f}s")
    print("Risk levels:", res["risk_level"].value_counts().to_dict())
    evaluate(res, cfg)
    print(f"\nTop 5 risky transactions:")
    cols = ["tx_id", "account_id", "receiver_id", "amount", "risk_score", "risk_level", "reasons"]
    print(res.sort_values("risk_score", ascending=False).head(5)[cols].to_string(index=False))


if __name__ == "__main__":
    main()

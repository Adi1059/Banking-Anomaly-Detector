"""Basic rule engine: runs rules, collects flags and reasons."""
from __future__ import annotations

import argparse
from typing import Iterable

import pandas as pd

from .amount_spike import AmountSpikeRule
from .base_rule import BaseRule
from .odd_hour import OddHourRule
from .repeated_transaction import RepeatedTransactionRule


class RuleEngine:
    def __init__(self, rules: Iterable[BaseRule] | None = None):
        self.rules = list(rules) if rules is not None else [
            AmountSpikeRule(), RepeatedTransactionRule(), OddHourRule(),
        ]

    def run(self, df: pd.DataFrame) -> pd.DataFrame:
        out = df.copy()
        for rule in self.rules:
            out[f"flag_{rule.name}"] = rule.apply(df).astype(bool)
        flag_cols = [f"flag_{r.name}" for r in self.rules]
        out["rules_triggered"] = out[flag_cols].apply(
            lambda row: ",".join(c.removeprefix("flag_") for c in flag_cols if row[c]), axis=1)
        out["is_anomaly"] = out[flag_cols].any(axis=1)
        return out


def main() -> None:
    p = argparse.ArgumentParser(description="Run rule-based anomaly detection")
    p.add_argument("--input", default="data/demo_transactions.csv")
    p.add_argument("--output", default="data/flagged_transactions.csv")
    a = p.parse_args()

    df = pd.read_csv(a.input, parse_dates=["timestamp"])
    res = RuleEngine().run(df)
    res.to_csv(a.output, index=False)

    print(f"Transactions: {len(res)} | flagged: {int(res.is_anomaly.sum())}")
    for r in RuleEngine().rules:
        print(f"  {r.name}: {int(res[f'flag_{r.name}'].sum())}")
    if "is_injected_anomaly" in res:
        tp = int((res.is_anomaly & (res.is_injected_anomaly == 1)).sum())
        inj = int((res.is_injected_anomaly == 1).sum())
        fl = int(res.is_anomaly.sum())
        print(f"Recall on injected: {tp}/{inj} | precision: {tp}/{fl}")


if __name__ == "__main__":
    main()

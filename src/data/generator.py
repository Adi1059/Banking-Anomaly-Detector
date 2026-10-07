"""Synthetic transaction generator with labelled, injected anomalies.

Columns
-------
tx_id, account_id (sender), receiver_id, timestamp, amount, merchant,
is_injected_anomaly (0/1), anomaly_type (evaluation only; detectors never read these).
"""
from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd

MERCHANT_CATEGORIES = ["Grocery", "Fuel", "Restaurant", "Utilities", "Pharmacy", "Online Shop", "Transport"]
COLUMNS = ["account_id", "receiver_id", "timestamp", "amount", "merchant", "is_injected_anomaly", "anomaly_type"]


def _acc(i: int) -> str:
    return f"ACC{i:04d}"


def generate_transactions(
    n_accounts: int = 60,
    days: int = 30,
    avg_tx_per_day: float = 2.0,
    n_anomaly_groups: int = 30,
    seed: int = 42,
) -> pd.DataFrame:
    rng = np.random.default_rng(seed)
    start = pd.Timestamp("2026-01-01")
    n_merchants = 40
    rows: list[tuple] = []

    # ---- normal behaviour -------------------------------------------------
    for a in range(1, n_accounts + 1):
        acc = _acc(a)
        base = rng.uniform(20, 200)
        fav_merchants = [f"MRC{int(m):04d}" for m in rng.choice(n_merchants, size=4, replace=False)]
        contacts = [_acc(int(c)) for c in rng.choice([x for x in range(1, n_accounts + 1) if x != a], size=2, replace=False)]
        for _ in range(rng.poisson(avg_tx_per_day * days)):
            ts = start + pd.Timedelta(
                days=int(rng.integers(0, days)),
                hours=int(np.clip(rng.normal(14, 4), 7, 22)),
                minutes=int(rng.integers(0, 60)),
                seconds=int(rng.integers(0, 60)),
            )
            receiver = str(rng.choice(contacts)) if rng.random() < 0.1 else str(rng.choice(fav_merchants))
            amount = float(np.round(rng.lognormal(np.log(base), 0.4), 2))
            rows.append((acc, receiver, ts, amount, str(rng.choice(MERCHANT_CATEGORIES)), 0, "normal"))

    normal = pd.DataFrame(rows, columns=COLUMNS)
    median_by_acc = normal.groupby("account_id")["amount"].median().to_dict()

    # ---- injected anomalies ----------------------------------------------
    extra: list[tuple] = []
    kinds = ["amount_spike", "repeated", "odd_hour", "ring", "fan_out", "fan_in"]
    for g in range(n_anomaly_groups):
        kind = kinds[g % len(kinds)]
        day = int(rng.integers(1, days - 1))
        ts = start + pd.Timedelta(days=day, hours=int(rng.integers(9, 20)), minutes=int(rng.integers(0, 60)))
        acc = _acc(int(rng.integers(1, n_accounts + 1)))
        merch = f"MRC{int(rng.integers(0, n_merchants)):04d}"

        if kind == "amount_spike":
            amt = round(median_by_acc.get(acc, 100.0) * float(rng.uniform(8, 20)), 2)
            extra.append((acc, merch, ts, amt, "Online Shop", 1, kind))
        elif kind == "repeated":
            amt = round(float(rng.uniform(50, 500)), 2)
            for k in range(int(rng.integers(3, 6))):
                extra.append((acc, merch, ts + pd.Timedelta(minutes=2 * k), amt, "Online Shop", 1, kind))
        elif kind == "odd_hour":
            t = ts.normalize() + pd.Timedelta(hours=int(rng.integers(1, 5)), minutes=int(rng.integers(0, 60)))
            extra.append((acc, merch, t, round(float(rng.uniform(100, 800)), 2), "Online Shop", 1, kind))
        elif kind == "ring":  # A -> B -> C -> A, near-identical amounts, within hours
            ids = [int(x) for x in rng.choice(np.arange(1, n_accounts + 1), size=3, replace=False)]
            a, b, c = (_acc(i) for i in ids)
            amt = round(float(rng.uniform(2000, 9000)), 2)
            for j, (s, r) in enumerate([(a, b), (b, c), (c, a)]):
                extra.append((s, r, ts + pd.Timedelta(minutes=40 * j), round(amt * (0.98 ** j), 2), "Transfer", 1, kind))
        elif kind == "fan_out":  # one account spraying money to many new accounts
            k = int(rng.integers(8, 13))
            targets = rng.choice([x for x in range(1, n_accounts + 1) if _acc(x) != acc], size=k, replace=False)
            for j, t in enumerate(targets):
                extra.append((acc, _acc(int(t)), ts + pd.Timedelta(minutes=4 * j), round(float(rng.uniform(300, 900)), 2), "Transfer", 1, kind))
        else:  # fan_in: many accounts paying one collector account
            k = int(rng.integers(8, 13))
            senders = rng.choice([x for x in range(1, n_accounts + 1) if _acc(x) != acc], size=k, replace=False)
            for j, s in enumerate(senders):
                extra.append((_acc(int(s)), acc, ts + pd.Timedelta(minutes=4 * j), round(float(rng.uniform(300, 900)), 2), "Transfer", 1, kind))

    df = pd.concat([normal, pd.DataFrame(extra, columns=COLUMNS)], ignore_index=True)
    df = df.sort_values("timestamp").reset_index(drop=True)
    df.insert(0, "tx_id", [f"TX{i:06d}" for i in range(1, len(df) + 1)])
    return df


def main() -> None:
    p = argparse.ArgumentParser(description="Generate synthetic transactions")
    p.add_argument("--out", default="data/demo_transactions.csv")
    p.add_argument("--accounts", type=int, default=60)
    p.add_argument("--days", type=int, default=30)
    p.add_argument("--groups", type=int, default=30, help="number of injected anomaly groups")
    p.add_argument("--seed", type=int, default=42)
    a = p.parse_args()
    df = generate_transactions(a.accounts, a.days, n_anomaly_groups=a.groups, seed=a.seed)
    Path(a.out).parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(a.out, index=False)
    print(f"Wrote {len(df)} transactions ({int(df.is_injected_anomaly.sum())} injected anomalous) to {a.out}")


if __name__ == "__main__":
    main()

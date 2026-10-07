"""Run with:  python -m unittest discover -s tests -v   (or pytest)"""
import sys, unittest
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.graph.graph_detector import GraphDetector
from src.rules.amount_spike import AmountSpikeRule
from src.rules.odd_hour import OddHourRule
from src.rules.repeated_transaction import RepeatedTransactionRule
from src.scoring.hybrid import HybridScorer


def tx(rows):
    return pd.DataFrame(rows, columns=["account_id", "receiver_id", "timestamp", "amount"]).assign(
        timestamp=lambda d: pd.to_datetime(d.timestamp))


class RuleTests(unittest.TestCase):
    def test_amount_spike(self):
        rows = [("A", "MRC1", f"2026-01-0{i} 12:00", 100 + i) for i in range(1, 8)] + [("A", "MRC1", "2026-01-09 12:00", 5000)]
        flags = AmountSpikeRule().apply(tx(rows))
        self.assertTrue(flags.iloc[-1]); self.assertFalse(flags.iloc[:-1].any())

    def test_repeated(self):
        rows = [("A", "MRC1", f"2026-01-01 12:0{i}", 250) for i in range(4)] + [("A", "MRC1", "2026-01-05 12:00", 250)]
        flags = RepeatedTransactionRule().apply(tx(rows))
        self.assertEqual(flags.tolist(), [True] * 4 + [False])

    def test_odd_hour(self):
        flags = OddHourRule().apply(tx([("A", "M", "2026-01-01 02:30", 1), ("A", "M", "2026-01-01 14:00", 1)]))
        self.assertEqual(flags.tolist(), [True, False])


class GraphTests(unittest.TestCase):
    def test_cycle_detected(self):
        d = tx([("ACC1", "ACC2", "2026-01-01 10:00", 1000), ("ACC2", "ACC3", "2026-01-01 10:30", 990),
                ("ACC3", "ACC1", "2026-01-01 11:00", 980), ("ACC1", "ACC9", "2026-01-03 10:00", 50)])
        out = GraphDetector().detect(d)
        self.assertEqual(out.graph_cycle.tolist(), [True, True, True, False])

    def test_fan_out(self):
        d = tx([("ACC1", f"ACC{i}", f"2026-01-01 10:{i:02d}", 300) for i in range(10, 20)])
        self.assertTrue(GraphDetector().detect(d).graph_fan_out.all())

    def test_merchants_not_fan_in(self):
        d = tx([(f"ACC{i}", "MRC1", f"2026-01-01 10:{i:02d}", 30) for i in range(10, 30)])
        self.assertFalse(GraphDetector().detect(d).graph_fan_in.any())


class ScoringTests(unittest.TestCase):
    def test_agreement_raises_risk(self):
        base = pd.DataFrame({"flag_amount_spike": [True, True, False], "flag_repeated_transaction": [False, True, False],
                             "flag_odd_hour": [False, False, False]})
        g = pd.DataFrame({"graph_cycle": [False] * 3, "graph_fan_out": [False] * 3, "graph_fan_in": [False] * 3})
        res = HybridScorer().score(base, g, pd.Series([0.0, 0.0, 0.0]))
        self.assertLess(res.risk_score[0], res.risk_score[1])
        self.assertEqual(res.risk_score[2], 0.0)


if __name__ == "__main__":
    unittest.main()

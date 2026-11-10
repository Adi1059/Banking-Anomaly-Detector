"""Tests for engine.py (the Python side that runs the C++ detector).

Run from the repository root:   python -m unittest discover tests -v
"""
import sys
import unittest
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import engine  # noqa: E402


class EngineTests(unittest.TestCase):
    def test_build(self):
        ok, message = engine.build()
        self.assertTrue(ok, message)
        self.assertTrue(engine.BINARY.exists())

    def test_demo_run_returns_scored_table(self):
        ok, message, df = engine.run_demo()
        self.assertTrue(ok, message)
        self.assertGreater(len(df), 3000)
        for column in ("tx_id", "account_id", "risk_score", "risk_level", "reasons"):
            self.assertIn(column, df.columns)
        alert_share = (df["risk_level"].isin(["MEDIUM", "HIGH"])).mean()
        self.assertGreater(alert_share, 0.01)       # some alerts ...
        self.assertLess(alert_share, 0.15)          # ... but not "everything is suspicious"

    def test_needs_scoring(self):
        raw = pd.DataFrame({"account_id": ["A"], "timestamp": ["2026-03-05 14:07:09"], "amount": [10.0]})
        self.assertTrue(engine.needs_scoring(raw))
        self.assertFalse(engine.needs_scoring(raw.assign(risk_score=0.1)))
        self.assertFalse(engine.needs_scoring(raw.rename(columns={"account_id": "sender"}).assign(Risk=0.1)))

    def test_missing_columns_are_reported(self):
        ok, message, df = engine.score_dataframe(pd.DataFrame({"amount": [1.0]}))
        self.assertFalse(ok)
        self.assertIn("account_id", message)
        self.assertIsNone(df)

    def test_empty_file_is_rejected(self):
        empty = pd.DataFrame({"account_id": [], "timestamp": [], "amount": []})
        ok, message, df = engine.score_dataframe(empty)
        self.assertFalse(ok)
        self.assertIsNone(df)

    def test_score_raw_transactions(self):
        raw = pd.DataFrame({
            "account_id": ["ACC1", "ACC1"],
            "receiver_id": ["MRC1", "MRC1"],
            "timestamp": ["2026-03-05 14:07:09", "2026-03-06 15:10:00"],
            "amount": [25.0, 30.0],
        })
        ok, message, df = engine.score_dataframe(raw)
        self.assertTrue(ok, message)
        self.assertEqual(len(df), 2)
        self.assertTrue((df["risk_level"] == "NONE").all())     # two ordinary payments are not alerts


if __name__ == "__main__":
    unittest.main()

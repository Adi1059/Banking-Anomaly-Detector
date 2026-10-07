# Banking Anomaly Detector

Hybrid transaction anomaly detection: **rules + graph analysis + machine learning**, fused into one
explainable risk score.

```
transactions ─► RuleEngine ───────┐
             ─► GraphDetector ────┼─► HybridScorer ─► risk score (0-1), level, reasons
             ─► MLDetector (IF) ──┘
```

## Run
```bash
pip install -r requirements.txt
python -m src.data.generator     # data/demo_transactions.csv (labelled synthetic data)
python -m src.pipeline           # data/scored_transactions.csv + evaluation table
python -m unittest discover -s tests -v
```

## Components
| Module | What it does |
|---|---|
| `src/data/generator.py` | Synthetic accounts/merchants + injected anomalies (spike, repeat, odd hour, ring, fan-out, fan-in) |
| `src/rules/` | Amount Spike (robust MAD z-score), Repeated Transaction (sliding window), Odd Hour, `RuleEngine` |
| `src/graph/graph_detector.py` | Time-ordered cycle detection (A→B→C→A), fan-out / fan-in hubs |
| `src/ml/` | Behavioural features + Isolation Forest (unsupervised) |
| `src/scoring/hybrid.py` | Weighted noisy-OR fusion → LOW / MEDIUM / HIGH + human-readable reasons |
| `src/pipeline.py` | End-to-end run and comparison of rules vs graph vs ML vs hybrid |

## Scoring idea
`risk = 1 − (1 − a·rule)(1 − b·graph)(1 − c·ml)`. One weak signal stays LOW (watchlist); a strong signal
or agreement between independent detectors becomes an alert (risk ≥ 0.5).

Not yet implemented: dashboard, C++ acceleration, more rules.

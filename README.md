# Banking Anomaly Detector

Rule-based transaction anomaly detection (part 1 of the project).

## Run
```bash
pip install -r requirements.txt
python -m src.data.generator          # writes data/demo_transactions.csv
python -m src.rules.engine            # writes data/flagged_transactions.csv
```

## Included
- Synthetic transaction generator (with injected, labelled anomalies)
- Rules: Amount Spike (robust MAD z-score), Repeated Transaction (sliding window), Odd Hour
- `RuleEngine` that runs rules and reports `rules_triggered` / `is_anomaly`

Not yet implemented: ML, graph detection, hybrid scoring, dashboard, remaining rules.

# Banking Anomaly Detector (C++17)

[![CI](https://github.com/Adi1059/Banking-Anomaly-Detector/actions/workflows/ci.yml/badge.svg)](https://github.com/Adi1059/Banking-Anomaly-Detector/actions/workflows/ci.yml)

Hybrid transaction anomaly detection for banking data, built for the PBL course (Data Structures, OOP with C++, OS, DBMS).
Three independent detectors - **six configurable rules**, **graph analysis** and an **Isolation Forest written from scratch** -
are fused into one explainable risk score (0-1) with a level (NONE / LOW / MEDIUM / HIGH) and plain-language reasons.

```
transactions -> RuleEngine (6 rules, parallel) --+
             -> GraphDetector (DFS, fan, Union-Find) --+--> HybridScorer --> ranked alerts
             -> MLDetector (Isolation Forest) ---------+        |-> CSV / JSON report
                                                                |-> MySQL load script
                                                                |-> Observer publishers (console table, CSV log)
```

Team Gol D Roger - DSCPP-III-2026-T249 - Graphic Era (Deemed to be University), Dehradun.

## Build and run (folder `cpp/`)
```bash
cd cpp
make                  # builds ./detector          (Windows: build.bat, needs g++ from MinGW-w64)
make test             # builds and runs the unit tests
./detector            # generates data/demo_transactions.csv, scores it, prints the report
```
Other commands (CSV or JSON is chosen from the file extension):
```bash
detector generate data/demo_transactions.json
detector detect data/demo_transactions.csv data/scored.json --config config/default.json
detector alerts data/demo_transactions.csv 10 data/alerts.csv      # Observer pattern: ranked table + CSV log
detector sql data/demo_transactions.csv data/load_alerts.sql       # then: mysql < sql/schema.sql ; mysql < data/load_alerts.sql
```
`detect` also works on your own file with columns `account_id, receiver_id, timestamp, amount`.

## Website (Streamlit dashboard)
The website is only a front end: all detection happens in the C++ program. `engine.py` builds the program if needed,
runs it, and hands the scored CSV to the page.
```bash
pip install -r requirements.txt
streamlit run app.py
```
* **Run the C++ detector on demo data** - generates synthetic transactions, scores them and shows the result.
* **Upload a raw CSV** (`account_id, timestamp, amount`, optional `receiver_id`) - the C++ backend scores it for you.
* **Upload an already scored CSV** - shown as it is.

The first run needs `make` or `g++` on the machine (MinGW-w64 on Windows) to compile `cpp/`.

## Repository layout
```
cpp/        C++17 backend: include/ src/ tests/ config/ data/ Makefile build.bat
sql/        MySQL schema and queries
app.py      Streamlit website            engine.py   Python wrapper that runs the C++ backend
tests/      Python tests for engine.py   docs/       Big-O analysis and viva notes
python_prototype/   early Python prototype (Phase I)
.github/workflows/ci.yml   builds and tests on every push
```

## Rules (thresholds in `cpp/config/default.json`)
| Rule | Idea | Data structure |
|---|---|---|
| Amount spike | robust z-score (median / MAD) per account | hash map |
| Repeated transaction | same amount repeated in a short window | hash map + two pointers |
| Odd hour | activity between 00:00 and 05:00 | - |
| High velocity | >= 6 transactions by one account in 10 min | deque sliding window |
| Large transfer to new payee | first payment to a receiver, >= 5x the account median | hash set + hash map |
| Structuring | >= 3 payments in [90%, 100%) of the 10,000 limit within 24 h | deque sliding window |

Graph module: DFS cycle detection (A->B->C->A), fan-in / fan-out windows, and **Union-Find** clusters of accounts linked by
flagged transfers on the same day. Merchant ids (prefix `MRC`) are excluded from fan analysis.

## OOP concepts used
| Concept | Where |
|---|---|
| Abstraction / inheritance / polymorphism | `Rule` (abstract) -> six concrete rules; `RuleEngine` calls them through base pointers |
| Template-method pattern | `Rule::apply()` calls the virtual `detect()` |
| Observer pattern | `AlertPublisher` (subject), `AlertObserver` (interface), `ConsoleRankedTable`, `CsvAlertLog` |
| Encapsulation / single responsibility | one class per job: `GraphDetector`, `IsolationForest`, `MLDetector`, `HybridScorer`, `TransactionGenerator` |
| RAII / smart pointers | `std::unique_ptr<Rule>` in `RuleEngine`; `std::shared_ptr<AlertObserver>` in `AlertPublisher` |

## Data structures and algorithms used
| Structure / algorithm | Where |
|---|---|
| Deque (`std::deque`) | velocity and structuring sliding windows |
| Union-Find (path halving + union by size) | `UnionFind`, account clusters in `GraphDetector` |
| Threads (`std::async`, `-pthread`) | `RuleEngine::run` evaluates all rules concurrently, results applied in order (deterministic) |
| Hash map / set (`unordered_map`, `unordered_set`) | per-account statistics, group-by, distinct-counterparty counts, seen payees |
| Sliding window / two pointers | repeated-transaction rule, fan-in / fan-out, hourly velocity features |
| Graph as adjacency list + DFS (explicit stack) | cycle detection with time, amount and length limits and a search budget |
| Binary trees stored in arrays | Isolation Forest (100 random trees, path-length scoring) |
| Min-heap (`priority_queue`) | top-K riskiest transactions and the ranked alert table |
| Sorting / median / MAD / percentiles | robust amount-spike statistic, ML score scaling |

The complexity of every part is derived in [`docs/COMPLEXITY.md`](docs/COMPLEXITY.md).

## Database (DBMS)
`sql/schema.sql` creates a normalised MySQL 8 schema (`accounts`, `transactions`, `alerts`, view `v_ranked_alerts`) with keys,
indexes and CHECK constraints; `sql/queries.sql` holds analytical queries; `detector sql` writes the INSERT script.

## Scoring
`risk = 1 - (1 - a*rule)(1 - b*graph)(1 - c*ml)`, with rule and graph weights themselves combined by noisy-OR.
One weak signal stays LOW; a strong signal, or agreement between detectors, becomes an alert (risk >= 0.5).

## Results (synthetic data, seed 42: 3,850 transactions, 216 injected anomalies, 9 anomaly types)
| Method | Precision | Recall | F1 |
|---|---|---|---|
| Rules only | 0.78 | 0.93 | 0.85 |
| Graph only | 1.00 | 0.49 | 0.66 |
| ML only | 0.98 | 0.56 | 0.71 |
| Hybrid | 0.98 | 0.99 | 0.98 |

Hybrid recall by type: velocity 51/51, structuring 25/25, ring 15/15, fan-in 48/48, fan-out 43/43, repeated 19/19, odd hour 5/5,
amount spike 4/5, new payee 3/5.

Scalability (2-core Linux machine, median of 5 runs): 3,850 tx in 0.027 s; 19,073 tx in 0.124 s; 75,884 tx in 0.488 s;
303,087 tx in 2.144 s - close to linear in n.

## Testing
Python wrapper: `python -m unittest discover tests -v` (6 tests; they build and run the real C++ program).

C++ backend: `make test` runs 23 test functions (`cpp/tests/test_core.cpp`, no framework needed): every rule, DFS cycles, fan-in/out,
Union-Find and clusters, Isolation Forest, scorer, JSON round-trip, config loader, SQL export, the Observer publisher, ranked table,
CSV alert log, and edge cases (empty input, a single transaction, identical timestamps, shuffled input order).

## Limitations
* All results are on **synthetic data** and the thresholds were tuned on it, so real-world numbers would be lower.
  Validation on a public dataset (for example PaySim) has **not** been done.
* The MySQL part is a schema, queries and an export script. There is **no live database connector**, and the script has not been
  run against a MySQL server in this repository's test suite.
* A single weak signal stays below the alert line: 1 of 5 amount spikes and 2 of 5 new-payee cases are missed.
* Cycle search is bounded by a per-edge budget, so very dense graphs may be searched incompletely.

## Future work
Live MySQL connector (C API), PaySim validation, weight calibration (for example an amount-aware boost for large first-time
transfers), streaming input.

# Banking Transaction Anomaly Detector

A rule-based, explainable fraud/anomaly detection engine for banking transactions — built with OOP design patterns and classic Data Structures & Algorithms (hash maps, deques, priority queues, graphs).

Every alert this system raises comes with a **plain-English reason** and a **severity score**, so a human reviewer never has to guess why a transaction was flagged.

> **Team:** Gol D Roger — DSCPP-III-2026-T249
> Department of Computer Science & Engineering, Graphic Era (Deemed to be University)

---

## Table of Contents

1. [Why This Exists](#why-this-exists)
2. [Key Features](#key-features)
3. [Architecture](#architecture)
4. [Detection Rules](#detection-rules)
5. [Tech Stack](#tech-stack)
6. [Project Structure](#project-structure)
7. [Getting Started](#getting-started)
8. [Configuration](#configuration)
9. [Usage Examples](#usage-examples)
10. [Running Tests](#running-tests)
11. [Extending the System (Adding a New Rule)](#extending-the-system-adding-a-new-rule)
12. [Roadmap](#roadmap)
13. [Team & Contributions](#team--contributions)
14. [References](#references)

---

## Why This Exists

Banks process an enormous volume of transactions daily — far more than any human team can review manually. Fraud rarely announces itself; it hides in patterns: a sudden spending spike, a repeated transfer, activity at odd hours, or money quietly moving in a circle between accounts.

Most existing solutions force a trade-off:

- **Rule engines** are fast and explainable, but rigid — adding a rule usually means editing core code.
- **ML-based systems** catch subtler patterns, but need large labeled datasets and can't justify their decisions to an auditor.

This project takes a **hybrid, explainable-first approach**: a configurable rule engine as the auditable core, extended with graph-based cross-account analysis and a lightweight feedback loop — without ever becoming a black box.

---

## Key Features

- ✅ **Six+ configurable anomaly rules**, each independently toggleable and tunable via JSON — no code changes needed
- ✅ **Explainable alerts** — every flag includes a human-readable reason, not just a score
- ✅ **Severity-ranked output** — alerts are ranked via a max-heap so reviewers see the worst first
- ✅ **Cross-account graph analysis** — detects circular transfers and money-mule-style relationship fraud that single-transaction rules miss
- ✅ **Zero training data required** — works out of the box on synthetic/cold-start ledgers
- ✅ **Pluggable architecture** — Strategy pattern for rules, Factory pattern for rule construction, Observer pattern for alert delivery
- ✅ **Tamper-evident audit log** — hash-chained alert history for compliance auditability

---

## Architecture

The system is organized into four layers, each with a single responsibility:

```
                 ┌───────────────────────┐
                 │   Synthetic CSV/JSON   │
                 │    Transaction Data    │
                 └───────────┬───────────┘
                              │
                 ┌───────────▼───────────┐
                 │      Domain Layer      │   Transaction, Account
                 └───────────┬───────────┘
                              │
                 ┌───────────▼───────────┐
                 │       Rule Layer       │   AnomalyRule (abstract)
                 │   (Strategy pattern)   │   → AmountSpikeRule, etc.
                 └───────────┬───────────┘
                              │
                 ┌───────────▼───────────┐
                 │     Detection Layer    │   AnomalyDetector
                 │  weighted severity +   │   Graph module (cycles,
                 │     graph analysis     │   mule/ring detection)
                 └───────────┬───────────┘
                              │
                 ┌───────────▼───────────┐
                 │   Presentation Layer   │   Alert objects, ranked
                 │  (Observer pattern)    │   via max-heap, delivered
                 │                        │   to console/file/dashboard
                 └───────────────────────┘
```

**Design patterns used:**

| Pattern | Where | Why |
|---|---|---|
| Strategy | `AnomalyRule` subclasses | New detection logic plugs in without touching existing rules |
| Factory | `RuleFactory` | Builds the active rule set from JSON config at runtime |
| Observer | Alert publisher | Decouples alert *detection* from alert *delivery* (console today, dashboard tomorrow) |

---

## Detection Rules

| Rule | What it catches | Core structure used |
|---|---|---|
| Amount Spike | Transaction far above an account's normal spend (rolling mean/variance) | Deque (sliding window) |
| Repeated Transaction | Same amount/payee repeated abnormally | Hash map (counters) |
| Odd-Hour Activity | Transactions at unusual times for that account | Hash map (time buckets) |
| Velocity Burst | Too many transactions in a short window | Monotonic deque |
| New Payee, Large Transfer | First-time large payment to an unseen payee | Hash set/Bloom filter |
| Structuring | Splitting a large sum into smaller sub-threshold transfers | Hash map + sliding window |
| Circular Transfers | A → B → C → A style laundering loops | Graph + DFS cycle detection |
| Relationship Anomalies | Mule-like pass-through accounts | Graph centrality |

Each rule outputs a **severity contribution** and a **reason string**; the `AnomalyDetector` merges these into one ranked alert per transaction/account.

---

## Tech Stack

- **Language:** C++ (or Java/Python — implementation-agnostic design)
- **Data Structures:** STL / standard library — hash map, deque, priority queue, custom adjacency-list graph
- **Data Format:** CSV / JSON
- **Config:** JSON (thresholds + rule weights)
- **Testing:** Unit tests per rule (GoogleTest/JUnit/pytest depending on language)
- **Version Control:** Git / GitHub

---

## Project Structure

```
banking-anomaly-detector/
├── data/
│   └── synthetic_ledger.csv        # Generated synthetic transaction data
├── config/
│   └── rules_config.json           # Thresholds & weights per rule
├── src/
│   ├── domain/
│   │   ├── Transaction.*
│   │   └── Account.*
│   ├── rules/
│   │   ├── AnomalyRule.*           # Abstract base
│   │   ├── AmountSpikeRule.*
│   │   ├── RepeatedTransactionRule.*
│   │   ├── OddHourRule.*
│   │   ├── VelocityRule.*
│   │   ├── NewPayeeRule.*
│   │   └── StructuringRule.*
│   ├── graph/
│   │   ├── AccountGraph.*          # Adjacency-list graph
│   │   └── CycleDetector.*         # DFS-based cycle detection
│   ├── detector/
│   │   ├── AnomalyDetector.*
│   │   └── RuleFactory.*
│   └── alert/
│       ├── Alert.*
│       ├── AlertRanker.*           # Max-heap ranking
│       └── AlertPublisher.*        # Observer-based delivery
├── tests/
│   └── ...                         # One test file per rule
├── docs/
│   └── complexity_analysis.md      # Big-O breakdown
└── README.md
```

---

## Getting Started

### Prerequisites

- A C++17-capable compiler (`g++`/`clang++`) **or** Python 3.10+ / JDK 17+, depending on the implementation branch
- CMake (if using C++) or `pip`/`maven` for the other language tracks
- Git

### Installation

```bash
# Clone the repository
git clone https://github.com/<your-org>/banking-anomaly-detector.git
cd banking-anomaly-detector

# --- C++ build ---
mkdir build && cd build
cmake ..
make

# --- OR Python setup ---
pip install -r requirements.txt

# --- OR Java setup ---
mvn clean install
```

### Generate Synthetic Data

```bash
python scripts/generate_synthetic_ledger.py --accounts 200 --transactions 5000 --seed 42
```

This produces `data/synthetic_ledger.csv` with seeded fraud scenarios (a mule ring, a structuring case, and an odd-hour spike) so the demo has known ground truth to catch.

---

## Configuration

All thresholds and weights live in `config/rules_config.json` — **no code changes needed to tune the system**:

```json
{
  "rules": [
    {
      "name": "AmountSpikeRule",
      "enabled": true,
      "weight": 0.8,
      "threshold_std_dev": 3.0
    },
    {
      "name": "VelocityRule",
      "enabled": true,
      "weight": 0.6,
      "window_seconds": 300,
      "max_transactions": 5
    },
    {
      "name": "StructuringRule",
      "enabled": true,
      "weight": 0.9,
      "reporting_threshold": 50000,
      "lookback_hours": 24
    }
  ],
  "severity_bands": {
    "low": 0.3,
    "medium": 0.6,
    "high": 0.85
  }
}
```

The `RuleFactory` reads this file at startup and constructs only the enabled rules — disabling a rule or changing a threshold is a config edit, not a redeploy.

---

## Usage Examples

### Run the detector on a dataset

```bash
./anomaly_detector --input data/synthetic_ledger.csv --config config/rules_config.json --output alerts.json
```

### Sample output

```
┌────┬────────────┬──────────┬──────────────────────────────────────────────┬──────────┐
│ #  │ Account    │ Severity │ Reason                                        │ Rule     │
├────┼────────────┼──────────┼──────────────────────────────────────────────┼──────────┤
│ 1  │ ACC-00291  │ HIGH     │ 3 accounts formed a circular transfer loop     │ Graph    │
│ 2  │ ACC-00104  │ HIGH     │ ₹85,000 split into 4 transfers within 6 hours  │ Struct.  │
│ 3  │ ACC-00042  │ MEDIUM   │ Amount ₹42,000 is 4.1σ above account average   │ Spike    │
│ 4  │ ACC-00187  │ LOW      │ First-ever transfer to a new payee (₹12,000)   │ NewPayee │
└────┴────────────┴──────────┴──────────────────────────────────────────────┴──────────┘
```

Alerts are ranked worst-first via the max-heap, so a reviewer can act on the top rows immediately.

---

## Running Tests

```bash
# C++ (GoogleTest)
cd build && ctest --output-on-failure

# Python (pytest)
pytest tests/ -v

# Java (JUnit)
mvn test
```

Each rule has isolated unit tests so a failure points directly at the broken component, not the whole pipeline.

---

## Extending the System (Adding a New Rule)

This is the core design promise of the project — a new rule should never require touching existing code.

1. **Create a new class** extending the abstract `AnomalyRule`:

   ```cpp
   class GeoVelocityRule : public AnomalyRule {
   public:
       AlertResult evaluate(const Transaction& tx, const Account& acc) override {
           // your detection logic here
       }
   };
   ```

2. **Register it** in `RuleFactory` so the factory knows how to build it from config.

3. **Add an entry** to `config/rules_config.json`:

   ```json
   {
     "name": "GeoVelocityRule",
     "enabled": true,
     "weight": 0.7,
     "max_distance_km_per_hour": 800
   }
   ```

4. **Write isolated unit tests** in `tests/` for the new rule.

No changes to `AnomalyDetector`, `Alert`, or any other existing rule are required — that's the point of the Strategy + Factory combination.

---

## Roadmap

| Phase | Milestone |
|---|---|
| M1 | Transaction/Account models, account registry (hash map), CSV loader |
| M2 | Abstract `AnomalyRule`, first three rules (Amount Spike, Repeated, Odd-Hour) |
| M3 | `AnomalyDetector`, weighted severity scoring, priority-queue ranking |
| M4 | Remaining rules (Velocity, New Payee, Structuring) + graph module for cycle detection |
| M5 | Testing on synthetic data, Big-O complexity documentation, final demo prep |

---

## Team & Contributions

| Member | Role | Contribution |
|---|---|---|
| Aditya Rana | Team Lead | Architecture, OOP design, anomaly detection core |
| Kartikeya Arya | Developer | Detection rules, transaction processing |
| Samarth Tomar | Testing / Documentation | Test cases, documentation, evaluation |
| Shivani Semwal | DSA / Developer | Graph analysis, DFS, Union-Find |

**Mentor:** Dr. Vidit Kumar (viditkumar.cse@geu.ac.in)

---

## References

1. Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2009). *Introduction to Algorithms* (3rd ed.). MIT Press.
2. Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). *Design Patterns: Elements of Reusable Object-Oriented Software*. Addison-Wesley.
3. Chandola, V., Banerjee, A., & Kumar, V. (2009). "Anomaly Detection: A Survey." *ACM Computing Surveys*.
4. West, J., & Bhattacharya, M. (2016). "Intelligent Financial Fraud Detection: A Comprehensive Review." *Computers & Security*.
5. Drools Business Rules Management System — https://www.drools.org/
6. Financial Action Task Force (FATF). Guidance on AML and transaction structuring — https://www.fatf-gafi.org/

---

## License

This is an academic project (Project-Based Learning, DSCPP-III) submitted for the 2026–27 session at Graphic Era (Deemed to be University). License terms to be added per department/institution policy.

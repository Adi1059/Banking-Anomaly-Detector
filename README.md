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
9. [Usage](#usage)
10. [Testing Approach](#testing-approach)
11. [Extending the System (Adding a New Rule)](#extending-the-system-adding-a-new-rule)
12. [Roadmap](#roadmap)
13. [Team & Contributions](#team--contributions)
14. [References](#references)

---

## Why This Exists

Banks process an enormous volume of transactions daily — far more than any human team can review manually. Fraud rarely announces itself; it hides in patterns: a sudden spending spike, a repeated transfer, activity at odd hours, or money quietly moving in a circle between accounts.

Most existing solutions force a trade-off:

- **Rule engines** are fast and explainable, but rigid — adding a rule usually means editing core logic.
- **ML-based systems** catch subtler patterns, but need large labeled datasets and can't justify their decisions to an auditor.

This project takes a **hybrid, explainable-first approach**: a configurable rule engine as the auditable core, extended with graph-based cross-account analysis and a lightweight feedback loop — without ever becoming a black box.

---

## Key Features

- **Six or more configurable anomaly rules**, each independently toggleable and tunable through a config file — no code changes needed to retune the system
- **Explainable alerts** — every flag includes a human-readable reason, not just a score
- **Severity-ranked output** — alerts are ranked using a max-heap so reviewers see the worst cases first
- **Cross-account graph analysis** — detects circular transfers and money-mule-style relationship fraud that single-transaction rules miss
- **Zero training data required** — works out of the box on synthetic or cold-start ledgers
- **Pluggable architecture** — a Strategy pattern for individual rules, a Factory pattern for rule construction, and an Observer pattern for alert delivery
- **Tamper-evident audit log** — alert history is chained so past records cannot be silently altered

---

## Architecture

The system is organized into four layers, each with a single responsibility.

**Domain layer** — Represents the ledger itself, through Transaction and Account entities.

**Rule layer** — An abstract anomaly-rule definition, extended by each concrete detection rule (amount spikes, structuring, and so on). This is the Strategy pattern in action: each rule is self-contained and interchangeable.

**Detection layer** — The anomaly detector runs every enabled rule against incoming transactions, merges their outputs into a single weighted severity score, and hands relationship-level questions off to the graph module for cycle and mule-pattern detection.

**Presentation layer** — Alerts become objects in their own right. A max-heap ranks them so the most suspicious activity surfaces immediately, and delivery is decoupled through an Observer-style publisher — console output today, a file or dashboard tomorrow, without changing how alerts are generated.

**Design patterns used:**

| Pattern | Where | Why |
|---|---|---|
| Strategy | Individual anomaly rules | New detection logic plugs in without touching existing rules |
| Factory | Rule construction | Builds the active rule set from configuration at runtime |
| Observer | Alert delivery | Decouples alert *detection* from alert *delivery* |

---

## Detection Rules

| Rule | What it catches | Core structure used |
|---|---|---|
| Amount Spike | Transaction far above an account's normal spend | Deque (sliding window) |
| Repeated Transaction | Same amount or payee repeated abnormally | Hash map (counters) |
| Odd-Hour Activity | Transactions at unusual times for that account | Hash map (time buckets) |
| Velocity Burst | Too many transactions in a short window | Sliding-window deque |
| New Payee, Large Transfer | First-time large payment to an unseen payee | Hash set |
| Structuring | Splitting a large sum into smaller sub-threshold transfers | Hash map plus sliding window |
| Circular Transfers | Loop-style transfers between accounts (laundering pattern) | Graph with cycle detection |
| Relationship Anomalies | Mule-like pass-through accounts | Graph centrality analysis |

Each rule contributes a severity value and a reason string; the detector merges these into one ranked alert per transaction or account.

---

## Tech Stack

- **Language:** C++ (or Java/Python — the design is implementation-agnostic)
- **Data Structures:** Standard library equivalents of hash map, deque, priority queue, and a custom adjacency-list graph
- **Data Format:** CSV or JSON transaction records
- **Configuration:** A structured config file for thresholds and rule weights
- **Testing:** Isolated unit tests per rule
- **Version Control:** Git and GitHub

---

## Project Structure

The repository is organized into a handful of top-level areas: a **data** folder holding the synthetic transaction ledger; a **config** folder holding rule thresholds and weights; a **source** area split into domain models, individual rule implementations, the graph module, the detector and rule factory, and the alert/ranking/publishing components; a **tests** folder with one set of tests per rule; and a **docs** folder holding the complexity analysis and any supporting write-ups.

---

## Getting Started

### Prerequisites

You'll need a working compiler or interpreter toolchain matching whichever implementation track the team settles on (C++, Python, or Java), along with Git for version control. No external services or paid tools are required.

### Setup

Clone the repository, then build or set up the project using the standard tooling for the chosen language track. Full setup steps will be documented in the docs folder once the implementation language is finalized (Milestone 1).

### Synthetic Data

A synthetic transaction ledger is generated with a set of deliberately seeded scenarios — a circular mule ring, a structuring case, and an odd-hour spike — so that during the demo, the detector can be shown catching known, engineered fraud patterns rather than relying purely on random noise.

---

## Configuration

All thresholds and rule weights live in a single configuration file, so tuning the system is a config edit, not a code change. Each rule entry specifies whether it's enabled, how heavily it should weigh into the overall severity score, and whatever thresholds are specific to that rule (a standard-deviation cutoff for amount spikes, a time window and transaction count for velocity bursts, a reporting threshold and lookback period for structuring, and so on). Severity bands (low, medium, high) are also configurable rather than hard-coded.

---

## Usage

The detector is run against a transaction dataset and a configuration file, and produces a ranked list of alerts. Each alert in the output includes the account involved, its severity level, the plain-English reason it was flagged, and which rule (or combination of rules) triggered it. Because alerts are ranked worst-first by the severity heap, a reviewer can act on the top of the list immediately rather than scanning the entire output.

---

## Testing Approach

Every rule is tested in isolation, so a failure points directly at the specific rule that broke rather than the pipeline as a whole. Graph-based detection (cycle finding, relationship analysis) is tested separately from the single-transaction rules, and the detector's merging/ranking logic has its own test coverage independent of any individual rule.

---

## Extending the System (Adding a New Rule)

This is the core design promise of the project — adding a new rule should never require touching existing code.

1. Define a new rule as a subclass of the abstract anomaly-rule type, implementing its own evaluation logic.
2. Register the new rule with the rule factory so it can be constructed from configuration.
3. Add an entry for it in the configuration file, specifying whether it's enabled, its weight, and any thresholds it needs.
4. Write isolated unit tests for the new rule under the tests folder.

No changes to the detector, the alert model, or any other existing rule are required — that's the point of combining the Strategy and Factory patterns.

---

## Roadmap

| Phase | Milestone |
|---|---|
| M1 | Transaction and Account models, account registry, CSV loader |
| M2 | Abstract rule definition, first rules (Amount Spike, Repeated Transaction, Odd-Hour) |
| M3 | Detector core, weighted severity scoring, priority-queue ranking |
| M4 | Remaining rules (Velocity, New Payee, Structuring) plus the graph module for cycle detection |
| M5 | Testing on synthetic data, Big-O complexity documentation, final demo preparation |

---

## Team & Contributions

| Member | Role | Contribution |
|---|---|---|
| Aditya Rana | Team Lead | Architecture, OOP design, anomaly detection core |
| Kartikeya Arya | Developer | Detection rules, transaction processing |
| Samarth Tomar | Testing / Documentation | Test cases, documentation, evaluation |
| Shivani Semwal | DSA / Developer | Graph analysis, cycle detection, Union-Find |

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

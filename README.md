# Banking Transaction Anomaly Detector

**PBL Project – Phase II (Working Prototype)**  
Team: DSCPP-III-2026-T249  
Graphic Era (Deemed to be University)

Hybrid rule-based + graph-based anomaly detection engine for banking transactions.

## Features (Current Status ~70-75%)

- 6 configurable detection rules (Amount Spike, Repeated Tx, Odd Hour, High Velocity, New Payee, Structuring)
- Graph analysis (cycle detection + fan-in/fan-out)
- Explainable alerts with severity levels and reasons
- Hybrid risk scoring
- Synthetic data generator
- CSV input/output
- Clean modular design (easy to explain)

## Project Structure

```
cpp/
├── include/          # Header files (one concept per file)
│   ├── types.hpp     # Core data structures
│   ├── rules.hpp     # Rule hierarchy (OOP)
│   ├── graph.hpp     # Graph-based detection
│   ├── scorer.hpp    # Hybrid scoring
│   ├── generator.hpp # Synthetic data
│   └── io.hpp        # File loading/saving
├── src/              # Implementations
├── config/           # Thresholds
├── data/             # Sample data
└── Makefile
```

## How to Build & Run

```bash
cd cpp
make
./detector              # generates data + runs detection + prints report
./detector generate     # only generate synthetic data
./detector detect       # run on existing data
```

## For the PBL Meeting

This code is intentionally modular so you can explain:

1. **OOP** → `Rule` abstract base class + inheritance + polymorphism
2. **Data Structures** → `unordered_map`, `deque`, adjacency list, priority queue
3. **Algorithms** → Sliding window, DFS cycle detection, robust statistics (median/MAD)
4. **Design** → Separation of concerns (each module has one job)

## Next Steps (remaining 25-30%)

- Full Isolation Forest (ML detector)
- JSON config loading
- SQL export
- Unit tests expansion
- Report exporter (Markdown/PDF)
- Live dashboard (optional)

# How to Explain This Code to the Teacher (PBL Meeting)

## 1. High-Level Architecture (30 seconds)

```
CSV / Synthetic Data
        ↓
   RuleEngine  (6 rules)  ──┐
        ↓                   ├──→ HybridScorer → Risk + Level + Reasons
   GraphDetector            ──┘
```

We have **two independent detectors** that look at the same transactions from different angles, then combine their opinions.

---

## 2. OOP Concepts (most important for marks)

### Abstraction + Inheritance + Polymorphism
- File: `include/rules.hpp`
- `class Rule` is **abstract** (has pure virtual functions)
- Six concrete rules inherit from it:
  - `AmountSpikeRule`
  - `RepeatedTransactionRule`
  - `OddHourRule`
  - `VelocityRule`
  - `NewPayeeRule`
  - `StructuringRule`
- `RuleEngine` stores `vector<unique_ptr<Rule>>` and calls them **through the base class pointer** → classic polymorphism.

### Template Method Pattern
- `Rule::apply()` contains the common logic
- It calls the pure virtual `detect()` that each child implements differently.

---

## 3. Data Structures Used

| Structure              | Where used                          | Why |
|------------------------|-------------------------------------|-----|
| `unordered_map`        | Rules + Graph                       | Fast per-account statistics |
| `deque`                | VelocityRule, StructuringRule       | Efficient sliding window |
| Adjacency list         | GraphDetector                       | Represent money-flow graph |
| `priority_queue` (min-heap) | main.cpp (Top-5)               | Efficiently keep only highest risk |
| `vector`               | Everywhere                          | Dynamic arrays |

---

## 4. Key Algorithms

1. **Robust Z-Score (Amount Spike)**  
   Uses **Median + MAD** instead of mean/std-dev → resistant to outliers.

2. **Sliding Window with deque**  
   High velocity and structuring rules.

3. **DFS for Cycle Detection**  
   Looks for money returning to the origin account (classic laundering pattern).

4. **Fan-in / Fan-out**  
   One-to-many or many-to-one patterns in short time.

---

## 5. What is Already Working (≈70-75%)

✅ 6 fully working rules  
✅ Graph cycle + fan-in/out detection  
✅ Hybrid scoring with severity levels  
✅ Explainable reasons for every alert  
✅ Synthetic data generator with injected anomalies  
✅ CSV load / save  
✅ Clean modular file structure  

## 6. What is Left (remaining 25-30%)

- Machine Learning detector (Isolation Forest)
- JSON configuration loading
- SQL export / database integration
- Markdown/PDF report generator
- More unit tests
- Optional dashboard

---

## 7. How to Demo Live

```bash
cd cpp
make
./detector          # generates data + runs everything + shows Top-5
```

Show the teacher:
1. The modular folder structure
2. `rules.hpp` (OOP)
3. One rule implementation (e.g. VelocityRule – deque)
4. Graph cycle detection
5. Final console output with reasons

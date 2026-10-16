#pragma once
// ============================================================
// rules.hpp  –  Rule hierarchy (OOP showcase)
// ============================================================
// Design:
//   Rule (abstract base)  →  many concrete rules
//   RuleEngine owns a list of rules and runs them polymorphically
//
// This demonstrates:
//   - Abstraction
//   - Inheritance
//   - Polymorphism (virtual functions)
//   - Template Method pattern
// ============================================================

#include <memory>
#include <string>
#include <vector>
#include "types.hpp"

// ------------------------------------------------------------
// Abstract base class for every detection rule
// ------------------------------------------------------------
class Rule {
public:
    virtual ~Rule() = default;

    virtual std::string name()   const = 0;   // human name
    virtual double      weight() const = 0;   // how strong this rule is (0..1)

    // Pure virtual – each rule implements its own logic
    virtual std::vector<bool> detect(const std::vector<Transaction>& tx) const = 0;

    // Template Method: common logic that calls the virtual detect()
    void apply(const std::vector<Transaction>& tx, std::vector<Signals>& signals) const {
        auto flags = detect(tx);
        for (size_t i = 0; i < tx.size(); ++i) {
            if (flags[i]) {
                signals[i].addRuleHit(name(), weight());
            }
        }
    }
};

// ------------------------------------------------------------
// 1. Amount Spike Rule
//    Flags transactions whose amount is far above the account's
//    usual spending (uses median + MAD – robust statistics)
// ------------------------------------------------------------
class AmountSpikeRule : public Rule {
public:
    explicit AmountSpikeRule(double zThreshold = 3.5) : threshold_(zThreshold) {}
    std::string name()   const override { return "amount_spike"; }
    double      weight() const override { return 0.55; }
    std::vector<bool> detect(const std::vector<Transaction>& tx) const override;
private:
    double threshold_;
};

// ------------------------------------------------------------
// 2. Repeated Transaction Rule
//    Same amount repeated many times in a short window
// ------------------------------------------------------------
class RepeatedTransactionRule : public Rule {
public:
    RepeatedTransactionRule(int windowSec = 600, int minRepeats = 3)
        : windowSec_(windowSec), minRepeats_(minRepeats) {}
    std::string name()   const override { return "repeated_transaction"; }
    double      weight() const override { return 0.60; }
    std::vector<bool> detect(const std::vector<Transaction>& tx) const override;
private:
    int windowSec_;
    int minRepeats_;
};

// ------------------------------------------------------------
// 3. Odd Hour Rule
//    Activity between 00:00 – 05:00
// ------------------------------------------------------------
class OddHourRule : public Rule {
public:
    OddHourRule(int start = 0, int end = 5) : startHour_(start), endHour_(end) {}
    std::string name()   const override { return "odd_hour"; }
    double      weight() const override { return 0.30; }
    std::vector<bool> detect(const std::vector<Transaction>& tx) const override;
private:
    int startHour_, endHour_;
};

// ------------------------------------------------------------
// 4. High Velocity Rule
//    Too many transactions by one account in a short time
//    (uses deque as sliding window)
// ------------------------------------------------------------
class VelocityRule : public Rule {
public:
    VelocityRule(int windowSec = 600, int maxCount = 6)
        : windowSec_(windowSec), maxCount_(maxCount) {}
    std::string name()   const override { return "high_velocity"; }
    double      weight() const override { return 0.60; }
    std::vector<bool> detect(const std::vector<Transaction>& tx) const override;
private:
    int windowSec_;
    int maxCount_;
};

// ------------------------------------------------------------
// 5. Large Transfer to New Payee
// ------------------------------------------------------------
class NewPayeeRule : public Rule {
public:
    explicit NewPayeeRule(double multiplier = 5.0) : mult_(multiplier) {}
    std::string name()   const override { return "new_payee_large"; }
    double      weight() const override { return 0.55; }
    std::vector<bool> detect(const std::vector<Transaction>& tx) const override;
private:
    double mult_;
};

// ------------------------------------------------------------
// 6. Structuring / Smurfing Rule
//    Many payments just below reporting limit (e.g. 10 000)
// ------------------------------------------------------------
class StructuringRule : public Rule {
public:
    StructuringRule(double limit = 10000.0, double band = 0.90, int minCount = 3)
        : limit_(limit), band_(band), minCount_(minCount) {}
    std::string name()   const override { return "structuring"; }
    double      weight() const override { return 0.65; }
    std::vector<bool> detect(const std::vector<Transaction>& tx) const override;
private:
    double limit_, band_;
    int minCount_;
};

// ------------------------------------------------------------
// RuleEngine – owns and runs all rules
// ------------------------------------------------------------
class RuleEngine {
public:
    void add(std::unique_ptr<Rule> rule) {
        rules_.push_back(std::move(rule));
    }

    // Factory that creates the default set of 6 rules
    static RuleEngine createDefault();

    void run(const std::vector<Transaction>& tx, std::vector<Signals>& signals) const;

    size_t size() const { return rules_.size(); }

private:
    std::vector<std::unique_ptr<Rule>> rules_;
};

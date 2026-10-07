#pragma once
// Rule-based detection. OOP showcase: abstract base class + virtual functions (polymorphism),
// template-method pattern (Rule::apply calls the virtual detect()).
#include <memory>
#include <string>
#include <vector>
#include "types.hpp"

class Rule {
public:
    virtual ~Rule() = default;
    virtual std::string name() const = 0;
    virtual double weight() const = 0;                                               // confidence of this rule alone
    virtual std::vector<char> detect(const std::vector<Transaction>& tx) const = 0;  // 1 = flagged
    void apply(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const;
};

// Amount far above the account's usual spending (robust z-score using median and MAD).
class AmountSpikeRule : public Rule {
public:
    explicit AmountSpikeRule(double threshold = 3.5, size_t minHistory = 5) : threshold_(threshold), minHistory_(minHistory) {}
    std::string name() const override { return "amount spike"; }
    double weight() const override { return 0.50; }
    std::vector<char> detect(const std::vector<Transaction>& tx) const override;
private:
    double threshold_; size_t minHistory_;
};

// Same account repeats the same amount several times within a short window (sliding window).
class RepeatedTransactionRule : public Rule {
public:
    explicit RepeatedTransactionRule(int64_t windowSec = 600, size_t minRepeats = 3) : windowSec_(windowSec), minRepeats_(minRepeats) {}
    std::string name() const override { return "repeated transaction"; }
    double weight() const override { return 0.60; }
    std::vector<char> detect(const std::vector<Transaction>& tx) const override;
private:
    int64_t windowSec_; size_t minRepeats_;
};

// Activity during unusual hours [startHour, endHour).
class OddHourRule : public Rule {
public:
    explicit OddHourRule(int startHour = 0, int endHour = 5) : start_(startHour), end_(endHour) {}
    std::string name() const override { return "odd hour"; }
    double weight() const override { return 0.30; }
    std::vector<char> detect(const std::vector<Transaction>& tx) const override;
private:
    int start_, end_;
};

// Owns a list of rules and runs them polymorphically.
class RuleEngine {
public:
    void add(std::unique_ptr<Rule> r) { rules_.push_back(std::move(r)); }
    static RuleEngine withDefaultRules();
    void run(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const;
    size_t size() const { return rules_.size(); }
private:
    std::vector<std::unique_ptr<Rule>> rules_;
};

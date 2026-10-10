#pragma once
// Rule-based detection. OOP showcase: abstract base class + virtual functions (polymorphism),
// template-method pattern (Rule::apply calls the virtual detect()).
#include <memory>
#include <string>
#include <vector>
#include "config.hpp"
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

// Too many transactions by one account in a short time (deque sliding window).
class VelocityRule : public Rule {
public:
    explicit VelocityRule(int64_t windowSec = 600, size_t maxCount = 6) : windowSec_(windowSec), maxCount_(maxCount) {}
    std::string name() const override { return "high velocity"; }
    double weight() const override { return 0.60; }
    std::vector<char> detect(const std::vector<Transaction>& tx) const override;
private:
    int64_t windowSec_; size_t maxCount_;
};

// First-ever payment to a receiver that is far larger than the account's usual amount (hash set of seen payees).
class NewPayeeLargeTransferRule : public Rule {
public:
    explicit NewPayeeLargeTransferRule(double multiplier = 5.0, size_t minHistory = 5) : mult_(multiplier), minHistory_(minHistory) {}
    std::string name() const override { return "large transfer to new payee"; }
    double weight() const override { return 0.60; }
    std::vector<char> detect(const std::vector<Transaction>& tx) const override;
private:
    double mult_; size_t minHistory_;
};

// Several payments just below a reporting limit within a day ("smurfing"), tracked with a deque window.
class StructuringRule : public Rule {
public:
    StructuringRule(double limit = 10000.0, double band = 0.9, size_t minCount = 3, int64_t windowSec = 86400)
        : limit_(limit), band_(band), minCount_(minCount), windowSec_(windowSec) {}
    std::string name() const override { return "structuring"; }
    double weight() const override { return 0.65; }
    std::vector<char> detect(const std::vector<Transaction>& tx) const override;
private:
    double limit_, band_; size_t minCount_; int64_t windowSec_;
};

// Owns a list of rules and runs them polymorphically.
class RuleEngine {
public:
    void add(std::unique_ptr<Rule> r) { rules_.push_back(std::move(r)); }
    static RuleEngine withDefaultRules();
    static RuleEngine fromConfig(const DetectorConfig& cfg);
    void run(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const;
    size_t size() const { return rules_.size(); }
private:
    std::vector<std::unique_ptr<Rule>> rules_;
};

// ============================================================
// rules.cpp  –  Implementation of all detection rules
// ============================================================

#include "rules.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <deque>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ------------------------------------------------------------
// Helper: median of a vector
// ------------------------------------------------------------
static double median(std::vector<double> v) {
  if (v.empty())
    return 0.0;
  std::sort(v.begin(), v.end());
  size_t n = v.size();
  return (n % 2 == 0) ? (v[n / 2 - 1] + v[n / 2]) / 2.0 : v[n / 2];
}

// ------------------------------------------------------------
// 1. Amount Spike (robust z-score using median + MAD)
// ------------------------------------------------------------
std::vector<bool>
AmountSpikeRule::detect(const std::vector<Transaction> &tx) const {
  std::vector<bool> flags(tx.size(), false);

  // Group amounts by sender
  std::unordered_map<std::string, std::vector<double>> history;
  for (const auto &t : tx) {
    history[t.sender].push_back(t.amount);
  }

  // Pre-compute median and MAD per account
  std::unordered_map<std::string, double> med, mad;
  for (auto &kv : history) {
    double m = median(kv.second);
    med[kv.first] = m;
    std::vector<double> absDev;
    for (double a : kv.second)
      absDev.push_back(std::abs(a - m));
    mad[kv.first] = median(absDev) * 1.4826; // consistency constant
  }

  for (size_t i = 0; i < tx.size(); ++i) {
    const auto &t = tx[i];
    double m = med[t.sender];
    double md = mad[t.sender];
    if (md < 1e-6)
      continue; // not enough variation
    double z = (t.amount - m) / md;
    if (z >= threshold_)
      flags[i] = true;
  }
  return flags;
}

// ------------------------------------------------------------
// 2. Repeated Transaction (same amount, short window)
// ------------------------------------------------------------
std::vector<bool>
RepeatedTransactionRule::detect(const std::vector<Transaction> &tx) const {
  std::vector<bool> flags(tx.size(), false);

  // Group by (sender, amount)
  std::map<std::pair<std::string, long long>, std::vector<size_t>> groups;
  for (size_t i = 0; i < tx.size(); ++i) {
    long long keyAmt = static_cast<long long>(tx[i].amount * 100); // cents
    groups[{tx[i].sender, keyAmt}].push_back(i);
  }

  for (auto &kv : groups) {
    auto &indices = kv.second;
    // Sliding window on timestamps
    std::deque<size_t> window;
    for (size_t idx : indices) {
      while (!window.empty() &&
             tx[idx].timestamp - tx[window.front()].timestamp > windowSec_) {
        window.pop_front();
      }
      window.push_back(idx);
      if ((int)window.size() >= minRepeats_) {
        for (size_t j : window)
          flags[j] = true;
      }
    }
  }
  return flags;
}

// ------------------------------------------------------------
// 3. Odd Hour
// ------------------------------------------------------------
std::vector<bool>
OddHourRule::detect(const std::vector<Transaction> &tx) const {
  std::vector<bool> flags(tx.size(), false);
  for (size_t i = 0; i < tx.size(); ++i) {
    // Convert Unix timestamp to hour (UTC)
    time_t t = static_cast<time_t>(tx[i].timestamp);
    struct tm *tm = gmtime(&t);
    int hour = tm->tm_hour;
    if (hour >= startHour_ && hour < endHour_) {
      flags[i] = true;
    }
  }
  return flags;
}

// ------------------------------------------------------------
// 4. High Velocity (deque sliding window)
// ------------------------------------------------------------
std::vector<bool>
VelocityRule::detect(const std::vector<Transaction> &tx) const {
  std::vector<bool> flags(tx.size(), false);

  // Group indices by sender
  std::unordered_map<std::string, std::vector<size_t>> bySender;
  for (size_t i = 0; i < tx.size(); ++i) {
    bySender[tx[i].sender].push_back(i);
  }

  for (auto &kv : bySender) {
    auto &indices = kv.second;
    std::deque<size_t> window;
    for (size_t idx : indices) {
      while (!window.empty() &&
             tx[idx].timestamp - tx[window.front()].timestamp > windowSec_) {
        window.pop_front();
      }
      window.push_back(idx);
      if ((int)window.size() >= maxCount_) {
        for (size_t j : window)
          flags[j] = true;
      }
    }
  }
  return flags;
}

// ------------------------------------------------------------
// 5. Large transfer to a brand-new payee
// ------------------------------------------------------------
std::vector<bool>
NewPayeeRule::detect(const std::vector<Transaction> &tx) const {
  std::vector<bool> flags(tx.size(), false);

  std::unordered_map<std::string, std::unordered_set<std::string>> seenPayees;
  std::unordered_map<std::string, std::vector<double>> amounts;

  // First pass: collect history
  for (const auto &t : tx) {
    amounts[t.sender].push_back(t.amount);
  }
  std::unordered_map<std::string, double> medAmt;
  for (auto &kv : amounts) {
    medAmt[kv.first] = median(kv.second);
  }

  // Second pass: detect
  for (size_t i = 0; i < tx.size(); ++i) {
    const auto &t = tx[i];
    bool isNew = seenPayees[t.sender].count(t.receiver) == 0;
    seenPayees[t.sender].insert(t.receiver);

    if (isNew && medAmt[t.sender] > 0 && t.amount >= mult_ * medAmt[t.sender]) {
      flags[i] = true;
    }
  }
  return flags;
}

// ------------------------------------------------------------
// 6. Structuring (smurfing)
// ------------------------------------------------------------
std::vector<bool>
StructuringRule::detect(const std::vector<Transaction> &tx) const {
  std::vector<bool> flags(tx.size(), false);
  double low = limit_ * band_;

  std::unordered_map<std::string, std::vector<size_t>> bySender;
  for (size_t i = 0; i < tx.size(); ++i) {
    if (tx[i].amount >= low && tx[i].amount < limit_) {
      bySender[tx[i].sender].push_back(i);
    }
  }

  for (auto &kv : bySender) {
    auto &indices = kv.second;
    std::deque<size_t> window;
    for (size_t idx : indices) {
      // 24-hour window
      while (!window.empty() &&
             tx[idx].timestamp - tx[window.front()].timestamp > 86400) {
        window.pop_front();
      }
      window.push_back(idx);
      if ((int)window.size() >= minCount_) {
        for (size_t j : window)
          flags[j] = true;
      }
    }
  }
  return flags;
}

// ------------------------------------------------------------
// RuleEngine
// ------------------------------------------------------------
RuleEngine RuleEngine::createDefault() {
  RuleEngine eng;
  eng.add(std::make_unique<AmountSpikeRule>());
  eng.add(std::make_unique<RepeatedTransactionRule>());
  eng.add(std::make_unique<OddHourRule>());
  eng.add(std::make_unique<VelocityRule>());
  eng.add(std::make_unique<NewPayeeRule>());
  eng.add(std::make_unique<StructuringRule>());
  return eng;
}

void RuleEngine::run(const std::vector<Transaction> &tx,
                     std::vector<Signals> &signals) const {
  for (const auto &rule : rules_) {
    rule->apply(tx, signals);
  }
}

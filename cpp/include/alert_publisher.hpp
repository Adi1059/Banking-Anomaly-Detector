#pragma once
// Observer pattern: the pipeline publishes every alert to any number of subscribed observers.
//   AlertPublisher      - the subject; keeps a list of observers and notifies them of each alert
//   AlertObserver       - abstract observer interface (onAlert / onFinish)
//   ConsoleRankedTable  - keeps only the K riskiest alerts in a min-heap and prints a ranked table at the end
//   CsvAlertLog         - appends every alert to a CSV file for human review
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include "types.hpp"

class AlertObserver {
public:
    virtual ~AlertObserver() = default;
    virtual void onAlert(const Transaction& tx, const Signals& sig) = 0;
    virtual void onFinish() {}
};

class AlertPublisher {
public:
    void subscribe(std::shared_ptr<AlertObserver> observer);
    size_t observers() const { return observers_.size(); }
    // Notifies every observer about each transaction with risk >= threshold (in input order),
    // then calls onFinish() on every observer once. Returns the number of alerts published.
    size_t publish(const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double threshold) const;

private:
    std::vector<std::shared_ptr<AlertObserver>> observers_;
};

class ConsoleRankedTable : public AlertObserver {
public:
    explicit ConsoleRankedTable(size_t topK = 10, std::ostream& out = std::cout) : topK_(topK), out_(out) {}
    void onAlert(const Transaction& tx, const Signals& sig) override;
    void onFinish() override;

private:
    struct Entry { Transaction tx; Signals sig; };
    // "a is better than b": higher risk first, ties broken by smaller transaction id (deterministic output).
    static bool better(const Entry& a, const Entry& b) {
        return a.sig.risk > b.sig.risk || (a.sig.risk == b.sig.risk && a.tx.id < b.tx.id);
    }
    struct Cmp { bool operator()(const Entry& a, const Entry& b) const { return better(a, b); } };

    size_t topK_;
    std::ostream& out_;
    std::priority_queue<Entry, std::vector<Entry>, Cmp> heap_;   // top() is the WORST kept alert (min-heap on risk)
};

class CsvAlertLog : public AlertObserver {
public:
    explicit CsvAlertLog(const std::string& path);
    bool ok() const { return file_.is_open() && file_.good(); }
    size_t written() const { return written_; }
    void onAlert(const Transaction& tx, const Signals& sig) override;
    void onFinish() override { file_.flush(); }

private:
    std::ofstream file_;
    size_t written_ = 0;
};

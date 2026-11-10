#include "alert_publisher.hpp"
#include <algorithm>
#include <iomanip>
#include <utility>
#include "timeutil.hpp"

// ---------------- AlertPublisher ----------------
void AlertPublisher::subscribe(std::shared_ptr<AlertObserver> observer) {
    if (observer) observers_.push_back(std::move(observer));
}

size_t AlertPublisher::publish(const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double threshold) const {
    size_t published = 0;
    const size_t n = std::min(tx.size(), sig.size());
    for (size_t i = 0; i < n; ++i) {
        if (sig[i].risk < threshold) continue;
        for (const auto& o : observers_) o->onAlert(tx[i], sig[i]);
        ++published;
    }
    for (const auto& o : observers_) o->onFinish();
    return published;
}

// ---------------- ConsoleRankedTable ----------------
void ConsoleRankedTable::onAlert(const Transaction& tx, const Signals& sig) {
    if (topK_ == 0) return;
    heap_.push({tx, sig});
    if (heap_.size() > topK_) heap_.pop();                       // drop the worst alert: O(log K) per alert
}

void ConsoleRankedTable::onFinish() {
    std::vector<Entry> ranked;
    while (!heap_.empty()) { ranked.push_back(heap_.top()); heap_.pop(); }
    std::reverse(ranked.begin(), ranked.end());                  // worst-first -> best-first

    out_ << "\nRanked alerts (top " << topK_ << ")\n";
    out_ << std::left << std::setw(5) << "Rank" << std::setw(10) << "Tx" << std::setw(10) << "From" << std::setw(10) << "To"
         << std::right << std::setw(11) << "Amount" << std::setw(7) << "Risk" << "  " << std::left << std::setw(8) << "Level"
         << "Reasons\n";
    size_t rank = 1;
    for (const auto& e : ranked) {
        out_ << std::left << std::setw(5) << rank++ << std::setw(10) << e.tx.id << std::setw(10) << e.tx.sender << std::setw(10)
             << e.tx.receiver << std::right << std::fixed << std::setprecision(2) << std::setw(11) << e.tx.amount << std::setw(7)
             << e.sig.risk << "  " << std::left << std::setw(8) << e.sig.level;
        for (size_t r = 0; r < e.sig.reasons.size(); ++r) out_ << (r ? "; " : "") << e.sig.reasons[r];
        out_ << "\n";
    }
    if (ranked.empty()) out_ << "(no alerts)\n";
}

// ---------------- CsvAlertLog ----------------
CsvAlertLog::CsvAlertLog(const std::string& path) : file_(path) {
    if (file_) file_ << "tx_id,account_id,receiver_id,timestamp,amount,risk_score,risk_level,reasons\n";
}

void CsvAlertLog::onAlert(const Transaction& tx, const Signals& sig) {
    if (!file_) return;
    std::string reasons;
    for (size_t r = 0; r < sig.reasons.size(); ++r) reasons += (r ? "; " : "") + sig.reasons[r];
    std::string quoted = "\"";
    for (char c : reasons) { if (c == '"') quoted += '"'; quoted += c; }   // CSV escaping: double any quote
    quoted += '"';
    file_ << tx.id << ',' << tx.sender << ',' << tx.receiver << ',' << formatTime(tx.ts) << ',' << std::fixed << std::setprecision(2)
          << tx.amount << ',' << std::setprecision(3) << sig.risk << ',' << sig.level << ',' << quoted << '\n';
    ++written_;
}

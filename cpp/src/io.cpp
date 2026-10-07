#include "io.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "timeutil.hpp"

namespace {
std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    std::stringstream ss(line);
    while (std::getline(ss, cur, ',')) {
        if (!cur.empty() && cur.back() == '\r') cur.pop_back();
        out.push_back(cur);
    }
    if (!line.empty() && line.back() == ',') out.emplace_back();
    return out;
}
std::string joinReasons(const std::vector<std::string>& r) {
    std::string s;
    for (size_t i = 0; i < r.size(); ++i) s += (i ? "; " : "") + r[i];
    return s;
}
}  // namespace

bool writeTransactions(const std::string& path, const std::vector<Transaction>& tx) {
    std::ofstream f(path);
    if (!f) return false;
    f << "tx_id,account_id,receiver_id,timestamp,amount,merchant,is_injected_anomaly,anomaly_type\n";
    char buf[32];
    for (const auto& t : tx) {
        std::snprintf(buf, sizeof buf, "%.2f", t.amount);
        f << t.id << ',' << t.sender << ',' << t.receiver << ',' << formatTime(t.ts) << ',' << buf << ','
          << t.merchant << ',' << (t.injected ? 1 : 0) << ',' << t.anomalyType << '\n';
    }
    return static_cast<bool>(f);
}

bool readTransactions(const std::string& path, std::vector<Transaction>& tx) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    if (!std::getline(f, line)) return false;
    const auto header = splitCsv(line);
    std::unordered_map<std::string, size_t> col;
    for (size_t i = 0; i < header.size(); ++i) col[header[i]] = i;
    for (const char* need : {"account_id", "timestamp", "amount"})
        if (!col.count(need)) return false;
    auto get = [&](const std::vector<std::string>& v, const char* name) -> std::string {
        auto it = col.find(name);
        return it != col.end() && it->second < v.size() ? v[it->second] : std::string();
    };
    tx.clear();
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        const auto v = splitCsv(line);
        Transaction t;
        t.id = get(v, "tx_id");
        t.sender = get(v, "account_id");
        t.receiver = get(v, "receiver_id");
        t.merchant = get(v, "merchant");
        t.anomalyType = get(v, "anomaly_type");
        t.injected = get(v, "is_injected_anomaly") == "1";
        if (!parseTime(get(v, "timestamp"), t.ts)) continue;
        t.amount = std::atof(get(v, "amount").c_str());
        tx.push_back(std::move(t));
    }
    std::stable_sort(tx.begin(), tx.end(), [](const Transaction& a, const Transaction& b) { return a.ts < b.ts; });
    return true;
}

bool writeScored(const std::string& path, const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double alertThreshold) {
    std::ofstream f(path);
    if (!f) return false;
    f << "tx_id,account_id,receiver_id,timestamp,amount,rule_score,graph_score,ml_score,risk_score,risk_level,is_alert,reasons,is_injected_anomaly,anomaly_type\n";
    char buf[128];
    for (size_t i = 0; i < tx.size(); ++i) {
        const auto& t = tx[i];
        const auto& s = sig[i];
        std::snprintf(buf, sizeof buf, "%.2f,%.3f,%.3f,%.3f,%.3f", t.amount, s.ruleScore(), s.graphScore(), s.ml, s.risk);
        f << t.id << ',' << t.sender << ',' << t.receiver << ',' << formatTime(t.ts) << ',' << buf << ',' << s.level << ','
          << (s.risk >= alertThreshold ? 1 : 0) << ',' << joinReasons(s.reasons) << ',' << (t.injected ? 1 : 0) << ',' << t.anomalyType << '\n';
    }
    return static_cast<bool>(f);
}

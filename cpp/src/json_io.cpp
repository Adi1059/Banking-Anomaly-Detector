#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "io.hpp"
#include "minijson.hpp"
#include "timeutil.hpp"

namespace {
bool endsWith(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() && std::equal(suf.rbegin(), suf.rend(), s.rbegin(),
        [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
}
std::string num(double v, int digits) { char b[40]; std::snprintf(b, sizeof b, "%.*f", digits, v); return b; }
}  // namespace

bool writeTransactionsJson(const std::string& path, const std::vector<Transaction>& tx) {
    std::ofstream f(path);
    if (!f) return false;
    f << "[\n";
    for (size_t i = 0; i < tx.size(); ++i) {
        const auto& t = tx[i];
        f << "  {\"tx_id\": \"" << minijson::escape(t.id) << "\", \"account_id\": \"" << minijson::escape(t.sender)
          << "\", \"receiver_id\": \"" << minijson::escape(t.receiver) << "\", \"timestamp\": \"" << formatTime(t.ts)
          << "\", \"amount\": " << num(t.amount, 2) << ", \"merchant\": \"" << minijson::escape(t.merchant)
          << "\", \"is_injected_anomaly\": " << (t.injected ? 1 : 0) << ", \"anomaly_type\": \"" << minijson::escape(t.anomalyType) << "\"}"
          << (i + 1 < tx.size() ? ",\n" : "\n");
    }
    f << "]\n";
    return static_cast<bool>(f);
}

bool readTransactionsJson(const std::string& path, std::vector<Transaction>& tx) {
    std::ifstream f(path);
    if (!f) return false;
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<minijson::Object> rows;
    if (!minijson::Parser(text).parse(rows)) return false;
    tx.clear();
    for (const auto& r : rows) {
        auto get = [&](const char* k) { auto it = r.find(k); return it == r.end() ? std::string() : it->second; };
        if (get("account_id").empty() || get("amount").empty()) continue;
        Transaction t;
        t.id = get("tx_id"); t.sender = get("account_id"); t.receiver = get("receiver_id");
        t.merchant = get("merchant"); t.anomalyType = get("anomaly_type");
        t.injected = get("is_injected_anomaly") == "1" || get("is_injected_anomaly") == "true";
        if (!parseTime(get("timestamp"), t.ts)) continue;
        t.amount = std::atof(get("amount").c_str());
        tx.push_back(std::move(t));
    }
    std::stable_sort(tx.begin(), tx.end(), [](const Transaction& a, const Transaction& b) { return a.ts < b.ts; });
    return true;
}

bool writeScoredJson(const std::string& path, const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double alertThreshold) {
    std::ofstream f(path);
    if (!f) return false;
    f << "[\n";
    for (size_t i = 0; i < tx.size(); ++i) {
        const auto& t = tx[i]; const auto& s = sig[i];
        f << "  {\"tx_id\": \"" << t.id << "\", \"account_id\": \"" << t.sender << "\", \"receiver_id\": \"" << t.receiver
          << "\", \"timestamp\": \"" << formatTime(t.ts) << "\", \"amount\": " << num(t.amount, 2)
          << ", \"rule_score\": " << num(s.ruleScore(), 3) << ", \"graph_score\": " << num(s.graphScore(), 3)
          << ", \"ml_score\": " << num(s.ml, 3) << ", \"risk_score\": " << num(s.risk, 3)
          << ", \"risk_level\": \"" << s.level << "\", \"is_alert\": " << (s.risk >= alertThreshold ? "true" : "false")
          << ", \"reasons\": [";
        for (size_t r = 0; r < s.reasons.size(); ++r) f << (r ? ", " : "") << '"' << minijson::escape(s.reasons[r]) << '"';
        f << "]}" << (i + 1 < tx.size() ? ",\n" : "\n");
    }
    f << "]\n";
    return static_cast<bool>(f);
}

bool loadAny(const std::string& p, std::vector<Transaction>& tx) { return endsWith(p, ".json") ? readTransactionsJson(p, tx) : readTransactions(p, tx); }
bool saveAny(const std::string& p, const std::vector<Transaction>& tx) { return endsWith(p, ".json") ? writeTransactionsJson(p, tx) : writeTransactions(p, tx); }
bool saveScoredAny(const std::string& p, const std::vector<Transaction>& tx, const std::vector<Signals>& s, double a) {
    return endsWith(p, ".json") ? writeScoredJson(p, tx, s, a) : writeScored(p, tx, s, a);
}

// ============================================================
// io.cpp  –  CSV load / save
// ============================================================

#include "io.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool loadCSV(const std::string& path, std::vector<Transaction>& out) {
    std::ifstream in(path);
    if (!in) return false;

    std::string line;
    // skip header if present
    if (std::getline(in, line)) {
        if (line.find("id") == std::string::npos) {
            // first line was data – parse it
            std::istringstream ss(line);
            Transaction t;
            char comma;
            std::getline(ss, t.id, ',');
            std::getline(ss, t.sender, ',');
            std::getline(ss, t.receiver, ',');
            ss >> t.amount >> comma >> t.timestamp;
            out.push_back(t);
        }
    }

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        Transaction t;
        char comma;
        std::getline(ss, t.id, ',');
        std::getline(ss, t.sender, ',');
        std::getline(ss, t.receiver, ',');
        ss >> t.amount >> comma >> t.timestamp;
        out.push_back(t);
    }
    return !out.empty();
}

bool saveScoredCSV(const std::string& path,
                   const std::vector<Transaction>& tx,
                   const std::vector<Signals>& signals) {
    std::ofstream out(path);
    if (!out) return false;

    out << "id,sender,receiver,amount,timestamp,risk,level,reasons\n";
    for (size_t i = 0; i < tx.size(); ++i) {
        out << tx[i].id << ","
            << tx[i].sender << ","
            << tx[i].receiver << ","
            << tx[i].amount << ","
            << tx[i].timestamp << ","
            << signals[i].risk << ","
            << signals[i].level << ",\"";

        for (size_t r = 0; r < signals[i].reasons.size(); ++r) {
            if (r) out << "; ";
            out << signals[i].reasons[r];
        }
        out << "\"\n";
    }
    return true;
}

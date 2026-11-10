#pragma once
#include <string>
#include <vector>
#include "types.hpp"

// CSV (columns: tx_id, account_id, receiver_id, timestamp, amount, merchant, is_injected_anomaly, anomaly_type)
bool writeTransactions(const std::string& path, const std::vector<Transaction>& tx);
bool readTransactions(const std::string& path, std::vector<Transaction>& tx);
bool writeScored(const std::string& path, const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double alertThreshold);

// JSON (array of objects with the same field names as the CSV columns)
bool writeTransactionsJson(const std::string& path, const std::vector<Transaction>& tx);
bool readTransactionsJson(const std::string& path, std::vector<Transaction>& tx);
bool writeScoredJson(const std::string& path, const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double alertThreshold);

// Format chosen from the file extension (.json -> JSON, anything else -> CSV)
bool loadAny(const std::string& path, std::vector<Transaction>& tx);
bool saveAny(const std::string& path, const std::vector<Transaction>& tx);
bool saveScoredAny(const std::string& path, const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double alertThreshold);

// MySQL: INSERT script for sql/schema.sql (tables accounts, transactions, alerts)
bool writeSqlScript(const std::string& path, const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double alertThreshold);

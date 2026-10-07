#pragma once
#include <string>
#include <vector>
#include "types.hpp"

bool writeTransactions(const std::string& path, const std::vector<Transaction>& tx);
bool readTransactions(const std::string& path, std::vector<Transaction>& tx);
bool writeScored(const std::string& path, const std::vector<Transaction>& tx, const std::vector<Signals>& sig, double alertThreshold);

#pragma once
// ============================================================
// io.hpp  –  Simple CSV loading and saving
// ============================================================

#include <string>
#include <vector>
#include "types.hpp"

// Load transactions from a CSV file
// Expected columns: id,sender,receiver,amount,timestamp
bool loadCSV(const std::string& path, std::vector<Transaction>& out);

// Save scored results
bool saveScoredCSV(const std::string& path,
                   const std::vector<Transaction>& tx,
                   const std::vector<Signals>& signals);

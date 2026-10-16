// ============================================================
// generator.cpp  –  Synthetic data with injected anomalies
// ============================================================

#include "generator.hpp"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

std::vector<Transaction> TransactionGenerator::generate(int n) const {
    std::vector<Transaction> tx;
    tx.reserve(n);

    std::mt19937 rng(42);  // fixed seed for reproducibility
    std::uniform_real_distribution<double> amountDist(50.0, 2500.0);
    std::uniform_int_distribution<int> accountDist(1, 120);
    std::uniform_int_distribution<int> hourDist(0, 23);
    std::uniform_int_distribution<int> delayDist(30, 3600);

    // Start from a fixed point in time
    int64_t baseTs = 1700000000;  // roughly Nov 2023

    auto makeId = [](int i) {
        std::ostringstream oss;
        oss << "TX" << std::setw(6) << std::setfill('0') << i;
        return oss.str();
    };

    auto acc = [&](int id) {
        std::ostringstream oss;
        oss << "ACC" << std::setw(3) << std::setfill('0') << id;
        return oss.str();
    };

    int injected = 0;
    const int targetInjected = n * 5 / 100;  // ~5%

    for (int i = 0; i < n; ++i) {
        Transaction t;
        t.id = makeId(i + 1);
        t.sender   = acc(accountDist(rng));
        t.receiver = acc(accountDist(rng));
        while (t.receiver == t.sender) t.receiver = acc(accountDist(rng));

        t.amount    = amountDist(rng);
        t.timestamp = baseTs + i * delayDist(rng);
        t.injected  = false;

        // ----- Inject anomalies -----
        if (injected < targetInjected && (rng() % 20 == 0)) {
            int type = rng() % 6;
            t.injected = true;
            injected++;

            switch (type) {
                case 0: // amount spike
                    t.amount = 8000 + (rng() % 5000);
                    t.anomalyType = "spike";
                    break;
                case 1: // odd hour
                    t.timestamp = baseTs + (rng() % 100000) * 3600;
                    // force hour 2
                    t.timestamp = (t.timestamp / 86400) * 86400 + 2 * 3600;
                    t.anomalyType = "odd_hour";
                    break;
                case 2: // high velocity (will be reinforced by consecutive txs)
                    t.anomalyType = "velocity";
                    break;
                case 3: // structuring
                    t.amount = 9200 + (rng() % 700);
                    t.anomalyType = "structuring";
                    break;
                case 4: // new payee large
                    t.amount = 6000 + (rng() % 3000);
                    t.anomalyType = "new_payee";
                    break;
                default: // repeated
                    t.amount = 499.99;
                    t.anomalyType = "repeated";
                    break;
            }
        }

        tx.push_back(t);
    }

    // Add a few clear cycles for the graph detector
    if (n > 50) {
        for (int c = 0; c < 5; ++c) {
            int base = 100 + c * 10;
            if (base + 2 >= n) break;
            std::string a = acc(200 + c);
            std::string b = acc(201 + c);
            std::string d = acc(202 + c);  // 'c' already used

            tx[base].sender = a; tx[base].receiver = b; tx[base].amount = 1500;
            tx[base].injected = true; tx[base].anomalyType = "cycle";

            tx[base+1].sender = b; tx[base+1].receiver = d; tx[base+1].amount = 1480;
            tx[base+1].timestamp = tx[base].timestamp + 300;
            tx[base+1].injected = true; tx[base+1].anomalyType = "cycle";

            tx[base+2].sender = d; tx[base+2].receiver = a; tx[base+2].amount = 1450;
            tx[base+2].timestamp = tx[base].timestamp + 600;
            tx[base+2].injected = true; tx[base+2].anomalyType = "cycle";
        }
    }

    return tx;
}

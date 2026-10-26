#pragma once
// The domain owns its objects. Bytecode sees only schema type names and pointers.
// All handles returned below remain valid until this Host is destroyed.
#include "dslvm.hpp"
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sample {
struct Money { int64_t cents; };
struct Items { std::vector<Money*> values; };
struct Shop { Items items; };
struct Ledger { int64_t balance = 0, entries = 0; std::string note; };

class Host {
public:
    dslvm::Registry registry; // Installs string/math/collections/file std by default.
    Shop shop;
    Ledger ledger;

    Host();
    dslvm::Context context() {
        dslvm::Context context;
        context.set("shop", &shop);
        context.set("ledger", &ledger);
        return context;
    }

private:
    std::vector<std::unique_ptr<Money>> arena;
    dslvm::Value money(int64_t cents) {
        arena.push_back(std::make_unique<Money>(Money{cents}));
        return dslvm::Value::object("Money", arena.back().get());
    }
    dslvm::Value units(double amount) {
        // Never cast a non-finite or out-of-range double to an integer.
        const double cents = std::round(amount * 100.0);
        if (!std::isfinite(cents) || cents < -9223372036854775808.0 || cents >= 9223372036854775808.0)
            throw dslvm::Error("money: amount out of range");
        return money(static_cast<int64_t>(cents));
    }
    static int64_t add(int64_t a, int64_t b) {
        return dslvm::from_twos_complement(uint64_t(a) + uint64_t(b));
    }
};
} // namespace sample

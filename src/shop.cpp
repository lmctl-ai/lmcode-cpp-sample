#include "shop.hpp"
#include "testing.hpp"
#include <cstdlib>

namespace sample {
Host::Host() {
    using dslvm::Args;
    using dslvm::Value;
    // Schema context names must match exactly; context() supplies live instances.
    registry.context("shop", "Shop").context("ledger", "Ledger");
    register_conversions(registry);
    dslvm::stdlib::register_testing(registry); // Explicit opt-in: print/assert_close.
    for (int64_t cents : {125, 250, 375})
        shop.items.values.push_back(static_cast<Money*>(money(cents).obj));

    // Distinct parameter tuples are overloads. int means cents; float means units.
    registry.function("money", {"int"}, "Money", [this](Args& a) {
        return money(a[0].i);
    });
    registry.function("money", {"float"}, "Money", [this](Args& a) {
        return units(a[0].f);
    });
    registry.function("fee", {"int"}, "int", [](Args& a) {
        return Value::integer(add(a[0].i, 5));
    });
    registry.function("sample_tmp", {}, "string", [](Args&) {
        const char* path = std::getenv("SAMPLE_TMP");
        if (!path || !*path) throw dslvm::Error("SAMPLE_TMP is required");
        return Value::string(path);
    });

    // Values are tagged. The loader binds these declared signatures before run().
    registry.method("Money", "amount", {}, "int", [](void* self, Args&) {
        return Value::integer(money_ref(self).cents);
    });
    registry.method("Money", "currency", {}, "Currency", [](void* self, Args&) {
        (void)money_ref(self);
        return Value::string("USD"); // Enums are string-backed at the host boundary.
    });
    registry.method("Money", "plus", {"Money"}, "Money", [this](void* self, Args& a) {
        return money(add(money_ref(self).cents, money_ref(a[0].obj).cents));
    });
    registry.method("Money", "minus", {"Money"}, "Money", [this](void* self, Args& a) {
        const auto left = uint64_t(money_ref(self).cents);
        const auto right = uint64_t(money_ref(a[0].obj).cents);
        return money(dslvm::from_twos_complement(left - right));
    });
    registry.method("Money", "times", {"int"}, "Money", [this](void* self, Args& a) {
        return money(dslvm::from_twos_complement(uint64_t(money_ref(self).cents) * uint64_t(a[0].i)));
    });
    registry.method("Money", "neg", {}, "Money", [this](void* self, Args&) {
        return money(dslvm::from_twos_complement(uint64_t(0) - uint64_t(money_ref(self).cents)));
    });
    registry.method("Money", "set", {"int"}, "Money", [](void* self, Args& a) {
        money_ref(self).cents = a[0].i;
        return Value::object("Money", self); // Schema returns:self normalizes to Money.
    });

    // A host collection needs len()/at(int). foreach snapshots len exactly once.
    registry.method("Items", "len", {}, "int", [](void* self, Args&) {
        return Value::integer(static_cast<int64_t>(static_cast<Items*>(self)->values.size()));
    });
    registry.method("Items", "at", {"int"}, "Money", [](void* self, Args& a) {
        auto& values = static_cast<Items*>(self)->values;
        if (a[0].i < 0 || uint64_t(a[0].i) >= values.size())
            throw dslvm::Error("items: index out of range");
        return Value::object("Money", values[static_cast<size_t>(a[0].i)]);
    });
    registry.method("Shop", "items", {}, "Items", [](void* self, Args&) {
        return Value::object("Items", &static_cast<Shop*>(self)->items);
    });
    registry.method("Shop", "total", {}, "Money", [this](void* self, Args&) {
        int64_t total = 0;
        for (auto* item : static_cast<Shop*>(self)->items.values) total = add(total, item->cents);
        return money(total);
    });
    registry.method("Shop", "price", {"int"}, "Money", [this](void*, Args& a) {
        return money(a[0].i);
    });
    registry.method("Shop", "price", {"float"}, "Money", [this](void*, Args& a) {
        return units(a[0].f);
    });
    registry.method("Shop", "currency", {}, "Currency", [](void*, Args&) {
        return Value::string("USD");
    });
    registry.method("Shop", "label", {}, "string", [](void*, Args&) {
        return Value::string("sample shop");
    });

    // Void, fluent mutation, and overloads can coexist on a single receiver.
    registry.method("Ledger", "record", {"Money"}, "Ledger", [](void* self, Args& a) {
        auto& ledger = *static_cast<Ledger*>(self);
        ledger.balance = add(ledger.balance, money_ref(a[0].obj).cents);
        ledger.entries = add(ledger.entries, 1);
        return Value::object("Ledger", self);
    });
    registry.method("Ledger", "balance", {}, "Money", [this](void* self, Args&) {
        return money(static_cast<Ledger*>(self)->balance);
    });
    registry.method("Ledger", "count", {}, "int", [](void* self, Args&) {
        return Value::integer(static_cast<Ledger*>(self)->entries);
    });
    registry.method("Ledger", "note", {"string"}, "void", [](void* self, Args& a) {
        static_cast<Ledger*>(self)->note = a[0].s;
        return Value::none();
    });
    registry.method("Ledger", "note", {}, "string", [](void* self, Args&) {
        return Value::string(static_cast<Ledger*>(self)->note);
    });
    registry.method("Ledger", "reject", {"string"}, "void", [](void*, Args& a) -> Value {
        throw dslvm::Error(a[0].s); // VM attaches the failing DSL call's line/column.
    });
}
} // namespace sample
